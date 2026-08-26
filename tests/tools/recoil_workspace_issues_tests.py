from __future__ import annotations

import argparse
from contextlib import redirect_stderr, redirect_stdout
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))

from _recoil.commands import workspace_issues
from _recoil.lib import git_change_control


PROJECT_ROOT = Path(__file__).resolve().parents[2]
TRANSIENT_RESERVATION_LOCK = Path(
    ".agent/.recoil-cross-ledger-reservation.revision.lock"
)


def run_git(root: Path, *arguments: str) -> str:
    completed = subprocess.run(
        ["git", *arguments], cwd=root, check=True, capture_output=True,
        text=True, encoding="utf-8",
    )
    return completed.stdout


class NativeGitWorkspaceIssueLifecycleTests(unittest.TestCase):
    packet_id = "issue:work:wsi-20260826-001:test-native-git"

    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        root = Path(self.temporary.name)
        self.repo = root / "repo"
        self.repo.mkdir()
        run_git(self.repo, "init", "-q")
        run_git(self.repo, "config", "user.email", "recoil-tests@example.invalid")
        run_git(self.repo, "config", "user.name", "Recoil Tests")
        (self.repo / "inside.txt").write_text("initial\n", encoding="utf-8")
        (self.repo / "outside.txt").write_text("outside\n", encoding="utf-8")
        run_git(self.repo, "add", ".")
        run_git(self.repo, "commit", "-q", "-m", "reviewed baseline")
        self.ledger = root / "issues.json"
        issue = {
            "id": "WSI-20260826-001", "status": "in-progress",
            "kind": "improvement", "severity": "high",
            "created": "2026-08-26T00:00:00Z",
            "updated": "2026-08-26T00:00:00Z",
            "summary": "native Git", "area": "workspace",
            "impact": "packet control", "next_action": "test",
            "requested_change": "native Git", "benefit": "simple",
            "commands": [], "files": [], "tags": [], "history": [],
        }
        packet = {
            "id": self.packet_id, "issue_id": issue["id"], "state": "ready",
            "handoff_role": "recoil_tool_maintainer", "scope": "test",
            "next_command": "test", "allowed_paths": ["inside.txt", "copy.txt"],
            "forbidden_paths": ["src"], "validation_commands": ["test"],
            "required_return_fields": ["changed_paths"],
            "resource_claims": workspace_issues.normalize_resource_claims([
                {"kind": "path", "id": "inside.txt", "access": "write"},
                {"kind": "path", "id": "copy.txt", "access": "write"},
                {"kind": "workspace", "id": "issue-ledger", "access": "write"},
                {"kind": "tracker", "id": "recoil", "access": "read"},
            ]),
            "reservation_id": None, "created": "2026-08-26T00:00:00Z",
            "updated": "2026-08-26T00:00:00Z",
            "semantic_contract_version": 1, "scope_versions": [],
            "role_contract_version": 1,
        }
        self.ledger.write_text(json.dumps({
            "version": 2, "revision": 0, "id_sequences": {},
            "issues": [issue], "work_packets": [packet], "reservations": [],
        }), encoding="utf-8")
        self.progress_observation = {
            "path": "copied-progress.sqlite3",
            "revision_vector": {
                "transaction_revision": 1, "semantic_revision": 1,
                "evidence_generation_revision": 1, "scheduler_revision": 1,
            },
            "schema_version": 5, "user_version": 1,
            "integrity_check": ["ok"], "foreign_key_violation_count": 0,
            "evidence_row_count": 0, "certificate_evidence_row_count": 0,
            "work_item_row_count": 0, "active_progress_reservation_count": 0,
        }

    def reserve_args(self, *, expected: int = 0, apply: bool = True):
        return argparse.Namespace(
            ledger=str(self.ledger), progress=Path("copied-progress.sqlite3"),
            id=self.packet_id, expected_revision=expected,
            apply=apply, dry_run=not apply,
        )

    def close_args(
        self, *, expected: int = 1, apply: bool = False, outcome: str = "closed"
    ):
        return argparse.Namespace(
            ledger=str(self.ledger), progress=Path("copied-progress.sqlite3"),
            id=self.packet_id, expected_revision=expected, apply=apply,
            dry_run=not apply, outcome=outcome, evidence_id=[],
        )

    def invoke(self, function, arguments):
        stdout, stderr = io.StringIO(), io.StringIO()
        with mock.patch.object(workspace_issues, "REPO_ROOT", self.repo), mock.patch.object(
            workspace_issues, "combined_lease_view",
            return_value={"incomplete_reservations": [], "conflicts": []},
        ), mock.patch.object(
            workspace_issues, "_capture_protected_progress_database",
            return_value=dict(self.progress_observation),
        ), redirect_stdout(stdout), redirect_stderr(stderr):
            result = function(arguments)
        return result, stdout.getvalue(), stderr.getvalue()

    def reserve(self) -> dict[str, object]:
        result, stdout, stderr = self.invoke(
            workspace_issues.command_work_reserve, self.reserve_args()
        )
        self.assertEqual(0, result, stderr)
        return json.loads(stdout)

    def ledger_document(self) -> dict[str, object]:
        return json.loads(self.ledger.read_text(encoding="utf-8"))

    def add_ignored_rules(self, *rules: str) -> None:
        (self.repo / ".gitignore").write_text(
            "".join(f"{rule}\n" for rule in rules), encoding="utf-8"
        )
        run_git(self.repo, "add", ".gitignore")
        run_git(self.repo, "commit", "-q", "-m", "ignored test outputs")

    def rejected_closeout(
        self, *, apply: bool = False
    ) -> tuple[dict[str, object], str]:
        result, stdout, stderr = self.invoke(
            workspace_issues.command_work_close,
            self.close_args(apply=apply),
        )
        self.assertEqual(2, result)
        payload = json.loads(stdout)
        self.assertFalse(payload["applied"])
        self.assertIsNone(payload["terminal_outcome"])
        self.assertFalse(payload["git_path_postflight"]["passed"])
        return payload, stderr

    def use_repository_ledger_for_lock_lifecycle(self) -> None:
        """Put a copied ledger beside the generated cross-ledger mutex."""

        agent_root = self.repo / ".agent"
        agent_root.mkdir(exist_ok=True)
        repository_ledger = agent_root / "issues.json"
        repository_ledger.write_bytes(self.ledger.read_bytes())
        (agent_root / "AGENTS.md").write_text(
            "Maintained test policy.\n", encoding="utf-8"
        )
        (self.repo / ".gitignore").write_bytes(
            (PROJECT_ROOT / ".gitignore").read_bytes()
        )
        run_git(
            self.repo,
            "add",
            ".gitignore",
            ".agent/AGENTS.md",
            ".agent/issues.json",
        )
        run_git(self.repo, "commit", "-q", "-m", "copied issue authority")
        self.ledger = repository_ledger

    def invoke_reserve_observing_transient_lock(
        self, *, apply: bool
    ) -> tuple[int, str, str, list[dict[str, object]]]:
        observations: list[dict[str, object]] = []
        original_capture = workspace_issues.capture_clean_git_baseline

        def capture_with_observation(*args, **kwargs):
            lock_path = self.repo / TRANSIENT_RESERVATION_LOCK
            check_ignore = subprocess.run(
                ["git", "check-ignore", "-q", "--", TRANSIENT_RESERVATION_LOCK.as_posix()],
                cwd=self.repo,
                check=False,
                capture_output=True,
            )
            status = run_git(
                self.repo,
                "status",
                "--porcelain=v2",
                "--untracked-files=all",
            )
            observations.append(
                {
                    "lock_exists": lock_path.is_file(),
                    "lock_is_ignored": check_ignore.returncode == 0,
                    "lock_in_status": TRANSIENT_RESERVATION_LOCK.as_posix() in status,
                }
            )
            return original_capture(*args, **kwargs)

        with mock.patch.object(
            workspace_issues,
            "capture_clean_git_baseline",
            side_effect=capture_with_observation,
        ):
            result, stdout, stderr = self.invoke(
                workspace_issues.command_work_reserve,
                self.reserve_args(apply=apply),
            )
        return result, stdout, stderr, observations

    def test_reservation_stores_only_native_git_workspace_baseline(self) -> None:
        payload = self.reserve()
        reservation = payload["reservation"]
        self.assertEqual(
            "recoil-git-workspace-baseline-v2",
            reservation["git_workspace_baseline"]["schema"],
        )
        self.assertEqual([], reservation["git_workspace_baseline"]["ignored_paths"])
        text = json.dumps(reservation).casefold()
        for forbidden in (
            "authored_baseline", "baseline_root", "primary_relative_path",
            "seal_relative_path", "cleanup_debt",
        ):
            self.assertNotIn(forbidden, text)

    def test_transient_reservation_lock_is_ignored_by_public_dry_run(self) -> None:
        self.use_repository_ledger_for_lock_lifecycle()
        result, stdout, stderr, observations = (
            self.invoke_reserve_observing_transient_lock(apply=False)
        )
        self.assertEqual(0, result, stderr)
        self.assertEqual(
            [{"lock_exists": True, "lock_is_ignored": True, "lock_in_status": False}],
            observations,
        )
        payload = json.loads(stdout)
        self.assertFalse(payload["commit"]["applied"])
        baseline = payload["reservation"]["git_workspace_baseline"]
        self.assertEqual([], baseline["status_porcelain_v2"])
        self.assertEqual([], baseline["ignored_paths"])
        self.assertNotIn(
            TRANSIENT_RESERVATION_LOCK.as_posix(), json.dumps(baseline)
        )
        self.assertFalse((self.repo / TRANSIENT_RESERVATION_LOCK).exists())
        self.assertEqual(0, self.ledger_document()["revision"])

    def test_public_reservation_stores_empty_deprecated_ignored_field(self) -> None:
        self.use_repository_ledger_for_lock_lifecycle()
        build_output = self.repo / "build" / "preexisting.obj"
        build_output.parent.mkdir(parents=True, exist_ok=True)
        build_output.write_text("generated\n", encoding="utf-8")
        unrelated_lock = self.repo / "retained.lock"
        unrelated_lock.write_text("generated\n", encoding="utf-8")
        with (self.repo / ".gitignore").open("a", encoding="utf-8") as stream:
            stream.write("\n/build/\n/retained.lock\n")
        run_git(self.repo, "add", ".gitignore")
        run_git(self.repo, "commit", "-q", "-m", "ignored fixture paths")

        baseline_globals = workspace_issues.capture_clean_git_baseline.__globals__
        original = baseline_globals["_run_git_text"]
        calls: list[tuple[str, ...]] = []

        def observe(repo: Path, arguments: list[str]) -> str:
            calls.append(tuple(arguments))
            return original(repo, arguments)

        with mock.patch.dict(baseline_globals, {"_run_git_text": observe}):
            dry_result, dry_stdout, dry_stderr, _ = (
                self.invoke_reserve_observing_transient_lock(apply=False)
            )
        self.assertEqual(0, dry_result, dry_stderr)
        dry_payload = json.loads(dry_stdout)
        self.assertEqual(0, self.ledger_document()["revision"])
        self.assertEqual(
            [], dry_payload["reservation"]["git_workspace_baseline"]["ignored_paths"]
        )

        with mock.patch.dict(baseline_globals, {"_run_git_text": observe}):
            result, stdout, stderr, _ = self.invoke_reserve_observing_transient_lock(
                apply=True
            )
        self.assertEqual(0, result, stderr)
        payload = json.loads(stdout)
        stored = self.ledger_document()["reservations"][0]["git_workspace_baseline"]
        self.assertEqual("recoil-git-workspace-baseline-v2", stored["schema"])
        self.assertEqual([], stored["ignored_paths"])
        self.assertEqual(payload["reservation"]["git_workspace_baseline"], stored)
        self.assertFalse(any(
            call[:4] == ("ls-files", "--others", "--ignored", "--exclude-standard")
            for call in calls
        ))
        self.assertFalse((self.repo / TRANSIENT_RESERVATION_LOCK).exists())
        self.assertEqual(1, self.ledger_document()["revision"])

    def test_transient_reservation_lock_is_removed_after_applying_reserve(self) -> None:
        self.use_repository_ledger_for_lock_lifecycle()
        result, stdout, stderr, observations = (
            self.invoke_reserve_observing_transient_lock(apply=True)
        )
        self.assertEqual(0, result, stderr)
        self.assertEqual(
            [{"lock_exists": True, "lock_is_ignored": True, "lock_in_status": False}],
            observations,
        )
        payload = json.loads(stdout)
        self.assertTrue(payload["commit"]["applied"])
        document = self.ledger_document()
        self.assertEqual(1, document["revision"])
        self.assertEqual("active", document["work_packets"][0]["state"])
        self.assertEqual(1, len(document["reservations"]))
        self.assertEqual("active", document["reservations"][0]["state"])
        self.assertEqual(
            payload["reservation"]["id"],
            document["work_packets"][0]["reservation_id"],
        )
        self.assertFalse((self.repo / TRANSIENT_RESERVATION_LOCK).exists())

    def test_transient_reservation_lock_is_owned_during_closeout_inventory(self) -> None:
        self.use_repository_ledger_for_lock_lifecycle()
        with (self.repo / ".gitignore").open("a", encoding="utf-8") as stream:
            stream.write("\n/.agent/issues.json\n")
        run_git(self.repo, "rm", "-q", "--cached", ".agent/issues.json")
        run_git(self.repo, "add", ".gitignore")
        run_git(self.repo, "commit", "-q", "-m", "ignore copied issue authority")
        self.reserve()
        (self.repo / "inside.txt").write_text("changed\n", encoding="utf-8")
        observations: list[dict[str, object]] = []
        original_capture = workspace_issues.capture_git_closeout

        def capture_with_observation(*args, **kwargs):
            lock_path = self.repo / TRANSIENT_RESERVATION_LOCK
            observations.append({
                "exists": lock_path.is_file(),
                "ignored": subprocess.run(
                    ["git", "check-ignore", "-q", "--", TRANSIENT_RESERVATION_LOCK.as_posix()],
                    cwd=self.repo, check=False, capture_output=True,
                ).returncode == 0,
            })
            return original_capture(*args, **kwargs)

        with mock.patch.object(
            workspace_issues, "capture_git_closeout",
            side_effect=capture_with_observation,
        ):
            result, stdout, stderr = self.invoke(
                workspace_issues.command_work_close, self.close_args()
            )
        self.assertEqual(0, result, stderr)
        self.assertEqual([{"exists": True, "ignored": True}], observations)
        postflight = json.loads(stdout)["git_path_postflight"]
        self.assertNotIn("reservation_ignored_paths", postflight)
        self.assertNotIn("closeout_ignored_paths", postflight)
        self.assertFalse(postflight["ignored_generated_paths_packet_gated"])
        self.assertFalse((self.repo / TRANSIENT_RESERVATION_LOCK).exists())

    def test_untracked_authored_file_still_blocks_and_removes_lock(self) -> None:
        self.use_repository_ledger_for_lock_lifecycle()
        before = self.ledger_document()
        (self.repo / "new_tool.py").write_text("authored = True\n", encoding="utf-8")
        result, _, stderr, observations = self.invoke_reserve_observing_transient_lock(
            apply=True
        )
        self.assertEqual(2, result)
        self.assertIn("new_tool.py", stderr)
        self.assertEqual(
            [{"lock_exists": True, "lock_is_ignored": True, "lock_in_status": False}],
            observations,
        )
        self.assertFalse((self.repo / TRANSIENT_RESERVATION_LOCK).exists())
        self.assertEqual(before, self.ledger_document())

    def test_untracked_agent_authored_file_still_blocks_reservation(self) -> None:
        self.use_repository_ledger_for_lock_lifecycle()
        before = self.ledger_document()
        authored = self.repo / ".agent" / "operator-policy.md"
        authored.write_text("maintained policy\n", encoding="utf-8")
        result, _, stderr, observations = self.invoke_reserve_observing_transient_lock(
            apply=True
        )
        self.assertEqual(2, result)
        self.assertIn(".agent/operator-policy.md", stderr)
        self.assertTrue(observations[0]["lock_exists"])
        self.assertTrue(observations[0]["lock_is_ignored"])
        self.assertFalse((self.repo / TRANSIENT_RESERVATION_LOCK).exists())
        self.assertEqual(before, self.ledger_document())

    def test_agent_instructions_remain_tracked_and_visible_when_modified(self) -> None:
        self.use_repository_ledger_for_lock_lifecycle()
        tracked = run_git(
            self.repo, "ls-files", "--error-unmatch", ".agent/AGENTS.md"
        )
        self.assertEqual(".agent/AGENTS.md", tracked.strip())
        (self.repo / ".agent" / "AGENTS.md").write_text(
            "Modified maintained policy.\n", encoding="utf-8"
        )
        status = run_git(
            self.repo,
            "status",
            "--porcelain=v2",
            "--untracked-files=all",
        )
        self.assertIn(".agent/AGENTS.md", status)
        result, _, stderr = self.invoke(
            workspace_issues.command_work_reserve, self.reserve_args()
        )
        self.assertEqual(2, result)
        self.assertIn(".agent/AGENTS.md", stderr)

    def test_gitignore_uses_only_the_exact_transient_lock_rule(self) -> None:
        rules = {
            line.strip()
            for line in (PROJECT_ROOT / ".gitignore").read_text(
                encoding="utf-8"
            ).splitlines()
            if line.strip() and not line.lstrip().startswith("#")
        }
        self.assertIn(
            "/.agent/.recoil-cross-ledger-reservation.revision.lock", rules
        )
        for broad_rule in (
            ".agent/",
            "/.agent/",
            "*.lock",
            ".agent/*.lock",
            "/.agent/*.lock",
            "**/*.lock",
        ):
            self.assertNotIn(broad_rule, rules)

        self.use_repository_ledger_for_lock_lifecycle()
        for path in ("other.lock", ".agent/other.lock", ".agent/authored.json"):
            ignored = subprocess.run(
                ["git", "check-ignore", "-q", "--", path],
                cwd=self.repo,
                check=False,
                capture_output=True,
            )
            self.assertEqual(1, ignored.returncode, path)

    def test_dirty_worktree_blocks_reservation_without_ledger_mutation(self) -> None:
        before = self.ledger_document()
        (self.repo / "outside.txt").write_text("dirty\n", encoding="utf-8")
        result, _, stderr = self.invoke(
            workspace_issues.command_work_reserve, self.reserve_args()
        )
        self.assertEqual(2, result)
        self.assertIn("clean Git worktree", stderr)
        self.assertEqual(before, self.ledger_document())

    def test_public_closeout_permits_in_closure_change(self) -> None:
        self.reserve()
        (self.repo / "inside.txt").write_text("changed\n", encoding="utf-8")
        result, stdout, stderr = self.invoke(
            workspace_issues.command_work_close, self.close_args()
        )
        self.assertEqual(0, result, stderr)
        payload = json.loads(stdout)
        self.assertTrue(payload["git_path_postflight"]["passed"])
        self.assertEqual(["inside.txt"], payload["git_path_postflight"]["changed_paths"])
        self.assertFalse(payload["commit"]["applied"])

    def test_public_closeout_rejects_out_of_closure_change_without_mutation(self) -> None:
        self.reserve()
        before = self.ledger_document()
        (self.repo / "outside.txt").write_text("changed\n", encoding="utf-8")
        result, _, stderr = self.invoke(
            workspace_issues.command_work_close, self.close_args()
        )
        self.assertEqual(2, result)
        self.assertIn("outside.txt", stderr)
        self.assertEqual(before, self.ledger_document())

    def test_public_closeout_rejects_real_in_closure_unmerged_state(self) -> None:
        self.reserve()
        branch = run_git(self.repo, "rev-parse", "--abbrev-ref", "HEAD").strip()
        run_git(self.repo, "switch", "-q", "-c", "conflicting-side")
        (self.repo / "inside.txt").write_text("side\n", encoding="utf-8")
        run_git(self.repo, "add", "inside.txt")
        run_git(self.repo, "commit", "-q", "-m", "side change")
        run_git(self.repo, "switch", "-q", branch)
        (self.repo / "inside.txt").write_text("main\n", encoding="utf-8")
        run_git(self.repo, "add", "inside.txt")
        run_git(self.repo, "commit", "-q", "-m", "main change")
        merge = subprocess.run(
            ["git", "merge", "conflicting-side"], cwd=self.repo,
            check=False, capture_output=True, text=True, encoding="utf-8",
        )
        self.assertNotEqual(0, merge.returncode)

        before = self.ledger_document()
        dry_payload, _ = self.rejected_closeout(apply=False)
        postflight = dry_payload["git_path_postflight"]
        self.assertEqual(["inside.txt"], postflight["unmerged_paths"])
        self.assertEqual(["inside.txt"], postflight["unexpected_paths"])
        self.assertEqual({1, 2, 3}, {
            row["stage"] for row in postflight["unmerged_index_entries"]
        })
        self.assertEqual(before, self.ledger_document())

        apply_payload, _ = self.rejected_closeout(apply=True)
        self.assertEqual(postflight, apply_payload["git_path_postflight"])
        self.assertEqual(before, self.ledger_document())

        (self.repo / "inside.txt").write_text("resolved\n", encoding="utf-8")
        run_git(self.repo, "add", "inside.txt")
        result, stdout, stderr = self.invoke(
            workspace_issues.command_work_close, self.close_args()
        )
        self.assertEqual(0, result, stderr)
        resolved = json.loads(stdout)["git_path_postflight"]
        self.assertTrue(resolved["passed"])
        self.assertEqual([], resolved["unmerged_paths"])
        self.assertEqual([], resolved["unmerged_index_entries"])

    def test_public_closeout_ignores_generated_output_classes(self) -> None:
        self.add_ignored_rules("/build/", "*.pyc", "/other.lock")
        self.reserve()
        for relative in (
            "build/unowned-audit-output/candidate.obj",
            "cache/local.pyc",
            "other.lock",
        ):
            path = self.repo / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("generated\n", encoding="utf-8")
        result, stdout, stderr = self.invoke(
            workspace_issues.command_work_close, self.close_args()
        )
        self.assertEqual(0, result, stderr)
        postflight = json.loads(stdout)["git_path_postflight"]
        self.assertTrue(postflight["passed"])
        self.assertEqual([], postflight["changed_paths"])
        self.assertEqual([], postflight["unexpected_paths"])
        self.assertEqual([], postflight["ignored_added_paths"])
        self.assertTrue(postflight["ignored_delta_fields_deprecated"])
        self.assertFalse(postflight["ignored_generated_paths_packet_gated"])
        self.assertFalse(postflight["ignored_generated_paths_inspected"])

    def test_lexical_output_root_claim_is_irrelevant_to_generated_output(self) -> None:
        self.add_ignored_rules("/build/")
        document = self.ledger_document()
        document["work_packets"][0]["resource_claims"] = (
            workspace_issues.normalize_resource_claims([
                *document["work_packets"][0]["resource_claims"],
                {
                    "kind": "output-root", "id": "build/example",
                    "access": "write",
                },
            ])
        )
        self.ledger.write_text(json.dumps(document), encoding="utf-8")
        self.reserve()
        output = self.repo / "build" / "example" / "candidate.obj"
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text("generated\n", encoding="utf-8")
        result, stdout, stderr = self.invoke(
            workspace_issues.command_work_close, self.close_args()
        )
        self.assertEqual(0, result, stderr)
        postflight = json.loads(stdout)["git_path_postflight"]
        self.assertTrue(postflight["passed"])
        self.assertEqual([], postflight["changed_paths"])
        self.assertEqual([], postflight["unexpected_paths"])

    def test_public_closeout_ignores_removed_generated_path(self) -> None:
        self.add_ignored_rules("/build/")
        generated = self.repo / "build" / "preexisting.obj"
        generated.parent.mkdir(parents=True, exist_ok=True)
        generated.write_text("generated\n", encoding="utf-8")
        self.reserve()
        generated.unlink()
        result, stdout, stderr = self.invoke(
            workspace_issues.command_work_close, self.close_args()
        )
        self.assertEqual(0, result, stderr)
        postflight = json.loads(stdout)["git_path_postflight"]
        self.assertTrue(postflight["passed"])
        self.assertEqual([], postflight["ignored_removed_paths"])

    def test_public_closeout_never_invokes_ignored_path_census(self) -> None:
        self.reserve()
        closeout_globals = git_change_control.capture_git_closeout.__globals__
        original = closeout_globals["_run_git_text"]
        calls: list[tuple[str, ...]] = []

        def observe(repo: Path, arguments: list[str]) -> str:
            calls.append(tuple(arguments))
            return original(repo, arguments)

        with mock.patch.dict(closeout_globals, {"_run_git_text": observe}):
            result, stdout, stderr = self.invoke(
                workspace_issues.command_work_close, self.close_args()
            )
        self.assertEqual(0, result, stderr)
        self.assertTrue(json.loads(stdout)["git_path_postflight"]["passed"])
        self.assertFalse(any(
            call[:4] == ("ls-files", "--others", "--ignored", "--exclude-standard")
            for call in calls
        ))
        self.assertFalse((self.repo / TRANSIENT_RESERVATION_LOCK).exists())

    def test_close_output_projects_historical_ignored_inventory_by_count_only(self) -> None:
        self.reserve()
        document = self.ledger_document()
        descriptor = document["reservations"][0]["git_workspace_baseline"]
        descriptor["ignored_paths"] = [
            "build/historical-a.obj", "build/historical-b.obj",
        ]
        self.ledger.write_text(json.dumps(document), encoding="utf-8")

        result, stdout, stderr = self.invoke(
            workspace_issues.command_work_close, self.close_args()
        )
        self.assertEqual(0, result, stderr)
        payload = json.loads(stdout)
        public_baseline = payload["reservation"]["git_workspace_baseline"]
        self.assertTrue(public_baseline["public_projection"])
        self.assertFalse(public_baseline["descriptor_complete"])
        self.assertTrue(public_baseline["ignored_paths_deprecated"])
        self.assertFalse(public_baseline["ignored_paths_packet_gated"])
        self.assertEqual(2, public_baseline["historical_ignored_path_count"])
        self.assertNotIn("ignored_paths", public_baseline)
        self.assertNotIn("build/historical-a.obj", stdout)
        self.assertNotIn("build/historical-b.obj", stdout)

    def test_work_show_projects_historical_ignored_inventory_by_count_only(self) -> None:
        self.reserve()
        document = self.ledger_document()
        descriptor = document["reservations"][0]["git_workspace_baseline"]
        descriptor["ignored_paths"] = [
            "build/historical-a.obj", "build/historical-b.obj",
        ]
        self.ledger.write_text(json.dumps(document), encoding="utf-8")

        result, stdout, stderr = self.invoke(
            workspace_issues.command_work_show,
            argparse.Namespace(ledger=self.ledger, id=self.packet_id),
        )
        self.assertEqual(0, result, stderr)
        baseline = json.loads(stdout)["reservations"][0]["git_workspace_baseline"]
        self.assertTrue(baseline["public_projection"])
        self.assertEqual(2, baseline["historical_ignored_path_count"])
        self.assertNotIn("ignored_paths", baseline)
        self.assertNotIn("build/historical-a.obj", stdout)
        self.assertNotIn("build/historical-b.obj", stdout)

    def test_malformed_unmerged_index_row_leaves_ledger_unchanged(self) -> None:
        self.reserve()
        before = self.ledger_document()
        closeout_globals = git_change_control.capture_git_closeout.__globals__
        original = closeout_globals["_run_git_text"]

        def malformed(repo: Path, arguments: list[str]) -> str:
            if arguments == ["ls-files", "-u", "-z"]:
                return "100644 opaque-object 2 conflict.txt\0"
            return original(repo, arguments)

        with mock.patch.dict(closeout_globals, {"_run_git_text": malformed}):
            result, _, stderr = self.invoke(
                workspace_issues.command_work_close, self.close_args(apply=True)
            )
        self.assertEqual(2, result)
        self.assertIn("malformed unmerged-index", stderr)
        self.assertEqual(before["revision"], self.ledger_document()["revision"])
        self.assertEqual(
            before["work_packets"], self.ledger_document()["work_packets"]
        )
        self.assertEqual(
            before["reservations"], self.ledger_document()["reservations"]
        )

    def test_public_closeout_treats_ordinary_copy_as_destination_only(self) -> None:
        self.reserve()
        (self.repo / "copy.txt").write_bytes((self.repo / "outside.txt").read_bytes())
        result, stdout, stderr = self.invoke(
            workspace_issues.command_work_close, self.close_args()
        )
        self.assertEqual(0, result, stderr)
        postflight = json.loads(stdout)["git_path_postflight"]
        self.assertTrue(postflight["passed"])
        self.assertEqual(["copy.txt"], postflight["changed_paths"])
        self.assertEqual("unchanged-source-destination-only", postflight["ordinary_copy_rule"])

    def test_public_closeout_reports_independently_changed_copy_source_without_mutation(self) -> None:
        for source_status in ("M", "D"):
            with self.subTest(source_status=source_status):
                self.setUp()
                self.reserve()
                before = self.ledger_document()
                closeout_globals = git_change_control.capture_git_closeout.__globals__
                original = closeout_globals["_run_git_text"]

                def synthetic(repo: Path, arguments: list[str]) -> str:
                    if arguments[:4] == [
                        "diff", "--no-ext-diff", "--name-status", "-z",
                    ]:
                        return (
                            "C100\0outside.txt\0copy.txt\0"
                            f"{source_status}\0outside.txt\0"
                        )
                    if arguments == [
                        "status", "--porcelain=v2", "-z", "--untracked-files=all",
                    ]:
                        return ""
                    if arguments in (
                        ["ls-files", "-u", "-z"],
                        ["ls-files", "-z", "--others", "--exclude-standard"],
                    ):
                        return ""
                    if arguments[:3] == ["diff", "--no-ext-diff", "--stat"]:
                        return ""
                    return original(repo, arguments)

                with mock.patch.dict(closeout_globals, {"_run_git_text": synthetic}):
                    result, stdout, stderr = self.invoke(
                        workspace_issues.command_work_close,
                        self.close_args(apply=True),
                    )
                self.assertEqual(2, result, stderr)
                payload = json.loads(stdout)
                postflight = payload["git_path_postflight"]
                self.assertEqual(
                    ["copy.txt", "outside.txt"], postflight["changed_paths"]
                )
                self.assertEqual(["outside.txt"], postflight["unexpected_paths"])
                self.assertTrue(any(
                    row["destination_path"] == "outside.txt"
                    for row in postflight["endpoint_violations"]
                ))
                after = self.ledger_document()
                self.assertEqual(before["revision"], after["revision"])
                self.assertEqual(before["work_packets"], after["work_packets"])
                self.assertEqual(before["reservations"], after["reservations"])

    def test_applying_close_releases_reservation(self) -> None:
        self.reserve()
        (self.repo / "inside.txt").write_text("changed\n", encoding="utf-8")
        result, stdout, stderr = self.invoke(
            workspace_issues.command_work_close,
            self.close_args(apply=True, outcome="closed"),
        )
        self.assertEqual(0, result, stderr)
        payload = json.loads(stdout)
        self.assertTrue(payload["commit"]["applied"])
        document = self.ledger_document()
        self.assertEqual([], document["work_packets"])
        self.assertEqual([], document["reservations"])


if __name__ == "__main__":
    unittest.main()
