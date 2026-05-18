#!/usr/bin/env python3
"""Fail on new reinterpret_cast use in production source."""

from __future__ import annotations

import argparse
from collections import Counter
import re
import sys
from pathlib import Path


SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".inl"}
REINTERPRET_CAST_RE = re.compile(r"\breinterpret_cast\s*<")


def strip_comments_and_strings(text: str) -> str:
    result: list[str] = []
    i = 0
    state = "code"
    while i < len(text):
        ch = text[i]
        nxt = text[i + 1] if i + 1 < len(text) else ""

        if state == "code":
            if ch == "/" and nxt == "/":
                result.extend("  ")
                i += 2
                state = "line_comment"
            elif ch == "/" and nxt == "*":
                result.extend("  ")
                i += 2
                state = "block_comment"
            elif ch == '"':
                result.append(" ")
                i += 1
                state = "string"
            elif ch == "'":
                result.append(" ")
                i += 1
                state = "char"
            else:
                result.append(ch)
                i += 1
        elif state == "line_comment":
            if ch == "\n":
                result.append(ch)
                state = "code"
            else:
                result.append(" ")
            i += 1
        elif state == "block_comment":
            if ch == "*" and nxt == "/":
                result.extend("  ")
                i += 2
                state = "code"
            else:
                result.append("\n" if ch == "\n" else " ")
                i += 1
        elif state == "string":
            if ch == "\\":
                result.extend("  " if nxt else " ")
                i += 2 if nxt else 1
            elif ch == '"':
                result.append(" ")
                i += 1
                state = "code"
            else:
                result.append("\n" if ch == "\n" else " ")
                i += 1
        elif state == "char":
            if ch == "\\":
                result.extend("  " if nxt else " ")
                i += 2 if nxt else 1
            elif ch == "'":
                result.append(" ")
                i += 1
                state = "code"
            else:
                result.append("\n" if ch == "\n" else " ")
                i += 1

    return "".join(result)


def iter_source_files(root: Path) -> list[Path]:
    return sorted(path for path in root.rglob("*") if path.is_file() and path.suffix in SOURCE_SUFFIXES)


def display_path(path: Path, scan_root: Path, repo_root: Path) -> str:
    try:
        return path.relative_to(repo_root).as_posix()
    except ValueError:
        return path.relative_to(scan_root).as_posix()


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
        rel = display_path(path, scan_root, repo_root)

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

    repo_root = Path.cwd()
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
