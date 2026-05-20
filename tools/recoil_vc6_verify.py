from __future__ import annotations

import argparse
from dataclasses import dataclass
import hashlib
import json
import os
import re
import shutil
from pathlib import Path
import sys
from typing import Any

from recoil_asm_verify import AssemblyComparison, ObjectByteComparison, compare_bn_to_cod, compare_bn_to_obj
from recoil_binja import BridgeError
from recoil_plan import normalize_address
from recoil_tooling import (
    CommandScriptResult,
    DEFAULT_VC6_ROOT as DEFAULT_VC6_ROOT_BASE,
    REPO_ROOT,
    display_path,
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
DEFAULT_SOURCE_POLICY_BASELINE = REPO_ROOT / ".agent" / "VC6_MANIFEST_SOURCE_POLICY_BASELINE.txt"
PROJECT_GENERATED_FILE_PREFIXES = ("src/", "GameZRecoil/", "Battlesport/", "recoil/")
QUOTED_INCLUDE_RE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.MULTILINE)


@dataclass(frozen=True)
class VerifyFunction:
    address: str
    symbol: str
    name: str
    bn_byte_length: int | None = None


@dataclass(frozen=True)
class VerifyTarget:
    name: str
    description: str
    source_filename: str
    source_text: str
    source_from: str
    compare_mode: str
    trim_trailing_nops: bool
    compiler_env: str
    compiler_flags: tuple[str, ...]
    include_dirs: tuple[str, ...]
    source_files: tuple[str, ...]
    generated_files: tuple[tuple[str, str], ...]
    functions: tuple[VerifyFunction, ...]
    manifest_path: Path


@dataclass(frozen=True)
class VerifySelection:
    target: VerifyTarget
    functions: tuple[VerifyFunction, ...]


@dataclass(frozen=True)
class CompiledTarget:
    target: VerifyTarget
    build_dir: Path
    source_path: Path
    cod_path: Path
    obj_path: Path
    compiler_env: Path
    compiler_version: str
    compile_command: str


@dataclass(frozen=True)
class VerificationResult:
    target: VerifyTarget
    function: VerifyFunction
    mode: str
    mismatches: int
    relocation_or_text_metric: int
    secondary_metric: int
    bn_size_or_normalized: int
    vc6_size_or_diff_count: int
    evidence_path: Path
    triage_path: Path | None
    comparison: ObjectByteComparison | AssemblyComparison


@dataclass(frozen=True)
class SourcePolicyBaseline:
    inline_manifests: frozenset[str]
    generated_project_files: frozenset[tuple[str, str]]


def repo_manifest_key(path: Path) -> str:
    return display_path(path.resolve()).replace("\\", "/")


def normalize_generated_path(path_text: str) -> str:
    return path_text.replace("\\", "/").lstrip("./")


def read_source_policy_baseline(path: Path = DEFAULT_SOURCE_POLICY_BASELINE) -> SourcePolicyBaseline:
    inline_manifests: set[str] = set()
    generated_project_files: set[tuple[str, str]] = set()
    if not path.exists():
        return SourcePolicyBaseline(frozenset(), frozenset())

    for line_number, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split()
        if len(parts) < 2:
            raise ValueError(f"{path}:{line_number}: expected '<kind> <manifest> [generated-path]'")
        kind = parts[0]
        manifest_key = parts[1].replace("\\", "/")
        if kind == "inline" and len(parts) == 2:
            inline_manifests.add(manifest_key)
        elif kind == "generated" and len(parts) == 3:
            generated_project_files.add((manifest_key, normalize_generated_path(parts[2])))
        else:
            raise ValueError(f"{path}:{line_number}: invalid source-policy baseline entry")
    return SourcePolicyBaseline(
        inline_manifests=frozenset(inline_manifests),
        generated_project_files=frozenset(generated_project_files),
    )


def generated_file_shadows_project(relative_path: str) -> bool:
    normalized = normalize_generated_path(relative_path)
    return normalized.startswith(PROJECT_GENERATED_FILE_PREFIXES)


def source_text_from_data(data: dict[str, Any], path: Path) -> str:
    source = data.get("source")
    if isinstance(source, list) and all(isinstance(line, str) for line in source):
        return "\n".join(source) + "\n"
    if isinstance(source, str):
        return source
    raise ValueError(f"{path}: expected 'source' as a string or string list")


def source_from_text(source_from: str, manifest_path: Path) -> str:
    source_path = repo_path(source_from)
    if not source_path.exists():
        raise ValueError(f"{manifest_path}: source_from does not exist: {source_path}")
    return source_path.read_text(encoding="utf-8", errors="ignore")


def resolve_project_include(include_text: str, including_source: Path) -> Path | None:
    include_path = Path(include_text)
    candidates = (
        including_source.parent / include_path,
        REPO_ROOT / include_path,
        REPO_ROOT / "src" / include_path,
    )
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    return None


def source_from_policy_text(source_from: str, manifest_path: Path) -> str:
    source_path = repo_path(source_from)
    source_text = source_from_text(source_from, manifest_path)
    chunks = [source_text]
    for match in QUOTED_INCLUDE_RE.finditer(source_text):
        include_text = match.group(1)
        if not include_text.lower().endswith(".inl"):
            continue
        include_path = resolve_project_include(include_text, source_path)
        if include_path is not None:
            chunks.append(include_path.read_text(encoding="utf-8", errors="ignore"))
    return "\n".join(chunks)


def validate_source_policy(
    *,
    data: dict[str, Any],
    manifest_path: Path,
    source_from: str,
    functions: tuple[VerifyFunction, ...],
    generated_files: tuple[tuple[str, str], ...],
    baseline: SourcePolicyBaseline,
) -> None:
    manifest_key = repo_manifest_key(manifest_path)
    has_inline_source = "source" in data and not source_from
    has_source_from = bool(source_from)

    if source_from and "source" in data:
        raise ValueError(f"{manifest_path}: use either 'source_from' or 'source', not both")

    if has_source_from:
        production_text = source_from_policy_text(source_from, manifest_path)
        for function in functions:
            required = f"Reimplements {function.address}:"
            if required not in production_text:
                raise ValueError(
                    f"{manifest_path}: source_from {source_from} does not contain provenance "
                    f"comment '{required}'"
                )
    elif has_inline_source:
        allow_inline = optional_bool(data, "allow_inline_source", False, manifest_path=manifest_path)
        inline_reason = data.get("inline_source_reason", "")
        if inline_reason and not isinstance(inline_reason, str):
            raise ValueError(f"{manifest_path}: expected 'inline_source_reason' as a string")
        if manifest_key not in baseline.inline_manifests:
            if not allow_inline:
                raise ValueError(
                    f"{manifest_path}: inline source bodies are not allowed for VC6 verification; "
                    "use source_from to compile production source"
                )
            inline_lines = source_text_from_data(data, manifest_path).splitlines()
            if len(inline_lines) > 40:
                raise ValueError(
                    f"{manifest_path}: allow_inline_source is limited to tiny provider harnesses "
                    "(40 lines or fewer)"
                )
            if not inline_reason:
                raise ValueError(
                    f"{manifest_path}: allow_inline_source requires inline_source_reason"
                )
    else:
        raise ValueError(f"{manifest_path}: expected 'source_from' for production verification")

    for generated_path, _contents in generated_files:
        normalized_generated_path = normalize_generated_path(generated_path)
        if not generated_file_shadows_project(normalized_generated_path):
            continue
        if (manifest_key, normalized_generated_path) not in baseline.generated_project_files:
            raise ValueError(
                f"{manifest_path}: generated file shadows project header '{generated_path}'; "
                "fix VC6 compatibility in production headers or record existing legacy debt in "
                f"{display_path(DEFAULT_SOURCE_POLICY_BASELINE)}"
            )


def optional_positive_int(data: dict[str, Any], key: str, *, manifest_path: Path) -> int | None:
    value = data.get(key)
    if value is None:
        return None
    if isinstance(value, bool) or not isinstance(value, int):
        raise ValueError(f"{manifest_path}: expected '{key}' to be a positive integer")
    if value <= 0:
        raise ValueError(f"{manifest_path}: expected '{key}' to be a positive integer")
    return value


def load_manifest(
    path: Path,
    *,
    enforce_source_policy: bool = True,
    source_policy_baseline: SourcePolicyBaseline | None = None,
) -> VerifyTarget:
    with path.open("r", encoding="utf-8") as handle:
        data = json.load(handle)
    if not isinstance(data, dict):
        raise ValueError(f"{path}: manifest root must be an object")

    source_from = data.get("source_from", "")
    if source_from and not isinstance(source_from, str):
        raise ValueError(f"{path}: expected 'source_from' as a string")
    if source_from:
        source_text = ""
    else:
        source_text = source_text_from_data(data, path)

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
        functions.append(
            VerifyFunction(
                address=address,
                symbol=symbol,
                name=name,
                bn_byte_length=optional_positive_int(item, "bn_byte_length", manifest_path=path),
            )
        )

    compare_mode = data.get("compare_mode", "coff_bytes")
    if not isinstance(compare_mode, str) or compare_mode not in {"coff_bytes", "text"}:
        raise ValueError(f"{path}: expected 'compare_mode' to be 'coff_bytes' or 'text'")
    compiler_env = data.get("compiler_env", "")
    if compiler_env and not isinstance(compiler_env, str):
        raise ValueError(f"{path}: expected 'compiler_env' as a string")

    generated_files_tuple = tuple(generated_files)
    functions_tuple = tuple(functions)
    if enforce_source_policy:
        validate_source_policy(
            data=data,
            manifest_path=path,
            source_from=source_from or "",
            functions=functions_tuple,
            generated_files=generated_files_tuple,
            baseline=source_policy_baseline or read_source_policy_baseline(),
        )

    return VerifyTarget(
        name=require_string(data, "name", manifest_path=path),
        description=require_string(data, "description", manifest_path=path),
        source_filename=require_string(data, "source_filename", manifest_path=path),
        source_text=source_text,
        source_from=source_from or "",
        compare_mode=compare_mode,
        trim_trailing_nops=optional_bool(data, "trim_trailing_nops", True, manifest_path=path),
        compiler_env=compiler_env or "",
        compiler_flags=require_string_list(data, "compiler_flags", manifest_path=path, allow_empty_items=True),
        include_dirs=require_string_list(data, "include_dirs", manifest_path=path, allow_empty_items=True),
        source_files=require_string_list(data, "source_files", manifest_path=path, allow_empty_items=True),
        generated_files=generated_files_tuple,
        functions=functions_tuple,
        manifest_path=path,
    )


def load_manifests(
    manifest_dir: Path,
    *,
    enforce_source_policy: bool = True,
    source_policy_baseline: SourcePolicyBaseline | None = None,
) -> list[VerifyTarget]:
    baseline = source_policy_baseline or read_source_policy_baseline()
    manifests = [
        load_manifest(path, enforce_source_policy=enforce_source_policy, source_policy_baseline=baseline)
        for path in sorted(manifest_dir.glob("*.json"))
    ]
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
        "source_from": "src/TODO_SOURCE_FILE.cpp",
        "functions": [
            {
                "address": normalized_address,
                "symbol": "TODO_DECORATED_VC6_SYMBOL",
                "name": "TODO_SOURCE_NAME",
            }
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


def run_vc6_script(command: str, *, cwd: Path) -> CommandScriptResult:
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
    return completed


def prepare_clean_build_dir(build_root: Path, relative_dir: str) -> Path:
    resolved_build_root = build_root.resolve()
    build_dir = (resolved_build_root / relative_dir).resolve()
    if resolved_build_root != build_dir and resolved_build_root not in build_dir.parents:
        raise ValueError(f"Refusing to clean VC6 build directory outside build root: {build_dir}")
    if build_dir.exists():
        shutil.rmtree(build_dir)
    build_dir.mkdir(parents=True, exist_ok=True)
    return build_dir


def compiler_env_path(target: VerifyTarget, vc6_env: Path) -> Path:
    return resolve_repo_path(target.compiler_env) if target.compiler_env else vc6_env


def target_compile_key(target: VerifyTarget, vc6_env: Path) -> tuple[object, ...]:
    compiler_env = compiler_env_path(target, vc6_env).resolve()
    source_identity = target.source_from or f"<inline>\n{target.source_text}"
    return (
        source_identity.replace("\\", "/"),
        target.source_filename,
        str(compiler_env).replace("\\", "/").lower(),
        target.compiler_flags,
        target.include_dirs,
        target.generated_files,
    )


def compile_key_digest(key: tuple[object, ...]) -> str:
    encoded = json.dumps(key, sort_keys=True).encode("utf-8")
    return hashlib.sha1(encoded).hexdigest()[:12]


def safe_path_component(text: str) -> str:
    safe = re.sub(r"[^A-Za-z0-9_.-]+", "_", text).strip("._")
    return safe[:80] or "target"


def group_selections_by_compile_key(
    selections: list[VerifySelection],
    vc6_env: Path,
) -> list[tuple[tuple[object, ...], list[VerifySelection]]]:
    grouped: dict[tuple[object, ...], list[VerifySelection]] = {}
    order: list[tuple[object, ...]] = []
    for selection in selections:
        key = target_compile_key(selection.target, vc6_env)
        if key not in grouped:
            grouped[key] = []
            order.append(key)
        grouped[key].append(selection)
    return [(key, grouped[key]) for key in order]


def source_from_matches(target: VerifyTarget, source_from: str) -> bool:
    if not target.source_from:
        return False
    return resolve_repo_path(target.source_from).resolve() == resolve_repo_path(source_from).resolve()


def selected_targets_for_source_from(manifests: list[VerifyTarget], source_from: str) -> list[VerifySelection]:
    return [
        VerifySelection(target=manifest, functions=manifest.functions)
        for manifest in manifests
        if source_from_matches(manifest, source_from)
    ]


def selected_targets_for_all(manifests: list[VerifyTarget]) -> list[VerifySelection]:
    return [VerifySelection(target=manifest, functions=manifest.functions) for manifest in manifests]


def write_compile_inputs(target: VerifyTarget, build_dir: Path) -> Path:
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
    return source_path


def detect_compiler_version(compiler_env: Path, *, cwd: Path) -> str:
    completed = run_tool_cmd_script(
        f"call {quote_cmd_arg(compiler_env)} && cl",
        cwd=cwd,
        script_name="_detect_vc6_version.cmd",
        capture_output=True,
    )
    output = "\n".join((completed.stdout, completed.stderr))
    for line in output.splitlines():
        if "Compiler Version" in line:
            return line.strip()
    return "<not detected>"


def compile_target(
    *,
    target: VerifyTarget,
    build_dir: Path,
    vc6_env: Path,
) -> tuple[CompiledTarget | None, int]:
    source_path = write_compile_inputs(target, build_dir)

    compiler_env = compiler_env_path(target, vc6_env)
    if not compiler_env.exists():
        print(f"VC6 environment not found: {compiler_env}", file=sys.stderr)
        return None, 2

    compile_cmd = build_compile_command(target, source_path, compiler_env, build_dir)
    completed = run_vc6_script(compile_cmd, cwd=build_dir)
    if completed.returncode != 0:
        return None, completed.returncode

    cod_path = source_path.with_suffix(".cod")
    if not cod_path.exists():
        print(f"Expected COD listing was not emitted: {cod_path}", file=sys.stderr)
        return None, 3
    obj_path = source_path.with_suffix(".obj")
    if target.compare_mode == "coff_bytes" and not obj_path.exists():
        print(f"Expected COFF object was not emitted: {obj_path}", file=sys.stderr)
        return None, 3

    return (
        CompiledTarget(
            target=target,
            build_dir=build_dir,
            source_path=source_path,
            cod_path=cod_path,
            obj_path=obj_path,
            compiler_env=compiler_env,
            compiler_version=detect_compiler_version(compiler_env, cwd=build_dir),
            compile_command=compile_cmd,
        ),
        0,
    )


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


def print_compiled_target_info(target: VerifyTarget, compiled: CompiledTarget) -> None:
    print(f"VC6 target: {target.name}")
    print(f"Manifest: {target.manifest_path}")
    print(f"Source files: {', '.join(target.source_files) if target.source_files else '<manifest source only>'}")
    print(f"Compare mode: {target.compare_mode}")
    print(f"Compiler env: {compiled.compiler_env}")
    print(f"Compiler version: {compiled.compiler_version}")
    print(f"Compiler flags: {' '.join(target.compiler_flags)}")
    print(f"VC6 listing: {compiled.cod_path}")
    if compiled.obj_path.exists():
        print(f"VC6 object: {compiled.obj_path}")


def compare_compiled_selections(
    *,
    compiled: CompiledTarget,
    selections: list[VerifySelection],
    bridge_url: str,
) -> tuple[list[VerificationResult], int]:
    verify_dir = compiled.build_dir / "verify"
    results: list[VerificationResult] = []
    overall = 0
    for selection in selections:
        target = selection.target
        for function in selection.functions:
            try:
                if target.compare_mode == "coff_bytes":
                    comparison = compare_bn_to_obj(
                        address=function.address,
                        obj_path=compiled.obj_path,
                        symbol=function.symbol,
                        out_dir=verify_dir,
                        bridge_url=bridge_url,
                        cod_path=compiled.cod_path,
                        trim_padding_nops=target.trim_trailing_nops,
                        bn_byte_length=function.bn_byte_length,
                    )
                    results.append(
                        VerificationResult(
                            target=target,
                            function=function,
                            mode="bytes",
                            mismatches=comparison.mismatch_count,
                            relocation_or_text_metric=comparison.relocation_masked_bytes,
                            secondary_metric=comparison.trailing_vc6_nops_trimmed,
                            bn_size_or_normalized=comparison.bn_size,
                            vc6_size_or_diff_count=comparison.vc6_size,
                            evidence_path=comparison.diff_path,
                            triage_path=comparison.triage_path,
                            comparison=comparison,
                        )
                    )
                else:
                    comparison = compare_bn_to_cod(
                        address=function.address,
                        cod_path=compiled.cod_path,
                        symbol=function.symbol,
                        out_dir=verify_dir,
                        bridge_url=bridge_url,
                    )
                    summary = read_classified_summary(comparison.classified_path)
                    results.append(
                        VerificationResult(
                            target=target,
                            function=function,
                            mode="text",
                            mismatches=comparison.mismatch_count,
                            relocation_or_text_metric=summary.get("relocation_sensitive_differences", 0),
                            secondary_metric=summary.get("schedule_equivalent_differences", 0),
                            bn_size_or_normalized=summary.get("exact_or_normalized_matches", 0),
                            vc6_size_or_diff_count=comparison.diff_count,
                            evidence_path=comparison.classified_path,
                            triage_path=None,
                            comparison=comparison,
                        )
                    )
            except (BridgeError, ValueError) as exc:
                print(f"{function.address} {function.name}: {exc}", file=sys.stderr)
                overall = 1
                continue
            if results[-1].mismatches:
                overall = 1
    return results, overall


def print_verification_summary(results: list[VerificationResult], *, include_target: bool = False) -> None:
    print("Verification summary:")
    if not results:
        print("- no functions compared")
        return

    if all(result.mode == "bytes" for result in results):
        prefix = "target                                      " if include_target else ""
        print(
            f"{prefix}address    status  mismatches  reloc-bytes  trim-nops  "
            "bn-size  vc6-size  evidence  triage"
        )
        for result in results:
            status = "FAIL" if result.mismatches else "OK"
            target_prefix = f"{result.target.name:42}  " if include_target else ""
            print(
                f"{target_prefix}"
                f"{result.function.address}  {status:5}  "
                f"{result.mismatches:10}  "
                f"{result.relocation_or_text_metric:11}  "
                f"{result.secondary_metric:9}  "
                f"{result.bn_size_or_normalized:7}  "
                f"{result.vc6_size_or_diff_count:8}  "
                f"{result.evidence_path}  "
                f"{result.triage_path}"
            )
        print("Relocation bytes are masked from COFF relocation records; unmasked byte differences are failures.")
        return

    if all(result.mode == "text" for result in results):
        prefix = "target                                      " if include_target else ""
        print(f"{prefix}address    status  mismatches  reloc  sched  normalized/exact  diff-lines  evidence")
        for result in results:
            status = "FAIL" if result.mismatches else ("OK*" if result.vc6_size_or_diff_count else "OK")
            target_prefix = f"{result.target.name:42}  " if include_target else ""
            print(
                f"{target_prefix}"
                f"{result.function.address}  {status:5}  "
                f"{result.mismatches:10}  "
                f"{result.relocation_or_text_metric:5}  "
                f"{result.secondary_metric:5}  "
                f"{result.bn_size_or_normalized:16}  "
                f"{result.vc6_size_or_diff_count:10}  "
                f"{result.evidence_path}"
            )
        print("OK* means no instruction mismatches; only accepted text-normalization differences remain.")
        return

    print("target                                      address    mode   status  mismatches  evidence  triage")
    for result in results:
        status = "FAIL" if result.mismatches else "OK"
        triage = result.triage_path if result.triage_path is not None else "<none>"
        print(
            f"{result.target.name:42}  {result.function.address}  {result.mode:5}  "
            f"{status:5}  {result.mismatches:10}  {result.evidence_path}  {triage}"
        )


def print_evidence_block(compiled: CompiledTarget, result: VerificationResult) -> None:
    if result.mismatches:
        return

    target = result.target
    function = result.function
    source = target.source_from or str(compiled.source_path)
    print()
    print("Binary-safe evidence block:")
    print(f"- Address: {function.address}")
    print(f"- Function: {function.name}")
    print(f"- Manifest: {target.manifest_path}")
    print(f"- Source: {source}")
    print(f"- Generated symbol: {function.symbol}")
    print(f"- Compiler env: {compiled.compiler_env}")
    print(f"- Compiler version: {compiled.compiler_version}")
    print(f"- Compiler flags: {' '.join(target.compiler_flags)}")
    print("- Target architecture: x86")
    print(f"- VC6 object: {compiled.obj_path}")
    print(f"- VC6 listing: {compiled.cod_path}")
    if isinstance(result.comparison, ObjectByteComparison):
        print(f"- Relocation mask: {result.comparison.mask_path}")
        print(f"- Byte diff: {result.comparison.diff_path}")
        print(f"- Triage: {result.comparison.triage_path}")
        if result.comparison.text_diff_path:
            print(f"- Normalized asm diff: {result.comparison.text_diff_path}")
        print("- Result: zero unmasked byte mismatches after COFF relocation masking.")
    else:
        print(f"- Classified text diff: {result.comparison.classified_path}")
        print("- Result: zero instruction mismatches in legacy text comparison mode.")


def run_target(
    *,
    target: VerifyTarget,
    selected_functions: tuple[VerifyFunction, ...],
    build_root: Path,
    vc6_env: Path,
    bridge_url: str,
    skip_bn_compare: bool,
) -> int:
    try:
        build_dir = prepare_clean_build_dir(build_root, target.name)
    except ValueError as exc:
        print(exc, file=sys.stderr)
        return 2

    compiled, rc = compile_target(target=target, build_dir=build_dir, vc6_env=vc6_env)
    if rc != 0:
        return rc
    if compiled is None:
        return 3

    print_compiled_target_info(target, compiled)

    if skip_bn_compare:
        return 0

    selection = VerifySelection(target=target, functions=selected_functions)
    results, overall = compare_compiled_selections(
        compiled=compiled,
        selections=[selection],
        bridge_url=bridge_url,
    )
    print_verification_summary(results)
    print(f"Verification evidence: {compiled.build_dir / 'verify'}")
    for result in results:
        print_evidence_block(compiled, result)
    return overall


def run_batch(
    *,
    selections: list[VerifySelection],
    build_root: Path,
    vc6_env: Path,
    bridge_url: str,
    skip_bn_compare: bool,
) -> int:
    if not selections:
        print("No VC6 verification targets matched the requested batch.")
        return 2

    grouped = group_selections_by_compile_key(selections, vc6_env)
    total_targets = sum(len(group) for _key, group in grouped)
    total_functions = sum(len(selection.functions) for _key, group in grouped for selection in group)
    print(
        f"VC6 batch: {total_targets} target(s), {total_functions} function(s), "
        f"{len(grouped)} unique compile(s)."
    )

    overall = 0
    all_results: list[VerificationResult] = []
    for key, group in grouped:
        representative = group[0].target
        digest = compile_key_digest(key)
        group_name = f"_batch/{safe_path_component(representative.name)}_{digest}"
        try:
            build_dir = prepare_clean_build_dir(build_root, group_name)
        except ValueError as exc:
            print(exc, file=sys.stderr)
            return 2

        names = ", ".join(selection.target.name for selection in group)
        print()
        print(f"Compile group {digest}: {names}")
        compiled, rc = compile_target(target=representative, build_dir=build_dir, vc6_env=vc6_env)
        if rc != 0:
            overall = rc if overall == 0 else overall
            continue
        if compiled is None:
            overall = 3 if overall == 0 else overall
            continue
        print_compiled_target_info(representative, compiled)

        if skip_bn_compare:
            continue

        results, compare_rc = compare_compiled_selections(
            compiled=compiled,
            selections=group,
            bridge_url=bridge_url,
        )
        all_results.extend(results)
        if compare_rc:
            overall = compare_rc if overall == 0 else overall

    if not skip_bn_compare:
        print()
        print_verification_summary(all_results, include_target=True)
    return overall


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Compile VC6 verification targets and compare relocation-masked COFF bytes to BN."
    )
    parser.add_argument("target", nargs="?", help="Target manifest name or covered function address, e.g. zsys_cpu or 0x4b3510")
    parser.add_argument("--all", action="store_true", help="Run every VC6 verification target, grouping identical compiles.")
    parser.add_argument(
        "--source-from",
        metavar="PATH",
        help="Run every target whose source_from resolves to PATH, grouping identical compiles.",
    )
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
        if args.all or args.source_from:
            if args.target:
                parser.error("target cannot be combined with --all or --source-from")
            selections = selected_targets_for_all(manifests) if args.all else selected_targets_for_source_from(manifests, args.source_from)
            return run_batch(
                selections=selections,
                build_root=Path(args.build_root),
                vc6_env=Path(args.vc6_env),
                bridge_url=args.bridge_url,
                skip_bn_compare=args.skip_bn_compare,
            )
        if not args.target:
            parser.error("target is required unless --list, --explain-missing, --all, or --source-from is used")
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
