"""Fail-closed one-hop call-contract repair routing.

The parent CLI owns fresh verifier execution, tracker CAS, Git allocation, and
integration.  This module only validates typed producer output and builds a
nonaccepting route descriptor; candidate observations never become expected
truth or reconstruction acceptance.
"""

from __future__ import annotations

from copy import deepcopy
from dataclasses import dataclass
from typing import Any, Mapping

from _recoil.lib.call_contract_generations import current_generations
from _recoil.lib.progress import (
    CONTINUATION_PRODUCER_PACKET_TYPE,
    REPAIR_CONTINUATION_PACKET_TYPE,
    REPAIR_ROUTE_DESCRIPTOR_SCHEMA,
    ProgressError,
    address_value,
    normalize_address,
)


CONTINUATION_MIGRATION_KEY = "authored_call_contract_repair_continuation_v2"
CONTINUATION_PACKET_TYPE = REPAIR_CONTINUATION_PACKET_TYPE
CONTINUATION_PRODUCER_TYPE = CONTINUATION_PRODUCER_PACKET_TYPE
RETURN_PROVENANCE_FIELD = "repair_continuation_provenance"
CONTINUATION_STATE_SCHEMA = "recoil-call-contract-continuation-state-v2"
PRODUCER_RESULT_SCHEMA = "call-contract-continuation-producer-result-v1"
LINKED_TOOL_ISSUE = "WSI-20260809-007"


@dataclass(frozen=True)
class ContinuationPreparation:
    checkpoint: Mapping[str, Any]
    child_descriptor: Mapping[str, Any] | None
    output: Mapping[str, Any]


def _data(document: Any) -> Mapping[str, Any]:
    value = getattr(document, "data", {})
    return value if isinstance(value, Mapping) else {}


def continuation_state(document: Any | None = None) -> dict[str, Any]:
    migration = _data(document).get("migration", {}) if document is not None else {}
    checkpoint = migration.get(CONTINUATION_MIGRATION_KEY) if isinstance(migration, Mapping) else None
    if checkpoint is None:
        return {"state": "none", "active": False, "checkpoint": None}
    if not validate_continuation_checkpoint(checkpoint):
        raise ProgressError("repair continuation checkpoint is malformed")
    state = str(checkpoint["state"])
    return {
        "state": state,
        "active": state not in {"route-blocked", "archived"},
        "checkpoint": deepcopy(dict(checkpoint)),
    }


def continuation_snapshots_equal(first: Any, second: Any) -> bool:
    return isinstance(first, Mapping) and isinstance(second, Mapping) and dict(first) == dict(second)


def validate_continuation_checkpoint(value: Any) -> bool:
    return bool(
        isinstance(value, Mapping)
        and value.get("schema") == CONTINUATION_STATE_SCHEMA
        and value.get("state") in {
            "producer-required", "producer-active", "route-blocked",
            "descriptor-ready", "child-active", "child-returned",
            "awaiting-parent-integration", "awaiting-parent-acceptance", "archived",
        }
        and value.get("checkpoint_id")
        and value.get("nonaccepting") is True
    )


def returned_tool_blocked_provenance(
    _document: Any,
    work_id: str,
    work: Mapping[str, Any],
    linked_issue_id: str,
) -> dict[str, Any]:
    if linked_issue_id != LINKED_TOOL_ISSUE:
        raise ProgressError(f"repair continuation is governed only by {LINKED_TOOL_ISSUE}")
    target_id = str(work.get("target_id", ""))
    if work.get("phase") != "authored-call-contract" or not target_id:
        raise ProgressError("tool-blocked predecessor is not one exact call-contract target")
    caller_edit_paths = sorted(
        {
            str(path)
            for field in ("allowed_paths", "source_edit_paths")
            for path in work.get(field, [])
            if isinstance(path, str)
        }, key=str.casefold,
    )
    return {
        "schema_version": 2,
        "kind": "authored-call-contract-tool-blocked-return-provenance",
        "predecessor_work_item_id": work_id,
        "linked_issue_id": linked_issue_id,
        "target_id": target_id,
        "cursor": str(work.get("cursor", "")),
        "slice_ids": deepcopy(list(work.get("original_slice_ids", []))),
        "caller_edit_paths": caller_edit_paths,
        "predecessor_snapshot": {
            key: deepcopy(work.get(key))
            for key in (
                "target_id", "cursor", "block_id", "covered_block_ids", "scope_ids",
                "target_ids", "original_slice_ids", "allowed_paths", "source_edit_paths",
                "definition_source_paths", "dependency_paths",
            )
        },
        "hop": 0,
        "max_hops": 1,
        "noncurrent": True,
        "nonaccepting": True,
    }


def capture_continuation_input_snapshot(
    document: Any, provenance: Mapping[str, Any]
) -> dict[str, Any]:
    return {
        "tracker_revision": int(getattr(document, "revision", -1)),
        "generations": current_generations(),
        "predecessor_work_item_id": str(provenance.get("predecessor_work_item_id", "")),
        "target_id": str(provenance.get("target_id", "")),
        "predecessor_snapshot": deepcopy(provenance.get("predecessor_snapshot", {})),
    }


def _qualifying_route(
    divergences: list[Mapping[str, Any]],
) -> tuple[Mapping[str, Any] | None, str]:
    """Select the earliest verifier-proven unique caller/declaration/definition route."""
    routes: list[tuple[int, int, Mapping[str, Any]]] = []
    for row in divergences:
        routing = row.get("repair_routing")
        if not isinstance(routing, Mapping):
            continue
        declaration = routing.get("controlling_declaration_path")
        definition = routing.get("controlling_definition_path")
        caller = routing.get("caller_edit_path")
        if not all(isinstance(value, str) and value for value in (caller, declaration, definition)):
            continue
        if not (
            routing.get("caller_symbol_id") == row.get("symbol_id")
            and isinstance(routing.get("caller_physical_block_id"), str)
            and bool(routing.get("caller_physical_block_id"))
            and isinstance(routing.get("caller_owner_id"), str)
            and bool(routing.get("caller_owner_id"))
            and isinstance(routing.get("caller_semantic_span_ids"), list)
            and all(
                isinstance(value, str) and value
                for value in routing.get("caller_semantic_span_ids", [])
            )
        ):
            continue
        if routing.get("unique_controlling_pair") is not True or routing.get("authored_route") is not True:
            continue
        if routing.get("provider_boundary") is True or routing.get("out_of_policy") is True:
            continue
        try:
            routes.append((address_value(str(row.get("address", ""))), int(row.get("ordinal", 0)), row))
        except (TypeError, ValueError):
            continue
    if not routes:
        return None, "no unique verifier-proven authored caller/declaration/definition route"
    routes.sort(key=lambda item: (item[0], item[1]))
    return routes[0][2], "earliest qualifying verifier-derived caller and singleton controlling pair"


def prepare_repair_continuation(
    document: Any,
    predecessor_work_item_id: str,
    predecessor: Mapping[str, Any],
    issue_ledger: Mapping[str, Any],
    _build_root: Any,
    *,
    producer_work_item_id: str,
    producer_result: Mapping[str, Any],
) -> ContinuationPreparation:
    provenance = predecessor.get(RETURN_PROVENANCE_FIELD)
    if not isinstance(provenance, Mapping) or provenance.get("linked_issue_id") != LINKED_TOOL_ISSUE:
        raise ProgressError("terminal predecessor lacks the governed continuation provenance")
    issues = [
        row for row in issue_ledger.get("issues", [])
        if isinstance(row, Mapping) and row.get("id") == LINKED_TOOL_ISSUE
        and row.get("status") in {"open", "in-progress"}
    ]
    if len(issues) != 1:
        raise ProgressError("linked continuation issue is not uniquely active")
    producer = _data(document).get("work_items", {}).get(producer_work_item_id)
    if not isinstance(producer, Mapping) or producer.get("packet_type") != CONTINUATION_PRODUCER_TYPE:
        raise ProgressError("continuation route requires the exact retained producer packet")
    if producer.get("state") not in {"returned", "closed"}:
        raise ProgressError("continuation producer has not returned a terminal diagnostic")
    if producer_result.get("schema") != PRODUCER_RESULT_SCHEMA or producer_result.get("packet_id") != producer_work_item_id:
        raise ProgressError("continuation producer result identity/schema changed")
    if not (
        producer_result.get("all_authored_bodies") is True
        and producer_result.get("all_caller_divergences_collected") is True
        and producer_result.get("candidate_expected_truth") is False
    ):
        raise ProgressError("continuation producer diagnostic is incomplete or accepting")
    divergences = producer_result.get("caller_divergences")
    if not isinstance(divergences, list) or any(not isinstance(row, Mapping) for row in divergences):
        raise ProgressError("continuation producer caller divergences are malformed")
    route_row, justification = _qualifying_route(list(divergences))
    checkpoint_id = f"recoil:call-contract-repair-continuation:r{int(getattr(document, 'revision', 0)) + 1}:0001"
    base = {
        "schema": CONTINUATION_STATE_SCHEMA,
        "checkpoint_id": checkpoint_id,
        "predecessor_work_item_id": predecessor_work_item_id,
        "producer_work_item_id": producer_work_item_id,
        "linked_issue_id": LINKED_TOOL_ISSUE,
        "input_snapshots": {"after": capture_continuation_input_snapshot(document, provenance)},
        "hop": 1, "max_hops": 1,
        "noncurrent": True, "nonaccepting": True, "acceptance_eligible": False,
    }
    if route_row is None:
        checkpoint = {**base, "state": "route-blocked", "route_blocker": justification}
        return ContinuationPreparation(checkpoint, None, {"outcome": "route-blocked", "reason": justification})
    routing = dict(route_row["repair_routing"])
    caller_path = str(routing["caller_edit_path"])
    declaration = str(routing["controlling_declaration_path"])
    definition = str(routing["controlling_definition_path"])
    predecessor_writes = set(provenance.get("caller_edit_paths", []))
    if caller_path not in predecessor_writes:
        raise ProgressError("verifier-derived caller path is outside predecessor caller edit closure")
    final_writes = sorted({caller_path, declaration, definition}, key=str.casefold)
    if len(final_writes) != 3:
        raise ProgressError("repair route does not provide one caller plus singleton declaration/definition")
    dependency_paths = sorted(
        {str(path) for path in predecessor.get("dependency_paths", []) if isinstance(path, str)}
        - set(final_writes), key=str.casefold,
    )
    descriptor_id = checkpoint_id.replace("continuation", "route")
    descriptor = {
        "schema": REPAIR_ROUTE_DESCRIPTOR_SCHEMA,
        "descriptor_id": descriptor_id,
        "predecessor_work_item_id": predecessor_work_item_id,
        "producer_work_item_id": producer_work_item_id,
        "linked_issue_id": LINKED_TOOL_ISSUE,
        "target_id": str(provenance.get("target_id", "")),
        "slice_ids": deepcopy(list(provenance.get("slice_ids", []))),
        "caller_symbol_id": str(route_row.get("symbol_id", "")),
        "caller_address": normalize_address(str(route_row.get("address"))),
        "caller_owner_id": str(routing.get("caller_owner_id", "")),
        "caller_physical_block_id": str(
            routing.get("caller_physical_block_id", "")
        ),
        "caller_semantic_span_ids": deepcopy(
            list(routing.get("caller_semantic_span_ids", []))
        ),
        "earliest_divergence_ordinal": int(route_row.get("ordinal", 0)),
        "earliest_divergence_kind": str(route_row.get("kind", "")),
        "expected_call": deepcopy(route_row.get("expected")),
        "candidate_call_routing_only": deepcopy(route_row.get("candidate")),
        "caller_edit_paths": [caller_path],
        "controlling_declaration_path": declaration,
        "controlling_definition_path": definition,
        "write_paths": final_writes,
        "dependency_paths": dependency_paths,
        "predecessor_snapshot": deepcopy(
            provenance.get("predecessor_snapshot", {})
        ),
        "producer_snapshot": {
            "packet_id": producer_work_item_id,
            "packet_type": producer.get("packet_type"),
            "target_id": producer.get("target_id"),
            "reservation_id": (
                producer.get("reservation", {}).get("id")
                if isinstance(producer.get("reservation"), Mapping)
                else None
            ),
            "result_schema": producer_result.get("schema"),
            "nonaccepting": True,
        },
        "linked_issue_snapshot": deepcopy(issues[0]),
        "revision_snapshots": {
            "tracker_revision": int(getattr(document, "revision", -1)),
            "issue_revision": issue_ledger.get("revision"),
        },
        "binary_ninja_snapshot": deepcopy(
            producer_result.get("binary_ninja_session")
        ),
        "dependency_snapshot": {
            "before": deepcopy(producer_result.get("dependency_states_before")),
            "after": deepcopy(producer_result.get("dependency_states_after")),
            "paths": dependency_paths,
        },
        "promotion_justification": justification,
        "routing_evidence": deepcopy(routing),
        "hop": 1, "max_hops": 1,
        "fresh_parent_acceptance_required": True,
        "candidate_expected_truth": False, "nonaccepting": True,
        "generations": current_generations(),
    }
    child = {
        "binary": "recoil", "handoff_role": "recoil_source_worker",
        "packet_type": CONTINUATION_PACKET_TYPE, "state": "ready",
        "phase": "authored-call-contract", "lane": "primary",
        "target_id": descriptor["target_id"], "cursor": str(predecessor.get("cursor", "")),
        "block_id": str(predecessor.get("block_id", "")),
        "covered_block_ids": deepcopy(list(predecessor.get("covered_block_ids", []))),
        "scope_ids": deepcopy(list(predecessor.get("scope_ids", []))),
        "target_ids": deepcopy(list(predecessor.get("target_ids", []))),
        "original_slice_ids": deepcopy(list(predecessor.get("original_slice_ids", []))),
        "allowed_paths": final_writes, "source_edit_paths": final_writes,
        "definition_source_paths": [definition], "dependency_paths": dependency_paths,
        "route_descriptor": descriptor,
        "continuation_provenance": {
            "schema_version": 2, "command": "progress work claim-current",
            "checkpoint_id": checkpoint_id, "descriptor_id": descriptor_id,
            "predecessor_work_item_id": predecessor_work_item_id,
            "producer_work_item_id": producer_work_item_id, "hop": 1, "max_hops": 1,
        },
        "nonaccepting": True, "acceptance_eligible": False,
        "worker_acceptance_allowed": False, "candidate_expected_truth": False,
        "full_convergence_required": True, "validation_commands": [],
        "required_return_fields": [
            "packet_id", "descriptor_id", "outcome", "changed_paths",
            "validation_command", "validation_passed", "first_divergence", "scope_contradiction",
        ],
        "resource_claims": [
            *({"kind": "path", "id": path, "access": "write"} for path in final_writes),
            *({"kind": "path", "id": path, "access": "read"} for path in dependency_paths),
            {"kind": "tracker", "id": "recoil", "access": "read"},
            {"kind": "binary-ninja-db", "id": "Recoil.bndb", "access": "read"},
            {"kind": "verification-target", "id": descriptor["target_id"], "access": "read"},
        ],
    }
    checkpoint = {
        **base,
        "state": "descriptor-ready",
        "route_descriptor": descriptor,
        "child_descriptor": deepcopy(child),
    }
    return ContinuationPreparation(checkpoint, child, {"outcome": "descriptor-ready", "route_descriptor": descriptor})


def activate_continuation_child(checkpoint: Mapping[str, Any], *, child_work_item_id: str) -> dict[str, Any]:
    if not validate_continuation_checkpoint(checkpoint) or checkpoint.get("state") != "descriptor-ready":
        raise ProgressError("only descriptor-ready continuation may activate a child")
    return {**deepcopy(dict(checkpoint)), "state": "child-active", "child_work_item_id": child_work_item_id}


def finalize_continuation_child(
    checkpoint: Mapping[str, Any], *, child_work_item_id: str, child_work: Mapping[str, Any]
) -> dict[str, Any]:
    if checkpoint.get("state") != "child-active" or checkpoint.get("child_work_item_id") != child_work_item_id:
        raise ProgressError("continuation child does not bind the active checkpoint")
    return {**deepcopy(dict(checkpoint)), "state": "awaiting-parent-integration", "child_terminal_state": str(child_work.get("state", "")), "worker_result_nonaccepting": True}


def archive_continuation_checkpoint(checkpoint: Mapping[str, Any]) -> dict[str, Any]:
    if not validate_continuation_checkpoint(checkpoint):
        raise ProgressError("cannot archive a malformed continuation checkpoint")
    return {**deepcopy(dict(checkpoint)), "state": "archived", "active": False}


__all__ = [
    "CONTINUATION_MIGRATION_KEY", "CONTINUATION_PACKET_TYPE", "CONTINUATION_PRODUCER_TYPE",
    "LINKED_TOOL_ISSUE", "PRODUCER_RESULT_SCHEMA", "RETURN_PROVENANCE_FIELD",
    "activate_continuation_child", "archive_continuation_checkpoint",
    "capture_continuation_input_snapshot", "continuation_snapshots_equal",
    "continuation_state", "finalize_continuation_child", "prepare_repair_continuation",
    "returned_tool_blocked_provenance", "validate_continuation_checkpoint",
]
