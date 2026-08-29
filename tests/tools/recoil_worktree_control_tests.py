from __future__ import annotations

import argparse
from contextlib import redirect_stderr, redirect_stdout
from dataclasses import replace
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

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))

from _recoil.commands import worktree_control as command
from _recoil.commands import workspace_issues
from _recoil.commands.workspace_packet_handoff import (
    WorkspacePacketHandoffError,
    _compact_reserved_packet,
    render_workspace_issue_handoff,
)
from _recoil.lib.worktree_control import (
    BUILD_ROOT_MARKER_NAME,
    PROGRESS_ADAPTER_REASON,
    PROGRESS_ADAPTER_STATE,
    WorktreeControlError,
    _capture_normalized_path_projection_child,
    authenticate_build_root,
    authenticate_temporary_build_root,
    canonical_validation_environment,
    common_git_directory,
    create_temporary_build_root,
    list_git_worktrees,
    parse_worktree_list_porcelain,
    reauthenticate_canonical_control_root,
    resolve_canonical_control_root,
    resolve_topology,
    routed_machine_local_path,
)
from _recoil.lib import worktree_control as worktree_library
from _recoil.lib.windows_identity import WindowsIdentityError


def git(root: Path, *args: str, check: bool = True) -> str:
    result = subprocess.run(
        ["git", *args], cwd=root, check=check, capture_output=True,
        text=True, encoding="utf-8",
    )
    return result.stdout


class WorktreeControlTests(unittest.TestCase):
    packet_id = "issue:work:wsi-20260826-001:linked-worktree-test"
    root_routing_environment = (
        "RECOIL_CANONICAL_ROOT",
        "RECOIL_EXECUTION_WORKTREE_ROOT",
        "RECOIL_EXTERNAL_BUILD_ROOT",
    )

    def setUp(self) -> None:
        inherited_root_routing = {
            name: os.environ.pop(name)
            for name in self.root_routing_environment
            if name in os.environ
        }

        def restore_root_routing() -> None:
            for name in self.root_routing_environment:
                os.environ.pop(name, None)
            os.environ.update(inherited_root_routing)

        self.addCleanup(restore_root_routing)
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        root = Path(self.temp.name)
        self.repo = root / "repo"
        self.repo.mkdir()
        git(self.repo, "init", "-q", "-b", "master")
        git(self.repo, "config", "user.email", "recoil-tests@example.invalid")
        git(self.repo, "config", "user.name", "Recoil Tests")
        (self.repo / "inside.txt").write_text("inside\n", encoding="utf-8")
        (self.repo / "outside.txt").write_text("outside\n", encoding="utf-8")
        (self.repo / "test_ok.py").write_text(
            "import unittest\n\n"
            "class TestOk(unittest.TestCase):\n"
            "    def test_ok(self):\n"
            "        self.assertTrue(True)\n",
            encoding="utf-8",
        )
        git(self.repo, "add", ".")
        git(self.repo, "commit", "-q", "-m", "baseline")
        self.ledger = root / "issues.json"
        issue = {
            "id": "WSI-20260826-001", "status": "in-progress",
            "kind": "improvement", "severity": "high",
            "created": "2026-08-26T00:00:00Z", "updated": "2026-08-26T00:00:00Z",
            "summary": "worktrees", "area": "tools", "impact": "isolation",
            "next_action": "test", "requested_change": "worktrees",
            "benefit": "isolation", "commands": [], "files": [], "tags": [],
            "history": [],
        }
        claims = workspace_issues.normalize_resource_claims([
            {"kind": "path", "id": "inside.txt", "access": "write"},
            {"kind": "workspace", "id": "issue-ledger", "access": "write"},
            {"kind": "tracker", "id": "recoil", "access": "read"},
        ])
        packet = {
            "id": self.packet_id, "issue_id": issue["id"], "state": "ready",
            "handoff_role": "recoil_tool_maintainer", "scope": "test worktree",
            "next_command": "python -B -m unittest test_ok",
            "allowed_paths": ["inside.txt"], "forbidden_paths": ["outside.txt"],
            "validation_commands": ["python -B -m unittest test_ok"],
            "required_return_fields": ["changed_paths"], "resource_claims": claims,
            "reservation_id": None, "created": "2026-08-26T00:00:00Z",
            "updated": "2026-08-26T00:00:00Z", "semantic_contract_version": 1,
            "scope_versions": [], "role_contract_version": 1,
            "validation_command_contract_version": 1,
        }
        self.ledger.write_text(json.dumps({
            "version": 2, "revision": 0, "id_sequences": {},
            "issues": [issue], "work_packets": [packet], "reservations": [],
        }), encoding="utf-8")
        self.progress = root / "progress.sqlite3"
        self.progress.write_bytes(b"fixture")
        self.progress_receipt = {
            "path": str(self.progress),
            "revision_vector": {
                "transaction_revision": 1, "semantic_revision": 1,
                "evidence_generation_revision": 1, "scheduler_revision": 1,
            },
            "schema_version": 5, "user_version": 1,
            "integrity_check": ["ok"], "foreign_key_violation_count": 0,
            "evidence_row_count": 0, "certificate_evidence_row_count": 0,
            "work_item_row_count": 0, "active_progress_reservation_count": 0,
        }

    def test_foreign_git_fixtures_ignore_inherited_doctor_root_routing(self) -> None:
        self.assertTrue(all(
            name not in os.environ
            for name in self.root_routing_environment
        ))

    def patched_issue_authority(self):
        return mock.patch.multiple(
            workspace_issues,
            combined_lease_view=mock.DEFAULT,
            _capture_protected_progress_database=mock.DEFAULT,
        )

    def detached_worktree(self, name: str) -> Path:
        path = Path(self.temp.name) / name
        git(self.repo, "worktree", "add", "--detach", str(path), "master")

        def cleanup() -> None:
            subprocess.run(
                ["git", "worktree", "unlock", str(path)],
                cwd=self.repo,
                check=False,
                capture_output=True,
            )
            subprocess.run(
                ["git", "worktree", "remove", str(path)],
                cwd=self.repo,
                check=False,
                capture_output=True,
            )

        self.addCleanup(cleanup)
        return path

    def named_worktree(self, name: str, branch: str) -> Path:
        path = Path(self.temp.name) / name
        git(self.repo, "worktree", "add", "-b", branch, str(path), "master")

        def cleanup() -> None:
            subprocess.run(
                ["git", "worktree", "unlock", str(path)],
                cwd=self.repo,
                check=False,
                capture_output=True,
            )
            subprocess.run(
                ["git", "worktree", "remove", str(path)],
                cwd=self.repo,
                check=False,
                capture_output=True,
            )
            subprocess.run(
                ["git", "branch", "-d", branch],
                cwd=self.repo,
                check=False,
                capture_output=True,
            )

        self.addCleanup(cleanup)
        return path

    @staticmethod
    def machine_input(root: Path, relative: str = "support/Recoil.exe") -> Path:
        path = root.joinpath(*relative.split("/"))
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(b"canonical machine-local fixture")
        return path

    def test_canonical_root_unique_discovery_routes_from_linked_execution(self) -> None:
        retail = self.machine_input(self.repo)
        executing = self.detached_worktree("linked-execution")
        self.assertFalse((executing / "support").exists())
        result = resolve_canonical_control_root(
            executing_worktree_root=executing,
            required_machine_local_paths=("support/Recoil.exe",),
        )
        self.assertEqual(self.repo.resolve(strict=True), result.canonical_control_root)
        self.assertEqual(executing.resolve(strict=True), result.executing_worktree_root)
        self.assertEqual("unique-worktree", result.resolution_source)
        self.assertEqual(("support/Recoil.exe",), result.required_machine_local_paths)
        self.assertTrue(retail.is_file())
        self.assertFalse((executing / "support").exists())

    def test_progress_next_sqlite_defaults_route_from_linked_execution(self) -> None:
        relative_paths = (
            ".agent/RECONSTRUCTION_PROGRESS.sqlite3",
            ".agent/WORKSPACE_ISSUES.sqlite3",
        )
        canonical_paths = {
            relative: self.machine_input(self.repo, relative)
            for relative in relative_paths
        }
        executing = self.detached_worktree("linked-progress-next")
        self.assertTrue(
            all(not executing.joinpath(*relative.split("/")).exists()
                for relative in relative_paths)
        )

        with mock.patch.dict(
            os.environ,
            {"RECOIL_CANONICAL_ROOT": str(self.repo)},
            clear=False,
        ):
            routed = {
                relative: routed_machine_local_path(
                    executing_worktree_root=executing,
                    relative_path=relative,
                )
                for relative in relative_paths
            }

        self.assertEqual(
            {relative: path.resolve(strict=True) for relative, path in canonical_paths.items()},
            {relative: path.resolve(strict=True) for relative, path in routed.items()},
        )
        self.assertTrue(
            all(not executing.joinpath(*relative.split("/")).exists()
                for relative in relative_paths)
        )

    def test_canonical_root_executing_root_and_validated_environment(self) -> None:
        self.machine_input(self.repo)
        result = resolve_canonical_control_root(
            executing_worktree_root=self.repo,
            required_machine_local_paths=("support/Recoil.exe",),
        )
        self.assertEqual("executing-root", result.resolution_source)
        build = Path(self.temp.name) / "external-build"
        identity = create_temporary_build_root(build)
        base = {"CL": "/nologo", "TEMP": "preserved", "CUSTOM": "yes"}
        environment = canonical_validation_environment(
            result,
            external_build_root=build,
            expected_external_build_root_identity=identity,
            base_environment=base,
        )
        self.assertEqual("/nologo", environment["CL"])
        self.assertEqual("preserved", environment["TEMP"])
        self.assertEqual("yes", environment["CUSTOM"])
        self.assertEqual(str(self.repo.resolve()), environment["RECOIL_CANONICAL_ROOT"])
        self.assertEqual(str(self.repo.resolve()), environment["RECOIL_EXECUTION_WORKTREE_ROOT"])
        self.assertEqual(str(build.resolve()), environment["RECOIL_EXTERNAL_BUILD_ROOT"])

    def test_empty_explicit_canonical_root_is_rejected(self) -> None:
        self.machine_input(self.repo)
        with self.assertRaisesRegex(WorktreeControlError, "explicit canonical root is empty"):
            resolve_canonical_control_root(
                executing_worktree_root=self.repo,
                required_machine_local_paths=("support/Recoil.exe",),
                explicit_root="",
            )
        with mock.patch.dict(os.environ, {"RECOIL_CANONICAL_ROOT": ""}):
            with self.assertRaisesRegex(WorktreeControlError, "explicit canonical root is empty"):
                resolve_canonical_control_root(
                    executing_worktree_root=self.repo,
                    required_machine_local_paths=("support/Recoil.exe",),
                )

    def test_machine_local_requirements_reject_rewritten_or_escaping_paths(self) -> None:
        invalid = (
            "support\\Recoil.exe",
            "support//Recoil.exe",
            "./support/Recoil.exe",
            "support/../Recoil.exe",
            "C:/support/Recoil.exe",
            "/support/Recoil.exe",
            "support/Recoil.exe\x00suffix",
        )
        for path in invalid:
            with self.subTest(path=path):
                with self.assertRaisesRegex(
                    WorktreeControlError,
                    "must be exact and repository-relative",
                ):
                    resolve_canonical_control_root(
                        executing_worktree_root=self.repo,
                        required_machine_local_paths=(path,),
                    )

    def test_explicit_canonical_root_must_be_listed_and_same_common_directory(self) -> None:
        self.machine_input(self.repo)
        subdirectory = self.repo / "ordinary-subdirectory"
        subdirectory.mkdir()
        with self.assertRaisesRegex(WorktreeControlError, "not an exact listed Git worktree"):
            resolve_canonical_control_root(
                executing_worktree_root=self.repo,
                required_machine_local_paths=("support/Recoil.exe",),
                explicit_root=subdirectory,
            )
        other = Path(self.temp.name) / "other-repository"
        other.mkdir()
        git(other, "init", "-q", "-b", "master")
        git(other, "config", "user.email", "recoil-tests@example.invalid")
        git(other, "config", "user.name", "Recoil Tests")
        (other / "tracked.txt").write_text("other\n", encoding="utf-8")
        git(other, "add", ".")
        git(other, "commit", "-q", "-m", "other")
        self.machine_input(other)
        with self.assertRaisesRegex(WorktreeControlError, "different Git common directory"):
            resolve_canonical_control_root(
                executing_worktree_root=self.repo,
                required_machine_local_paths=("support/Recoil.exe",),
                explicit_root=other,
            )

    def test_detached_packet_integration_and_prunable_roots_are_rejected(self) -> None:
        detached = self.detached_worktree("detached-canonical")
        self.machine_input(detached)
        with self.assertRaisesRegex(WorktreeControlError, "detached worktree"):
            resolve_canonical_control_root(
                executing_worktree_root=self.repo,
                required_machine_local_paths=("support/Recoil.exe",),
                explicit_root=detached,
            )
        packet = self.named_worktree("packet-canonical", "packet/issue/rejected")
        self.machine_input(packet)
        with self.assertRaisesRegex(WorktreeControlError, "packet-associated worktree"):
            resolve_canonical_control_root(
                executing_worktree_root=self.repo,
                required_machine_local_paths=("support/Recoil.exe",),
                explicit_root=packet,
            )
        integration = self.named_worktree(
            "integration-canonical", "integration/arbitrary/rejected"
        )
        self.machine_input(integration)
        with self.assertRaisesRegex(WorktreeControlError, "integration worktree"):
            resolve_canonical_control_root(
                executing_worktree_root=self.repo,
                required_machine_local_paths=("support/Recoil.exe",),
                explicit_root=integration,
            )
        self.machine_input(self.repo)
        rows = list(list_git_worktrees(self.repo))
        rows[0] = replace(rows[0], prunable=True, prunable_reason="fixture")
        with mock.patch.object(worktree_library, "list_git_worktrees", return_value=tuple(rows)):
            with self.assertRaisesRegex(WorktreeControlError, "prunable"):
                resolve_canonical_control_root(
                    executing_worktree_root=self.repo,
                    required_machine_local_paths=("support/Recoil.exe",),
                    explicit_root=self.repo,
                )

    def test_missing_directory_ambiguous_and_identity_failures_are_closed(self) -> None:
        with self.assertRaisesRegex(WorktreeControlError, "machine-local input is missing"):
            resolve_canonical_control_root(
                executing_worktree_root=self.repo,
                required_machine_local_paths=("support/Recoil.exe",),
                explicit_root=self.repo,
            )
        (self.repo / "support" / "Recoil.exe").mkdir(parents=True)
        with self.assertRaisesRegex(WorktreeControlError, "not an ordinary file"):
            resolve_canonical_control_root(
                executing_worktree_root=self.repo,
                required_machine_local_paths=("support/Recoil.exe",),
                explicit_root=self.repo,
            )
        (self.repo / "support" / "Recoil.exe").rmdir()
        self.machine_input(self.repo)
        second = self.named_worktree("second-canonical", "second-canonical")
        self.machine_input(second)
        executing = self.detached_worktree("ambiguous-execution")
        with self.assertRaisesRegex(WorktreeControlError, "ambiguous"):
            resolve_canonical_control_root(
                executing_worktree_root=executing,
                required_machine_local_paths=("support/Recoil.exe",),
            )
        with mock.patch.object(
            worktree_library,
            "physical_identity",
            side_effect=WindowsIdentityError("injected identity failure"),
        ):
            with self.assertRaisesRegex(WorktreeControlError, "cannot authenticate"):
                resolve_canonical_control_root(
                    executing_worktree_root=self.repo,
                    required_machine_local_paths=("support/Recoil.exe",),
                    explicit_root=self.repo,
                )

    def test_reparse_machine_input_is_rejected_before_resolution(self) -> None:
        target = Path(self.temp.name) / "outside-retail.exe"
        target.write_bytes(b"outside")
        link = self.repo / "support" / "Recoil.exe"
        link.parent.mkdir()
        try:
            os.symlink(target, link)
        except OSError as exc:
            self.skipTest(f"symlink creation unavailable: {exc}")
        with self.assertRaisesRegex(WorktreeControlError, "reparse/symlink"):
            resolve_canonical_control_root(
                executing_worktree_root=self.repo,
                required_machine_local_paths=("support/Recoil.exe",),
                explicit_root=self.repo,
            )

    def test_canonical_file_and_transient_build_replacement_are_rejected(self) -> None:
        retail = self.machine_input(self.repo)
        result = resolve_canonical_control_root(
            executing_worktree_root=self.repo,
            required_machine_local_paths=("support/Recoil.exe",),
        )
        original = retail.with_name("Recoil-original.exe")
        retail.rename(original)
        retail.write_bytes(b"replacement")
        with self.assertRaisesRegex(WorktreeControlError, "physical object was replaced"):
            reauthenticate_canonical_control_root(result)

        build = Path(self.temp.name) / "authenticated-build"
        identity = create_temporary_build_root(build)
        with self.assertRaisesRegex(WorktreeControlError, "identity is not a directory"):
            authenticate_temporary_build_root(
                build,
                expected_identity=replace(identity, is_directory=False),
            )
        build.rmdir()
        with self.assertRaisesRegex(WorktreeControlError, "disappeared"):
            authenticate_temporary_build_root(build, expected_identity=identity)
        build.mkdir()
        with self.assertRaisesRegex(WorktreeControlError, "physical object was replaced"):
            authenticate_temporary_build_root(build, expected_identity=identity)

    def test_live_sqlite_identity_observation_allows_concurrent_reader(self) -> None:
        progress = self.repo / ".agent" / "RECONSTRUCTION_PROGRESS.sqlite3"
        progress.parent.mkdir()
        connection = sqlite3.connect(progress)
        self.addCleanup(connection.close)
        connection.execute("CREATE TABLE metadata (key TEXT PRIMARY KEY, value TEXT)")
        connection.execute("INSERT INTO metadata VALUES ('revision', '1')")
        connection.commit()
        connection.execute("BEGIN")
        self.assertEqual(
            "1",
            connection.execute(
                "SELECT value FROM metadata WHERE key = 'revision'"
            ).fetchone()[0],
        )
        resolution = resolve_canonical_control_root(
            executing_worktree_root=self.repo,
            required_machine_local_paths=(
                ".agent/RECONSTRUCTION_PROGRESS.sqlite3",
            ),
        )
        reauthenticated = reauthenticate_canonical_control_root(resolution)
        self.assertEqual(
            resolution.required_path_identities,
            reauthenticated.required_path_identities,
        )

    @unittest.skipUnless(os.name == "nt", "Windows share-mode regression")
    def test_live_sqlite_observation_does_not_use_share_denying_file_handle(self) -> None:
        progress = self.machine_input(
            self.repo, ".agent/RECONSTRUCTION_PROGRESS.sqlite3"
        )
        original = worktree_library.physical_identity
        strict_identity = original(progress, directory=False)

        def deny_file_handle(path, *, directory=None):
            if directory is False:
                raise WindowsIdentityError(32, "injected sharing violation")
            return original(path, directory=directory)

        with mock.patch.object(
            worktree_library,
            "physical_identity",
            side_effect=deny_file_handle,
        ):
            resolution = resolve_canonical_control_root(
                executing_worktree_root=self.repo,
                required_machine_local_paths=(
                    ".agent/RECONSTRUCTION_PROGRESS.sqlite3",
                ),
            )
            reauthenticate_canonical_control_root(resolution)
        observed_identity = dict(resolution.required_path_identities)[
            ".agent/RECONSTRUCTION_PROGRESS.sqlite3"
        ]
        self.assertEqual(strict_identity.volume_identity, observed_identity.volume_identity)
        self.assertEqual(strict_identity.file_id, observed_identity.file_id)
        self.assertTrue(progress.is_file())

    def create(self) -> dict[str, object]:
        with mock.patch.object(
            workspace_issues, "combined_lease_view",
            return_value={"incomplete_reservations": [], "conflicts": []},
        ), mock.patch.object(
            workspace_issues, "_capture_protected_progress_database",
            return_value=dict(self.progress_receipt),
        ):
            return command.create_issue_packet_worktree(
                repository_root=self.repo, ledger_path=self.ledger,
                progress_path=self.progress, packet_id=self.packet_id,
                expected_revision=0, apply=True,
            )

    def create_with_worker_commit(self) -> tuple[dict[str, object], Path, str]:
        created = self.create()
        packet_root = Path(created["worktree_root"])
        (packet_root / "inside.txt").write_text("changed\n", encoding="utf-8")
        git(packet_root, "add", "--", "inside.txt")
        git(packet_root, "commit", "-q", "-m", f"packet result {self.packet_id}")
        return created, packet_root, git(packet_root, "rev-parse", "HEAD").strip()

    def close_packet(self, *, outcome: str) -> None:
        document = json.loads(self.ledger.read_text(encoding="utf-8"))
        expected_revision = int(document["revision"])
        close_args = argparse.Namespace(
            ledger=str(self.ledger), progress=self.progress, id=self.packet_id,
            expected_revision=expected_revision, apply=True, dry_run=False,
            outcome=outcome, evidence_id=[],
        )
        out, err = io.StringIO(), io.StringIO()
        with mock.patch.object(workspace_issues, "REPO_ROOT", self.repo), mock.patch.object(
            workspace_issues, "_capture_protected_progress_database",
            return_value=dict(self.progress_receipt),
        ), redirect_stdout(out), redirect_stderr(err):
            self.assertEqual(0, workspace_issues.command_work_close(close_args), err.getvalue())

    def assert_no_integration_residue(self) -> None:
        topology = resolve_topology(self.repo)
        self.assertEqual(2, len(topology.worktrees))
        self.assertFalse(any(
            row.branch and row.branch.startswith("integration/recoil-worktree/")
            for row in topology.worktrees
        ))
        self.assertFalse(any(
            row.startswith("integration/recoil-worktree/")
            for row in git(self.repo, "branch", "--format=%(refname:short)").splitlines()
        ))

    def test_worktree_porcelain_parser_retains_association(self) -> None:
        reason = (
            "recoil-packet-worktree-v1|issue|packet-id|"
            + str((Path(self.temp.name) / "build").resolve())
        )
        parsed = parse_worktree_list_porcelain(
            "worktree " + str(self.repo.resolve()) + "\n"
            "HEAD deadbeef\nbranch refs/heads/packet/test\n"
            "locked " + reason + "\n\n"
        )
        self.assertEqual(1, len(parsed))
        self.assertEqual("packet-id", parsed[0].association.packet_id)
        self.assertTrue(parsed[0].locked)

    def test_worktree_porcelain_parser_retains_progress_authority(self) -> None:
        reason = (
            "recoil-packet-worktree-v1|progress|recoil:work:packet|"
            + str((Path(self.temp.name) / "progress-build").resolve())
        )
        parsed = parse_worktree_list_porcelain(
            "worktree " + str(self.repo.resolve()) + "\n"
            "HEAD deadbeef\nbranch refs/heads/packet/progress/test\n"
            "locked " + reason + "\n\n"
        )
        self.assertEqual("progress", parsed[0].association.authority)

    def test_progress_adapter_is_native_and_standalone_create_is_rejected(self) -> None:
        result = command._status_projection(
            repository_root=self.repo, ledger_path=self.ledger
        )
        self.assertEqual(PROGRESS_ADAPTER_STATE, result["progress_adapter"]["state"])
        self.assertEqual(PROGRESS_ADAPTER_REASON, result["progress_adapter"]["reason"])
        args = argparse.Namespace(
            authority="progress", id="progress:packet", ledger=str(self.ledger),
            progress=self.progress, expected_revision=0, apply=True,
        )
        stderr = io.StringIO()
        with mock.patch.object(command, "REPO_ROOT", self.repo), redirect_stderr(stderr):
            self.assertEqual(2, command.command_create(args))
        self.assertIn("claim-current", stderr.getvalue())

    def test_abandonment_cli_requires_parent_supplied_guard_fields(self) -> None:
        parser = command.build_parser()
        parsed = parser.parse_args([
            "retire", "--id", self.packet_id, "--apply",
            "--outcome", "abandoned-unmerged",
            "--reason", "reviewed abandonment",
            "--expected-tip", "a" * 40,
            "--parent-reviewed-abandonment",
            "--discard-unmerged-without-tag",
            "--confirm-discard-without-tag",
            f"discard-unmerged:{self.packet_id}:{'a' * 40}",
        ])
        self.assertEqual("abandoned-unmerged", parsed.outcome)
        self.assertEqual("a" * 40, parsed.expected_tip)
        self.assertEqual("reviewed abandonment", parsed.reason)
        self.assertTrue(parsed.parent_reviewed_abandonment)
        self.assertTrue(parsed.discard_unmerged_without_tag)

    def test_create_handoff_validate_integrate_close_and_retire(self) -> None:
        created = self.create()
        packet_root = Path(created["worktree_root"])
        build_root = Path(created["external_build_root"])
        self.assertTrue(packet_root.is_dir())
        self.assertTrue((build_root / BUILD_ROOT_MARKER_NAME).is_file())
        authenticate_build_root(build_root, authority="issue", packet_id=self.packet_id)
        handoff = render_workspace_issue_handoff(
            repository_root=self.repo, issue_ledger_path=self.ledger,
            packet_id=self.packet_id,
        )["work_item"]
        self.assertEqual(str(packet_root.resolve()), handoff["worktree_root"])
        self.assertEqual(str(build_root.resolve()), handoff["external_build_root"])
        self.assertTrue(handoff["git_restrictions"]["worker_may_create_one_packet_commit"])
        self.assertFalse(handoff["git_restrictions"]["worker_branch_worktree_integration_allowed"])

        (packet_root / "inside.txt").write_text("changed\n", encoding="utf-8")
        git(packet_root, "add", "--", "inside.txt")
        git(packet_root, "commit", "-q", "-m", f"packet result {self.packet_id}")
        validated = command.validate_issue_packet_worktree(
            repository_root=self.repo, ledger_path=self.ledger, packet_id=self.packet_id
        )
        self.assertEqual(["inside.txt"], validated["changed_paths"])
        master_before = git(self.repo, "rev-parse", "master").strip()
        integrated = command.integrate_issue_packet_worktree(
            repository_root=self.repo, ledger_path=self.ledger,
            packet_id=self.packet_id, apply=True,
        )
        self.assertNotEqual(master_before, integrated["master_after"])
        self.assertEqual("changed\n", (self.repo / "inside.txt").read_text(encoding="utf-8"))
        integrated_status = command._status_projection(
            repository_root=self.repo, ledger_path=self.ledger
        )
        self.assertIn(
            "merged-packet-branch-retained",
            {row["kind"] for row in integrated_status["branch_hygiene_findings"]},
        )

        close_args = argparse.Namespace(
            ledger=str(self.ledger), progress=self.progress, id=self.packet_id,
            expected_revision=1, apply=True, dry_run=False, outcome="closed",
            evidence_id=[],
        )
        out, err = io.StringIO(), io.StringIO()
        with mock.patch.object(workspace_issues, "REPO_ROOT", self.repo), mock.patch.object(
            workspace_issues, "_capture_protected_progress_database",
            return_value=dict(self.progress_receipt),
        ), redirect_stdout(out), redirect_stderr(err):
            self.assertEqual(0, workspace_issues.command_work_close(close_args), err.getvalue())
        retired = command.retire_issue_packet_worktree(
            repository_root=self.repo, ledger_path=self.ledger,
            packet_id=self.packet_id, apply=True,
        )
        self.assertTrue(retired["worktree_removed"])
        self.assertTrue(retired["build_root_removed"])
        self.assertFalse(Path(retired["removed_worktree"]).exists())
        self.assertFalse(Path(retired["removed_build_root"]).exists())
        self.assertEqual("completed", retired["worktree_prune"])
        self.assertNotIn(created["branch"], git(self.repo, "branch", "--format=%(refname:short)"))

    def _direct_handoff_command_fixture(
        self,
        command_text: str,
        *,
        validation_commands: list[str] | None = None,
        next_command: str | None = None,
    ) -> tuple[dict[str, object], dict[str, object]]:
        data = json.loads(self.ledger.read_text(encoding="utf-8"))
        packet = dict(data["work_packets"][0])
        claims = packet["resource_claims"]
        reservation_id = f"{self.packet_id}:attempt:1"
        packet.update(
            {
                "state": "active",
                "reservation_id": reservation_id,
                "validation_command_contract_version": 1,
                "validation_commands": (
                    [command_text]
                    if validation_commands is None
                    else validation_commands
                ),
                "next_command": command_text if next_command is None else next_command,
            }
        )
        reservation = {
            "id": reservation_id,
            "packet_id": self.packet_id,
            "state": "active",
            "handoff_role": packet["handoff_role"],
            "resource_claims": claims,
        }
        return packet, reservation

    def _assert_direct_handoff_command_rejected(
        self,
        command_text: str,
        *,
        validation_commands: list[str] | None = None,
        next_command: str | None = None,
    ) -> None:
        packet, reservation = self._direct_handoff_command_fixture(
            command_text,
            validation_commands=validation_commands,
            next_command=next_command,
        )
        with self.assertRaises(WorkspacePacketHandoffError):
            _compact_reserved_packet(
                self.packet_id,
                packet,
                reservation,
                repository_root=self.repo,
            )

    def test_direct_handoff_rejects_direct_script_validation(self) -> None:
        self._assert_direct_handoff_command_rejected(
            "python -B tests/tools/recoil_binja_tests.py"
        )

    def test_direct_handoff_rejects_shell_composition(self) -> None:
        self._assert_direct_handoff_command_rejected(
            "python -B -m unittest test_ok & python -B -m unittest test_ok"
        )

    def test_direct_handoff_rejects_mutating_public_route(self) -> None:
        self._assert_direct_handoff_command_rejected(
            "python tools/recoil.py issue work close --id packet "
            "--expected-revision 1 --apply"
        )

    def test_direct_handoff_rejects_multiple_validation_commands(self) -> None:
        command_text = "python -B -m unittest test_ok"
        self._assert_direct_handoff_command_rejected(
            command_text,
            validation_commands=[command_text, command_text],
        )

    def test_direct_handoff_rejects_nonidentical_next_command(self) -> None:
        self._assert_direct_handoff_command_rejected(
            "python -B -m unittest test_ok",
            next_command="python -m unittest test_ok",
        )

    def test_every_validation_runs_in_integration_worktree_before_fast_forward(self) -> None:
        created, packet_root, packet_tip = self.create_with_worker_commit()
        master_before = git(self.repo, "rev-parse", "master").strip()
        expected_build_parent = resolve_topology(self.repo).build_parent
        calls: list[tuple[Path, str, str, dict[str, str]]] = []

        def observe_validation(command_text, root, *, environment, **_kwargs):
            calls.append((
                Path(root).resolve(),
                git(self.repo, "rev-parse", "master").strip(),
                git(Path(root), "rev-parse", "HEAD").strip(),
                dict(environment),
            ))
            return {
                "command": command_text,
                "cwd": str(Path(root).resolve()),
                "returncode": 0,
                "stdout": "",
                "stderr": "",
                "passed": True,
            }

        with mock.patch.object(command, "_run_validation", side_effect=observe_validation):
            result = command.integrate_issue_packet_worktree(
                repository_root=self.repo,
                ledger_path=self.ledger,
                packet_id=self.packet_id,
                apply=True,
            )
        self.assertEqual(2, len(calls))
        self.assertTrue(all(root != self.repo.resolve() for root, _tip, _head, _env in calls))
        self.assertTrue(all(tip == master_before for _root, tip, _head, _env in calls))
        self.assertTrue(all(
            environment["RECOIL_CANONICAL_ROOT"] == str(self.repo.resolve())
            for _root, _tip, _head, environment in calls
        ))
        self.assertTrue(all(
            environment["RECOIL_EXECUTION_WORKTREE_ROOT"] == str(root)
            for root, _tip, _head, environment in calls
        ))
        self.assertTrue(all(
            Path(environment["RECOIL_EXTERNAL_BUILD_ROOT"]).parent
            == expected_build_parent
            for _root, _tip, _head, environment in calls
        ))
        self.assertEqual(
            0,
            result["post_fast_forward_git_assertions"][
                "validation_subprocesses_after_fast_forward"
            ],
        )
        self.assertEqual(master_before, result["pre_fast_forward_reauthentication"]["master"]["head"])
        self.assertEqual(packet_tip, result["pre_fast_forward_reauthentication"]["packet"]["head"])
        self.assertEqual(result["integration_head"], result["pre_fast_forward_reauthentication"]["integration"]["head"])
        self.assertEqual(result["integration_head"], result["master_after"])
        self.assertEqual(master_before, result["captured_opaque_tips"]["master_input"])
        self.assertEqual(packet_tip, result["captured_opaque_tips"]["packet_input"])
        self.assertEqual(
            result["integration_head"],
            result["captured_opaque_tips"]["master_fast_forward_operand"],
        )
        self.assertEqual(
            "pre-fast-forward-integration-worktree",
            result["validation_execution_phase"],
        )
        self.assertTrue(result["validation_completed_before_master_advance"])
        common = result["pre_fast_forward_reauthentication"]["common_git_directory"]
        self.assertTrue(all(
            value == common
            for value in result["post_fast_forward_git_assertions"][
                "common_git_directories"
            ].values()
        ))
        self.assertEqual(master_before, calls[0][2])
        self.assertEqual(result["integration_head"], calls[1][2])
        self.assertTrue(all(
            row["cwd"] == str(calls[0][0])
            for row in result["integration_validation"] + result["post_integration_validation"]
        ))

    def test_first_validation_failure_leaves_master_and_no_integration_residue(self) -> None:
        self.create_with_worker_commit()
        master_before = git(self.repo, "rev-parse", "master").strip()

        def fail_first(command_text, root, **_kwargs):
            return {
                "command": command_text, "cwd": str(root), "returncode": 9,
                "stdout": "", "stderr": "injected", "passed": False,
            }

        with mock.patch.object(command, "_run_validation", side_effect=fail_first):
            with self.assertRaisesRegex(WorktreeControlError, "validation failed"):
                command.integrate_issue_packet_worktree(
                    repository_root=self.repo,
                    ledger_path=self.ledger,
                    packet_id=self.packet_id,
                    apply=True,
                )
        self.assertEqual(master_before, git(self.repo, "rev-parse", "master").strip())
        self.assert_no_integration_residue()

    def test_parent_selected_validation_failure_precedes_master_advance(self) -> None:
        self.create_with_worker_commit()
        master_before = git(self.repo, "rev-parse", "master").strip()

        def fail_parent(command_text, root, *, require_public_route, **_kwargs):
            return {
                "command": command_text,
                "cwd": str(root),
                "returncode": 7 if require_public_route else 0,
                "stdout": "",
                "stderr": "injected parent validation failure" if require_public_route else "",
                "passed": not require_public_route,
            }

        with mock.patch.object(command, "_run_validation", side_effect=fail_parent):
            with self.assertRaisesRegex(WorktreeControlError, "validation failed"):
                command.integrate_issue_packet_worktree(
                    repository_root=self.repo,
                    ledger_path=self.ledger,
                    packet_id=self.packet_id,
                    additional_commands=(
                        "python tools/recoil.py audit workflow-contracts --strict --json",
                    ),
                    apply=True,
                )
        self.assertEqual(master_before, git(self.repo, "rev-parse", "master").strip())
        self.assert_no_integration_residue()

    def test_common_directory_drift_blocks_pre_fast_forward_reauthentication(self) -> None:
        created, packet_root, packet_tip = self.create_with_worker_commit()
        expected_common = common_git_directory(self.repo)
        with mock.patch.object(
            command,
            "common_git_directory",
            side_effect=(expected_common, self.repo.resolve(strict=True)),
        ):
            with self.assertRaisesRegex(WorktreeControlError, "common-directory identity"):
                command._reauthenticate_integration_before_fast_forward(
                    expected_common_git_directory=expected_common,
                    master_root=self.repo,
                    master_tip=git(self.repo, "rev-parse", "master").strip(),
                    packet_root=packet_root,
                    packet_branch=str(created["branch"]),
                    packet_tip=packet_tip,
                    integration_root=packet_root,
                    integration_branch=str(created["branch"]),
                    integration_tip=packet_tip,
                )

    def test_last_validation_failure_leaves_master_and_no_integration_residue(self) -> None:
        self.create_with_worker_commit()
        master_before = git(self.repo, "rev-parse", "master").strip()
        call_index = 0

        def fail_last(command_text, root, **_kwargs):
            nonlocal call_index
            current = call_index
            call_index += 1
            return {
                "command": command_text, "cwd": str(root),
                "returncode": 9 if current == 1 else 0,
                "stdout": "", "stderr": "injected" if current == 1 else "",
                "passed": current != 1,
            }

        with mock.patch.object(command, "_run_validation", side_effect=fail_last):
            with self.assertRaisesRegex(
                command.IntegrationValidationError,
                "post-integration-validation failed",
            ) as captured:
                command.integrate_issue_packet_worktree(
                    repository_root=self.repo,
                    ledger_path=self.ledger,
                    packet_id=self.packet_id,
                    apply=True,
                )
        self.assertEqual(
            "post-integration-validation",
            captured.exception.receipt["validation_phase"],
        )
        self.assertEqual(
            9,
            captured.exception.receipt["failed_validation_results"][0]["returncode"],
        )
        self.assertEqual(master_before, git(self.repo, "rev-parse", "master").strip())
        self.assert_no_integration_residue()

    def test_reauthentication_failure_leaves_master_and_no_integration_residue(self) -> None:
        self.create_with_worker_commit()
        master_before = git(self.repo, "rev-parse", "master").strip()
        with mock.patch.object(
            command,
            "_reauthenticate_integration_before_fast_forward",
            side_effect=WorktreeControlError("injected reauthentication failure"),
        ):
            with self.assertRaisesRegex(WorktreeControlError, "reauthentication failure"):
                command.integrate_issue_packet_worktree(
                    repository_root=self.repo,
                    ledger_path=self.ledger,
                    packet_id=self.packet_id,
                    apply=True,
                )
        self.assertEqual(master_before, git(self.repo, "rev-parse", "master").strip())
        self.assert_no_integration_residue()

    def test_fast_forward_failure_leaves_master_unchanged(self) -> None:
        self.create_with_worker_commit()
        master_before = git(self.repo, "rev-parse", "master").strip()
        original_run = subprocess.run

        def fail_fast_forward(arguments, *args, **kwargs):
            if list(arguments[:3]) == ["git", "merge", "--ff-only"]:
                return subprocess.CompletedProcess(arguments, 9, "", "injected fast-forward failure")
            return original_run(arguments, *args, **kwargs)

        with mock.patch.object(command.subprocess, "run", side_effect=fail_fast_forward):
            with self.assertRaisesRegex(WorktreeControlError, "master fast-forward failed"):
                command.integrate_issue_packet_worktree(
                    repository_root=self.repo,
                    ledger_path=self.ledger,
                    packet_id=self.packet_id,
                    apply=True,
                )
        self.assertEqual(master_before, git(self.repo, "rev-parse", "master").strip())
        self.assert_no_integration_residue()

    def test_disappeared_transient_build_root_blocks_before_master_advance(self) -> None:
        self.create_with_worker_commit()
        master_before = git(self.repo, "rev-parse", "master").strip()

        def remove_build(command_text, root, *, environment, **_kwargs):
            build = Path(environment["RECOIL_EXTERNAL_BUILD_ROOT"])
            build.rmdir()
            return {
                "command": command_text,
                "cwd": str(root),
                "returncode": 0,
                "stdout": "",
                "stderr": "",
                "passed": True,
            }

        with mock.patch.object(command, "_run_validation", side_effect=remove_build):
            with self.assertRaisesRegex(WorktreeControlError, "build root disappeared"):
                command.integrate_issue_packet_worktree(
                    repository_root=self.repo,
                    ledger_path=self.ledger,
                    packet_id=self.packet_id,
                    apply=True,
                )
        self.assertEqual(master_before, git(self.repo, "rev-parse", "master").strip())
        self.assert_no_integration_residue()

    def test_replaced_transient_build_root_blocks_before_master_advance(self) -> None:
        self.create_with_worker_commit()
        master_before = git(self.repo, "rev-parse", "master").strip()
        replacement_parent = Path(self.temp.name) / "replaced-build-roots"
        replacement_parent.mkdir()

        def replace_build(command_text, root, *, environment, **_kwargs):
            build = Path(environment["RECOIL_EXTERNAL_BUILD_ROOT"])
            saved = replacement_parent / build.name
            build.rename(saved)
            build.mkdir()
            return {
                "command": command_text,
                "cwd": str(root),
                "returncode": 0,
                "stdout": "",
                "stderr": "",
                "passed": True,
            }

        with mock.patch.object(command, "_run_validation", side_effect=replace_build):
            with self.assertRaisesRegex(WorktreeControlError, "physical object was replaced"):
                command.integrate_issue_packet_worktree(
                    repository_root=self.repo,
                    ledger_path=self.ledger,
                    packet_id=self.packet_id,
                    apply=True,
                )
        self.assertEqual(master_before, git(self.repo, "rev-parse", "master").strip())
        self.assert_no_integration_residue()

    def test_integrated_retirement_rejects_abandonment_controls(self) -> None:
        self.create_with_worker_commit()
        command.integrate_issue_packet_worktree(
            repository_root=self.repo,
            ledger_path=self.ledger,
            packet_id=self.packet_id,
            apply=True,
        )
        self.close_packet(outcome="closed")
        with self.assertRaisesRegex(WorktreeControlError, "does not accept abandonment controls"):
            command.retire_issue_packet_worktree(
                repository_root=self.repo,
                ledger_path=self.ledger,
                packet_id=self.packet_id,
                apply=True,
                outcome="integrated",
                reason="not valid for an integrated retirement",
            )

    def test_retirement_requires_exactly_one_checkout_for_packet_branch(self) -> None:
        created, _packet_root, packet_tip = self.create_with_worker_commit()
        self.close_packet(outcome="abandoned")
        topology = resolve_topology(self.repo)
        packet_row = next(
            row for row in topology.worktrees if row.branch == created["branch"]
        )
        duplicate_checkout = replace(
            packet_row,
            root=self.repo.resolve(strict=True),
            association=None,
            locked=False,
            lock_reason=None,
        )
        duplicate_topology = replace(
            topology,
            worktrees=topology.worktrees + (duplicate_checkout,),
        )
        with mock.patch.object(
            command, "resolve_topology", return_value=duplicate_topology
        ):
            with self.assertRaisesRegex(
                WorktreeControlError, "exactly one worktree checkout"
            ):
                command.retire_issue_packet_worktree(
                    repository_root=self.repo,
                    ledger_path=self.ledger,
                    packet_id=self.packet_id,
                    apply=True,
                    outcome="abandoned-unmerged",
                    reason="reviewed duplicate checkout fixture",
                    expected_tip=packet_tip,
                    parent_reviewed_abandonment=True,
                    archive_tag="archive/packet/test/duplicate-checkout",
                )

    def test_abandoned_unmerged_retirement_archives_exact_tip_then_cleans_state(self) -> None:
        created, _packet_root, packet_tip = self.create_with_worker_commit()
        self.close_packet(outcome="abandoned")
        tag = "archive/packet/linked-worktree-test/reviewed-abandonment"
        result = command.retire_issue_packet_worktree(
            repository_root=self.repo,
            ledger_path=self.ledger,
            packet_id=self.packet_id,
            apply=True,
            outcome="abandoned-unmerged",
            reason="Reviewed fixture abandonment after a nonintegrated worker proposal.",
            expected_tip=packet_tip,
            archive_tag=tag,
            parent_reviewed_abandonment=True,
        )
        self.assertTrue(result["passed"])
        self.assertFalse(result["partial_retirement"])
        self.assertEqual("tag", git(self.repo, "cat-file", "-t", tag).strip())
        self.assertEqual(packet_tip, git(self.repo, "rev-parse", f"{tag}^{{commit}}").strip())
        annotation = git(self.repo, "cat-file", "tag", tag)
        self.assertIn(self.packet_id, annotation)
        self.assertIn("Reviewed fixture abandonment", annotation)
        self.assertIn("worktree-pruned", result["completed_steps"])
        self.assertFalse(Path(created["worktree_root"]).exists())
        self.assertFalse(Path(created["external_build_root"]).exists())
        self.assertNotIn(created["branch"], git(self.repo, "branch", "--format=%(refname:short)"))

    def test_abandoned_unmerged_retirement_accepts_exact_discard_confirmation(self) -> None:
        created, _packet_root, packet_tip = self.create_with_worker_commit()
        self.close_packet(outcome="abandoned")
        confirmation = f"discard-unmerged:{self.packet_id}:{packet_tip}"
        result = command.retire_issue_packet_worktree(
            repository_root=self.repo,
            ledger_path=self.ledger,
            packet_id=self.packet_id,
            apply=True,
            outcome="abandoned-unmerged",
            reason="Reviewed discard without an archival tag for an isolated fixture.",
            expected_tip=packet_tip,
            parent_reviewed_abandonment=True,
            discard_unmerged_without_tag=True,
            discard_confirmation=confirmation,
        )
        self.assertTrue(result["passed"])
        self.assertIsNone(result["archive_tag"])
        self.assertNotIn(created["branch"], git(self.repo, "branch", "--format=%(refname:short)"))

    def test_abandoned_unmerged_retirement_guard_matrix_is_nonmutating(self) -> None:
        created, packet_root, packet_tip = self.create_with_worker_commit()
        self.close_packet(outcome="abandoned")
        branch = str(created["branch"])
        exact_confirmation = f"discard-unmerged:{self.packet_id}:{packet_tip}"
        cases = (
            ({"reason": "", "expected_tip": packet_tip,
              "parent_reviewed_abandonment": True,
              "discard_unmerged_without_tag": True,
              "discard_confirmation": exact_confirmation},
             "explicit parent reason"),
            ({"reason": "reviewed", "expected_tip": None,
              "parent_reviewed_abandonment": True,
              "discard_unmerged_without_tag": True,
              "discard_confirmation": "anything"}, "exact expected branch tip"),
            ({"reason": "reviewed", "expected_tip": "0" * 40,
              "parent_reviewed_abandonment": True,
              "discard_unmerged_without_tag": True,
              "discard_confirmation": "anything"}, "Git command failed"),
            ({"reason": "reviewed", "expected_tip": packet_tip},
             "parent-reviewed authorization"),
            ({"reason": "reviewed", "expected_tip": packet_tip,
              "parent_reviewed_abandonment": True},
             "exactly one archive tag or explicit discard-without-tag"),
            ({"reason": "reviewed", "expected_tip": packet_tip,
              "parent_reviewed_abandonment": True,
              "archive_tag": "archive/packet/test/one",
              "discard_unmerged_without_tag": True,
              "discard_confirmation": exact_confirmation},
             "exactly one archive tag or explicit discard-without-tag"),
            ({"reason": "reviewed", "expected_tip": packet_tip,
              "parent_reviewed_abandonment": True,
              "discard_unmerged_without_tag": True,
              "discard_confirmation": "wrong"}, "confirmation does not match"),
            ({"reason": "reviewed", "expected_tip": packet_tip,
              "parent_reviewed_abandonment": True,
              "archive_tag": "wrong/tag"}, "archive/packet"),
            ({"reason": "reviewed", "expected_tip": packet_tip,
              "parent_reviewed_abandonment": True,
              "archive_tag": "archive/packet/test/two",
              "discard_confirmation": exact_confirmation},
             "does not accept discard confirmation"),
        )
        for kwargs, message in cases:
            with self.subTest(message=message):
                with self.assertRaisesRegex(WorktreeControlError, message):
                    command.retire_issue_packet_worktree(
                        repository_root=self.repo,
                        ledger_path=self.ledger,
                        packet_id=self.packet_id,
                        apply=True,
                        outcome="abandoned-unmerged",
                        **kwargs,
                    )
                self.assertTrue(packet_root.exists())
                self.assertEqual(packet_tip, git(packet_root, "rev-parse", "HEAD").strip())
                self.assertIn(branch, git(self.repo, "branch", "--format=%(refname:short)"))

    def test_abandoned_unmerged_partial_branch_failure_is_truthful_and_hygiene_visible(self) -> None:
        created, _packet_root, packet_tip = self.create_with_worker_commit()
        self.close_packet(outcome="abandoned")
        confirmation = f"discard-unmerged:{self.packet_id}:{packet_tip}"
        original_run = subprocess.run

        def fail_forced_delete(arguments, *args, **kwargs):
            if list(arguments[:3]) == ["git", "branch", "-D"]:
                return subprocess.CompletedProcess(arguments, 9, "", "injected branch deletion failure")
            return original_run(arguments, *args, **kwargs)

        with mock.patch.object(command.subprocess, "run", side_effect=fail_forced_delete):
            result = command.retire_issue_packet_worktree(
                repository_root=self.repo,
                ledger_path=self.ledger,
                packet_id=self.packet_id,
                apply=True,
                outcome="abandoned-unmerged",
                reason="Reviewed partial-retirement failure fixture.",
                expected_tip=packet_tip,
                parent_reviewed_abandonment=True,
                discard_unmerged_without_tag=True,
                discard_confirmation=confirmation,
            )
        self.assertFalse(result["passed"])
        self.assertTrue(result["partial"])
        self.assertTrue(result["partial_retirement"])
        self.assertEqual(packet_tip, result["expected_branch_tip"])
        self.assertFalse(result["archive_tag_created"])
        self.assertFalse(result["archive_tag_verified"])
        self.assertTrue(result["worktree_removed"])
        self.assertFalse(result["branch_removed"])
        self.assertFalse(result["build_root_removed"])
        self.assertTrue(result["hygiene_expected_to_detect_partial_state"])
        self.assertEqual("branch-delete", result["failed_step"])
        self.assertIn("worktree-removed", result["completed_steps"])
        kinds = {row["kind"] for row in result["hygiene_findings"]}
        self.assertIn("packet-branch-without-worktree", kinds)
        self.assertIn("unassociated-build-root", kinds)
        self.assertTrue(Path(created["external_build_root"]).exists())

    def test_abandonment_rejects_active_merged_and_dirty_packet_state(self) -> None:
        _created, packet_root, packet_tip = self.create_with_worker_commit()
        confirmation = f"discard-unmerged:{self.packet_id}:{packet_tip}"
        with self.assertRaisesRegex(WorktreeControlError, "terminal with no active reservation"):
            command.retire_issue_packet_worktree(
                repository_root=self.repo,
                ledger_path=self.ledger,
                packet_id=self.packet_id,
                apply=True,
                outcome="abandoned-unmerged",
                reason="active packet must not retire",
                expected_tip=packet_tip,
                parent_reviewed_abandonment=True,
                discard_unmerged_without_tag=True,
                discard_confirmation=confirmation,
            )
        self.close_packet(outcome="abandoned")
        (packet_root / "inside.txt").write_text("dirty after close\n", encoding="utf-8")
        with self.assertRaisesRegex(WorktreeControlError, "worktree is not clean"):
            command.retire_issue_packet_worktree(
                repository_root=self.repo,
                ledger_path=self.ledger,
                packet_id=self.packet_id,
                apply=True,
                outcome="abandoned-unmerged",
                reason="dirty packet must not retire",
                expected_tip=packet_tip,
                parent_reviewed_abandonment=True,
                discard_unmerged_without_tag=True,
                discard_confirmation=confirmation,
            )

    def test_abandonment_rejects_branch_already_merged_to_master(self) -> None:
        self.create_with_worker_commit()
        integrated = command.integrate_issue_packet_worktree(
            repository_root=self.repo,
            ledger_path=self.ledger,
            packet_id=self.packet_id,
            apply=True,
        )
        self.close_packet(outcome="abandoned")
        packet_tip = str(integrated["captured_opaque_tips"]["packet_input"])
        with self.assertRaisesRegex(WorktreeControlError, "already merged"):
            command.retire_issue_packet_worktree(
                repository_root=self.repo,
                ledger_path=self.ledger,
                packet_id=self.packet_id,
                apply=True,
                outcome="abandoned-unmerged",
                reason="merged branch is not an abandonment",
                expected_tip=packet_tip,
                parent_reviewed_abandonment=True,
                discard_unmerged_without_tag=True,
                discard_confirmation=(
                    f"discard-unmerged:{self.packet_id}:{packet_tip}"
                ),
            )

    def test_only_guarded_abandonment_contains_force_branch_deletion(self) -> None:
        source = Path(command.__file__).read_text(encoding="utf-8")
        self.assertEqual(1, source.count('"-D"'))
        self.assertIn("fully guarded", source)
        self.assertIn("exact-tip abandoned-unmerged", source)

    def test_normalized_zreader_probe_child_has_explicit_governed_projection(self) -> None:
        with tempfile.TemporaryDirectory(prefix="recoil-zreader-child-") as temporary:
            projection = _capture_normalized_path_projection_child(
                Path(temporary) / "build"
            )
        self.assertTrue(projection["compiled"])
        self.assertEqual("zreader_file_exists", projection["target"])
        self.assertEqual(
            "src/GameZRecoil/zUtil/zutl_zar.cpp", projection["source"]
        )
        self.assertEqual(
            ["?FileExists@zReader@@YIHPBD@Z", "@zReader_FileExists_Wrapper@4"],
            projection["authored_symbol_order"],
        )
        self.assertEqual(
            "ordered-non-file-non-debug-symbols-with-normalized-section-identity-value-type-storage",
            projection["coff_symbol_population_policy"],
        )
        self.assertTrue(all(
            "section_identity" in row and "section_number" not in row
            for row in projection["coff_symbol_population"]
        ))
        self.assertEqual(
            "vc5_o2_ob1_md_gx_facs",
            projection["compile_contract"]["compiler_profile"],
        )
        self.assertFalse(projection["compile_contract"]["whole_link_performed"])
        self.assertTrue(projection["relocation_set_explicit"])
        self.assertEqual(
            [{
                "offset": 5,
                "form": "call",
                "dispatch": "indirect-relocation",
                "target": "__imp___access",
            }],
            projection["functions"]["?FileExists@zReader@@YIHPBD@Z"]["emitted_calls"],
        )

    def test_reservation_failure_compensates_all_created_state(self) -> None:
        with mock.patch.object(
            command, "reserve_issue_work_item", side_effect=ValueError("reservation rejected")
        ):
            with self.assertRaisesRegex(WorktreeControlError, "reservation rejected"):
                command.create_issue_packet_worktree(
                    repository_root=self.repo, ledger_path=self.ledger,
                    progress_path=self.progress, packet_id=self.packet_id,
                    expected_revision=0, apply=True,
                )
        topology = resolve_topology(self.repo)
        self.assertEqual(1, len(topology.worktrees))
        self.assertFalse(Path(str(self.repo) + ".worktrees").exists() and any(Path(str(self.repo) + ".worktrees").iterdir()))
        self.assertFalse(Path(str(self.repo) + ".builds").exists() and any(Path(str(self.repo) + ".builds").iterdir()))
        self.assertEqual(("master",), tuple(git(self.repo, "branch", "--format=%(refname:short)").splitlines()))

    def test_out_of_closure_packet_commit_is_rejected(self) -> None:
        created = self.create()
        packet_root = Path(created["worktree_root"])
        (packet_root / "outside.txt").write_text("changed\n", encoding="utf-8")
        git(packet_root, "add", "--", "outside.txt")
        git(packet_root, "commit", "-q", "-m", f"packet result {self.packet_id}")
        with self.assertRaisesRegex(WorktreeControlError, "writable closure"):
            command.validate_issue_packet_worktree(
                repository_root=self.repo, ledger_path=self.ledger,
                packet_id=self.packet_id,
            )

    def test_dirty_packet_worktree_blocks_handoff(self) -> None:
        created = self.create()
        packet_root = Path(created["worktree_root"])
        (packet_root / "inside.txt").write_text("dirty\n", encoding="utf-8")
        with self.assertRaisesRegex(Exception, "dirty or unmerged"):
            render_workspace_issue_handoff(
                repository_root=self.repo, issue_ledger_path=self.ledger,
                packet_id=self.packet_id,
            )

    def test_replaced_build_root_blocks_handoff_and_hygiene(self) -> None:
        created = self.create()
        build_root = Path(created["external_build_root"])
        original_root = build_root.with_name(build_root.name + "-original")
        build_root.rename(original_root)
        build_root.mkdir()
        (build_root / BUILD_ROOT_MARKER_NAME).write_text(
            (original_root / BUILD_ROOT_MARKER_NAME).read_text(encoding="utf-8"),
            encoding="utf-8",
        )
        with self.assertRaisesRegex(Exception, "physical object was replaced"):
            render_workspace_issue_handoff(
                repository_root=self.repo,
                issue_ledger_path=self.ledger,
                packet_id=self.packet_id,
            )
        status = command._status_projection(
            repository_root=self.repo, ledger_path=self.ledger
        )
        kinds = {row["kind"] for row in status["branch_hygiene_findings"]}
        self.assertIn("build-root-authentication", kinds)

    def test_archival_tag_makes_safety_branch_a_hygiene_finding(self) -> None:
        git(self.repo, "branch", "pre-git-change-control-migration-r3599", "master")
        git(
            self.repo,
            "tag",
            "-a",
            "archive/pre-native-git-r3599",
            "-m",
            "archival test",
            "master",
        )
        status = command._status_projection(
            repository_root=self.repo, ledger_path=self.ledger
        )
        kinds = {row["kind"] for row in status["branch_hygiene_findings"]}
        self.assertIn("safety-branch-after-archive-tag", kinds)

    def test_failed_integration_keeps_master_and_cleans_transient_state(self) -> None:
        created = self.create()
        packet_root = Path(created["worktree_root"])
        (packet_root / "inside.txt").write_text("changed\n", encoding="utf-8")
        git(packet_root, "add", "--", "inside.txt")
        git(packet_root, "commit", "-q", "-m", f"packet result {self.packet_id}")
        document = json.loads(self.ledger.read_text(encoding="utf-8"))
        document["work_packets"][0]["validation_commands"] = [
            "python -B -m unittest definitely_missing_test_module"
        ]
        self.ledger.write_text(json.dumps(document), encoding="utf-8")
        master_before = git(self.repo, "rev-parse", "master").strip()
        with self.assertRaisesRegex(
            command.IntegrationValidationError, "integration-validation failed"
        ) as captured:
            command.integrate_issue_packet_worktree(
                repository_root=self.repo, ledger_path=self.ledger,
                packet_id=self.packet_id, apply=True,
            )
        self.assertEqual(
            "integration-validation",
            captured.exception.receipt["validation_phase"],
        )
        failed = captured.exception.receipt["failed_validation_results"]
        self.assertEqual(1, len(failed))
        self.assertEqual(
            "python -B -m unittest definitely_missing_test_module",
            failed[0]["command"],
        )
        self.assertNotEqual(0, failed[0]["returncode"])
        self.assertEqual(master_before, git(self.repo, "rev-parse", "master").strip())
        topology = resolve_topology(self.repo)
        self.assertEqual(2, len(topology.worktrees))
        self.assertNotIn(
            "integration/recoil-worktree/",
            git(self.repo, "branch", "--format=%(refname:short)"),
        )

    def test_hygiene_detects_associated_worktree_without_packet(self) -> None:
        self.create()
        document = json.loads(self.ledger.read_text(encoding="utf-8"))
        document["work_packets"] = []
        document["reservations"] = []
        self.ledger.write_text(json.dumps(document), encoding="utf-8")
        status = command._status_projection(
            repository_root=self.repo, ledger_path=self.ledger
        )
        kinds = {row["kind"] for row in status["branch_hygiene_findings"]}
        self.assertIn("worktree-without-packet", kinds)

    def test_additional_validation_rejects_shell_composition_before_integration(self) -> None:
        created = self.create()
        packet_root = Path(created["worktree_root"])
        (packet_root / "inside.txt").write_text("changed\n", encoding="utf-8")
        git(packet_root, "add", "--", "inside.txt")
        git(packet_root, "commit", "-q", "-m", f"packet result {self.packet_id}")
        master_before = git(self.repo, "rev-parse", "master").strip()
        with self.assertRaisesRegex(WorktreeControlError, "shell composition"):
            command.integrate_issue_packet_worktree(
                repository_root=self.repo,
                ledger_path=self.ledger,
                packet_id=self.packet_id,
                additional_commands=(
                    "python -B tools/recoil.py audit agent-surface --strict; echo unsafe",
                ),
                apply=True,
            )
        self.assertEqual(master_before, git(self.repo, "rev-parse", "master").strip())
        self.assertNotIn(
            "integration/recoil-worktree/",
            git(self.repo, "branch", "--format=%(refname:short)"),
        )

    def test_additional_validation_uses_public_registry_and_rejects_mutation(self) -> None:
        tokens = command._validation_command_tokens(
            "python -B tools/recoil.py audit workflow-contracts --strict",
            require_public_route=True,
        )
        self.assertEqual("audit", tokens[3])
        with self.assertRaisesRegex(WorktreeControlError, "authenticated public route"):
            command._validation_command_tokens(
                "python -B tools/recoil.py issue set WSI-20260826-001 --apply",
                require_public_route=True,
            )
        with self.assertRaisesRegex(WorktreeControlError, "authenticated public route"):
            command._validation_command_tokens(
                "python -B tools/recoil.py audit definitely-missing",
                require_public_route=True,
            )

    def test_integrate_cli_reports_structured_failed_command_receipt(self) -> None:
        failure = command.IntegrationValidationError(
            packet_id=self.packet_id,
            phase="integration-validation",
            results=[
                {
                    "command": "python -B -m unittest missing.module",
                    "cwd": str(self.repo),
                    "returncode": 7,
                    "stdout": "fixture stdout",
                    "stderr": "fixture stderr",
                    "passed": False,
                    "root_routing": {
                        "canonical_control_root": str(self.repo),
                        "execution_worktree_root": str(self.repo),
                        "external_build_root": str(self.repo.parent / "build"),
                    },
                }
            ],
        )
        args = argparse.Namespace(
            ledger=str(self.ledger),
            id=self.packet_id,
            validation_command=[],
            apply=True,
        )
        stdout, stderr = io.StringIO(), io.StringIO()
        with (
            mock.patch.object(
                command,
                "integrate_issue_packet_worktree",
                side_effect=failure,
            ),
            redirect_stdout(stdout),
            redirect_stderr(stderr),
        ):
            result = command.command_integrate(args)
        self.assertEqual(2, result)
        payload = json.loads(stdout.getvalue())
        self.assertEqual("integration-validation", payload["validation_phase"])
        self.assertEqual(7, payload["failed_validation_results"][0]["returncode"])
        self.assertIn("integration-validation failed", stderr.getvalue())

    def test_hygiene_detects_unassociated_build_parent_child(self) -> None:
        build_parent = Path(str(self.repo.resolve()) + ".builds")
        (build_parent / "orphan").mkdir(parents=True)
        status = command._status_projection(
            repository_root=self.repo, ledger_path=self.ledger
        )
        self.assertIn(
            "unassociated-build-root",
            {row["kind"] for row in status["branch_hygiene_findings"]},
        )


if __name__ == "__main__":
    unittest.main()
