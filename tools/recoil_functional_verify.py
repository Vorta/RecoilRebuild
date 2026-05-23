from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
from pathlib import Path
import subprocess
import sys

from recoil_plan import normalize_address
from recoil_tooling import configure_stdio


REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST_DIR = REPO_ROOT / "tools" / "functional_verify_targets"
DEFAULT_SMOKE_CPP = REPO_ROOT / "tests" / "native" / "smoke.cpp"
SMOKE_RUNNER = REPO_ROOT / "tests" / "native" / "run_native_smokes.py"
DEFAULT_NATIVE_SMOKE_EXES = (
    REPO_ROOT / "build" / "ninja-x86-debug" / "tests" / "native" / "recoil_native_smoke.exe",
    REPO_ROOT / "build" / "ninja-x86-debug" / "tests" / "native" / "recoil_native_smoke",
)


@dataclass(frozen=True)
class FunctionalTarget:
    name: str
    description: str
    address: str
    source_from: str
    smoke_tests: tuple[str, ...]
    vc6_attempt: str
    known_limits: tuple[str, ...]
    binary_safe_evidence: tuple[str, ...]
    path: Path


def load_manifest(path: Path) -> FunctionalTarget:
    data = json.loads(path.read_text(encoding="utf-8"))
    name = str(data.get("name", "")).strip()
    address = normalize_address(str(data.get("address", "")))
    source_from = str(data.get("source_from", "")).strip()
    smoke_tests = tuple(str(item).strip() for item in data.get("smoke_tests", []) if str(item).strip())
    vc6_attempt = str(data.get("vc6_attempt", "")).strip()
    known_limits = tuple(str(item).strip() for item in data.get("known_limits", []) if str(item).strip())
    binary_safe_evidence = tuple(
        str(item).strip() for item in data.get("binary_safe_evidence", []) if str(item).strip()
    )
    description = str(data.get("description", "")).strip()

    if not name:
        raise ValueError(f"{path}: missing name")
    if not source_from:
        raise ValueError(f"{path}: missing source_from")
    if not smoke_tests:
        raise ValueError(f"{path}: smoke_tests must list at least one native smoke")
    if vc6_attempt and not vc6_attempt.startswith("python tools/recoil_vc6_verify.py"):
        raise ValueError(f"{path}: vc6_attempt must begin with 'python tools/recoil_vc6_verify.py'")
    if not known_limits and not binary_safe_evidence:
        raise ValueError(
            f"{path}: known_limits or binary_safe_evidence must list the binary-safety state"
        )
    return FunctionalTarget(
        name=name,
        description=description,
        address=address,
        source_from=source_from,
        smoke_tests=smoke_tests,
        vc6_attempt=vc6_attempt,
        known_limits=known_limits,
        binary_safe_evidence=binary_safe_evidence,
        path=path,
    )


def load_manifests(manifest_dir: Path) -> list[FunctionalTarget]:
    if not manifest_dir.exists():
        return []
    return [load_manifest(path) for path in sorted(manifest_dir.glob("*.json"))]


def find_target(targets: list[FunctionalTarget], query: str) -> FunctionalTarget:
    normalized_address = None
    try:
        normalized_address = normalize_address(query)
    except ValueError:
        pass

    matches = [
        target
        for target in targets
        if query == target.name or (normalized_address is not None and target.address == normalized_address)
    ]
    if not matches:
        raise ValueError(f"no functional verification target covers {query}")
    if len(matches) > 1:
        names = ", ".join(target.name for target in matches)
        raise ValueError(f"multiple functional verification targets match {query}: {names}")
    return matches[0]


def default_executable() -> Path | None:
    for candidate in DEFAULT_NATIVE_SMOKE_EXES:
        if candidate.exists():
            return candidate
    return None


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
    dry_run: bool = False,
) -> int:
    print(f"Functional target: {target.name}")
    print(f"Address: {target.address}")
    print(f"Source: {target.source_from}")
    print(f"Manifest: {target.path}")
    print(f"VC byte attempt: {target.vc6_attempt or 'not recorded'}")
    if target.known_limits:
        print("Known binary-safety limits:")
        for limit in target.known_limits:
            print(f"- {limit}")
    if target.binary_safe_evidence:
        print("Binary-safe evidence:")
        for evidence in target.binary_safe_evidence:
            print(f"- {evidence}")
    print()

    failures: list[str] = []
    for smoke_name in target.smoke_tests:
        command = smoke_command(executable, smoke_cpp, smoke_name)
        if dry_run:
            print(" ".join(command))
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
        else:
            print(f"[PASS] {smoke_name}")

    if dry_run:
        return 0
    if failures:
        print(f"{len(failures)} functional smoke(s) failed.")
        return 1

    print()
    print("Functional verification passed.")
    evidence = f"functional target {target.name} passed"
    if target.vc6_attempt:
        evidence += f"; reviewed {target.vc6_attempt}"
    print(
        "marker_command: "
        f"python tools/recoil_plan_cli.py set {target.address} functional ✅ "
        f"--target {target.name} --evidence \"{evidence}\""
    )
    return 0


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    parser = argparse.ArgumentParser(
        description="Run functional-equivalence smoke evidence for a reconstructed function."
    )
    parser.add_argument("target", nargs="?", help="Functional target name or original address.")
    parser.add_argument("--manifest-dir", default=str(DEFAULT_MANIFEST_DIR))
    parser.add_argument("--executable", type=Path, help="Path to recoil_native_smoke executable.")
    parser.add_argument("--smoke-cpp", type=Path, default=DEFAULT_SMOKE_CPP)
    parser.add_argument("--list", action="store_true", help="List functional verification targets.")
    parser.add_argument("--dry-run", action="store_true", help="Print smoke commands without running them.")
    args = parser.parse_args(argv)

    try:
        targets = load_manifests(Path(args.manifest_dir))
        if args.list:
            for target in targets:
                print(f"{target.address} {target.name} smokes={len(target.smoke_tests)}")
            return 0
        if not args.target:
            print("target is required unless --list is used", file=sys.stderr)
            return 2
        target = find_target(targets, args.target)
        executable = args.executable or default_executable()
        if executable is None:
            print(
                "native smoke executable not found; build with "
                "`cmake --preset ninja-x86-debug` and "
                "`cmake --build --preset ninja-x86-debug`, or pass --executable",
                file=sys.stderr,
            )
            return 2
        if not args.dry_run and not executable.exists():
            print(f"native smoke executable not found: {executable}", file=sys.stderr)
            return 2
        return run_target(
            target,
            executable=executable,
            smoke_cpp=args.smoke_cpp,
            dry_run=args.dry_run,
        )
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(str(exc), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
