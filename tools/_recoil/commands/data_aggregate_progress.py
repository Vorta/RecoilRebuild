"""Reviewed coalescence of split data globals into one native aggregate.

This is a semantic correction, not data, owner, byte, or linkage acceptance.
The deliberately narrow first route requires one owner, standalone physical
fields, complete field/padding coverage, and exact preservation of function registrations.
"""
from __future__ import annotations

import argparse
from copy import deepcopy
import json
from pathlib import Path
import re
import sys
from typing import Any, Mapping

from _recoil.commands.data_artifact_progress import (
    _normalize_reviewed_evidence, _pending_claim, PENDING_BINARY_DIMENSIONS,
    DataArtifactProgressError,
)
from _recoil.commands.data_extent_progress import (
    _require_exact_keys, _require_mapping, _section_extent, DataExtentProgressError,
)
from _recoil.commands.progress_v2 import allocate_evidence_id
from _recoil.commands.source_trace_progress import (
    tracker_store, SourceTraceProgressError,
)
from _recoil.commands.storage_contribution_progress import (
    APPLICABILITY, STORAGE_DIMENSIONS,
)
from _recoil.commands.vc5_verify import load_manifest
from _recoil.lib.live_progress import (
    TRACKER_SCHEMA_VERSION, ConcurrentRevisionUpdate, LiveProgressError,
)
from _recoil.lib.progress import (
    DEFAULT_PROGRESS_PATH, ProgressError, validate_owner_invariants,
    invalidate_order_dependencies, CALL_CONTRACT_DIMENSION, state_record,
)
from _recoil.lib.tooling import REPO_ROOT, configure_stdio
from _recoil.lib.verification_targets import vc5_target_registration


OPERATION = "coalesce-authored-data-aggregate"
SCHEMA = "recoil-data-aggregate-coalesce-v1"
PHYSICAL = re.compile(r"^(recoil|messages):data:(0x[0-9a-f]+)$")


class DataAggregateError(ValueError):
    pass


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise DataAggregateError(reason)


def positive(value: Any, label: str) -> int:
    require(type(value) is int and value > 0, f"{label} must be a positive integer")
    return value


def references(value: Any, identities: set[str]) -> bool:
    if isinstance(value, str):
        return value in identities
    if isinstance(value, Mapping):
        return any(str(key) in identities or references(item, identities)
                   for key, item in value.items())
    return isinstance(value, list) and any(references(item, identities) for item in value)


def extent_overlap(row: Mapping[str, Any], start: int, end: int) -> bool:
    address = row.get("address", row.get("image_address"))
    if address is None:
        return False
    first = int(address, 0) if isinstance(address, str) else int(address)
    if row.get("extent_state") == "known":
        size = positive(row.get("size"), "existing extent size")
        limit = row.get("end_exclusive")
        require(limit is not None and int(str(limit), 0) == first + size,
                "existing known extent has inconsistent end")
        return first < end and start < first + size
    # Unknown external predecessors cannot prove a boundary. The reviewed
    # layout evidence must establish it; interior identities are never ignored.
    return start <= first < end


def plan_coalescence(tracker: Mapping[str, Any], payload: Mapping[str, Any], *,
                    expected_revision: int, repo_root: Path = REPO_ROOT) -> dict[str, Any]:
    require(type(expected_revision) is int and expected_revision >= 0,
            "expected revision must be a nonnegative integer")
    if tracker.get("revision") != expected_revision:
        raise ConcurrentRevisionUpdate("data aggregate expected revision is stale")
    require(tracker.get("schema_version") == TRACKER_SCHEMA_VERSION, "wrong tracker schema")
    _require_mapping(payload, label="aggregate payload")
    _require_exact_keys(payload, keys=("schema", "operation", "reviewed", "artifact_id",
        "owner_id", "navigation_name", "size", "fields", "padding", "expected_current",
        "new_evidence"), label="aggregate payload")
    require(payload["schema"] == SCHEMA and payload["operation"] == OPERATION
            and payload["reviewed"] is True, "aggregate requires exact schema/operation and review")
    match = PHYSICAL.fullmatch(str(payload["artifact_id"]))
    require(match is not None, "aggregate requires a canonical physical data identity")
    binary, address = match.groups()
    aggregate_id = payload["artifact_id"]
    start = int(address, 16)
    size = positive(payload["size"], "aggregate size")
    end = start + size
    name = payload["navigation_name"]
    require(isinstance(name, str) and re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", name) is not None,
            "aggregate navigation name must be one source identifier")
    expected = _require_mapping(payload["expected_current"], label="expected_current")
    _require_exact_keys(expected, keys=("symbols", "storage_contributions", "owner",
        "verification_targets"), label="expected_current")
    for collection in ("symbols", "storage_contributions", "owner", "verification_targets"):
        _require_mapping(expected[collection], label=f"expected_current.{collection}")
    owner_id = payload["owner_id"]
    owner = tracker.get("owners", {}).get(owner_id)
    require(isinstance(owner, Mapping) and owner == expected["owner"], "exact owner snapshot is stale")
    require(owner.get("binary") == binary and owner.get("kind") != "provider-boundary",
            "aggregate requires one same-binary authored owner")
    symbols = tracker["symbols"]
    storages = tracker["storage_contributions"]
    fields = payload["fields"]
    require(isinstance(fields, list) and len(fields) >= 2, "aggregate requires at least two split fields")
    field_ids: set[str] = set()
    logical_ids: set[str] = set()
    storage_ids: set[str] = set()
    target_ids: set[str] = set()
    intervals: list[tuple[int, int]] = []
    section_id = symbols.get(aggregate_id, {}).get("output_section_id")
    require(section_id is not None, "aggregate base must be an existing field")
    field_addresses: set[str] = set()
    for field in fields:
        _require_mapping(field, label="aggregate field")
        _require_exact_keys(field, keys=("artifact_id", "logical_artifact_id", "field_name", "size"),
                            label="aggregate field")
        field_id = field["artifact_id"]
        m = PHYSICAL.fullmatch(str(field_id))
        require(m is not None and m.group(1) == binary and field_id not in field_ids,
                "field identities must be unique, physical, and same-binary")
        field_address = m.group(2)
        field_start = int(field_address, 16)
        field_size = positive(field["size"], "field size")
        require(start <= field_start < field_start + field_size <= end, "field is outside aggregate")
        logical_id = field["logical_artifact_id"]
        require(isinstance(logical_id, str) and re.fullmatch(
            re.escape(f"{binary}:logical-data:{field_address}:") + r"[a-z0-9][a-z0-9_-]*", logical_id)
            is not None and logical_id not in logical_ids, "invalid or duplicate logical field identity")
        require(isinstance(field["field_name"], str) and re.fullmatch(
            r"[A-Za-z_][A-Za-z_0-9]*", field["field_name"]) is not None, "invalid field name")
        row = symbols.get(field_id)
        require(isinstance(row, Mapping) and row == expected["symbols"].get(field_id),
                f"exact field snapshot is stale: {field_id}")
        require(row.get("kind") == "data" and row.get("disposition") == "authored"
                and row.get("binary") == binary and row.get("address") == field_address
                and row.get("output_section_id") == section_id, "field is not same-section authored data")
        require(row.get("physical_block_id") is None and not row.get("semantic_span_ids")
                and not row.get("logical_aliases"),
                "coalescence requires standalone fields without physical blocks or aliases")
        require(row.get("extent_state") in {"unknown", "known"}, "field extent state is unresolved")
        if row["extent_state"] == "known":
            require(row.get("size") == field_size and row.get("end_exclusive") == hex(field_start + field_size),
                    "reviewed field size contradicts existing known extent")
        else:
            require("size" not in row and "end_exclusive" not in row, "unknown field carries an extent")
        storage_id = f"{binary}:storage:va:{field_address}"
        storage = storages.get(storage_id)
        require(row.get("storage_contribution_ids") == [storage_id]
                and isinstance(storage, Mapping)
                and storage == expected["storage_contributions"].get(storage_id), "exact field storage is stale")
        require(storage.get("symbol_ids") == [field_id] and storage.get("owner_ids") == [owner_id]
                and storage.get("parent_contribution_id") is None and storage.get("overlap") == "none"
                and storage.get("kind") == "data-symbol" and storage.get("binary") == binary
                and storage.get("output_section_id") == section_id
                and storage.get("reference", {}).get("address") == field_address,
                "field storage must be standalone and uniquely owned")
        reference = storage["reference"]
        require(reference.get("extent_state") in {"known", "unknown"}, "invalid storage extent state")
        if reference["extent_state"] == "known":
            require(reference.get("size") == field_size
                    and reference.get("end_exclusive") == hex(field_start + field_size),
                    "reviewed field size contradicts known storage extent")
        else:
            require("size" not in reference and "end_exclusive" not in reference,
                    "unknown storage carries an extent")
        field_ids.add(field_id)
        field_addresses.add(field_address)
        logical_ids.add(logical_id)
        storage_ids.add(storage_id)
        target_ids.update(row.get("verification_target_ids", []))
        intervals.append((field_start - start, field_start + field_size - start))
    require(aggregate_id in field_ids, "aggregate base is not in field census")
    require(len({field["field_name"] for field in fields}) == len(fields), "duplicate aggregate field name")
    require(set(expected["symbols"]) == field_ids and set(expected["storage_contributions"]) == storage_ids,
            "expected field/storage census is not exact")
    require(isinstance(payload["padding"], list), "padding must be explicit, including an empty set")
    for padding in payload["padding"]:
        _require_mapping(padding, label="aggregate padding")
        _require_exact_keys(padding, keys=("offset", "size"), label="aggregate padding")
        offset = padding["offset"]
        require(type(offset) is int and offset >= 0, "padding offset must be nonnegative")
        intervals.append((offset, offset + positive(padding["size"], "padding size")))
    cursor = 0
    for first, limit in sorted(intervals):
        require(first == cursor and first < limit <= size, "field/padding coverage has a gap, overlap, or overrun")
        cursor = limit
    require(cursor == size, "field/padding coverage does not reach aggregate end")
    for symbol_id, row in symbols.items():
        require(symbol_id not in logical_ids and not logical_ids.intersection(row.get("logical_aliases", {})),
                "logical field identity already exists")
        if symbol_id not in field_ids and row.get("binary") == binary:
            require(not extent_overlap(row, start, end), f"unlisted physical symbol overlaps aggregate: {symbol_id}")
    for storage_id, row in storages.items():
        if storage_id not in storage_ids and row.get("binary") == binary:
            require(not extent_overlap(row.get("reference", {}), start, end),
                    f"unlisted storage overlaps aggregate: {storage_id}")
    section_start, section_end = _section_extent(tracker, artifact_id=aggregate_id, output_section_id=section_id)
    require(section_start <= start < end <= section_end, "aggregate is outside its retail section")
    for current_id, current_owner in tracker["owners"].items():
        claims = [r for r in current_owner.get("relationships", [])
                  if r.get("symbol_id") in field_ids or r.get("address") in field_addresses]
        if current_id != owner_id:
            # A navigation anchor retains its literal retail address; it makes
            # no physical ownership or storage claim and needs no remapping.
            other = deepcopy(dict(current_owner))
            other["relationships"] = [r for r in other.get("relationships", [])
                if not (set(r) == {"kind", "address"} and r.get("kind") == "anchor-address"
                        and r.get("address") in field_addresses)]
            require(not references(other, field_ids | storage_ids | field_addresses),
                    f"foreign owner refers to aggregate fields: {current_id}")
        else:
            require(len(claims) == len(fields) and {r.get("symbol_id") for r in claims} == field_ids
                    and all(r.get("kind") == "primary-data"
                            and r.get("address") == r["symbol_id"].split(":")[2] for r in claims),
                    "owner must have exactly one primary-data relationship per field")
    actual_targets = {target_id for target_id, target in tracker["verification_targets"].items()
                      if references(target, field_ids | field_addresses)}
    require(target_ids == actual_targets == set(expected["verification_targets"]),
            "verification-target reference census is not exact")
    artifact = {"binary": binary, "address": address, "size": size, "end_exclusive": hex(end),
                "output_section_id": section_id, "verification_target_ids": sorted(target_ids)}
    evidence = _normalize_reviewed_evidence(payload["new_evidence"], artifact_id=aggregate_id, artifact=artifact)
    # Read only the reviewed existing manifest closure. New registrations must
    # retain every non-data registration fact, including all function gates.
    # Old source edges describe the superseded physical definitions and are
    # archived below; logical field views require a fresh source relationship.
    replacements: dict[str, Any] = {}
    for target_id in sorted(target_ids):
        old = tracker["verification_targets"][target_id]
        require(old == expected["verification_targets"][target_id], "target snapshot is stale")
        registration = old.get("registration", {})
        path = (repo_root / registration["manifest_path"]).resolve()
        require(path.is_relative_to((repo_root / "tools/vc5_verify_targets").resolve()), "manifest outside governed directory")
        manifest = load_manifest(path, enforce_source_policy=True)
        new_id, new = vc5_target_registration(path)
        nr = new["registration"]
        require(new_id == target_id and nr["source_from"] == registration["source_from"]
                and nr["compiler_profile"] == registration["compiler_profile"], "target identity/source/profile changed")
        require({k: v for k, v in nr.items() if k != "data_addresses"} ==
                {k: v for k, v in registration.items() if k != "data_addresses"},
                "replacement target changes non-data registration facts")
        old_addresses = set(registration["data_addresses"])
        require(set(nr["data_addresses"]) == (old_addresses - field_addresses) | {address},
                "replacement target data census is not exact")
        aggregate_entries = [s for s in manifest.data_symbols if s.address == address]
        require(len(aggregate_entries) == 1 and aggregate_entries[0].byte_length == size
                and aggregate_entries[0].name == name, "manifest aggregate name/extent differs from review")
        replacements[target_id] = new
    proposed = deepcopy(dict(tracker))
    evidence_id = allocate_evidence_id(proposed)
    require(evidence_id not in proposed["evidence"], "evidence identity collision")
    aggregate_storage_id = f"{binary}:storage:va:{address}"
    aliases = {}
    for field in fields:
        old = symbols[field["artifact_id"]]
        field_start = int(old["address"], 16)
        aliases[field["logical_artifact_id"]] = {
            "kind": "data", "binary": binary, "disposition": "authored",
            "navigation_name": old["navigation_name"], "address": old["address"],
            "size": field["size"], "end_exclusive": hex(field_start + field["size"]), "extent_state": "known",
            "subobject": {"physical_artifact_id": aggregate_id, "offset": field_start - start,
                          "field_name": field["field_name"], "evidence_ids": [evidence_id]},
            "evidence_ids": [*old.get("evidence_ids", []), evidence_id],
            "source_traceability": {"state": "unresolved", "source_edges": [], "reason_code": "aggregate-field-view"},
        }
        del proposed["symbols"][field["artifact_id"]]
    proposed["symbols"][aggregate_id] = {
        **artifact, "kind": "data", "disposition": "authored", "navigation_name": name,
        "extent_state": "known", "evidence_ids": [evidence_id], "physical_block_id": None,
        "semantic_span_ids": [], "logical_aliases": aliases,
        "aggregate_layout": {"padding": deepcopy(payload["padding"]), "evidence_ids": [evidence_id]},
        "storage_contribution_ids": [aggregate_storage_id],
        "binary_state": {d: _pending_claim() for d in PENDING_BINARY_DIMENSIONS},
        "source_traceability": {"state": "unresolved", "source_edges": [], "reason_code": "reviewed-aggregate-unanchored"},
    }
    for storage_id in storage_ids:
        del proposed["storage_contributions"][storage_id]
    proposed["storage_contributions"][aggregate_storage_id] = {
        "kind": "data-symbol", "binary": binary, "output_section_id": section_id,
        "owner_ids": [owner_id], "symbol_ids": [aggregate_id], "parent_contribution_id": None,
        "overlap": "none", "applicability": deepcopy(APPLICABILITY), "evidence_ids": [evidence_id],
        "candidate": {"state": "missing", "evidence_ids": []},
        "reference": {k: v for k, v in proposed["symbols"][aggregate_id].items()
                      if k in {"address", "size", "end_exclusive", "extent_state", "evidence_ids"}},
        "verification": {d: _pending_claim() for d in STORAGE_DIMENSIONS},
    }
    section = proposed["output_sections"][section_id]
    contributions = section.get("contribution_ids", [])
    require(all(contributions.count(s) == 1 for s in storage_ids), "section storage census is not reciprocal")
    section["contribution_ids"] = [s for s in contributions if s not in storage_ids] + [aggregate_storage_id]
    changed_owner = proposed["owners"][owner_id]
    changed_owner["relationships"] = [r for r in changed_owner["relationships"] if r.get("symbol_id") not in field_ids]
    changed_owner["relationships"].append({"kind": "primary-data", "address": address,
                                            "symbol_id": aggregate_id, "name": name})
    entries = changed_owner.get("reimplementation", {}).get("entries", {})
    for field_id in field_ids:
        entries.pop(field_id, None)
    entries[aggregate_id] = {"kind": "data", "tier": "X", "evidence_ids": [evidence_id]}
    for field_address in field_addresses:
        changed_owner.get("address_metadata", {}).pop(field_address, None)
    for gate in ("boundary", "source", "data", "owner_linkage", "byte"):
        changed_owner.setdefault("gates", {})[gate] = "pending"
    changed_owner["blocker"] = "Aggregate identity corrected; source/data/linkage and byte gates require fresh scoped proof."
    changed_owner["evidence_ids"] = [*changed_owner.get("evidence_ids", []), evidence_id]
    proposed["verification_targets"].update(replacements)
    for row in proposed["symbols"].values():
        retained = set(row.get("verification_target_ids", [])) - target_ids
        for target_id, target in replacements.items():
            if row.get("address") in target["registered_addresses"]:
                retained.add(target_id)
        row["verification_target_ids"] = sorted(retained)
    # A global physical identity can be referenced by any body. Preserve exact
    # function order; conservatively invalidate code/data/target and call facts.
    affected = [sid for sid, row in proposed["symbols"].items() if row.get("binary") == binary]
    invalidate_order_dependencies(proposed, symbol_ids=affected)
    for sid in affected:
        row = proposed["symbols"][sid]
        if row.get("kind") in {"function", "provider-function"}:
            row.setdefault("binary_state", {})[CALL_CONTRACT_DIMENSION] = state_record("pending", "observed", "changed", [])
        row.pop("accepted_call_contract_facts", None)
    retired = (field_ids - {aggregate_id}) | (storage_ids - {aggregate_storage_id})
    # The reused base id changes from a field to an aggregate too. Never let
    # unrelated old field bindings silently acquire the new aggregate meaning.
    all_old_ids = field_ids | storage_ids
    superseded_matches = {}
    for sid, row in proposed["symbols"].items():
        if row.get("binary") == binary and references(row.get("function_match"), all_old_ids):
            superseded_matches[sid] = deepcopy(tracker["symbols"][sid]["function_match"])
            row.pop("function_match")
    for sid, row in proposed["symbols"].items():
        if sid != aggregate_id:
            require(not references(row, all_old_ids), f"unhandled current field binding in symbol {sid}")
    for storage_id, row in proposed["storage_contributions"].items():
        if storage_id != aggregate_storage_id:
            require(not references(row, all_old_ids), f"unhandled current field binding in storage {storage_id}")
    for current_section_id, row in proposed["output_sections"].items():
        if current_section_id != section_id:
            require(not references(row, all_old_ids), f"unhandled current field binding in section {current_section_id}")
    for collection, rows in proposed.items():
        if collection != "evidence":
            require(not references(rows, retired), f"unhandled current reference in {collection}; no mutation applied")
        if collection not in {"symbols", "storage_contributions", "owners", "output_sections",
                               "verification_targets", "evidence"}:
            require(not references(rows, all_old_ids), f"unhandled current base reference in {collection}")
    archive = {"symbols": deepcopy(expected["symbols"]), "storage_contributions": deepcopy(expected["storage_contributions"]),
               "owner": deepcopy(expected["owner"]), "verification_targets": deepcopy(expected["verification_targets"])}
    proposed["evidence"][evidence_id] = {
        "kind": "reviewed-data-aggregate-correction", "summary": evidence["summary"],
        "scope_ids": [aggregate_id, owner_id], "result": "observed", "disposition": "observed",
        "freshness": "historical", "gating": False, "validation_mode": "historical-observation",
        "artifacts": deepcopy(evidence["artifacts"]),
        "provenance": {**deepcopy(evidence), "superseded_records": archive,
                       "superseded_function_matches": superseded_matches,
                       "layout_fields": deepcopy(fields), "layout_padding": deepcopy(payload["padding"]),
                       "acceptance_effect": "invalidation-only"},
    }
    validate_owner_invariants(proposed)
    return {"proposed": proposed, "operation": OPERATION, "artifact_id": aggregate_id,
            "retired_physical_ids": sorted(retired), "logical_field_ids": sorted(logical_ids),
            "evidence_id": evidence_id, "changed_owner": changed_owner,
            "aggregate": proposed["symbols"][aggregate_id],
            "storage": proposed["storage_contributions"][aggregate_storage_id],
            "targets": replacements, "invalidated_symbol_count": len(affected),
            "function_order_changed": False, "positive_acceptance": False}


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tracker", default=str(DEFAULT_PROGRESS_PATH))
    parser.add_argument("--expected-revision", required=True, type=int)
    parser.add_argument("--payload-file", required=True)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--dry-run", action="store_true")
    mode.add_argument("--apply", action="store_true")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args(argv)
    try:
        payload = json.loads(Path(args.payload_file).read_text(encoding="utf-8"))
        store = tracker_store(args.tracker)
        plan = plan_coalescence(store.load(), payload, expected_revision=args.expected_revision)
        proposed = plan.pop("proposed")
        commit = store.commit(proposed, expected_revision=args.expected_revision, apply=args.apply)
    except (DataAggregateError, DataExtentProgressError, DataArtifactProgressError,
            SourceTraceProgressError, ConcurrentRevisionUpdate, LiveProgressError,
            ProgressError, OSError, ValueError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2
    print(json.dumps({**plan, **commit.to_dict()}, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
