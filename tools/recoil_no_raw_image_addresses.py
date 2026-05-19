#!/usr/bin/env python3
"""Fail on original-image address literals used as source expressions.

The rebuilt executable is not the original image. Original 0x004xxxxx values are
evidence from Binary Ninja, not valid source-level substitutes for symbols.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

from recoil_tooling import REPO_ROOT, display_path, iter_source_files, strip_comments_and_strings

HEX_RE = re.compile(r"\b0x[0-9a-fA-F]+\b")


def load_allowlist(path: Path) -> set[tuple[str, str]]:
    allowed: set[tuple[str, str]] = set()
    if not path.exists():
        return allowed

    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split(None, 1)
        if len(parts) != 2:
            raise ValueError(f"invalid allowlist line: {raw_line}")
        allowed.add((parts[0].replace("\\", "/"), parts[1].lower()))
    return allowed


def is_original_image_literal(token: str) -> bool:
    value = int(token, 16)
    return 0x00400000 <= value <= 0x004FFFFF


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default="src", help="source root to scan")
    parser.add_argument(
        "--allowlist",
        default=".agent/RAW_ADDRESS_ALLOWLIST.txt",
        help="path containing '<repo-relative-path> <hex-literal>' entries",
    )
    args = parser.parse_args()

    repo_root = REPO_ROOT
    scan_root = (repo_root / args.root).resolve()
    allowlist = load_allowlist(repo_root / args.allowlist)
    violations: list[tuple[str, int, str, str]] = []

    for path in iter_source_files(scan_root):
        rel = display_path(path, repo_root, fallback_root=scan_root)
        text = path.read_text(encoding="utf-8", errors="ignore")
        stripped = strip_comments_and_strings(text)
        lines = text.splitlines()
        for match in HEX_RE.finditer(stripped):
            token = match.group(0).lower()
            if not is_original_image_literal(token):
                continue
            if (rel, token) in allowlist:
                continue
            line_no = stripped.count("\n", 0, match.start()) + 1
            violations.append((rel, line_no, token, lines[line_no - 1].strip()))

    if violations:
        print("Original-image address literals are not allowed in source code.")
        print("Use named symbols, typed objects, imports, or data tables instead.")
        print()
        for rel, line_no, token, line in violations:
            print(f"{rel}:{line_no}: {token}: {line}")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
