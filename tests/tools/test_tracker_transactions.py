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
