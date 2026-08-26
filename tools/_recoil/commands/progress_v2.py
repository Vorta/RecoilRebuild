from __future__ import annotations

from copy import deepcopy
from typing import Any, Iterable, Mapping

from _recoil.lib.progress import (
    AUTHORED_BYTE_DIMENSIONS,
    AUTHORED_ORDER_DIMENSIONS,
    CALL_CONTRACT_DIMENSION,
    EXACT_LINK_DIMENSIONS,
    FULL_ORDER_DIMENSIONS,
    ProgressError,
    state_record,
)


def allocate_evidence_id(data: dict[str, Any]) -> str:
    """Allocate a revision-scoped sequential evidence id.

    The id provides stable ledger identity only.  It is deliberately unrelated
    to candidate or artifact contents and is never a validation input.
    """

    revision = data.get("revision")
    if not isinstance(revision, int) or isinstance(revision, bool) or revision < 0:
        raise ProgressError("progress tracker revision must be a non-negative integer")
    target_revision = revision + 1
    sequences = data.setdefault("id_sequences", {})
    if not isinstance(sequences, dict):
        raise ProgressError("progress id_sequences must be an object")
    sequence = sequences.setdefault("evidence", {})
    if not isinstance(sequence, dict):
        raise ProgressError("progress evidence id sequence must be an object")
    if sequence.get("revision") != target_revision:
        sequence.clear()
        sequence.update({"revision": target_revision, "next_ordinal": 1})
    ordinal = sequence.get("next_ordinal")
    if not isinstance(ordinal, int) or isinstance(ordinal, bool) or ordinal < 1:
        raise ProgressError("progress evidence next_ordinal must be a positive integer")
    sequence["next_ordinal"] = ordinal + 1
    return f"recoil:evidence:r{target_revision}:{ordinal:06d}"


def add_live_evidence(
    data: dict[str, Any],
    *,
    kind: str,
    summary: str,
    scope_ids: Iterable[str],
    provenance: Mapping[str, Any],
) -> str:
    evidence = data.setdefault("evidence", {})
    if not isinstance(evidence, dict):
        raise ProgressError("progress evidence collection must be an object")
    evidence_id = allocate_evidence_id(data)
    if evidence_id in evidence:
        raise ProgressError(f"evidence id collision: {evidence_id}")
    evidence[evidence_id] = {
        "kind": str(kind),
        "summary": str(summary),
        "scope_ids": sorted({str(item) for item in scope_ids}),
        "result": "passed",
        "disposition": "accepted",
        "freshness": "current",
        "gating": True,
        "validation_mode": "live",
        "artifacts": [],
        "provenance": deepcopy(dict(provenance)),
    }
    return evidence_id


def accept_live_order_block(
    data: dict[str, Any],
    *,
    block_id: str,
    phase: str,
    evidence_id: str,
    facts: Mapping[str, Any],
) -> None:
    block = data.get("physical_blocks", {}).get(block_id)
    if not isinstance(block, dict):
        raise ProgressError(f"unknown physical block {block_id}")
    if phase == "authored-function-order":
        group_name = "authored"
        dimensions = AUTHORED_ORDER_DIMENSIONS
    elif phase == "full-function-order":
        group_name = "full"
        dimensions = FULL_ORDER_DIMENSIONS
    else:
        raise ProgressError(f"phase {phase!r} is not an order phase")
    order = block.setdefault("order", {})
    if not isinstance(order, dict):
        raise ProgressError(f"physical block {block_id} order must be an object")
    group = order.setdefault(group_name, {})
    if not isinstance(group, dict):
        raise ProgressError(f"physical block {block_id} {group_name} order must be an object")
    for dimension in dimensions:
        group[dimension] = state_record(
            "passed",
            "accepted",
            "current",
            [evidence_id],
            gating=True,
            validation_mode="live",
        )
    block["accepted_order_facts"] = deepcopy(dict(facts))
    block["evidence_ids"] = sorted(
        set(str(item) for item in block.get("evidence_ids", [])) | {evidence_id}
    )


def accept_live_byte_groups(
    data: dict[str, Any],
    *,
    lane: str,
    groups: Iterable[Iterable[str]],
    evidence_id: str,
    facts: Mapping[str, Any],
) -> list[str]:
    if lane == "object":
        dimensions = ("object_byte",)
    elif lane == "authored":
        dimensions = AUTHORED_BYTE_DIMENSIONS
    elif lane == "linked":
        dimensions = EXACT_LINK_DIMENSIONS
    else:
        raise ProgressError(f"unknown byte lane {lane!r}")
    symbols = data.get("symbols", {})
    if not isinstance(symbols, dict):
        raise ProgressError("progress symbols collection must be an object")
    accepted: list[str] = []
    for scope_ids in groups:
        for symbol_id in scope_ids:
            symbol = symbols.get(symbol_id)
            if not isinstance(symbol, dict):
                raise ProgressError(f"unknown live-byte scope {symbol_id}")
            binary_state = symbol.setdefault("binary_state", {})
            if not isinstance(binary_state, dict):
                raise ProgressError(f"symbol {symbol_id} binary_state must be an object")
            for dimension in dimensions:
                binary_state[dimension] = state_record(
                    "passed",
                    "accepted",
                    "current",
                    [evidence_id],
                    gating=True,
                    validation_mode="live",
                )
            symbol["accepted_byte_facts"] = deepcopy(dict(facts))
            symbol["evidence_ids"] = sorted(
                set(str(item) for item in symbol.get("evidence_ids", [])) | {evidence_id}
            )
            accepted.append(str(symbol_id))
    return accepted


def accept_live_call_contract_symbols(
    data: dict[str, Any],
    *,
    symbol_ids: Iterable[str],
    evidence_id: str,
    facts: Mapping[str, Any],
) -> list[str]:
    """Accept only the authored invocation-contract dimension.

    Order, byte, provider, owner, and tier state are deliberately untouched.
    """
    symbols = data.get("symbols", {})
    if not isinstance(symbols, dict):
        raise ProgressError("progress symbols collection must be an object")
    accepted: list[str] = []
    seen: set[str] = set()
    superseded_evidence_ids: set[str] = set()
    for raw_symbol_id in symbol_ids:
        symbol_id = str(raw_symbol_id)
        if symbol_id in seen:
            raise ProgressError(f"duplicate live call-contract scope {symbol_id}")
        seen.add(symbol_id)
        symbol = symbols.get(symbol_id)
        if not isinstance(symbol, dict):
            raise ProgressError(f"unknown live call-contract scope {symbol_id}")
        binary_state = symbol.setdefault("binary_state", {})
        if not isinstance(binary_state, dict):
            raise ProgressError(f"symbol {symbol_id} binary_state must be an object")
        prior_state = binary_state.get(CALL_CONTRACT_DIMENSION)
        if isinstance(prior_state, Mapping):
            superseded_evidence_ids.update(
                str(item)
                for item in prior_state.get("evidence_ids", [])
                if isinstance(item, str) and item != evidence_id
            )
        binary_state[CALL_CONTRACT_DIMENSION] = state_record(
            "passed",
            "accepted",
            "current",
            [evidence_id],
            gating=True,
            validation_mode="live",
        )
        symbol.pop("accepted_call_contract_facts", None)
        symbol["evidence_ids"] = sorted(
            set(str(item) for item in symbol.get("evidence_ids", [])) | {evidence_id}
        )
        accepted.append(symbol_id)

    referenced_call_contract_evidence = {
        str(item)
        for symbol in symbols.values()
        if isinstance(symbol, Mapping)
        for binary_state in [symbol.get("binary_state")]
        if isinstance(binary_state, Mapping)
        for state in [binary_state.get(CALL_CONTRACT_DIMENSION)]
        if isinstance(state, Mapping)
        for item in state.get("evidence_ids", [])
        if isinstance(item, str)
    }
    evidence = data.get("evidence", {})
    if not isinstance(evidence, dict):
        raise ProgressError("progress evidence collection must be an object")
    removable_candidates = {
        item
        for item in superseded_evidence_ids - referenced_call_contract_evidence
        if isinstance(evidence.get(item), Mapping)
        and evidence[item].get("kind")
        == "live-authored-call-contract-validation"
    }
    unexpected_references: set[str] = set()

    def inspect_references(value: Any, path: tuple[str, ...] = ()) -> None:
        if isinstance(value, Mapping):
            for key, item in value.items():
                if not path and key == "evidence":
                    continue
                inspect_references(item, (*path, str(key)))
        elif isinstance(value, list):
            for index, item in enumerate(value):
                inspect_references(item, (*path, str(index)))
        elif isinstance(value, str) and value in removable_candidates:
            allowed_generic_symbol_link = (
                len(path) == 4
                and path[0] == "symbols"
                and path[2] == "evidence_ids"
                and path[3].isdigit()
            )
            if not allowed_generic_symbol_link:
                unexpected_references.add(value)

    inspect_references(data)
    removable = removable_candidates - unexpected_references
    if removable:
        for symbol in symbols.values():
            if not isinstance(symbol, dict) or not isinstance(
                symbol.get("evidence_ids"), list
            ):
                continue
            symbol["evidence_ids"] = [
                item for item in symbol["evidence_ids"] if item not in removable
            ]
        for item in removable:
            evidence.pop(item, None)
    return accepted

