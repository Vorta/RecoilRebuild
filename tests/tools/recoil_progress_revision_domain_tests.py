from __future__ import annotations

from pathlib import Path
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from _recoil.lib.progress import ProgressDocument  # noqa: E402
from _recoil.lib.progress_sqlite import ProgressSQLiteStore  # noqa: E402


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


if __name__ == "__main__":
    unittest.main()
