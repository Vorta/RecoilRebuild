"""Fresh pre-launch scalar-initialization regression guard; not a stage.

The selected initializer is interpreted from immutable retail and the current
canonical object and linked image. This is not a complete gameplay/call/byte
proof. In particular, no saved runtime capture supplies expected values.
"""
from __future__ import annotations

import argparse
from dataclasses import replace
import json
import re
from pathlib import Path
import subprocess
import sys

from _recoil.commands.asm_verify import CoffObject
from _recoil.lib.initializer_contract import constant_member_bytes
from _recoil.lib.pe import parse_pe_headers, rva_to_offset
from _recoil.lib.tooling import REPO_ROOT
from _recoil.lib.path_contract import (
    MATRIX_PATH_BODIES, path_depths, image_matrix_effects,
    MATRIX_PRIMITIVES, local_primitive_effects,
)


def disassembled_image_rows(config, paths, image_path, start, size):
    from _recoil.lib.tooling import run_cmd_script, quote_cmd_arg
    result = run_cmd_script(
        f"call {quote_cmd_arg(config.vc5_env)} && dumpbin /nologo /disasm {quote_cmd_arg(image_path)}",
        cwd=paths.build_dir, script_name="_startup_disasm.cmd", capture_output=True,
    )
    if result.returncode:
        raise ValueError("startup image disassembly failed: " + result.stderr)
    return parse_image_listing(result.stdout, start, size)


def parse_image_listing(listing, start, size):
    rows = []
    cursor = start
    selected = False
    for line in listing.splitlines():
        match = re.match(r"\s*([0-9A-Fa-f]{6,8}):\s+((?:[0-9A-Fa-f]{2} )+)\s*(\S+)", line)
        continuation = re.fullmatch(r"\s+((?:[0-9A-Fa-f]{2}\s*)+)\s*", line)
        if selected and continuation:
            raw = bytes.fromhex(continuation[1])
            rows[-1] = (rows[-1][0] + raw, rows[-1][1])
            cursor += len(raw)
            continue
        selected = bool(match and start <= int(match[1], 16) < start + size)
        if selected:
            if int(match[1], 16) != cursor:
                raise ValueError("noncontiguous startup image disassembly")
            raw = bytes.fromhex(match[2])
            rows.append((raw, match[3]))
            cursor += len(raw)
    if cursor != start + size:
        raise ValueError("incomplete startup image disassembly")
    return rows


def check_matrix_paths(config, paths, reference, linked_map, retail_start):
    from _recoil.commands.vc5_build import object_path
    symbol, source, size, reviewed = MATRIX_PATH_BODIES[retail_start]
    def unique(symbol):
        matches = {row.address for row in linked_map.symbols if row.symbol == symbol and row.is_function}
        if len(matches) != 1:
            raise ValueError("matrix path lacks unique linked identity: " + symbol)
        return matches.pop()
    retail = image_bytes(reference, retail_start, size)
    rows = disassembled_image_rows(config, paths, REPO_ROOT / "support/Recoil.exe", retail_start, len(retail))
    effects = (image_matrix_effects(rows, retail_start, dict(reviewed.values())) if reviewed is not None
               else local_primitive_effects(rows, retail_start, dict(MATRIX_PRIMITIVES.values())))
    expected = path_depths(rows, retail, effects)
    obj = CoffObject.from_path(object_path(config, paths, REPO_ROOT / source))
    body = obj.function_bytes(symbol)
    start = unique(symbol)
    linked = image_bytes(paths.exe_path.read_bytes(), start, len(body.data))
    if any(a != b and not body.relocation_mask[i] for i, (a, b) in enumerate(zip(body.data, linked))):
        raise ValueError("linked matrix path disagrees with fresh object")
    for relocation in body.relocations:
        offset = relocation.offset - body.start
        if relocation.type not in (6, 0x14):
            raise ValueError("unsupported matrix path relocation")
        matches = {row.address for row in linked_map.symbols if row.symbol == relocation.symbol_name}
        if not matches and relocation.type == 6:
            # MAP omits TU-local constants. Prove the relocated payload against
            # the fresh COFF contribution, not a guessed map-name alias.
            local = obj.symbols_by_name.get(relocation.symbol_name)
            if local is not None and local.storage_class == 3 and local.section_number > 0:
                payload = obj.data_symbol_bytes(local.name)
                if payload.relocations or not payload.data or len(payload.data) > 4096:
                    raise ValueError("local matrix operand lacks bounded relocation-free data")
                target = int.from_bytes(linked[offset:offset + 4], "little")
                target -= int.from_bytes(body.data[offset:offset + 4], "little")
                executable = paths.exe_path.read_bytes()
                pe = parse_pe_headers(executable)
                target_rva = target - pe.image_base
                sections = [s for s in pe.sections if s.virtual_address <= target_rva
                            and target_rva + len(payload.data) <= s.virtual_address + s.raw_size
                            and not s.characteristics & 0x20000000]
                if len(sections) != 1:
                    raise ValueError("local matrix operand lacks file-backed data extent")
                file_offset = rva_to_offset(target_rva, pe.sections)
                if executable[file_offset:file_offset + len(payload.data)] != payload.data:
                    raise ValueError("local matrix operand disagrees with fresh COFF data")
                continue
        if len(matches) != 1:
            raise ValueError("matrix path relocation lacks unique target: " + relocation.symbol_name)
        required = matches.pop() + int.from_bytes(body.data[offset:offset + 4], "little")
        if relocation.type == 0x14:
            required -= start + offset + 4
        if int.from_bytes(linked[offset:offset + 4], "little") != required & 0xffffffff:
            raise ValueError("matrix path relocation target/addend differs")
    rows = disassembled_image_rows(config, paths, paths.exe_path, start, len(linked))
    targets = {unique(name): effect for name, (_, effect) in (reviewed or MATRIX_PRIMITIVES).items()}
    effects = (image_matrix_effects(rows, start, targets) if reviewed is not None
               else local_primitive_effects(rows, start, targets))
    observed = path_depths(rows, linked, effects)
    return {"identity": f"recoil:function:{retail_start:#x}", "retail": expected,
            "effect_scope": "reviewed callees" if reviewed is not None else "local primitive invocations only; no transitive callee proof",
            "candidate": observed, "passed": expected == observed}


def image_bytes(data: bytes, address: int, count: int) -> bytes:
    image = parse_pe_headers(data)
    rva = address - image.image_base
    sections = [s for s in image.sections if s.virtual_address <= rva
                and rva + count <= s.virtual_address + s.raw_size]
    if len(sections) != 1 or not sections[0].characteristics & 0x20000000:
        raise ValueError("selected body lacks an exact file-backed executable extent")
    offset = rva_to_offset(rva, image.sections)
    result = data[offset:offset + count]
    if len(result) != count:
        raise ValueError("truncated selected image body")
    return result


def check_startup_contract(config, paths) -> dict[str, object]:
    from _recoil.commands.vc5_build import object_path, parse_link_map

    report = {"kind": "startup-contract", "passed": False, "accepts_reconstruction": False,
              "scope": "turret initializer constant member bytes and selected unprojection/light/model matrix-stack paths", "checks": []}
    try:
        # Selection coordinates and object extent are retail constructor/allocator
        # facts; store offsets/values are derived anew from the complete body.
        symbol = "?InitDefaults@zTurret_Runtime@@QAEPAU1@XZ"
        reference = (REPO_ROOT / "support/Recoil.exe").read_bytes()
        expected = constant_member_bytes(image_bytes(reference, 0x436630, 0x170), object_size=0x180)
        obj = CoffObject.from_path(object_path(config, paths, REPO_ROOT / "src/Battlesport/turret.cpp"))
        body = obj.function_bytes(symbol)
        observed = constant_member_bytes(body.data, object_size=0x180)
        linked_map = parse_link_map(paths.map_path)
        matches = {row.address for row in linked_map.symbols if row.symbol == symbol and row.is_function}
        if len(matches) != 1:
            raise ValueError("initializer lacks unique linked identity")
        linked = image_bytes(paths.exe_path.read_bytes(), matches.pop(), len(body.data))
        if any(a != b and not body.relocation_mask[i] for i, (a, b) in enumerate(zip(body.data, linked))):
            raise ValueError("linked initializer disagrees with fresh object outside relocations")
        for relocation in body.relocations:
            targets = {row.address for row in linked_map.symbols if row.symbol == relocation.symbol_name}
            if relocation.type != 6 or len(targets) != 1:
                raise ValueError("initializer relocation lacks unique DIR32 linked identity")
            i = relocation.offset
            required = (targets.pop() + int.from_bytes(body.data[i:i + 4], "little")) & 0xffffffff
            if int.from_bytes(linked[i:i + 4], "little") != required:
                raise ValueError("initializer relocation resolved to the wrong target/addend")
        linked_values = constant_member_bytes(linked, object_size=0x180)
        if observed != linked_values:
            raise ValueError("linked initializer changes interpreted member stores")
        differences = [{"offset": hex(i), "retail": expected.get(i), "candidate": observed.get(i)}
                       for i in sorted(set(expected) | set(observed)) if expected.get(i) != observed.get(i)]
        report["checks"] = [{"identity": "recoil:function:0x436630", "symbol": symbol,
                             "retail_constant_bytes": len(expected), "differences": differences,
                             "passed": not differences}]
        report["checks"].extend(check_matrix_paths(config, paths, reference, linked_map, start)
                                for start in MATRIX_PATH_BODIES)
        report["passed"] = all(row["passed"] for row in report["checks"])
    except (OSError, ValueError) as error:
        report["error"] = str(error)
    (paths.build_dir / "startup-contract.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return report


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-root", type=Path, required=True)
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args(argv)
    root = args.build_root.resolve()
    if root.exists() or not root.is_relative_to((REPO_ROOT / "build/live-validation").resolve()):
        parser.error("build root must be fresh, absent, and below build/live-validation")
    from _recoil.commands.vc5_build import load_config, build_paths
    root.mkdir(parents=True)
    build = root / "canonical"
    with (root / "build.log").open("x", encoding="utf-8") as output:
        result = subprocess.run([sys.executable, str(REPO_ROOT / "tools/recoil.py"), "verify", "final-build",
                                 "--linkability-only", "--clean", "--build-dir", build.relative_to(REPO_ROOT).as_posix()],
                                cwd=REPO_ROOT, stdout=output, stderr=subprocess.STDOUT, check=False)
    if result.returncode:
        report = {"passed": False, "failure_stage": "canonical-build", "log": str(root / "build.log")}
    else:
        config = replace(load_config(), build_dir=build, build_dir_explicit=True)
        report = check_startup_contract(config, build_paths(config))
    print(json.dumps(report, indent=2))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
