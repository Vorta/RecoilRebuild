from __future__ import annotations

from dataclasses import dataclass, replace
import json
from pathlib import Path
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands.ledger_sqlite_migration import (  # noqa: E402
    LedgerMigrationContractError,
    LedgerMigrationValidationError,
    MigrationRequest,
    _Backends,
    migrate_ledgers_sqlite,
)
from _recoil.commands.workspace_issues import empty_ledger  # noqa: E402
from _recoil.lib.progress import empty_progress_document  # noqa: E402


@dataclass(frozen=True)
class _Metadata:
    revision: int
    cutover_pair_id: str
    application_id: int = 1
    user_version: int = 1
    schema_version: int = 1


@dataclass(frozen=True)
class _Validation:
    ok: bool = True
    integrity_check: tuple[str, ...] = ("ok",)
    foreign_key_violations: tuple[object, ...] = ()


class _ProgressStore:
    def __init__(self, path: Path) -> None:
        self.path = path

    def _payload(self) -> dict[str, object]:
        return json.loads(self.path.read_text(encoding="utf-8"))

    def materialize(self) -> dict[str, object]:
        return self._payload()["document"]

    def metadata(self) -> _Metadata:
        payload = self._payload()
        return _Metadata(payload["document"]["revision"], payload["cutover_pair_id"])

    def validate_integrity(self) -> _Validation:
        payload = self._payload()
        return _Validation(payload.get("valid", True))


class _FixtureBackends:
    """A persistent test adapter for the cutover state machine.

    The production integration uses real SQLite backends.  This fixture keeps the
    state-machine tests small and makes atomic replacement/interruption observable
    without reaching outside each TemporaryDirectory.
    """

    def create_progress(
        self,
        path: Path,
        source: dict[str, object],
        *,
        cutover_pair_id: str,
        overwrite: bool,
    ) -> None:
        if path.exists() and not overwrite:
            raise RuntimeError("destination exists")
        path.write_text(
            json.dumps({"document": source, "cutover_pair_id": cutover_pair_id}),
            encoding="utf-8",
        )

    def open_progress(self, path: Path) -> _ProgressStore:
        if not path.exists():
            raise FileNotFoundError(path)
        return _ProgressStore(path)

    def create_issues(
        self,
        path: Path,
        source: dict[str, object],
        *,
        cutover_pair_id: str,
        overwrite: bool,
    ) -> None:
        if path.exists() and not overwrite:
            raise RuntimeError("destination exists")
        path.write_text(
            json.dumps({"document": source, "cutover_pair_id": cutover_pair_id}),
            encoding="utf-8",
        )

    def export_issues(self, path: Path) -> dict[str, object]:
        return json.loads(path.read_text(encoding="utf-8"))["document"]

    def metadata_issues(self, path: Path) -> _Metadata:
        payload = json.loads(path.read_text(encoding="utf-8"))
        return _Metadata(payload["document"]["revision"], payload["cutover_pair_id"])

    def validate_issues(self, path: Path, *, document_validator=None) -> list[str]:
        payload = json.loads(path.read_text(encoding="utf-8"))
        if not payload.get("valid", True):
            return ["integrity_check failed"]
        if document_validator is not None:
            document_validator(payload["document"])
        return []

    def api(self) -> _Backends:
        return _Backends(
            self.create_progress,
            self.open_progress,
            self.create_issues,
            self.export_issues,
            self.metadata_issues,
            self.validate_issues,
        )


class RecoilLedgerSQLiteMigrationTests(unittest.TestCase):
    def fixture(self, root: Path, *, progress_revision: int = 7, issue_revision: int = 4):
        progress = empty_progress_document()
        progress["revision"] = progress_revision
        issues = empty_ledger()
        issues["revision"] = issue_revision
        progress_json = root / "progress.json"
        issues_json = root / "issues.json"
        progress_json.write_text(json.dumps(progress), encoding="utf-8")
        issues_json.write_text(json.dumps(issues), encoding="utf-8")
        request = MigrationRequest(
            progress_json,
            issues_json,
            root / "progress.sqlite3",
            root / "issues.sqlite3",
            progress_revision,
            issue_revision,
            False,
        )
        return request, progress, issues

    def test_dry_run_validates_both_imports_without_install_or_json_removal(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            request, progress, issues = self.fixture(root)
            result = migrate_ledgers_sqlite(
                request, backends=_FixtureBackends().api()
            )

            self.assertFalse(result["applied"])
            self.assertFalse(result["resumed"])
            self.assertTrue(result["parity"]["semantic_document_equal"])
            self.assertTrue(result["parity"]["scheduler_equal"])
            self.assertEqual(progress, json.loads(request.progress_json.read_text()))
            self.assertEqual(issues, json.loads(request.issues_json.read_text()))
            self.assertFalse(request.progress_db.exists())
            self.assertFalse(request.issues_db.exists())
            self.assertFalse(Path(f"{request.progress_db}.migration-tmp").exists())
            self.assertFalse(Path(f"{request.issues_db}.migration-tmp").exists())

    def test_apply_installs_matched_pair_then_removes_both_json_sources(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            request, progress, issues = self.fixture(root)
            request = replace(request, apply=True)
            backend = _FixtureBackends()
            result = migrate_ledgers_sqlite(request, backends=backend.api())

            self.assertTrue(result["applied"])
            self.assertEqual(
                {"progress": True, "issues": True}, result["legacy_json_removed"]
            )
            self.assertFalse(request.progress_json.exists())
            self.assertFalse(request.issues_json.exists())
            progress_store = backend.open_progress(request.progress_db)
            self.assertEqual(progress, progress_store.materialize())
            self.assertEqual(issues, backend.export_issues(request.issues_db))
            self.assertEqual(
                progress_store.metadata().cutover_pair_id,
                backend.metadata_issues(request.issues_db).cutover_pair_id,
            )

    def test_active_reservation_refuses_cutover_before_any_database_is_created(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            request, _progress, _issues = self.fixture(root)
            issues = json.loads(request.issues_json.read_text(encoding="utf-8"))
            issues["issues"] = [{
                "id": "WSI-20260812-001", "status": "in-progress",
                "kind": "tool-error", "severity": "high",
                "created": "2026-08-12T00:00:00Z", "updated": "2026-08-12T00:00:00Z",
                "summary": "test", "area": "tools", "impact": "test",
                "next_action": "finish", "actual": "active", "repro": "test",
            }]
            claim = {"kind": "path", "id": "tools/test.py", "access": "write"}
            issues["work_packets"] = [{
                "id": "issue:work:test", "issue_id": "WSI-20260812-001",
                "handoff_role": "recoil_tool_maintainer", "state": "active",
                "semantic_contract_version": 1, "scope": "test", "next_command": "test",
                "allowed_paths": ["tools/test.py"], "forbidden_paths": ["src"],
                "validation_commands": ["test"], "required_return_fields": ["result"],
                "resource_claims": [claim], "reservation_id": "issue:reservation:test",
            }]
            issues["reservations"] = [{
                "id": "issue:reservation:test", "packet_id": "issue:work:test",
                "state": "active", "semantic_contract_version": 1,
                "resource_claims": [claim], "evidence_ids": [],
                "released": None, "outcome": None,
                "git_workspace_baseline": {
                    "schema": "recoil-git-workspace-baseline-v1",
                    "packet_id": "issue:work:test",
                    "baseline_commit": "opaque-test-commit",
                    "branch": "test-packet",
                    "writable_paths": ["tools/test.py"],
                    "git_object_ids_are_opaque": True,
                },
            }]
            request.issues_json.write_text(json.dumps(issues), encoding="utf-8")

            with self.assertRaisesRegex(LedgerMigrationContractError, "active reservations"):
                migrate_ledgers_sqlite(request, backends=_FixtureBackends().api())
            self.assertFalse(request.progress_db.exists())
            self.assertFalse(request.issues_db.exists())

    def test_interruption_after_first_install_is_resumed_from_unchanged_json(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            request, progress, issues = self.fixture(root)
            request = replace(request, apply=True)
            backend = _FixtureBackends()

            def interrupt() -> None:
                raise RuntimeError("injected interruption")

            with self.assertRaisesRegex(RuntimeError, "injected interruption"):
                migrate_ledgers_sqlite(
                    request, backends=backend.api(), _after_first_install=interrupt
                )
            self.assertTrue(request.progress_db.exists())
            self.assertFalse(request.issues_db.exists())
            self.assertTrue(request.progress_json.exists())
            self.assertTrue(request.issues_json.exists())

            result = migrate_ledgers_sqlite(request, backends=backend.api())
            self.assertTrue(result["applied"])
            self.assertTrue(result["resumed"])
            self.assertEqual(progress, backend.open_progress(request.progress_db).materialize())
            self.assertEqual(issues, backend.export_issues(request.issues_db))
            self.assertFalse(request.progress_json.exists())
            self.assertFalse(request.issues_json.exists())

    def test_mismatched_existing_cutover_pair_fails_closed(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            request, progress, issues = self.fixture(root)
            backend = _FixtureBackends()
            backend.create_progress(
                request.progress_db, progress, cutover_pair_id="pair-a", overwrite=False
            )
            backend.create_issues(
                request.issues_db, issues, cutover_pair_id="pair-b", overwrite=False
            )
            with self.assertRaisesRegex(
                LedgerMigrationValidationError, "cutover pair mismatch"
            ):
                migrate_ledgers_sqlite(request, backends=backend.api())
            self.assertTrue(request.progress_json.exists())
            self.assertTrue(request.issues_json.exists())

    def test_cleanup_interruption_resumes_when_one_json_source_is_already_absent(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            request, progress, issues = self.fixture(root)
            backend = _FixtureBackends()
            pair_id = "pair-cleanup-resume"
            backend.create_progress(
                request.progress_db, progress, cutover_pair_id=pair_id, overwrite=False
            )
            backend.create_issues(
                request.issues_db, issues, cutover_pair_id=pair_id, overwrite=False
            )
            request.progress_json.unlink()
            request = replace(request, apply=True)

            result = migrate_ledgers_sqlite(request, backends=backend.api())
            self.assertTrue(result["applied"])
            self.assertTrue(result["resumed"])
            self.assertFalse(request.issues_json.exists())
            self.assertTrue(request.progress_db.exists())
            self.assertTrue(request.issues_db.exists())

    def test_missing_json_does_not_silently_initialize_empty_databases(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            request = MigrationRequest(
                root / "progress.json", root / "issues.json",
                root / "progress.sqlite3", root / "issues.sqlite3",
                0, 0, True,
            )
            with self.assertRaisesRegex(LedgerMigrationContractError, "both guarded JSON"):
                migrate_ledgers_sqlite(request, backends=_FixtureBackends().api())
            self.assertFalse(request.progress_db.exists())
            self.assertFalse(request.issues_db.exists())

    def test_revision_guard_is_checked_before_import(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            request, _progress, _issues = self.fixture(root)
            request = MigrationRequest(
                request.progress_json, request.issues_json,
                request.progress_db, request.issues_db,
                request.expected_progress_revision + 1,
                request.expected_issues_revision,
                False,
            )
            with self.assertRaisesRegex(LedgerMigrationContractError, "progress revision changed"):
                migrate_ledgers_sqlite(request, backends=_FixtureBackends().api())
            self.assertFalse(request.progress_db.exists())
            self.assertFalse(request.issues_db.exists())

    def test_real_sqlite_backends_apply_and_reopen_the_installed_pair(self) -> None:
        from _recoil.lib.issue_sqlite import export_issue_document, read_issue_metadata
        from _recoil.lib.progress_sqlite import USER_VERSION, ProgressSQLiteStore

        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            request, progress, issues = self.fixture(root)
            request = replace(request, apply=True)

            result = migrate_ledgers_sqlite(request)

            progress_store = ProgressSQLiteStore(request.progress_db, read_only=True)
            progress_metadata = progress_store.metadata()
            self.assertTrue(result["applied"])
            self.assertEqual(progress, progress_store.materialize())
            self.assertEqual(issues, export_issue_document(request.issues_db))
            self.assertEqual(
                progress_metadata.cutover_pair_id,
                read_issue_metadata(request.issues_db).cutover_pair_id,
            )
            self.assertEqual(USER_VERSION, progress_metadata.user_version)
            self.assertEqual(
                {
                    "transaction_revision": request.expected_progress_revision,
                    "semantic_revision": request.expected_progress_revision,
                    "evidence_generation_revision": request.expected_progress_revision,
                    "scheduler_revision": request.expected_progress_revision,
                },
                progress_metadata.revision_vector.to_dict(),
            )
            self.assertFalse(request.progress_json.exists())
            self.assertFalse(request.issues_json.exists())


if __name__ == "__main__":
    unittest.main()
