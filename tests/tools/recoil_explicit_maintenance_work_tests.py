from __future__ import annotations

from contextlib import closing
from copy import deepcopy
from functools import partial
import io
import json
from pathlib import Path
import sqlite3
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
import uuid


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands.progress_cli import (  # noqa: E402
    create_explicit_maintenance_work,
    main as progress_main,
    recover_explicit_maintenance_allocation,
    return_explicit_maintenance_work_with_binja,
)
from _recoil.commands import progress_cli  # noqa: E402
from _recoil.commands.workspace_issues import empty_ledger  # noqa: E402
from _recoil.lib.binja import (  # noqa: E402
    BinaryNinjaSnapshot,
    BridgeError,
    GovernedBinaryNinjaReadReceipt,
    GovernedBinaryNinjaReadSession,
)
from _recoil.lib.issue_sqlite import create_issue_database, read_issue_metadata  # noqa: E402
from _recoil.lib.progress import (  # noqa: E402
    EXPLICIT_OUTPUT_MARKER_NAME,
    ProgressDocument,
    ProgressError,
    ProgressStore,
    activate_explicit_maintenance_work_item,
    construct_explicit_maintenance_work_item,
    create_and_reserve_explicit_maintenance_work_item,
    empty_progress_document,
    explicit_output_marker_record,
    normalize_resource_claims,
    fail_explicit_maintenance_allocation,
    recover_explicit_maintenance_cleanup_debt,
    recover_expired_explicit_maintenance_work_item,
    reserve_work_item,
    resource_claim_conflicts,
    return_explicit_maintenance_work_item,
)
from _recoil.lib.progress_sqlite import ProgressSQLiteStore  # noqa: E402
from _recoil.lib.worktree_control import resolve_canonical_control_root  # noqa: E402


def canonical_progress_path() -> Path:
    resolution = resolve_canonical_control_root(
        executing_worktree_root=REPO_ROOT,
        required_machine_local_paths=(
            ".agent/RECONSTRUCTION_PROGRESS.sqlite3",
        ),
    )
    return (
        resolution.canonical_control_root
        / ".agent"
        / "RECONSTRUCTION_PROGRESS.sqlite3"
    )


def structured_sqlite_observation(path: Path) -> dict[str, object]:
    """Capture exact routine no-mutation evidence without file-byte equality."""

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
        row_counts: dict[str, int] = {}
        selected_rows: dict[str, list[tuple[object, ...]]] = {}
        metadata_named_rows: list[dict[str, object]] = []
        for name in table_names:
            quoted = name.replace('"', '""')
            row_counts[name] = connection.execute(
                f'SELECT COUNT(*) FROM "{quoted}"'
            ).fetchone()[0]
            if any(token in name.casefold() for token in (
                "metadata", "work", "reservation", "issue", "packet"
            )):
                selected_rows[name] = [
                    tuple(row) for row in connection.execute(
                        f'SELECT * FROM "{quoted}" ORDER BY rowid'
                    )
                ]
            if name == "metadata":
                cursor = connection.execute(f'SELECT * FROM "{quoted}" ORDER BY rowid')
                columns = [str(item[0]) for item in cursor.description or ()]
                metadata_named_rows = [
                    dict(zip(columns, tuple(row))) for row in cursor.fetchall()
                ]
        metadata_row = metadata_named_rows[0] if metadata_named_rows else {}
        logical_schema_version = metadata_row.get("schema_version")
        revision_fields = {
            key: metadata_row[key]
            for key in (
                "revision", "transaction_revision", "semantic_revision",
                "evidence_generation_revision", "scheduler_revision",
            )
            if key in metadata_row
        }
    if integrity != ["ok"] or foreign_keys:
        raise AssertionError(
            f"structured SQLite observation is unhealthy: {integrity!r}, {foreign_keys!r}"
        )
    return {
        "user_version": user_version,
        "schema_version": schema_version,
        "logical_schema_version": logical_schema_version,
        "revision_fields": revision_fields,
        "metadata_rows": metadata_named_rows,
        "row_counts": row_counts,
        "selected_rows": selected_rows,
        "integrity_check": integrity,
        "foreign_key_check": foreign_keys,
    }


def fixture(root: Path) -> dict:
    (root / "src").mkdir()
    (root / "src" / "a.cpp").write_text("int a;\n", encoding="utf-8")
    (root / "src" / "a.h").write_text("extern int a;\n", encoding="utf-8")
    (root / "src" / "other.h").write_text("extern int b;\n", encoding="utf-8")
    (root / "support").mkdir()
    (root / "support" / "Recoil.exe").write_bytes(b"retail")
    subprocess.run(
        ["git", "init", "--quiet"],
        cwd=root,
        check=True,
        capture_output=True,
    )
    subprocess.run(
        ["git", "add", "--", "src/a.cpp", "src/a.h", "src/other.h"],
        cwd=root,
        check=True,
        capture_output=True,
    )
    data = empty_progress_document()
    data["revision"] = 7
    data["verification_targets"]["recoil:vc5-target:a"] = {
        "binary": "recoil",
        "kind": "vc5",
        "symbol_ids": ["recoil:function:0x401000"],
        "registration": {"source_from": "src/a.cpp", "order_edit_paths": ["src/a.cpp"]},
    }
    data["physical_blocks"]["recoil:block:0x401000"] = {
        "source_path": "src/a.cpp",
        "symbol_ids": ["recoil:function:0x401000"],
    }
    data["symbols"]["recoil:function:0x401000"] = {
        "address": "0x401000",
        "physical_block_id": "recoil:block:0x401000",
        "owner_id": "recoil:owner:a",
    }
    data["owners"]["recoil:owner:a"] = {
        "source_paths": ["src/a.cpp", "src/a.h"],
        "primary_entry_ids": ["recoil:function:0x401000"],
    }
    data["owners"]["recoil:owner:other"] = {
        "source_paths": ["src/other.h"],
        "primary_entry_ids": [],
    }
    return data


def payload(*, packet_id: str = "recoil:explicit-work:a", kind: str = "source-maintenance") -> dict:
    return {
        "schema": "recoil-explicit-maintenance-packet-v1",
        "packet_id": packet_id,
        "kind": kind,
        "selected_scope": {
            "verification_target_ids": ["recoil:vc5-target:a"],
            "physical_block_ids": [],
            "source_owner_ids": [],
        },
        "writable_paths": ["src/a.cpp"] if kind == "source-maintenance" else [],
        "writable_overrides": [],
        "read_dependencies": ["src/a.h"],
        "output_root": f"build/{packet_id.rsplit(':', 1)[-1]}",
        "resources": {
            "binary_ninja_saved_view_read": False,
            "whole_link_window": False,
            "tracker_read": True,
            "manifest_read": False,
            "support_read": False,
        },
        "objective": "Verify the explicit selected scope.",
        "stop_condition": "Return at PASS or the first scope contradiction.",
        "validation_command": "python -B tools/recoil.py verify vc5-order a --build-root build/a",
        "worker_role": "recoil_source_worker" if kind == "source-maintenance" else "recoil_verifier",
        "return_schema": ["outcome", "changed_paths"],
        "user_selected_rationale": "The user explicitly selected target a.",
        "scheduler_inappropriate_reason": "This is maintenance outside the phase frontier.",
    }


def allocate_packet(
    data: dict,
    request: dict,
    *,
    root: Path,
) -> tuple[dict, Path]:
    """Exercise the allocating -> marker-authenticated -> active library path."""

    (root / "build").mkdir(exist_ok=True)
    progress = root / "progress.sqlite3"
    if not progress.exists():
        ProgressSQLiteStore.create_from_mapping(
            progress,
            deepcopy(data),
            cutover_pair_id="explicit-library-fixture",
        )
    create_and_reserve_explicit_maintenance_work_item(
        data,
        request,
        repo_root=root,
        progress_path=progress,
        operation_nonce=uuid.uuid4().hex,
        issue_ledger_identity={"fixture": "explicit-library"},
    )
    work_id = str(request["packet_id"])
    journal = data["migration"]["explicit_output_allocation_journals"]["rows"][work_id]
    allocation = journal["expected_ownership_marker"]
    output_root = root / allocation["normalized_output_root"]
    sidecar = root / allocation["ownership_sidecar"]
    sidecar.write_text(
        json.dumps(allocation, sort_keys=True, separators=(",", ":")) + "\n",
        encoding="utf-8",
    )
    output_root.mkdir(exist_ok=False)
    marker = explicit_output_marker_record(allocation, output_root)
    (output_root / EXPLICIT_OUTPUT_MARKER_NAME).write_text(
        json.dumps(marker, sort_keys=True, separators=(",", ":")) + "\n",
        encoding="utf-8",
    )
    result = activate_explicit_maintenance_work_item(
        data,
        work_id,
        progress_path=progress,
        repo_root=root,
    )
    return result, progress


def sync_packet_store(progress: Path, data: dict) -> int:
    current = ProgressStore(progress).load()

    def replace(mapping: dict) -> None:
        mapping.clear()
        mapping.update(deepcopy(data))

    result = ProgressStore(progress).mutate(
        replace,
        expected_revision=current.revision,
        apply=True,
    )
    return int(result.revision)


def remove_owned_test_root(path: Path) -> None:
    marker = path / EXPLICIT_OUTPUT_MARKER_NAME
    if marker.is_file():
        marker.unlink()
    if path.is_dir():
        path.rmdir()
    sidecar = path.parent / f".{path.name}.recoil-explicit-allocation-owner.json"
    if sidecar.is_file():
        sidecar.unlink()


class FakeBridge:
    def __init__(self, *_args, snapshots=(), **_kwargs) -> None:
        self.snapshots = list(snapshots)
        self.calls: list[tuple[str, object]] = []

    def fresh_snapshot(self) -> BinaryNinjaSnapshot:
        return self.snapshots.pop(0)

    def get_json(self, endpoint: str, **params: object) -> dict:
        self.calls.append((endpoint, params))
        if endpoint == "status":
            return {
                "loaded": True,
                "filename": "D:/Recoil Project/Decomp/Recoil.bndb",
                "platform": "windows-x86",
                "arch": "x86",
                "open_binaries": 1,
            }
        if endpoint == "binaries":
            return {"binaries": [{"filename": "D:/Recoil Project/Decomp/Recoil.bndb"}]}
        return {"endpoint": endpoint, "params": params}

    def hexdump(self, address: str, length: int) -> str:
        self.calls.append(("hexdump", (address, length)))
        return "401000 c3"


def authenticated_snapshot(token: str = "t", revision: str = "r") -> BinaryNinjaSnapshot:
    return BinaryNinjaSnapshot(
        available=True,
        generation_token=token,
        revision=revision,
        schema="recoil-binja-authenticated-snapshot-v2",
        authenticated=True,
        provider="binary-ninja",
        capability_version="2",
        saved_view="Recoil.bndb",
    )


def bind_issue_identity(data: dict, issue_path: Path) -> None:
    metadata = read_issue_metadata(issue_path)
    work = data["work_items"]["recoil:explicit-work:a"]
    work["explicit_provenance"]["issue_ledger_identity"] = {
        "path": str(issue_path.resolve()),
        "application_id": metadata.application_id,
        "user_version": metadata.user_version,
        "schema_version": metadata.schema_version,
        "ledger_version": metadata.ledger_version,
        "cutover_pair_id": metadata.cutover_pair_id,
    }


class ExplicitMaintenanceWorkTests(unittest.TestCase):
    def test_01_exact_target_creates_and_reserves_atomically(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data = fixture(root)
            result, _progress = allocate_packet(data, payload(), root=root)
            work = data["work_items"]["recoil:explicit-work:a"]
            self.assertEqual("active", work["state"])
            self.assertEqual(result["reservation_id"], work["reservation"]["id"])
            self.assertTrue((root / "build" / "a").is_dir())
            self.assertTrue(
                (root / "build" / "a" / EXPLICIT_OUTPUT_MARKER_NAME).is_file()
            )

    def test_02_unknown_target_fails_without_mutation(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data = fixture(root)
            before = deepcopy(data)
            request = payload()
            request["selected_scope"]["verification_target_ids"] = ["missing"]
            with self.assertRaises(ProgressError):
                allocate_packet(data, request, root=root)
            self.assertEqual(before, data)

    def test_03_broad_escape_and_unrelated_writes_fail(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data = fixture(root)
            for value in ("src", "../escape.cpp", "src/other.h"):
                request = payload(packet_id=f"recoil:explicit-work:{uuid.uuid4().hex}")
                request["writable_paths"] = [value]
                with self.subTest(value=value), self.assertRaises(ProgressError):
                    construct_explicit_maintenance_work_item(
                        ProgressDocument(data), request, repo_root=root
                    )

    def test_04_reviewed_cross_owner_override_requires_complete_binding(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data = fixture(root)
            request = payload()
            request["writable_paths"].append("src/other.h")
            override = {
                "path": "src/other.h",
                "relation": "reviewed-cross-owner-declaration",
                "selected_scope_id": "recoil:vc5-target:a",
                "related_owner_id": "recoil:owner:other",
                "evidence": "Reviewed declaration xref.",
                "rationale": "The selected definition requires this declaration.",
            }
            request["writable_overrides"] = [override]
            _work_id, work = construct_explicit_maintenance_work_item(
                ProgressDocument(data), request, repo_root=root
            )
            self.assertIn("src/other.h", work["allowed_paths"])
            self.assertIn(
                "recoil:owner:other", work["related_source_owner_ids"]
            )
            self.assertIn(
                {"kind": "owner", "id": "recoil:owner:other", "access": "write"},
                work["resource_claims"],
            )
            self.assertEqual(
                [override],
                work["explicit_provenance"]["closure"][
                    "reviewed_cross_owner_overrides"
                ],
            )
            override["evidence"] = ""
            with self.assertRaises(ProgressError):
                construct_explicit_maintenance_work_item(
                    ProgressDocument(data), request, repo_root=root
                )

    def test_05_source_packet_rejects_empty_writes(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data = fixture(root)
            request = payload()
            request["writable_paths"] = []
            with self.assertRaisesRegex(ProgressError, "non-empty writable"):
                construct_explicit_maintenance_work_item(ProgressDocument(data), request, repo_root=root)

    def test_06_diagnostic_has_no_source_writes_and_reads_scope(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data = fixture(root)
            request = payload(kind="read-only-diagnostic")
            _work_id, work = construct_explicit_maintenance_work_item(
                ProgressDocument(data), request, repo_root=root
            )
            self.assertEqual([], work["allowed_paths"])
            self.assertIn("src/a.cpp", work["read_only_paths"])
            request["writable_paths"] = ["src/a.cpp"]
            with self.assertRaisesRegex(ProgressError, "cannot acquire source write"):
                construct_explicit_maintenance_work_item(ProgressDocument(data), request, repo_root=root)

    def test_07_constructor_performs_only_read_only_git_path_authentication(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data = fixture(root)
            with patch("tempfile.mkdtemp", side_effect=AssertionError("temp")), patch(
                "_recoil.lib.binja.BinaryNinjaBridge", side_effect=AssertionError("BN")
            ):
                construct_explicit_maintenance_work_item(
                    ProgressDocument(data), payload(), repo_root=root
                )
            self.assertFalse((root / "build" / "a").exists())

    def test_07a_tracked_and_generated_path_classes_fail_closed(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data = fixture(root)
            exact_id, exact = construct_explicit_maintenance_work_item(
                ProgressDocument(data), payload(), repo_root=root
            )
            self.assertEqual("recoil:explicit-work:a", exact_id)
            self.assertEqual(["src/a.cpp"], exact["allowed_paths"])
            self.assertEqual(
                "build/a",
                exact["explicit_provenance"]["closure"]["output_root"],
            )

            for supplied, message in (
                ("src/A.cpp", "expected 'src/a.cpp'"),
                (str((root / "src" / "a.cpp").resolve()), "normalized repo-local"),
            ):
                request = payload(packet_id=f"recoil:explicit-work:{uuid.uuid4().hex}")
                request["writable_paths"] = [supplied]
                with self.subTest(supplied=supplied), self.assertRaisesRegex(
                    ProgressError, message
                ):
                    construct_explicit_maintenance_work_item(
                        ProgressDocument(data), request, repo_root=root
                    )

            request = payload(packet_id=f"recoil:explicit-work:{uuid.uuid4().hex}")
            request["output_root"] = "src/generated"
            with self.assertRaisesRegex(ProgressError, "allowed generated root"):
                construct_explicit_maintenance_work_item(
                    ProgressDocument(data), request, repo_root=root
                )

            with self.assertRaisesRegex(ProgressError, "normalized repo-local"):
                normalize_resource_claims(
                    [
                        {
                            "kind": "path",
                            "id": str((root / "src" / "a.cpp").resolve()),
                            "access": "write",
                        }
                    ]
                )
            self.assertEqual(
                [
                    {"kind": "output-root", "id": "build/generated", "access": "write"},
                    {"kind": "reference", "id": "support/Recoil.exe", "access": "read"},
                    {"kind": "tracker", "id": "recoil", "access": "read"},
                ],
                normalize_resource_claims(
                    [
                        {"kind": "tracker", "id": "recoil", "access": "read"},
                        {"kind": "reference", "id": "support/Recoil.exe", "access": "read"},
                        {
                            "kind": "output-root",
                            "id": "build/generated",
                            "access": "write",
                        },
                    ]
                ),
            )
            external_output = (root.parent / "external-packet-build").resolve()
            self.assertEqual(
                [
                    {
                        "kind": "output-root",
                        "id": external_output.as_posix(),
                        "access": "write",
                    }
                ],
                normalize_resource_claims(
                    [
                        {
                            "kind": "output-root",
                            "id": str(external_output),
                            "access": "write",
                        }
                    ]
                ),
            )

    def test_08_binja_readers_overlap_and_writer_conflicts(self) -> None:
        reader = [{"kind": "binary-ninja-db", "id": "D:/x/Recoil.bndb", "access": "read"}]
        other_reader = [{"kind": "binary-ninja-db", "id": "Recoil.bndb", "access": "read"}]
        writer = [{"kind": "binary-ninja-db", "id": "recoil.bndb", "access": "write"}]
        self.assertEqual([], resource_claim_conflicts(reader, "reader", other_reader))
        self.assertTrue(resource_claim_conflicts(reader, "writer", writer))

    def _reserved_binja_json(self, root: Path, snapshots) -> tuple[Path, FakeBridge, dict]:
        data = fixture(root)
        request = payload(kind="read-only-diagnostic")
        request["resources"]["binary_ninja_saved_view_read"] = True
        _reserved, progress = allocate_packet(data, request, root=root)
        issues = root / "issues.sqlite3"
        create_issue_database(issues, empty_ledger(), cutover_pair_id="direct-binja")
        bind_issue_identity(data, issues)
        sync_packet_store(progress, data)
        path = progress
        fake = FakeBridge(snapshots=snapshots)
        return path, fake, data

    def test_09_binja_equal_snapshots_emit_complete_sealed_transcript(self) -> None:
        snap = authenticated_snapshot()
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path, fake, data = self._reserved_binja_json(root, [snap, snap])
            with patch("_recoil.lib.binja.BinaryNinjaBridge", return_value=fake):
                session = GovernedBinaryNinjaReadSession(
                    path, "recoil:explicit-work:a"
                )
            session.get_json("assembly", address="0x401000")
            session.hexdump("0x401000", 1)
            receipt = session.finish()
            self.assertEqual(4, receipt.as_dict()["fact_read_count"])
            return_explicit_maintenance_work_item(
                data,
                "recoil:explicit-work:a",
                {"outcome": "PASS", "changed_paths": []},
                binja_receipt=receipt,
                progress_path=path,
            )
            with self.assertRaises(BridgeError):
                GovernedBinaryNinjaReadReceipt(
                    _seal=True,
                    packet_id="x",
                    reservation_id="x",
                    reference_image={},
                    issue_ledger_identity={},
                    begin_snapshot={},
                    end_snapshot={},
                    fact_reads=(),
                    resource_claims=(),
                )

    def test_09b_binja_drift_and_unavailable_snapshot_fail(self) -> None:
        a = authenticated_snapshot("a")
        b = authenticated_snapshot("b")
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path, fake, _data = self._reserved_binja_json(root, [a, b])
            with patch("_recoil.lib.binja.BinaryNinjaBridge", return_value=fake):
                session = GovernedBinaryNinjaReadSession(
                    path, "recoil:explicit-work:a"
                )
            with self.assertRaisesRegex(BridgeError, "snapshot changed"):
                session.finish()
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            unavailable = BinaryNinjaSnapshot(False, reason="unsupported")
            path, fake, _data = self._reserved_binja_json(root, [unavailable])
            with patch("_recoil.lib.binja.BinaryNinjaBridge", return_value=fake), self.assertRaisesRegex(BridgeError, "unavailable"):
                GovernedBinaryNinjaReadSession(path, "recoil:explicit-work:a")

    def test_10_v2_apply_increments_only_transaction_and_scheduler(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            (tmp_path / "build").mkdir()
            data = fixture(tmp_path)
            progress = tmp_path / "progress.sqlite3"
            store = ProgressSQLiteStore.create_from_mapping(
                progress, data, cutover_pair_id="explicit-test"
            )
            issues = tmp_path / "issues.sqlite3"
            create_issue_database(
                issues, empty_ledger(), cutover_pair_id="explicit-issue-test"
            )
            request = payload()
            request["read_dependencies"] = []
            request["validation_command"] = "python -B tools/recoil.py verify vc5-order a --build-root build/a"
            request["output_root"] = f"build/explicit-test-{uuid.uuid4().hex}"
            args = type("Args", (), {
                "progress": progress,
                "issue_ledger": issues,
                "payload_json": json.dumps(request),
                "payload_file": None,
                "expected_scheduler_revision": 7,
                "expected_semantic_revision": 7,
                "expected_revision": None,
                "apply": True,
            })()
            with (
                patch.object(progress_cli, "REPO_ROOT", tmp_path),
                patch.object(
                    progress_cli,
                    "construct_explicit_maintenance_work_item",
                    partial(
                        progress_cli.construct_explicit_maintenance_work_item,
                        repo_root=tmp_path,
                    ),
                ),
                patch.object(
                    progress_cli,
                    "create_and_reserve_explicit_maintenance_work_item",
                    partial(
                        progress_cli.create_and_reserve_explicit_maintenance_work_item,
                        repo_root=tmp_path,
                    ),
                ),
                patch.object(
                    progress_cli,
                    "authenticate_explicit_output_root",
                    partial(
                        progress_cli.authenticate_explicit_output_root,
                        repo_root=tmp_path,
                    ),
                ),
                patch.object(
                    progress_cli,
                    "activate_explicit_maintenance_work_item",
                    partial(
                        progress_cli.activate_explicit_maintenance_work_item,
                        repo_root=tmp_path,
                    ),
                ),
            ):
                create_explicit_maintenance_work(args)
            vector = store.read_revision_vector()
            self.assertEqual((9, 7, 7, 9), (
                vector.transaction_revision,
                vector.semantic_revision,
                vector.evidence_generation_revision,
                vector.scheduler_revision,
            ))

    def test_11_worker_pass_is_nonaccepting_and_bounded(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data = fixture(root)
            accepted_before = deepcopy(
                {key: data[key] for key in ("symbols", "physical_blocks", "owners", "evidence")}
            )
            _reserved, progress = allocate_packet(data, payload(), root=root)
            result = return_explicit_maintenance_work_item(
                data,
                "recoil:explicit-work:a",
                {"outcome": "PASS", "changed_paths": ["src/a.cpp"]},
                progress_path=progress,
            )
            self.assertFalse(result["acceptance_changed"])
            self.assertEqual(accepted_before, {key: data[key] for key in accepted_before})
            self.assertEqual(
                "activated",
                data["migration"]["explicit_output_allocation_journals"]["rows"][
                    "recoil:explicit-work:a"
                ]["state"],
            )

    def test_12_two_nonconflicting_packets_coexist(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data = fixture(root)
            first = payload(kind="read-only-diagnostic")
            allocate_packet(data, first, root=root)
            second = payload(packet_id="recoil:explicit-work:b", kind="read-only-diagnostic")
            second["output_root"] = "build/b"
            allocate_packet(data, second, root=root)
            self.assertEqual(2, len(data["work_items"]))

    def test_13_path_read_write_and_write_write_conflict_but_reads_overlap(self) -> None:
        read = [{"kind": "path", "id": "src/a.h", "access": "read"}]
        write = [{"kind": "path", "id": "src/a.h", "access": "write"}]
        self.assertEqual([], resource_claim_conflicts(read, "r", read))
        self.assertTrue(resource_claim_conflicts(read, "w", write))
        self.assertTrue(resource_claim_conflicts(write, "w", write))

    def test_14_whole_link_window_conflicts_with_tu_readers(self) -> None:
        tu = [{"kind": "tu-build", "id": "recoil:vc5-target:a", "access": "read"}]
        whole = [{"kind": "whole-link-window", "id": "recoil", "access": "write"}]
        self.assertEqual([], resource_claim_conflicts(tu, "tu", tu))
        self.assertTrue(resource_claim_conflicts(whole, "tu", tu))

    def test_15_reservation_failure_leaves_only_nonrunnable_intent(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data = fixture(root)
            with patch("_recoil.lib.progress.reserve_work_item", side_effect=ProgressError("fail")):
                with self.assertRaises(ProgressError):
                    allocate_packet(data, payload(), root=root)
            work = data["work_items"].get("recoil:explicit-work:a")
            if work is not None:
                self.assertNotEqual("active", work["state"])
                self.assertIsNone(work["reservation"])
                self.assertEqual([], work["execution_attempts"])

    def test_16_live_tracker_read_only_observation_is_revision_stable(self) -> None:
        path = canonical_progress_path()

        def observation() -> dict[str, object]:
            connection = sqlite3.connect(path.as_uri() + "?mode=ro", uri=True)
            try:
                connection.execute("PRAGMA query_only=ON")
                table_names = tuple(
                    str(row[0])
                    for row in connection.execute(
                        "SELECT name FROM sqlite_master "
                        "WHERE type='table' AND name NOT LIKE 'sqlite_%' ORDER BY name"
                    )
                )
                return {
                    "user_version": int(connection.execute("PRAGMA user_version").fetchone()[0]),
                    "integrity": tuple(
                        str(row[0]) for row in connection.execute("PRAGMA integrity_check")
                    ),
                    "metadata": tuple(connection.execute(
                        "SELECT schema_version, revision FROM metadata WHERE singleton=1"
                    ).fetchone()),
                    "row_counts": {
                        table: int(connection.execute(
                            f'SELECT COUNT(*) FROM "{table}"'
                        ).fetchone()[0])
                        for table in table_names
                    },
                }
            finally:
                connection.close()

        before_metadata = ProgressSQLiteStore(path, read_only=True).metadata()
        before_revision_vector = (
            before_metadata.revision,
            before_metadata.semantic_revision,
            before_metadata.evidence_generation_revision,
            before_metadata.scheduler_revision,
        )
        before = observation()
        after = observation()
        after_metadata = ProgressSQLiteStore(path, read_only=True).metadata()
        after_revision_vector = (
            after_metadata.revision,
            after_metadata.semantic_revision,
            after_metadata.evidence_generation_revision,
            after_metadata.scheduler_revision,
        )
        self.assertEqual(before, after)
        self.assertEqual(before_revision_vector, after_revision_vector)
        self.assertEqual(("ok",), before["integrity"])
        self.assertEqual(before_metadata.revision, before["metadata"][1])
        self.assertEqual(after_metadata.revision, after["metadata"][1])
        self.assertEqual(before_metadata.user_version, before["user_version"])
        self.assertEqual(after_metadata.user_version, after["user_version"])
        self.assertEqual(before_metadata.user_version, after_metadata.user_version)

    def test_expiry_recovery_releases_attempt_and_returns_ready(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data = fixture(root)
            request = payload()
            request["lease_expires_at"] = "2000-01-01T00:00:00Z"
            allocate_packet(data, request, root=root)
            recover_expired_explicit_maintenance_work_item(data, "recoil:explicit-work:a")
            work = data["work_items"]["recoil:explicit-work:a"]
            self.assertEqual("ready", work["state"])
            self.assertEqual("released", work["execution_attempts"][0]["state"])
            self.assertIsNone(work["lease_expires_at"])
            reserve_work_item(data, "recoil:explicit-work:a")
            self.assertEqual("active", work["state"])
            self.assertIsNone(work["reservation"]["expires"])

    def _public_binja_return_fixture(
        self, root: Path, snapshots: list[BinaryNinjaSnapshot]
    ) -> tuple[Path, Path, FakeBridge]:
        data = fixture(root)
        request = payload(kind="read-only-diagnostic")
        request["resources"]["binary_ninja_saved_view_read"] = True
        _reserved, progress = allocate_packet(data, request, root=root)
        issues = root / "issues.sqlite3"
        create_issue_database(issues, empty_ledger(), cutover_pair_id="explicit-binja")
        bind_issue_identity(data, issues)
        sync_packet_store(progress, data)
        fake = FakeBridge(snapshots=snapshots)

        return progress, issues, fake

    def test_17_public_binja_return_executes_plan_and_cas_returns(self) -> None:
        snap = authenticated_snapshot()
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            progress, issues, fake = self._public_binja_return_fixture(
                root, [snap, snap]
            )
            plan_json = json.dumps({
                "schema": "recoil-governed-binja-read-plan-v1",
                "requests": [
                    {"transport": "json", "endpoint": "assembly", "parameters": {"address": "0x401000"}},
                    {"transport": "hexdump", "address": "0x401000", "length": 1},
                ],
            })
            result_json = json.dumps({"outcome": "PASS", "changed_paths": []})
            output = io.StringIO()
            with patch("_recoil.lib.binja.BinaryNinjaBridge", return_value=fake), patch(
                "sys.stdout", new=output
            ):
                code = progress_main([
                    "work", "return-binja",
                    "--id", "recoil:explicit-work:a",
                    "--read-plan-json", plan_json,
                    "--result-json", result_json,
                    "--progress", str(progress),
                    "--expected-revision", str(ProgressStore(progress).load().revision),
                    "--apply", "--json",
                ])
            self.assertEqual(0, code)
            self.assertTrue(json.loads(output.getvalue())["commit"]["applied"])
            stored = ProgressStore(progress).load().collection("work_items")[
                "recoil:explicit-work:a"
            ]
            self.assertEqual("returned", stored["state"])
            self.assertEqual(4, stored["binary_ninja_read_receipt"]["fact_read_count"])
            self.assertEqual(
                ["status", "binaries", "assembly", "hexdump"],
                [row[0] for row in fake.calls],
            )

    def test_18_public_binja_unavailable_or_drift_never_mutates(self) -> None:
        cases = (
            [BinaryNinjaSnapshot(False, reason="unsupported")],
            [
                authenticated_snapshot("a"),
                authenticated_snapshot("b"),
            ],
        )
        for snapshots in cases:
            with self.subTest(snapshots=snapshots), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                progress, issues, fake = self._public_binja_return_fixture(
                    root, list(snapshots)
                )
                args = type("Args", (), {
                    "progress": progress,
                    "issue_ledger": issues,
                    "id": "recoil:explicit-work:a",
                    "read_plan_json": json.dumps({"schema": "recoil-governed-binja-read-plan-v1", "requests": []}),
                    "result_json": json.dumps({"outcome": "PASS", "changed_paths": []}),
                    "expected_scheduler_revision": None,
                    "expected_revision": ProgressStore(progress).load().revision,
                    "apply": True,
                })()
                before = structured_sqlite_observation(progress)
                with patch("_recoil.lib.binja.BinaryNinjaBridge", return_value=fake), self.assertRaises(BridgeError):
                    return_explicit_maintenance_work_with_binja(args)
                self.assertEqual(before, structured_sqlite_observation(progress))

    def test_19_public_binja_pre_reservation_never_constructs_bridge(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data = fixture(root)
            request = payload(kind="read-only-diagnostic")
            request["resources"]["binary_ninja_saved_view_read"] = True
            work_id, packet = construct_explicit_maintenance_work_item(
                ProgressDocument(data), request, repo_root=root
            )
            data["work_items"][work_id] = packet
            progress = root / "progress.json"
            progress.write_text(json.dumps(data), encoding="utf-8")
            issues = root / "issues.sqlite3"
            create_issue_database(issues, empty_ledger(), cutover_pair_id="no-reservation")
            bridge_constructed = False

            def bridge_factory(*_args: object, **_kwargs: object):
                nonlocal bridge_constructed
                bridge_constructed = True
                raise AssertionError("bridge constructed")

            args = type("Args", (), {
                "progress": progress,
                "issue_ledger": issues,
                "id": work_id,
                "read_plan_json": json.dumps({"schema": "recoil-governed-binja-read-plan-v1", "requests": []}),
                "result_json": json.dumps({"outcome": "PASS", "changed_paths": []}),
                "expected_scheduler_revision": None,
                "expected_revision": ProgressStore(progress).load().revision,
                "apply": True,
            })()
            with patch("_recoil.lib.binja.BinaryNinjaBridge", side_effect=bridge_factory), self.assertRaisesRegex(ProgressError, "active explicit maintenance"):
                return_explicit_maintenance_work_with_binja(args)
            self.assertFalse(bridge_constructed)
            with self.assertRaises(TypeError):
                GovernedBinaryNinjaReadSession(  # type: ignore[call-arg]
                    progress, work_id, bridge_factory=lambda: None
                )

    def test_20_close_retains_terminal_explicit_provenance(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data = fixture(root)
            _reserved, progress = allocate_packet(data, payload(), root=root)
            return_explicit_maintenance_work_item(
                data,
                "recoil:explicit-work:a",
                {"outcome": "PASS", "changed_paths": []},
                progress_path=progress,
            )
            revision = sync_packet_store(progress, data)
            with patch("sys.stdout", new=io.StringIO()):
                code = progress_main([
                    "work", "close", "recoil:explicit-work:a",
                    "--progress", str(progress),
                    "--expected-revision", str(revision), "--apply", "--json",
                ])
            self.assertEqual(0, code)
            stored = ProgressStore(progress).load().collection("work_items")[
                "recoil:explicit-work:a"
            ]
            self.assertEqual("closed", stored["state"])
            self.assertEqual("released", stored["reservation"]["state"])
            self.assertEqual({"outcome": "PASS", "changed_paths": []}, stored["returned_result"])
            with self.assertRaises(ProgressError):
                reserve_work_item(ProgressStore(progress).load().data, "recoil:explicit-work:a")

    def test_21_active_abandon_retains_released_terminal_record(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data = fixture(root)
            _reserved, progress = allocate_packet(data, payload(), root=root)
            revision = sync_packet_store(progress, data)
            with patch("sys.stdout", new=io.StringIO()):
                code = progress_main([
                    "work", "close", "recoil:explicit-work:a",
                    "--outcome", "abandoned", "--abandonment-reason", "worker failed",
                    "--progress", str(progress),
                    "--expected-revision", str(revision), "--apply", "--json",
                ])
            self.assertEqual(0, code)
            stored = ProgressStore(progress).load().collection("work_items")[
                "recoil:explicit-work:a"
            ]
            self.assertEqual("abandoned", stored["state"])
            self.assertEqual("released", stored["reservation"]["state"])
            self.assertEqual("released", stored["execution_attempts"][0]["state"])
            self.assertFalse(stored["acceptance_eligible"])

    def test_22_v2_creation_guards_semantic_closure_without_incrementing_it(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data = empty_progress_document()
            data["revision"] = 7
            data["verification_targets"]["recoil:vc5-target:a"] = {
                "registration": {"source_from": "src/Battlesport/Briefing.h"}
            }
            progress = root / "progress.sqlite3"
            store = ProgressSQLiteStore.create_from_mapping(
                progress, data, cutover_pair_id="semantic-guard"
            )
            issues = root / "issues.sqlite3"
            create_issue_database(issues, empty_ledger(), cutover_pair_id="semantic-issue")
            store.persist_scoped_changes(
                expected_domain_revisions={"semantic": 7},
                entity_patches={},
                top_level_patches={},
                increment_domains={"semantic"},
                apply=True,
            )
            request = payload()
            request["read_dependencies"] = []
            request["writable_paths"] = ["src/Battlesport/Briefing.h"]
            request["output_root"] = f"build/semantic-guard-{uuid.uuid4().hex}"
            args = type("Args", (), {
                "progress": progress,
                "issue_ledger": issues,
                "payload_json": json.dumps(request),
                "payload_file": None,
                "expected_scheduler_revision": 7,
                "expected_semantic_revision": 7,
                "expected_revision": None,
                "apply": True,
            })()
            before_progress = structured_sqlite_observation(progress)
            before_issues = structured_sqlite_observation(issues)
            with self.assertRaisesRegex(Exception, "semantic revision changed"):
                create_explicit_maintenance_work(args)
            self.assertEqual(before_progress, structured_sqlite_observation(progress))
            self.assertEqual(before_issues, structured_sqlite_observation(issues))
            vector = store.read_revision_vector()
            self.assertEqual((8, 8, 7, 7), (
                vector.transaction_revision,
                vector.semantic_revision,
                vector.evidence_generation_revision,
                vector.scheduler_revision,
            ))

    def test_23_binja_plan_cannot_override_recoil_saved_view(self) -> None:
        snap = authenticated_snapshot()
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            progress, issues, _fake = self._public_binja_return_fixture(root, [snap, snap])
            args = type("Args", (), {
                "progress": progress,
                "issue_ledger": issues,
                "id": "recoil:explicit-work:a",
                "read_plan_json": json.dumps({
                    "schema": "recoil-governed-binja-read-plan-v1",
                    "requests": [{
                        "transport": "json",
                        "endpoint": "assembly",
                        "parameters": {"address": "0x401000", "binary": "messages.bndb"},
                    }],
                }),
                "result_json": json.dumps({"outcome": "PASS", "changed_paths": []}),
                "expected_scheduler_revision": None,
                "expected_revision": ProgressStore(progress).load().revision,
                "apply": True,
            })()
            before = structured_sqlite_observation(progress)
            with patch("_recoil.lib.binja.BinaryNinjaBridge", side_effect=AssertionError("bridge")), self.assertRaisesRegex(ProgressError, "cannot override"):
                return_explicit_maintenance_work_with_binja(args)
            self.assertEqual(before, structured_sqlite_observation(progress))

    def test_24_binja_transcript_overflow_fails_before_progress_mutation(self) -> None:
        snap = authenticated_snapshot()
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            progress, issues, fake = self._public_binja_return_fixture(root, [snap, snap])
            original_get_json = fake.get_json

            def large_get_json(endpoint: str, **params: object) -> dict:
                if endpoint == "assembly":
                    return {"exact_payload": "x" * (4 * 1024 * 1024)}
                return original_get_json(endpoint, **params)

            fake.get_json = large_get_json  # type: ignore[method-assign]
            args = type("Args", (), {
                "progress": progress,
                "issue_ledger": issues,
                "id": "recoil:explicit-work:a",
                "read_plan_json": json.dumps({
                    "schema": "recoil-governed-binja-read-plan-v1",
                    "requests": [{"transport": "json", "endpoint": "assembly", "parameters": {"address": "0x401000"}}],
                }),
                "result_json": json.dumps({"outcome": "PASS", "changed_paths": []}),
                "expected_scheduler_revision": None,
                "expected_revision": ProgressStore(progress).load().revision,
                "apply": True,
            })()
            before = structured_sqlite_observation(progress)
            with patch("_recoil.lib.binja.BinaryNinjaBridge", return_value=fake), self.assertRaisesRegex(BridgeError, "transcript exceeds"):
                return_explicit_maintenance_work_with_binja(args)
            self.assertEqual(before, structured_sqlite_observation(progress))

    def test_25_explicit_dry_run_preview_is_byte_pure(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data = empty_progress_document()
            data["revision"] = 7
            data["verification_targets"]["recoil:vc5-target:a"] = {
                "registration": {"source_from": "src/Battlesport/Briefing.h"}
            }
            progress = root / "progress.sqlite3"
            ProgressSQLiteStore.create_from_mapping(
                progress, data, cutover_pair_id="pure-preview"
            )
            issues = root / "issues.sqlite3"
            create_issue_database(issues, empty_ledger(), cutover_pair_id="pure-issue")
            request = payload()
            request["read_dependencies"] = []
            request["writable_paths"] = ["src/Battlesport/Briefing.h"]
            request["output_root"] = f"build/pure-preview-{uuid.uuid4().hex}"
            args = type("Args", (), {
                "progress": progress,
                "issue_ledger": issues,
                "payload_json": json.dumps(request),
                "payload_file": None,
                "expected_scheduler_revision": 7,
                "expected_semantic_revision": 7,
                "expected_revision": None,
                "apply": False,
            })()
            before = (
                structured_sqlite_observation(progress),
                structured_sqlite_observation(issues),
                sorted(p.name for p in root.iterdir()),
            )
            with patch(
                "_recoil.lib.binja.BinaryNinjaBridge", side_effect=AssertionError("BN")
            ):
                result = create_explicit_maintenance_work(args)
            after = (
                structured_sqlite_observation(progress),
                structured_sqlite_observation(issues),
                sorted(p.name for p in root.iterdir()),
            )
            self.assertFalse(result["commit"]["applied"])
            self.assertEqual(before, after)

    def test_26_cross_ledger_conflict_rolls_back_both_sqlite_ledgers(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data = empty_progress_document()
            data["revision"] = 7
            data["verification_targets"]["recoil:vc5-target:a"] = {
                "registration": {"source_from": "src/Battlesport/Briefing.h"}
            }
            progress = root / "progress.sqlite3"
            ProgressSQLiteStore.create_from_mapping(
                progress, data, cutover_pair_id="rollback-progress"
            )
            issues = root / "issues.sqlite3"
            create_issue_database(issues, empty_ledger(), cutover_pair_id="rollback-issues")
            request = payload()
            request["read_dependencies"] = []
            request["writable_paths"] = ["src/Battlesport/Briefing.h"]
            request["output_root"] = f"build/rollback-{uuid.uuid4().hex}"
            args = type("Args", (), {
                "progress": progress,
                "issue_ledger": issues,
                "payload_json": json.dumps(request),
                "payload_file": None,
                "expected_scheduler_revision": 7,
                "expected_semantic_revision": 7,
                "expected_revision": None,
                "apply": True,
            })()
            before = (
                structured_sqlite_observation(progress),
                structured_sqlite_observation(issues),
            )
            conflict = [{
                "work_item_id": "issue:work:conflict",
                "kind": "resource",
                "paths": ["src/Battlesport/Briefing.h"],
            }]
            with patch(
                "_recoil.commands.workspace_issues.workspace_issue_reservation_conflicts",
                return_value=conflict,
            ), self.assertRaisesRegex(ProgressError, "conflicts with an active"):
                create_explicit_maintenance_work(args)
            self.assertEqual(before, (
                structured_sqlite_observation(progress),
                structured_sqlite_observation(issues),
            ))

    def test_27_unregistered_declaration_debt_is_exact_and_ownerless(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data = fixture(root)
            (root / "src" / "unregistered.h").write_text("struct U;\n", encoding="utf-8")
            request = payload()
            request["writable_paths"].append("src/unregistered.h")
            request["writable_overrides"] = [{
                "path": "src/unregistered.h",
                "relation": "reviewed-unregistered-declaration-debt",
                "selected_scope_id": "recoil:vc5-target:a",
                "related_owner_id": "",
                "evidence": "The selected target includes this declaration but current ownership registration omits it.",
                "rationale": "The user explicitly selected the exact declaration repair seam.",
            }]
            _work_id, work = construct_explicit_maintenance_work_item(
                ProgressDocument(data), request, repo_root=root
            )
            self.assertIn("src/unregistered.h", work["allowed_paths"])
            request["writable_overrides"][0]["related_owner_id"] = "recoil:owner:other"
            with self.assertRaisesRegex(ProgressError, "empty related_owner_id"):
                construct_explicit_maintenance_work_item(
                    ProgressDocument(data), request, repo_root=root
                )

    def test_28_validation_command_is_one_parsed_public_nonmutating_route(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data = fixture(root)
            allowed = payload()
            allowed["validation_command"] = (
                "python -B tools/recoil.py audit workflow-contracts --strict"
            )
            construct_explicit_maintenance_work_item(
                ProgressDocument(data), allowed, repo_root=root
            )
            forbidden = (
                "cmd.exe /c python -B tools/recoil.py verify vc5-order a",
                "python -B tools/recoil.py progress status",
                "python -B tools/recoil.py docs readme-progress",
                "python -B tools/recoil.py verify definitely-not-a-command",
                "python -B other.py verify vc5-order a",
                "python -B tools/recoil.py verify vc5-order a & whoami",
            )
            for command in forbidden:
                request = payload(packet_id=f"recoil:explicit-work:{uuid.uuid4().hex}")
                request["validation_command"] = command
                with self.subTest(command=command), self.assertRaises(ProgressError):
                    construct_explicit_maintenance_work_item(
                        ProgressDocument(data), request, repo_root=root
                    )

    def test_28b_allocating_packet_cannot_handoff_before_marker_activation(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data = fixture(root)
            (root / "build").mkdir()
            progress = root / "progress.sqlite3"
            ProgressSQLiteStore.create_from_mapping(
                progress, deepcopy(data), cutover_pair_id="allocation-pending"
            )
            create_and_reserve_explicit_maintenance_work_item(
                data,
                payload(),
                repo_root=root,
                progress_path=progress,
                operation_nonce="pending",
                issue_ledger_identity={"fixture": "allocation-pending"},
            )
            self.assertNotIn("recoil:explicit-work:a", data["work_items"])
            journal = data["migration"]["explicit_output_allocation_journals"]["rows"][
                "recoil:explicit-work:a"
            ]
            self.assertEqual("allocating", journal["state"])
            self.assertFalse(journal["active_reservation_created"])
            self.assertFalse(journal["normal_claims_installed"])

    def test_28c_allocation_failure_terminalizes_and_removes_owned_root(self) -> None:
        data = empty_progress_document()
        data["revision"] = 7
        data["verification_targets"]["recoil:vc5-target:a"] = {
            "registration": {"source_from": "src/Battlesport/Briefing.h"}
        }
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "build").mkdir()
            progress = root / "progress.sqlite3"
            ProgressSQLiteStore.create_from_mapping(
                progress, data, cutover_pair_id="allocation-failure"
            )
            issues = root / "issues.sqlite3"
            create_issue_database(
                issues, empty_ledger(), cutover_pair_id="allocation-failure-issues"
            )
            output_relative = f"build/allocation-failure-{uuid.uuid4().hex}"
            request = payload()
            request["read_dependencies"] = []
            request["writable_paths"] = ["src/Battlesport/Briefing.h"]
            request["output_root"] = output_relative
            args = type("Args", (), {
                "progress": progress,
                "issue_ledger": issues,
                "payload_json": json.dumps(request),
                "payload_file": None,
                "expected_scheduler_revision": 7,
                "expected_semantic_revision": 7,
                "expected_revision": None,
                "apply": True,
            })()
            observed_marker_roots: list[Path] = []

            def observe_marker_creation(
                allocation: dict[str, object],
                output_root: Path,
            ) -> dict[str, object]:
                self.assertTrue(output_root.is_dir())
                observed_marker_roots.append(output_root.resolve())
                return explicit_output_marker_record(allocation, output_root)

            with (
                patch.object(progress_cli, "REPO_ROOT", root),
                patch(
                    "os.fsync",
                    side_effect=(None, OSError("forced marker failure")),
                ) as fsync,
                patch.object(
                    progress_cli,
                    "explicit_output_marker_record",
                    side_effect=observe_marker_creation,
                ) as marker_record,
                self.assertRaisesRegex(ProgressError, "forced marker failure"),
            ):
                create_explicit_maintenance_work(args)
            self.assertEqual(2, fsync.call_count)
            marker_record.assert_called_once()
            self.assertEqual(
                [(root / output_relative).resolve()],
                observed_marker_roots,
            )
            loaded = ProgressStore(progress).load()
            self.assertNotIn("recoil:explicit-work:a", loaded.collection("work_items"))
            stored = loaded.data["migration"]["explicit_output_allocation_journals"][
                "rows"
            ]["recoil:explicit-work:a"]
            self.assertEqual("failed-allocation", stored["state"])
            self.assertFalse(stored["active_reservation_created"])
            self.assertFalse(stored["normal_claims_installed"])
            self.assertEqual("absent", stored["allocation_failure_receipt"]["cleanup_state"])
            allocation = stored["expected_ownership_marker"]
            self.assertEqual(output_relative, allocation["normalized_output_root"])
            self.assertFalse((root / output_relative).exists())
            self.assertFalse((root / allocation["ownership_sidecar"]).exists())

    def test_28d_public_cleanup_recovery_authenticates_and_removes_journal_root(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data = fixture(root)
            (root / "build").mkdir()
            progress = root / "progress.sqlite3"
            ProgressSQLiteStore.create_from_mapping(
                progress, deepcopy(data), cutover_pair_id="cleanup-recovery"
            )
            issues = root / "issues.sqlite3"
            create_issue_database(
                issues, empty_ledger(), cutover_pair_id="cleanup-recovery-issues"
            )
            metadata = read_issue_metadata(issues)
            issue_identity = {
                "path": str(issues.resolve()),
                "application_id": metadata.application_id,
                "user_version": metadata.user_version,
                "schema_version": metadata.schema_version,
                "ledger_version": metadata.ledger_version,
                "cutover_pair_id": metadata.cutover_pair_id,
            }
            create_and_reserve_explicit_maintenance_work_item(
                data,
                payload(),
                repo_root=root,
                progress_path=progress,
                operation_nonce="cleanup-recovery-operation",
                issue_ledger_identity=issue_identity,
            )
            journal = data["migration"]["explicit_output_allocation_journals"][
                "rows"
            ]["recoil:explicit-work:a"]
            allocation = journal["expected_ownership_marker"]
            output_root = root / allocation["normalized_output_root"]
            sidecar = root / allocation["ownership_sidecar"]
            output_root.mkdir()
            sidecar.write_text(
                json.dumps(allocation, sort_keys=True, separators=(",", ":")) + "\n",
                encoding="utf-8",
            )
            fail_explicit_maintenance_allocation(
                data,
                "recoil:explicit-work:a",
                reason="injected marker creation failure",
                cleanup_state="quarantined",
            )
            sync_packet_store(progress, data)
            vector = ProgressSQLiteStore(progress).read_revision_vector()
            args = type("Args", (), {
                "progress": progress,
                "issue_ledger": issues,
                "id": "recoil:explicit-work:a",
                "expected_scheduler_revision": vector.scheduler_revision,
                "expected_semantic_revision": vector.semantic_revision,
                "expected_revision": None,
                "apply": True,
                "dry_run": False,
            })()
            result = recover_explicit_maintenance_allocation(args)
            self.assertFalse(result["acceptance_changed"])
            self.assertTrue(result["root_verified_absent"])
            self.assertFalse(output_root.exists())
            self.assertFalse(sidecar.exists())
            recovered = ProgressStore(progress).load().data["migration"][
                "explicit_output_allocation_journals"
            ]["rows"]["recoil:explicit-work:a"]
            self.assertEqual("recovered", recovered["state"])
            self.assertNotIn("cleanup_debt", recovered)

    def test_28e_cleanup_debt_cannot_be_cleared_by_caller_boolean(self) -> None:
        data = empty_progress_document()
        with self.assertRaises(TypeError):
            recover_explicit_maintenance_cleanup_debt(
                data,
                "recoil:explicit-work:a",
                root_absent=True,
            )

    def test_29_governed_reader_rejects_legacy_untyped_snapshot(self) -> None:
        legacy = BinaryNinjaSnapshot(True, "t", "c", "r")
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path, fake, _data = self._reserved_binja_json(root, [legacy])
            with patch("_recoil.lib.binja.BinaryNinjaBridge", return_value=fake), self.assertRaisesRegex(BridgeError, "typed authenticated"):
                GovernedBinaryNinjaReadSession(path, "recoil:explicit-work:a")

    def test_30_governed_reader_requires_bound_issue_ledger_identity(self) -> None:
        snap = authenticated_snapshot()
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            data = fixture(root)
            request = payload(kind="read-only-diagnostic")
            request["resources"]["binary_ninja_saved_view_read"] = True
            _reserved, progress = allocate_packet(data, request, root=root)
            sync_packet_store(progress, data)
            fake = FakeBridge(snapshots=[snap])
            with patch("_recoil.lib.binja.BinaryNinjaBridge", return_value=fake), self.assertRaisesRegex(BridgeError, "bound issue-ledger identity"):
                GovernedBinaryNinjaReadSession(progress, "recoil:explicit-work:a")
            self.assertEqual([], fake.calls)

    def test_31_public_binja_cli_unavailable_or_drift_is_nonmutating(self) -> None:
        cases = (
            [BinaryNinjaSnapshot(False, reason="unsupported")],
            [authenticated_snapshot("a"), authenticated_snapshot("b")],
        )
        for snapshots in cases:
            with self.subTest(snapshots=snapshots), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                progress, _issues, fake = self._public_binja_return_fixture(
                    root, list(snapshots)
                )
                before = structured_sqlite_observation(progress)
                stdout = io.StringIO()
                stderr = io.StringIO()
                with patch("_recoil.lib.binja.BinaryNinjaBridge", return_value=fake), patch(
                    "sys.stdout", new=stdout
                ), patch("sys.stderr", new=stderr):
                    code = progress_main([
                        "work", "return-binja",
                        "--id", "recoil:explicit-work:a",
                        "--read-plan-json", json.dumps({
                            "schema": "recoil-governed-binja-read-plan-v1",
                            "requests": [],
                        }),
                        "--result-json", json.dumps({"outcome": "PASS", "changed_paths": []}),
                        "--progress", str(progress),
                        "--expected-revision", str(ProgressStore(progress).load().revision), "--apply", "--json",
                    ])
                self.assertEqual(2, code)
                self.assertIn("Binary Ninja", stderr.getvalue())
                self.assertEqual(before, structured_sqlite_observation(progress))


if __name__ == "__main__":
    unittest.main()
