from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
import os
import shutil
from pathlib import Path
import sys

from recoil_asm_verify import compare_bn_to_cod, compare_bn_to_obj
from recoil_binja import BridgeError
from recoil_plan import normalize_address
from recoil_tooling import (
    DEFAULT_VC6_ROOT as DEFAULT_VC6_ROOT_BASE,
    REPO_ROOT,
    optional_bool,
    quote_cmd_arg,
    repo_path,
    require_string,
    require_string_list,
    run_cmd_script as run_tool_cmd_script,
)


DEFAULT_MANIFEST_DIR = REPO_ROOT / "tools" / "vc6_verify_targets"
DEFAULT_BUILD_ROOT = REPO_ROOT / "build" / "vc6-verify"
DEFAULT_VC6_ROOT = Path(os.environ.get("RECOIL_VC6_ROOT", str(DEFAULT_VC6_ROOT_BASE)))
DEFAULT_VC6_ENV = Path(os.environ.get("RECOIL_VC6_ENV", str(DEFAULT_VC6_ROOT / "vc6-env.cmd")))


@dataclass(frozen=True)
class VerifyFunction:
    address: str
    symbol: str
    name: str


@dataclass(frozen=True)
class VerifyTarget:
    name: str
    description: str
    source_filename: str
    source_text: str
    source_from: str
    compare_mode: str
    trim_trailing_nops: bool
    compiler_flags: tuple[str, ...]
    include_dirs: tuple[str, ...]
    source_files: tuple[str, ...]
    generated_files: tuple[tuple[str, str], ...]
    functions: tuple[VerifyFunction, ...]
    manifest_path: Path


def load_manifest(path: Path) -> VerifyTarget:
    with path.open("r", encoding="utf-8") as handle:
        data = json.load(handle)
    if not isinstance(data, dict):
        raise ValueError(f"{path}: manifest root must be an object")

    source_from = data.get("source_from", "")
    if source_from and not isinstance(source_from, str):
        raise ValueError(f"{path}: expected 'source_from' as a string")
    source = data.get("source")
    if source_from:
        source_text = ""
    elif isinstance(source, list) and all(isinstance(line, str) for line in source):
        source_text = "\n".join(source) + "\n"
    elif isinstance(source, str):
        source_text = source
    else:
        raise ValueError(f"{path}: expected 'source' as a string or string list")

    generated_files_data = data.get("generated_files", {})
    generated_files: list[tuple[str, str]] = []
    if not isinstance(generated_files_data, dict):
        raise ValueError(f"{path}: expected 'generated_files' as an object")
    for generated_path, generated_source in sorted(generated_files_data.items()):
        if not isinstance(generated_path, str):
            raise ValueError(f"{path}: generated file paths must be strings")
        if isinstance(generated_source, list) and all(isinstance(line, str) for line in generated_source):
            generated_files.append((generated_path, "\n".join(generated_source) + "\n"))
        elif isinstance(generated_source, str):
            generated_files.append((generated_path, generated_source))
        else:
            raise ValueError(f"{path}: generated file '{generated_path}' must be a string or string list")

    functions_data = data.get("functions")
    if not isinstance(functions_data, list) or not functions_data:
        raise ValueError(f"{path}: expected non-empty 'functions' list")

    functions: list[VerifyFunction] = []
    seen_addresses: set[str] = set()
    for item in functions_data:
        if not isinstance(item, dict):
            raise ValueError(f"{path}: function entries must be objects")
        address = normalize_address(require_string(item, "address", manifest_path=path))
        if address in seen_addresses:
            raise ValueError(f"{path}: duplicate function address {address}")
        seen_addresses.add(address)
        symbol = require_string(item, "symbol", manifest_path=path)
        name = item.get("name", symbol)
        if not isinstance(name, str) or not name:
            raise ValueError(f"{path}: expected non-empty function name for {address}")
        functions.append(VerifyFunction(address=address, symbol=symbol, name=name))

    compare_mode = data.get("compare_mode", "coff_bytes")
    if not isinstance(compare_mode, str) or compare_mode not in {"coff_bytes", "text"}:
        raise ValueError(f"{path}: expected 'compare_mode' to be 'coff_bytes' or 'text'")

    return VerifyTarget(
        name=require_string(data, "name", manifest_path=path),
        description=require_string(data, "description", manifest_path=path),
        source_filename=require_string(data, "source_filename", manifest_path=path),
        source_text=source_text,
        source_from=source_from or "",
        compare_mode=compare_mode,
        trim_trailing_nops=optional_bool(data, "trim_trailing_nops", True, manifest_path=path),
        compiler_flags=require_string_list(data, "compiler_flags", manifest_path=path, allow_empty_items=True),
        include_dirs=require_string_list(data, "include_dirs", manifest_path=path, allow_empty_items=True),
        source_files=require_string_list(data, "source_files", manifest_path=path, allow_empty_items=True),
        generated_files=tuple(generated_files),
        functions=tuple(functions),
        manifest_path=path,
    )


def load_manifests(manifest_dir: Path) -> list[VerifyTarget]:
    manifests = [load_manifest(path) for path in sorted(manifest_dir.glob("*.json"))]
    names: set[str] = set()
    for manifest in manifests:
        if manifest.name in names:
            raise ValueError(f"Duplicate VC6 verification target name: {manifest.name}")
        names.add(manifest.name)
    return manifests


def find_target(manifests: list[VerifyTarget], selector: str) -> tuple[VerifyTarget, tuple[VerifyFunction, ...], str]:
    normalized_address = None
    if selector.lower().startswith("0x"):
        normalized_address = normalize_address(selector)

    if normalized_address:
        matches = [
            (manifest, function)
            for manifest in manifests
            for function in manifest.functions
            if function.address == normalized_address
        ]
        if not matches:
            raise ValueError(f"No VC6 verification manifest covers {normalized_address}")
        if len(matches) > 1:
            names = ", ".join(manifest.name for manifest, _function in matches)
            raise ValueError(f"Multiple VC6 verification manifests cover {normalized_address}: {names}")
        manifest, function = matches[0]
        return manifest, (function,), normalized_address

    for manifest in manifests:
        if manifest.name == selector:
            return manifest, manifest.functions, manifest.name
    raise ValueError(f"Unknown VC6 verification target: {selector}")


def covering_targets(manifests: list[VerifyTarget], address: str) -> list[tuple[VerifyTarget, VerifyFunction]]:
    normalized_address = normalize_address(address)
    return [
        (manifest, function)
        for manifest in manifests
        for function in manifest.functions
        if function.address == normalized_address
    ]


def manifest_skeleton(address: str) -> dict[str, object]:
    normalized_address = normalize_address(address)
    safe_address = normalized_address[2:]
    return {
        "name": f"verify_{safe_address}",
        "description": f"VC6 verification target for {normalized_address}.",
        "source_filename": f"verify_{safe_address}.cpp",
        "compiler_flags": ["/nologo", "/TP", "/O2", "/FAcs"],
        "include_dirs": ["src"],
        "source_files": [],
        "functions": [
            {
                "address": normalized_address,
                "symbol": "TODO_DECORATED_VC6_SYMBOL",
                "name": "TODO_SOURCE_NAME",
            }
        ],
        "source": [
            "// TODO: include the production body or a focused VC6-compatible translation unit.",
        ],
    }


def print_missing_explanation(manifests: list[VerifyTarget], address: str) -> None:
    normalized_address = normalize_address(address)
    matches = covering_targets(manifests, normalized_address)
    if matches:
        print(f"{normalized_address} is covered by:")
        for manifest, function in matches:
            print(f"- {manifest.name}: {function.symbol} ({manifest.manifest_path})")
        return

    print(f"No VC6 verification manifest covers {normalized_address}.")
    print("Create or extend a JSON manifest under tools/vc6_verify_targets/.")
    print("Suggested starting point:")
    print(json.dumps(manifest_skeleton(normalized_address), indent=2))


def resolve_repo_path(path_text: str) -> Path:
    return repo_path(path_text)


def build_compile_command(target: VerifyTarget, source_path: Path, vc6_env: Path, build_dir: Path | None = None) -> str:
    include_args = " ".join(f"/I {quote_cmd_arg(resolve_repo_path(include_dir))}" for include_dir in target.include_dirs)
    flags = " ".join(target.compiler_flags)
    if build_dir is not None:
        try:
            source_arg = source_path.relative_to(build_dir)
        except ValueError:
            source_arg = source_path
    else:
        source_arg = source_path.name
    return f"call {quote_cmd_arg(vc6_env)} && cl {flags} {include_args} /c {quote_cmd_arg(source_arg)}"


def run_vc6_script(command: str, *, cwd: Path) -> int:
    completed = run_tool_cmd_script(
        command,
        cwd=cwd,
        script_name="_run_vc6_verify.cmd",
        capture_output=True,
    )
    if completed.stdout:
        print(completed.stdout, end="")
    if completed.stderr:
        print(completed.stderr, end="", file=sys.stderr)
    return completed.returncode


def read_classified_summary(path: Path) -> dict[str, int]:
    result: dict[str, int] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line.strip():
            break
        if ":" not in line:
            continue
        key, value = line.strip().split(":", 1)
        try:
            result[key] = int(value.strip())
        except ValueError:
            continue
    return result


def print_target_list(manifests: list[VerifyTarget]) -> None:
    for manifest in manifests:
        addresses = ", ".join(function.address for function in manifest.functions)
        print(f"{manifest.name}: {addresses}")
        print(f"  {manifest.description}")


def run_target(
    *,
    target: VerifyTarget,
    selected_functions: tuple[VerifyFunction, ...],
    build_root: Path,
    vc6_env: Path,
    bridge_url: str,
    skip_bn_compare: bool,
) -> int:
    build_dir = (build_root / target.name).resolve()
    build_dir.mkdir(parents=True, exist_ok=True)

    source_path = build_dir / target.source_filename
    source_path.parent.mkdir(parents=True, exist_ok=True)
    if target.source_from:
        shutil.copyfile(resolve_repo_path(target.source_from), source_path)
    else:
        source_path.write_text(target.source_text, encoding="ascii")

    for relative_path, contents in target.generated_files:
        generated_path = build_dir / relative_path
        generated_path.parent.mkdir(parents=True, exist_ok=True)
        generated_path.write_text(contents, encoding="ascii")

    if not vc6_env.exists():
        print(f"VC6 environment not found: {vc6_env}", file=sys.stderr)
        return 2

    compile_cmd = build_compile_command(target, source_path, vc6_env, build_dir)
    rc = run_vc6_script(compile_cmd, cwd=build_dir)
    if rc != 0:
        return rc

    cod_path = source_path.with_suffix(".cod")
    if not cod_path.exists():
        print(f"Expected COD listing was not emitted: {cod_path}", file=sys.stderr)
        return 3
    obj_path = source_path.with_suffix(".obj")
    if target.compare_mode == "coff_bytes" and not obj_path.exists():
        print(f"Expected COFF object was not emitted: {obj_path}", file=sys.stderr)
        return 3

    print(f"VC6 target: {target.name}")
    print(f"Manifest: {target.manifest_path}")
    print(f"Source files: {', '.join(target.source_files) if target.source_files else '<manifest source only>'}")
    print(f"Compare mode: {target.compare_mode}")
    print(f"Compiler flags: {' '.join(target.compiler_flags)}")
    print(f"VC6 listing: {cod_path}")
    if target.compare_mode == "coff_bytes":
        print(f"VC6 object: {obj_path}")

    if skip_bn_compare:
        return 0

    verify_dir = build_dir / "verify"
    rows: list[tuple[VerifyFunction, str, int, int, int, int, int, Path]] = []
    overall = 0
    for function in selected_functions:
        try:
            if target.compare_mode == "coff_bytes":
                comparison = compare_bn_to_obj(
                    address=function.address,
                    obj_path=obj_path,
                    symbol=function.symbol,
                    out_dir=verify_dir,
                    bridge_url=bridge_url,
                    cod_path=cod_path,
                    trim_padding_nops=target.trim_trailing_nops,
                )
                rows.append(
                    (
                        function,
                        "bytes",
                        comparison.mismatch_count,
                        comparison.relocation_masked_bytes,
                        comparison.trailing_vc6_nops_trimmed,
                        comparison.bn_size,
                        comparison.vc6_size,
                        comparison.diff_path,
                    )
                )
            else:
                comparison = compare_bn_to_cod(
                    address=function.address,
                    cod_path=cod_path,
                    symbol=function.symbol,
                    out_dir=verify_dir,
                    bridge_url=bridge_url,
                )
                summary = read_classified_summary(comparison.classified_path)
                rows.append(
                    (
                        function,
                        "text",
                        comparison.mismatch_count,
                        summary.get("relocation_sensitive_differences", 0),
                        summary.get("schedule_equivalent_differences", 0),
                        summary.get("exact_or_normalized_matches", 0),
                        comparison.diff_count,
                        comparison.classified_path,
                    )
                )
        except (BridgeError, ValueError) as exc:
            print(f"{function.address} {function.name}: {exc}", file=sys.stderr)
            overall = 1
            continue
        if rows[-1][2]:
            overall = 1

    print("Verification summary:")
    if target.compare_mode == "coff_bytes":
        print("address    status  mismatches  reloc-bytes  trim-nops  bn-size  vc6-size  evidence")
        for function, _mode, mismatches, reloc_bytes, trim_nops, bn_size, vc6_size, evidence_path in rows:
            status = "FAIL" if mismatches else "OK"
            print(
                f"{function.address}  {status:5}  "
                f"{mismatches:10}  "
                f"{reloc_bytes:11}  "
                f"{trim_nops:9}  "
                f"{bn_size:7}  "
                f"{vc6_size:8}  "
                f"{evidence_path}"
            )
        print("Relocation bytes are masked from COFF relocation records; unmasked byte differences are failures.")
    else:
        print("address    status  mismatches  reloc  sched  normalized/exact  diff-lines  evidence")
        for function, _mode, mismatches, reloc, sched, normalized, diff_count, evidence_path in rows:
            status = "FAIL" if mismatches else ("OK*" if diff_count else "OK")
            print(
                f"{function.address}  {status:5}  "
                f"{mismatches:10}  "
                f"{reloc:5}  "
                f"{sched:5}  "
                f"{normalized:16}  "
                f"{diff_count:10}  "
                f"{evidence_path}"
            )
        print("OK* means no instruction mismatches; only accepted text-normalization differences remain.")
    print(f"Verification evidence: {verify_dir}")
    return overall


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Compile VC6 verification targets and compare relocation-masked COFF bytes to BN."
    )
    parser.add_argument("target", nargs="?", help="Target manifest name or covered function address, e.g. zsys_cpu or 0x4b3510")
    parser.add_argument("--list", action="store_true", help="List available VC6 verification targets.")
    parser.add_argument(
        "--explain-missing",
        metavar="ADDRESS",
        help="Explain VC6 coverage for an address and print a skeleton manifest if none exists.",
    )
    parser.add_argument("--manifest-dir", default=str(DEFAULT_MANIFEST_DIR))
    parser.add_argument("--build-root", default=str(DEFAULT_BUILD_ROOT))
    parser.add_argument("--vc6-env", default=str(DEFAULT_VC6_ENV))
    parser.add_argument("--bridge-url", default="http://localhost:9009")
    parser.add_argument("--skip-bn-compare", action="store_true")
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        manifests = load_manifests(Path(args.manifest_dir))
        if args.list:
            print_target_list(manifests)
            return 0
        if args.explain_missing:
            print_missing_explanation(manifests, args.explain_missing)
            return 0
        if not args.target:
            parser.error("target is required unless --list or --explain-missing is used")
        target, functions, selector = find_target(manifests, args.target)
        if selector != target.name:
            print(f"Resolved {selector} to VC6 target {target.name}")
        return run_target(
            target=target,
            selected_functions=functions,
            build_root=Path(args.build_root),
            vc6_env=Path(args.vc6_env),
            bridge_url=args.bridge_url,
            skip_bn_compare=args.skip_bn_compare,
        )
    except ValueError as exc:
        print(str(exc), file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
