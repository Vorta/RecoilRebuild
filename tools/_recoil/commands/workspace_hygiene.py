#!/usr/bin/env python3
"""Detect generated artifacts inside authored workspace surfaces."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import sys

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from _recoil.lib.tooling import REPO_ROOT, configure_stdio, display_path


AUTHORED_SCAN_ROOTS = (".codex", "docs", "src", "tests", "tools")
AUTHORED_ROOT_FILES = (
    "AGENTS.md",
    "README.md",
)
MACHINE_LOCAL_OR_OUTPUT_ROOTS = frozenset(
    {
        ".agent",
        ".devspace",
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
)
ARTIFACT_SUFFIXES = frozenset(
    {
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
        ".pyc",
        ".res",
        ".rsp",
        ".sbr",
        ".sdf",
        ".tlb",
        ".tli",
        ".tlh",
        ".tmp_proj",
    }
)
ARTIFACT_NAMES = frozenset({"NUL", "NUL.obj", "NUL.pdb"})
IGNORED_TRANSIENT_DIRECTORY_NAMES = frozenset({"__pycache__"})


def is_upgrade_log(path: Path) -> bool:
    name = path.name.casefold()
    return name.startswith("upgradelog") and name.endswith((".htm", ".html", ".xml"))


def is_generated_artifact(path: Path) -> bool:
    return (
        path.name in ARTIFACT_NAMES
        or path.suffix.casefold() in ARTIFACT_SUFFIXES
        or is_upgrade_log(path)
    )


def _scan_authored_tree(root: Path, logical_root: str) -> list[Path]:
    physical_root = root / logical_root
    if not physical_root.is_dir():
        return []
    offenders: list[Path] = []
    pending = [physical_root]
    while pending:
        directory = pending.pop()
        try:
            entries = os.scandir(directory)
        except OSError:
            offenders.append(directory)
            continue
        with entries:
            for entry in entries:
                path = Path(entry.path)
                try:
                    if entry.is_dir(follow_symlinks=False):
                        if entry.name not in IGNORED_TRANSIENT_DIRECTORY_NAMES:
                            pending.append(path)
                    elif entry.is_file(follow_symlinks=False) and is_generated_artifact(path):
                        offenders.append(path)
                except OSError:
                    offenders.append(path)
    return offenders


def find_offenders(root: Path = REPO_ROOT) -> list[Path]:
    """Scan only authored roots and declared root files.

    Machine-local, support, repository-control, and output roots are outside the
    hygiene surface and are never traversed. In particular, ``.devspace`` is
    wholly machine-local; no receipt or scratch-manifest parsing is performed.
    """

    root = root.resolve()
    offenders: list[Path] = []
    for logical_root in AUTHORED_SCAN_ROOTS:
        if logical_root in MACHINE_LOCAL_OR_OUTPUT_ROOTS:
            raise RuntimeError(f"authored hygiene root is machine-local: {logical_root}")
        offenders.extend(_scan_authored_tree(root, logical_root))
    for name in AUTHORED_ROOT_FILES:
        path = root / name
        if path.is_file() and is_generated_artifact(path):
            offenders.append(path)
    return sorted(set(offenders), key=lambda path: str(path).casefold())


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Reject generated compiler/IDE artifacts in authored workspace roots."
    )
    parser.add_argument("--summary", action="store_true")
    parser.add_argument("--strict", action="store_true")
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    offenders = find_offenders()
    if args.summary:
        print(f"workspace hygiene offenders: {len(offenders)}")
    if offenders:
        print("Generated artifacts in authored workspace roots:")
        for path in offenders:
            print(f"- {display_path(path, REPO_ROOT)}")
    else:
        print("Workspace hygiene OK: authored roots contain no generated artifacts.")
    return 1 if args.strict and offenders else 0


if __name__ == "__main__":
    raise SystemExit(main())
