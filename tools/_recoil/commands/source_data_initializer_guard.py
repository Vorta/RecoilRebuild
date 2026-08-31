#!/usr/bin/env python3
"""Guard original global-data ledger entries against placeholder source."""

from __future__ import annotations

import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import argparse
import json
import re
import sys
from collections.abc import Mapping
from pathlib import Path
from typing import Any

from _recoil.lib.progress import DEFAULT_PROGRESS_PATH, SCHEMA_VERSION, ProgressDocument
from _recoil.lib.tooling import REPO_ROOT, display_path, strip_comments_and_strings


DEFAULT_PROGRESS = DEFAULT_PROGRESS_PATH.relative_to(REPO_ROOT).as_posix()
VALID_DATA_STATUSES = {"✅", "❌"}
VALID_EXPECTED_SHAPES = {"concrete-initializer-list", "bss-zero"}


def find_workspace_root(scan_root: Path) -> Path:
    default_manifest = Path(DEFAULT_PROGRESS)
    for candidate in (scan_root, *scan_root.parents):
        if (candidate / default_manifest).exists():
            return candidate
    return scan_root


def resolve_manifest_path(
    scan_root: Path,
    workspace_root: Path,
    manifest_arg: str | None,
) -> Path:
    if manifest_arg is None:
        return workspace_root / DEFAULT_PROGRESS

    manifest_path = Path(manifest_arg)
    if manifest_path.is_absolute():
        return manifest_path

    scan_root_manifest = scan_root / manifest_path
    if scan_root_manifest.exists():
        return scan_root_manifest
    return workspace_root / manifest_path


def is_relative_to(path: Path, root: Path) -> bool:
    try:
        path.relative_to(root)
    except ValueError:
        return False
    return True


def source_data_records_from_progress(
    path: Path,
    document: ProgressDocument,
) -> list[Mapping[str, Any]]:
    symbols = document.collection("symbols")
    evidence = document.collection("evidence")

    for evidence_id, evidence_record in evidence.items():
        if not isinstance(evidence_record, Mapping):
            raise ValueError(f"{path}: evidence[{evidence_id!r}] must be an object")

    records: list[Mapping[str, Any]] = []
    for symbol_id, symbol in symbols.items():
        location = f"{path}: symbols[{symbol_id!r}]"
        if not isinstance(symbol, Mapping):
            raise ValueError(f"{location} must be an object")
        if "legacy_initializer" not in symbol:
            continue
        if symbol.get("kind") != "data":
            raise ValueError(f"{location} with legacy_initializer must have kind 'data'")
        if not isinstance(symbol.get("binary"), str) or not symbol["binary"]:
            raise ValueError(f"{location} expected non-empty string field 'binary'")
        if not isinstance(symbol.get("address"), str) or not symbol["address"]:
            raise ValueError(f"{location} expected non-empty string field 'address'")

        initializer = symbol.get("legacy_initializer")
        if not isinstance(initializer, Mapping):
            raise ValueError(f"{location} expected object field 'legacy_initializer'")
        evidence_ids = symbol.get("evidence_ids")
        if (
            not isinstance(evidence_ids, list)
            or not evidence_ids
            or any(not isinstance(item, str) or not item for item in evidence_ids)
        ):
            raise ValueError(
                f"{location} with legacy_initializer expected non-empty string list "
                "field 'evidence_ids'"
            )

        matched_initializer_evidence = False
        for evidence_id in evidence_ids:
            evidence_record = evidence.get(evidence_id)
            if not isinstance(evidence_record, Mapping):
                continue
            if evidence_record.get("kind") != "legacy-data-initializer":
                continue
            provenance = evidence_record.get("provenance")
            if not isinstance(provenance, Mapping):
                raise ValueError(
                    f"{path}: evidence[{evidence_id!r}] expected object field 'provenance'"
                )
            evidence_initializer = provenance.get("record")
            if not isinstance(evidence_initializer, Mapping):
                raise ValueError(
                    f"{path}: evidence[{evidence_id!r}].provenance "
                    "expected object field 'record'"
                )
            if dict(evidence_initializer) == dict(initializer):
                matched_initializer_evidence = True

        if not matched_initializer_evidence:
            raise ValueError(
                f"{location} legacy_initializer lacks a matching linked "
                "legacy-data-initializer evidence record"
            )
        records.append(initializer)
    return records


def load_manifest(path: Path) -> list[dict[str, str]]:
    if path.suffix.lower() in {".sqlite", ".sqlite3", ".db"}:
        document = ProgressDocument.load(path)
        data: Any = document.data
        if data.get("schema_version") != SCHEMA_VERSION:
            raise ValueError(
                f"{path}: unsupported unified progress schema_version "
                f"{data.get('schema_version')!r}; expected schema_version {SCHEMA_VERSION}"
            )
        globals_value: Any = source_data_records_from_progress(path, document)
    else:
        # A standalone guard manifest may be JSON, but the unified progress
        # authority is SQLite-only.
        data = json.loads(path.read_text(encoding="utf-8"))
        if not isinstance(data, dict):
            raise ValueError(f"{path}: expected a manifest object")
        if "schema_version" in data:
            raise ValueError(f"{path}: unified progress input must be SQLite")
        globals_value = data.get("globals")

    if not isinstance(globals_value, list):
        raise ValueError(f"{path}: expected list field 'globals'")

    entries: list[dict[str, str]] = []
    for index, item in enumerate(globals_value):
        if not isinstance(item, dict):
            raise ValueError(f"{path}: globals[{index}] must be an object")
        entry: dict[str, str] = {}
        if "state" in item:
            raise ValueError(
                f"{path}: globals[{index}] uses retired field 'state'; use 'data_reimplemented'"
            )
        for key in ("path", "name", "expected", "data_reimplemented", "evidence"):
            value = item.get(key)
            if not isinstance(value, str) or not value:
                raise ValueError(f"{path}: globals[{index}] expected non-empty string field '{key}'")
            entry[key] = value
        if entry["data_reimplemented"] not in VALID_DATA_STATUSES:
            valid = ", ".join(sorted(VALID_DATA_STATUSES))
            raise ValueError(
                f"{path}: globals[{index}] invalid data_reimplemented "
                f"{entry['data_reimplemented']!r}; expected one of: {valid}"
            )
        if entry["expected"] not in VALID_EXPECTED_SHAPES:
            valid = ", ".join(sorted(VALID_EXPECTED_SHAPES))
            raise ValueError(
                f"{path}: globals[{index}] invalid expected {entry['expected']!r}; expected one of: {valid}"
            )
        for optional_key in (
            "group",
            "original_address",
            "original_section",
            "original_type",
            "source_shape",
            "touched_by",
        ):
            value = item.get(optional_key, "")
            if isinstance(value, list):
                entry[optional_key] = ", ".join(str(part) for part in value)
            elif isinstance(value, str):
                entry[optional_key] = value
        entries.append(entry)
    return entries


def definition_re(name: str) -> re.Pattern[str]:
    escaped = re.escape(name)
    return re.compile(
        r"(?P<decl>^[ \t]*(?:extern[ \t]+\"C\"[ \t]+)?[A-Za-z_][A-Za-z0-9_:<>, \t\*&]*"
        + escaped
        + r"[ \t]*(?:\[[^\]]*\])?[ \t]*(?:=[ \t]*(?P<init>.*?))?;)",
        re.MULTILINE | re.DOTALL,
    )


def line_number(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def is_zero_placeholder(initializer: str) -> bool:
    compact = re.sub(r"\s+", "", initializer)
    compact = compact.rstrip(";")
    return compact in {
        "",
        "0",
        "NULL",
        "nullptr",
        "{}",
        "{0}",
        "{0,}",
        "{NULL}",
        "{NULL,}",
        "{nullptr}",
        "{nullptr,}",
    }


def scan_entry(workspace_root: Path, scan_root: Path, entry: dict[str, str]) -> list[str]:
    rel_path = entry["path"].replace("\\", "/")
    source_path = workspace_root / rel_path
    if not is_relative_to(source_path, scan_root):
        return []
    name = entry["name"]
    data_status = entry["data_reimplemented"]
    expected = entry["expected"]
    if data_status == "❌":
        return [
            (
                f"{rel_path}: concrete global {name} has Data reimplemented ❌; "
                f"{entry['evidence']}"
            )
        ]
    if not source_path.exists():
        return [f"{rel_path}: concrete global {name} source file is missing"]

    text = source_path.read_text(encoding="utf-8", errors="ignore")
    stripped = strip_comments_and_strings(text)
    match = definition_re(name).search(stripped)
    if match is None:
        return [f"{rel_path}: concrete global {name} definition was not found"]

    initializer = match.group("init") or ""
    if expected == "bss-zero":
        if is_zero_placeholder(initializer):
            return []
        return [
            (
                f"{display_path(source_path, workspace_root)}:{line_number(stripped, match.start('decl'))}: "
                f"BSS global {name} has a non-zero initializer but Data reimplemented is ✅; "
                f"{entry['evidence']}"
            )
        ]

    if expected == "concrete-initializer-list" and not is_zero_placeholder(initializer):
        return []

    line_no = line_number(stripped, match.start("decl"))
    display = display_path(source_path, workspace_root)
    return [
        (
            f"{display}:{line_no}: concrete global {name} is still a zero placeholder "
            f"but Data reimplemented is ✅; "
            f"{entry['evidence']}"
        )
    ]


def run_guard(workspace_root: Path, scan_root: Path, manifest_path: Path) -> list[str]:
    violations: list[str] = []
    for entry in load_manifest(manifest_path):
        violations.extend(scan_entry(workspace_root, scan_root, entry))
    return violations


def main(argv: list[str] | None = None) -> int:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    if hasattr(sys.stderr, "reconfigure"):
        sys.stderr.reconfigure(encoding="utf-8")

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--root",
        default=str(REPO_ROOT),
        help="source file or directory to scan; defaults to repository root",
    )
    parser.add_argument(
        "--path",
        dest="root",
        help="alias for --root; source file or directory to scan",
    )
    parser.add_argument("--progress", help="unified reconstruction progress path")
    args = parser.parse_args(argv)


    scan_root = Path(args.root).resolve()
    workspace_root = find_workspace_root(scan_root)
    manifest_path = resolve_manifest_path(scan_root, workspace_root, args.progress)
    violations = run_guard(workspace_root, scan_root, manifest_path)
    if violations:
        print("Concrete original global data must not be left as zero placeholders.")
        print()
        for violation in violations:
            print(violation)
        return 1

    print("recoil_source_data_initializer_guard passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
