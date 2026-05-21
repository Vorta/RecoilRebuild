#!/usr/bin/env python3
"""Run the common reconstruction process checks from one entry point."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import subprocess
import sys

from recoil_tooling import REPO_ROOT, hidden_creation_flags


@dataclass(frozen=True)
class DoctorStep:
    label: str
    command: tuple[str, ...]


def py(*args: str | Path) -> tuple[str, ...]:
    return (sys.executable, *(str(arg) for arg in args))


def quick_steps() -> list[DoctorStep]:
    tools = REPO_ROOT / "tools"
    return [
        DoctorStep("VC manifest source policy", py(tools / "recoil_vc6_manifest_source_guard.py")),
        DoctorStep("compiler/linker provenance", py(tools / "recoil_provenance_audit.py", "--strict")),
        DoctorStep("workspace hygiene", py(tools / "recoil_workspace_hygiene.py", "--strict")),
        DoctorStep(
            "raw image address guard",
            py(
                tools / "recoil_no_raw_image_addresses.py",
                "--root",
                "src",
                "--allowlist",
                ".agent/RAW_ADDRESS_ALLOWLIST.txt",
            ),
        ),
        DoctorStep(
            "raw assembly guard",
            py(
                tools / "recoil_no_raw_assembly.py",
                "--root",
                "src",
                "--allowlist",
                ".agent/RAW_ASSEMBLY_ALLOWLIST.txt",
            ),
        ),
        DoctorStep(
            "reinterpret_cast guard",
            py(
                tools / "recoil_no_reinterpret_cast.py",
                "--root",
                "src",
            ),
        ),
        DoctorStep(
            "reference executable manifest",
            py(
                tools / "recoil_pe_reference.py",
                "--reference",
                "support/Recoil.exe",
                "--manifest",
                ".agent/REFERENCE_EXECUTABLE.json",
                "--verify",
            ),
        ),
        DoctorStep(
            "source file map freshness",
            py(tools / "recoil_source_file_map.py", "--check", "docs/reconstruction/source_file_map.md"),
        ),
        DoctorStep("functional manifest load", py(tools / "recoil_functional_verify.py", "--list")),
        DoctorStep("VC manifest load", py(tools / "recoil_vc6_verify.py", "--list")),
        DoctorStep("Python tool tests", py("-m", "unittest", "discover", "-s", "tests/tools", "-p", "*_tests.py")),
    ]


def native_x86_steps() -> list[DoctorStep]:
    return [DoctorStep("native x86 environment", py(REPO_ROOT / "tools" / "recoil_env_check.py", "--native-x86"))]


def binja_steps() -> list[DoctorStep]:
    return [DoctorStep("Binary Ninja bridge", py(REPO_ROOT / "tools" / "recoil_binja_preflight.py", "--strict"))]


def active_steps(address: str, *, bn_compare: bool) -> list[DoctorStep]:
    tools = REPO_ROOT / "tools"
    verify_args: list[str | Path] = [tools / "recoil_vc6_verify.py", address, "--all-covering"]
    if not bn_compare:
        verify_args.append("--skip-bn-compare")
    return [
        DoctorStep(f"active status {address}", py(tools / "recoil_status.py", address)),
        DoctorStep(f"active VC {'byte verify' if bn_compare else 'compile'} {address}", py(*verify_args)),
    ]


def build_steps(args: argparse.Namespace) -> list[DoctorStep]:
    steps = quick_steps()
    if args.binja:
        steps.extend(binja_steps())
    if args.native_x86:
        steps.extend(native_x86_steps())
    if args.active:
        steps.extend(active_steps(args.active, bn_compare=args.bn_compare))
    return steps


def format_command(command: tuple[str, ...]) -> str:
    return " ".join(f'"{part}"' if any(char.isspace() for char in part) else part for part in command)


def run_step(step: DoctorStep, *, verbose: bool) -> int:
    completed = subprocess.run(
        step.command,
        cwd=REPO_ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        encoding="utf-8",
        errors="replace",
        creationflags=hidden_creation_flags(),
    )
    if completed.returncode == 0:
        print(f"OK: {step.label}")
        if verbose and completed.stdout:
            print(completed.stdout.rstrip())
        return 0

    print(f"FAIL: {step.label} (exit {completed.returncode})")
    if completed.stdout:
        print(completed.stdout.rstrip())
    return completed.returncode


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Run common Recoil reconstruction process checks.")
    parser.add_argument("--quick", action="store_true", help="Run the standard quick checks (default).")
    parser.add_argument("--binja", action="store_true", help="Also check the expected Binary Ninja bridge/database state.")
    parser.add_argument("--native-x86", action="store_true", help="Also check the current native x86 MSVC environment.")
    parser.add_argument("--active", metavar="ADDRESS", help="Also run status and VC verification for an active address.")
    parser.add_argument(
        "--bn-compare",
        action="store_true",
        help="Use full Binary Ninja byte comparison for --active instead of compile-only VC verification.",
    )
    parser.add_argument("--dry-run", action="store_true", help="Print the planned checks without running them.")
    parser.add_argument("--verbose", action="store_true", help="Print successful command output.")
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    steps = build_steps(args)

    if args.dry_run:
        for step in steps:
            print(f"{step.label}: {format_command(step.command)}")
        return 0

    print(f"Running {len(steps)} process check(s).")
    failures = 0
    for index, step in enumerate(steps, start=1):
        print(f"[{index}/{len(steps)}] {step.label}")
        if run_step(step, verbose=args.verbose) != 0:
            failures += 1

    if failures:
        print(f"\nDoctor failed: {failures} check(s) failed.")
        return 1
    print("\nDoctor passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
