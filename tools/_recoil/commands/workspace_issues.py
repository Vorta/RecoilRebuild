#!/usr/bin/env python3
"""Record and inspect Recoil workspace/tool issues without allocating work."""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import re
import sys
from typing import Any, Mapping

from _recoil.lib.issue_sqlite import (
    IssueSQLiteStore,
    export_issue_document,
    validate_issue_database,
)
from _recoil.lib.live_progress import (
    ISSUE_LEDGER_VERSION,
    ConcurrentRevisionUpdate,
    LiveProgressError,
    validate_issue_ledger_v2,
)
from _recoil.lib.tooling import REPO_ROOT, configure_stdio


DEFAULT_LEDGER = REPO_ROOT / ".agent" / "WORKSPACE_ISSUES.sqlite3"
ISSUE_ID_RE = re.compile(r"^WSI-(\d{8})-(\d{3})$")
STATUSES = {"open", "in-progress", "resolved", "wont-fix"}
KINDS = {
    "tool-error",
    "workspace-issue",
    "instruction-gap",
    "environment-blocker",
    "improvement",
}
PROBLEM_KINDS = KINDS - {"improvement"}
SEVERITIES = {"critical", "high", "medium", "low", "info"}
BASE_REQUIRED = (
    "id",
    "status",
    "kind",
    "severity",
    "created",
    "updated",
    "summary",
    "area",
    "impact",
    "next_action",
)


def now_iso() -> str:
    return (
        datetime.now(timezone.utc)
        .replace(microsecond=0)
        .isoformat()
        .replace("+00:00", "Z")
    )


def today_key() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%d")


def empty_ledger() -> dict[str, Any]:
    return {
        "version": ISSUE_LEDGER_VERSION,
        "revision": 0,
        "id_sequences": {},
        "issues": [],
    }


def load_ledger(path: Path) -> dict[str, Any]:
    return export_issue_document(path)


def issue_store(path: Path) -> IssueSQLiteStore:
    return IssueSQLiteStore(path, validator=validate_issue_document)


def _nonempty(value: Any, field: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise ValueError(f"issue field {field!r} must be non-empty")
    return value.strip()


def validate_issue_document(data: Mapping[str, Any]) -> None:
    validate_issue_ledger_v2(data)
    revision = data.get("revision")
    if not isinstance(revision, int) or isinstance(revision, bool) or revision < 0:
        raise ValueError("workspace issue revision must be a non-negative integer")
    issues = data.get("issues")
    assert isinstance(issues, list)
    seen: set[str] = set()
    for issue in issues:
        if not isinstance(issue, Mapping):
            raise ValueError("workspace issue rows must be objects")
        issue_id = str(issue.get("id", ""))
        if ISSUE_ID_RE.fullmatch(issue_id) is None or issue_id in seen:
            raise ValueError(f"invalid or duplicate workspace issue id {issue_id!r}")
        seen.add(issue_id)
        missing = [field for field in BASE_REQUIRED if field not in issue]
        if missing:
            raise ValueError(
                f"workspace issue {issue_id} is missing: {', '.join(missing)}"
            )
        if issue.get("status") not in STATUSES:
            raise ValueError(f"workspace issue {issue_id} has invalid status")
        if issue.get("kind") not in KINDS:
            raise ValueError(f"workspace issue {issue_id} has invalid kind")
        if issue.get("severity") not in SEVERITIES:
            raise ValueError(f"workspace issue {issue_id} has invalid severity")
        for field in ("summary", "area", "impact", "next_action"):
            _nonempty(issue.get(field), field)
        for field in ("commands", "files", "tags", "history"):
            value = issue.get(field, [])
            if not isinstance(value, list):
                raise ValueError(
                    f"workspace issue {issue_id} field {field!r} must be a list"
                )


def _next_issue_id(data: dict[str, Any]) -> str:
    day = today_key()
    sequence_root = data.setdefault("id_sequences", {}).setdefault("issues", {})
    observed = max(
        (
            int(match.group(2))
            for issue in data.get("issues", [])
            if isinstance(issue, Mapping)
            and (match := ISSUE_ID_RE.fullmatch(str(issue.get("id", ""))))
            and match.group(1) == day
        ),
        default=0,
    )
    next_value = max(int(sequence_root.get(day, 0)), observed) + 1
    sequence_root[day] = next_value
    return f"WSI-{day}-{next_value:03d}"


def _payload_from_args(args: argparse.Namespace) -> dict[str, Any]:
    payload: dict[str, Any] = {}
    if args.from_json:
        decoded = json.loads(args.from_json)
        if not isinstance(decoded, dict):
            raise ValueError("--from-json must decode to one object")
        payload.update(decoded)
    for name in (
        "kind",
        "severity",
        "summary",
        "area",
        "impact",
        "next_action",
        "actual",
        "expected",
        "repro",
        "workaround",
        "requested_change",
        "benefit",
        "evidence",
    ):
        value = getattr(args, name, None)
        if value is not None:
            payload[name] = value
    for source, target in (
        ("command", "commands"),
        ("file", "files"),
        ("tag", "tags"),
    ):
        values = getattr(args, source, None)
        if values:
            payload[target] = list(values)
    return payload


def _create_issue(args: argparse.Namespace, *, improvement: bool) -> int:
    try:
        data = issue_store(Path(args.ledger)).load()
        if data["revision"] != args.expected_revision:
            raise ValueError(
                f"ledger revision changed: expected {args.expected_revision}, "
                f"found {data['revision']}"
            )
        payload = _payload_from_args(args)
        if improvement:
            payload["kind"] = "improvement"
            for field in ("requested_change", "benefit"):
                _nonempty(payload.get(field), field)
        else:
            if payload.get("kind") not in PROBLEM_KINDS:
                raise ValueError("--kind is required for a problem report")
            _nonempty(payload.get("actual"), "actual")
        for field in ("severity", "summary", "area", "impact", "next_action"):
            _nonempty(payload.get(field), field)
        if payload["severity"] not in SEVERITIES:
            raise ValueError("invalid issue severity")
        timestamp = now_iso()
        issue_id = _next_issue_id(data)
        issue = {
            "id": issue_id,
            "status": "open",
            "created": timestamp,
            "updated": timestamp,
            "commands": [],
            "files": [],
            "tags": [],
            **payload,
            "history": [
                {
                    "action": "created",
                    "at": timestamp,
                    "note": "Issue recorded for direct single-agent maintenance.",
                }
            ],
        }
        data["issues"].append(issue)
        validate_issue_document(data)
        commit = issue_store(Path(args.ledger)).commit(
            data,
            expected_revision=args.expected_revision,
            apply=bool(args.apply),
        )
        print(json.dumps({"issue": issue, "commit": commit.to_dict()}, indent=2))
        return 0
    except (ValueError, OSError, LiveProgressError, ConcurrentRevisionUpdate) as exc:
        print(f"recoil issue: {exc}", file=sys.stderr)
        return 2


def command_report(args: argparse.Namespace) -> int:
    return _create_issue(args, improvement=False)


def command_request(args: argparse.Namespace) -> int:
    return _create_issue(args, improvement=True)


def command_list(args: argparse.Namespace) -> int:
    try:
        rows = [
            issue
            for issue in load_ledger(Path(args.ledger))["issues"]
            if (args.status == "all" or issue.get("status") == args.status)
            and (args.kind is None or issue.get("kind") == args.kind)
        ][: args.limit]
        if args.json:
            print(json.dumps(rows, indent=2))
        elif not rows:
            print("No matching workspace issues.")
        else:
            for issue in rows:
                print(
                    f"{issue['id']} {issue['status']} {issue['severity']} "
                    f"{issue['summary']}"
                )
        return 0
    except (OSError, LiveProgressError) as exc:
        print(f"recoil issue list: {exc}", file=sys.stderr)
        return 2


def command_show(args: argparse.Namespace) -> int:
    try:
        issue = next(
            (
                row
                for row in load_ledger(Path(args.ledger))["issues"]
                if row.get("id") == args.issue_id
            ),
            None,
        )
        if issue is None:
            print(f"workspace issue not found: {args.issue_id}", file=sys.stderr)
            return 1
        print(json.dumps(issue, indent=2) if args.json else _format_issue(issue))
        return 0
    except (OSError, LiveProgressError) as exc:
        print(f"recoil issue show: {exc}", file=sys.stderr)
        return 2


def _format_issue(issue: Mapping[str, Any]) -> str:
    lines = [
        f"{issue['id']} [{issue['status']}] {issue['summary']}",
        f"kind={issue['kind']} severity={issue['severity']}",
        f"area={issue['area']}",
        f"impact={issue['impact']}",
        f"next={issue['next_action']}",
    ]
    return "\n".join(lines)


def transition_issue(
    args: argparse.Namespace,
    *,
    status: str,
    action: str,
    note_field: str,
) -> int:
    try:
        data = issue_store(Path(args.ledger)).load()
        if data["revision"] != args.expected_revision:
            raise ValueError(
                f"ledger revision changed: expected {args.expected_revision}, "
                f"found {data['revision']}"
            )
        issue = next(
            (
                row
                for row in data["issues"]
                if row.get("id") == args.issue_id
            ),
            None,
        )
        if issue is None:
            raise ValueError(f"unknown workspace issue {args.issue_id}")
        note = _nonempty(getattr(args, note_field), note_field)
        timestamp = now_iso()
        issue["status"] = status
        issue["updated"] = timestamp
        issue[note_field] = note
        issue.setdefault("history", []).append(
            {"action": action, "at": timestamp, "note": note}
        )
        commit = issue_store(Path(args.ledger)).commit(
            data,
            expected_revision=args.expected_revision,
            apply=bool(args.apply),
        )
        print(json.dumps({"issue": issue, "commit": commit.to_dict()}, indent=2))
        return 0
    except (ValueError, OSError, LiveProgressError, ConcurrentRevisionUpdate) as exc:
        print(f"recoil issue {action}: {exc}", file=sys.stderr)
        return 2


def command_audit(args: argparse.Namespace) -> int:
    findings = validate_issue_database(
        Path(args.ledger), document_validator=validate_issue_document
    )
    payload = {"passed": not findings, "findings": findings}
    if args.json:
        print(json.dumps(payload, indent=2))
    else:
        print("workspace issue audit OK" if not findings else "\n".join(findings))
    return 0 if not findings or not args.strict else 1


def _add_mutation(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--expected-revision", required=True, type=int)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--dry-run", action="store_true")
    mode.add_argument("--apply", action="store_true")


def _add_create(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--ledger", default=str(DEFAULT_LEDGER))
    parser.add_argument("--from-json")
    parser.add_argument("--severity", choices=sorted(SEVERITIES))
    parser.add_argument("--summary")
    parser.add_argument("--area")
    parser.add_argument("--impact")
    parser.add_argument("--next-action", dest="next_action")
    parser.add_argument("--evidence")
    parser.add_argument("--command", action="append", default=[])
    parser.add_argument("--file", action="append", default=[])
    parser.add_argument("--tag", action="append", default=[])
    _add_mutation(parser)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Record and inspect Recoil workspace/tool issues."
    )
    children = parser.add_subparsers(dest="command", required=True)

    report = children.add_parser("report")
    _add_create(report)
    report.add_argument("--kind", choices=sorted(PROBLEM_KINDS))
    report.add_argument("--actual")
    report.add_argument("--expected")
    report.add_argument("--repro")
    report.add_argument("--workaround")
    report.set_defaults(func=command_report)

    request = children.add_parser("request")
    _add_create(request)
    request.add_argument("--requested-change")
    request.add_argument("--benefit")
    request.set_defaults(func=command_request)

    listing = children.add_parser("list")
    listing.add_argument("--ledger", default=str(DEFAULT_LEDGER))
    listing.add_argument(
        "--status", default="open", choices=["all", *sorted(STATUSES)]
    )
    listing.add_argument("--kind", choices=sorted(KINDS))
    listing.add_argument("--limit", type=int, default=80)
    listing.add_argument("--json", action="store_true")
    listing.set_defaults(func=command_list)

    show = children.add_parser("show")
    show.add_argument("issue_id")
    show.add_argument("--ledger", default=str(DEFAULT_LEDGER))
    show.add_argument("--json", action="store_true")
    show.set_defaults(func=command_show)

    resolve = children.add_parser("resolve")
    resolve.add_argument("issue_id")
    resolve.add_argument("--ledger", default=str(DEFAULT_LEDGER))
    resolve.add_argument("--resolution", required=True)
    _add_mutation(resolve)
    resolve.set_defaults(
        func=lambda args: transition_issue(
            args, status="resolved", action="resolved", note_field="resolution"
        )
    )

    wont_fix = children.add_parser("wont-fix")
    wont_fix.add_argument("issue_id")
    wont_fix.add_argument("--ledger", default=str(DEFAULT_LEDGER))
    wont_fix.add_argument("--reason", required=True)
    _add_mutation(wont_fix)
    wont_fix.set_defaults(
        func=lambda args: transition_issue(
            args, status="wont-fix", action="wont-fix", note_field="reason"
        )
    )

    reopen = children.add_parser("reopen")
    reopen.add_argument("issue_id")
    reopen.add_argument("--ledger", default=str(DEFAULT_LEDGER))
    reopen.add_argument("--reason", required=True)
    _add_mutation(reopen)
    reopen.set_defaults(
        func=lambda args: transition_issue(
            args, status="open", action="reopened", note_field="reason"
        )
    )

    audit = children.add_parser("audit")
    audit.add_argument("--ledger", default=str(DEFAULT_LEDGER))
    audit.add_argument("--strict", action="store_true")
    audit.add_argument("--json", action="store_true")
    audit.set_defaults(func=command_audit)
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
