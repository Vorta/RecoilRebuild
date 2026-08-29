#!/usr/bin/env python3
"""Record and inspect agent tooling/process issues for future agents."""

from __future__ import annotations

import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import argparse
from contextlib import contextmanager
from dataclasses import dataclass
from datetime import datetime, timezone
import json
from pathlib import Path
import re
import sys
from typing import Any, Mapping

from _recoil.lib.progress import (
    DEFAULT_PROGRESS_PATH,
    ProgressDocument,
    ProgressError,
    ProgressStore,
    normalize_resource_claims,
    resource_claim_conflicts,
    work_resource_claims,
)
from _recoil.lib.live_progress import (
    ISSUE_LEDGER_VERSION,
    ConcurrentRevisionUpdate,
    LiveProgressError,
    RevisionStore,
    atomic_replace,
    canonical_json_bytes,
    revision_lock,
    validate_issue_ledger_v2,
)
from _recoil.lib.issue_sqlite import (
    IssueSQLiteStore,
    export_issue_document,
    validate_issue_database,
)
from _recoil.lib.git_change_control import (
    GIT_WORKSPACE_BASELINE_SCHEMA,
    GitChangeControlError,
    capture_clean_git_baseline,
    validate_git_baseline_descriptor,
)
from _recoil.lib.worktree_control import (
    WorktreeControlError,
    authenticated_validation_command_tokens,
    capture_packet_git_closeout as capture_git_closeout,
)
from _recoil.lib.progress_sqlite import ProgressSQLiteStore, read_progress_metadata
from _recoil.lib.tooling import REPO_ROOT, configure_stdio, display_path


DEFAULT_LEDGER = REPO_ROOT / ".agent" / "WORKSPACE_ISSUES.sqlite3"
ISSUE_ID_RE = re.compile(r"^WSI-(\d{8})-(\d{3})$")
STATUSES = {"open", "in-progress", "resolved", "wont-fix"}
KINDS = {"tool-error", "workspace-issue", "instruction-gap", "environment-blocker", "improvement"}
PROBLEM_KINDS = KINDS - {"improvement"}
SEVERITIES = {"critical", "high", "medium", "low", "info"}
ISSUE_WORK_ROLES = {"recoil_tool_maintainer", "recoil_verifier"}
ISSUE_WORK_STATES = {"ready", "active", "closed"}
ISSUE_WORK_OUTCOMES = {"returned", "closed", "abandoned"}
SELF_HOSTING_NATIVE_GIT_PACKET = (
    "issue:work:wsi-20260826-001:native-git-change-control-migration"
)
SELF_HOSTING_NATIVE_GIT_RESERVATION = (
    SELF_HOSTING_NATIVE_GIT_PACKET + ":attempt:1"
)
SELF_HOSTING_NATIVE_GIT_BASELINE_COMMIT = (
    "ce8422afb2b69a870a0ed74b633e2979d01d0763"
)
SELF_HOSTING_NATIVE_GIT_BRANCH = "master"

BASE_REQUIRED = ("id", "status", "kind", "severity", "created", "updated", "summary", "area", "impact", "next_action")
PROBLEM_REQUIRED = ("actual",)
IMPROVEMENT_REQUIRED = ("requested_change", "benefit")


@dataclass(frozen=True)
class ValidationIssue:
    path: str
    message: str


def now_iso() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def today_key() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%d")


def empty_ledger() -> dict[str, Any]:
    return {
        "version": ISSUE_LEDGER_VERSION,
        "revision": 0,
        "id_sequences": {},
        "issues": [],
        "work_packets": [],
        "reservations": [],
    }


def load_ledger(path: Path) -> dict[str, Any]:
    if path.suffix.casefold() == ".json":
        if not path.exists():
            return empty_ledger()
        with path.open("r", encoding="utf-8") as handle:
            data = json.load(handle)
        if not isinstance(data, dict):
            raise ValueError(f"{path}: expected JSON object")
        return data
    return export_issue_document(path)


def issue_store(path: Path) -> IssueSQLiteStore | RevisionStore:
    if path.suffix.casefold() == ".json":
        return RevisionStore(
            path,
            schema_field="version",
            schema_version=ISSUE_LEDGER_VERSION,
            validator=validate_issue_document,
            initializer=empty_ledger,
        )
    return IssueSQLiteStore(path, validator=validate_issue_document)


@contextmanager
def cross_ledger_reservation_critical_section(
    progress_path: Path,
    issue_ledger_path: Path,
):
    """Serialize cross-ledger reservation decisions without owning either DB.

    The lock is deliberately separate from each authority's revision lock, so
    SQLite transactions and legacy JSON CAS can retain their existing locking.
    Both reconstruction explicit creation and issue-work reservation use this
    exact pair-keyed critical section; neither ledger is written merely by
    acquiring it.
    """

    # One conservative lock per issue-ledger directory serializes every
    # progress/issue reservation pairing used by that authority.  The paths
    # remain independently revision guarded inside the critical section.
    del progress_path
    anchor = Path(issue_ledger_path).resolve().with_name(
        ".recoil-cross-ledger-reservation"
    )
    with revision_lock(anchor):
        yield


def require_expected_ledger_revision(path: Path, expected: int) -> dict[str, Any]:
    current = issue_store(path).load()
    if current["revision"] != expected:
        raise ValueError(
            f"ledger revision changed: expected {expected}, found {current['revision']}"
        )
    return current


def _capture_protected_progress_database(path: Path) -> dict[str, Any]:
    """Capture exact revision/schema/count evidence without content summaries."""

    resolved = Path(path).resolve(strict=True)
    metadata = read_progress_metadata(resolved)
    validation = ProgressSQLiteStore(resolved).validate_integrity()
    document = ProgressStore(resolved).load()
    evidence = document.collection("evidence")
    work_items = document.collection("work_items")
    return {
        "path": str(resolved),
        "revision_vector": metadata.revision_vector.to_dict(),
        "schema_version": metadata.schema_version,
        "user_version": metadata.user_version,
        "integrity_check": list(validation.integrity_check),
        "foreign_key_violation_count": len(validation.foreign_key_violations),
        "evidence_row_count": len(evidence),
        "certificate_evidence_row_count": sum(
            "certificate" in str(row.get("kind", "")).casefold()
            for row in evidence.values()
            if isinstance(row, Mapping)
        ),
        "work_item_row_count": len(work_items),
        "active_progress_reservation_count": sum(
            isinstance(row, Mapping)
            and row.get("state") == "active"
            and isinstance(row.get("reservation"), Mapping)
            and row["reservation"].get("state") == "active"
            for row in work_items.values()
        ),
    }


def validate_issue_document(data: Mapping[str, Any]) -> None:
    validate_issue_ledger_v2(data)
    findings = validate_ledger(dict(data))
    if findings:
        raise LiveProgressError(
            "invalid workspace issue ledger: "
            + "; ".join(f"{row.path}: {row.message}" for row in findings[:8])
        )


def require_text(data: dict[str, Any], field: str) -> str:
    value = data.get(field)
    if not isinstance(value, str) or not value.strip():
        raise ValueError(f"missing required field: {field}")
    return value.strip()


def list_field(values: list[str] | None) -> list[str]:
    return [value.strip() for value in values or [] if value.strip()]


def next_issue_id(
    issues: list[dict[str, Any]],
    *,
    date_key: str | None = None,
    id_sequences: dict[str, Any] | None = None,
) -> str:
    key = date_key or today_key()
    highest = 0
    for issue in issues:
        issue_id = issue.get("id")
        if not isinstance(issue_id, str):
            continue
        match = ISSUE_ID_RE.match(issue_id)
        if match is None or match.group(1) != key:
            continue
        highest = max(highest, int(match.group(2)))
    if id_sequences is not None:
        issue_sequences = id_sequences.setdefault("issue", {})
        if not isinstance(issue_sequences, dict):
            raise ValueError("id_sequences.issue must be an object")
        recorded = issue_sequences.get(key, 0)
        if not isinstance(recorded, int) or isinstance(recorded, bool) or recorded < 0:
            raise ValueError(f"id_sequences.issue.{key} must be a non-negative integer")
        highest = max(highest, recorded)
        issue_sequences[key] = highest + 1
    return f"WSI-{key}-{highest + 1:03d}"


def history_event(action: str, note: str) -> dict[str, str]:
    return {"at": now_iso(), "action": action, "note": note}


def normalize_entry(raw: dict[str, Any], *, issue_id: str, kind: str) -> dict[str, Any]:
    timestamp = now_iso()
    entry = {
        "id": issue_id,
        "status": "open",
        "kind": kind,
        "severity": require_text(raw, "severity"),
        "created": timestamp,
        "updated": timestamp,
        "summary": require_text(raw, "summary"),
        "area": require_text(raw, "area"),
        "impact": require_text(raw, "impact"),
        "next_action": require_text(raw, "next_action"),
        "commands": list_field(raw.get("commands") if isinstance(raw.get("commands"), list) else None),
        "files": list_field(raw.get("files") if isinstance(raw.get("files"), list) else None),
        "tags": list_field(raw.get("tags") if isinstance(raw.get("tags"), list) else None),
        "history": [history_event("created", "Issue recorded for future agent tooling/process repair.")],
    }
    for optional in ("expected", "workaround", "resolution"):
        value = raw.get(optional)
        if isinstance(value, str) and value.strip():
            entry[optional] = value.strip()

    if kind == "improvement":
        entry["requested_change"] = require_text(raw, "requested_change")
        entry["benefit"] = require_text(raw, "benefit")
        evidence = raw.get("evidence")
        if isinstance(evidence, str) and evidence.strip():
            entry["evidence"] = evidence.strip()
    else:
        entry["actual"] = require_text(raw, "actual")
        repro = raw.get("repro")
        evidence = raw.get("evidence")
        if isinstance(repro, str) and repro.strip():
            entry["repro"] = repro.strip()
        if isinstance(evidence, str) and evidence.strip():
            entry["evidence"] = evidence.strip()
        if "repro" not in entry and "evidence" not in entry:
            raise ValueError("problem reports require either `repro` or `evidence`")

    return entry


def merge_json_payload(path_text: str | None, cli: dict[str, Any]) -> dict[str, Any]:
    payload: dict[str, Any] = {}
    if path_text:
        path = Path(path_text)
        with path.open("r", encoding="utf-8") as handle:
            loaded = json.load(handle)
        if not isinstance(loaded, dict):
            raise ValueError(f"{path}: expected JSON object")
        payload.update(loaded)
    for key, value in cli.items():
        if value is None:
            continue
        if isinstance(value, list) and not value:
            continue
        payload[key] = value
    return payload


def validate_issue(issue: Any, index: int, seen_ids: set[str]) -> list[ValidationIssue]:
    path = f"issues[{index}]"
    findings: list[ValidationIssue] = []
    if not isinstance(issue, dict):
        return [ValidationIssue(path, "expected object")]

    for field in BASE_REQUIRED:
        if not isinstance(issue.get(field), str) or not issue.get(field, "").strip():
            findings.append(ValidationIssue(f"{path}.{field}", "expected non-empty string"))

    issue_id = issue.get("id")
    if isinstance(issue_id, str):
        if ISSUE_ID_RE.match(issue_id) is None:
            findings.append(ValidationIssue(f"{path}.id", "expected WSI-YYYYMMDD-NNN"))
        if issue_id in seen_ids:
            findings.append(ValidationIssue(f"{path}.id", "duplicate id"))
        seen_ids.add(issue_id)

    status = issue.get("status")
    if isinstance(status, str) and status not in STATUSES:
        findings.append(ValidationIssue(f"{path}.status", f"expected one of {sorted(STATUSES)}"))
    kind = issue.get("kind")
    if isinstance(kind, str) and kind not in KINDS:
        findings.append(ValidationIssue(f"{path}.kind", f"expected one of {sorted(KINDS)}"))
    severity = issue.get("severity")
    if isinstance(severity, str) and severity not in SEVERITIES:
        findings.append(ValidationIssue(f"{path}.severity", f"expected one of {sorted(SEVERITIES)}"))

    if kind == "improvement":
        for field in IMPROVEMENT_REQUIRED:
            if not isinstance(issue.get(field), str) or not issue.get(field, "").strip():
                findings.append(ValidationIssue(f"{path}.{field}", "expected non-empty string"))
    elif isinstance(kind, str):
        for field in PROBLEM_REQUIRED:
            if not isinstance(issue.get(field), str) or not issue.get(field, "").strip():
                findings.append(ValidationIssue(f"{path}.{field}", "expected non-empty string"))
        if not issue.get("repro") and not issue.get("evidence"):
            findings.append(ValidationIssue(path, "problem report needs repro or evidence"))

    for field in ("commands", "files", "tags", "history"):
        if field in issue and not isinstance(issue[field], list):
            findings.append(ValidationIssue(f"{path}.{field}", "expected list"))
    return findings


def validate_ledger(data: dict[str, Any]) -> list[ValidationIssue]:
    findings: list[ValidationIssue] = []
    if data.get("version") != ISSUE_LEDGER_VERSION:
        findings.append(ValidationIssue("version", f"expected {ISSUE_LEDGER_VERSION}"))
    revision = data.get("revision")
    if not isinstance(revision, int) or isinstance(revision, bool) or revision < 0:
        findings.append(ValidationIssue("revision", "expected non-negative integer"))
    issues = data.get("issues")
    if not isinstance(issues, list):
        return [*findings, ValidationIssue("issues", "expected list")]
    seen_ids: set[str] = set()
    for index, issue in enumerate(issues):
        findings.extend(validate_issue(issue, index, seen_ids))
    issue_by_id = {
        str(issue.get("id")): issue
        for issue in issues
        if isinstance(issue, dict) and isinstance(issue.get("id"), str)
    }
    issue_ids = set(issue_by_id)
    work_packets = data.get("work_packets", [])
    reservations = data.get("reservations", [])
    if not isinstance(work_packets, list):
        findings.append(ValidationIssue("work_packets", "expected list"))
        work_packets = []
    if not isinstance(reservations, list):
        findings.append(ValidationIssue("reservations", "expected list"))
        reservations = []
    packet_ids: set[str] = set()
    for index, packet in enumerate(work_packets):
        path = f"work_packets[{index}]"
        if not isinstance(packet, dict):
            findings.append(ValidationIssue(path, "expected object"))
            continue
        packet_id = packet.get("id")
        if not isinstance(packet_id, str) or not packet_id.strip():
            findings.append(ValidationIssue(f"{path}.id", "expected non-empty string"))
        elif not packet_id.startswith("issue:work:"):
            findings.append(ValidationIssue(f"{path}.id", "expected issue:work: namespace"))
        elif packet_id in packet_ids:
            findings.append(ValidationIssue(f"{path}.id", "duplicate id"))
        else:
            packet_ids.add(packet_id)
        if packet.get("issue_id") not in issue_ids:
            findings.append(ValidationIssue(f"{path}.issue_id", "unknown issue id"))
        if packet.get("handoff_role") not in ISSUE_WORK_ROLES:
            findings.append(ValidationIssue(f"{path}.handoff_role", "unsupported issue-work role"))
        if packet.get("state") not in ISSUE_WORK_STATES:
            findings.append(ValidationIssue(f"{path}.state", "unsupported packet state"))
        if packet.get("semantic_contract_version") != 1:
            findings.append(ValidationIssue(f"{path}.semantic_contract_version", "expected 1"))
        for field in (
            "scope",
            "next_command",
            "allowed_paths",
            "forbidden_paths",
            "validation_commands",
            "required_return_fields",
            "resource_claims",
        ):
            value = packet.get(field)
            if field in {"scope", "next_command"}:
                if not isinstance(value, str) or not value.strip():
                    findings.append(ValidationIssue(f"{path}.{field}", "expected non-empty string"))
            elif not isinstance(value, list) or not value:
                findings.append(ValidationIssue(f"{path}.{field}", "expected non-empty list"))
            elif field != "resource_claims" and any(
                not isinstance(item, str) or not item.strip() for item in value
            ):
                findings.append(ValidationIssue(f"{path}.{field}", "expected non-empty strings"))
        claims = packet.get("resource_claims")
        if isinstance(claims, list):
            try:
                normalized = normalize_resource_claims(
                    row for row in claims if isinstance(row, dict)
                )
                if normalized != claims or len(normalized) != len(claims):
                    findings.append(
                        ValidationIssue(f"{path}.resource_claims", "claims must be normalized and duplicate-free")
                    )
            except ProgressError as exc:
                findings.append(ValidationIssue(f"{path}.resource_claims", str(exc)))
        state = packet.get("state")
        reservation_id = packet.get("reservation_id")
        commands = packet.get("validation_commands")
        next_command = packet.get("next_command")
        validation_command_contract_version = packet.get(
            "validation_command_contract_version"
        )
        if (
            validation_command_contract_version is not None
            and validation_command_contract_version != 1
        ):
            findings.append(
                ValidationIssue(
                    f"{path}.validation_command_contract_version",
                    "expected 1 when present",
                )
            )
        if (
            validation_command_contract_version == 1
            and state in {"ready", "active"}
            and isinstance(commands, list)
            and commands
            and all(isinstance(command, str) and command.strip() for command in commands)
            and isinstance(next_command, str)
            and next_command.strip()
            and isinstance(claims, list)
        ):
            if len(commands) != 1:
                findings.append(
                    ValidationIssue(
                        f"{path}.validation_commands",
                        "launchable packet requires exactly one stored validation command",
                    )
                )
            elif next_command != commands[0]:
                findings.append(
                    ValidationIssue(
                        f"{path}.next_command",
                        "must exactly equal the single stored validation command",
                    )
                )
            else:
                try:
                    authenticated_validation_command_tokens(
                        commands[0],
                        require_public_route=False,
                        resource_claims=claims,
                    )
                except WorktreeControlError as exc:
                    findings.append(
                        ValidationIssue(
                            f"{path}.validation_commands[0]",
                            str(exc),
                        )
                    )
        if state == "ready" and reservation_id is not None:
            findings.append(ValidationIssue(f"{path}.reservation_id", "ready packet must be unreserved"))
        if state == "closed":
            if reservation_id is not None:
                findings.append(ValidationIssue(f"{path}.reservation_id", "closed packet must be unreserved"))
            if packet.get("outcome") not in ISSUE_WORK_OUTCOMES:
                findings.append(ValidationIssue(f"{path}.outcome", "closed packet requires a valid outcome"))
        issue = issue_by_id.get(str(packet.get("issue_id")))
        if state in {"ready", "active"} and isinstance(issue, dict) and issue.get("status") not in {
            "open",
            "in-progress",
        }:
            findings.append(ValidationIssue(path, "open packet belongs to a closed issue"))
    reservation_ids: set[str] = set()
    active_by_packet: dict[str, str] = {}
    for index, reservation in enumerate(reservations):
        path = f"reservations[{index}]"
        if not isinstance(reservation, dict):
            findings.append(ValidationIssue(path, "expected object"))
            continue
        reservation_id = reservation.get("id")
        packet_id = reservation.get("packet_id")
        if not isinstance(reservation_id, str) or not reservation_id:
            findings.append(ValidationIssue(f"{path}.id", "expected non-empty string"))
        elif reservation_id in reservation_ids:
            findings.append(ValidationIssue(f"{path}.id", "duplicate id"))
        else:
            reservation_ids.add(reservation_id)
        if packet_id not in packet_ids:
            findings.append(ValidationIssue(f"{path}.packet_id", "unknown packet id"))
        if reservation.get("state") not in {"active", "released"}:
            findings.append(ValidationIssue(f"{path}.state", "expected active or released"))
        if reservation.get("semantic_contract_version") != 1:
            findings.append(ValidationIssue(f"{path}.semantic_contract_version", "expected 1"))
        if reservation.get("state") == "active" and isinstance(packet_id, str):
            if packet_id in active_by_packet:
                findings.append(ValidationIssue(path, "packet has multiple active reservations"))
            active_by_packet[packet_id] = str(reservation_id)
        claims = reservation.get("resource_claims")
        if not isinstance(claims, list) or not claims:
            findings.append(ValidationIssue(f"{path}.resource_claims", "expected non-empty list"))
        else:
            try:
                normalized = normalize_resource_claims(
                    row for row in claims if isinstance(row, dict)
                )
                if normalized != claims or len(normalized) != len(claims):
                    findings.append(
                        ValidationIssue(f"{path}.resource_claims", "claims must be normalized and duplicate-free")
                    )
                packet = next(
                    (
                        row for row in work_packets
                        if isinstance(row, dict) and row.get("id") == packet_id
                    ),
                    None,
                )
                if isinstance(packet, dict) and normalized != packet.get("resource_claims"):
                    findings.append(ValidationIssue(f"{path}.resource_claims", "reservation claims differ from packet"))
            except ProgressError as exc:
                findings.append(ValidationIssue(f"{path}.resource_claims", str(exc)))
        evidence_ids = reservation.get("evidence_ids")
        if not isinstance(evidence_ids, list) or any(
            not isinstance(item, str) or not item.strip() for item in evidence_ids
        ):
            findings.append(ValidationIssue(f"{path}.evidence_ids", "expected a list of non-empty strings"))
        git_baseline = reservation.get("git_workspace_baseline")
        if reservation.get("state") == "active":
            exact_self_hosting = (
                packet_id == SELF_HOSTING_NATIVE_GIT_PACKET
                and reservation_id == SELF_HOSTING_NATIVE_GIT_RESERVATION
            )
            if git_baseline is None and not exact_self_hosting:
                findings.append(ValidationIssue(
                    f"{path}.git_workspace_baseline",
                    "active reservation requires an immutable Git workspace baseline",
                ))
            elif git_baseline is not None and (
                not isinstance(git_baseline, Mapping)
                or git_baseline.get("schema") != GIT_WORKSPACE_BASELINE_SCHEMA
            ):
                findings.append(ValidationIssue(
                    f"{path}.git_workspace_baseline",
                    "unsupported Git workspace baseline descriptor",
                ))
            elif isinstance(git_baseline, Mapping):
                packet = next(
                    (
                        row for row in work_packets
                        if isinstance(row, dict) and row.get("id") == packet_id
                    ),
                    None,
                )
                if isinstance(packet, Mapping):
                    try:
                        validate_git_baseline_descriptor(
                            git_baseline,
                            packet_id=str(packet_id or ""),
                            writable_paths=packet.get("allowed_paths", []),
                        )
                    except (ValueError, GitChangeControlError) as exc:
                        findings.append(ValidationIssue(
                            f"{path}.git_workspace_baseline",
                            f"Git workspace baseline validation failed: {exc}",
                        ))
        if reservation.get("state") == "active":
            if reservation.get("released") is not None or reservation.get("outcome") is not None:
                findings.append(ValidationIssue(path, "active reservation cannot have release metadata"))
        elif reservation.get("state") == "released":
            if not isinstance(reservation.get("released"), str) or not reservation.get("released", "").strip():
                findings.append(ValidationIssue(f"{path}.released", "released reservation needs a timestamp"))
            if reservation.get("outcome") not in ISSUE_WORK_OUTCOMES:
                findings.append(ValidationIssue(f"{path}.outcome", "released reservation needs a valid outcome"))
    for index, packet in enumerate(work_packets):
        if not isinstance(packet, dict) or not isinstance(packet.get("id"), str):
            continue
        packet_id = packet["id"]
        active_id = active_by_packet.get(packet_id)
        if packet.get("state") == "active" and packet.get("reservation_id") != active_id:
            findings.append(
                ValidationIssue(f"work_packets[{index}].reservation_id", "active packet/reservation mismatch")
            )
        if packet.get("state") != "active" and active_id is not None:
            findings.append(ValidationIssue(f"work_packets[{index}].state", "active reservation on inactive packet"))
    return findings


def find_issue(issues: list[dict[str, Any]], issue_id: str) -> dict[str, Any] | None:
    for issue in issues:
        if issue.get("id") == issue_id:
            return issue
    return None


def print_issue_row(issue: dict[str, Any]) -> None:
    print(
        f"{issue['id']} {issue['status']:<11} {issue['severity']:<8} "
        f"{issue['kind']:<19} {issue['area']}: {issue['summary']}"
    )
    print(f"  next: {issue['next_action']}")


def print_issue_detail(issue: dict[str, Any]) -> None:
    print(f"{issue['id']} [{issue['status']}] {issue['severity']} {issue['kind']}")
    print(f"area: {issue['area']}")
    print(f"summary: {issue['summary']}")
    print(f"impact: {issue['impact']}")
    print(f"next_action: {issue['next_action']}")
    for field in ("actual", "expected", "repro", "evidence", "requested_change", "benefit", "workaround", "resolution"):
        value = issue.get(field)
        if value:
            print()
            print(f"{field}:")
            print(value)
    for field in ("commands", "files", "tags"):
        values = issue.get(field)
        if values:
            print()
            print(f"{field}:")
            for value in values:
                print(f"- {value}")
    history = issue.get("history")
    if history:
        print()
        print("history:")
        for event in history:
            if isinstance(event, dict):
                print(f"- {event.get('at', '?')} {event.get('action', '?')}: {event.get('note', '')}")


def print_validation_help(error: str, *, command: str) -> None:
    print(f"recoil issue: {error}", file=sys.stderr)
    if command == "request":
        print(
            "example: python tools/recoil.py issue request --severity medium --summary \"...\" "
            "--area tools/recoil.py --impact \"...\" --next-action \"...\" "
            "--requested-change \"...\" --benefit \"...\"",
            file=sys.stderr,
        )
    else:
        print(
            "example: python tools/recoil.py issue report --kind tool-error --severity high "
            "--summary \"...\" --area tools/recoil.py --impact \"...\" --actual \"...\" "
            "--repro \"...\" --next-action \"...\"",
            file=sys.stderr,
        )


def command_report(args: argparse.Namespace) -> int:
    ledger_path = Path(args.ledger)
    try:
        data = require_expected_ledger_revision(ledger_path, args.expected_revision)
        issues = data.setdefault("issues", [])
        if not isinstance(issues, list):
            raise ValueError("ledger field `issues` must be a list")
        raw = merge_json_payload(
            args.from_json,
            {
                "severity": args.severity,
                "summary": args.summary,
                "area": args.area,
                "impact": args.impact,
                "next_action": args.next_action,
                "actual": args.actual,
                "expected": args.expected,
                "repro": args.repro,
                "evidence": args.evidence,
                "workaround": args.workaround,
                "commands": args.command,
                "files": args.file,
                "tags": args.tag,
            },
        )
        kind = args.kind or raw.get("kind")
        if kind not in PROBLEM_KINDS:
            raise ValueError(f"problem report requires --kind from {sorted(PROBLEM_KINDS)}")
        sequences = data.setdefault("id_sequences", {})
        if not isinstance(sequences, dict):
            raise ValueError("ledger field `id_sequences` must be an object")
        entry = normalize_entry(
            raw,
            issue_id=next_issue_id(issues, id_sequences=sequences),
            kind=kind,
        )
    except (OSError, ValueError, json.JSONDecodeError, LiveProgressError) as exc:
        print_validation_help(str(exc), command="report")
        return 2
    issues.append(entry)
    try:
        commit = issue_store(ledger_path).commit(
            data, expected_revision=args.expected_revision, apply=args.apply
        )
    except (LiveProgressError, ConcurrentRevisionUpdate) as exc:
        print_validation_help(str(exc), command="report")
        return 2
    print(json.dumps({"commit": commit.to_dict(), "issue": entry}, indent=2))
    return 0


def command_request(args: argparse.Namespace) -> int:
    ledger_path = Path(args.ledger)
    try:
        data = require_expected_ledger_revision(ledger_path, args.expected_revision)
        issues = data.setdefault("issues", [])
        if not isinstance(issues, list):
            raise ValueError("ledger field `issues` must be a list")
        raw = merge_json_payload(
            args.from_json,
            {
                "severity": args.severity,
                "summary": args.summary,
                "area": args.area,
                "impact": args.impact,
                "next_action": args.next_action,
                "requested_change": args.requested_change,
                "benefit": args.benefit,
                "evidence": args.evidence,
                "commands": args.command,
                "files": args.file,
                "tags": args.tag,
            },
        )
        sequences = data.setdefault("id_sequences", {})
        if not isinstance(sequences, dict):
            raise ValueError("ledger field `id_sequences` must be an object")
        entry = normalize_entry(
            raw,
            issue_id=next_issue_id(issues, id_sequences=sequences),
            kind="improvement",
        )
    except (OSError, ValueError, json.JSONDecodeError, LiveProgressError) as exc:
        print_validation_help(str(exc), command="request")
        return 2
    issues.append(entry)
    try:
        commit = issue_store(ledger_path).commit(
            data, expected_revision=args.expected_revision, apply=args.apply
        )
    except (LiveProgressError, ConcurrentRevisionUpdate) as exc:
        print_validation_help(str(exc), command="request")
        return 2
    print(json.dumps({"commit": commit.to_dict(), "issue": entry}, indent=2))
    return 0


def command_list(args: argparse.Namespace) -> int:
    try:
        data = load_ledger(Path(args.ledger))
    except (OSError, ValueError, json.JSONDecodeError, LiveProgressError) as exc:
        print(str(exc), file=sys.stderr)
        return 2
    issues = data.get("issues", [])
    if not isinstance(issues, list):
        print("ledger field `issues` must be a list", file=sys.stderr)
        return 2
    selected = [
        issue
        for issue in issues
        if isinstance(issue, dict)
        and (args.status == "all" or issue.get("status") == args.status)
        and (args.kind is None or issue.get("kind") == args.kind)
    ]
    selected = selected[: args.limit] if args.limit >= 0 else selected
    if args.json:
        print(json.dumps(selected, indent=2))
        return 0
    if not selected:
        print("No matching agent tooling/process issues.")
        return 0
    for issue in selected:
        print_issue_row(issue)
    return 0


def command_show(args: argparse.Namespace) -> int:
    try:
        data = load_ledger(Path(args.ledger))
    except (OSError, ValueError, json.JSONDecodeError, LiveProgressError) as exc:
        print(str(exc), file=sys.stderr)
        return 2
    issues = data.get("issues", [])
    issue = find_issue(issues if isinstance(issues, list) else [], args.issue_id)
    if issue is None:
        print(f"agent tooling/process issue not found: {args.issue_id}", file=sys.stderr)
        return 1
    if args.json:
        print(json.dumps(issue, indent=2))
    else:
        print_issue_detail(issue)
    return 0


def transition_issue(args: argparse.Namespace, *, status: str, action: str, note_field: str) -> int:
    ledger_path = Path(args.ledger)
    try:
        data = require_expected_ledger_revision(ledger_path, args.expected_revision)
    except (OSError, ValueError, json.JSONDecodeError, LiveProgressError) as exc:
        print(str(exc), file=sys.stderr)
        return 2
    issues = data.get("issues", [])
    issue = find_issue(issues if isinstance(issues, list) else [], args.issue_id)
    if issue is None:
        print(f"agent tooling/process issue not found: {args.issue_id}", file=sys.stderr)
        return 1
    if status in {"resolved", "wont-fix"}:
        packets = data.get("work_packets", [])
        unfinished = [
            str(packet.get("id"))
            for packet in packets if isinstance(packet, dict)
            and packet.get("issue_id") == args.issue_id
            and packet.get("state") != "closed"
        ] if isinstance(packets, list) else []
        if unfinished:
            print(
                "cannot close issue with unfinished work packets: " + ", ".join(sorted(unfinished)),
                file=sys.stderr,
            )
            return 2
    note = require_text(vars(args), note_field)
    updated = dict(issue)
    updated["status"] = status
    updated["updated"] = now_iso()
    if action == "resolved":
        updated["resolution"] = note
    updated.setdefault("history", [])
    if isinstance(updated["history"], list):
        updated["history"].append(history_event(action, note))
    issue.clear()
    issue.update(updated)
    if status in {"resolved", "wont-fix"}:
        completed_packet_ids = {
            str(packet.get("id"))
            for packet in data.get("work_packets", [])
            if isinstance(packet, dict)
            and packet.get("issue_id") == args.issue_id
            and packet.get("state") == "closed"
        }
        data["work_packets"] = [
            packet
            for packet in data.get("work_packets", [])
            if not (
                isinstance(packet, dict)
                and packet.get("issue_id") == args.issue_id
                and packet.get("state") == "closed"
            )
        ]
        data["reservations"] = [
            reservation
            for reservation in data.get("reservations", [])
            if not (
                isinstance(reservation, dict)
                and reservation.get("packet_id") in completed_packet_ids
            )
        ]
        data["issues"] = [
            row
            for row in issues
            if not (isinstance(row, dict) and row.get("id") == args.issue_id)
        ]
    try:
        commit = issue_store(ledger_path).commit(
            data, expected_revision=args.expected_revision, apply=args.apply
        )
    except (LiveProgressError, ConcurrentRevisionUpdate) as exc:
        print(str(exc), file=sys.stderr)
        return 2
    print(json.dumps({"commit": commit.to_dict(), "issue": updated}, indent=2))
    return 0


def command_audit(args: argparse.Namespace) -> int:
    path = Path(args.ledger)
    try:
        data = load_ledger(path)
    except (OSError, ValueError, json.JSONDecodeError, LiveProgressError) as exc:
        print(str(exc), file=sys.stderr)
        return 2
    findings = validate_ledger(data)
    if path.suffix.casefold() != ".json":
        findings.extend(
            ValidationIssue("database", finding)
            for finding in validate_issue_database(path)
        )
    issues = data.get("issues", []) if isinstance(data.get("issues"), list) else []
    counts = {status: 0 for status in sorted(STATUSES)}
    for issue in issues:
        if isinstance(issue, dict) and issue.get("status") in counts:
            counts[issue["status"]] += 1
    if args.json:
        print(
            json.dumps(
                {
                    "ledger": display_path(path, REPO_ROOT),
                    "ledger_revision": data.get("revision"),
                    "issue_count": len(issues),
                    "status_counts": counts,
                    "work_packet_count": len(data.get("work_packets", [])) if isinstance(data.get("work_packets", []), list) else 0,
                    "reservation_count": len(data.get("reservations", [])) if isinstance(data.get("reservations", []), list) else 0,
                    "findings": [finding.__dict__ for finding in findings],
                },
                indent=2,
            )
        )
    else:
        print(f"agent tooling/process issue ledger: {display_path(path, REPO_ROOT)}")
        print(f"issues: {len(issues)} " + " ".join(f"{key}={value}" for key, value in counts.items()))
        if findings:
            print("findings:")
            for finding in findings:
                print(f"- {finding.path}: {finding.message}")
        else:
            print("Agent tooling/process issue ledger OK.")
    return 1 if args.strict and findings else 0


def command_compact(args: argparse.Namespace) -> int:
    ledger_path = Path(args.ledger)
    try:
        _require_work_mutation_mode(args)
        data = require_expected_ledger_revision(ledger_path, args.expected_revision)
        from _recoil.commands.ledger_compact import (
            prepare_issue_compaction,
            require_compaction_apply_allowed,
        )

        candidate, details = prepare_issue_compaction(data)
        findings = validate_ledger(candidate)
        if findings:
            raise ValueError(
                "issue compaction proposal failed audit: "
                + "; ".join(
                    f"{finding.path}: {finding.message}" for finding in findings[:8]
                )
            )
        if args.apply:
            require_compaction_apply_allowed(details)
        commit = issue_store(ledger_path).commit(
            candidate,
            expected_revision=args.expected_revision,
            apply=args.apply,
        )
        print(json.dumps({"commit": commit.to_dict(), **details}, indent=2))
        return 0
    except (
        OSError,
        ValueError,
        json.JSONDecodeError,
        ProgressError,
        LiveProgressError,
        ConcurrentRevisionUpdate,
    ) as exc:
        print(f"recoil issue compact: {exc}", file=sys.stderr)
        return 2


def parse_resource_claim(value: str) -> dict[str, str]:
    try:
        kind, remainder = value.split(":", 1)
        identifier, access = remainder.rsplit(":", 1)
    except ValueError as exc:
        raise ValueError("claims must use KIND:ID:read|write") from exc
    try:
        return normalize_resource_claims(
            [{"kind": kind, "id": identifier, "access": access}]
        )[0]
    except (ProgressError, IndexError) as exc:
        raise ValueError(str(exc)) from exc


def find_work_packet(data: dict[str, Any], packet_id: str) -> dict[str, Any] | None:
    packets = data.get("work_packets", [])
    if not isinstance(packets, list):
        return None
    for packet in packets:
        if isinstance(packet, dict) and packet.get("id") == packet_id:
            return packet
    return None


def packet_repository_read_dependencies(
    packet: Mapping[str, Any],
) -> tuple[str, ...]:
    """Return exact reviewed repository-file reads retained by one packet.

    Workspace-issue creation normalizes and freezes ``resource_claims`` in the
    packet record; reservation copies that same set without accepting a
    closeout-time override.  Only an exact ``path`` read is meaningful to Git
    copy-source authorization.  Other path-shaped concurrency resources remain
    outside this boundary, and a path claim is never expanded to its children.
    """

    raw_claims = packet.get("resource_claims")
    if not isinstance(raw_claims, list) or any(
        not isinstance(row, Mapping) for row in raw_claims
    ):
        raise ValueError("issue work packet has malformed resource claims")
    normalized = normalize_resource_claims(raw_claims)
    if normalized != raw_claims or len(normalized) != len(raw_claims):
        raise ValueError(
            "issue work packet resource claims are not immutable normalized claims"
        )

    exact_paths: list[str] = []
    for claim in normalized:
        if claim["kind"] != "path" or claim["access"] != "read":
            continue
        path = claim["id"]
        # Pattern spellings and existing directory roots are concurrency
        # resources, not exact file-level copy-source authority.  A missing
        # exact spelling remains harmless: the comparator grants no sibling or
        # descendant authority and requires direct endpoint equality.
        if any(token in path for token in ("*", "?", "[", "]", "{", "}")):
            continue
        repository_path = REPO_ROOT.joinpath(*path.split("/"))
        if repository_path.is_dir():
            continue
        exact_paths.append(path)
    return tuple(sorted(exact_paths, key=str.casefold))


def _load_valid_issue_ledger(path: Path) -> dict[str, Any]:
    data = load_ledger(path)
    findings = validate_ledger(data)
    if path.suffix.casefold() != ".json":
        findings.extend(
            ValidationIssue("database", finding)
            for finding in validate_issue_database(path)
        )
    if findings:
        raise ValueError(
            "invalid workspace issue ledger: "
            + "; ".join(f"{row.path}: {row.message}" for row in findings)
        )
    return data


def _active_issue_lease_rows(data: dict[str, Any]) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    reservations = data.get("reservations", [])
    if not isinstance(reservations, list):
        return rows
    for reservation in reservations:
        if not isinstance(reservation, dict) or reservation.get("state") != "active":
            continue
        packet_id = str(reservation.get("packet_id") or "")
        packet = find_work_packet(data, packet_id)
        if packet is None:
            continue
        rows.append(
            {
                "packet_id": packet_id,
                "packet_source": "workspace-issues",
                "handoff_role": packet.get("handoff_role"),
                "owner_id": "",
                "block_id": "",
                "resource_claims": list(reservation.get("resource_claims", [])),
                "resource_claims_complete": True,
                "resource_claim_source": "explicit",
                "reservation": reservation,
            }
        )
    return rows


def workspace_issue_reservation_conflicts(
    issue_ledger_path: Path,
    packet_id: str,
    resource_claims: list[dict[str, str]],
) -> list[dict[str, Any]]:
    """Check one reconstruction packet against all active issue reservations."""
    issue_data = _load_valid_issue_ledger(issue_ledger_path)
    if find_work_packet(issue_data, packet_id) is not None:
        raise ValueError(
            f"work packet id {packet_id} is already used by the workspace issue ledger"
        )
    conflicts: list[dict[str, Any]] = []
    for other in _active_issue_lease_rows(issue_data):
        for conflict in resource_claim_conflicts(
            resource_claims,
            str(other["packet_id"]),
            other["resource_claims"],
        ):
            conflicts.append(
                {
                    **conflict,
                    "selected_packet_id": packet_id,
                    "selected_packet_source": "reconstruction-progress",
                    "other_packet_source": "workspace-issues",
                }
            )
    return conflicts


def _active_lease_rows(
    document: ProgressDocument,
    issue_data: dict[str, Any],
) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for work_id, work in document.collection("work_items").items():
        if not isinstance(work, dict):
            continue
        reservation = work.get("reservation")
        if not isinstance(reservation, dict) or reservation.get("state") != "active":
            continue
        claims, complete, source = work_resource_claims(work)
        rows.append(
            {
                "packet_id": work_id,
                "packet_source": "reconstruction-progress",
                "handoff_role": work.get("handoff_role"),
                "owner_id": work.get("owner_id", ""),
                "block_id": work.get("block_id", ""),
                "resource_claims": claims,
                "resource_claims_complete": complete,
                "resource_claim_source": source,
                "reservation": reservation,
            }
        )
    rows.extend(_active_issue_lease_rows(issue_data))
    rows.sort(key=lambda row: (str(row["packet_source"]), str(row["packet_id"])))
    return rows


def combined_lease_view(
    progress_path: Path,
    issue_ledger_path: Path,
    *,
    selector: str | None = None,
    document: ProgressDocument | None = None,
) -> dict[str, Any]:
    document = document or ProgressStore(progress_path).load()
    issue_data = _load_valid_issue_ledger(issue_ledger_path)
    active_rows = _active_lease_rows(document, issue_data)
    progress_packet_ids = set(document.collection("work_items"))
    issue_packet_ids = {
        str(row.get("id"))
        for row in issue_data.get("work_packets", [])
        if isinstance(row, dict) and isinstance(row.get("id"), str)
    }
    duplicate_packet_ids = sorted(progress_packet_ids & issue_packet_ids, key=str.casefold)
    if duplicate_packet_ids:
        raise ValueError(
            "work packet ids must be globally unique across reconstruction and issue ledgers: "
            + ", ".join(duplicate_packet_ids)
        )
    selected: dict[str, Any] | None = None
    if selector is not None:
        work = document.collection("work_items").get(selector)
        if isinstance(work, dict):
            claims, complete, source = work_resource_claims(work)
            selected = {
                "packet_id": selector,
                "packet_source": "reconstruction-progress",
                "handoff_role": work.get("handoff_role"),
                "owner_id": work.get("owner_id", ""),
                "block_id": work.get("block_id", ""),
                "resource_claims": claims,
                "resource_claims_complete": complete,
                "resource_claim_source": source,
                "reservation": work.get("reservation"),
            }
        else:
            packet = find_work_packet(issue_data, selector)
            if packet is not None:
                selected = {
                    "packet_id": selector,
                    "packet_source": "workspace-issues",
                    "handoff_role": packet.get("handoff_role"),
                    "owner_id": "",
                    "block_id": "",
                    "resource_claims": list(packet.get("resource_claims", [])),
                    "resource_claims_complete": True,
                    "resource_claim_source": "explicit",
                    "reservation": next(
                        (
                            row
                            for row in issue_data.get("reservations", [])
                            if isinstance(row, dict)
                            and row.get("packet_id") == selector
                            and row.get("state") == "active"
                        ),
                        None,
                    ),
                }
        if selected is None:
            raise ValueError(f"unknown work packet {selector}")

        if selected.get("packet_source") == "workspace-issues":
            reservation = selected.get("reservation")
            packet = find_work_packet(issue_data, selector)
            if isinstance(reservation, Mapping) and reservation.get("state") == "active":
                descriptor = reservation.get("git_workspace_baseline")
                exact_self_hosting = (
                    selector == SELF_HOSTING_NATIVE_GIT_PACKET
                    and reservation.get("id") == SELF_HOSTING_NATIVE_GIT_RESERVATION
                )
                if not isinstance(descriptor, Mapping) and not exact_self_hosting:
                    raise ValueError(
                        "active issue packet has no immutable Git workspace baseline; "
                        "handoff is disabled"
                    )
                if not isinstance(packet, Mapping):
                    raise ValueError("active issue packet is unavailable")
                if isinstance(descriptor, Mapping):
                    validate_git_baseline_descriptor(
                        descriptor,
                        packet_id=selector,
                        writable_paths=packet.get("allowed_paths", []),
                    )

    conflicts: list[dict[str, Any]] = []
    if selected is not None:
        comparisons = [row for row in active_rows if row["packet_id"] != selector]
        for other in comparisons:
            for conflict in resource_claim_conflicts(
                selected["resource_claims"],
                str(other["packet_id"]),
                other["resource_claims"],
                second_owner_id=str(other.get("owner_id") or ""),
                second_block_id=str(other.get("block_id") or ""),
            ):
                conflicts.append(
                    {
                        **conflict,
                        "selected_packet_id": selector,
                        "selected_packet_source": selected["packet_source"],
                        "other_packet_source": other["packet_source"],
                    }
                )
    else:
        for index, first in enumerate(active_rows):
            for second in active_rows[index + 1 :]:
                for conflict in resource_claim_conflicts(
                    first["resource_claims"],
                    str(second["packet_id"]),
                    second["resource_claims"],
                    second_owner_id=str(second.get("owner_id") or ""),
                    second_block_id=str(second.get("block_id") or ""),
                ):
                    conflicts.append(
                        {
                            **conflict,
                            "selected_packet_id": first["packet_id"],
                            "selected_packet_source": first["packet_source"],
                            "other_packet_source": second["packet_source"],
                        }
                    )
    public_selected = None if selected is None else {
        **selected,
        "reservation": _public_reservation_projection(selected.get("reservation")),
    }
    public_active_rows = [
        {
            **row,
            "reservation": _public_reservation_projection(row.get("reservation")),
        }
        for row in active_rows
    ]
    payload = {
        "selected_packet": public_selected,
        "active_reservation_count": len(active_rows),
        "active_reservations": public_active_rows,
        "conflicts": conflicts,
        "incomplete_reservations": [
            str(row["packet_id"])
            for row in active_rows if not row.get("resource_claims_complete")
        ],
        "issue_ledger": display_path(issue_ledger_path, REPO_ROOT),
        "issue_ledger_revision": issue_data.get("revision"),
        "expires": None,
    }
    return document.scheduler_output(payload)


def _require_work_mutation_mode(args: argparse.Namespace) -> None:
    if bool(args.dry_run) == bool(args.apply):
        raise ValueError("select exactly one of --dry-run or --apply")


def _public_git_baseline_projection(value: object) -> object:
    """Bound public output without changing the immutable stored descriptor."""
    if not isinstance(value, Mapping):
        return value
    ignored = value.get("ignored_paths")
    projection = {
        key: item for key, item in value.items() if key != "ignored_paths"
    }
    projection.update({
        "public_projection": True,
        "descriptor_complete": False,
        "ignored_paths_deprecated": True,
        "ignored_paths_packet_gated": False,
        "historical_ignored_path_count": (
            len(ignored) if isinstance(ignored, list) else None
        ),
    })
    return projection


def _public_reservation_projection(value: object) -> object:
    if not isinstance(value, Mapping):
        return value
    projection = dict(value)
    if "git_workspace_baseline" in projection:
        projection["git_workspace_baseline"] = _public_git_baseline_projection(
            projection["git_workspace_baseline"]
        )
    return projection


def command_work_set(args: argparse.Namespace) -> int:
    ledger_path = Path(args.ledger)
    try:
        _require_work_mutation_mode(args)
        data = require_expected_ledger_revision(ledger_path, args.expected_revision)
        issues = data.get("issues", [])
        issue = find_issue(issues if isinstance(issues, list) else [], args.issue_id)
        if issue is None:
            raise ValueError(f"unknown workspace issue {args.issue_id}")
        if issue.get("status") not in {"open", "in-progress"}:
            raise ValueError("issue work requires an open or in-progress issue")
        packets = data.setdefault("work_packets", [])
        data.setdefault("reservations", [])
        if not isinstance(packets, list):
            raise ValueError("ledger field `work_packets` must be a list")
        if find_work_packet(data, args.id) is not None:
            raise ValueError(f"duplicate issue work packet id {args.id}")
        requested_claims = [parse_resource_claim(value) for value in args.claim]
        required_claims = [
            {"kind": "lane", "id": f"workspace-issue/{args.issue_id}", "access": "write"},
            {"kind": "issue", "id": args.issue_id, "access": "read"},
            {"kind": "issue-ledger", "id": ".agent/WORKSPACE_ISSUES.sqlite3", "access": "read"},
            {"kind": "tracker", "id": "recoil", "access": "read"},
        ]
        claims = normalize_resource_claims([*required_claims, *requested_claims])
        packet = {
            "id": args.id,
            "issue_id": args.issue_id,
            "state": "ready",
            "handoff_role": args.handoff_role,
            "scope": args.scope.strip(),
            "next_command": args.next_command.strip(),
            "allowed_paths": list_field(args.allowed_path),
            "forbidden_paths": list_field(args.forbidden_path),
            "validation_commands": list_field(args.validation_command),
            "required_return_fields": list_field(args.return_field),
            "resource_claims": claims,
            "reservation_id": None,
            "created": now_iso(),
            "updated": now_iso(),
            "semantic_contract_version": 1,
            "validation_command_contract_version": 1,
            "scope_versions": [],
            "role_contract_version": 1,
        }
        candidate = {**data, "work_packets": [*packets, packet]}
        findings = validate_ledger(candidate)
        if findings:
            raise ValueError("invalid proposed issue packet: " + "; ".join(
                f"{row.path}: {row.message}" for row in findings
            ))
        packets.append(packet)
        issue["status"] = "in-progress"
        issue["updated"] = now_iso()
        issue.setdefault("history", []).append(
            history_event("work-packet-created", f"Created governed packet {args.id}.")
        )
        commit = issue_store(ledger_path).commit(
            data,
            expected_revision=args.expected_revision,
            apply=args.apply,
        )
        print(json.dumps({"commit": commit.to_dict(), "packet": packet}, indent=2))
        return 0
    except (OSError, ValueError, json.JSONDecodeError, GitChangeControlError, ProgressError, LiveProgressError, ConcurrentRevisionUpdate) as exc:
        print(f"recoil issue work set: {exc}", file=sys.stderr)
        return 2


def reserve_issue_work_item(
    *,
    ledger_path: Path,
    progress_path: Path,
    packet_id: str,
    expected_revision: int,
    apply: bool,
    repository_root: Path,
) -> dict[str, object]:
    """Reserve one ready issue item against an orchestrator-selected Git root.

    This is an internal parent/tool seam.  The public issue command always
    supplies the canonical root; the worktree controller supplies only the
    exact linked root it just created and associated.
    """
    with cross_ledger_reservation_critical_section(progress_path, ledger_path):
        data = require_expected_ledger_revision(ledger_path, expected_revision)
        packet = find_work_packet(data, packet_id)
        if packet is None:
            raise ValueError(f"unknown issue work packet {packet_id}")
        if packet.get("state") != "ready" or packet.get("reservation_id") is not None:
            raise ValueError("only an unreserved ready issue packet may be reserved")
        lease_view = combined_lease_view(progress_path, ledger_path, selector=packet_id)
        if lease_view["incomplete_reservations"]:
            raise ValueError(
                "cannot prove non-overlap with incomplete active reservations: "
                + ", ".join(lease_view["incomplete_reservations"])
            )
        if lease_view["conflicts"]:
            raise ValueError(
                "issue packet conflicts with active reservations: "
                + json.dumps(lease_view["conflicts"], sort_keys=True)
            )
        reservations = data.setdefault("reservations", [])
        if not isinstance(reservations, list):
            raise ValueError("ledger field `reservations` must be a list")
        sequence = 1 + sum(
            1 for row in reservations
            if isinstance(row, dict) and row.get("packet_id") == packet_id
        )
        reservation_id = f"{packet_id}:attempt:{sequence}"
        git_baseline = capture_clean_git_baseline(
            repository_root,
            packet_id=packet_id,
            writable_paths=packet.get("allowed_paths", []),
        )
        require_expected_ledger_revision(ledger_path, expected_revision)
        rechecked_lease_view = combined_lease_view(
            progress_path, ledger_path, selector=packet_id
        )
        if rechecked_lease_view["incomplete_reservations"]:
            raise ValueError(
                "cannot prove non-overlap after Git baseline capture: "
                + ", ".join(rechecked_lease_view["incomplete_reservations"])
            )
        if rechecked_lease_view["conflicts"]:
            raise ValueError(
                "issue packet conflicts after Git baseline capture: "
                + json.dumps(rechecked_lease_view["conflicts"], sort_keys=True)
            )
        progress_database_baseline = _capture_protected_progress_database(progress_path)
        reservation = {
            "id": reservation_id,
            "packet_id": packet_id,
            "state": "active",
            "created": now_iso(),
            "released": None,
            "outcome": None,
            "evidence_ids": [],
            "resource_claims": list(packet["resource_claims"]),
            "expires": None,
            "semantic_contract_version": 1,
            "git_workspace_baseline": git_baseline,
            "protected_progress_database_baseline": progress_database_baseline,
        }
        reservations.append(reservation)
        packet["state"] = "active"
        packet["reservation_id"] = reservation["id"]
        packet["updated"] = now_iso()
        commit = issue_store(ledger_path).commit(
            data, expected_revision=expected_revision, apply=apply
        )
    return {"commit": commit.to_dict(), "reservation": reservation}


def command_work_reserve(args: argparse.Namespace) -> int:
    ledger_path = Path(args.ledger)
    try:
        _require_work_mutation_mode(args)
        result = reserve_issue_work_item(
            ledger_path=ledger_path,
            progress_path=Path(args.progress),
            packet_id=args.id,
            expected_revision=args.expected_revision,
            apply=bool(args.apply),
            repository_root=REPO_ROOT,
        )
        print(json.dumps(result, indent=2))
        return 0
    except (OSError, ValueError, json.JSONDecodeError, GitChangeControlError, ProgressError, LiveProgressError, ConcurrentRevisionUpdate) as exc:
        print(f"recoil issue work reserve: {exc}", file=sys.stderr)
        return 2


def command_work_close(args: argparse.Namespace) -> int:
    ledger_path = Path(args.ledger)
    try:
        _require_work_mutation_mode(args)
        data = load_ledger(ledger_path)
        transition_v1 = (
            ledger_path.suffix.casefold() == ".json" and data.get("version") == 1
        )
        if transition_v1:
            if args.expected_revision != 0:
                raise ValueError(
                    "the temporary version-1 close path requires --expected-revision 0"
                )
        else:
            data = require_expected_ledger_revision(ledger_path, args.expected_revision)
        packet = find_work_packet(data, args.id)
        if packet is None:
            raise ValueError(f"unknown issue work packet {args.id}")
        if packet.get("state") != "active" or not packet.get("reservation_id"):
            raise ValueError("only an active issue packet may be closed")
        reservations = data.get("reservations", [])
        reservation = next(
            (
                row for row in reservations
                if isinstance(row, dict)
                and row.get("id") == packet.get("reservation_id")
                and row.get("state") == "active"
            ),
            None,
        ) if isinstance(reservations, list) else None
        if reservation is None:
            raise ValueError("active issue packet has no matching active reservation")
        descriptor = reservation.get("git_workspace_baseline")
        exact_self_hosting = (
            args.id == SELF_HOSTING_NATIVE_GIT_PACKET
            and reservation.get("id") == SELF_HOSTING_NATIVE_GIT_RESERVATION
        )
        if descriptor is None and exact_self_hosting:
            descriptor = {
                "schema": GIT_WORKSPACE_BASELINE_SCHEMA,
                "packet_id": args.id,
                "baseline_commit": SELF_HOSTING_NATIVE_GIT_BASELINE_COMMIT,
                "branch": SELF_HOSTING_NATIVE_GIT_BRANCH,
                "writable_paths": sorted(
                    packet.get("allowed_paths", []), key=str.casefold
                ),
                "git_object_ids_are_opaque": True,
            }
        if not isinstance(descriptor, Mapping):
            raise ValueError(
                "active issue packet lacks its immutable Git workspace baseline"
            )
        with cross_ledger_reservation_critical_section(
            Path(args.progress), ledger_path
        ):
            git_postflight = capture_git_closeout(
                REPO_ROOT,
                descriptor,
                packet_id=args.id,
                writable_paths=packet.get("allowed_paths", []),
            )
        if git_postflight.get("passed") is not True:
            rejected = {
                "applied": False,
                "packet_id": args.id,
                "terminal_outcome": None,
                "git_path_postflight": git_postflight,
            }
            print(json.dumps(rejected, indent=2))
            print(
                "recoil issue work close: Git closeout preflight rejected: "
                + ", ".join(git_postflight.get("unexpected_paths", [])),
                file=sys.stderr,
            )
            return 2
        progress_before = reservation.get("protected_progress_database_baseline")
        if not isinstance(progress_before, Mapping):
            raise ValueError(
                "active issue packet lacks its protected progress database baseline"
            )
        else:
            progress_after = _capture_protected_progress_database(Path(args.progress))
            progress_postflight = {
                "passed": dict(progress_before) == progress_after,
                "before": dict(progress_before),
                "after": progress_after,
            }
            if progress_postflight["passed"] is not True:
                raise ValueError(
                    "protected progress database revision/schema/count state changed during issue packet"
                )
        updated_reservation = {
            **reservation,
            "state": "released",
            "released": now_iso(),
            "outcome": args.outcome,
            "evidence_ids": list_field(args.evidence_id),
            "git_path_postflight": git_postflight,
            "protected_progress_database_postflight": progress_postflight,
        }
        reservation.clear()
        reservation.update(updated_reservation)
        packet["state"] = "closed"
        packet["outcome"] = args.outcome
        packet["reservation_id"] = None
        packet["updated"] = now_iso()
        packets = data.get("work_packets", [])
        data["work_packets"] = [
            row
            for row in packets
            if not (isinstance(row, dict) and row.get("id") == args.id)
        ] if isinstance(packets, list) else packets
        data["reservations"] = [
            row
            for row in reservations
            if not (isinstance(row, dict) and row.get("id") == updated_reservation["id"])
        ] if isinstance(reservations, list) else reservations
        if transition_v1:
            result = {
                "applied": args.apply,
                "path": ledger_path.as_posix(),
                "previous_revision": 0,
                "revision": 0,
                "temporary_v1_transition": True,
            }
            if args.apply:
                before = load_ledger(ledger_path)
                with revision_lock(ledger_path):
                    if load_ledger(ledger_path) != before:
                        raise ValueError(
                            "version-1 issue ledger changed during close preflight"
                        )
                    atomic_replace(ledger_path, canonical_json_bytes(data))
        else:
            commit = issue_store(ledger_path).commit(
                data,
                expected_revision=args.expected_revision,
                apply=args.apply,
            )
            result = commit.to_dict()
        print(json.dumps({
            "applied": bool(result.get("applied")),
            "terminal_outcome": (
                args.outcome if result.get("applied") is True else None
            ),
            "commit": result,
            "reservation": _public_reservation_projection(updated_reservation),
            "git_path_postflight": git_postflight,
            "protected_progress_database_postflight": progress_postflight,
        }, indent=2))
        return 0
    except (OSError, ValueError, json.JSONDecodeError, GitChangeControlError, LiveProgressError, ConcurrentRevisionUpdate) as exc:
        print(f"recoil issue work close: {exc}", file=sys.stderr)
        return 2


def command_work_list(args: argparse.Namespace) -> int:
    try:
        data = load_ledger(Path(args.ledger))
        packets = data.get("work_packets", [])
        if not isinstance(packets, list):
            raise ValueError("ledger field `work_packets` must be a list")
        selected = [
            row for row in packets
            if isinstance(row, dict)
            and (args.state == "all" or row.get("state") == args.state)
            and (args.issue_id is None or row.get("issue_id") == args.issue_id)
        ]
        if args.json:
            print(json.dumps(selected, indent=2))
        elif not selected:
            print("No matching workspace-issue work packets.")
        else:
            for row in selected:
                print(f"{row['id']} {row['state']} {row['handoff_role']} {row['issue_id']}")
        return 0
    except (OSError, ValueError, json.JSONDecodeError, LiveProgressError) as exc:
        print(f"recoil issue work list: {exc}", file=sys.stderr)
        return 2


def command_work_show(args: argparse.Namespace) -> int:
    try:
        data = load_ledger(Path(args.ledger))
        packet = find_work_packet(data, args.id)
        if packet is None:
            print(f"workspace-issue work packet not found: {args.id}", file=sys.stderr)
            return 1
        reservations = [
            row for row in data.get("reservations", [])
            if isinstance(row, dict) and row.get("packet_id") == args.id
        ]
        print(json.dumps({
            "packet": packet,
            "reservations": [
                _public_reservation_projection(row) for row in reservations
            ],
        }, indent=2))
        return 0
    except (OSError, ValueError, json.JSONDecodeError, LiveProgressError) as exc:
        print(f"recoil issue work show: {exc}", file=sys.stderr)
        return 2


def add_common_create_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--ledger", default=str(DEFAULT_LEDGER))
    parser.add_argument("--from-json", help="Read issue fields from a JSON object.")
    parser.add_argument("--severity", choices=sorted(SEVERITIES))
    parser.add_argument("--summary")
    parser.add_argument("--area")
    parser.add_argument("--impact")
    parser.add_argument("--next-action", dest="next_action")
    parser.add_argument("--evidence")
    parser.add_argument("--command", action="append", default=[])
    parser.add_argument("--file", action="append", default=[])
    parser.add_argument("--tag", action="append", default=[])
    add_revision_mutation_mode(parser)


def add_revision_mutation_mode(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--expected-revision", required=True, type=int)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--dry-run", action="store_true")
    mode.add_argument("--apply", action="store_true")


def add_work_mutation_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--ledger", default=str(DEFAULT_LEDGER))
    add_revision_mutation_mode(parser)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Record and inspect Recoil agent tooling/process issue reports.")
    subparsers = parser.add_subparsers(dest="command", required=True)

    report = subparsers.add_parser("report", help="Record an agent tooling/process problem for a future agent to fix.")
    add_common_create_args(report)
    report.add_argument("--kind", choices=sorted(PROBLEM_KINDS))
    report.add_argument("--actual")
    report.add_argument("--expected")
    report.add_argument("--repro")
    report.add_argument("--workaround")
    report.set_defaults(func=command_report)

    request = subparsers.add_parser("request", help="Record an agent tooling/process improvement request.")
    add_common_create_args(request)
    request.add_argument("--requested-change")
    request.add_argument("--benefit")
    request.set_defaults(func=command_request)

    list_parser = subparsers.add_parser("list", help="List agent tooling/process issues.")
    list_parser.add_argument("--ledger", default=str(DEFAULT_LEDGER))
    list_parser.add_argument("--status", default="open", choices=["all", *sorted(STATUSES)])
    list_parser.add_argument("--kind", choices=sorted(KINDS))
    list_parser.add_argument("--limit", type=int, default=80)
    list_parser.add_argument("--json", action="store_true")
    list_parser.set_defaults(func=command_list)

    show = subparsers.add_parser("show", help="Show one agent tooling/process issue.")
    show.add_argument("issue_id")
    show.add_argument("--ledger", default=str(DEFAULT_LEDGER))
    show.add_argument("--json", action="store_true")
    show.set_defaults(func=command_show)

    resolve = subparsers.add_parser("resolve", help="Mark one agent tooling/process issue resolved.")
    resolve.add_argument("issue_id")
    resolve.add_argument("--ledger", default=str(DEFAULT_LEDGER))
    resolve.add_argument("--resolution", required=True)
    add_revision_mutation_mode(resolve)
    resolve.set_defaults(func=lambda args: transition_issue(args, status="resolved", action="resolved", note_field="resolution"))

    wont_fix = subparsers.add_parser(
        "wont-fix",
        help="Close one issue without resolution and remove its terminal rows.",
    )
    wont_fix.add_argument("issue_id")
    wont_fix.add_argument("--ledger", default=str(DEFAULT_LEDGER))
    wont_fix.add_argument("--reason", required=True)
    add_revision_mutation_mode(wont_fix)
    wont_fix.set_defaults(
        func=lambda args: transition_issue(
            args,
            status="wont-fix",
            action="wont-fix",
            note_field="reason",
        )
    )

    reopen = subparsers.add_parser("reopen", help="Reopen one agent tooling/process issue.")
    reopen.add_argument("issue_id")
    reopen.add_argument("--ledger", default=str(DEFAULT_LEDGER))
    reopen.add_argument("--reason", required=True)
    add_revision_mutation_mode(reopen)
    reopen.set_defaults(func=lambda args: transition_issue(args, status="open", action="reopened", note_field="reason"))

    audit = subparsers.add_parser("audit", help="Validate the agent tooling/process issue ledger shape.")
    audit.add_argument("--ledger", default=str(DEFAULT_LEDGER))
    audit.add_argument("--strict", action="store_true")
    audit.add_argument("--json", action="store_true")
    audit.set_defaults(func=command_audit)

    compact = subparsers.add_parser(
        "compact",
        help="Parent-only active-only workspace-issue ledger compaction.",
    )
    compact.add_argument("--ledger", default=str(DEFAULT_LEDGER))
    add_revision_mutation_mode(compact)
    compact.add_argument("--json", action="store_true")
    compact.set_defaults(func=command_compact)

    work = subparsers.add_parser(
        "work",
        help="Create, reserve, inspect, and close explicit workspace-issue packets.",
    )
    work_subparsers = work.add_subparsers(dest="work_command", required=True)

    work_set = work_subparsers.add_parser("set", help="Create one governed issue work packet.")
    work_set.add_argument("issue_id")
    work_set.add_argument("--id", required=True)
    work_set.add_argument("--handoff-role", required=True, choices=sorted(ISSUE_WORK_ROLES))
    work_set.add_argument("--scope", required=True)
    work_set.add_argument("--next-command", required=True)
    work_set.add_argument("--allowed-path", action="append", required=True)
    work_set.add_argument("--forbidden-path", action="append", required=True)
    work_set.add_argument("--validation-command", action="append", required=True)
    work_set.add_argument("--return-field", action="append", required=True)
    work_set.add_argument("--claim", action="append", required=True)
    add_work_mutation_args(work_set)
    work_set.set_defaults(func=command_work_set)

    work_list = work_subparsers.add_parser("list", help="List workspace-issue work packets.")
    work_list.add_argument("--ledger", default=str(DEFAULT_LEDGER))
    work_list.add_argument("--state", default="all", choices=["all", *sorted(ISSUE_WORK_STATES)])
    work_list.add_argument("--issue-id")
    work_list.add_argument("--json", action="store_true")
    work_list.set_defaults(func=command_work_list)

    work_show = work_subparsers.add_parser("show", help="Show one workspace-issue work packet.")
    work_show.add_argument("--ledger", default=str(DEFAULT_LEDGER))
    work_show.add_argument("--id", required=True)
    work_show.set_defaults(func=command_work_show)

    work_reserve = work_subparsers.add_parser("reserve", help="Reserve one ready issue work packet.")
    work_reserve.add_argument("--id", required=True)
    work_reserve.add_argument(
        "--progress",
        type=Path,
        default=DEFAULT_PROGRESS_PATH,
    )
    add_work_mutation_args(work_reserve)
    work_reserve.set_defaults(func=command_work_reserve)

    work_close = work_subparsers.add_parser("close", help="Release and close one active issue packet.")
    work_close.add_argument("--id", required=True)
    work_close.add_argument("--outcome", required=True, choices=sorted(ISSUE_WORK_OUTCOMES))
    work_close.add_argument("--evidence-id", action="append", default=[])
    work_close.add_argument(
        "--progress",
        type=Path,
        default=DEFAULT_PROGRESS_PATH,
    )
    add_work_mutation_args(work_close)
    work_close.set_defaults(func=command_work_close)

    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    parser = build_parser()
    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
