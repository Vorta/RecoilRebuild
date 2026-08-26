#!/usr/bin/env python3
"""Fail on source-shape and ABI scaffolds in production source."""

from __future__ import annotations

import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import argparse
from collections import Counter
import re
import sys

from _recoil.lib.owner_entries import OwnerEntryIndex, TIER_DATA_EQUIVALENT, tier_at_least
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH
from _recoil.lib.tooling import (
    REPO_ROOT,
    SOURCE_SUFFIXES,
    display_path,
    iter_source_files,
    strip_comments_and_strings,
)


VIRTUAL_SCAFFOLD_DECL_RE = re.compile(
    r"\b(?:struct|class)\s+"
    r"(?P<name>[A-Za-z_][A-Za-z0-9_]*(?:Dispatch|Virtual)[A-Za-z0-9_]*)"
    r"\b[^{;]*\{(?P<body>.*?)\};",
    re.DOTALL,
)

VTABLE_FACTORY_RE = re.compile(
    r"\bMake[A-Za-z_][A-Za-z0-9_]*(?:Vtable|Vtbl|FTable)\s*\("
)

TABLE_NAME_PATTERN = (
    r"[A-Za-z_][A-Za-z0-9_]*(?:(?:_)?FTable|VTable|Vtable|Vtbl)[A-Za-z0-9_]*"
)

TABLE_TYPE_DECL_RE = re.compile(
    r"\b(?:struct|class|union)\s+(?P<name>" + TABLE_NAME_PATTERN + r")\b"
)

TABLE_TYPEDEF_RE = re.compile(
    r"\btypedef\b[^;{}]*(?P<name>" + TABLE_NAME_PATTERN + r")\s*;"
)

TABLE_GLOBAL_RE = re.compile(
    r"(?m)^\s*"
    r"(?:extern\s+)?(?:static\s+)?(?:const\s+)?(?:volatile\s+)?"
    r"(?:struct\s+|class\s+|union\s+)?"
    r"(?P<type>" + TABLE_NAME_PATTERN + r")"
    r"(?:\s*\*+\s*|\s+)"
    r"(?P<name>[A-Za-z_][A-Za-z0-9_]*)\b"
)

RAW_SLOT_ARRAY_RE = re.compile(
    r"(?:"
    r"\b[A-Za-z_][A-Za-z0-9_]*(?:table|Table|Vtbl|Vtable|VTable|FTable|ftable|vtable)"
    r"[A-Za-z0-9_]*(?:->|\.)slots\s*\["
    r"|\btable(?:\.[A-Za-z_][A-Za-z0-9_]*)?\.slots\s*\["
    r"|\b(?:unsigned\s+int|void\s*\*|DWORD|RecoilFn32)\s+\*?slots\s*\["
    r"|\b(?:primarySlots|secondarySlots)\s*\["
    r")"
)

DISPATCH_VIEW_DECL_RE = re.compile(
    r"\b(?:struct|class)\s+"
    r"(?P<name>[A-Za-z_][A-Za-z0-9_]*(?:DispatchView|SlotView|VtblView|VtableView|FTableView|AbiView))"
    r"\b[^{;]*\{(?P<body>.*?)\};",
    re.DOTALL,
)

SCAFFOLD_IDENTIFIER_RE = re.compile(
    r"\b[A-Za-z_][A-Za-z0-9_]*"
    r"(?:SourceShapeScaffold|AbiScaffold|ScaffoldOnly|RawVtbl|RawVtable|RawVTable|RawFTable|RawSlots)"
    r"[A-Za-z0-9_]*\b"
)

RAW_TABLE_STORAGE_RE = re.compile(
    r"(?m)^\s*"
    r"(?:static\s+)?(?:const\s+)?"
    r"(?:DWORD|unsigned\s+int|void\s*\*|RecoilFn32)"
    r"\s+(?:\*+\s*)?"
    r"(?P<name>[A-Za-z_][A-Za-z0-9_]*(?:Slots|SlotArray|Vtable|VTable|Vtbl|FTable)[A-Za-z0-9_]*)"
    r"\s*\["
)

SLOT_INVOKE_HELPER_RE = re.compile(
    r"\b(?:Call|Invoke|Dispatch)[A-Za-z_][A-Za-z0-9_]*(?:Slot|Vtbl|Vtable|VTable|FTable)\s*\("
)

SCAFFOLD_COMMENT_RE = re.compile(
    r"\b(?:temporary\s+)?(?:abi|source-shape)\s+scaffold\b"
    r"|\bscaffold-only\b",
    re.IGNORECASE,
)


def line_for_offset(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def source_paths_for(scan_path) -> list:
    if scan_path.is_file():
        if scan_path.suffix.lower() in SOURCE_SUFFIXES:
            return [scan_path]
        return []
    return iter_source_files(scan_path)


def find_occurrences(scan_paths, repo_root) -> list[tuple[str, int, str, str]]:
    locations: list[tuple[str, int, str, str]] = []

    for scan_path in scan_paths:
        fallback_root = scan_path.parent if scan_path.is_file() else scan_path
        for path in source_paths_for(scan_path):
            locations.extend(find_occurrences_in_file(path, repo_root, fallback_root))

    return sorted(set(locations))


def find_occurrences_in_file(path, repo_root, fallback_root) -> list[tuple[str, int, str, str]]:
    locations: list[tuple[str, int, str, str]] = []

    text = path.read_text(encoding="utf-8", errors="ignore")
    stripped = strip_comments_and_strings(text)
    lines = text.splitlines()
    rel = display_path(path, repo_root, fallback_root=fallback_root)

    for match in VIRTUAL_SCAFFOLD_DECL_RE.finditer(stripped):
        body = match.group("body")
        if re.search(r"\bvirtual\b", body) is None:
            continue
        line_no = line_for_offset(stripped, match.start())
        line = lines[line_no - 1].strip()
        locations.append(
            (
                rel,
                line_no,
                "virtual dispatch/source-shape scaffold",
                line,
            )
        )

    for match in VTABLE_FACTORY_RE.finditer(stripped):
        line_no = line_for_offset(stripped, match.start())
        line = lines[line_no - 1].strip()
        locations.append((rel, line_no, "vtable/ftable factory scaffold", line))

    for match in TABLE_TYPE_DECL_RE.finditer(stripped):
        line_no = line_for_offset(stripped, match.start())
        line = lines[line_no - 1].strip()
        locations.append((rel, line_no, "authored vtable/ftable type scaffold", line))

    for match in TABLE_TYPEDEF_RE.finditer(stripped):
        line_no = line_for_offset(stripped, match.start())
        line = lines[line_no - 1].strip()
        locations.append((rel, line_no, "authored vtable/ftable typedef scaffold", line))

    for match in TABLE_GLOBAL_RE.finditer(stripped):
        if stripped[match.end() :].lstrip().startswith("("):
            continue
        line_no = line_for_offset(stripped, match.start())
        line = lines[line_no - 1].strip()
        locations.append((rel, line_no, "authored vtable/ftable object/global scaffold", line))

    for match in RAW_SLOT_ARRAY_RE.finditer(stripped):
        line_no = line_for_offset(stripped, match.start())
        line = lines[line_no - 1].strip()
        locations.append((rel, line_no, "raw slot table scaffold", line))

    for match in DISPATCH_VIEW_DECL_RE.finditer(stripped):
        body = match.group("body")
        if re.search(r"\b(?:slots|vtable|vtbl|ftable|DWORD|RecoilFn32|void\s*\*)\b", body, re.IGNORECASE) is None:
            continue
        line_no = line_for_offset(stripped, match.start())
        line = lines[line_no - 1].strip()
        locations.append((rel, line_no, "dispatch-view owner-shape scaffold", line))

    for match in SCAFFOLD_IDENTIFIER_RE.finditer(stripped):
        line_no = line_for_offset(stripped, match.start())
        line = lines[line_no - 1].strip()
        locations.append((rel, line_no, "source-shape scaffold identifier", line))

    for match in RAW_TABLE_STORAGE_RE.finditer(stripped):
        line_no = line_for_offset(stripped, match.start())
        line = lines[line_no - 1].strip()
        locations.append((rel, line_no, "raw table storage scaffold", line))

    for match in SLOT_INVOKE_HELPER_RE.finditer(stripped):
        line_no = line_for_offset(stripped, match.start())
        line = lines[line_no - 1].strip()
        locations.append((rel, line_no, "slot-dispatch helper scaffold", line))

    for match in SCAFFOLD_COMMENT_RE.finditer(text):
        line_no = line_for_offset(text, match.start())
        line = lines[line_no - 1].strip()
        locations.append((rel, line_no, "scaffold marker in production source", line))

    return locations


def normalize_rel(path_text: str) -> str:
    return path_text.replace("\\", "/").lstrip("./")


def owner_claims_for_locations(
    locations: list[tuple[str, int, str, str]],
    *,
    owners_path: Path,
) -> list[dict[str, str]]:
    if not locations:
        return []
    files = {normalize_rel(rel) for rel, _line_no, _label, _line in locations}
    doc = OwnerEntryIndex.load(owners_path)
    claims: list[dict[str, str]] = []
    for entry in doc.entries.values():
        file_name = normalize_rel(entry.reimplemented_file)
        if not file_name or file_name not in files or entry.is_provider_boundary:
            continue
        if entry.has_accepted_source_owner or tier_at_least(entry.accepted_reimplementation_tier, TIER_DATA_EQUIVALENT):
            claims.append(
                {
                    "file": entry.reimplemented_file,
                    "address": entry.address,
                    "tier": entry.accepted_reimplementation_tier,
                    "owner_status": entry.source_owner_status or "pending",
                    "source_owner": entry.source_owner or "pending",
                }
            )
    return claims


def print_summary(*, locations: list[tuple[str, int, str, str]], top: int) -> None:
    print("source-shape scaffold production-source summary:")
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


def print_owner_claims(claims: list[dict[str, str]], *, top: int) -> None:
    print("- owner claims touching scaffolded files:")
    if not claims:
        print("     0  <none>")
        return
    selected = claims[:top] if top >= 0 else claims
    for row in selected:
        print(
            f"  {row['address']} tier={row['tier']} owner_status={row['owner_status']} "
            f"owner={row['source_owner']} file={row['file']}"
        )
    if top >= 0 and len(claims) > top:
        print(f"  ... {len(claims) - top} more")


def default_owner_ledger_path() -> str:
    return str(DEFAULT_PROGRESS_PATH)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--root",
        action="append",
        default=None,
        help=(
            "production source file or directory to scan; may be repeated. "
            "Defaults to src when omitted"
        ),
    )
    parser.add_argument(
        "--path",
        action="append",
        default=[],
        help="additional source file or directory to scan; may be repeated",
    )
    parser.add_argument(
        "--paths",
        action="append",
        nargs="+",
        default=[],
        metavar="PATH",
        help="compatibility alias for one or more --path arguments",
    )
    parser.add_argument("--summary", action="store_true", help="print current scaffold usage")
    parser.add_argument("--top", dest="top", type=int, default=None, help="number of labels/files to print with --summary")
    parser.add_argument("--max", dest="top", type=int, default=None, help="compatibility alias for --top")
    parser.add_argument(
        "--include-owners",
        action="store_true",
        help="show accepted owner entries in files containing scaffold hits",
    )
    parser.add_argument(
        "--progress",
        default=default_owner_ledger_path(),
        help="unified reconstruction progress tracker used by --include-owners",
    )
    args = parser.parse_args()

    repo_root = REPO_ROOT
    roots = args.root if args.root is not None else ["src"]
    scan_paths = [(repo_root / root).resolve() for root in roots]
    scan_paths.extend((repo_root / path).resolve() for path in args.path)
    scan_paths.extend((repo_root / path).resolve() for paths in args.paths for path in paths)
    top = max(args.top if args.top is not None else 10, 0)

    locations = find_occurrences(scan_paths, repo_root)
    owner_claims: list[dict[str, str]] = []
    if args.include_owners:
        owner_claims = owner_claims_for_locations(locations, owners_path=Path(args.progress))
    if args.summary:
        print_summary(locations=locations, top=top)
        if args.include_owners:
            print_owner_claims(owner_claims, top=top)

    if locations:
        if args.summary:
            print()
        print("Source-shape and ABI scaffolds are not allowed in production source.")
        print("Recover the Binary Ninja-proven owner model instead: class/interface first when")
        print("evidence fits, otherwise provider boundary, callback/data system, record,")
        print("namespace/source-cluster owner, global-data set, or subsystem.")
        print("Do not reimplement authored VTables/FTables as production source.")
        print()
        for rel, line_no, label, line in locations:
            print(f"{rel}:{line_no}: {label}: {line}")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
