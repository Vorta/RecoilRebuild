from __future__ import annotations

from contextlib import closing
from pathlib import Path
import sqlite3
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from _recoil.lib.issue_sqlite import (  # noqa: E402
    IssueSQLiteStore,
    create_issue_database,
    export_issue_document,
    validate_issue_database,
)
from _recoil.lib.live_progress import ISSUE_LEDGER_VERSION  # noqa: E402


class IssueSQLiteTests(unittest.TestCase):
    def test_schema_contains_only_issue_authority_tables(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "issues.sqlite3"
            document = {
                "version": ISSUE_LEDGER_VERSION,
                "revision": 0,
                "id_sequences": {},
                "issues": [],
            }
            create_issue_database(path, document, cutover_pair_id="serial-test")
            with closing(sqlite3.connect(path)) as connection:
                tables = {
                    str(row[0])
                    for row in connection.execute(
                        "SELECT name FROM sqlite_master WHERE type='table'"
                    )
                }
            self.assertEqual({"metadata", "issues", "id_sequences"}, tables)
            self.assertEqual([], validate_issue_database(path))
            self.assertEqual(document, export_issue_document(path))

    def test_commit_is_revision_guarded(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "issues.sqlite3"
            document = {
                "version": ISSUE_LEDGER_VERSION,
                "revision": 0,
                "id_sequences": {},
                "issues": [],
            }
            create_issue_database(path, document, cutover_pair_id="serial-test")
            result = IssueSQLiteStore(path).commit(
                document,
                expected_revision=0,
                apply=True,
            )
            self.assertTrue(result.applied)
            self.assertEqual(1, export_issue_document(path)["revision"])


if __name__ == "__main__":
    unittest.main()
