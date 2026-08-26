#!/usr/bin/env python3
"""Reject every production-source ``goto`` using a structured retirement inventory."""

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


DEFAULT_BASELINE = REPO_ROOT / "tools" / "_recoil" / "config" / "no_source_goto_baseline.json"
SCHEMA_VERSION = 2
REVIEWED_TOTAL = 98
INDEX_POLICY = "exact-reviewed-retirement-inventory-v2"
SHARD_POLICY = "exact-reviewed-retirement-shard-v2"
GOTO_RE = re.compile(r"\bgoto\s+(?P<target>[A-Za-z_][A-Za-z0-9_]*)\s*;")


class GotoGuardError(ValueError):
    pass


@dataclass(frozen=True, order=True)
class GotoOccurrence:
    path: str
    line: int
    column: int
    target: str


@dataclass(frozen=True, order=True)
class BaselineEntry:
    row_id: int
    path: str
    reviewed_line: int
    column: int
    target: str
    state: str
    line_delta: int


@dataclass(frozen=True)
class GuardResult:
    mode: str
    passed: bool
    reviewed_baseline: int
    remaining_debt: int
    removed_debt: int
    additions: tuple[GotoOccurrence, ...]
    unrecorded_removals: tuple[BaselineEntry, ...]


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


def _canonical_relative(path: Path, root: Path, source_root: str) -> str:
    relative = path.resolve().relative_to(root.resolve()).as_posix()
    return f"{source_root}/{relative}" if source_root else relative


def scan_source_root(root: Path, *, source_root: str = "src") -> tuple[GotoOccurrence, ...]:
    rows: list[GotoOccurrence] = []
    for path in iter_source_files(root):
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        masked = _mask_non_code(text)
        relative = _canonical_relative(path, root, source_root)
        for match in GOTO_RE.finditer(masked):
            line = masked.count("\n", 0, match.start()) + 1
            line_start = masked.rfind("\n", 0, match.start()) + 1
            rows.append(
                GotoOccurrence(
                    path=relative,
                    line=line,
                    column=match.start() - line_start + 1,
                    target=match.group("target"),
                )
            )
    return tuple(sorted(rows))


def _read_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise GotoGuardError(f"cannot read goto inventory {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise GotoGuardError(f"goto inventory must be an object: {path}")
    return value


def load_baseline(path: Path) -> tuple[str, tuple[BaselineEntry, ...]]:
    index = _read_json(path)
    if (
        index.get("schema_version") != SCHEMA_VERSION
        or index.get("policy") != INDEX_POLICY
        or index.get("reviewed_total") != REVIEWED_TOTAL
        or not isinstance(index.get("source_root"), str)
        or not isinstance(index.get("shards"), list)
    ):
        raise GotoGuardError("goto inventory index is not exact schema version 2")
    entries: list[BaselineEntry] = []
    expected_row_id = 1
    for shard_ref in index["shards"]:
        if not isinstance(shard_ref, dict) or set(shard_ref) != {"path", "config", "reviewed_total"}:
            raise GotoGuardError("goto inventory shard reference is malformed")
        shard = _read_json(path.parent / str(shard_ref["config"]))
        if (
            shard.get("schema_version") != SCHEMA_VERSION
            or shard.get("policy") != SHARD_POLICY
            or shard.get("source_root") != index["source_root"]
            or not isinstance(shard.get("entries"), list)
            or shard.get("reviewed_total") != len(shard["entries"])
            or shard_ref["reviewed_total"] != len(shard["entries"])
        ):
            raise GotoGuardError(f"goto inventory shard is malformed: {shard_ref['config']}")
        for raw in shard["entries"]:
            if not isinstance(raw, dict) or set(raw) != {
                "row_id", "path", "reviewed_line", "column", "target", "state", "line_delta"
            }:
                raise GotoGuardError("goto inventory row is malformed")
            entry = BaselineEntry(**raw)
            if entry.row_id != expected_row_id or entry.path != shard_ref["path"]:
                raise GotoGuardError("goto inventory row order/path is not exact")
            if entry.state != "retired":
                raise GotoGuardError("schema-v2 goto inventory must contain retired rows only")
            entries.append(entry)
            expected_row_id += 1
    if len(entries) != REVIEWED_TOTAL:
        raise GotoGuardError(f"goto inventory must contain exactly {REVIEWED_TOTAL} rows")
    return str(index["source_root"]), tuple(entries)


def evaluate(
    current: tuple[GotoOccurrence, ...],
    baseline: tuple[BaselineEntry, ...],
    *,
    strict_zero: bool,
) -> GuardResult:
    del strict_zero
    return GuardResult(
        mode="strict-zero",
        passed=not current,
        reviewed_baseline=len(baseline),
        remaining_debt=len(current),
        removed_debt=len(baseline),
        additions=current,
        unrecorded_removals=(),
    )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Reject every source-level goto and validate the exact 98-row retirement inventory."
    )
    parser.add_argument("--root", type=Path, default=REPO_ROOT / "src")
    parser.add_argument("--baseline", type=Path, default=DEFAULT_BASELINE)
    parser.add_argument("--strict-zero", action="store_true")
    parser.add_argument("--summary", action="store_true")
    parser.add_argument("--json", action="store_true")
    return parser


def _result_payload(result: GuardResult, current: tuple[GotoOccurrence, ...]) -> dict[str, Any]:
    counts: dict[str, int] = {}
    for row in current:
        counts[row.path] = counts.get(row.path, 0) + 1
    return {
        "status": "pass" if result.passed else "fail",
        "mode": result.mode,
        "inventory_schema_version": SCHEMA_VERSION,
        "reviewed_baseline": result.reviewed_baseline,
        "remaining_debt": result.remaining_debt,
        "removed_debt": result.removed_debt,
        "file_counts": counts,
        "violations": [asdict(row) for row in result.additions],
        "unrecorded_removals": [],
    }


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        source_root, baseline = load_baseline(args.baseline)
        current = scan_source_root(args.root, source_root=source_root)
        result = evaluate(current, baseline, strict_zero=args.strict_zero)
    except (GotoGuardError, OSError) as exc:
        if args.json:
            print(json.dumps({"status": "error", "error": str(exc)}, sort_keys=True))
        else:
            print(f"no-source-goto guard error: {exc}", file=sys.stderr)
        return 2
    payload = _result_payload(result, current)
    if args.json:
        print(json.dumps(payload, indent=2, sort_keys=True))
    else:
        for row in result.additions:
            print(f"{row.path}:{row.line}:{row.column}: source-level goto {row.target!r} is forbidden")
        if args.summary:
            for source_path, count in payload["file_counts"].items():
                print(f"remaining goto debt: {source_path}: {count}")
        print(
            f"no-source-goto guard {'passed' if result.passed else 'failed'}: "
            f"mode={result.mode} remaining_debt={result.remaining_debt} "
            f"removed_debt={result.removed_debt} reviewed_baseline={result.reviewed_baseline} "
            f"violations={len(result.additions)} unrecorded_removals=0"
        )
    return 0 if result.passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
