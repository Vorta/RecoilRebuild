#!/usr/bin/env python3
"""Fail on new reinterpret_cast use in production source."""

from __future__ import annotations

import argparse
from collections import Counter
import re
import sys

from recoil_tooling import REPO_ROOT, display_path, iter_source_files, strip_comments_and_strings

REINTERPRET_CAST_RE = re.compile(r"\breinterpret_cast\s*<")


def find_occurrences(scan_root, repo_root) -> list[tuple[str, int, str]]:
    locations: list[tuple[str, int, str]] = []

    for path in iter_source_files(scan_root):
        text = path.read_text(encoding="utf-8", errors="ignore")
        stripped = strip_comments_and_strings(text)
        lines = text.splitlines()
        rel = display_path(path, repo_root, fallback_root=scan_root)

        for match in REINTERPRET_CAST_RE.finditer(stripped):
            line_no = stripped.count("\n", 0, match.start()) + 1
            line = lines[line_no - 1].strip()
            locations.append((rel, line_no, line))

    return locations


def subsystem_for_path(rel: str) -> str:
    parts = rel.replace("\\", "/").split("/")
    if len(parts) >= 3 and parts[0] == "src":
        return "/".join(parts[:3])
    if len(parts) >= 2:
        return "/".join(parts[:2])
    return rel


def print_summary(
    *,
    locations: list[tuple[str, int, str]],
    top: int,
) -> None:
    print("reinterpret_cast summary:")
    print(f"- current occurrences: {len(locations)}")

    by_file: Counter[str] = Counter()
    by_subsystem: Counter[str] = Counter()
    for rel, _line_no, _line in locations:
        by_file[rel] += 1
        by_subsystem[subsystem_for_path(rel)] += 1

    print(f"- top files (limit {top}):")
    if by_file:
        for rel, count in by_file.most_common(top):
            print(f"  {count:4}  {rel}")
    else:
        print("     0  <none>")

    print(f"- top subsystems (limit {top}):")
    if by_subsystem:
        for subsystem, count in by_subsystem.most_common(top):
            print(f"  {count:4}  {subsystem}")
    else:
        print("     0  <none>")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default="src", help="production source root to scan")
    parser.add_argument("--summary", action="store_true", help="print current reinterpret_cast usage")
    parser.add_argument("--top", type=int, default=10, help="number of files/subsystems to print with --summary")
    args = parser.parse_args()

    repo_root = REPO_ROOT
    scan_root = (repo_root / args.root).resolve()

    locations = find_occurrences(scan_root, repo_root)
    if args.summary:
        print_summary(locations=locations, top=max(args.top, 0))

    if locations:
        if args.summary:
            print()
        print("reinterpret_cast is not allowed in production source.")
        print("Remove the cast from src before continuing reconstruction work.")
        print()
        for rel, line_no, line in locations:
            print(f"{rel}:{line_no}: {line}")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
