#!/usr/bin/env python3
"""Run a command inside an x86 MSVC developer environment."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from recoil_tooling import REPO_ROOT, hidden_creation_flags


def candidate_vcvarsall_paths() -> list[Path]:
    candidates: list[Path] = []

    explicit = os.environ.get("RECOIL_VCVARSALL")
    if explicit:
        candidates.append(Path(explicit))

    vs_install_dir = os.environ.get("VSINSTALLDIR")
    if vs_install_dir:
        candidates.append(Path(vs_install_dir) / "VC" / "Auxiliary" / "Build" / "vcvarsall.bat")

    roots = [
        os.environ.get("ProgramFiles"),
        os.environ.get("ProgramFiles(x86)"),
    ]
    editions = ["Community", "Professional", "Enterprise", "BuildTools"]
    versions = ["18", "17", "2026", "2022"]
    for root_text in roots:
        if not root_text:
            continue
        root = Path(root_text) / "Microsoft Visual Studio"
        for version in versions:
            for edition in editions:
                candidates.append(root / version / edition / "VC" / "Auxiliary" / "Build" / "vcvarsall.bat")

    unique: list[Path] = []
    seen: set[str] = set()
    for candidate in candidates:
        key = str(candidate).lower()
        if key not in seen:
            seen.add(key)
            unique.append(candidate)
    return unique


def find_vcvarsall() -> Path:
    for candidate in candidate_vcvarsall_paths():
        if candidate.exists():
            return candidate
    searched = "\n".join(f"- {path}" for path in candidate_vcvarsall_paths())
    raise FileNotFoundError(
        "vcvarsall.bat was not found. Set RECOIL_VCVARSALL to the full path.\n"
        f"Searched:\n{searched}"
    )


def normalize_command(command: list[str]) -> list[str]:
    if command and command[0] == "--":
        command = command[1:]
    if not command:
        raise ValueError("missing command after --")
    return command


def build_script(vcvarsall: Path, command: list[str]) -> str:
    command_line = subprocess.list2cmdline(command)
    return (
        "@echo off\r\n"
        f"call {subprocess.list2cmdline([str(vcvarsall), 'x86'])} >nul\r\n"
        "if errorlevel 1 exit /b %errorlevel%\r\n"
        f"{command_line}\r\n"
        "exit /b %errorlevel%\r\n"
    )


def run_in_msvc_x86(command: list[str]) -> int:
    vcvarsall = find_vcvarsall()
    script = build_script(vcvarsall, command)
    comspec = os.environ.get("ComSpec", r"C:\Windows\System32\cmd.exe")
    with tempfile.TemporaryDirectory(prefix="recoil-msvc-x86-") as tmp_text:
        script_path = Path(tmp_text) / "run.cmd"
        script_path.write_text(script, encoding="utf-8")
        completed = subprocess.run(
            [comspec, "/d", "/c", str(script_path)],
            cwd=REPO_ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            encoding="utf-8",
            errors="replace",
            creationflags=hidden_creation_flags(),
        )
    if completed.stdout:
        print(completed.stdout, end="")
    if completed.stderr:
        print(completed.stderr, end="", file=sys.stderr)
    return completed.returncode


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Run a command after loading vcvarsall x86 for Recoil native build work."
    )
    parser.add_argument("command", nargs=argparse.REMAINDER, help="Command to run, usually after --.")
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        return run_in_msvc_x86(normalize_command(args.command))
    except (FileNotFoundError, ValueError) as exc:
        print(str(exc), file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
