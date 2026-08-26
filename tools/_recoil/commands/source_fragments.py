#!/usr/bin/env python3
"""Audit or guard temporary production source-fragment preservation forms."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys

from _recoil.lib.source_fragments import inventory_source_fragments
from _recoil.lib.tooling import REPO_ROOT


DEFAULT_FINAL_BUILD_MANIFEST = REPO_ROOT / "tools" / "_recoil" / "config" / "vc5_final_build.json"


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Mechanically inventory forbidden preservation headers, quoted production-source "
            "includes, .inl files, and compatibility-only final-build exclusions."
        )
    )
    parser.add_argument("--audit", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--root", default="src", help="production source root (default: src)")
    parser.add_argument(
        "--final-build-manifest",
        default=str(DEFAULT_FINAL_BUILD_MANIFEST),
        help="final-build manifest containing physical-block source exclusions",
    )
    parser.add_argument("--json", action="store_true", help="emit the complete typed inventory as JSON")
    return parser


def _typed_lines(result: dict[str, object], *, per_type_limit: int = 5) -> list[str]:
    findings = result["findings"]
    assert isinstance(findings, dict)
    lines: list[str] = []
    def add_group(label: str, rows: list[str]) -> None:
        lines.extend(rows[:per_type_limit])
        remaining = len(rows) - per_type_limit
        if remaining > 0:
            lines.append(f"{label}: ... {remaining} more (use audit source-fragments --json)")

    add_group(
        "source-fragment-file",
        [f"source-fragment-file: {path}" for path in findings["fragment_files"]],
    )
    add_group(
        "source-fragment-include",
        [
            f"source-fragment-{edge['edge_scope']}-include: "
            f"{edge['source']}:{edge['line']} -> {edge['target']}"
            for edge in findings["fragment_include_edges"]
        ],
    )
    add_group(
        "included-production-source",
        [
            f"included-production-source: {edge['source']}:{edge['line']} -> {edge['target']}"
            for edge in findings["included_source_edges"]
        ],
    )
    add_group(
        "production-inl-file",
        [f"production-inl-file: {path}" for path in findings["inl_files"]],
    )
    add_group(
        "compatibility-final-build-exclusion",
        [
            f"compatibility-final-build-exclusion: {exclusion['path']} "
            f"[{exclusion['reason']}]"
            for exclusion in findings["compatibility_final_build_exclusions"]
        ],
    )
    return lines


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    root = Path(args.root)
    if not root.is_absolute():
        root = REPO_ROOT / root
    manifest = Path(args.final_build_manifest)
    if not manifest.is_absolute():
        manifest = REPO_ROOT / manifest
    try:
        result = inventory_source_fragments(
            root,
            repo_root=REPO_ROOT,
            final_build_manifest=manifest,
        )
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"source-fragment-audit-error: {exc}", file=sys.stderr)
        return 2

    if args.json:
        print(json.dumps(result, indent=2, ensure_ascii=False, sort_keys=True))
    else:
        counts = result["counts"]
        assert isinstance(counts, dict)
        print(
            "source-fragment-summary: "
            f"fragment_files={counts['fragment_files']} "
            f"fragment_include_edges={counts['fragment_include_edges']} "
            f"included_source_edges={counts['included_source_edges']} "
            f"inl_files={counts['inl_files']} "
            f"compatibility_final_build_exclusions="
            f"{counts['compatibility_final_build_exclusions']}"
        )
        for line in _typed_lines(result):
            print(line, file=sys.stderr if not args.audit else sys.stdout)

    return 0 if args.audit or result["ok"] else 1


if __name__ == "__main__":
    sys.exit(main())
