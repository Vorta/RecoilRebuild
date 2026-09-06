from __future__ import annotations

import json
from pathlib import Path
import sqlite3
import sys
from types import MappingProxyType

import pytest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from _recoil.lib.progress import ProgressDocument  # noqa: E402
from _recoil.commands.progress_cli import (  # noqa: E402
    OrderTargetRoleGateError,
    _order_row_role_gate,
    _parse_source_path_relocation_payload,
    _relocate_source_paths,
    _source_path_relocation_matches,
)
from _recoil.lib.repository_paths import RepositoryPathInventory  # noqa: E402


OLD_TIME_PREFIX = "src/GameZRecoil/" + "Time"
NEW_TIME_PREFIX = "src/GameZRecoil/zTime"
from _recoil.lib.progress_sqlite import (  # noqa: E402
    APPLICATION_ID,
    DELETE_FACET,
    ConcurrentSQLiteProgressUpdate,
    ProgressSQLiteStore,
)


def make_store(path: Path) -> ProgressSQLiteStore:
    document = ProgressDocument.empty().data
    document["evidence"] = {
        "recoil:evidence:unit": {"kind": "unit-evidence", "scope_ids": []}
    }
    return ProgressSQLiteStore.create_from_mapping(
        path, document, cutover_pair_id="proof-kernel"
    )


@pytest.mark.parametrize("phase", ["authored-function-order", "full-function-order"])
@pytest.mark.parametrize("registered,tracked", [("non-authored", "authored"), ("authored", "non-authored")])
def test_order_role_gate_rejects_independent_classification_disagreement(phase, registered, tracked):
    with pytest.raises(OrderTargetRoleGateError, match="pipeline_class disagrees"):
        _order_row_role_gate(
            target_id="unit-target", phase=phase,
            row={"pipeline_class": registered, "authored_order_role": "non-authored" if registered == "non-authored" else "authored-body"},
            tracker_row={"pipeline_class": tracked},
            address="0x401000", identity="recoil:function:0x401000",
        )


def test_order_role_gate_rejects_independent_role_disagreement():
    with pytest.raises(OrderTargetRoleGateError, match="authored_order_role disagrees"):
        _order_row_role_gate(
            target_id="unit-target", phase="authored-function-order",
            row={"pipeline_class": "authored", "authored_order_role": "authored-body"},
            tracker_row={"pipeline_class": "authored", "authored_order_role": "compiler-generated-thunk"},
            address="0x401000", identity="recoil:function:0x401000",
        )


def test_order_role_gate_preserves_legacy_missing_role_fallback():
    _order_row_role_gate(
        target_id="unit-target", phase="authored-function-order",
        row={"pipeline_class": "authored", "authored_order_role": "authored-body"},
        tracker_row={"pipeline_class": "authored"},
        address="0x401000", identity="recoil:function:0x401000",
    )


def test_provider_registration_accepts_only_detached_non_authored_inventory():
    from _recoil.commands.provider_function_mutation import (
        _validate_existing_function, _validate_tracker_ownership,
        ProviderFunctionMutationError,
    )
    document = ProgressDocument.empty()
    symbol_id = "recoil:function:0x401000"
    row = {
        "binary": "recoil", "kind": "function", "address": "0x401000",
        "end_exclusive": "0x401020", "size": 32, "extent_state": "known",
        "pipeline_class": "non-authored", "authored_order_role": "non-authored",
        "ownership_state": "primary-owned", "disposition": "unresolved",
        "output_section_id": "recoil:section:.text",
    }
    document.data["symbols"][symbol_id] = row
    for role in ("non-authored", "compiler-generated-icf-representative"):
        row["authored_order_role"] = role
        assert _validate_existing_function(
            document, function_id=symbol_id, address="0x401000",
        )[1:] == (0x401000, 0x401020)
        _validate_tracker_ownership(
            document, owner_id="new-provider", function_id=symbol_id,
            start=0x401000, end=0x401020,
        )
        for reference in ({"symbol_id": symbol_id}, {"address": "0x401000"}):
            document.data["owners"]["existing-owner"] = {
                "relationships": [{"kind": "primary-function", **reference}],
            }
            with pytest.raises(ProviderFunctionMutationError, match="still claimed"):
                _validate_existing_function(
                    document, function_id=symbol_id, address="0x401000",
                )
            document.data["owners"].clear()
        document.data["owners"]["existing-owner"] = {
            "relationships": [{"kind": "helper", "symbol_id": symbol_id}],
        }
        with pytest.raises(ProviderFunctionMutationError, match="already owned"):
            _validate_tracker_ownership(
                document, owner_id="new-provider", function_id=symbol_id,
                start=0x401000, end=0x401020,
            )
        document.data["owners"].clear()
    row["authored_order_role"] = "authored-body"
    with pytest.raises(ProviderFunctionMutationError, match="existing row is not"):
        _validate_existing_function(document, function_id=symbol_id, address="0x401000")


def test_provider_trace_producer_emits_explicit_empty_edge_population():
    from _recoil.commands.provider_function_mutation import _provider_boundary_source_traceability
    from _recoil.commands.source_trace_progress import normalize_source_traceability
    trace = _provider_boundary_source_traceability()
    assert trace == {"state": "not-applicable", "source_edges": [], "reason_code": "provider-boundary"}
    assert normalize_source_traceability(trace) == trace


def test_data_aggregate_coalescence_preserves_views_and_rejects_incomplete_closure(tmp_path, monkeypatch):
    from copy import deepcopy
    from types import SimpleNamespace
    from _recoil.commands import data_aggregate_progress as aggregate
    data = ProgressDocument.empty().data
    owner_id = "recoil:owner:unit.aggregate"
    section_id = "recoil:section:.data"
    target_id = "recoil:vc5-target:unit_data"
    base, tail = "0x4da010", "0x4da014"
    field_ids = [f"recoil:data:{address}" for address in (base, tail)]
    storage_ids = [f"recoil:storage:va:{address}" for address in (base, tail)]
    data["output_sections"][section_id] = {
        "binary": "recoil", "reference": {"image_address": "0x4da000", "virtual_size": 256},
        "contribution_ids": storage_ids.copy(),
    }
    data["owners"][owner_id] = {
        "binary": "recoil", "kind": "record", "name": "Unit aggregate", "section": "unit",
        "lifecycle_state": "discovered", "source_paths": [], "evidence_ids": [],
        "gates": {g: "pending" for g in ("boundary", "source", "data", "owner_linkage", "byte")},
        "relationships": [{"kind": "primary-data", "symbol_id": sid, "address": a, "name": n}
                          for sid, a, n in zip(field_ids, (base, tail), ("unitTag", "unitValue"))],
        "reimplementation": {"entries": {sid: {"kind": "data", "tier": "X", "evidence_ids": []}
                                         for sid in field_ids}},
    }
    for sid, storage_id, address, name in zip(field_ids, storage_ids, (base, tail), ("unitTag", "unitValue")):
        data["symbols"][sid] = {
            "binary": "recoil", "kind": "data", "disposition": "authored", "address": address,
            "navigation_name": name, "extent_state": "unknown", "output_section_id": section_id,
            "storage_contribution_ids": [storage_id], "verification_target_ids": [target_id],
        }
        data["storage_contributions"][storage_id] = {
            "binary": "recoil", "kind": "data-symbol", "output_section_id": section_id,
            "reference": {"address": address, "extent_state": "unknown"}, "symbol_ids": [sid],
            "owner_ids": [owner_id], "parent_contribution_id": None, "overlap": "none",
        }
    registration = {
        "manifest_path": "tools/vc5_verify_targets/unit_data.json", "source_from": "src/unit.cpp",
        "compiler_profile": "unit", "data_addresses": [base, tail], "function_addresses": [],
        "translation_unit_function_order": [], "linked_function_intervals": [],
        "check_function_order": False, "check_translation_unit_function_order": False,
    }
    data["verification_targets"][target_id] = {"registration": registration}
    replacement = {"registration": {**registration, "data_addresses": [base]}, "registered_addresses": [base]}
    monkeypatch.setattr(aggregate, "vc5_target_registration", lambda path: (target_id, deepcopy(replacement)))
    monkeypatch.setattr(aggregate, "load_manifest", lambda *a, **kw: SimpleNamespace(
        data_symbols=[SimpleNamespace(address=base, byte_length=8, name="unitAggregate")]))
    function_id = "recoil:function:0x401000"
    data["symbols"][function_id] = {"kind": "function", "binary": "recoil", "address": "0x401000",
        "binary_state": {"call_contract": {"result": "passed"}}, "accepted_byte_facts": {"unit": True}}
    data["physical_blocks"]["recoil:block:0x401000"] = {"accepted_order_facts": {"unit": True}}
    payload = {
        "schema": aggregate.SCHEMA, "operation": aggregate.OPERATION, "reviewed": True,
        "artifact_id": field_ids[0], "owner_id": owner_id, "navigation_name": "unitAggregate", "size": 8,
        "fields": [{"artifact_id": sid, "logical_artifact_id": f"recoil:logical-data:{a}:{n}",
                    "field_name": n, "size": s}
                   for sid, a, n, s in zip(field_ids, (base, tail), ("tag", "value"), (1, 4))],
        "padding": [{"offset": 1, "size": 3}],
        "expected_current": {"symbols": {sid: deepcopy(data["symbols"][sid]) for sid in field_ids},
            "storage_contributions": deepcopy(data["storage_contributions"]),
            "owner": deepcopy(data["owners"][owner_id]), "verification_targets": deepcopy(data["verification_targets"])},
        "new_evidence": {"kind": "reviewed-data-artifact-observation",
            "method": "immutable-retail-bn-plus-vc5sp3-listing",
            "summary": "Retail 0x4da010 accesses and native ABI prove the unit aggregate field and padding partition.",
            "scope_ids": [field_ids[0]], "observation": {"artifact_id": field_ids[0], "address": base,
                "size": 8, "end_exclusive": "0x4da018", "output_section_id": section_id},
            "command": f"python tools/recoil.py audit bn-data-evidence {base} --size 8 --binary recoil --json",
            "target_id": target_id, "artifacts": []},
    }
    before = deepcopy(data)
    result = aggregate.plan_coalescence(data, payload, expected_revision=data["revision"], repo_root=tmp_path)
    proposed = result["proposed"]
    assert data == before
    assert field_ids[1] not in proposed["symbols"]
    assert len(proposed["symbols"][field_ids[0]]["logical_aliases"]) == 2
    assert proposed["symbols"][field_ids[0]]["size"] == 8
    assert proposed["symbols"][function_id]["binary_state"]["call_contract"]["result"] == "pending"
    assert "accepted_byte_facts" not in proposed["symbols"][function_id]
    assert proposed["physical_blocks"] == data["physical_blocks"]
    assert proposed["evidence"][result["evidence_id"]]["provenance"]["superseded_records"] == payload["expected_current"]
    assert len(proposed["storage_contributions"]) == 1
    for corrupt, message in (
        (lambda d, p: p["padding"].clear(), "coverage"),
        (lambda d, p: p["fields"][0].update(size=5), "overlap"),
        (lambda d, p: p["fields"][1].update(logical_artifact_id=p["fields"][0]["logical_artifact_id"]), "logical"),
        (lambda d, p: p["expected_current"]["symbols"][field_ids[0]].update(navigation_name="stale"), "stale"),
        (lambda d, p: d["owners"].update({"foreign": {"relationships": [{"address": tail}]}}), "foreign"),
        (lambda d, p: d["symbols"].update({"recoil:data:0x4da012": {"binary": "recoil", "address": "0x4da012"}}), "overlaps"),
        (lambda d, p: d["symbols"].update({"recoil:data:0x4da008": {"binary": "recoil", "address": "0x4da008",
            "extent_state": "known", "size": 16, "end_exclusive": "0x4da018"}}), "overlaps"),
        (lambda d, p: d.update(unit_unhandled_reference={"target": field_ids[1]}), "unhandled current reference"),
        (lambda d, p: d.update(unit_unhandled_reference={"target": field_ids[0]}), "unhandled current base"),
        (lambda d, p: d["symbols"][function_id].update(target_binding=field_ids[0]), "unhandled current field"),
        (lambda d, p: p["expected_current"]["verification_targets"].clear(), "census"),
        (lambda d, p: d["storage_contributions"].update({"unhandled-parent": {"parent_contribution_id": storage_ids[0]}}), "field binding in storage"),
    ):
        changed, request = deepcopy(data), deepcopy(payload)
        corrupt(changed, request)
        with pytest.raises(aggregate.DataAggregateError, match=message):
            aggregate.plan_coalescence(changed, request, expected_revision=data["revision"], repo_root=tmp_path)


def test_source_trace_repair_compares_raw_current_but_validates_replacement():
    from copy import deepcopy
    from _recoil.commands.source_trace_progress import (
        plan_source_traceability_batch, SourceTraceProgressError,
    )
    data = ProgressDocument.empty().data
    symbol = "recoil:function:0x401000"
    malformed = {"state": "not-applicable", "reason_code": "provider-boundary"}
    data["symbols"][symbol] = {"source_traceability": malformed}
    replacement = {**malformed, "source_edges": []}
    payload = {"operation": "replace-batch", "reviewed": True, "updates": [{
        "artifact_id": symbol, "expected_current": malformed,
        "source_traceability": replacement,
    }]}
    plan = plan_source_traceability_batch(data, payload, expected_revision=data["revision"])
    assert plan.proposed["symbols"][symbol]["source_traceability"] == replacement
    assert data["symbols"][symbol]["source_traceability"] == malformed
    stale = deepcopy(payload)
    stale["updates"][0]["expected_current"] = replacement
    with pytest.raises(SourceTraceProgressError, match="expected_current is stale"):
        plan_source_traceability_batch(data, stale, expected_revision=data["revision"])
    invalid = deepcopy(payload)
    invalid["updates"][0]["source_traceability"] = malformed
    with pytest.raises(SourceTraceProgressError, match="missing"):
        plan_source_traceability_batch(data, invalid, expected_revision=data["revision"])


def test_new_store_has_one_atomic_three_domain_revision_vector(tmp_path: Path) -> None:
    path = tmp_path / "progress.sqlite3"
    store = make_store(path)
    document, vector = store.materialize_with_revision_vector()
    assert document["revision"] == 0
    assert vector.to_dict() == {
        "transaction_revision": 0,
        "semantic_revision": 0,
        "evidence_generation_revision": 0,
    }
    with sqlite3.connect(path) as connection:
        assert connection.execute("PRAGMA application_id").fetchone()[0] == APPLICATION_ID


def test_stale_transaction_revision_is_rejected(tmp_path: Path) -> None:
    store = make_store(tmp_path / "progress.sqlite3")
    candidate = store.materialize()
    candidate["id_sequences"] = {"unit": 1}
    store.commit(candidate, expected_revision=0, apply=True)
    with pytest.raises(ConcurrentSQLiteProgressUpdate):
        store.commit(candidate, expected_revision=0, apply=True)


def test_scoped_delete_updates_all_requested_domains_atomically(tmp_path: Path) -> None:
    store = make_store(tmp_path / "progress.sqlite3")
    commit = store.persist_scoped_changes(
        expected_domain_revisions={"semantic": 0, "evidence_generation": 0},
        entity_patches={
            "evidence": {"recoil:evidence:unit": {"": DELETE_FACET}}
        },
        increment_domains={"semantic", "evidence_generation"},
        apply=True,
    )
    assert commit.deleted_entities == 1
    assert store.materialize()["evidence"] == {}
    assert commit.revision_vector.to_dict() == {
        "transaction_revision": 1,
        "semantic_revision": 1,
        "evidence_generation_revision": 1,
    }


def _path_inventory(tmp_path: Path, *paths: str) -> RepositoryPathInventory:
    exact = frozenset(paths)
    return RepositoryPathInventory(
        repository_root=tmp_path,
        exact_paths=exact,
        casefolded_paths=MappingProxyType(
            {path.casefold(): (path,) for path in exact}
        ),
        allowed_roots=("src",),
        allowed_paths=(),
    )


def _source_path_payload(**expected_overrides: list[str]) -> dict[str, object]:
    expected = {
        "physical_block_ids": [],
        "semantic_span_ids": ["recoil:semantic:unit"],
        "owner_ids": ["recoil:owner:unit"],
        "artifact_ids": ["recoil:function:unit"],
        "pre_synced_verification_target_ids": ["recoil:vc5-target:unit"],
    }
    expected.update(expected_overrides)
    return {
        "schema": "recoil-source-path-relocation-v1",
        "reviewed": True,
        "reason": "unit relocation",
        "binary": "recoil",
        "old_prefix": OLD_TIME_PREFIX,
        "new_prefix": NEW_TIME_PREFIX,
        "expected_matches": expected,
    }


def _source_path_document() -> dict[str, object]:
    data = ProgressDocument.empty().data
    data["semantic_spans"] = {
        "recoil:semantic:unit": {
            "binary": "recoil",
            "source_path": OLD_TIME_PREFIX + "/Time.cpp",
        }
    }
    data["owners"] = {
        "recoil:owner:unit": {
            "binary": "recoil",
            "tier": "B",
            "gates": {"source": "accepted"},
            "source_paths": [
                OLD_TIME_PREFIX + "/Time.cpp",
                OLD_TIME_PREFIX + "/Time.h",
            ],
            "address_metadata": {
                "0x401000": {
                    "source_path": OLD_TIME_PREFIX + "/Time.cpp",
                    "anchor_id": "recoil:anchor:unit",
                }
            },
        }
    }
    data["symbols"] = {
        "recoil:function:unit": {
            "binary": "recoil",
            "binary_state": {
                "call_contract": {
                    "result": "passed",
                    "disposition": "accepted",
                    "freshness": "current",
                    "evidence_ids": ["recoil:evidence:unit"],
                }
            },
            "accepted_call_contract_facts": {"unit": True},
            "source_traceability": {
                "state": "resolved",
                "source_edges": [
                    {
                        "anchor_id": "recoil:anchor:unit",
                        "relation": "defines",
                        "evidence_ids": [],
                        "emission_context": {
                            "translation_unit": OLD_TIME_PREFIX + "/Time.cpp"
                        },
                    }
                ],
            },
            "logical_aliases": {
                "recoil:logical:unit": {
                    "source_traceability": {
                        "state": "resolved",
                        "source_edges": [
                            {
                                "anchor_id": "recoil:anchor:logical-unit",
                                "relation": "defines",
                                "evidence_ids": [],
                                "emission_context": {
                                    "translation_unit": OLD_TIME_PREFIX + "/Time.cpp"
                                },
                            }
                        ],
                    }
                }
            },
        }
    }
    data["verification_targets"] = {
        "recoil:vc5-target:unit": {
            "binary": "recoil",
            "registration": {
                "source_from": NEW_TIME_PREFIX + "/Time.cpp",
                "order_edit_paths": [NEW_TIME_PREFIX + "/Time.h"],
            },
        }
    }
    return data


def test_source_path_relocation_is_boundary_aware_and_conservatively_invalidates(
    tmp_path: Path,
) -> None:
    data = _source_path_document()
    data["migration"]["unrelated_path"] = OLD_TIME_PREFIX + "keeper/Clock.cpp"
    owner_before = dict(data["owners"]["recoil:owner:unit"])
    details = _relocate_source_paths(
        data,
        _source_path_payload(),
        inventory=_path_inventory(
            tmp_path,
            NEW_TIME_PREFIX + "/Time.cpp",
            NEW_TIME_PREFIX + "/Time.h",
        ),
    )
    owner = data["owners"]["recoil:owner:unit"]
    symbol = data["symbols"]["recoil:function:unit"]
    assert owner["source_paths"] == [
        NEW_TIME_PREFIX + "/Time.cpp",
        NEW_TIME_PREFIX + "/Time.h",
    ]
    assert owner["tier"] == owner_before["tier"]
    assert owner["gates"] == owner_before["gates"]
    assert owner["address_metadata"]["0x401000"]["anchor_id"] == "recoil:anchor:unit"
    assert symbol["binary_state"]["call_contract"]["result"] == "pending"
    assert "accepted_call_contract_facts" not in symbol
    assert (
        symbol["logical_aliases"]["recoil:logical:unit"]["source_traceability"]
        ["source_edges"][0]["emission_context"]["translation_unit"]
        == NEW_TIME_PREFIX + "/Time.cpp"
    )
    assert data["migration"]["unrelated_path"] == OLD_TIME_PREFIX + "keeper/Clock.cpp"
    assert details["scheduler_before"]["phase"] == details["scheduler_after"]["phase"]
    assert details["preserved"]["acceptance_not_expanded"] is True


def test_source_path_relocation_refuses_unsynchronized_or_drifted_scope(
    tmp_path: Path,
) -> None:
    inventory = _path_inventory(
        tmp_path,
        NEW_TIME_PREFIX + "/Time.cpp",
        NEW_TIME_PREFIX + "/Time.h",
    )
    unsynced = _source_path_document()
    unsynced["verification_targets"]["recoil:vc5-target:unit"]["registration"][
        "source_from"
    ] = OLD_TIME_PREFIX + "/Time.cpp"
    with pytest.raises(ValueError, match="verification-target sync first"):
        _relocate_source_paths(unsynced, _source_path_payload(), inventory=inventory)

    drifted = _source_path_document()
    with pytest.raises(ValueError, match="exact match scope changed"):
        _relocate_source_paths(
            drifted,
            _source_path_payload(owner_ids=["recoil:owner:wrong"]),
            inventory=inventory,
        )


def test_source_path_relocation_refuses_missing_new_files_and_protected_survivors(
    tmp_path: Path,
) -> None:
    with pytest.raises(ValueError, match="no authenticated repository files"):
        _relocate_source_paths(
            _source_path_document(),
            _source_path_payload(),
            inventory=_path_inventory(tmp_path, "src/Elsewhere.cpp"),
        )

    protected = _source_path_document()
    protected["physical_blocks"] = {
        "recoil:block:protected": {
            "binary": "other",
            "original_source_path": OLD_TIME_PREFIX + "/Original.cpp",
        }
    }
    with pytest.raises(ValueError, match="outside its allowed"):
        _relocate_source_paths(
            protected,
            _source_path_payload(),
            inventory=_path_inventory(
                tmp_path,
                NEW_TIME_PREFIX + "/Time.cpp",
                NEW_TIME_PREFIX + "/Time.h",
            ),
        )


def test_source_path_relocation_payload_rejects_noop_case_and_nested_moves() -> None:
    payload = _source_path_payload()
    payload["new_prefix"] = "src/GameZRecoil/" + "time"
    with pytest.raises(ValueError, match="differ beyond path case"):
        _parse_source_path_relocation_payload(json.dumps(payload))

    payload = _source_path_payload()
    payload["new_prefix"] = OLD_TIME_PREFIX + "/Runtime"
    with pytest.raises(ValueError, match="nested old/new prefixes"):
        _parse_source_path_relocation_payload(json.dumps(payload))


def test_source_path_match_inventory_ignores_neighboring_prefixes() -> None:
    data = _source_path_document()
    data["owners"]["recoil:owner:neighbor"] = {
        "binary": "recoil",
        "source_paths": [OLD_TIME_PREFIX + "keeper/Clock.cpp"],
    }
    assert _source_path_relocation_matches(
        data,
        binary="recoil",
        old_prefix=OLD_TIME_PREFIX,
        new_prefix=NEW_TIME_PREFIX,
    )["owner_ids"] == ["recoil:owner:unit"]
