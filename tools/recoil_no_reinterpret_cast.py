#!/usr/bin/env python3
"""Fail on new reinterpret_cast use in production source."""

from __future__ import annotations

import argparse
from collections import Counter
import re
import sys
from pathlib import Path

from recoil_tooling import REPO_ROOT, display_path, iter_source_files, strip_comments_and_strings

REINTERPRET_CAST_RE = re.compile(r"\breinterpret_cast\s*<")


def baseline_key(rel: str, line: str) -> str:
    return f"{rel}\t{line.strip()}"


def load_baseline(path: Path) -> Counter[str]:
    if not path.exists():
        return Counter()
    return Counter(line for line in path.read_text(encoding="utf-8").splitlines() if line.strip())


def write_baseline(path: Path, keys: Counter[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    lines: list[str] = []
    for key in sorted(keys):
        lines.extend([key] * keys[key])
    path.write_text("\n".join(lines) + ("\n" if lines else ""), encoding="utf-8")


def find_occurrences(scan_root: Path, repo_root: Path) -> tuple[Counter[str], list[tuple[str, int, str, str]]]:
    counts: Counter[str] = Counter()
    locations: list[tuple[str, int, str, str]] = []

    for path in iter_source_files(scan_root):
        text = path.read_text(encoding="utf-8", errors="ignore")
        stripped = strip_comments_and_strings(text)
        lines = text.splitlines()
        rel = display_path(path, repo_root, fallback_root=scan_root)

        for match in REINTERPRET_CAST_RE.finditer(stripped):
            line_no = stripped.count("\n", 0, match.start()) + 1
            line = lines[line_no - 1].strip()
            key = baseline_key(rel, line)
            counts[key] += 1
            locations.append((rel, line_no, key, line))

    return counts, locations


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default="src", help="production source root to scan")
    parser.add_argument("--baseline", required=True, help="existing reinterpret_cast baseline")
    parser.add_argument("--write-baseline", action="store_true", help="write the current baseline and exit")
    args = parser.parse_args()

    repo_root = REPO_ROOT
    scan_root = (repo_root / args.root).resolve()
    baseline_path = repo_root / args.baseline

    current, locations = find_occurrences(scan_root, repo_root)
    if args.write_baseline:
        write_baseline(baseline_path, current)
        return 0

    baseline = load_baseline(baseline_path)
    seen: Counter[str] = Counter()
    violations: list[tuple[str, int, str]] = []

    for rel, line_no, key, line in locations:
        seen[key] += 1
        if seen[key] > baseline[key]:
            violations.append((rel, line_no, line))

    if violations:
        print("reinterpret_cast is not allowed in production source.")
        print("Remove the cast or explicitly update the baseline for pre-existing debt.")
        print()
        for rel, line_no, line in violations:
            print(f"{rel}:{line_no}: {line}")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
