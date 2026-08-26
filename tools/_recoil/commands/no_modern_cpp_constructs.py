#!/usr/bin/env python3
"""Fail on post-VC5 constructs and named C++ casts in production source."""

from __future__ import annotations

import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import argparse
from collections import Counter
import re
import sys

from _recoil.lib.tooling import REPO_ROOT, display_path, iter_source_files, strip_comments_and_strings


PATTERNS: tuple[tuple[str, re.Pattern[str]], ...] = (
    (
        "reconstruction call-convention wrapper",
        re.compile(r"\bRECOIL_(?:CDECL|FASTCALL|STDCALL|THISCALL)\b"),
    ),
    ("reconstruction inline marker", re.compile(r"\bRECOIL_FORCE" r"INLINE\b")),
    ("reconstruction noinline marker", re.compile(r"\bRECOIL_(?:[A-Z_]+_)?NOINLINE\b")),
    ("explicit thiscall", re.compile(r"\b__thiscall\b")),
    ("named C++ cast", re.compile(r"\b(?:static_cast|const_cast|reinterpret_cast|dynamic_cast)\s*<")),
    ("auto", re.compile(r"\bauto\b")),
    ("nullptr", re.compile(r"\bnullptr\b")),
    ("constexpr", re.compile(r"\bconstexpr\b")),
    ("decltype", re.compile(r"\bdecltype\b")),
    ("override", re.compile(r"\boverride\b")),
    ("final", re.compile(r"\bfinal\b")),
    ("thread_local", re.compile(r"\bthread_local\b")),
    ("enum class", re.compile(r"\benum\s+class\b")),
    ("using alias", re.compile(r"\busing\s+[A-Za-z_][A-Za-z0-9_]*\s*=")),
    ("static_assert", re.compile(r"\bstatic_assert\s*\(")),
    ("lambda", re.compile(r"\[[^\]\n]*\]\s*\(")),
    ("range for", re.compile(r"\bfor\s*\([^;{}\n()]*:[^;{}\n()]*\)")),
    ("defaulted/deleted function", re.compile(r"\)\s*=\s*(?:default|delete)\s*;")),
    ("noexcept", re.compile(r"\bnoexcept\b")),
    ("alignas/alignof", re.compile(r"\b(?:alignas|alignof)\b")),
    ("long long literal suffix", re.compile(r"\b[0-9]+(?:ull|ULL|ll|LL)\b")),
    (
        "modern standard header",
        re.compile(
            r"^\s*#\s*include\s*<\s*(?:array|filesystem|optional|variant|string_view|thread|mutex|atomic|tuple|type_traits)\s*>",
            re.MULTILINE,
        ),
    ),
    (
        "modern std helper",
        re.compile(
            r"\bstd::(?:array|optional|variant|string_view|unique_ptr|shared_ptr|weak_ptr|function|move|forward)\b"
        ),
    ),
    ("std array alias", re.compile(r"\barray\s*<")),
)

ALLOW_RECOIL_STATIC_ASSERT_RE = re.compile(r"\bRECOIL_STATIC_ASSERT\s*\(")


def find_occurrences(scan_root, repo_root) -> list[tuple[str, int, str, str]]:
    locations: list[tuple[str, int, str, str]] = []

    for path in iter_source_files(scan_root):
        text = path.read_text(encoding="utf-8", errors="ignore")
        stripped = strip_comments_and_strings(text)
        lines = text.splitlines()
        rel = display_path(path, repo_root, fallback_root=scan_root)

        for label, pattern in PATTERNS:
            for match in pattern.finditer(stripped):
                line_no = stripped.count("\n", 0, match.start()) + 1
                line = lines[line_no - 1].strip()
                if label == "static_assert" and ALLOW_RECOIL_STATIC_ASSERT_RE.search(line):
                    continue
                locations.append((rel, line_no, label, line))

    return locations


def print_summary(*, locations: list[tuple[str, int, str, str]], top: int) -> None:
    print("modern C++ production-source construct summary:")
    print(f"- current occurrences: {len(locations)}")

    by_label: Counter[str] = Counter()
    by_file: Counter[str] = Counter()
    for rel, _line_no, label, _line in locations:
        by_label[label] += 1
        by_file[rel] += 1

    print(f"- top labels (limit {top}):")
    if by_label:
        for label, count in by_label.most_common(top):
            print(f"  {count:4}  {label}")
    else:
        print("     0  <none>")

    print(f"- top files (limit {top}):")
    if by_file:
        for rel, count in by_file.most_common(top):
            print(f"  {count:4}  {rel}")
    else:
        print("     0  <none>")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default="src", help="production source root to scan")
    parser.add_argument("--summary", action="store_true", help="print current modern construct usage")
    parser.add_argument("--top", type=int, default=10, help="number of labels/files to print with --summary")
    args = parser.parse_args()

    repo_root = REPO_ROOT
    scan_root = (repo_root / args.root).resolve()

    locations = find_occurrences(scan_root, repo_root)
    if args.summary:
        print_summary(locations=locations, top=max(args.top, 0))

    if locations:
        if args.summary:
            print()
        print("Post-VC5SP3 constructs and named C++ casts are not allowed in production source.")
        print("Use VC5/VS97-era/source-faithful spelling under src before continuing reconstruction work.")
        print()
        for rel, line_no, label, line in locations:
            print(f"{rel}:{line_no}: {label}: {line}")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
