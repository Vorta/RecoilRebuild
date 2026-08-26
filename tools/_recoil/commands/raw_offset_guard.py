#!/usr/bin/env python3
"""Fail when tiered authored source still uses raw runtime-state offsets."""

from __future__ import annotations

import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import argparse
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
import re
import sys

from _recoil.lib.source_owners import DEFAULT_OWNER_LEDGER, SourceOwner, SourceOwnerDocument
from _recoil.lib.tooling import REPO_ROOT, display_path, iter_source_files, strip_comments_and_strings


RAW_OFFSET_PATTERNS: tuple[tuple[str, re.Pattern[str]], ...] = (
    (
        "named-offset-constant",
        re.compile(
            r"(?<!\w)(?:static[ \t]+)?const[ \t]+"
            r"(?:size_t|int|long|unsigned[ \t]+int|unsigned[ \t]+long|DWORD|WORD|BYTE)[ \t]+"
            r"k[A-Z][A-Za-z0-9_]*Offset\b"
        ),
    ),
    (
        "object-bytes-offset",
        re.compile(r"(?:->|\.)\s*bytes\s*(?:\+\s*|\[\s*)(?:0x[0-9A-Fa-f]+|\d+)"),
    ),
    (
        "byte-pointer-offset",
        re.compile(
            r"\b(?:char|unsigned[ \t]+char|BYTE|short|unsigned[ \t]+short|WORD|int|unsigned[ \t]+int|DWORD)[ \t]*\*[ \t]*"
            r"[A-Za-z_][A-Za-z0-9_]*[ \t]*=[ \t]*"
            r"\([^;\n]*\)[ \t]*(?:\([^;\n]*\)|[A-Za-z_&][A-Za-z0-9_>.\-&]*)[ \t]*"
            r"(?:\+|\-)\s*(?:0x[0-9A-Fa-f]+|k[A-Z][A-Za-z0-9_]*Offset)"
        ),
    ),
    (
        "cast-offset-access",
        re.compile(
            r"\(\s*(?:char|unsigned\s+char|BYTE|short|unsigned\s+short|WORD|int|unsigned\s+int|DWORD|void)\s*\*\s*\)"
            r"\s*(?:\([^;\n]*\)|[A-Za-z_&][A-Za-z0-9_>.\-&]*)\s*"
            r"(?:\+|\-)\s*(?:0x[0-9A-Fa-f]+|k[A-Z][A-Za-z0-9_]*Offset)"
        ),
    ),
    (
        "deref-offset-access",
        re.compile(
            r"\*\s*\(\s*[^;\n]*\*\s*\)\s*\(\s*[^;\n]*(?:\+|\-)\s*"
            r"(?:0x[0-9A-Fa-f]+|k[A-Z][A-Za-z0-9_]*Offset)"
        ),
    ),
)

OFFSET_EVIDENCE_RE = re.compile(r"\b(?:offsetof|RECOIL_STATIC_ASSERT)\s*\(")


@dataclass(frozen=True)
class RawOffsetLocation:
    rel: str
    line_no: int
    label: str
    line: str


@dataclass(frozen=True)
class RawOffsetViolation:
    location: RawOffsetLocation
    owners: tuple[SourceOwner, ...]


def normalize_source_path(path_text: str) -> str:
    return path_text.strip().replace("\\", "/")


def load_allowlist(path: Path) -> set[tuple[str, int, str]]:
    allowed: set[tuple[str, int, str]] = set()
    if not path.exists():
        return allowed

    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split()
        if len(parts) != 3:
            raise ValueError(f"invalid raw-offset allowlist line: {raw_line}")
        rel, line_no_text, label = parts
        try:
            line_no = int(line_no_text, 10)
        except ValueError as exc:
            raise ValueError(f"invalid raw-offset allowlist line number: {raw_line}") from exc
        allowed.add((normalize_source_path(rel), line_no, label))
    return allowed


def find_raw_offset_locations(
    scan_root: Path,
    repo_root: Path,
    *,
    allowlist: set[tuple[str, int, str]] | None = None,
) -> list[RawOffsetLocation]:
    allowed = allowlist or set()
    locations: list[RawOffsetLocation] = []

    for path in iter_source_files(scan_root):
        text = path.read_text(encoding="utf-8", errors="ignore")
        stripped = strip_comments_and_strings(text)
        lines = text.splitlines()
        rel = display_path(path, repo_root, fallback_root=scan_root)

        for label, pattern in RAW_OFFSET_PATTERNS:
            for match in pattern.finditer(stripped):
                line_no = stripped.count("\n", 0, match.start()) + 1
                line = lines[line_no - 1].strip()
                if OFFSET_EVIDENCE_RE.search(line):
                    continue
                if (rel, line_no, label) in allowed:
                    continue
                locations.append(RawOffsetLocation(rel, line_no, label, line))

    return locations


def owner_source_paths(owner: SourceOwner) -> set[str]:
    paths = {
        normalize_source_path(item)
        for item in owner.data.get("source_paths", [])
        if isinstance(item, str) and item.strip()
    }
    address_metadata = owner.data.get("address_metadata", {})
    if isinstance(address_metadata, dict):
        for raw in address_metadata.values():
            if not isinstance(raw, dict):
                continue
            source_path = raw.get("source_path")
            if isinstance(source_path, str) and source_path.strip():
                paths.add(normalize_source_path(source_path))
    return {item for item in paths if item not in {"pending", "external"}}


def tiered_owners_by_file(owners: list[SourceOwner], *, binary: str) -> dict[str, list[SourceOwner]]:
    by_file: dict[str, list[SourceOwner]] = {}
    for owner in owners:
        if owner.kind == "provider-boundary":
            continue
        if binary != "all" and (owner.binary or "recoil") != binary:
            continue
        if owner.reimplementation_tier not in {"C", "B", "A", "S"}:
            continue
        for rel in owner_source_paths(owner):
            by_file.setdefault(rel, []).append(owner)
    return by_file


def find_reimplemented_raw_offset_violations(
    owners: list[SourceOwner],
    locations: list[RawOffsetLocation],
    *,
    binary: str,
) -> list[RawOffsetViolation]:
    owners_by_file = tiered_owners_by_file(owners, binary=binary)
    violations: list[RawOffsetViolation] = []
    for location in locations:
        matched_owners = owners_by_file.get(normalize_source_path(location.rel), [])
        if matched_owners:
            violations.append(RawOffsetViolation(location, tuple(matched_owners)))
    return violations


def print_summary(*, locations: list[RawOffsetLocation], violations: list[RawOffsetViolation], top: int) -> None:
    print("raw-offset production-source summary:")
    print(f"- raw-offset candidates: {len(locations)}")
    print(f"- tiered-owner violations: {len(violations)}")

    by_label: Counter[str] = Counter(location.label for location in locations)
    by_file: Counter[str] = Counter(location.rel for location in locations)
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


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Reject raw runtime-state offsets in files with accepted unified-progress owner tiers."
    )
    parser.add_argument("--root", default="src", help="production source root to scan")
    parser.add_argument("--progress", default=str(DEFAULT_OWNER_LEDGER), help="path to unified reconstruction progress")
    parser.add_argument(
        "--binary",
        choices=("recoil", "messages", "all"),
        default="recoil",
        help="unified progress binary to inspect; use all to inspect every binary.",
    )
    parser.add_argument(
        "--allowlist",
        default=".agent/RAW_OFFSET_ALLOWLIST.txt",
        help="path containing '<repo-relative-path> <line-number> <label>' exemptions",
    )
    parser.add_argument("--summary", action="store_true", help="print current raw-offset usage")
    parser.add_argument("--top", type=int, default=10, help="number of labels/files to print with --summary")
    args = parser.parse_args(argv)

    repo_root = REPO_ROOT
    scan_root = (repo_root / args.root).resolve()
    owners_doc = SourceOwnerDocument.load(repo_root / args.progress)
    allowlist = load_allowlist(repo_root / args.allowlist)
    locations = find_raw_offset_locations(scan_root, repo_root, allowlist=allowlist)
    violations = find_reimplemented_raw_offset_violations(list(owners_doc.owners), locations, binary=args.binary)

    if args.summary:
        print_summary(locations=locations, violations=violations, top=max(args.top, 0))

    if violations:
        if args.summary:
            print()
        print("Raw runtime-state offsets are not allowed for tiered authored source.")
        print("Recover typed fields/classes/tables, or downgrade affected owners to Reimplemented [X]/not done.")
        print()
        for violation in violations:
            owner_text = ", ".join(f"{owner.id}:{owner.reimplementation_tier}" for owner in violation.owners[:5])
            if len(violation.owners) > 5:
                owner_text += f", ... {len(violation.owners) - 5} more"
            location = violation.location
            print(f"{location.rel}:{location.line_no}: {location.label}: {location.line}")
            print(f"  tiered owners: {owner_text}")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
