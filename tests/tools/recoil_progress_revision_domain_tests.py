from __future__ import annotations

from contextlib import redirect_stderr, redirect_stdout
from contextlib import closing
from copy import deepcopy
import io
import json
import os
from pathlib import Path
import sqlite3
import subprocess
import sys
import tempfile
import unittest
from unittest import mock


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands import progress_cli, workspace_issues  # noqa: E402
from _recoil.lib.progress import (  # noqa: E402
    bind_progress_packet_native_git,
    empty_progress_document,
)
from _recoil.lib.progress_sqlite import (  # noqa: E402
    LEGACY_USER_VERSION,
    USER_VERSION,
    ConcurrentSQLiteProgressUpdate,
    ProgressSQLiteError,
    ProgressSQLiteStore,
    read_progress_revision_vector,
)
from _recoil.lib.worktree_control import (  # noqa: E402
    CANONICAL_ROOT_ENV,
    VALIDATION_READ_ONLY_AUTHORITY_ENV,
)


class RecoilProgressRevisionDomainTests(unittest.TestCase):
    @staticmethod
    def create_native_git_packet(
        root: Path,
        *,
        revision: int = 7,
        work_id: str = "recoil:work:native-git-close-r7",
    ) -> tuple[Path, ProgressSQLiteStore, str]:
        document = empty_progress_document()
        document["revision"] = revision
        document["work_items"][work_id] = {
            "state": "active",
            "packet_type": "byte-edit-v1",
            "packet_contract_version": 4,
            "phase": "authored-byte-match",
            "lane": "authored",
            "nonaccepting": False,
            "acceptance_eligible": True,
            "resource_claims": [
                {"kind": "path", "id": "inside.txt", "access": "write"}
            ],
            "reservation": {
                "id": f"{work_id}:attempt:1",
                "state": "active",
            },
        }
        bind_progress_packet_native_git(
            document,
            work_id=work_id,
            baseline={
                "packet_id": work_id,
                "writable_paths": ["inside.txt"],
            },
            association={
                "authority": "progress",
                "packet_id": work_id,
                "external_build_root": str(root / "build-root"),
            },
            build_root_marker={"fixture": True},
            operation_id=f"{work_id}:allocation:1",
        )
        path = root / "progress.sqlite3"
        store = ProgressSQLiteStore.create_from_mapping(
            path, document, cutover_pair_id="native-git-close-test"
        )
        return path, store, work_id

    @staticmethod
    def concurrent_writers(
        path: Path, rows: tuple[tuple[str, str, str], ...]
    ) -> list[dict[str, object]]:
        script = r'''
import json
from pathlib import Path
import sys
from _recoil.lib.progress_sqlite import ProgressSQLiteStore

path = Path(sys.argv[1])
domain, pointer, value = sys.argv[2:5]
sys.stdin.readline()
try:
    result = ProgressSQLiteStore(path).persist_scoped_changes(
        expected_domain_revisions={domain: 7},
        entity_patches={
            "symbols": {
                "recoil:function:0x401000": {pointer: value}
            }
        },
        apply=True,
    )
except Exception as exc:
    print(json.dumps({"status": "error", "type": type(exc).__name__, "message": str(exc)}))
else:
    print(json.dumps({"status": "ok", "revisions": result.revision_vector.to_dict()}))
'''
        environment = os.environ.copy()
        existing = environment.get("PYTHONPATH", "")
        environment["PYTHONPATH"] = os.pathsep.join(
            [str(REPO_ROOT / "tools"), *([existing] if existing else [])]
        )
        processes = [
            subprocess.Popen(
                [sys.executable, "-B", "-c", script, str(path), *row],
                cwd=REPO_ROOT,
                env=environment,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                encoding="utf-8",
            )
            for row in rows
        ]
        for process in processes:
            assert process.stdin is not None
            process.stdin.write("start\n")
            process.stdin.flush()
            process.stdin.close()
        results: list[dict[str, object]] = []
        for process in processes:
            process.wait(timeout=30)
            assert process.stdout is not None and process.stderr is not None
            stdout = process.stdout.read().strip()
            stderr = process.stderr.read().strip()
            process.stdout.close()
            process.stderr.close()
            if process.returncode != 0:
                raise AssertionError(
                    f"writer exited {process.returncode}: stdout={stdout!r} stderr={stderr!r}"
                )
            results.append(json.loads(stdout))
        return results

    @staticmethod
    def structured_observation(path: Path) -> dict[str, object]:
        store = ProgressSQLiteStore(path, read_only=True)
        metadata = store.metadata()
        with closing(sqlite3.connect(f"file:{path}?mode=ro", uri=True)) as connection:
            user_version = connection.execute("PRAGMA user_version").fetchone()[0]
            schema_version = connection.execute("PRAGMA schema_version").fetchone()[0]
            integrity = [row[0] for row in connection.execute("PRAGMA integrity_check")]
            foreign_keys = [tuple(row) for row in connection.execute("PRAGMA foreign_key_check")]
            table_names = [
                row[0] for row in connection.execute(
                    "SELECT name FROM sqlite_schema WHERE type='table' ORDER BY name"
                )
            ]
            row_counts = {
                name: connection.execute(
                    f'SELECT COUNT(*) FROM "{name.replace(chr(34), chr(34) * 2)}"'
                ).fetchone()[0]
                for name in table_names
            }
            metadata_cursor = connection.execute(
                "SELECT * FROM metadata WHERE singleton=1"
            )
            metadata_columns = [
                str(item[0]) for item in metadata_cursor.description or ()
            ]
            metadata_rows = [
                dict(zip(metadata_columns, tuple(row)))
                for row in metadata_cursor.fetchall()
            ]
            selected_top_level = [
                tuple(row) for row in connection.execute(
                    "SELECT key, payload FROM top_level_values "
                    "WHERE key IN ('revision', 'migration', 'schema_version') "
                    "ORDER BY key"
                )
            ]
            selected_entities = [
                tuple(row) for row in connection.execute(
                    "SELECT collection, entity_id, payload FROM entities "
                    "WHERE collection IN ('symbols', 'work_items', 'reservations') "
                    "ORDER BY collection, entity_id"
                )
            ]
        if integrity != ["ok"] or foreign_keys:
            raise AssertionError(
                f"structured SQLite observation is unhealthy: {integrity!r}, {foreign_keys!r}"
            )
        return {
            "user_version": user_version,
            "schema_version": schema_version,
            "logical_schema_version": metadata.schema_version,
            "revisions": store.read_revision_vector().to_dict(),
            "row_counts": row_counts,
            "selected_rows": {
                "metadata": metadata_rows,
                "top_level_values": selected_top_level,
                "entities": selected_entities,
            },
            "integrity_check": integrity,
            "foreign_key_check": foreign_keys,
        }

    def create(self, root: Path, *, revision: int = 7) -> tuple[Path, ProgressSQLiteStore]:
        document = empty_progress_document()
        document["revision"] = revision
        document["symbols"]["recoil:function:0x401000"] = {
            "address": "0x401000",
            "semantic": {"state": "pending"},
            "scheduler": {"state": "ready"},
        }
        path = root / "progress.sqlite3"
        return path, ProgressSQLiteStore.create_from_mapping(
            path, document, cutover_pair_id="revision-domain-test"
        )

    @staticmethod
    def downgrade_storage_fixture_to_v1(path: Path) -> None:
        with closing(sqlite3.connect(path)) as connection:
            connection.executescript(
                """
                ALTER TABLE metadata RENAME TO metadata_v2;
                CREATE TABLE metadata (
                    singleton INTEGER PRIMARY KEY CHECK (singleton = 1),
                    schema_version INTEGER NOT NULL CHECK (schema_version > 0),
                    revision INTEGER NOT NULL CHECK (revision >= 0),
                    cutover_pair_id TEXT NOT NULL CHECK (length(cutover_pair_id) > 0)
                ) STRICT;
                INSERT INTO metadata(singleton, schema_version, revision, cutover_pair_id)
                    SELECT singleton, schema_version, revision, cutover_pair_id
                    FROM metadata_v2;
                DROP TABLE metadata_v2;
                """
            )
            connection.execute(f"PRAGMA user_version={LEGACY_USER_VERSION}")
            connection.commit()

    def test_new_store_seeds_all_domains_from_document_revision(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path, store = self.create(Path(temporary), revision=7)

            vector = store.read_revision_vector()

            self.assertEqual(
                {
                    "transaction_revision": 7,
                    "semantic_revision": 7,
                    "evidence_generation_revision": 7,
                    "scheduler_revision": 7,
                },
                vector.to_dict(),
            )
            self.assertEqual(vector, read_progress_revision_vector(path))
            self.assertEqual(USER_VERSION, store.metadata().user_version)
            self.assertEqual(7, store.metadata().revision)

    def test_v1_read_is_nonmutating_and_explicit_migration_seeds_domains(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path, _store = self.create(Path(temporary), revision=11)
            self.downgrade_storage_fixture_to_v1(path)
            before = self.structured_observation(path)

            read_only = ProgressSQLiteStore(path, read_only=True)
            self.assertEqual((11, 11, 11, 11), tuple(read_only.read_revision_vector().__dict__.values()))
            self.assertEqual(before, self.structured_observation(path))
            self.assertEqual(["ok"], before["integrity_check"])
            self.assertEqual([], before["foreign_key_check"])
            self.assertEqual(LEGACY_USER_VERSION, read_only.metadata().user_version)

            writable = ProgressSQLiteStore(path)
            projected = writable.migrate_revision_domains(
                expected_revision=11, apply=False
            )
            self.assertEqual(11, projected.semantic_revision)
            with closing(sqlite3.connect(path)) as connection:
                self.assertEqual(
                    LEGACY_USER_VERSION,
                    connection.execute("PRAGMA user_version").fetchone()[0],
                )

            writable.migrate_revision_domains(expected_revision=11, apply=True)
            metadata = writable.metadata()
            self.assertEqual(USER_VERSION, metadata.user_version)
            self.assertEqual(11, metadata.revision)
            self.assertEqual(11, metadata.semantic_revision)
            self.assertEqual(11, metadata.evidence_generation_revision)
            self.assertEqual(11, metadata.scheduler_revision)

    def test_scoped_writes_increment_only_owning_domains_and_preserve_other_facets(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            _path, store = self.create(Path(temporary), revision=7)
            symbol_id = "recoil:function:0x401000"

            scheduler_result = store.persist_scoped_changes(
                expected_domain_revisions={"scheduler": 7},
                entity_patches={
                    "symbols": {symbol_id: {"/scheduler/state": "active"}}
                },
                apply=True,
            )
            self.assertEqual(
                {
                    "transaction_revision": 8,
                    "semantic_revision": 7,
                    "evidence_generation_revision": 7,
                    "scheduler_revision": 8,
                },
                scheduler_result.revision_vector.to_dict(),
            )

            semantic_result = store.persist_scoped_changes(
                expected_domain_revisions={"semantic": 7},
                entity_patches={
                    "symbols": {symbol_id: {"/semantic/state": "accepted"}}
                },
                apply=True,
            )
            self.assertEqual(
                {
                    "transaction_revision": 9,
                    "semantic_revision": 8,
                    "evidence_generation_revision": 7,
                    "scheduler_revision": 8,
                },
                semantic_result.revision_vector.to_dict(),
            )
            symbol = store.materialize()["symbols"][symbol_id]
            self.assertEqual("active", symbol["scheduler"]["state"])
            self.assertEqual("accepted", symbol["semantic"]["state"])

    def test_cross_process_disjoint_domain_writes_both_survive(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path, store = self.create(Path(temporary), revision=7)
            results = self.concurrent_writers(
                path,
                (
                    ("semantic", "/semantic/state", "accepted-cross-process"),
                    ("scheduler", "/scheduler/state", "active-cross-process"),
                ),
            )
            self.assertEqual(["ok", "ok"], sorted(str(row["status"]) for row in results))
            self.assertEqual(
                {
                    "transaction_revision": 9,
                    "semantic_revision": 8,
                    "evidence_generation_revision": 7,
                    "scheduler_revision": 8,
                },
                store.read_revision_vector().to_dict(),
            )
            symbol = store.materialize()["symbols"]["recoil:function:0x401000"]
            self.assertEqual("accepted-cross-process", symbol["semantic"]["state"])
            self.assertEqual("active-cross-process", symbol["scheduler"]["state"])
            observation = self.structured_observation(path)
            self.assertEqual(["ok"], observation["integrity_check"])
            self.assertEqual([], observation["foreign_key_check"])

    def test_cross_process_same_domain_has_exactly_one_cas_winner(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path, store = self.create(Path(temporary), revision=7)
            results = self.concurrent_writers(
                path,
                (
                    ("semantic", "/semantic/state", "winner-a"),
                    ("semantic", "/semantic/state", "winner-b"),
                ),
            )
            self.assertEqual(1, sum(row["status"] == "ok" for row in results))
            errors = [row for row in results if row["status"] == "error"]
            self.assertEqual(1, len(errors))
            self.assertIn("semantic revision changed", str(errors[0]["message"]))
            vector = store.read_revision_vector()
            self.assertEqual((8, 8, 7, 7), tuple(vector.__dict__.values()))
            state = store.materialize()["symbols"]["recoil:function:0x401000"]["semantic"]["state"]
            self.assertIn(state, {"winner-a", "winner-b"})
            observation = self.structured_observation(path)
            self.assertEqual(["ok"], observation["integrity_check"])
            self.assertEqual([], observation["foreign_key_check"])

    def test_validation_context_blocks_live_authority_but_not_fixture_writes(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            live_path, live_store = self.create(
                root / ".agent", revision=7
            )
            # create() uses a fixed basename; placing its root at .agent gives
            # the canonical live authority path after the explicit rename.
            canonical_live = root / ".agent" / "RECONSTRUCTION_PROGRESS.sqlite3"
            live_path.replace(canonical_live)
            live_store = ProgressSQLiteStore(canonical_live)
            fixture_path, fixture_store = self.create(root / "fixtures", revision=7)
            environment = {
                CANONICAL_ROOT_ENV: str(root),
                VALIDATION_READ_ONLY_AUTHORITY_ENV: "1",
            }
            with mock.patch.dict(os.environ, environment, clear=False):
                with self.assertRaisesRegex(
                    ProgressSQLiteError,
                    "validation cannot apply live progress-authority mutations",
                ):
                    live_store.persist_scoped_changes(
                        expected_domain_revisions={"scheduler": 7},
                        top_level_patches={"migration": {"/blocked": True}},
                        apply=True,
                    )
                fixture_store.persist_scoped_changes(
                    expected_domain_revisions={"scheduler": 7},
                    top_level_patches={"migration": {"/fixture": True}},
                    apply=True,
                )
            self.assertEqual(7, live_store.read_revision())
            self.assertEqual(8, fixture_store.read_revision())

    def test_multi_domain_commit_and_new_root_entity_are_atomic(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            _path, store = self.create(Path(temporary), revision=7)
            evidence_id = "recoil:evidence:r8:000001"

            result = store.persist_scoped_changes(
                expected_domain_revisions={
                    "semantic": 7,
                    "evidence_generation": 7,
                },
                entity_patches={
                    "evidence": {
                        evidence_id: {
                            "": {
                                "kind": "live-authored-call-contract-validation",
                                "scope_ids": ["recoil:function:0x401000"],
                            }
                        }
                    }
                },
                top_level_patches={
                    "migration": {"/call_contract_expected_fact_schema_version": 4}
                },
                apply=True,
            )

            self.assertEqual(8, result.revision)
            self.assertEqual(8, result.revision_vector.semantic_revision)
            self.assertEqual(8, result.revision_vector.evidence_generation_revision)
            self.assertEqual(7, result.revision_vector.scheduler_revision)
            materialized = store.materialize()
            self.assertIn(evidence_id, materialized["evidence"])
            self.assertEqual(
                4,
                materialized["migration"]["call_contract_expected_fact_schema_version"],
            )
            self.assertTrue(store.validate_integrity().ok)

    def test_guard_only_semantic_domain_detects_drift_without_incrementing_it(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            _path, store = self.create(Path(temporary), revision=7)

            convergence = store.persist_scoped_changes(
                expected_domain_revisions={
                    "semantic": 7,
                    "evidence_generation": 7,
                },
                increment_domains={"evidence_generation"},
                top_level_patches={"migration": {"/convergence_scan": "current"}},
                apply=True,
            )
            self.assertEqual(8, convergence.revision)
            self.assertEqual(7, convergence.revision_vector.semantic_revision)
            self.assertEqual(
                8, convergence.revision_vector.evidence_generation_revision
            )

            store.persist_scoped_changes(
                expected_domain_revisions={"semantic": 7},
                top_level_patches={"migration": {"/semantic_change": True}},
                apply=True,
            )
            with self.assertRaisesRegex(
                ConcurrentSQLiteProgressUpdate,
                "semantic revision changed: expected 7, found 8",
            ):
                store.persist_scoped_changes(
                    expected_domain_revisions={
                        "semantic": 7,
                        "evidence_generation": 8,
                    },
                    increment_domains={"evidence_generation"},
                    top_level_patches={
                        "migration": {"/stale_convergence_scan": True}
                    },
                    apply=True,
                )

            current = store.read_revision_vector()
            self.assertEqual(9, current.transaction_revision)
            self.assertEqual(8, current.semantic_revision)
            self.assertEqual(8, current.evidence_generation_revision)
            self.assertNotIn(
                "stale_convergence_scan", store.materialize()["migration"]
            )

    def test_increment_domains_must_be_a_nonempty_guarded_subset(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            _path, store = self.create(Path(temporary), revision=7)

            with self.assertRaisesRegex(
                ProgressSQLiteError, "increment_domains must not be empty"
            ):
                store.persist_scoped_changes(
                    expected_domain_revisions={"semantic": 7},
                    increment_domains=set(),
                    apply=True,
                )
            with self.assertRaisesRegex(
                ProgressSQLiteError, "must also be guarded"
            ):
                store.persist_scoped_changes(
                    expected_domain_revisions={"semantic": 7},
                    increment_domains={"evidence_generation"},
                    apply=True,
                )
            self.assertEqual(7, store.read_revision())

    def test_same_domain_stale_revision_is_rejected_without_partial_patch(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            _path, store = self.create(Path(temporary), revision=7)
            symbol_id = "recoil:function:0x401000"
            store.persist_scoped_changes(
                expected_domain_revisions={"semantic": 7},
                entity_patches={
                    "symbols": {symbol_id: {"/semantic/state": "accepted"}}
                },
                apply=True,
            )

            with self.assertRaisesRegex(
                ConcurrentSQLiteProgressUpdate,
                "semantic revision changed: expected 7, found 8",
            ):
                store.persist_scoped_changes(
                    expected_domain_revisions={"semantic": 7},
                    entity_patches={
                        "symbols": {symbol_id: {"/semantic/state": "stale-write"}}
                    },
                    apply=True,
                )

            self.assertEqual(
                "accepted",
                store.materialize()["symbols"][symbol_id]["semantic"]["state"],
            )
            self.assertEqual(8, store.read_revision())

    def test_legacy_global_commit_is_rejected_after_domain_migration(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path, _store = self.create(Path(temporary), revision=7)
            self.downgrade_storage_fixture_to_v1(path)
            store = ProgressSQLiteStore(path)
            store.persist_scoped_changes(
                expected_domain_revisions={"scheduler": 7},
                increment_domains={"scheduler"},
                apply=True,
            )
            with self.assertRaisesRegex(
                ProgressSQLiteError, "legacy single-revision mutation is disabled"
            ):
                store.persist_changes(
                    top_level_updates={"migration": {"legacy": "global"}},
                    expected_revision=8,
                    apply=True,
                )
            self.assertEqual(8, store.materialize()["revision"])

    def test_native_v2_scoped_write_disables_legacy_global_fallback(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path, store = self.create(Path(temporary), revision=7)
            store.persist_scoped_changes(
                expected_domain_revisions={"scheduler": 7},
                increment_domains={"scheduler"},
                apply=True,
            )
            with self.assertRaisesRegex(
                ProgressSQLiteError, "legacy single-revision mutation is disabled"
            ):
                store.persist_changes(
                    top_level_updates={"migration": {"legacy": "global"}},
                    expected_revision=8,
                    apply=True,
                )
            self.assertEqual(8, store.materialize()["revision"])

    def test_first_scoped_write_migrates_v1_atomically(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path, _store = self.create(Path(temporary), revision=7)
            self.downgrade_storage_fixture_to_v1(path)
            store = ProgressSQLiteStore(path)

            dry_run = store.persist_scoped_changes(
                expected_domain_revisions={"scheduler": 7},
                top_level_patches={"migration": {"/dry_run": True}},
                apply=False,
            )
            self.assertEqual(8, dry_run.revision_vector.scheduler_revision)
            with closing(sqlite3.connect(path)) as connection:
                self.assertEqual(
                    LEGACY_USER_VERSION,
                    connection.execute("PRAGMA user_version").fetchone()[0],
                )
            self.assertNotIn("dry_run", store.materialize()["migration"])

            applied = store.persist_scoped_changes(
                expected_domain_revisions={"scheduler": 7},
                top_level_patches={"migration": {"/applied": True}},
                apply=True,
            )
            self.assertEqual(8, applied.revision)
            self.assertEqual(7, applied.revision_vector.semantic_revision)
            self.assertEqual(7, applied.revision_vector.evidence_generation_revision)
            self.assertEqual(8, applied.revision_vector.scheduler_revision)
            self.assertEqual(USER_VERSION, store.metadata().user_version)
            self.assertTrue(store.materialize()["migration"]["applied"])

    def test_native_git_abandonment_dry_run_and_apply_are_scheduler_scoped(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path, store, work_id = self.create_native_git_packet(Path(temporary))
            before = deepcopy(store.materialize())
            before_vector = store.read_revision_vector().to_dict()
            command = [
                "work",
                "close",
                work_id,
                "--outcome",
                "abandoned",
                "--abandonment-reason",
                "superseded",
                "--progress",
                str(path),
                "--expected-scheduler-revision",
                "7",
                "--json",
            ]

            stdout, stderr = io.StringIO(), io.StringIO()
            with redirect_stdout(stdout), redirect_stderr(stderr):
                rc = progress_cli.main([*command, "--dry-run"])
            self.assertEqual(0, rc, stderr.getvalue())
            self.assertFalse(json.loads(stdout.getvalue())["commit"]["applied"])
            self.assertEqual(before, store.materialize())
            self.assertEqual(before_vector, store.read_revision_vector().to_dict())

            stdout, stderr = io.StringIO(), io.StringIO()
            with redirect_stdout(stdout), redirect_stderr(stderr):
                rc = progress_cli.main([*command, "--apply"])
            self.assertEqual(0, rc, stderr.getvalue())
            result = json.loads(stdout.getvalue())
            self.assertTrue(result["commit"]["applied"])
            self.assertTrue(result["retained_terminal_work_item"])
            self.assertTrue(result["terminal_allocation_journal"])
            stored = store.materialize()
            packet = stored["work_items"][work_id]
            self.assertEqual("abandoned", packet["state"])
            self.assertEqual("superseded", packet["abandonment_reason"])
            self.assertEqual("released", packet["reservation"]["state"])
            self.assertEqual("abandoned", packet["reservation"]["outcome"])
            self.assertTrue(packet["nonaccepting"])
            self.assertFalse(packet["acceptance_eligible"])
            journal = stored["migration"]["progress_packet_allocation_journals"][
                "rows"
            ][work_id]
            self.assertEqual("terminal", journal["state"])
            self.assertEqual("abandoned", journal["terminal_outcome"])
            self.assertTrue(journal["terminal_nonaccepting"])
            self.assertEqual(
                {
                    "transaction_revision": 8,
                    "semantic_revision": 7,
                    "evidence_generation_revision": 7,
                    "scheduler_revision": 8,
                },
                store.read_revision_vector().to_dict(),
            )

    def test_native_git_tool_blocked_return_terminalizes_exact_journal(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path, store, work_id = self.create_native_git_packet(Path(temporary))
            issue_ledger = {
                "revision": 12,
                "issues": [{"id": "WSI-TEST-OPEN", "status": "open"}],
            }
            fake_store = mock.Mock()
            fake_store.load.return_value = issue_ledger
            stdout, stderr = io.StringIO(), io.StringIO()
            with (
                mock.patch.object(workspace_issues, "issue_store", return_value=fake_store),
                redirect_stdout(stdout),
                redirect_stderr(stderr),
            ):
                rc = progress_cli.main(
                    [
                        "work",
                        "close",
                        work_id,
                        "--outcome",
                        "returned-tool-blocked",
                        "--linked-tool-issue",
                        "WSI-TEST-OPEN",
                        "--progress",
                        str(path),
                        "--issue-ledger",
                        str(Path(temporary) / "issues.sqlite3"),
                        "--expected-scheduler-revision",
                        "7",
                        "--apply",
                        "--json",
                    ]
                )
            self.assertEqual(0, rc, stderr.getvalue())
            packet = store.materialize()["work_items"][work_id]
            self.assertEqual("returned-tool-blocked", packet["state"])
            self.assertEqual(
                "WSI-TEST-OPEN",
                packet[progress_cli.TOOL_BLOCKED_PROVENANCE_FIELD]["linked_issue_id"],
            )
            journal = store.materialize()["migration"][
                "progress_packet_allocation_journals"
            ]["rows"][work_id]
            self.assertEqual("terminal", journal["state"])
            self.assertEqual("returned-tool-blocked", journal["terminal_outcome"])
            vector = store.read_revision_vector()
            self.assertEqual((8, 7, 7, 8), tuple(vector.__dict__.values()))

    def test_native_git_close_rejects_stale_or_drifted_journal_without_mutation(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path, store, work_id = self.create_native_git_packet(Path(temporary))
            stale_stderr = io.StringIO()
            with redirect_stdout(io.StringIO()), redirect_stderr(stale_stderr):
                rc = progress_cli.main(
                    [
                        "work",
                        "close",
                        work_id,
                        "--outcome",
                        "abandoned",
                        "--abandonment-reason",
                        "superseded",
                        "--progress",
                        str(path),
                        "--expected-scheduler-revision",
                        "6",
                        "--apply",
                        "--json",
                    ]
                )
            self.assertEqual(2, rc)
            self.assertIn("scheduler revision changed", stale_stderr.getvalue())
            self.assertEqual("active", store.materialize()["work_items"][work_id]["state"])

            document = store.materialize()
            document["migration"]["progress_packet_allocation_journals"]["rows"][
                work_id
            ]["operation_id"] = "drifted"
            drift_path = Path(temporary) / "drifted.sqlite3"
            drift_store = ProgressSQLiteStore.create_from_mapping(
                drift_path, document, cutover_pair_id="native-git-close-drift-test"
            )
            before = deepcopy(drift_store.materialize())
            stderr = io.StringIO()
            with redirect_stdout(io.StringIO()), redirect_stderr(stderr):
                rc = progress_cli.main(
                    [
                        "work",
                        "close",
                        work_id,
                        "--outcome",
                        "abandoned",
                        "--abandonment-reason",
                        "superseded",
                        "--progress",
                        str(drift_path),
                        "--expected-scheduler-revision",
                        "7",
                        "--apply",
                        "--json",
                    ]
                )
            self.assertEqual(2, rc)
            self.assertIn("exact activated allocation journal", stderr.getvalue())
            self.assertEqual(before, drift_store.materialize())
            self.assertEqual(7, drift_store.read_revision())

    def test_scoped_apply_action_is_apply_only_and_rolls_back_on_exception(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path, store = self.create(Path(temporary), revision=7)
            calls: list[str] = []

            with self.assertRaisesRegex(ProgressSQLiteError, "requires apply=True"):
                store.persist_scoped_changes(
                    expected_domain_revisions={"scheduler": 7},
                    top_level_patches={"migration": {"/action_test": "dry-run"}},
                    apply_action=lambda: calls.append("dry-run"),
                    apply=False,
                )
            self.assertEqual([], calls)
            self.assertNotIn("action_test", store.materialize()["migration"])

            read_only = ProgressSQLiteStore(path, read_only=True)
            with self.assertRaisesRegex(ProgressSQLiteError, "read-only"):
                read_only.persist_scoped_changes(
                    expected_domain_revisions={"scheduler": 7},
                    top_level_patches={"migration": {"/action_test": "read-only"}},
                    apply_action=lambda: calls.append("read-only"),
                    apply=True,
                )
            self.assertEqual([], calls)

            def fail_action() -> None:
                calls.append("apply")
                raise RuntimeError("injected lifecycle failure")

            with self.assertRaisesRegex(RuntimeError, "injected lifecycle failure"):
                store.persist_scoped_changes(
                    expected_domain_revisions={"scheduler": 7},
                    top_level_patches={"migration": {"/action_test": "apply"}},
                    apply_action=fail_action,
                    apply=True,
                )
            self.assertEqual(["apply"], calls)
            self.assertNotIn("action_test", store.materialize()["migration"])
            self.assertEqual(
                {
                    "transaction_revision": 7,
                    "semantic_revision": 7,
                    "evidence_generation_revision": 7,
                    "scheduler_revision": 7,
                },
                store.read_revision_vector().to_dict(),
            )


if __name__ == "__main__":
    unittest.main()
