from __future__ import annotations

from contextlib import closing
import json
from pathlib import Path
import sqlite3
import sys
import tempfile
import unittest
from unittest.mock import patch


ROOT = Path(__file__).resolve().parents[2]
TOOLS = ROOT / "tools"
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

from _recoil.lib.progress_sqlite import (  # noqa: E402
    APPLICATION_ID,
    BUSY_TIMEOUT_MS,
    ENTITY_COLLECTIONS,
    USER_VERSION,
    ConcurrentSQLiteProgressUpdate,
    ProgressSQLiteError,
    ProgressSQLiteStore,
    import_legacy_json,
    read_progress_metadata,
)
from _recoil.lib.progress import (  # noqa: E402
    ConcurrentProgressUpdate,
    ProgressDocument,
    ProgressStore,
    empty_progress_document,
)


def fixture() -> dict:
    data = {
        "schema_version": 5,
        "revision": 17,
        "id_sequences": {"evidence": 4},
        "migration": {"source": "legacy-json"},
        **{name: {} for name in ENTITY_COLLECTIONS},
    }
    data["binaries"]["recoil"] = {
        "reference": {"image_base": "0x400000", "text_start": "0x401000"}
    }
    data["physical_blocks"]["recoil:block:0x401000"] = {
        "start": "0x401000",
        "end_exclusive": "0x401030",
        "semantic_span_ids": ["recoil:semantic:0x401000-0x401020"],
        "order": {
            "authored": {"result": "passed"},
            "full": {"result": "pending"},
        },
    }
    data["semantic_spans"]["recoil:semantic:0x401000-0x401020"] = {
        "start": "0x401000",
        "end_exclusive": "0x401020",
        "physical_block_id": "recoil:block:0x401000",
        "observation_ids": ["recoil:evidence:r17:000001"],
    }
    data["symbols"]["recoil:function:0x401000"] = {
        "address": "0x401000",
        "physical_block_id": "recoil:block:0x401000",
        "owner_id": "recoil:owner:about-dialog",
        "pipeline_class": "authored",
        "authored_order_role": "authored-body",
        "binary_state": {
            "authored_linked_order": {"result": "passed"},
            "authored_object_order": {"result": "passed"},
            "linked_byte": {"result": "pending"},
        },
    }
    data["owners"]["recoil:owner:about-dialog"] = {
        "entry_ids": ["recoil:function:0x401000"],
        "gates": {"source": "accepted", "byte": "deferred"},
        "tier": "B",
    }
    data["work_items"]["recoil:work:r17:000001"] = {
        "physical_block_id": "recoil:block:0x401000",
        "owner_id": "recoil:owner:about-dialog",
        "state": "ready",
        "reservation": {"state": "active"},
    }
    data["evidence"]["recoil:evidence:r17:000001"] = {
        "kind": "semantic-observation",
        "scope_ids": [
            "recoil:semantic:0x401000-0x401020",
            "recoil:function:0x401000",
        ],
    }
    return data


class ProgressSQLiteBackendTests(unittest.TestCase):
    def create(self, root: Path) -> tuple[Path, ProgressSQLiteStore, dict]:
        data = fixture()
        path = root / "progress.sqlite3"
        store = ProgressSQLiteStore.create_from_mapping(
            path, data, cutover_pair_id="cutover-test-17"
        )
        return path, store, data

    def test_create_materializes_exact_document_and_metadata(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path, store, data = self.create(Path(temporary))
            self.assertEqual(data, store.materialize())
            metadata = read_progress_metadata(path)
            self.assertEqual(5, metadata.schema_version)
            self.assertEqual(17, metadata.revision)
            self.assertEqual("cutover-test-17", metadata.cutover_pair_id)
            self.assertEqual(APPLICATION_ID, metadata.application_id)
            self.assertEqual(USER_VERSION, metadata.user_version)

            with closing(sqlite3.connect(path)) as connection:
                self.assertEqual("delete", connection.execute("PRAGMA journal_mode").fetchone()[0])
                self.assertEqual(APPLICATION_ID, connection.execute("PRAGMA application_id").fetchone()[0])
                self.assertEqual(USER_VERSION, connection.execute("PRAGMA user_version").fetchone()[0])
            with closing(store._connect()) as connection:
                self.assertEqual(2, connection.execute("PRAGMA synchronous").fetchone()[0])
                self.assertEqual(1, connection.execute("PRAGMA foreign_keys").fetchone()[0])
                self.assertEqual(
                    BUSY_TIMEOUT_MS,
                    connection.execute("PRAGMA busy_timeout").fetchone()[0],
                )

    def test_entity_rows_and_normalized_indexes_preserve_distinctions(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path, store, data = self.create(Path(temporary))
            with closing(sqlite3.connect(path)) as connection:
                count = connection.execute("SELECT count(*) FROM entities").fetchone()[0]
                expected = sum(len(data[name]) for name in ENTITY_COLLECTIONS)
                self.assertEqual(expected, count)
                addresses = connection.execute(
                    "SELECT address, typeof(address) FROM address_index "
                    "WHERE collection='physical_blocks' AND entity_id=? ORDER BY address",
                    ("recoil:block:0x401000",),
                ).fetchall()
                self.assertIn((0x401000, "integer"), addresses)
                self.assertIn((0x401030, "integer"), addresses)
                relations = connection.execute(
                    "SELECT source_collection, target_collection, target_entity_id "
                    "FROM relationship_index WHERE source_entity_id=?",
                    ("recoil:function:0x401000",),
                ).fetchall()
                self.assertIn(
                    ("symbols", "physical_blocks", "recoil:block:0x401000"), relations
                )
                self.assertIn(("symbols", "owners", "recoil:owner:about-dialog"), relations)
            self.assertTrue(store.validate_integrity().ok)
            self.assertEqual(
                [("symbols", "recoil:function:0x401000")],
                store.query_entity_ids(
                    collection="symbols",
                    pipeline_class="authored",
                    authored_order_role="authored-body",
                    physical_block_id="recoil:block:0x401000",
                    address_at_or_after=0x401000,
                ),
            )
            self.assertEqual(
                [("work_items", "recoil:work:r17:000001")],
                store.query_entity_ids(
                    collection="work_items",
                    work_state="ready",
                    reservation_state="active",
                ),
            )

    def test_dry_run_rolls_back_entities_indexes_and_revision(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            _, store, before = self.create(Path(temporary))
            proposed = json.loads(json.dumps(before))
            proposed["owners"]["recoil:owner:about-dialog"]["note"] = "dry run only"
            result = store.commit(proposed, expected_revision=17, apply=False)
            self.assertFalse(result.applied)
            self.assertEqual(18, result.revision)
            self.assertEqual(1, result.upserted_entities)
            self.assertEqual(17, store.read_revision())
            self.assertEqual(before, store.materialize())

    def test_apply_uses_expected_revision_cas_and_bounded_rows(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            _, store, _ = self.create(Path(temporary))
            result = store.persist_changes(
                upserts={
                    "work_items": {
                        "recoil:work:r17:000001": {
                            "physical_block_id": "recoil:block:0x401000",
                            "owner_id": "recoil:owner:about-dialog",
                            "state": "active",
                            "reservation": {"state": "active"},
                        }
                    }
                },
                expected_revision=17,
                apply=True,
            )
            self.assertEqual(1, result.upserted_entities)
            self.assertEqual(0, result.deleted_entities)
            self.assertEqual(18, store.read_revision())
            self.assertEqual(
                "active",
                store.materialize()["work_items"]["recoil:work:r17:000001"]["state"],
            )
            with self.assertRaises(ConcurrentSQLiteProgressUpdate):
                store.persist_changes(expected_revision=17, apply=True)

    def test_read_only_and_missing_open_fail_closed(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            path, _, data = self.create(root)
            read_only = ProgressSQLiteStore(path, read_only=True)
            self.assertEqual(data, read_only.materialize())
            with self.assertRaises(ProgressSQLiteError):
                read_only.persist_changes(expected_revision=17, apply=True)
            missing = root / "missing.sqlite3"
            with self.assertRaises(ProgressSQLiteError):
                ProgressSQLiteStore(missing)
            self.assertFalse(missing.exists())

    def test_wrong_database_is_rejected_without_stamping_header(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "unrelated.sqlite3"
            with closing(sqlite3.connect(path)) as connection:
                connection.execute("CREATE TABLE unrelated(value TEXT)")
            with self.assertRaises(ProgressSQLiteError):
                ProgressSQLiteStore(path)
            with closing(sqlite3.connect(path)) as connection:
                self.assertEqual(0, connection.execute("PRAGMA application_id").fetchone()[0])
                self.assertEqual(0, connection.execute("PRAGMA user_version").fetchone()[0])

    def test_unsupported_semantic_schema_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            data = fixture()
            data["schema_version"] = 4
            with self.assertRaisesRegex(ProgressSQLiteError, "schema_version must be 5"):
                ProgressSQLiteStore.create_from_mapping(
                    Path(temporary) / "progress.sqlite3",
                    data,
                    cutover_pair_id="wrong-schema",
                )

    def test_integrity_validation_detects_index_drift(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path, store, _ = self.create(Path(temporary))
            with closing(sqlite3.connect(path)) as connection:
                connection.execute(
                    "DELETE FROM address_index WHERE collection='symbols' AND entity_id=?",
                    ("recoil:function:0x401000",),
                )
                connection.commit()
            validation = store.validate_integrity()
            self.assertFalse(validation.ok)
            self.assertIn(
                "address_index does not match authoritative entity payloads",
                validation.errors,
            )

    def test_legacy_json_import_and_referenced_delete_fail_closed(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            data = fixture()
            evidence_id = "recoil:evidence:r725:000001"
            data["evidence"][evidence_id] = {
                "kind": "semantic-observation",
                "freshness": "historical",
                "validation_mode": "historical-observation",
                "scope_ids": ["recoil:owner:about-dialog"],
            }
            source = root / "progress.json"
            source.write_text(json.dumps(data), encoding="utf-8")
            path = root / "progress.sqlite3"
            store = import_legacy_json(source, path, cutover_pair_id="legacy-pair")
            self.assertEqual(data, store.materialize())
            with self.assertRaises(ProgressSQLiteError):
                store.persist_changes(
                    deletes={"owners": ["recoil:owner:about-dialog"]},
                    expected_revision=17,
                    apply=True,
                )
            self.assertEqual(17, store.read_revision())
            with closing(store._connect()) as connection:
                inbound_sources = [
                    tuple(row)
                    for row in connection.execute(
                        "SELECT source_collection, source_entity_id "
                        "FROM relationship_index WHERE target_collection='owners' "
                        "AND target_entity_id=? ORDER BY source_collection, source_entity_id",
                        ("recoil:owner:about-dialog",),
                    )
                ]
            self.assertIn(("evidence", evidence_id), inbound_sources)
            self.assertIn(("symbols", "recoil:function:0x401000"), inbound_sources)
            self.assertIn(("work_items", "recoil:work:r17:000001"), inbound_sources)

    def test_retiring_target_preserves_historical_evidence_without_stale_index(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            data = fixture()
            retired_owner_id = "recoil:owner:hud-retired"
            evidence_id = "recoil:evidence:r725:000001"
            data["owners"][retired_owner_id] = {
                "entry_ids": [],
                "gates": {"source": "retired"},
            }
            data["evidence"][evidence_id] = {
                "kind": "semantic-observation",
                "freshness": "historical",
                "validation_mode": "historical-observation",
                "scope_ids": [retired_owner_id, "recoil:function:0x401000"],
            }
            path = Path(temporary) / "progress.sqlite3"
            store = ProgressSQLiteStore.create_from_mapping(
                path, data, cutover_pair_id="historical-evidence-retirement"
            )

            result = store.persist_changes(
                deletes={"owners": [retired_owner_id]},
                expected_revision=17,
                apply=True,
            )

            self.assertTrue(result.applied)
            self.assertEqual(18, result.revision)
            self.assertEqual(1, result.deleted_entities)
            materialized = store.materialize()
            self.assertNotIn(retired_owner_id, materialized["owners"])
            self.assertEqual(
                [retired_owner_id, "recoil:function:0x401000"],
                materialized["evidence"][evidence_id]["scope_ids"],
            )
            with closing(store._connect()) as connection:
                self.assertEqual(
                    [],
                    connection.execute(
                        "SELECT source_collection, source_entity_id "
                        "FROM relationship_index WHERE target_entity_id=?",
                        (retired_owner_id,),
                    ).fetchall(),
                )
                self.assertEqual(
                    [("evidence", evidence_id)],
                    [
                        tuple(row)
                        for row in connection.execute(
                            "SELECT source_collection, source_entity_id "
                            "FROM relationship_index WHERE source_collection='evidence' "
                            "AND source_entity_id=? AND target_entity_id=?",
                            (evidence_id, "recoil:function:0x401000"),
                        )
                    ],
                )
            self.assertTrue(store.validate_integrity().ok)

    def test_progress_document_and_store_compatibility_adapter(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "adapter.sqlite3"
            data = empty_progress_document()
            ProgressSQLiteStore.create_from_mapping(
                path, data, cutover_pair_id="adapter-test"
            )
            self.assertEqual(data, ProgressDocument.load(path).data)

            store = ProgressStore(path)
            self.assertEqual(0, store.read_revision())
            proposed = store.load()
            proposed.data["migration"]["adapter"] = "bounded-sqlite"
            dry_run = store.commit(proposed, expected_revision=0, apply=False)
            self.assertFalse(dry_run.applied)
            self.assertEqual(0, store.read_revision())

            applied = store.commit(proposed, expected_revision=0, apply=True)
            self.assertTrue(applied.applied)
            self.assertEqual(1, store.read_revision())
            self.assertEqual(
                "bounded-sqlite", store.load().data["migration"]["adapter"]
            )
            with self.assertRaises(ConcurrentProgressUpdate):
                store.commit(proposed, expected_revision=0, apply=True)

    def test_progress_store_revision_read_does_not_materialize(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path, _, _ = self.create(Path(temporary))
            with patch.object(
                ProgressSQLiteStore,
                "materialize",
                side_effect=AssertionError("revision read materialized entities"),
            ):
                self.assertEqual(17, ProgressStore(path).read_revision())


if __name__ == "__main__":
    unittest.main()
