from __future__ import annotations

from contextlib import closing
from pathlib import Path
import sqlite3
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
TOOLS = ROOT / "tools"
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

from _recoil.lib.progress_sqlite import (  # noqa: E402
    APPLICATION_ID,
    ENTITY_COLLECTIONS,
    SEMANTIC_SCHEMA_VERSION,
    USER_VERSION,
    ConcurrentSQLiteProgressUpdate,
    ProgressSQLiteStore,
    read_progress_metadata,
)


def fixture() -> dict[str, object]:
    data: dict[str, object] = {
        "schema_version": SEMANTIC_SCHEMA_VERSION,
        "revision": 17,
        "id_sequences": {"evidence": 4},
        "migration": {"source": "single-agent-test"},
        **{name: {} for name in ENTITY_COLLECTIONS},
    }
    data["binaries"]["recoil"] = {"name": "Recoil.exe"}
    data["physical_blocks"]["recoil:block:0x401000"] = {
        "binary": "recoil",
        "start": "0x401000",
        "end_exclusive": "0x401010",
        "contribution_ids": ["recoil:function:0x401000"],
    }
    data["symbols"]["recoil:function:0x401000"] = {
        "binary": "recoil",
        "kind": "function",
        "address": "0x401000",
        "end_exclusive": "0x401010",
        "pipeline_class": "authored",
        "authored_order_role": "authored-body",
        "physical_block_id": "recoil:block:0x401000",
    }
    return data


class ProgressBackendTests(unittest.TestCase):
    def create(self, root: Path) -> tuple[Path, ProgressSQLiteStore]:
        path = root / "progress.sqlite3"
        store = ProgressSQLiteStore.create_from_mapping(
            path,
            fixture(),
            cutover_pair_id="single-agent-test",
        )
        return path, store

    def test_current_schema_round_trips_without_packet_collections(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path, store = self.create(Path(temporary))
            self.assertEqual(fixture(), store.materialize())
            metadata = read_progress_metadata(path)
            self.assertEqual(SEMANTIC_SCHEMA_VERSION, metadata.schema_version)
            self.assertEqual(USER_VERSION, metadata.user_version)
            with closing(sqlite3.connect(path)) as connection:
                self.assertEqual(
                    APPLICATION_ID,
                    connection.execute("PRAGMA application_id").fetchone()[0],
                )
                tables = {
                    row[0]
                    for row in connection.execute(
                        "SELECT name FROM sqlite_schema WHERE type='table'"
                    )
                }
            self.assertNotIn("work_items", tables)
            self.assertNotIn("reservations", tables)

    def test_materialize_and_revision_vector_are_one_snapshot(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            _, store = self.create(Path(temporary))
            document, vector = store.materialize_with_revision_vector()
            self.assertEqual(17, document["revision"])
            self.assertEqual(17, vector.transaction_revision)
            self.assertEqual(17, vector.semantic_revision)
            self.assertEqual(17, vector.evidence_generation_revision)

    def test_revision_guard_rejects_stale_write(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            _, store = self.create(Path(temporary))
            store.persist_changes(
                upserts={"blockers": {"recoil:blocker:test": {"state": "open"}}},
                expected_revision=17,
                apply=True,
            )
            with self.assertRaises(ConcurrentSQLiteProgressUpdate):
                store.persist_changes(
                    upserts={"blockers": {"recoil:blocker:stale": {"state": "open"}}},
                    expected_revision=17,
                    apply=True,
                )

    def test_integrity_detects_address_index_drift(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path, store = self.create(Path(temporary))
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


if __name__ == "__main__":
    unittest.main()
