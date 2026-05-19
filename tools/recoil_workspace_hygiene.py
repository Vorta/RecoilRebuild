#!/usr/bin/env python3
"""Detect generated compiler/IDE artifacts outside approved local output roots."""

from __future__ import annotations

import argparse
from pathlib import Path
import sys

from recoil_tooling import REPO_ROOT, configure_stdio, display_path


DEFAULT_ALLOWED_ROOTS = {
    ".git",
    ".vs",
    "_scratch",
    "artifacts",
    "build",
    "Debug",
    "DebugPublic",
    "Release",
    "Releases",
    "support",
    "temp",
    "x64",
    "x86",
}

ARTIFACT_SUFFIXES = {
    ".aps",
    ".binlog",
    ".cache",
    ".cod",
    ".coverage",
    ".coveragexml",
    ".exp",
    ".iobj",
    ".ilk",
    ".ipdb",
    ".lib",
    ".map",
    ".meta",
    ".ncb",
    ".obj",
    ".opendb",
    ".opensdf",
    ".pch",
    ".pdb",
    ".pgc",
    ".pgd",
    ".plg",
    ".res",
    ".rsp",
    ".sbr",
    ".sdf",
    ".tlb",
    ".tli",
    ".tlh",
    ".tmp_proj",
}

ARTIFACT_NAMES = {
    "NUL",
    "NUL.obj",
    "NUL.pdb",
}


def is_upgrade_log(path: Path) -> bool:
    name = path.name.lower()
    return name.startswith("upgradelog") and name.endswith((".htm", ".html", ".xml"))


def is_generated_artifact(path: Path) -> bool:
    if path.name in ARTIFACT_NAMES:
        return True
    if path.suffix.lower() in ARTIFACT_SUFFIXES:
        return True
    return is_upgrade_log(path)


def is_under_allowed_root(path: Path, root: Path, allowed_roots: set[str]) -> bool:
    try:
        relative = path.relative_to(root)
    except ValueError:
        return False
    if not relative.parts:
        return False
    return relative.parts[0] in allowed_roots


def find_offenders(root: Path, allowed_roots: set[str]) -> list[Path]:
    offenders: list[Path] = []
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        if is_under_allowed_root(path, root, allowed_roots):
            continue
        if is_generated_artifact(path):
            offenders.append(path)
    return sorted(offenders)


def parse_allowed_roots(values: list[str]) -> set[str]:
    result = set(DEFAULT_ALLOWED_ROOTS)
    result.update(value.strip("/\\") for value in values if value.strip("/\\"))
    return result


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Reject generated compiler/IDE artifacts outside approved output roots."
    )
    parser.add_argument("--root", default=str(REPO_ROOT), help="Repository root to scan.")
    parser.add_argument(
        "--allow-root",
        action="append",
        default=[],
        help="Additional top-level directory allowed to contain generated artifacts.",
    )
    parser.add_argument("--summary", action="store_true", help="Print a compact summary.")
    parser.add_argument("--strict", action="store_true", help="Return nonzero when artifacts are found.")
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    parser = build_parser()
    args = parser.parse_args(argv)
    root = Path(args.root).resolve()
    allowed_roots = parse_allowed_roots(args.allow_root)
    offenders = find_offenders(root, allowed_roots)

    if args.summary:
        print(f"workspace hygiene offenders: {len(offenders)}")
    if offenders:
        print("Generated artifacts outside approved output roots:")
        for path in offenders:
            print(f"- {display_path(path, root)}")
    else:
        print("Workspace hygiene OK: no generated compiler/IDE artifacts outside approved output roots.")

    return 1 if args.strict and offenders else 0


if __name__ == "__main__":
    raise SystemExit(main())
