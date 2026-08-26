#!/usr/bin/env python3
"""Register one exact authored data-symbol storage contribution without acceptance."""

from __future__ import annotations

from copy import deepcopy
from dataclasses import dataclass
import json
from pathlib import Path
import re
from typing import Any, Mapping

from _recoil.commands.data_extent_progress import (
    DataExtentProgressError,
    _nonnegative_integer,
    _require_exact_keys,
    _require_mapping,
    _section_extent,
)
from _recoil.commands.source_trace_progress import (
    SourceTraceProgressError,
    normalize_artifact_address,
    tracker_store,
)
from _recoil.lib.live_progress import (
    TRACKER_SCHEMA_VERSION,
    ConcurrentRevisionUpdate,
)
from _recoil.lib.progress import ProgressError, validate_owner_invariants
from _recoil.lib.source_owners import normalize_owner_id


REGISTER_OPERATION = "register-authored-data-storage"
PAYLOAD_SCHEMA = "recoil-authored-data-storage-register-v1"
STORAGE_DIMENSIONS = (
    "extent",
    "object",
    "relocation",
    "order",
    "link",
    "raw",
    "zero-fill",
)
APPLICABILITY = {dimension: True for dimension in STORAGE_DIMENSIONS}
PHYSICAL_DATA_ID_RE = re.compile(
    r"^(?P<binary>recoil|messages):data:(?P<address>0x[0-9a-f]+)$"
)
STORAGE_ID_RE = re.compile(
    r"^(?P<binary>recoil|messages):storage:va:(?P<address>0x[0-9a-f]+)$"
)


class StorageContributionProgressError(ValueError):
    """A fail-closed authored data-symbol storage registration error."""


@dataclass(frozen=True)
class StorageContributionPlan:
    expected_revision: int
    proposed_revision: int
    symbol_id: str
    storage_contribution_id: str
    owner_id: str
    storage_row: Mapping[str, Any]
    symbol_storage_before: tuple[str, ...]
    symbol_storage_after: tuple[str, ...]
    output_section_start: str
    output_section_end_exclusive: str
    proposed: Mapping[str, Any]

    def to_dict(self) -> dict[str, Any]:
        return {
            "operation": REGISTER_OPERATION,
            "payload_schema": PAYLOAD_SCHEMA,
            "expected_revision": self.expected_revision,
            "proposed_revision": self.proposed_revision,
            "symbol_id": self.symbol_id,
            "storage_contribution_id": self.storage_contribution_id,
            "owner_id": self.owner_id,
            "storage_contribution": deepcopy(dict(self.storage_row)),
            "symbol_storage_contribution_ids": {
                "before": list(self.symbol_storage_before),
                "after": list(self.symbol_storage_after),
            },
            "output_section": {
                "id": self.storage_row["output_section_id"],
                "start": self.output_section_start,
                "end_exclusive": self.output_section_end_exclusive,
            },
            "invariants": {
                "existing_known_extent_authored_data_symbol": True,
                "unique_existing_primary_data_owner": True,
                "exact_binary_section_extent_equality": True,
                "output_section_containment": True,
                "canonical_ids": True,
                "non_overlapping": True,
                "current_absence": True,
            },
        }


def _normalize_expected_symbol(
    value: Any,
    *,
    symbol_id: str,
    binary: str,
    address: str,
) -> dict[str, Any]:
    expected = _require_mapping(
        value,
        label="authored storage expected_symbol",
    )
    _require_exact_keys(
        expected,
        keys=(
            "binary",
            "kind",
            "disposition",
            "address",
            "extent_state",
            "size",
            "end_exclusive",
            "output_section_id",
            "storage_contribution_ids",
        ),
        label="authored storage expected_symbol",
    )
    if expected["binary"] != binary:
        raise StorageContributionProgressError(
            "authored storage expected_symbol.binary must match symbol_id"
        )
    if expected["kind"] != "data":
        raise StorageContributionProgressError(
            "authored storage expected_symbol.kind must be 'data'"
        )
    if expected["disposition"] != "authored":
        raise StorageContributionProgressError(
            "authored storage requires an existing authored data symbol"
        )
    expected_address = normalize_artifact_address(expected["address"])
    if expected_address != address:
        raise StorageContributionProgressError(
            "authored storage expected_symbol.address must match symbol_id"
        )
    if expected["extent_state"] != "known":
        raise StorageContributionProgressError(
            "authored storage expected_symbol.extent_state must be 'known'"
        )
    size = _nonnegative_integer(
        expected["size"],
        label="authored storage expected_symbol.size",
    )
    if size == 0:
        raise StorageContributionProgressError(
            "authored storage expected_symbol.size must be positive"
        )
    end_exclusive = normalize_artifact_address(expected["end_exclusive"])
    if int(address, 16) + size != int(end_exclusive, 16):
        raise StorageContributionProgressError(
            "authored storage expected_symbol.end_exclusive must equal "
            "address + size"
        )
    output_section_id = expected["output_section_id"]
    if (
        not isinstance(output_section_id, str)
        or not output_section_id.startswith(f"{binary}:section:")
    ):
        raise StorageContributionProgressError(
            "authored storage expected_symbol.output_section_id must be a "
            "canonical section id for the symbol binary"
        )
    if expected["storage_contribution_ids"] != []:
        raise StorageContributionProgressError(
            "authored storage expected_symbol.storage_contribution_ids must "
            "be the exact empty current array"
        )
    return {
        "binary": binary,
        "kind": "data",
        "disposition": "authored",
        "address": address,
        "extent_state": "known",
        "size": size,
        "end_exclusive": end_exclusive,
        "output_section_id": output_section_id,
        "storage_contribution_ids": [],
    }


def _normalize_expected_owner_relationship(
    value: Any,
    *,
    symbol_id: str,
    address: str,
) -> dict[str, Any]:
    relationship = _require_mapping(
        value,
        label="authored storage expected_owner_relationship",
    )
    _require_exact_keys(
        relationship,
        keys=("kind", "symbol_id", "address", "name"),
        label="authored storage expected_owner_relationship",
    )
    if relationship["kind"] != "primary-data":
        raise StorageContributionProgressError(
            "authored storage owner relationship must be primary-data"
        )
    if relationship["symbol_id"] != symbol_id:
        raise StorageContributionProgressError(
            "authored storage owner relationship symbol_id must match"
        )
    if normalize_artifact_address(relationship["address"]) != address:
        raise StorageContributionProgressError(
            "authored storage owner relationship address must match the symbol"
        )
    name = relationship["name"]
    if not isinstance(name, str) or not name or name.strip() != name:
        raise StorageContributionProgressError(
            "authored storage owner relationship name must be non-empty and trimmed"
        )
    return {
        "kind": "primary-data",
        "symbol_id": symbol_id,
        "address": address,
        "name": name,
    }


def _normalize_register_payload(value: Any) -> dict[str, Any]:
    payload = _require_mapping(value, label="authored storage payload")
    _require_exact_keys(
        payload,
        keys=(
            "schema",
            "operation",
            "reviewed",
            "parent_reviewed",
            "symbol_id",
            "storage_contribution_id",
            "owner_id",
            "expected_symbol",
            "expected_owner_relationship",
        ),
        label="authored storage payload",
    )
    if payload["schema"] != PAYLOAD_SCHEMA:
        raise StorageContributionProgressError(
            f"authored storage payload.schema must be {PAYLOAD_SCHEMA!r}"
        )
    if payload["operation"] != REGISTER_OPERATION:
        raise StorageContributionProgressError(
            f"authored storage payload.operation must be {REGISTER_OPERATION!r}"
        )
    if payload["reviewed"] is not True or payload["parent_reviewed"] is not True:
        raise StorageContributionProgressError(
            "authored storage registration requires reviewed=true and "
            "parent_reviewed=true"
        )
    symbol_id = payload["symbol_id"]
    symbol_match = (
        PHYSICAL_DATA_ID_RE.fullmatch(symbol_id)
        if isinstance(symbol_id, str)
        else None
    )
    if symbol_match is None:
        raise StorageContributionProgressError(
            "authored storage symbol_id must be one canonical physical "
            "<binary>:data:<lowercase-address> id"
        )
    binary = symbol_match.group("binary")
    address = normalize_artifact_address(symbol_match.group("address"))
    if symbol_id != f"{binary}:data:{address}":
        raise StorageContributionProgressError(
            "authored storage symbol_id must use the canonical normalized address"
        )
    storage_id = payload["storage_contribution_id"]
    storage_match = (
        STORAGE_ID_RE.fullmatch(storage_id)
        if isinstance(storage_id, str)
        else None
    )
    if (
        storage_match is None
        or storage_match.group("binary") != binary
        or normalize_artifact_address(storage_match.group("address")) != address
        or storage_id != f"{binary}:storage:va:{address}"
    ):
        raise StorageContributionProgressError(
            "authored storage storage_contribution_id must be the canonical "
            "<binary>:storage:va:<symbol-address> id"
        )
    owner_id = payload["owner_id"]
    if not isinstance(owner_id, str):
        raise StorageContributionProgressError(
            "authored storage owner_id must be one canonical owner id for "
            "the symbol binary"
        )
    try:
        normalized_owner_id = normalize_owner_id(owner_id)
    except ValueError as exc:
        raise StorageContributionProgressError(str(exc)) from exc
    if (
        normalized_owner_id != owner_id
        or not owner_id.startswith(f"{binary}:owner:")
        or owner_id == f"{binary}:owner:"
    ):
        raise StorageContributionProgressError(
            "authored storage owner_id must be one canonical owner id for "
            "the symbol binary"
        )
    return {
        "schema": PAYLOAD_SCHEMA,
        "operation": REGISTER_OPERATION,
        "reviewed": True,
        "parent_reviewed": True,
        "symbol_id": symbol_id,
        "storage_contribution_id": storage_id,
        "owner_id": owner_id,
        "expected_symbol": _normalize_expected_symbol(
            payload["expected_symbol"],
            symbol_id=symbol_id,
            binary=binary,
            address=address,
        ),
        "expected_owner_relationship": (
            _normalize_expected_owner_relationship(
                payload["expected_owner_relationship"],
                symbol_id=symbol_id,
                address=address,
            )
        ),
    }


def normalize_register_payload(value: Any) -> dict[str, Any]:
    try:
        return _normalize_register_payload(value)
    except (DataExtentProgressError, SourceTraceProgressError) as exc:
        raise StorageContributionProgressError(str(exc)) from exc


def _pending_claim() -> dict[str, Any]:
    return {
        "disposition": "claim",
        "evidence_ids": [],
        "freshness": "current-unhashed",
        "result": "pending",
    }


def _known_extent(
    row: Mapping[str, Any],
    *,
    address_key: str = "address",
) -> tuple[int, int] | None:
    raw_address = row.get(address_key, row.get("start"))
    if not isinstance(raw_address, str):
        return None
    try:
        start = int(normalize_artifact_address(raw_address), 16)
    except (ProgressError, ValueError):
        return None
    if row.get("extent_state") != "known":
        return None
    raw_end = row.get("end_exclusive")
    raw_size = row.get("size")
    try:
        size = _nonnegative_integer(
            raw_size,
            label="existing known extent size",
        )
        end = int(normalize_artifact_address(raw_end), 16)
    except (DataExtentProgressError, ProgressError, ValueError):
        raise StorageContributionProgressError(
            "existing known extent must carry canonical size and end_exclusive"
        )
    if size <= 0 or end != start + size:
        raise StorageContributionProgressError(
            "existing known extent end_exclusive must equal address + "
            "positive size"
        )
    return start, end


def _reject_physical_overlap(
    tracker: Mapping[str, Any],
    *,
    binary: str,
    symbol_id: str,
    storage_contribution_id: str,
    start: int,
    end: int,
) -> None:
    storage_rows = _require_mapping(
        tracker.get("storage_contributions"),
        label="tracker.storage_contributions",
    )
    for existing_id, raw_row in storage_rows.items():
        if not isinstance(raw_row, Mapping):
            raise StorageContributionProgressError(
                f"existing storage contribution {existing_id!r} must be an object"
            )
        if existing_id == storage_contribution_id:
            raise StorageContributionProgressError(
                f"authored storage contribution {storage_contribution_id!r} "
                "already exists"
            )
        if raw_row.get("binary") != binary:
            continue
        reference = raw_row.get("reference")
        if not isinstance(reference, Mapping):
            raise StorageContributionProgressError(
                f"existing storage contribution {existing_id!r} has no "
                "reference object"
            )
        raw_address = reference.get("address")
        if not isinstance(raw_address, str):
            raise StorageContributionProgressError(
                f"existing storage contribution {existing_id!r} has no "
                "reference address"
            )
        existing_start = int(normalize_artifact_address(raw_address), 16)
        extent = _known_extent(reference)
        if extent is not None:
            existing_start, existing_end = extent
            if start < existing_end and existing_start < end:
                raise StorageContributionProgressError(
                    f"authored storage extent overlaps existing storage "
                    f"{existing_id!r}"
                )
        elif start <= existing_start < end:
            raise StorageContributionProgressError(
                f"authored storage extent contains the unknown-extent start "
                f"of existing storage {existing_id!r}"
            )

    symbols = _require_mapping(tracker.get("symbols"), label="tracker.symbols")
    for existing_id, raw_row in symbols.items():
        if existing_id == symbol_id or not isinstance(raw_row, Mapping):
            continue
        if (
            raw_row.get("binary") != binary
            or raw_row.get("kind") not in {"data", "provider-data"}
            or ":logical-data:" in str(existing_id)
        ):
            continue
        raw_address = raw_row.get("address", raw_row.get("start"))
        if not isinstance(raw_address, str):
            continue
        existing_start = int(normalize_artifact_address(raw_address), 16)
        extent = _known_extent(raw_row)
        if extent is not None:
            existing_start, existing_end = extent
            if start < existing_end and existing_start < end:
                raise StorageContributionProgressError(
                    f"authored storage extent overlaps existing physical data "
                    f"symbol {existing_id!r}"
                )
        elif start <= existing_start < end:
            raise StorageContributionProgressError(
                f"authored storage extent contains the unknown-extent start "
                f"of existing physical data symbol {existing_id!r}"
            )


def plan_authored_storage_registration(
    tracker: Mapping[str, Any],
    payload: Mapping[str, Any],
    *,
    expected_revision: int,
) -> StorageContributionPlan:
    if (
        not isinstance(expected_revision, int)
        or isinstance(expected_revision, bool)
        or expected_revision < 0
    ):
        raise StorageContributionProgressError(
            "expected_revision must be a non-negative integer"
        )
    if tracker.get("schema_version") != TRACKER_SCHEMA_VERSION:
        raise StorageContributionProgressError(
            f"tracker schema_version must remain {TRACKER_SCHEMA_VERSION}"
        )
    if tracker.get("revision") != expected_revision:
        raise ConcurrentRevisionUpdate(
            f"revision changed: expected {expected_revision}, "
            f"found {tracker.get('revision')}"
        )
    normalized = normalize_register_payload(payload)
    symbol_id = normalized["symbol_id"]
    storage_id = normalized["storage_contribution_id"]
    owner_id = normalized["owner_id"]
    expected_symbol = normalized["expected_symbol"]
    expected_relationship = normalized["expected_owner_relationship"]

    symbols = _require_mapping(tracker.get("symbols"), label="tracker.symbols")
    symbol = _require_mapping(
        symbols.get(symbol_id),
        label=f"tracker.symbols[{symbol_id!r}]",
    )
    actual_symbol = {
        key: deepcopy(symbol.get(key))
        for key in expected_symbol
    }
    if actual_symbol != expected_symbol:
        raise StorageContributionProgressError(
            f"authored storage exact current symbol snapshot is stale for "
            f"{symbol_id!r}"
        )
    owners = _require_mapping(tracker.get("owners"), label="tracker.owners")
    primary_matches: list[tuple[str, Mapping[str, Any], Mapping[str, Any]]] = []
    address_matches: list[
        tuple[str, Mapping[str, Any], Mapping[str, Any]]
    ] = []
    for current_owner_id, raw_owner in owners.items():
        if not isinstance(raw_owner, Mapping):
            continue
        relationships = raw_owner.get("relationships")
        if not isinstance(relationships, list):
            continue
        for relationship in relationships:
            if not (
                isinstance(relationship, Mapping)
                and relationship.get("kind") == "primary-data"
            ):
                continue
            relationship_address = relationship.get("address")
            try:
                normalized_relationship_address = (
                    normalize_artifact_address(relationship_address)
                    if isinstance(relationship_address, str)
                    else None
                )
            except SourceTraceProgressError as exc:
                raise StorageContributionProgressError(str(exc)) from exc
            if normalized_relationship_address == expected_symbol["address"]:
                address_matches.append(
                    (str(current_owner_id), raw_owner, relationship)
                )
            if relationship.get("symbol_id") == symbol_id:
                primary_matches.append(
                    (str(current_owner_id), raw_owner, relationship)
                )
    if len(primary_matches) != 1:
        raise StorageContributionProgressError(
            f"authored storage requires exactly one existing primary-data "
            f"owner for {symbol_id!r}; found {len(primary_matches)}"
        )
    if (
        len(address_matches) != 1
        or address_matches[0][2].get("symbol_id") != symbol_id
    ):
        raise StorageContributionProgressError(
            "authored storage requires exactly one primary-data identity at "
            f"address {expected_symbol['address']!r}; found "
            f"{len(address_matches)}"
        )
    actual_owner_id, owner, relationship = primary_matches[0]
    if actual_owner_id != owner_id:
        raise StorageContributionProgressError(
            f"authored storage primary-data owner is {actual_owner_id!r}, "
            f"not reviewed owner {owner_id!r}"
        )
    if dict(relationship) != expected_relationship:
        raise StorageContributionProgressError(
            "authored storage exact current primary-data relationship is stale"
        )
    if owner.get("binary") != expected_symbol["binary"]:
        raise StorageContributionProgressError(
            "authored storage owner binary must equal the data symbol binary"
        )
    if (
        owner.get("kind") == "provider-boundary"
        or owner.get("provider_state") == "accepted"
    ):
        raise StorageContributionProgressError(
            "authored storage rejects provider-boundary owners"
        )
    storage_rows = _require_mapping(
        tracker.get("storage_contributions"),
        label="tracker.storage_contributions",
    )
    if storage_id in storage_rows:
        raise StorageContributionProgressError(
            f"authored storage contribution {storage_id!r} already exists"
        )
    existing_symbol_references = [
        str(existing_id)
        for existing_id, row in storage_rows.items()
        if isinstance(row, Mapping)
        and symbol_id in row.get("symbol_ids", [])
    ]
    if existing_symbol_references:
        raise StorageContributionProgressError(
            f"authored storage symbol {symbol_id!r} is already referenced by "
            f"storage contributions {sorted(existing_symbol_references)}"
        )
    dangling_symbol_references = [
        str(existing_id)
        for existing_id, row in symbols.items()
        if isinstance(row, Mapping)
        and storage_id in row.get("storage_contribution_ids", [])
    ]
    if dangling_symbol_references:
        raise StorageContributionProgressError(
            f"authored storage contribution {storage_id!r} is already "
            "referenced by symbols "
            f"{sorted(dangling_symbol_references)}"
        )

    try:
        section_start, section_end = _section_extent(
            tracker,
            artifact_id=symbol_id,
            output_section_id=expected_symbol["output_section_id"],
        )
    except DataExtentProgressError as exc:
        raise StorageContributionProgressError(str(exc)) from exc
    start = int(expected_symbol["address"], 16)
    end = int(expected_symbol["end_exclusive"], 16)
    if not section_start <= start < end <= section_end:
        raise StorageContributionProgressError(
            f"authored storage extent [0x{start:x},0x{end:x}) is outside "
            f"retail output section {expected_symbol['output_section_id']!r} "
            f"[0x{section_start:x},0x{section_end:x})"
        )
    _reject_physical_overlap(
        tracker,
        binary=expected_symbol["binary"],
        symbol_id=symbol_id,
        storage_contribution_id=storage_id,
        start=start,
        end=end,
    )

    storage_row = {
        "applicability": deepcopy(APPLICABILITY),
        "binary": expected_symbol["binary"],
        "candidate": {
            "evidence_ids": [],
            "state": "missing",
        },
        "evidence_ids": [],
        "kind": "data-symbol",
        "output_section_id": expected_symbol["output_section_id"],
        "overlap": "none",
        "owner_ids": [owner_id],
        "parent_contribution_id": None,
        "reference": {
            "address": expected_symbol["address"],
            "end_exclusive": expected_symbol["end_exclusive"],
            "evidence_ids": [],
            "extent_state": "known",
            "size": expected_symbol["size"],
        },
        "symbol_ids": [symbol_id],
        "verification": {
            dimension: _pending_claim()
            for dimension in STORAGE_DIMENSIONS
        },
    }

    proposed = deepcopy(dict(tracker))
    proposed_symbols = proposed.get("symbols")
    proposed_storage = proposed.get("storage_contributions")
    if not isinstance(proposed_symbols, dict) or not isinstance(
        proposed_storage, dict
    ):
        raise StorageContributionProgressError(
            "tracker symbols and storage_contributions must remain objects"
        )
    proposed_storage[storage_id] = deepcopy(storage_row)
    proposed_symbol = proposed_symbols.get(symbol_id)
    if not isinstance(proposed_symbol, dict):
        raise StorageContributionProgressError(
            "authored storage proposed symbol must remain an object"
        )
    proposed_symbol["storage_contribution_ids"] = [storage_id]
    try:
        validate_owner_invariants(proposed)
    except ProgressError as exc:
        raise StorageContributionProgressError(str(exc)) from exc

    return StorageContributionPlan(
        expected_revision=expected_revision,
        proposed_revision=expected_revision + 1,
        symbol_id=symbol_id,
        storage_contribution_id=storage_id,
        owner_id=owner_id,
        storage_row=storage_row,
        symbol_storage_before=(),
        symbol_storage_after=(storage_id,),
        output_section_start=f"0x{section_start:x}",
        output_section_end_exclusive=f"0x{section_end:x}",
        proposed=proposed,
    )


def mutate_authored_storage(
    tracker_path: str | Path,
    payload: Mapping[str, Any],
    *,
    expected_revision: int,
    apply: bool,
) -> dict[str, Any]:
    store = tracker_store(tracker_path)
    current = store.load()
    plan = plan_authored_storage_registration(
        current,
        payload,
        expected_revision=expected_revision,
    )
    commit = store.commit(
        plan.proposed,
        expected_revision=expected_revision,
        apply=apply,
    )
    return {
        **plan.to_dict(),
        **commit.to_dict(),
        "schema_version": TRACKER_SCHEMA_VERSION,
        "acceptance_changed": False,
        "source_edges_created": 0,
        "owner_records_changed": 0,
        "owner_gates_changed": 0,
        "owner_tiers_changed": 0,
        "order_state_changed": False,
        "byte_state_changed": False,
        "provider_state_changed": False,
        "link_state_changed": False,
        "final_image_state_changed": False,
    }


def load_payload(
    *,
    payload_json: str | None = None,
    payload_file: str | Path | None = None,
) -> dict[str, Any]:
    if (payload_json is None) == (payload_file is None):
        raise StorageContributionProgressError(
            "provide exactly one of --payload-json or --payload-file"
        )
    if payload_file is not None:
        path = Path(payload_file)
        try:
            text = path.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError) as exc:
            raise StorageContributionProgressError(
                f"cannot read authored storage payload file {path}: {exc}"
            ) from exc
    else:
        text = str(payload_json)
    try:
        return normalize_register_payload(json.loads(text))
    except json.JSONDecodeError as exc:
        raise StorageContributionProgressError(
            f"authored storage payload is invalid JSON: {exc}"
        ) from exc


__all__ = [
    "PAYLOAD_SCHEMA",
    "REGISTER_OPERATION",
    "StorageContributionPlan",
    "StorageContributionProgressError",
    "load_payload",
    "mutate_authored_storage",
    "normalize_register_payload",
    "plan_authored_storage_registration",
]
