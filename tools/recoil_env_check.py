#!/usr/bin/env python3
"""Check local reconstruction environment prerequisites."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

from recoil_tooling import REPO_ROOT, hidden_creation_flags


class EnvReport:
    def __init__(self) -> None:
        self.failures = 0
        self.warnings = 0

    def ok(self, message: str) -> None:
        print(f"OK: {message}")

    def warn(self, message: str) -> None:
        self.warnings += 1
        print(f"WARN: {message}")

    def fail(self, message: str) -> None:
        self.failures += 1
        print(f"FAIL: {message}")


def split_path_env(value: str) -> list[Path]:
    return [Path(item) for item in value.split(os.pathsep) if item]


def find_case_insensitive_file(directories: list[Path], filename: str) -> Path | None:
    target = filename.lower()
    for directory in directories:
        candidate = directory / filename
        if candidate.exists():
            return candidate
        if not directory.exists():
            continue
        try:
            for child in directory.iterdir():
                if child.name.lower() == target:
                    return child
        except OSError:
            continue
    return None


def require_path(report: EnvReport, path: Path, description: str) -> None:
    if path.exists():
        report.ok(f"{description}: {path}")
    else:
        report.fail(f"missing {description}: {path}")


def run_x86_compile_smoke(report: EnvReport, cl_path: str) -> None:
    source = (
        "#include <stddef.h>\n"
        "#include <windows.h>\n"
        "#ifndef _M_IX86\n"
        "#error Expected an x86 MSVC compiler target.\n"
        "#endif\n"
        "int main(void) { return sizeof(void*) == 4 ? 0 : 1; }\n"
    )
    with tempfile.TemporaryDirectory(prefix="recoil-env-check-") as tmp_text:
        tmp = Path(tmp_text)
        source_path = tmp / "native_x86_check.c"
        obj_path = tmp / "native_x86_check.obj"
        source_path.write_text(source, encoding="ascii")
        completed = subprocess.run(
            [
                cl_path,
                "/nologo",
                "/W3",
                "/TC",
                "/c",
                str(source_path),
                f"/Fo{obj_path}",
            ],
            cwd=str(tmp),
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            creationflags=hidden_creation_flags(),
        )
    if completed.returncode == 0:
        report.ok("MSVC compile smoke can include stddef.h/windows.h and targets x86")
        return

    output = "\n".join(
        line for line in (completed.stdout + completed.stderr).splitlines() if line.strip()
    )
    if output:
        report.fail(f"MSVC x86 compile smoke failed:\n{output}")
    else:
        report.fail("MSVC x86 compile smoke failed without compiler output")


def check_native_x86() -> int:
    report = EnvReport()

    cl_path = shutil.which("cl.exe") or shutil.which("cl")
    if cl_path:
        report.ok(f"cl.exe on PATH: {cl_path}")
    else:
        report.fail("cl.exe is not on PATH; run from an x86 MSVC Developer PowerShell")

    include_env = os.environ.get("INCLUDE", "")
    include_dirs = split_path_env(include_env)
    if include_dirs:
        report.ok(f"INCLUDE has {len(include_dirs)} entries")
    else:
        report.fail("INCLUDE is empty; MSVC standard and Windows SDK headers are unavailable")

    lib_env = os.environ.get("LIB", "")
    lib_dirs = split_path_env(lib_env)
    if lib_dirs:
        report.ok(f"LIB has {len(lib_dirs)} entries")
    else:
        report.fail("LIB is empty; MSVC and Windows SDK libraries are unavailable")

    stddef_path = find_case_insensitive_file(include_dirs, "stddef.h")
    if stddef_path:
        report.ok(f"stddef.h found: {stddef_path}")
    else:
        report.fail("stddef.h not found in INCLUDE")

    windows_path = find_case_insensitive_file(include_dirs, "windows.h")
    if windows_path:
        report.ok(f"windows.h found: {windows_path}")
    else:
        report.fail("windows.h not found in INCLUDE")

    kernel32_path = find_case_insensitive_file(lib_dirs, "kernel32.lib")
    if kernel32_path:
        report.ok(f"kernel32.lib found: {kernel32_path}")
    else:
        report.fail("kernel32.lib not found in LIB")

    target_arch = os.environ.get("VSCMD_ARG_TGT_ARCH", "")
    if target_arch:
        if target_arch.lower() == "x86":
            report.ok("VSCMD_ARG_TGT_ARCH=x86")
        else:
            report.fail(f"VSCMD_ARG_TGT_ARCH={target_arch}; expected x86")
    else:
        report.warn("VSCMD_ARG_TGT_ARCH is not set; using compile smoke to prove target")

    if cl_path:
        run_x86_compile_smoke(report, cl_path)

    require_path(
        report,
        REPO_ROOT / "support" / "Recoil.exe",
        "private reference executable",
    )
    require_path(
        report,
        REPO_ROOT / "support" / "sdk" / "DirectX_Aug2007" / "Include" / "dplay.h",
        "DirectX August 2007 dplay.h",
    )
    require_path(
        report,
        REPO_ROOT / "support" / "sdk" / "DirectX_Aug2007" / "Lib" / "x86" / "dplayx.lib",
        "DirectX August 2007 x86 dplayx.lib",
    )
    require_path(
        report,
        REPO_ROOT / "support" / "sdk" / "MFC42" / "Include" / "AFXWIN.H",
        "MFC42 AFXWIN.H",
    )
    require_path(
        report,
        REPO_ROOT / "support" / "sdk" / "MFC42" / "Lib" / "x86" / "MFC42.LIB",
        "MFC42 x86 import library",
    )
    require_path(
        report,
        REPO_ROOT / "support" / "sdk" / "MFC42" / "Src" / "APPCORE.CPP",
        "MFC42 source evidence",
    )
    require_path(
        report,
        REPO_ROOT / "support" / "mfc42.dll",
        "MFC42 runtime DLL",
    )

    if report.failures:
        print(f"\nEnvironment check failed: {report.failures} failure(s), {report.warnings} warning(s).")
        return 1
    print(f"\nEnvironment check passed with {report.warnings} warning(s).")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Check Recoil reconstruction environment.")
    parser.add_argument(
        "--native-x86",
        action="store_true",
        help="Check prerequisites for the CMake x86 native smoke build.",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    if args.native_x86:
        return check_native_x86()
    parser.error("select a check, for example --native-x86")
    return 2


if __name__ == "__main__":
    sys.exit(main())
