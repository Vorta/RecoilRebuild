#!/usr/bin/env python3
"""Fail on raw assembly and naked functions in production source."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

from recoil_tooling import (
    ASM_SUFFIXES,
    REPO_ROOT,
    SOURCE_SUFFIXES,
    display_path,
    iter_source_files,
    strip_comments_and_strings,
)

FORBIDDEN_PATTERNS = [
    ("__asm", re.compile(r"\b__asm\b")),
    ("_asm", re.compile(r"\b_asm\b")),
    ("_emit", re.compile(r"\b_emit\b")),
    ("asm(...)", re.compile(r"\basm\s*(?:volatile\s*)?\(")),
    ("__declspec(naked)", re.compile(r"__declspec\s*\(\s*naked\s*\)")),
    ("__attribute__((naked))", re.compile(r"__attribute__\s*\(\s*\(\s*naked\s*\)\s*\)")),
    ("RECOIL_NAKED", re.compile(r"\bRECOIL_NAKED\b")),
]
REIMPLEMENTS_RE = re.compile(r"Reimplements\s+(0x[0-9a-fA-F]+)")


def load_allowlist(path: Path) -> set[tuple[str, str, str]]:
    allowed: set[tuple[str, str, str]] = set()
    if not path.exists():
        return allowed

    for line_no, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw_line.split("#", 1)[0].strip()
        if not line:
            continue

        fields = line.split()
        if len(fields) < 3:
            raise ValueError(f"{path}:{line_no}: expected '<path> <address> <token>'")

        rel, address, label = fields[:3]
        allowed.add((rel.replace("\\", "/"), address.lower(), label))

    return allowed


def find_reimplements_address(lines: list[str], line_no: int) -> str | None:
    for index in range(line_no - 1, -1, -1):
        match = REIMPLEMENTS_RE.search(lines[index])
        if match:
            return match.group(1).lower()
    return None


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default="src", help="production source root to scan")
    parser.add_argument("--allowlist", default=None, help="approved raw-assembly exceptions")
    args = parser.parse_args()

    repo_root = REPO_ROOT
    scan_root = (repo_root / args.root).resolve()
    allowlist = load_allowlist(repo_root / args.allowlist) if args.allowlist else set()
    violations: list[tuple[str, int, str, str]] = []

    for path in iter_source_files(scan_root, suffixes=ASM_SUFFIXES.union(SOURCE_SUFFIXES)):
        rel = display_path(path, repo_root, fallback_root=scan_root)
        if path.suffix.lower() in {suffix.lower() for suffix in ASM_SUFFIXES}:
            violations.append((rel, 1, path.suffix, "checked-in assembly source file"))
            continue

        text = path.read_text(encoding="utf-8", errors="ignore")
        stripped = strip_comments_and_strings(text)
        lines = text.splitlines()
        for label, pattern in FORBIDDEN_PATTERNS:
            for match in pattern.finditer(stripped):
                line_no = stripped.count("\n", 0, match.start()) + 1
                address = find_reimplements_address(lines, line_no)
                if address is not None and (rel, address, label) in allowlist:
                    continue
                violations.append((rel, line_no, label, lines[line_no - 1].strip()))

    if violations:
        print("Raw assembly and naked functions are not allowed in production source.")
        print("Use C/C++ source, typed symbols, compiler/runtime providers, or documented blockers instead.")
        print()
        for rel, line_no, label, line in violations:
            print(f"{rel}:{line_no}: {label}: {line}")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
