#!/usr/bin/env python3
"""Fail on known incorrect Recoil source-subsystem placements."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".inl"}
CONFIG_SUFFIXES = {
    ".cmake",
    ".dsp",
    ".dsw",
    ".json",
    ".mak",
    ".mk",
    ".props",
    ".targets",
    ".toml",
    ".vcxproj",
    ".yaml",
    ".yml",
}
ROOT_CONFIG_FILES = (
    Path("CMakeLists.txt"),
    Path("CMakePresets.json"),
)
ACTIVE_CONFIG_TREES = (
    Path("cmake"),
    Path("tools/_recoil/config"),
    Path("tools/functional_verify_targets"),
    Path("tools/vc5_verify_targets"),
)
ACTIVE_CONFIG_FILES = (
    Path("tests/native/CMakeLists.txt"),
)
FORBIDDEN_SUBTREES = (
    Path("Battlesport/zModel"),
    Path("Battlesport/zUtil"),
)
FORBIDDEN_INCLUDES = (
    '#include "Battlesport/zUtil/zutil.h"',
    "#include <Battlesport/zUtil/zutil.h>",
)
AMBIGUOUS_PROVENANCE_LABEL_RE = re.compile(
    r"\bOriginal\s+(?:source\s+path|source\s+file|file)\s*:",
    re.IGNORECASE,
)
RETIRED_LAYOUT_PATHS = (
    "src/native",
    "src/Battlesport/Mfc42Abi.h",
    "src/GameZRecoil/zRndr",
    "src/GameZRecoil/zTurret",
    "src/GameZRecoil/wwonline",
    "src/GameZRecoil/RecoilApp",
    "src/GameZRecoil/mission.h",
)
RETIRED_LAYOUT_PATH_RES = tuple(
    (
        retired_path,
        re.compile(
            r"(?<![A-Za-z0-9_.-])"
            + re.escape(retired_path).replace("/", r"[/\\]")
            + (
                r"(?=$|[/\\]|[^A-Za-z0-9_.-])"
                if "." not in Path(retired_path).name
                else r"(?=$|[^A-Za-z0-9_.-])"
            ),
            re.IGNORECASE,
        ),
    )
    for retired_path in RETIRED_LAYOUT_PATHS
)


def display_path(path: Path, repo_root: Path) -> str:
    try:
        return path.relative_to(repo_root).as_posix()
    except ValueError:
        return path.as_posix()


def iter_source_files(root: Path) -> list[Path]:
    return sorted(
        path
        for path in root.rglob("*")
        if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES
    )


def is_config_file(path: Path) -> bool:
    return path.name == "CMakeLists.txt" or path.suffix.lower() in CONFIG_SUFFIXES


def iter_config_files(root: Path) -> list[Path]:
    if root.is_file():
        return [root] if is_config_file(root) else []
    if not root.is_dir():
        return []
    return sorted(path for path in root.rglob("*") if path.is_file() and is_config_file(path))


def iter_active_config_files(repo_root: Path, scan_root: Path) -> list[Path]:
    paths: set[Path] = set()
    for relative in ROOT_CONFIG_FILES + ACTIVE_CONFIG_FILES:
        candidate = repo_root / relative
        if candidate.is_file():
            paths.add(candidate)
    for relative in ACTIVE_CONFIG_TREES:
        paths.update(iter_config_files(repo_root / relative))
    paths.update(iter_config_files(scan_root))
    return sorted(paths)


def is_under_forbidden_subtree(path: Path, scan_root: Path) -> bool:
    try:
        relative = path.relative_to(scan_root)
    except ValueError:
        return False

    parts = relative.parts
    for subtree in FORBIDDEN_SUBTREES:
        subtree_parts = subtree.parts
        if len(parts) >= len(subtree_parts) and parts[: len(subtree_parts)] == subtree_parts:
            return True
    return False


def line_violations(
    rel: str,
    text: str,
    *,
    check_provenance_labels: bool,
) -> list[tuple[str, int, str, str]]:
    violations: list[tuple[str, int, str, str]] = []
    for line_no, line in enumerate(text.splitlines(), start=1):
        stripped = line.strip()
        if check_provenance_labels and AMBIGUOUS_PROVENANCE_LABEL_RE.search(line):
            violations.append(
                (rel, line_no, "ambiguous source-provenance label", stripped)
            )
        if stripped in FORBIDDEN_INCLUDES:
            violations.append((rel, line_no, "stale zUtil include path", stripped))
        for retired_path, pattern in RETIRED_LAYOUT_PATH_RES:
            if pattern.search(line):
                violations.append(
                    (
                        rel,
                        line_no,
                        "retired source-layout path",
                        f"{retired_path}: {stripped}",
                    )
                )
    return violations


def find_violations(scan_root: Path, repo_root: Path) -> list[tuple[str, int, str, str]]:
    violations: list[tuple[str, int, str, str]] = []
    source_files = iter_source_files(scan_root)
    source_file_set = set(source_files)
    for path in source_files:
        rel = display_path(path, repo_root)
        if is_under_forbidden_subtree(path, scan_root):
            violations.append((rel, 1, "misplaced Recoil engine source", rel))

        text = path.read_text(encoding="utf-8", errors="ignore")
        violations.extend(
            line_violations(rel, text, check_provenance_labels=True)
        )

    for path in iter_active_config_files(repo_root, scan_root):
        if path in source_file_set:
            continue
        rel = display_path(path, repo_root)
        text = path.read_text(encoding="utf-8", errors="ignore")
        violations.extend(
            line_violations(rel, text, check_provenance_labels=False)
        )

    return violations


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default="src", help="Source tree to scan.")
    args = parser.parse_args(argv)


    repo_root = Path.cwd()
    scan_root = (repo_root / args.root).resolve()
    violations = find_violations(scan_root, repo_root)
    if not violations:
        print("recoil_source_placement_guard passed")
        return 0

    print("recoil_source_placement_guard failed:", file=sys.stderr)
    for rel, line_no, reason, snippet in violations:
        print(f"{rel}:{line_no}: {reason}: {snippet}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
