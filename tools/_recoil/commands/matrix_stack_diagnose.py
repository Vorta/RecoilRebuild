"""Survey retail BN matrix-stack callers against an existing canonical build.

This is triage, not acceptance: other callees are explicitly assumed locally
balanced; correlated predicates and indirect transfers may remain unresolved.
It never compiles, deploys, mutates BN, or updates reconstruction evidence.
"""
from __future__ import annotations

import argparse
from dataclasses import replace
import json
import re
import shutil
import subprocess
from pathlib import Path

from _recoil.commands.asm_verify import CoffObject, parse_assembly
from _recoil.commands.startup_contract import image_bytes, parse_image_listing
from _recoil.commands.vc5_build import load_config, build_paths, parse_link_map, object_path
from _recoil.lib.binja import BinaryNinjaBridge
from _recoil.lib.path_contract import path_depths
from _recoil.lib.tooling import REPO_ROOT, run_cmd_script, quote_cmd_arg


def local_depth_result(rows, data, start, primitives):
    effects = {}
    offset = 0
    for raw, _ in rows:
        if raw[0] == 0xe8 and len(raw) == 5:
            target = start + offset + 5 + int.from_bytes(raw[1:], "little", signed=True)
            effects[offset] = primitives.get(target, 0)
        elif raw[0] == 0xff and len(raw) > 1 and (raw[1] >> 3 & 7) == 2:
            effects[offset] = 0
        offset += len(raw)
    try:
        return {"resolved": True, **path_depths(rows, data, effects)}
    except ValueError as error:
        return {"resolved": False, "reason": str(error)}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args(argv)
    root = args.output_dir.resolve()
    if root.exists() or not root.is_relative_to((REPO_ROOT / "build/diagnostics").resolve()):
        parser.error("output directory must be fresh below build/diagnostics")
    config = replace(load_config(), build_dir=args.build_dir.resolve())
    paths = build_paths(config)
    linked_map = parse_link_map(paths.map_path)
    candidate = paths.exe_path.read_bytes()
    retail = (REPO_ROOT / "support/Recoil.exe").read_bytes()
    root.mkdir(parents=True)
    dumped = run_cmd_script(
        f"call {quote_cmd_arg(config.vc5_env)} && dumpbin /nologo /disasm {quote_cmd_arg(paths.exe_path)}",
        cwd=root, capture_output=True,
    )
    if dumped.returncode:
        raise ValueError("candidate disassembly failed")
    bridge = BinaryNinjaBridge(binary="Recoil.bndb", call_budget=256)
    primitives = {0x472ef0: 1, 0x472f30: 1, 0x472f60: -1}
    names = set()
    for address in primitives:
        refs = bridge.get_json("getXrefsTo", address=hex(address))
        names.update(row["function"] for row in refs["code_references"])
    candidate_primitives = {}
    for symbol, effect in (("?MatStackPushAndCloneParent@zMath@@YIXPAM@Z", 1),
                           ("?MatStackPushPtr@zMath@@YIXPAM@Z", 1),
                           ("?MatStackPopPtr@zMath@@YAXXZ", -1)):
        found = {row.address for row in linked_map.symbols if row.symbol == symbol and row.is_function}
        if len(found) != 1:
            raise ValueError("ambiguous candidate stack primitive")
        candidate_primitives[found.pop()] = effect
    identities = {}
    def collect(target, inherited_source=""):
        source = target.get("source_from", inherited_source)
        if target.get("address") and (target.get("symbol") or target.get("symbol_regex")) and source:
            identities.setdefault(int(target["address"], 16), set()).add(
                (source, target.get("symbol", ""), target.get("symbol_regex", "")))
        for key in ("functions", "translation_unit_function_order"):
            for row in target.get(key, []):
                collect(row, source)
    for manifest in (REPO_ROOT / "tools/vc5_verify_targets").glob("*.json"):
        target = json.loads(manifest.read_text(encoding="utf-8"))
        collect(target)
    objects = {}
    report = {"kind": "matrix-stack-survey", "accepts_reconstruction": False,
              "assumption": "nonprimitive calls have zero net matrix-stack effect; branch predicates are unconstrained",
              "build_dir": str(paths.build_dir), "rows": []}
    for name in sorted(names):
        result = {"name": name}
        try:
            assembly = bridge.assembly(name)
            instructions = parse_assembly(assembly, source="bn")
            if not instructions:
                raise ValueError("empty BN function")
            start = int(instructions[0].source_line.split()[0], 16)
            result["address"] = hex(start)
            rows = [(bytes.fromhex(" ".join(row.bytes)), row.raw_text.split()[0]) for row in instructions]
            data = b"".join(row[0] for row in rows)
            if data != image_bytes(retail, start, len(data)):
                raise ValueError("BN listing disagrees with immutable retail")
            result["retail"] = local_depth_result(rows, data, start, primitives)
            matches = {}
            for source, symbol, pattern in identities.get(start, ()):
                obj_path = object_path(config, paths, REPO_ROOT / source)
                if not obj_path.is_file():
                    continue
                if obj_path not in objects:
                    objects[obj_path] = CoffObject.from_path(obj_path)
                obj = objects[obj_path]
                symbols = [symbol] if symbol else [s for s in obj.symbols_by_name if re.fullmatch(pattern, s)]
                for selected in symbols:
                    if selected not in obj.symbols_by_name:
                        continue
                    found = {row.address for row in linked_map.symbols if row.symbol == selected and row.is_function}
                    if len(found) == 1:
                        matches[(found.pop(), selected)] = obj.function_bytes(selected)
            if len(matches) != 1:
                raise ValueError(f"expected one manifest/object/map body, found {len(matches)}")
            (address, symbol), body = next(iter(matches.items()))
            data = image_bytes(candidate, address, len(body.data))
            if any(a != b and not body.relocation_mask[i] for i, (a, b) in enumerate(zip(data, body.data))):
                raise ValueError("candidate linked body disagrees with object")
            result["candidate_symbol"] = symbol
            result["candidate_address"] = hex(address)
            result["candidate_size"] = len(data)
            try:
                rows = parse_image_listing(dumped.stdout, address, len(data))
            except ValueError:
                # Whole-section DUMPBIN can lose synchronization after inline
                # data. Restart decoding at the proven COFF/MAP body boundary.
                objdump = shutil.which("llvm-objdump.exe")
                if objdump is None:
                    raise ValueError("bounded redisassembly requires llvm-objdump on PATH")
                decoded = subprocess.run([objdump, "-d", "--x86-asm-syntax=intel",
                                          f"--start-address={address:#x}",
                                          f"--stop-address={address + len(data):#x}",
                                          str(paths.exe_path)], capture_output=True, text=True, check=True)
                rows = parse_image_listing(decoded.stdout, address, len(data))
            result["candidate"] = local_depth_result(rows, data, address, candidate_primitives)
            result["status"] = ("same-local-depth" if result["retail"] == result["candidate"]
                                and result["retail"]["resolved"] else "inspect")
        except (OSError, ValueError, RuntimeError) as error:
            result["status"] = "unresolved"
            result["error"] = str(error)
        report["rows"].append(result)
        print(f"{result['status']}: {name}", flush=True)
    (root / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"total": len(report["rows"]), "counts": {
        status: sum(row["status"] == status for row in report["rows"])
        for status in ("same-local-depth", "inspect", "unresolved")}, "report": str(root / "report.json")}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
