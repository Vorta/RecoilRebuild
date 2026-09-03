from __future__ import annotations

from pathlib import Path
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from _recoil.lib.progress import ProgressDocument  # noqa: E402
from _recoil.lib.progress_sqlite import (  # noqa: E402
    DELETE_FACET,
    ProgressSQLiteStore,
)


class ProgressRevisionDomainTests(unittest.TestCase):
    def test_vector_has_exactly_three_domains(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "progress.sqlite3"
            store = ProgressSQLiteStore.create_from_mapping(
                path,
                ProgressDocument.empty().data,
                cutover_pair_id="serial-test",
            )
            self.assertEqual(
                {
                    "transaction_revision": 0,
                    "semantic_revision": 0,
                    "evidence_generation_revision": 0,
                },
                store.read_revision_vector().to_dict(),
            )
            candidate = store.materialize()
            candidate["id_sequences"] = {"test": 1}
            commit = store.commit(candidate, expected_revision=0, apply=True)
            self.assertEqual(
                {
                    "transaction_revision": 1,
                    "semantic_revision": 1,
                    "evidence_generation_revision": 1,
                },
                commit.revision_vector.to_dict(),
            )

    def test_scoped_commit_can_delete_one_complete_entity(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "progress.sqlite3"
            source = ProgressDocument.empty().data
            source["evidence"] = {
                "recoil:evidence:unit": {
                    "kind": "unit-evidence",
                    "scope_ids": [],
                }
            }
            store = ProgressSQLiteStore.create_from_mapping(
                path,
                source,
                cutover_pair_id="scoped-delete-test",
            )

            commit = store.persist_scoped_changes(
                expected_domain_revisions={
                    "semantic": 0,
                    "evidence_generation": 0,
                },
                entity_patches={
                    "evidence": {
                        "recoil:evidence:unit": {"": DELETE_FACET}
                    }
                },
                increment_domains={"semantic", "evidence_generation"},
                apply=True,
            )

            self.assertEqual(0, commit.upserted_entities)
            self.assertEqual(1, commit.deleted_entities)
            self.assertEqual({}, store.materialize()["evidence"])
            self.assertEqual(
                {
                    "transaction_revision": 1,
                    "semantic_revision": 1,
                    "evidence_generation_revision": 1,
                },
                commit.revision_vector.to_dict(),
            )


if __name__ == "__main__":
    unittest.main()
