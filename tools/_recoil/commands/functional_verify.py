from __future__ import annotations

import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import argparse
from dataclasses import dataclass
import json
from pathlib import Path
import re
import subprocess
import sys

from _recoil.lib.owner_entries import normalize_address
from _recoil.lib.target_binary import validated_target_binary
from _recoil.lib.tooling import REPO_ROOT, configure_stdio


DEFAULT_MANIFEST_DIR = REPO_ROOT / "tools" / "functional_verify_targets"
DEFAULT_SMOKE_CPP = REPO_ROOT / "tests" / "native" / "smoke.cpp"
TARGET_ID_RE = re.compile(r"^[a-z0-9_]+$")
SMOKE_RUNNER = REPO_ROOT / "tests" / "native" / "run_native_smokes.py"
CANONICAL_NATIVE_PRESET = "ninja-x86-debug"
CANONICAL_NATIVE_BUILD_SCRIPT = REPO_ROOT / "cmake" / "recoil_native_x86_build.ps1"
CANONICAL_NATIVE_SMOKE_EXE = (
    REPO_ROOT
    / "build"
    / CANONICAL_NATIVE_PRESET
    / "tests"
    / "native"
    / "recoil_native_smoke.exe"
)
VC5_ATTEMPT_PREFIXES = (
    "python tools/recoil.py verify vc5",
)


@dataclass(frozen=True)
class FunctionalTarget:
    name: str
    description: str
    target_binary: str
    address: str
    covered_addresses: tuple[str, ...]
    source_from: str
    smoke_tests: tuple[str, ...]
    vc5_attempt: str
    known_limits: tuple[str, ...]
    tier_s_evidence: tuple[str, ...]
    path: Path


def load_manifest(path: Path) -> FunctionalTarget:
    data = json.loads(path.read_text(encoding="utf-8"))
    name = str(data.get("name", "")).strip()
    address = normalize_address(str(data.get("address", "")))
    covered_addresses = tuple(
        dict.fromkeys(
            [
                address,
                *(
                    normalize_address(str(item))
                    for item in data.get("covered_addresses", [])
                    if str(item).strip()
                ),
            ]
        )
    )
    source_from = str(data.get("source_from", "")).strip()
    target_binary = validated_target_binary(
        source_from=source_from,
        addresses=covered_addresses,
        explicit=data.get("target_binary"),
        context=str(path),
    )
    smoke_tests = tuple(str(item).strip() for item in data.get("smoke_tests", []) if str(item).strip())
    vc5_attempt = str(data.get("vc5_attempt", "")).strip()
    known_limits = tuple(str(item).strip() for item in data.get("known_limits", []) if str(item).strip())
    retired_tier_key = "binary_" "safe_evidence"
    if retired_tier_key in data:
        raise ValueError(f"{path}: retired tier-S evidence key is no longer accepted; use tier_s_evidence")
    tier_s_evidence = tuple(str(item).strip() for item in data.get("tier_s_evidence", []) if str(item).strip())
    description = str(data.get("description", "")).strip()

    if not name:
        raise ValueError(f"{path}: missing name")
    if name != path.stem:
        raise ValueError(
            f"{path}: name must match the functional verification target id from the filename stem: {path.stem}"
        )
    if TARGET_ID_RE.fullmatch(name) is None:
        raise ValueError(f"{path}: functional verification target id must be lowercase snake_case: {name}")
    if not source_from:
        raise ValueError(f"{path}: missing source_from")
    if not smoke_tests:
        raise ValueError(f"{path}: smoke_tests must list at least one native smoke")
    if vc5_attempt and not any(vc5_attempt.startswith(prefix) for prefix in VC5_ATTEMPT_PREFIXES):
        expected = "' or '".join(VC5_ATTEMPT_PREFIXES)
        raise ValueError(f"{path}: vc5_attempt must begin with '{expected}'")
    if not known_limits and not tier_s_evidence:
        raise ValueError(f"{path}: known_limits or tier_s_evidence must list the tier S state")
    return FunctionalTarget(
        name=name,
        description=description,
        target_binary=target_binary,
        address=address,
        covered_addresses=covered_addresses,
        source_from=source_from,
        smoke_tests=smoke_tests,
        vc5_attempt=vc5_attempt,
        known_limits=known_limits,
        tier_s_evidence=tier_s_evidence,
        path=path,
    )


def load_manifests(manifest_dir: Path) -> list[FunctionalTarget]:
    if not manifest_dir.exists():
        return []
    return [load_manifest(path) for path in sorted(manifest_dir.glob("*.json"))]


def target_covers_address(target: FunctionalTarget, address: str) -> bool:
    return normalize_address(address) in target.covered_addresses


def find_target(targets: list[FunctionalTarget], query: str) -> FunctionalTarget:
    normalized_address = None
    try:
        normalized_address = normalize_address(query)
    except ValueError:
        pass

    matches = [
        target
        for target in targets
        if query == target.name
        or (normalized_address is not None and normalized_address in target.covered_addresses)
    ]
    if not matches:
        raise ValueError(f"no tier C verification target covers {query}")
    if len(matches) > 1:
        names = ", ".join(target.name for target in matches)
        raise ValueError(f"multiple functional verification targets match {query}: {names}")
    return matches[0]


def canonical_build_commands() -> tuple[tuple[str, list[str]], ...]:
    return (
        (
            "configure",
            [
                "powershell",
                "-NoProfile",
                "-ExecutionPolicy",
                "Bypass",
                "-File",
                str(CANONICAL_NATIVE_BUILD_SCRIPT),
                "-Preset",
                CANONICAL_NATIVE_PRESET,
                "-ConfigureOnly",
            ],
        ),
        (
            "target build",
            [
                sys.executable,
                str(REPO_ROOT / "tools" / "recoil.py"),
                "build",
                "msvc-x86",
                "--",
                "cmake",
                "--build",
                "--preset",
                CANONICAL_NATIVE_PRESET,
                "--target",
                "recoil_native_smoke",
            ],
        ),
    )


def resolve_executable(executable: Path | None, *, dry_run: bool) -> Path:
    if executable is not None:
        print(
            "Diagnostic-only executable override (canonical current-source build bypassed): "
            f"{executable}"
        )
        return executable

    commands = canonical_build_commands()
    print("Canonical current-source x86 native smoke build:")
    for label, command in commands:
        print(f"{label}: {subprocess.list2cmdline(command)}")
    if dry_run:
        return CANONICAL_NATIVE_SMOKE_EXE

    for label, command in commands:
        result = subprocess.run(
            command,
            cwd=REPO_ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        if result.returncode != 0:
            if result.stdout:
                print(result.stdout, end="" if result.stdout.endswith("\n") else "\n")
            raise RuntimeError(
                f"canonical current-source x86 recoil_native_smoke {label} failed "
                f"with exit {result.returncode}"
            )
    if not CANONICAL_NATIVE_SMOKE_EXE.exists():
        raise RuntimeError(
            "canonical current-source x86 build completed but did not produce "
            f"{CANONICAL_NATIVE_SMOKE_EXE}"
        )
    print("[PASS] Canonical current-source x86 recoil_native_smoke build")
    return CANONICAL_NATIVE_SMOKE_EXE


def smoke_command(executable: Path, smoke_cpp: Path, smoke_name: str) -> list[str]:
    return [
        sys.executable,
        str(SMOKE_RUNNER),
        str(executable),
        "--smoke-cpp",
        str(smoke_cpp),
        "--only",
        smoke_name,
    ]


def run_target(
    target: FunctionalTarget,
    *,
    executable: Path,
    smoke_cpp: Path,
    marker_address: str | None = None,
    dry_run: bool = False,
    diagnostic_only: bool = False,
) -> int:
    marker_address = marker_address or target.address
    print(f"Functional target id: {target.name}")
    print(f"Target binary: {target.target_binary}")
    print(f"Address: {target.address}")
    if len(target.covered_addresses) > 1:
        print("Covered addresses: " + ", ".join(target.covered_addresses))
    print(f"Source: {target.source_from}")
    print(f"Manifest: {target.path}")
    print(
        "Execution mode: "
        + (
            "diagnostic-only existing executable (not current-source functional evidence)"
            if diagnostic_only
            else "canonical current-source x86 build"
        )
    )
    print(f"VC byte attempt: {target.vc5_attempt or 'not recorded'}")
    if target.known_limits:
        print("Known tier S limits:")
        for limit in target.known_limits:
            print(f"- {limit}")
    if target.tier_s_evidence:
        print("Tier S evidence:")
        for evidence in target.tier_s_evidence:
            print(f"- {evidence}")
    print()

    failures: list[str] = []
    smoke_results: list[dict[str, object]] = []
    for smoke_name in target.smoke_tests:
        command = smoke_command(executable, smoke_cpp, smoke_name)
        if dry_run:
            print(" ".join(command))
            smoke_results.append({"name": smoke_name, "passed": False, "dry_run": True})
            continue
        result = subprocess.run(
            command,
            cwd=REPO_ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        if result.returncode != 0:
            print(f"[FAIL] {smoke_name}: exit {result.returncode}")
            if result.stdout:
                print(result.stdout, end="" if result.stdout.endswith("\n") else "\n")
            failures.append(smoke_name)
            smoke_results.append({"name": smoke_name, "passed": False, "exit_code": result.returncode})
        else:
            print(f"[PASS] {smoke_name}")
            smoke_results.append({"name": smoke_name, "passed": True, "exit_code": 0})

    if dry_run:
        owner_follow_up = (
            "dry_run: no canonical build or functional smoke executed; "
            "do not record this plan as functional evidence or use it for owner acceptance."
        )
    elif diagnostic_only:
        owner_follow_up = (
            "diagnostic_only: --executable bypassed the canonical current-source x86 build; "
            "do not record this result as functional evidence or use it for owner acceptance."
        )
    else:
        evidence = f"functional target id {target.name} passed"
        if target.vc5_attempt:
            evidence += f"; reviewed {target.vc5_attempt}"
        owner_follow_up = (
            "parent_follow_up: review unified progress owner gates and address metadata; "
            f"record target={target.name!r} and evidence={evidence!r} only through "
            "`python tools/recoil.py progress owner ...` after owner acceptance checks."
        )

    if dry_run:
        print(owner_follow_up)
        return 0
    if failures:
        print(f"{len(failures)} functional smoke(s) failed.")
        return 1

    print()
    if diagnostic_only:
        print("Diagnostic functional run passed; this is not current-source functional evidence.")
    else:
        print("Functional verification passed.")
    print(owner_follow_up)
    return 0


def resolve_target(targets: list[FunctionalTarget], query: str) -> tuple[FunctionalTarget, str]:
    target = find_target(targets, query)
    marker_address = target.address
    try:
        normalized_target = normalize_address(query)
    except ValueError:
        normalized_target = ""
    if normalized_target and target_covers_address(target, normalized_target):
        marker_address = normalized_target
    return target, marker_address


def parse_targets_json(payload: str) -> list[str]:
    data = json.loads(payload)
    if not isinstance(data, list):
        raise ValueError("--targets-json must be a JSON array")
    targets = [str(item).strip() for item in data if str(item).strip()]
    if len(targets) != len(data):
        raise ValueError("--targets-json entries must be non-empty target strings")
    return targets


def run_batch(
    queries: list[str],
    *,
    targets: list[FunctionalTarget],
    executable: Path,
    smoke_cpp: Path,
    dry_run: bool = False,
    diagnostic_only: bool = False,
) -> int:
    failures = 0
    summary: list[tuple[str, str, int]] = []
    for index, query in enumerate(queries, start=1):
        if index > 1:
            print()
        print(f"== functional batch target {index}/{len(queries)}: {query} ==")
        try:
            target, marker_address = resolve_target(targets, query)
        except ValueError as exc:
            print(f"[FAIL] {query}: {exc}")
            failures += 1
            summary.append((query, "-", 1))
            continue
        result = run_target(
            target,
            executable=executable,
            smoke_cpp=smoke_cpp,
            marker_address=marker_address,
            dry_run=dry_run,
            diagnostic_only=diagnostic_only,
        )
        if result != 0:
            failures += 1
        summary.append((query, target.name, result))

    print()
    print("Functional batch summary:")
    for query, target_name, result in summary:
        status = "DRY-RUN" if dry_run and result == 0 else ("PASS" if result == 0 else "FAIL")
        print(f"[{status}] {query} -> {target_name}")
    if failures:
        print(f"{failures} of {len(queries)} functional target(s) failed.")
        return 1
    print(f"{len(queries)} functional target(s) {'prepared' if dry_run else 'passed'}.")
    return 0


def main_batch(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Run tier C smoke evidence for multiple reconstructed functions."
    )
    parser.add_argument("targets", nargs="*", help="Functional verification target ids or original addresses.")
    parser.add_argument("--targets-json", help="JSON array of target ids or original addresses.")
    parser.add_argument("--manifest-dir", default=str(DEFAULT_MANIFEST_DIR))
    parser.add_argument(
        "--executable",
        type=Path,
        help="Diagnostic-only existing recoil_native_smoke path; bypasses the canonical current-source build.",
    )
    parser.add_argument("--smoke-cpp", type=Path, default=DEFAULT_SMOKE_CPP)
    parser.add_argument("--list", action="store_true", help="List functional verification targets.")
    parser.add_argument("--dry-run", action="store_true", help="Print smoke commands without running them.")
    args = parser.parse_args(argv)

    try:
        targets = load_manifests(Path(args.manifest_dir))
        if args.list:
            for target in targets:
                print(
                    f"{target.address} {target.name} binary={target.target_binary} "
                    f"smokes={len(target.smoke_tests)}"
                )
            return 0
        queries = list(args.targets)
        if args.targets_json:
            queries.extend(parse_targets_json(args.targets_json))
        if not queries:
            print("at least one target is required unless --list is used", file=sys.stderr)
            return 2
        executable = resolve_executable(args.executable, dry_run=args.dry_run)
        if not args.dry_run and not executable.exists():
            print(f"native smoke executable not found: {executable}", file=sys.stderr)
            return 2
        return run_batch(
            queries,
            targets=targets,
                executable=executable,
                smoke_cpp=args.smoke_cpp,
                dry_run=args.dry_run,
                diagnostic_only=args.executable is not None,
            )
    except (OSError, RuntimeError, ValueError, json.JSONDecodeError) as exc:
        print(str(exc), file=sys.stderr)
        return 1


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    argv = list(sys.argv[1:] if argv is None else argv)
    if argv[:1] == ["batch"]:
        return main_batch(argv[1:])
    parser = argparse.ArgumentParser(
        description="Run tier C smoke evidence for a reconstructed function."
    )
    parser.add_argument("target", nargs="?", help="Functional verification target id or original address.")
    parser.add_argument("--targets-json", help="JSON array of target ids or original addresses.")
    parser.add_argument("--manifest-dir", default=str(DEFAULT_MANIFEST_DIR))
    parser.add_argument(
        "--executable",
        type=Path,
        help="Diagnostic-only existing recoil_native_smoke path; bypasses the canonical current-source build.",
    )
    parser.add_argument("--smoke-cpp", type=Path, default=DEFAULT_SMOKE_CPP)
    parser.add_argument("--list", action="store_true", help="List functional verification targets.")
    parser.add_argument("--dry-run", action="store_true", help="Print smoke commands without running them.")
    args = parser.parse_args(argv)

    try:
        targets = load_manifests(Path(args.manifest_dir))
        if args.list:
            for target in targets:
                print(
                    f"{target.address} {target.name} binary={target.target_binary} "
                    f"smokes={len(target.smoke_tests)}"
                )
            return 0
        if args.target and args.targets_json:
            print("use either positional target or --targets-json, not both", file=sys.stderr)
            return 2
        if args.targets_json:
            queries = parse_targets_json(args.targets_json)
            executable = resolve_executable(args.executable, dry_run=args.dry_run)
            if not args.dry_run and not executable.exists():
                print(f"native smoke executable not found: {executable}", file=sys.stderr)
                return 2
            return run_batch(
                queries,
                targets=targets,
                executable=executable,
                smoke_cpp=args.smoke_cpp,
                dry_run=args.dry_run,
                diagnostic_only=args.executable is not None,
            )
        if not args.target:
            print("target is required unless --list or --targets-json is used", file=sys.stderr)
            return 2
        target, marker_address = resolve_target(targets, args.target)
        executable = resolve_executable(args.executable, dry_run=args.dry_run)
        if not args.dry_run and not executable.exists():
            print(f"native smoke executable not found: {executable}", file=sys.stderr)
            return 2
        return run_target(
            target,
            executable=executable,
            smoke_cpp=args.smoke_cpp,
            marker_address=marker_address,
            dry_run=args.dry_run,
            diagnostic_only=args.executable is not None,
        )
    except (OSError, RuntimeError, ValueError, json.JSONDecodeError) as exc:
        print(str(exc), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
