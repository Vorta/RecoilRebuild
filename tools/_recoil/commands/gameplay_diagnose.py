"""Bounded external Windows x86 process diagnosis; never reconstruction acceptance.

Win32 declarations follow Microsoft's DEBUG_EVENT and WOW64_CONTEXT headers.
The debugger observes an explicitly launched process; it injects no game code,
sets no breakpoints, and never attaches to or terminates another process.
"""
from __future__ import annotations

import argparse
from bisect import bisect_right
import ctypes as C
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import time

from _recoil.lib.pe import parse_pe_headers
from _recoil.lib.tooling import REPO_ROOT

D = C.c_uint32
P = C.c_void_p


class ExceptionRecord(C.Structure):
    _fields_ = [("code", D), ("flags", D), ("record", P), ("address", P),
                ("count", D), ("parameters", C.c_size_t * 15)]


class ExceptionInfo(C.Structure):
    _fields_ = [("record", ExceptionRecord), ("first", D)]


class CreateProcessInfo(C.Structure):
    _fields_ = [("file", P), ("process", P), ("thread", P), ("base", P),
                ("debug_offset", D), ("debug_size", D), ("tls", P),
                ("start", P), ("name", P), ("unicode", C.c_uint16)]


class CreateThreadInfo(C.Structure):
    _fields_ = [("thread", P), ("tls", P), ("start", P)]


class LoadDllInfo(C.Structure):
    _fields_ = [("file", P), ("base", P), ("debug_offset", D),
                ("debug_size", D), ("name", P), ("unicode", C.c_uint16)]


class EventUnion(C.Union):
    _fields_ = [("exception", ExceptionInfo), ("process", CreateProcessInfo),
                ("thread", CreateThreadInfo), ("dll", LoadDllInfo),
                ("exit_code", D), ("unload_base", P)]


class DebugEvent(C.Structure):
    _fields_ = [("code", D), ("pid", D), ("tid", D), ("data", EventUnion)]


class FloatSave(C.Structure):
    _fields_ = [("words", D * 7), ("registers", C.c_ubyte * 80), ("cr0", D)]


class X86Context(C.Structure):
    _fields_ = [("ContextFlags", D), ("debug", D * 6), ("float", FloatSave)] + [
        (name, D) for name in ("SegGs", "SegFs", "SegEs", "SegDs", "Edi",
                               "Esi", "Ebx", "Edx", "Ecx", "Eax", "Ebp",
                               "Eip", "SegCs", "EFlags", "Esp", "SegSs")
    ] + [("extended", C.c_ubyte * 512)]


def read_map(path: Path) -> tuple[list[tuple[int, str]], dict[str, int]]:
    code, symbols = [], {}
    ambiguous = set()
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        match = re.match(r"\s+([0-9a-fA-F]{4}):[0-9a-fA-F]+\s+(\S+)\s+([0-9a-fA-F]{8})\s+(.*)", line)
        if match:
            segment, name, address, tail = match.groups()
            address = int(address, 16)
            if name in symbols and symbols[name] != address:
                ambiguous.add(name)
            if name not in ambiguous:
                symbols[name] = address
            else:
                symbols.pop(name, None)
            if segment == "0001" and re.match(r"f\s", tail):
                code.append((address, name + " [" + tail.split()[-1] + "]"))
    if not code:
        raise ValueError("map has no linked x86 function symbols")
    return sorted(set(code)), symbols


def resolve_watch(symbols: dict[str, int], requested: list[str]) -> dict[str, int]:
    watches = {}
    for name in requested:
        matches = [(key, value) for key, value in symbols.items() if key == name]
        if len(matches) != 1:
            raise ValueError(f"watch requires an exact unambiguous map symbol: {name}")
        watches.update(matches)
    return watches


def exception_disposition(code: int, address: int, first: bool, image_base: int,
                          image_size: int, initial_breaks: set[int]) -> int:
    """Consume first loader notifications only; preserve application exceptions."""
    if (first and code in (0x80000003, 0x4000001f) and code not in initial_breaks
            and not image_base <= address < image_base + image_size):
        initial_breaks.add(code)
        return 0x10002
    return 0x80010001


def capture_exception_detail(code: int, first: bool, occurrence: int) -> bool:
    """Retain fatal/fault evidence, bound repeated handled guard-page snapshots."""
    return not first or code not in (0x80000001, 0x406D1388) or occurrence == 1


class WindowsDebugger:
    def __init__(self, code: list[tuple[int, str]], watches: dict[str, int], image):
        self.kernel = C.WinDLL("kernel32", use_last_error=True)
        declarations = {
            "WaitForDebugEvent": ([C.POINTER(DebugEvent), D], C.c_int),
            "ContinueDebugEvent": ([D, D, D], C.c_int),
            "ReadProcessMemory": ([P, P, P, C.c_size_t, C.POINTER(C.c_size_t)], C.c_int),
            "Wow64GetThreadContext": ([P, C.POINTER(X86Context)], C.c_int),
            "SuspendThread": ([P], D), "ResumeThread": ([P], D),
            "CloseHandle": ([P], C.c_int),
            "GetFinalPathNameByHandleW": ([P, C.c_wchar_p, D, D], D),
        }
        for name, (args, result) in declarations.items():
            fn = getattr(self.kernel, name)
            fn.argtypes, fn.restype = args, result
        self.code, self.watches, self.image = code, watches, image
        self.addresses = [address for address, _ in code]
        self.threads: dict[int, int] = {}
        self.modules: dict[int, str] = {}
        self.process = None
        self.slide = 0
        self.initial_breaks: set[int] = set()

    def read(self, address: int, size: int) -> bytes:
        buf, count = C.create_string_buffer(size), C.c_size_t()
        if not self.kernel.ReadProcessMemory(self.process, address, buf, size, C.byref(count)):
            return buf.raw[:count.value]
        return buf.raw[:count.value]

    def location(self, address: int) -> dict[str, object]:
        adjusted = address - self.slide
        index = bisect_right(self.addresses, adjusted) - 1
        in_text = any(s.characteristics & 0x20000000 and
                      self.image.image_base + s.virtual_address <= adjusted <
                      self.image.image_base + s.virtual_address + s.virtual_size
                      for s in self.image.sections)
        if index >= 0 and in_text:
            base, name = self.code[index]
            return {"address": hex(address), "nearest_map_symbol": name,
                    "offset": hex(adjusted - base), "exact_frame": False}
        bases = [base for base in self.modules if base <= address]
        base = max(bases) if bases else None
        return {"address": hex(address), "nearest_module": self.modules.get(base),
                "module_offset": hex(address - base) if base is not None else None,
                "exact_frame": False}

    def path_and_close(self, handle: int | None) -> str | None:
        if not handle:
            return None
        try:
            buf = C.create_unicode_buffer(32768)
            n = self.kernel.GetFinalPathNameByHandleW(handle, buf, len(buf), 0)
            return buf.value if 0 < n < len(buf) else None
        finally:
            self.kernel.CloseHandle(handle)

    def snapshot(self, *, stopped: bool) -> dict[str, object]:
        rows = []
        for tid, handle in list(self.threads.items()):
            suspended = False
            try:
                if not stopped:
                    if self.kernel.SuspendThread(handle) == 0xffffffff:
                        raise OSError(C.get_last_error(), "SuspendThread")
                    suspended = True
                context = X86Context()
                context.ContextFlags = 0x10007
                if not self.kernel.Wow64GetThreadContext(handle, C.byref(context)):
                    raise OSError(C.get_last_error(), "Wow64GetThreadContext")
                registers = {name: hex(getattr(context, name)) for name in
                             ("Eip", "Esp", "Ebp", "Eax", "Ebx", "Ecx", "Edx", "Esi", "Edi", "EFlags")}
                stack = self.read(context.Esp, 1024)
                candidates = []
                for offset in range(0, len(stack) - 3, 4):
                    value = struct.unpack_from("<I", stack, offset)[0]
                    location = self.location(value)
                    if "nearest_map_symbol" in location:
                        candidates.append({"stack_offset": hex(offset), **location})
                rows.append({"tid": tid, "registers": registers,
                             "instruction": self.location(context.Eip),
                             "instruction_bytes": self.read(context.Eip, 32).hex(),
                             "stack_hex": stack.hex(), "stack_candidates": candidates,
                             "register_memory": {name: self.read(getattr(context, name), 64).hex()
                                                 for name in ("Eax", "Ebx", "Ecx", "Edx", "Esi", "Edi")}})
            except OSError as error:
                rows.append({"tid": tid, "error": str(error)})
            finally:
                if suspended:
                    if self.kernel.ResumeThread(handle) == 0xffffffff:
                        raise OSError(C.get_last_error(), "ResumeThread")
        values = {name: self.read(address + self.slide, 4).hex()
                  for name, address in self.watches.items()}
        return {"threads": rows, "watch_bytes": values,
                "simultaneous": stopped, "stack_candidates_are_not_unwound_frames": True}


def run(args) -> int:
    if os.name != "nt" or C.sizeof(P) != 8:
        raise ValueError("requires 64-bit Windows Python diagnosing an x86 executable")
    exe, map_path, root = args.exe.resolve(), args.map.resolve(), args.output_dir.resolve()
    if root.exists() or not root.is_relative_to((REPO_ROOT / "build").resolve()):
        raise ValueError("output directory must be absent and below workspace build/")
    image = parse_pe_headers(exe.read_bytes(), source=str(exe))
    if image.machine != 0x14c:
        raise ValueError("only Windows x86 PE executables are supported")
    code, symbols = read_map(map_path)
    watches = resolve_watch(symbols, args.watch_symbol)
    if any(not image.image_base <= address < image.image_base + image.size_of_image - 3
           for address in watches.values()):
        raise ValueError("map watch is outside the selected executable image")
    debugger = WindowsDebugger(code, watches, image)
    root.mkdir(parents=True)
    log_paths = [exe.parent / "recoil.out", exe.parent / "recoil.err"]
    for path in log_paths:
        if path.is_file():
            shutil.copy2(path, root / (path.name + ".before"))
    # The process runs with its existing assets and settings; no UI automation.
    started = time.monotonic()
    process = subprocess.Popen([str(exe)], cwd=exe.parent, creationflags=0x2)
    exit_code, timed_out, snapshot_count = None, False, 0
    last_snapshot, burst_remaining = started, 0
    checkpoint, checkpoint_time, captured_checkpoint = "", started, None
    exception_counts = {}
    with (root / "events.jsonl").open("x", encoding="utf-8") as output:
        def emit(kind, **fields):
            row = {"kind": kind, "elapsed": round(time.monotonic() - started, 3), **fields}
            output.write(json.dumps(row) + "\n")
            output.flush()

        emit("launch", pid=process.pid, exe=str(exe), map=str(map_path),
             map_is_diagnostic_only=True, accepts_reconstruction=False)
        print(f"Debugger launched PID {process.pid}; reproduce the menu route now. Output: {root}", flush=True)
        try:
            while True:
                event = DebugEvent()
                available = debugger.kernel.WaitForDebugEvent(C.byref(event), 100)
                if available:
                    status = 0x10002  # DBG_CONTINUE for non-exception events.
                    try:
                        if event.code == 3:
                            info = event.data.process
                            debugger.process = info.process
                            debugger.threads[event.tid] = info.thread
                            debugger.slide = int(info.base) - image.image_base
                            debugger.modules[int(info.base)] = debugger.path_and_close(info.file) or str(exe)
                        elif event.code == 2:
                            debugger.threads[event.tid] = event.data.thread.thread
                        elif event.code == 4:
                            debugger.threads.pop(event.tid, None)
                        elif event.code == 6:
                            info = event.data.dll
                            debugger.modules[int(info.base)] = debugger.path_and_close(info.file) or "unknown"
                        elif event.code == 7:
                            debugger.modules.pop(int(event.data.unload_base), None)
                        elif event.code == 1:
                            info = event.data.exception
                            record = info.record
                            # Only the debugger's initial system breakpoint is consumed.
                            # Application exceptions still reach their normal handlers.
                            status = exception_disposition(
                                record.code, record.address or 0, bool(info.first),
                                image.image_base + debugger.slide, image.size_of_image,
                                debugger.initial_breaks,
                            )
                            if status != 0x10002:
                                key = f"{record.code:#x}@{record.address or 0:#x}:first={bool(info.first)}"
                                exception_counts[key] = exception_counts.get(key, 0) + 1
                                detailed = capture_exception_detail(record.code, bool(info.first), exception_counts[key])
                                emit("exception", tid=event.tid, code=hex(record.code),
                                     first_chance=bool(info.first), address=hex(record.address or 0),
                                     parameters=list(record.parameters)[:min(record.count, 15)],
                                     occurrence=exception_counts[key],
                                     snapshot=debugger.snapshot(stopped=True) if detailed else None)
                                if detailed:
                                    print(f"Exception {record.code:#x} at {record.address:#x}, first={info.first}", flush=True)
                        elif event.code == 5:
                            exit_code = int(event.data.exit_code)
                            emit("exit", code=exit_code)
                    finally:
                        if not debugger.kernel.ContinueDebugEvent(event.pid, event.tid, status):
                            raise OSError(C.get_last_error(), "ContinueDebugEvent")
                    if event.code == 5:
                        break
                elif C.get_last_error() not in (0, 121):
                    raise OSError(C.get_last_error(), "WaitForDebugEvent")
                now = time.monotonic()
                if log_paths[0].exists():
                    lines = log_paths[0].read_text(errors="replace").splitlines()
                    latest = lines[-1] if lines else ""
                    if latest != checkpoint:
                        checkpoint, checkpoint_time = latest, now
                        emit("checkpoint", text=checkpoint)
                if checkpoint and now - checkpoint_time >= 20 and captured_checkpoint != checkpoint:
                    captured_checkpoint, burst_remaining = checkpoint, 3
                    last_snapshot = now - 1
                interval = 1 if burst_remaining else args.snapshot_interval
                if debugger.process and now - last_snapshot >= interval:
                    emit("snapshot", last_checkpoint=checkpoint, snapshot=debugger.snapshot(stopped=False))
                    snapshot_count += 1
                    last_snapshot = now
                    burst_remaining = max(0, burst_remaining - 1)
                if now - started >= args.timeout:
                    timed_out = True
                    emit("timeout", last_checkpoint=checkpoint, snapshot=debugger.snapshot(stopped=False))
                    break
        finally:
            # Popen owns this exact process handle, so PID reuse cannot target another process.
            if process.poll() is None:
                process.terminate()
                cleanup_deadline = time.monotonic() + 5
                while time.monotonic() < cleanup_deadline:
                    pending = DebugEvent()
                    if debugger.kernel.WaitForDebugEvent(C.byref(pending), 100):
                        debugger.kernel.ContinueDebugEvent(pending.pid, pending.tid, 0x10002)
                        if pending.code == 5:
                            break
                    elif process.poll() is not None:
                        break
                process.wait(timeout=10)
            for path in log_paths:
                if path.is_file():
                    shutil.copy2(path, root / path.name)
            summary = {"kind": "gameplay-start-diagnostic", "pid": process.pid,
                       "exit_code": exit_code, "timed_out": timed_out,
                       "snapshots": snapshot_count, "last_checkpoint": checkpoint,
                       "exception_counts": exception_counts,
                       "modules": {hex(k): v for k, v in debugger.modules.items()},
                       "accepts_reconstruction": False}
            (root / "summary.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: value for key, value in summary.items() if key not in {"modules", "exception_counts"}}), flush=True)
    return 1 if timed_out or exit_code != 0 else 0


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--map", type=Path, required=True, help="matching linker map; diagnostic identity only")
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--timeout", type=int, default=300)
    parser.add_argument("--snapshot-interval", type=int, default=5)
    parser.add_argument("--watch-symbol", action="append", default=[])
    args = parser.parse_args(argv)
    if not 1 <= args.timeout <= 300 or not 1 <= args.snapshot_interval <= 30:
        parser.error("timeout must be 1..300 seconds and snapshot interval 1..30 seconds")
    try:
        return run(args)
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        print(f"gameplay diagnosis failed: {error}", flush=True)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
