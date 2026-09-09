"""Repair an exact pending creation that duplicated a pre-existing data owner."""
from __future__ import annotations

import argparse
from copy import deepcopy
import json
from pathlib import Path
import sys

from _recoil.commands.relocation_target_mutation import _pending_data_symbol, _validate_owner_evidence, RelocationTargetMutationError
from _recoil.commands.relocation_expectations import (
    DEFAULT_REFERENCE, RelocationExpectationError, normalize_relocation_target_binding, relocation_target_row_context,
    relocation_target_owner_context, relocation_target_binding_staleness,
)
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH, ProgressDocument, ProgressError, ProgressStore, address_value
from _recoil.lib.tooling import REPO_ROOT, configure_stdio


def repair_pending_owner(document: ProgressDocument, payload: dict) -> tuple[dict, dict]:
    fields = {"schema", "reviewed", "reason", "target_symbol_id", "current_target", "current_owner",
              "retained_owner_id", "current_retained_owner", "evidence_ids"}
    if set(payload) != fields or payload["schema"] != "recoil-repair-created-relocation-owner-v1":
        raise ProgressError("invalid created-relocation-owner repair payload")
    if payload["reviewed"] is not True or not isinstance(payload["reason"], str) or not payload["reason"].strip():
        raise ProgressError("repair requires reviewed=true and a reason")
    target_id = payload["target_symbol_id"]
    target = document.collection("symbols").get(target_id)
    if not isinstance(target, dict) or target != payload["current_target"]:
        raise ProgressError("created target snapshot is stale")
    binding = target.get("relocation_target_binding")
    if not isinstance(binding, dict):
        raise ProgressError("repair requires exactly one singular creation binding")
    context = binding.get("binding_context", {})
    if context.get("creation_mode") != "created-data-symbol":
        raise ProgressError("only a created-data-symbol binding may be repaired")
    owner_id = context.get("owner", {}).get("owner_id")
    owner = document.collection("owners").get(owner_id)
    if not isinstance(owner, dict) or owner != payload["current_owner"]:
        raise ProgressError("created owner snapshot is stale")
    pending = _pending_data_symbol(address=address_value(target["address"]),
        end_exclusive=address_value(target["end_exclusive"]), name=target["navigation_name"],
        output_section_id=target["output_section_id"], evidence_ids=target["evidence_ids"])
    pending["relocation_target_binding"] = binding
    if target != pending or target_id != "recoil:data:" + target["address"]:
        raise ProgressError("created data has acquired additional state or no longer matches the pending creation")
    relationship = context.get("relationship")
    if not isinstance(relationship, dict) or relationship.get("kind") != "primary-data" or relationship.get("symbol_id") != target_id:
        raise ProgressError("creation binding has no exact primary-data relationship")
    current_relationships = [item for item in owner.get("relationships", [])
                             if item.get("symbol_id") == target_id or
                             (item.get("kind") == "primary-data" and item.get("address") == target["address"])]
    if current_relationships != [relationship]:
        raise ProgressError("created owner relationship is stale or duplicated")
    entry = owner.get("reimplementation", {}).get("entries", {}).get(target_id)
    if entry != {"kind": "data", "tier": "X", "evidence_ids": []}:
        raise ProgressError("repair cannot remove a pre-existing or subsequently reviewed owner entry")
    if target["address"] in owner.get("address_metadata", {}):
        raise ProgressError("created owner has acquired address metadata")

    retained_id = payload["retained_owner_id"]
    retained = document.collection("owners").get(retained_id)
    if retained_id == owner_id or retained != payload["current_retained_owner"]:
        raise ProgressError("retained owner snapshot is stale or selects the created owner")
    _validate_owner_evidence(document, owner_id=retained_id, evidence_ids=payload["evidence_ids"])
    retained_relations = [item for item in retained.get("relationships", [])
        if item.get("kind") == "primary-data" and item.get("symbol_id") == target_id and
           item.get("address") == target["address"]]
    if len(retained_relations) != 1 or not retained_relations[0].get("name"):
        raise ProgressError("retained owner must already have one exact named primary-data relationship")
    retained_entry = retained.get("reimplementation", {}).get("entries", {}).get(target_id)
    if not isinstance(retained_entry, dict) or retained_entry.get("kind") != "data":
        raise ProgressError("retained owner lacks its pre-existing data entry")
    for other_id, other in document.collection("owners").items():
        if other_id in {owner_id, retained_id}:
            continue
        if any(item.get("kind") == "primary-data" and
               (item.get("symbol_id") == target_id or item.get("address") == target["address"])
               for item in other.get("relationships", [])):
            raise ProgressError("more than two owners claim the created data")
    proposed = deepcopy(document.data)
    revised_owner = proposed["owners"][owner_id]
    revised_owner["relationships"].remove(relationship)
    del revised_owner["reimplementation"]["entries"][target_id]
    revised_target = proposed["symbols"][target_id]
    revised_target["navigation_name"] = retained_relations[0]["name"]
    revised_binding = deepcopy(binding)
    revised_binding["reason"] = payload["reason"]
    revised_binding["evidence_ids"] = payload["evidence_ids"]
    revised_context = revised_binding["binding_context"]
    revised_context["owner"] = relocation_target_owner_context(owner_id=retained_id,
        owner=retained, evidence_ids=payload["evidence_ids"])
    revised_context["relationship"] = deepcopy(retained_relations[0])
    revised_context["target"] = relocation_target_row_context(symbol_id=target_id,
        row=revised_target, object_symbol=binding["object_symbol"])
    revised_target["relocation_target_binding"] = normalize_relocation_target_binding(revised_binding)
    return proposed, dict(target_symbol_id=target_id, owner_id=owner_id,
        removed_relationship=relationship, removed_entry=entry, retained_owner_id=retained_id,
        target_before=target, target_after=revised_target,
        reason=payload["reason"], other_owner_facts_preserved=True)


def main(argv=None):
    configure_stdio()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--payload-file", type=Path, required=True)
    parser.add_argument("--progress", type=Path, default=DEFAULT_PROGRESS_PATH)
    parser.add_argument("--reference", type=Path, default=DEFAULT_REFERENCE)
    parser.add_argument("--expected-revision", type=int, required=True)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--dry-run", action="store_true")
    mode.add_argument("--apply", action="store_true")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args(argv)
    try:
        path = args.payload_file.resolve()
        if not path.is_relative_to((REPO_ROOT / "build").resolve()):
            raise ProgressError("repair payload must be under workspace build/")
        payload = json.loads(path.read_text(encoding="utf-8-sig"))
        store = ProgressStore(args.progress)
        document = store.load()
        proposed, detail = repair_pending_owner(document, payload)
        from _recoil.commands.live_byte_verify import _bindings
        from _recoil.commands.relocation_target_mutation import DEFAULT_MANIFEST_DIR
        target_id = payload["target_symbol_id"]
        revised_binding = proposed["symbols"][target_id]["relocation_target_binding"]
        _, stale = relocation_target_binding_staleness(revised_binding,
            document=ProgressDocument(proposed), bindings=_bindings(ProgressDocument(proposed), DEFAULT_MANIFEST_DIR),
            target_symbol_id=target_id, reference=args.reference)
        if stale:
            raise ProgressError("repaired binding does not pass the live retail context check: " + json.dumps(stale))
        commit = store.commit(proposed, expected_revision=args.expected_revision, apply=args.apply)
        print(json.dumps(dict(kind="created-relocation-owner-repair", **detail, commit=commit.to_dict()), indent=2))
    except (OSError, ValueError, KeyError, TypeError, ProgressError, RelocationTargetMutationError, RelocationExpectationError) as exc:
        print(f"created relocation owner repair error: {exc}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
