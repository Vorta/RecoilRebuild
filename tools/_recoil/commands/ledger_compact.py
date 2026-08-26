from __future__ import annotations

from copy import deepcopy
from typing import Any, Iterable, Mapping

from _recoil.lib.live_progress import canonical_json_bytes
from _recoil.lib.progress import (
    CALL_CONTRACT_EXPECTED_TRUTH,
    ProgressDocument,
    ProgressError,
)


TERMINAL_PROGRESS_WORK_STATES = frozenset(
    {"closed", "returned", "abandoned", "deferred"}
)
OBSOLETE_PROGRESS_MIGRATION_KEYS = frozenset(
    {
        "artifact_policy",
        "date",
        "legacy_sources",
        "live_validation_v5",
        "policy",
        "reconciliation_snapshot",
        "resolved_boundary_conflicts",
        "schema_v2",
        "schema_v3",
        "schema_v4",
        "section_catalog",
        "target_refresh_policy",
    }
)
RETAINED_PROGRESS_MIGRATION_KEYS = frozenset(
    {"authored_call_contract_v1", "source_traceability_v1"}
)
CALL_CONTRACT_EVIDENCE_KIND = "live-authored-call-contract-validation"


def _projected_canonical_bytes(data: dict[str, Any]) -> int:
    revision = data.get("revision")
    if not isinstance(revision, int) or isinstance(revision, bool):
        raise ProgressError("revision must be a non-negative integer")
    data["revision"] = revision + 1
    try:
        return len(canonical_json_bytes(data))
    finally:
        data["revision"] = revision


def _first_difference(left: Any, right: Any, path: str = "$") -> str:
    if type(left) is not type(right):
        return f"{path}: type {type(left).__name__} != {type(right).__name__}"
    if isinstance(left, Mapping):
        left_keys = list(left)
        right_keys = list(right)
        if left_keys != right_keys:
            return f"{path}: keys differ"
        for key in left_keys:
            difference = _first_difference(left[key], right[key], f"{path}.{key}")
            if difference:
                return difference
        return ""
    if isinstance(left, list):
        if len(left) != len(right):
            return f"{path}: list lengths {len(left)} != {len(right)}"
        for index, (left_item, right_item) in enumerate(zip(left, right)):
            difference = _first_difference(
                left_item, right_item, f"{path}[{index}]"
            )
            if difference:
                return difference
        return ""
    return "" if left == right else f"{path}: values differ"


def _progress_audit_errors(document: ProgressDocument) -> list[str]:
    return [
        f"{finding.code}: {finding.message}"
        for finding in document.audit()
        if finding.severity == "error"
    ]


def _call_contract_state_evidence_ids(data: Mapping[str, Any]) -> set[str]:
    result: set[str] = set()
    symbols = data.get("symbols", {})
    if not isinstance(symbols, Mapping):
        raise ProgressError("progress symbols collection must be an object")
    for symbol in symbols.values():
        if not isinstance(symbol, Mapping):
            continue
        binary_state = symbol.get("binary_state")
        state = (
            binary_state.get("call_contract")
            if isinstance(binary_state, Mapping)
            else None
        )
        if not isinstance(state, Mapping):
            continue
        result.update(
            str(item)
            for item in state.get("evidence_ids", [])
            if isinstance(item, str)
        )
    return result


def _evidence_reference_paths(
    value: Any,
    evidence_ids: set[str],
    *,
    path: tuple[str, ...] = (),
) -> dict[str, list[tuple[str, ...]]]:
    references = {evidence_id: [] for evidence_id in evidence_ids}

    def visit(node: Any, current: tuple[str, ...]) -> None:
        if isinstance(node, Mapping):
            for key, item in node.items():
                if not current and key == "evidence":
                    continue
                visit(item, (*current, str(key)))
        elif isinstance(node, list):
            for index, item in enumerate(node):
                visit(item, (*current, str(index)))
        elif isinstance(node, str) and node in references:
            references[node].append(current)

    visit(value, path)
    return references


def _allowed_generic_symbol_evidence_path(path: tuple[str, ...]) -> bool:
    return (
        len(path) == 4
        and path[0] == "symbols"
        and path[2] == "evidence_ids"
        and path[3].isdigit()
    )


def prepare_progress_compaction(
    source: Mapping[str, Any],
) -> tuple[dict[str, Any], dict[str, Any]]:
    """Build one audited active-only tracker proposal without writing it."""

    before = deepcopy(dict(source))
    before_document = ProgressDocument._from_owned_data(before)
    before_errors = _progress_audit_errors(before_document)
    if before_errors:
        raise ProgressError(
            "progress compaction refuses tracker audit errors: "
            + "; ".join(before_errors[:8])
        )
    before_projection = before_document.next_work("recoil")
    candidate = deepcopy(before)
    candidate_document = ProgressDocument._from_owned_data(candidate)

    legacy_symbols = sorted(
        symbol_id
        for symbol_id, symbol in candidate_document.collection("symbols").items()
        if isinstance(symbol, Mapping) and "accepted_call_contract_facts" in symbol
    )
    if legacy_symbols:
        raise ProgressError(
            "progress compaction refuses legacy call-contract certificate rows; "
            "fresh direct per-body verification is required: "
            + ", ".join(legacy_symbols[:8])
        )
    converted_slices = 0
    removed_symbol_fact_blobs = 0

    state_linked = _call_contract_state_evidence_ids(candidate)
    evidence = candidate.get("evidence")
    if not isinstance(evidence, dict):
        raise ProgressError("progress evidence collection must be an object")
    superseded = {
        evidence_id
        for evidence_id, row in evidence.items()
        if isinstance(row, Mapping)
        and row.get("kind") == CALL_CONTRACT_EVIDENCE_KIND
        and evidence_id not in state_linked
    }
    references = _evidence_reference_paths(candidate, superseded)
    unexpected_references = {
        evidence_id: paths
        for evidence_id, paths in references.items()
        if any(not _allowed_generic_symbol_evidence_path(path) for path in paths)
    }
    if unexpected_references:
        evidence_id = sorted(unexpected_references)[0]
        rendered = [".".join(path) for path in unexpected_references[evidence_id][:4]]
        raise ProgressError(
            f"superseded call-contract evidence {evidence_id} has unexpected references: "
            + ", ".join(rendered)
        )
    removed_generic_symbol_links = 0
    for symbol in candidate_document.collection("symbols").values():
        if not isinstance(symbol, dict) or not isinstance(symbol.get("evidence_ids"), list):
            continue
        prior = list(symbol["evidence_ids"])
        symbol["evidence_ids"] = [item for item in prior if item not in superseded]
        removed_generic_symbol_links += len(prior) - len(symbol["evidence_ids"])
    for evidence_id in superseded:
        evidence.pop(evidence_id, None)

    work_items = candidate.get("work_items")
    if not isinstance(work_items, dict):
        raise ProgressError("progress work_items collection must be an object")
    active_reservation_ids = sorted(
        str(work_id)
        for work_id, work in work_items.items()
        if isinstance(work, Mapping)
        and (
            work.get("state") == "active"
            or (
                isinstance(work.get("reservation"), Mapping)
                and work["reservation"].get("state") == "active"
            )
        )
    )
    terminal_work_ids = sorted(
        str(work_id)
        for work_id, work in work_items.items()
        if isinstance(work, Mapping)
        and work.get("state") in TERMINAL_PROGRESS_WORK_STATES
    )
    for work_id in terminal_work_ids:
        work_items.pop(work_id, None)

    migration = candidate.get("migration")
    if not isinstance(migration, dict):
        raise ProgressError("progress migration must be an object")
    removed_migration_keys = sorted(
        key for key in migration if key in OBSOLETE_PROGRESS_MIGRATION_KEYS
    )
    for key in removed_migration_keys:
        migration.pop(key, None)

    after_document = ProgressDocument._from_owned_data(candidate)
    after_errors = _progress_audit_errors(after_document)
    if after_errors:
        raise ProgressError(
            "progress compaction proposal failed tracker audit: "
            + "; ".join(after_errors[:8])
        )
    after_projection = after_document.next_work("recoil")
    difference = _first_difference(before_projection, after_projection)
    if difference:
        raise ProgressError(
            "progress compaction changed the public scheduler projection: " + difference
        )

    before_bytes = len(canonical_json_bytes(before))
    after_bytes = _projected_canonical_bytes(candidate)
    report = {
        "kind": "progress-active-ledger-compaction",
        "schema_version": candidate.get("schema_version"),
        "previous_revision": before.get("revision"),
        "revision": int(before.get("revision", -1)) + 1,
        "before_canonical_bytes": before_bytes,
        "after_canonical_bytes": after_bytes,
        "removed_canonical_bytes": before_bytes - after_bytes,
        "removed_counts": {
            "legacy_call_contract_fact_blobs": removed_symbol_fact_blobs,
            "superseded_call_contract_evidence": len(superseded),
            "generic_symbol_evidence_links": removed_generic_symbol_links,
            "terminal_work_items": len(terminal_work_ids),
            "obsolete_migration_keys": len(removed_migration_keys),
        },
        "removed_categories": [
            "legacy_per_symbol_call_contract_facts",
            "unreferenced_superseded_live_call_contract_evidence",
            "generic_symbol_links_to_removed_call_contract_evidence",
            "terminal_work_items",
            "exact_obsolete_migration_keys",
        ],
        "removed_migration_keys": removed_migration_keys,
        "converted_call_contract_slices": converted_slices,
        "retained_semantic_categories": [
            "binaries",
            "physical_blocks",
            "semantic_spans",
            "symbols",
            "output_sections",
            "storage_contributions",
            "owners",
            "verification_targets",
            "active_work_items",
            "blockers",
            "current_and_referenced_evidence",
            "tombstones",
            "authored_call_contract_v1",
            "source_traceability_v1",
            "unknown_future_migration_keys",
        ],
        "blocked_reasons": [
            f"active tracker reservation: {work_id}"
            for work_id in active_reservation_ids
        ],
        "apply_allowed": not active_reservation_ids,
        "parity": {
            "passed": True,
            "projection": "exact-progress-next-output-at-the-same-in-memory-revision",
            "normalized_fields": [],
            "first_difference": "",
        },
    }
    return candidate, report


def _issue_semantic_projection(data: Mapping[str, Any]) -> dict[str, Any]:
    issues = []
    for issue in data.get("issues", []):
        if not isinstance(issue, Mapping) or issue.get("status") not in {
            "open",
            "in-progress",
        }:
            continue
        issues.append({key: deepcopy(value) for key, value in issue.items() if key != "history"})
    packets = [
        deepcopy(packet)
        for packet in data.get("work_packets", [])
        if isinstance(packet, Mapping) and packet.get("state") in {"ready", "active"}
    ]
    reservations = [
        deepcopy(reservation)
        for reservation in data.get("reservations", [])
        if isinstance(reservation, Mapping) and reservation.get("state") == "active"
    ]
    return {"issues": issues, "work_packets": packets, "reservations": reservations}


def _seed_issue_high_water(data: dict[str, Any]) -> dict[str, int]:
    sequences = data.setdefault("id_sequences", {})
    if not isinstance(sequences, dict):
        raise ValueError("workspace issue id_sequences must be an object")
    issue_sequences = sequences.setdefault("issue", {})
    if not isinstance(issue_sequences, dict):
        raise ValueError("workspace issue id_sequences.issue must be an object")
    seeded: dict[str, int] = {}
    for issue in data.get("issues", []):
        issue_id = issue.get("id") if isinstance(issue, Mapping) else None
        if not isinstance(issue_id, str):
            continue
        import re

        match = re.fullmatch(r"WSI-(\d{8})-(\d{3})", issue_id)
        if match is None:
            continue
        key = match.group(1)
        ordinal = int(match.group(2))
        current = issue_sequences.get(key, 0)
        if not isinstance(current, int) or isinstance(current, bool) or current < 0:
            raise ValueError(f"workspace issue high-water {key!r} must be non-negative")
        issue_sequences[key] = max(current, ordinal)
        seeded[key] = issue_sequences[key]
    return seeded


def prepare_issue_compaction(
    source: Mapping[str, Any],
) -> tuple[dict[str, Any], dict[str, Any]]:
    """Build one active-only workspace-issue proposal without writing it."""

    before = deepcopy(dict(source))
    before_projection = _issue_semantic_projection(before)
    candidate = deepcopy(before)
    seeded = _seed_issue_high_water(candidate)
    issues = candidate.get("issues")
    packets = candidate.get("work_packets")
    reservations = candidate.get("reservations")
    if not isinstance(issues, list) or not isinstance(packets, list) or not isinstance(
        reservations, list
    ):
        raise ValueError("workspace issue collections must be lists")
    removed_issues = [
        issue
        for issue in issues
        if isinstance(issue, Mapping) and issue.get("status") in {"resolved", "wont-fix"}
    ]
    retained_issues = [
        issue
        for issue in issues
        if not isinstance(issue, Mapping)
        or issue.get("status") not in {"resolved", "wont-fix"}
    ]
    stripped_histories = 0
    for issue in retained_issues:
        if isinstance(issue, dict) and "history" in issue:
            issue.pop("history")
            stripped_histories += 1
    removed_packets = [
        packet
        for packet in packets
        if isinstance(packet, Mapping) and packet.get("state") == "closed"
    ]
    removed_reservations = [
        reservation
        for reservation in reservations
        if isinstance(reservation, Mapping) and reservation.get("state") == "released"
    ]
    candidate["issues"] = retained_issues
    candidate["work_packets"] = [
        packet
        for packet in packets
        if not isinstance(packet, Mapping) or packet.get("state") != "closed"
    ]
    candidate["reservations"] = [
        reservation
        for reservation in reservations
        if not isinstance(reservation, Mapping) or reservation.get("state") != "released"
    ]
    active_reservation_ids = sorted(
        str(reservation.get("id"))
        for reservation in candidate["reservations"]
        if isinstance(reservation, Mapping) and reservation.get("state") == "active"
    )
    after_projection = _issue_semantic_projection(candidate)
    difference = _first_difference(before_projection, after_projection)
    if difference:
        raise ValueError(
            "issue compaction changed the active scheduler/lease projection: " + difference
        )
    before_bytes = len(canonical_json_bytes(before))
    after_bytes = _projected_canonical_bytes(candidate)
    report = {
        "kind": "issue-active-ledger-compaction",
        "version": candidate.get("version"),
        "previous_revision": before.get("revision"),
        "revision": int(before.get("revision", -1)) + 1,
        "before_canonical_bytes": before_bytes,
        "after_canonical_bytes": after_bytes,
        "removed_canonical_bytes": before_bytes - after_bytes,
        "removed_counts": {
            "resolved_or_wont_fix_issues": len(removed_issues),
            "closed_work_packets": len(removed_packets),
            "released_reservations": len(removed_reservations),
            "history_arrays_stripped_from_retained_issues": stripped_histories,
        },
        "removed_categories": [
            "resolved_or_wont_fix_issues",
            "closed_work_packets",
            "released_reservations",
            "history_arrays_stripped_from_retained_issues",
        ],
        "seeded_issue_id_high_water": seeded,
        "retained_semantic_categories": [
            "open_and_in_progress_issues",
            "active_and_ready_work_packets",
            "active_reservations",
            "issue_id_high_water",
        ],
        "blocked_reasons": [
            f"active issue reservation: {reservation_id}"
            for reservation_id in active_reservation_ids
        ],
        "apply_allowed": not active_reservation_ids,
        "parity": {
            "passed": True,
            "projection": "active-issues-packets-reservations-excluding-history",
            "normalized_fields": [],
            "first_difference": "",
        },
    }
    return candidate, report


def require_compaction_apply_allowed(report: Mapping[str, Any]) -> None:
    reasons = report.get("blocked_reasons")
    if report.get("apply_allowed") is False:
        rendered = "; ".join(str(item) for item in reasons or [])
        raise ValueError("compaction apply refused: " + (rendered or "active reservation"))
