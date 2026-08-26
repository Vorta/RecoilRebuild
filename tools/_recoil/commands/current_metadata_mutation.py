from __future__ import annotations

import argparse
from copy import deepcopy
import json
from pathlib import Path
import re
import sys
from typing import Any, Mapping

from _recoil.commands.current_metadata_audit import (
    DEFAULT_PROGRESS_PATH,
    GENERATED_CURRENT_PATH,
    STATIC_CURRENT_PATTERN,
    audit_current_metadata,
    refresh_remaining_blocker_metadata,
)
from _recoil.lib.progress import (
    ConcurrentProgressUpdate,
    ProgressDocument,
    ProgressError,
    ProgressStore,
)
from _recoil.lib.tooling import configure_stdio, display_path


class CurrentMetadataMutationError(RuntimeError):
    pass


_WORK_ITEM_PREFIX = "work_items."
_WORK_ITEM_SUFFIX = ".first_unresolved_item"
_HISTORICAL_REWRITES: tuple[tuple[str, str], ...] = (
    (r"\bauthoritative current\b", "authoritative at that time"),
    (r"\bcurrent primary cursor\b", "primary cursor at that time"),
    (r"\bcurrent divergence\b", "divergence observed at that time"),
    (r"\bcurrent cursor\s+is\b", "cursor was"),
    (r"\bcurrent cursor\s+at\b", "cursor was at"),
    (
        r"\bcurrent ((?:authored|full|linked)[^.!?]{0,40} cursor)\b",
        r"then-observed \1",
    ),
)


def _historical_narrative(value: str) -> str:
    """Retain the material statement while removing its live-scheduler claim."""

    result = value
    for pattern, replacement in _HISTORICAL_REWRITES:
        result = re.sub(pattern, replacement, result, flags=re.IGNORECASE)
    if result == value or STATIC_CURRENT_PATTERN.search(result):
        raise CurrentMetadataMutationError(
            "audited current-cursor narrative has no safe historical rewrite"
        )
    return result


def _audited_work_item_ids(document: ProgressDocument) -> list[str]:
    ids: list[str] = []
    for finding in audit_current_metadata(document):
        path = str(finding.get("path", ""))
        if finding.get("code") != "static-current-narrative":
            continue
        if not path.startswith(_WORK_ITEM_PREFIX) or not path.endswith(_WORK_ITEM_SUFFIX):
            continue
        work_id = path[len(_WORK_ITEM_PREFIX) : -len(_WORK_ITEM_SUFFIX)]
        if work_id and work_id not in ids:
            ids.append(work_id)
    return ids


def _historicalize_work_item(
    data: dict[str, Any],
    *,
    work_id: str,
    committed_revision: int,
) -> dict[str, Any]:
    work_items = data.get("work_items")
    work = work_items.get(work_id) if isinstance(work_items, dict) else None
    if not isinstance(work, dict):
        raise CurrentMetadataMutationError(
            f"audited work item {work_id!r} disappeared during mutation"
        )
    old = work.get("first_unresolved_item")
    if not isinstance(old, str) or not STATIC_CURRENT_PATTERN.search(old):
        raise CurrentMetadataMutationError(
            f"audited work item {work_id!r} no longer contains a static current narrative"
        )
    rewritten = _historical_narrative(old)
    history = work.setdefault("first_unresolved_item_history", [])
    if not isinstance(history, list):
        raise CurrentMetadataMutationError(
            f"work item {work_id!r} first_unresolved_item_history must be a list"
        )
    historical = {
        "historicalized_at_revision": committed_revision,
        "narrative": old,
    }
    if not any(
        isinstance(item, Mapping) and item.get("narrative") == old
        for item in history
    ):
        history.append(historical)
    work["first_unresolved_item"] = rewritten
    return {
        "work_item_id": work_id,
        "path": f"work_items.{work_id}.first_unresolved_item",
        "historicalized_at_revision": committed_revision,
        "narrative": rewritten,
    }


def refresh_current_metadata(
    *,
    progress: Path,
    expected_revision: int,
    apply: bool,
) -> dict[str, Any]:
    """Refresh only the governed scheduler record and audited stale work narratives."""

    store = ProgressStore(progress)
    try:
        document = store.load()
    except ProgressError as exc:
        raise CurrentMetadataMutationError(str(exc)) from exc
    if document.revision != expected_revision:
        raise CurrentMetadataMutationError(
            f"revision changed: expected {expected_revision}, found {document.revision}"
        )

    work_item_ids = _audited_work_item_ids(document)
    proposed = deepcopy(document.data)
    try:
        generated_change = refresh_remaining_blocker_metadata(proposed)
    except (ProgressError, ValueError) as exc:
        raise CurrentMetadataMutationError(str(exc)) from exc

    committed_revision = expected_revision + 1
    work_item_changes = [
        _historicalize_work_item(
            proposed,
            work_id=work_id,
            committed_revision=committed_revision,
        )
        for work_id in work_item_ids
    ]

    preview = deepcopy(proposed)
    preview["revision"] = committed_revision
    remaining_findings = audit_current_metadata(ProgressDocument(preview))
    if remaining_findings:
        first = remaining_findings[0]
        raise CurrentMetadataMutationError(
            "refresh would leave current-metadata audit errors: "
            f"{first.get('code')} {first.get('path')}: {first.get('message')}"
        )

    try:
        commit = store.commit(
            proposed,
            expected_revision=expected_revision,
            apply=apply,
        )
    except (ConcurrentProgressUpdate, ProgressError) as exc:
        raise CurrentMetadataMutationError(str(exc)) from exc

    return {
        "report_version": 1,
        "kind": "current-metadata-refresh",
        "validation_mode": "current-scheduler-derivation",
        "progress": display_path(progress),
        "generated_current": {
            "path": GENERATED_CURRENT_PATH,
            "record": generated_change["generated_current"],
        },
        "historicalized_work_items": work_item_changes,
        "commit": commit.to_dict(),
    }


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Revision-guarded refresh of governed current scheduler metadata."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)
    child = subparsers.add_parser("refresh")
    child.add_argument("--progress", type=Path, default=DEFAULT_PROGRESS_PATH)
    child.add_argument("--expected-revision", type=int, required=True)
    mode = child.add_mutually_exclusive_group(required=True)
    mode.add_argument("--dry-run", action="store_true")
    mode.add_argument("--apply", action="store_true")
    child.add_argument("--json", action="store_true")
    return parser


def run(args: argparse.Namespace) -> dict[str, Any]:
    if args.command != "refresh":
        raise CurrentMetadataMutationError(f"unsupported operation {args.command!r}")
    return refresh_current_metadata(
        progress=args.progress,
        expected_revision=args.expected_revision,
        apply=bool(args.apply),
    )


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    try:
        report = run(args)
    except (OSError, ValueError, CurrentMetadataMutationError) as exc:
        print(f"current metadata refresh error: {exc}", file=sys.stderr)
        return 2
    if args.json:
        print(json.dumps(report, indent=2, sort_keys=True))
    else:
        mode = "APPLIED" if report["commit"]["applied"] else "DRY-RUN"
        print(
            f"Current metadata {mode}: revision "
            f"{report['commit']['previous_revision']} -> {report['commit']['revision']}"
        )
        print(
            f"historicalized_work_items={len(report['historicalized_work_items'])}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
