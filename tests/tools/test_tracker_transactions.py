from __future__ import annotations

from pathlib import Path
import sqlite3
import sys

import pytest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from _recoil.lib.progress import ProgressDocument  # noqa: E402
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
