from __future__ import annotations

import json
from pathlib import Path
from typing import Iterable


GATE_NAMES = ("boundary", "source", "data", "functional", "linkage", "byte")


def owner_record(
    owner_id: str,
    *,
    kind: str = "class",
    binary: str = "recoil",
    anchors: Iterable[str] = (),
    functions: Iterable[str] = (),
    data: Iterable[tuple[str, str]] = (),
    dependencies: Iterable[tuple[str, str]] = (),
    tiers: dict[str, str] | None = None,
    gates: dict[str, str] | None = None,
    name: str | None = None,
    section: str = "test-section",
    source_paths: Iterable[str] = ("src/Test.cpp",),
    address_metadata: dict[str, dict[str, object]] | None = None,
    blocker: str = "none",
) -> dict[str, object]:
    anchor_list = list(anchors)
    function_list = list(functions)
    data_list = list(data)
    dependency_list = list(dependencies)
    relationships: list[dict[str, object]] = []
    relationships.extend({"kind": "anchor-address", "address": address} for address in anchor_list)
    relationships.extend({"kind": "primary-function", "address": address} for address in function_list)
    relationships.extend(
        {"kind": "primary-data", "address": address, "name": data_name}
        for address, data_name in data_list
    )
    relationships.extend(
        {"kind": "depends-on-owner", "target_owner_id": target, "reason": reason}
        for target, reason in dependency_list
    )
    gate_payload = {gate: "pending" for gate in GATE_NAMES}
    gate_payload.update(gates or {})
    payload: dict[str, object] = {
        "id": owner_id,
        "kind": kind,
        "name": name or owner_id,
        "binary": binary,
        "section": section,
        "state": "active",
        "anchors": anchor_list,
        "member_addresses": function_list,
        "data_addresses": [
            {"address": address, "name": data_name} for address, data_name in data_list
        ],
        "source_paths": list(source_paths),
        "dependencies": [target for target, _reason in dependency_list],
        "relationships": relationships,
        "gates": gate_payload,
        "blocker": blocker,
        "evidence": ["schema-v4 unit fixture"],
    }
    if address_metadata is not None:
        payload["address_metadata"] = address_metadata
    if kind != "provider-boundary":
        entry_tiers = tiers or {}
        entries: dict[str, object] = {}
        for address in function_list:
            tier = entry_tiers.get(address, "X")
            entries[address] = {"kind": "function", "tier": tier, "evidence": f"fixture tier {tier}"}
        for address, _data_name in data_list:
            tier = entry_tiers.get(address, "X")
            entries[address] = {"kind": "data", "tier": tier, "evidence": f"fixture tier {tier}"}
        payload["reimplementation"] = {"entries": entries}
    return payload


def ledger_payload(*owners: dict[str, object]) -> dict[str, object]:
    return {"schema_version": 4, "owners": list(owners)}


def write_ledger(path: Path, *owners: dict[str, object]) -> None:
    path.write_text(json.dumps(ledger_payload(*owners), indent=2) + "\n", encoding="utf-8")
