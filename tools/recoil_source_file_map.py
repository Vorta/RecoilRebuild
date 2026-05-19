#!/usr/bin/env python3
"""Generate a compact original-source to repo-source map from provenance comments."""

from __future__ import annotations

import argparse
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
import re
import sys


REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE_ROOT = REPO_ROOT / "src"
DEFAULT_OUTPUT = REPO_ROOT / "docs" / "reconstruction" / "source_file_map.md"

SOURCE_EXTENSIONS = {".c", ".cpp", ".h", ".hpp", ".inl"}
REIMPLEMENTS_RE = re.compile(
    r"Reimplements\s+(0x[0-9A-Fa-f]{6,8}):\s*([^(\r\n]+?)(?:\s*\(([^)\r\n]+)\))?\s*$"
)


@dataclass(frozen=True)
class SourceMapEntry:
    original_source: str
    address: str
    symbol: str
    repo_file: str
    line: int


def normalize_address(address: str) -> str:
    return f"0x{int(address, 16):06x}"


def normalize_original_source(value: str | None) -> str:
    if not value:
        return "unknown original source"
    text = value.strip().replace("\\", "/")
    lower = text.lower()
    for prefix in ("d:/proj/", "c:/proj/"):
        if lower.startswith(prefix):
            text = text[len(prefix) :]
            break
    if text.startswith("./"):
        text = text[2:]
    return text or "unknown original source"


def repo_relative_path(path: Path, repo_root: Path) -> str:
    try:
        return path.relative_to(repo_root).as_posix()
    except ValueError:
        return path.as_posix()


def iter_source_files(source_root: Path) -> list[Path]:
    return sorted(
        path
        for path in source_root.rglob("*")
        if path.is_file() and path.suffix.lower() in SOURCE_EXTENSIONS
    )


def collect_entries(source_root: Path, repo_root: Path = REPO_ROOT) -> list[SourceMapEntry]:
    entries: list[SourceMapEntry] = []
    for path in iter_source_files(source_root):
        rel = repo_relative_path(path, repo_root)
        text = path.read_text(encoding="utf-8", errors="replace")
        for line_number, line in enumerate(text.splitlines(), start=1):
            match = REIMPLEMENTS_RE.search(line)
            if not match:
                continue
            address, symbol, original_source = match.groups()
            entries.append(
                SourceMapEntry(
                    original_source=normalize_original_source(original_source),
                    address=normalize_address(address),
                    symbol=" ".join(symbol.strip().split()),
                    repo_file=rel,
                    line=line_number,
                )
            )
    return sorted(entries, key=lambda item: (item.original_source.lower(), item.address, item.repo_file, item.line))


def render_markdown(entries: list[SourceMapEntry]) -> str:
    grouped: dict[str, list[SourceMapEntry]] = defaultdict(list)
    for entry in entries:
        grouped[entry.original_source].append(entry)

    lines: list[str] = [
        "# Source File Map",
        "",
        "Generated from `Reimplements 0xNNNNNN: Name (original/source/path)` comments in `src/`.",
        "Binary Ninja remains authoritative; this map is an agent navigation aid.",
        "",
        f"Entries: {len(entries)}",
        "",
    ]

    if not entries:
        lines.append("No source-map entries found.")
        return "\n".join(lines) + "\n"

    for original_source in sorted(grouped, key=str.lower):
        lines.append(f"## {original_source}")
        lines.append("")
        for entry in grouped[original_source]:
            lines.append(f"- `{entry.address}` `{entry.symbol}` -> `{entry.repo_file}:{entry.line}`")
        lines.append("")
    return "\n".join(lines).rstrip() + "\n"


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Generate the Recoil original-source placement map.")
    parser.add_argument("--root", default=str(DEFAULT_SOURCE_ROOT), help="Source tree to scan.")
    parser.add_argument("--output", default=str(DEFAULT_OUTPUT), help="Markdown file to write.")
    parser.add_argument(
        "--check",
        nargs="?",
        const=str(DEFAULT_OUTPUT),
        help="Compare generated output with an existing markdown file instead of writing.",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    source_root = Path(args.root)
    output = Path(args.output)
    entries = collect_entries(source_root)
    rendered = render_markdown(entries)

    if args.check:
        check_path = Path(args.check)
        actual = check_path.read_text(encoding="utf-8") if check_path.exists() else ""
        if actual != rendered:
            print(f"Source file map is stale: {check_path}")
            return 1
        print(f"Source file map is current: {check_path}")
        return 0

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(rendered, encoding="utf-8", newline="\n")
    print(f"Wrote {output} ({len(entries)} entries).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
