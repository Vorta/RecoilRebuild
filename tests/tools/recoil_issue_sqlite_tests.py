from __future__ import annotations

from contextlib import closing
from copy import deepcopy
from pathlib import Path
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands.workspace_issues import (  # noqa: E402
    empty_ledger,
    normalize_entry,
    validate_issue_document,
)
from _recoil.lib.issue_sqlite import (  # noqa: E402
    ISSUE_APPLICATION_ID,
    ISSUE_DATABASE_SCHEMA_VERSION,
    IssueSQLiteStore,
    create_issue_database,
    export_issue_document,
    open_issue_database,
    read_issue_metadata,
    validate_issue_database,
)
from _recoil.lib.live_progress import (  # noqa: E402
    ConcurrentRevisionUpdate,
    LiveProgressError,
)
from _recoil.lib.progress import normalize_resource_claims  # noqa: E402


def issue(issue_id: str, summary: str) -> dict[str, object]:
    return normalize_entry(
        {
            "severity": "medium",
            "summary": summary,
            "area": "tools",
            "impact": "governed persistence needs exact state",
            "next_action": "validate the SQLite backend",
            "actual": "the JSON store rewrites a whole document",
            "expected": "targeted transactional SQLite updates",
            "repro": "run the issue backend tests",
            "commands": ["python -m unittest tests.tools.recoil_issue_sqlite_tests"],
            "files": ["tools/_recoil/lib/issue_sqlite.py"],
            "tags": ["sqlite", "ledger"],
        },
        issue_id=issue_id,
        kind="tool-error",
    )


def document() -> dict[str, object]:
    value = empty_ledger()
    value["revision"] = 7
    value["id_sequences"] = {"issue": {"20260812": 19}}
    value["issues"] = [
        issue("WSI-20260812-018", "first issue"),
        issue("WSI-20260812-019", "unrelated issue"),
    ]
    claims = normalize_resource_claims(
        [
            {"kind": "lane", "id": "workspace-issue/WSI-20260812-018", "access": "write"},
            {"kind": "issue", "id": "WSI-20260812-018", "access": "read"},
            {"kind": "path", "id": "tools/_recoil/lib/issue_sqlite.py", "access": "write"},
        ]
    )
    value["work_packets"] = [
        {
            "id": "issue:work:sqlite-backend",
            "issue_id": "WSI-20260812-018",
            "state": "active",
            "handoff_role": "recoil_tool_maintainer",
            "scope": "implement the issue SQLite backend",
            "next_command": "python -m unittest tests.tools.recoil_issue_sqlite_tests",
            "allowed_paths": ["tools/_recoil/lib/issue_sqlite.py"],
            "forbidden_paths": ["src"],
            "validation_commands": ["python -m unittest tests.tools.recoil_issue_sqlite_tests"],
            "required_return_fields": ["changed_paths"],
            "resource_claims": claims,
            "reservation_id": "issue:work:sqlite-backend:attempt:1",
            "created": "2026-08-12T00:00:00Z",
            "updated": "2026-08-12T00:00:00Z",
            "semantic_contract_version": 1,
            "scope_versions": [],
            "role_contract_version": 1,
        }
    ]
    value["reservations"] = [
        {
            "id": "issue:work:sqlite-backend:attempt:1",
            "packet_id": "issue:work:sqlite-backend",
            "state": "active",
            "created": "2026-08-12T00:00:00Z",
            "released": None,
            "outcome": None,
            "evidence_ids": ["transcript:test:sqlite"],
            "resource_claims": claims,
            "expires": None,
            "semantic_contract_version": 1,
            "git_workspace_baseline": {
                "schema": "recoil-git-workspace-baseline-v2",
                "packet_id": "issue:work:sqlite-backend",
                "baseline_commit": "opaque-test-commit",
                "branch": "test-packet",
                "writable_paths": ["tools/_recoil/lib/issue_sqlite.py"],
                "status_porcelain_v2": [],
                "ignored_paths": [],
                "git_object_ids_are_opaque": True,
            },
        }
    ]
    validate_issue_document(value)
    return value


class RecoilIssueSQLiteTests(unittest.TestCase):
    def test_missing_database_fails_closed_without_creation(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "issues.sqlite3"
            with self.assertRaisesRegex(LiveProgressError, "missing.*cutover migration"):
                export_issue_document(path)
            self.assertFalse(path.exists())

    def test_explicit_create_round_trips_exact_semantic_document(self) -> None:
        expected = document()
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "issues.sqlite3"
            create_issue_database(path, expected, cutover_pair_id="pair:test:0001")
            actual = export_issue_document(path)
            metadata = read_issue_metadata(path)
            findings = validate_issue_database(
                path, document_validator=validate_issue_document
            )
        self.assertEqual(expected, actual)
        self.assertEqual(ISSUE_APPLICATION_ID, metadata.application_id)
        self.assertEqual(ISSUE_DATABASE_SCHEMA_VERSION, metadata.user_version)
        self.assertEqual(7, metadata.revision)
        self.assertEqual("pair:test:0001", metadata.cutover_pair_id)
        self.assertEqual([], findings)

    def test_special_list_presence_distinguishes_absent_from_explicit_empty(self) -> None:
        expected = document()
        expected["issues"][0].pop("history")
        expected["issues"][1]["history"] = []
        expected["work_packets"][0]["resource_claims"] = []
        expected["reservations"][0].pop("evidence_ids")
        expected["reservations"][0].pop("resource_claims")
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "issues.sqlite3"
            create_issue_database(
                path, expected, cutover_pair_id="pair:test:list-presence"
            )
            actual = export_issue_document(path)
        self.assertEqual(expected, actual)
        self.assertNotIn("history", actual["issues"][0])
        self.assertIn("history", actual["issues"][1])
        self.assertEqual([], actual["issues"][1]["history"])
        self.assertIn("resource_claims", actual["work_packets"][0])
        self.assertEqual([], actual["work_packets"][0]["resource_claims"])
        self.assertNotIn("evidence_ids", actual["reservations"][0])
        self.assertNotIn("resource_claims", actual["reservations"][0])

    def test_connection_policy_is_delete_full_fk_and_ten_second_busy_timeout(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "issues.sqlite3"
            create_issue_database(path, document(), cutover_pair_id="pair:test:policy")
            with closing(open_issue_database(path, writable=True)) as connection:
                self.assertEqual("delete", connection.execute("PRAGMA journal_mode").fetchone()[0])
                self.assertEqual(2, connection.execute("PRAGMA synchronous").fetchone()[0])
                self.assertEqual(1, connection.execute("PRAGMA foreign_keys").fetchone()[0])
                self.assertEqual(10000, connection.execute("PRAGMA busy_timeout").fetchone()[0])

    def test_dry_run_is_transactional_and_apply_preserves_unrelated_row(self) -> None:
        initial = document()
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "issues.sqlite3"
            create_issue_database(path, initial, cutover_pair_id="pair:test:cas")
            store = IssueSQLiteStore(path, validator=validate_issue_document)
            candidate = deepcopy(initial)
            candidate["issues"][0]["summary"] = "narrowly updated first issue"
            with closing(open_issue_database(path)) as connection:
                unrelated_before = connection.execute(
                    "SELECT rowid, scalar_json FROM issues WHERE issue_id = ?",
                    ("WSI-20260812-019",),
                ).fetchone()
                before = tuple(unrelated_before)

            preview = store.commit(candidate, expected_revision=7, apply=False)
            self.assertFalse(preview.applied)
            self.assertEqual(initial, store.load())

            applied = store.commit(candidate, expected_revision=7, apply=True)
            self.assertTrue(applied.applied)
            self.assertEqual(8, applied.revision)
            result = store.load()
            self.assertEqual("narrowly updated first issue", result["issues"][0]["summary"])
            self.assertEqual(19, result["id_sequences"]["issue"]["20260812"])
            with closing(open_issue_database(path)) as connection:
                unrelated_after = connection.execute(
                    "SELECT rowid, scalar_json FROM issues WHERE issue_id = ?",
                    ("WSI-20260812-019",),
                ).fetchone()
                after = tuple(unrelated_after)
            self.assertEqual(before, after)

    def test_expected_revision_cas_is_independent_and_stale_write_fails(self) -> None:
        initial = document()
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "issues.sqlite3"
            create_issue_database(path, initial, cutover_pair_id="pair:test:stale")
            store = IssueSQLiteStore(path, validator=validate_issue_document)
            candidate = deepcopy(initial)
            candidate["issues"][0]["summary"] = "first accepted update"
            store.commit(candidate, expected_revision=7, apply=True)
            with self.assertRaisesRegex(ConcurrentRevisionUpdate, "expected 7, found 8"):
                store.commit(candidate, expected_revision=7, apply=True)
            self.assertEqual(8, read_issue_metadata(path).revision)

    def test_json_file_is_rejected_by_sqlite_open_without_handle_leak(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "issues.json"
            path.write_text("{}\n", encoding="utf-8")
            with self.assertRaisesRegex(LiveProgressError, "legacy JSON"):
                open_issue_database(path)
            path.unlink()
            self.assertFalse(path.exists())


if __name__ == "__main__":
    unittest.main()
