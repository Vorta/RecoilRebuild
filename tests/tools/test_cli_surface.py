from __future__ import annotations

import json
from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]
TOOLS = ROOT / "tools"
sys.path.insert(0, str(TOOLS))

import recoil  # noqa: E402


RETIRED_ROUTES = {
    "progress status",
    "progress owner find",
    "progress owner relationships",
    "audit call-contract-readiness",
    "audit zinterp",
    "binja data-overlap",
    "verify authored-order scaffold",
    "verify authored-order sweep",
    "verify vc5-abi-equivalence",
    "verify zui-inline-context",
}


def test_registry_is_unique_reachable_and_free_of_retired_routes() -> None:
    names = [item.name for item in recoil.COMMAND_SPECS]
    assert len(names) == len(set(names))
    assert not RETIRED_ROUTES.intersection(names)
    for item in recoil.COMMAND_SPECS:
        assert (TOOLS / "_recoil" / "commands" / f"{item.module}.py").is_file()


def test_machine_readable_command_inventory_matches_registry() -> None:
    completed = subprocess.run(
        [sys.executable, str(TOOLS / "recoil.py"), "commands", "--json"],
        cwd=ROOT,
        capture_output=True,
        text=True,
        check=False,
    )
    assert completed.returncode == 0, completed.stderr
    payload = json.loads(completed.stdout)
    assert {row["command"] for row in payload} == {
        item.name for item in recoil.COMMAND_SPECS
    }


def test_dispatch_preserves_prepend_arguments() -> None:
    item = recoil.COMMANDS[("verify", "vc5-order")]
    command = recoil.build_command(item, ["unit", "--build-root", "scratch"])
    assert command[-4:] == ["--order-only", "unit", "--build-root", "scratch"]


def test_vc5_smoke_is_on_the_retained_verifier_route() -> None:
    item = recoil.COMMANDS[("verify", "vc5")]
    assert "--smoke" in " ".join(item.examples)
    assert ("audit", "source-policy") in recoil.COMMANDS


def test_source_path_relocation_is_one_public_global_cas_route() -> None:
    item = recoil.COMMANDS[("progress", "source-path", "relocate")]
    assert item.prepend_args == ("source-path", "relocate")
    assert item.required_revision_domains == ("global",)
    assert item.mutation_scope == "source-path"


def test_process_diagnostic_map_keeps_local_aliases_but_rejects_ambiguous_watches(tmp_path) -> None:
    import pytest
    from _recoil.commands.gameplay_diagnose import read_map, resolve_watch

    path = tmp_path / "synthetic.map"
    path.write_text(
        " 0001:00000000 _local 00401000 f one.obj\n"
        " 0001:00000010 _local 00401010 f two.obj\n"
        " 0003:00000000 _state 00402000 one.obj\n", encoding="ascii")
    code, symbols = read_map(path)
    assert len(code) == 2 and code[0][1] != code[1][1]
    assert resolve_watch(symbols, ["_state"]) == {"_state": 0x402000}
    for missing in ("_local", "state", "_absent"):
        with pytest.raises(ValueError, match="exact unambiguous"):
            resolve_watch(symbols, [missing])
    path.write_text("not a map", encoding="ascii")
    with pytest.raises(ValueError, match="no linked"):
        read_map(path)


def test_process_diagnostic_never_consumes_faults_or_application_breakpoints() -> None:
    from _recoil.commands.gameplay_diagnose import exception_disposition, capture_exception_detail

    assert capture_exception_detail(0x80000001, True, 1)
    assert not capture_exception_detail(0x80000001, True, 100)
    assert capture_exception_detail(0x80000001, False, 100)
    assert capture_exception_detail(0xc0000005, True, 100)

    seen = set()
    assert exception_disposition(0x80000003, 0x70000000, True, 0x400000, 0x10000, seen) == 0x10002
    for code, address, first in (
        (0x80000003, 0x70000000, True), (0xc0000005, 0x401000, True),
        (0x80000003, 0x401000, True), (0x4000001f, 0x70000000, False),
    ):
        assert exception_disposition(code, address, first, 0x400000, 0x10000, seen) == 0x80010001


def test_process_diagnostic_context_layout_and_resume_on_read_failure() -> None:
    import ctypes
    from types import SimpleNamespace
    from _recoil.commands.gameplay_diagnose import WindowsDebugger, X86Context

    assert ctypes.sizeof(X86Context) == 716
    assert X86Context.Eip.offset == 184 and X86Context.Esp.offset == 196
    resumed = []
    debugger = object.__new__(WindowsDebugger)
    debugger.threads = {7: 99}
    debugger.watches = {}
    debugger.kernel = SimpleNamespace(
        SuspendThread=lambda handle: 0,
        ResumeThread=lambda handle: resumed.append(handle) or 1,
        Wow64GetThreadContext=lambda *args: (_ for _ in ()).throw(OSError("unreadable context")),
    )
    report = debugger.snapshot(stopped=False)
    assert resumed == [99] and "unreadable context" in report["threads"][0]["error"]
    assert report["stack_candidates_are_not_unwound_frames"] is True
