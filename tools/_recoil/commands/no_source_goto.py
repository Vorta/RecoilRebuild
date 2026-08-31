#!/usr/bin/env python3
"""Reject source-level ``goto`` in production source."""

from __future__ import annotations

import argparse
from dataclasses import asdict, dataclass
import json
from pathlib import Path
import re
import sys
from typing import Any

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from _recoil.lib.tooling import REPO_ROOT, iter_source_files


GOTO_RE = re.compile(r"\bgoto\s+(?P<target>[A-Za-z_][A-Za-z0-9_]*)\s*;")


@dataclass(frozen=True, order=True)
class GotoOccurrence:
    path: str
    line: int
    column: int
    target: str


def _mask_non_code(text: str) -> str:
    result = list(text)
    index = 0
    state = "code"
    quote = ""
    while index < len(text):
        current = text[index]
        following = text[index + 1] if index + 1 < len(text) else ""
        if state == "code":
            if current == "/" and following == "/":
                result[index] = result[index + 1] = " "
                index += 2
                state = "line-comment"
                continue
            if current == "/" and following == "*":
                result[index] = result[index + 1] = " "
                index += 2
                state = "block-comment"
                continue
            if current in {'"', "'"}:
                quote = current
                result[index] = " "
                index += 1
                state = "literal"
                continue
        elif state == "line-comment":
            if current in "\r\n":
                state = "code"
            else:
                result[index] = " "
        elif state == "block-comment":
            if current == "*" and following == "/":
                result[index] = result[index + 1] = " "
                index += 2
                state = "code"
                continue
            if current not in "\r\n":
                result[index] = " "
        else:
            if current == "\\" and following:
                result[index] = " "
                if following not in "\r\n":
                    result[index + 1] = " "
                index += 2
                continue
            if current == quote:
                result[index] = " "
                index += 1
                state = "code"
                continue
            if current not in "\r\n":
                result[index] = " "
        index += 1
    return "".join(result)


def scan_source_root(root: Path, *, source_root: str = "src") -> tuple[GotoOccurrence, ...]:
    rows: list[GotoOccurrence] = []
    root_resolved = root.resolve()
    for path in iter_source_files(root):
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        masked = _mask_non_code(text)
        relative = path.resolve().relative_to(root_resolved).as_posix()
        display = f"{source_root}/{relative}" if source_root else relative
        for match in GOTO_RE.finditer(masked):
            line_start = masked.rfind("\n", 0, match.start()) + 1
            rows.append(
                GotoOccurrence(
                    path=display,
                    line=masked.count("\n", 0, match.start()) + 1,
                    column=match.start() - line_start + 1,
                    target=match.group("target"),
                )
            )
    return tuple(sorted(rows))


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Reject every source-level goto.")
    parser.add_argument("--root", type=Path, default=REPO_ROOT / "src")
    parser.add_argument("--summary", action="store_true")
    parser.add_argument("--json", action="store_true")
    return parser


def _payload(rows: tuple[GotoOccurrence, ...]) -> dict[str, Any]:
    counts: dict[str, int] = {}
    for row in rows:
        counts[row.path] = counts.get(row.path, 0) + 1
    return {
        "status": "pass" if not rows else "fail",
        "mode": "zero-tolerance",
        "violation_count": len(rows),
        "file_counts": counts,
        "violations": [asdict(row) for row in rows],
    }


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        rows = scan_source_root(args.root)
    except OSError as exc:
        if args.json:
            print(json.dumps({"status": "error", "error": str(exc)}, sort_keys=True))
        else:
            print(f"no-source-goto guard error: {exc}", file=sys.stderr)
        return 2
    payload = _payload(rows)
    if args.json:
        print(json.dumps(payload, indent=2, sort_keys=True))
    else:
        for row in rows:
            print(f"{row.path}:{row.line}:{row.column}: source-level goto {row.target!r} is forbidden")
        if args.summary:
            for source_path, count in payload["file_counts"].items():
                print(f"source-level goto: {source_path}: {count}")
        print(
            f"no-source-goto guard {'passed' if not rows else 'failed'}: "
            f"violations={len(rows)}"
        )
    return 0 if not rows else 1


if __name__ == "__main__":
    raise SystemExit(main())
