from __future__ import annotations

import contextlib
from copy import deepcopy
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

import recoil  # noqa: E402
from _recoil.commands.binja_preflight import DataItem, audit_data_overlaps  # noqa: E402
from _recoil.commands import progress_cli  # noqa: E402
from _recoil.commands.workspace_issues import empty_ledger  # noqa: E402
from _recoil.lib.progress import empty_progress_document, normalize_resource_claims  # noqa: E402
from _recoil.lib.worktree_control import resolve_canonical_control_root  # noqa: E402


def canonical_retail_reference() -> Path:
    resolution = resolve_canonical_control_root(
        executing_worktree_root=REPO_ROOT,
        required_machine_local_paths=("support/Recoil.exe",),
    )
    return resolution.canonical_control_root / "support" / "Recoil.exe"


def run_git(root: Path, *arguments: str) -> str:
    completed = subprocess.run(
        ["git", *arguments], cwd=root, check=True, capture_output=True,
        text=True, encoding="utf-8",
    )
    return completed.stdout.strip()


class RecoilCliTests(unittest.TestCase):
    def run_cli(self, *args: str) -> tuple[int, str, str]:
        stdout = io.StringIO()
        stderr = io.StringIO()
        with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
            rc = recoil.main(list(args))
        return rc, stdout.getvalue(), stderr.getvalue()

    def run_progress_cli(self, *args: str) -> tuple[int, str, str]:
        stdout = io.StringIO()
        stderr = io.StringIO()
        with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
            rc = progress_cli.main(list(args))
        return rc, stdout.getvalue(), stderr.getvalue()

    @staticmethod
    def issue_handoff_ledger(
        *,
        state: str = "active",
        baseline_commit: str = "opaque-test-commit",
        branch: str = "test-packet",
    ) -> dict[str, object]:
        ledger = empty_ledger()
        ledger["revision"] = 9
        issue_id = "WSI-20260723-007"
        packet_id = "issue:work:wsi-20260723-007-provider-object-proof"
        reservation_id = f"{packet_id}:attempt:1"
        ledger["issues"] = [
            {
                "id": issue_id,
                "status": "in-progress",
                "kind": "tool-error",
                "severity": "high",
                "created": "2026-07-23T00:00:00Z",
                "updated": "2026-07-23T00:00:00Z",
                "summary": "Issue handoff is unavailable.",
                "area": "tools/_recoil/commands/progress_cli.py",
                "impact": "Reserved issue packets cannot be launched.",
                "next_action": "Render the exact active issue reservation.",
                "actual": "progress handoff reports an unknown packet.",
                "repro": "python tools/recoil.py progress handoff --packet-id issue:work:test",
                "commands": [],
                "files": [],
                "tags": [],
                "history": [],
            }
        ]
        claims = normalize_resource_claims(
            [
                {"kind": "issue", "id": issue_id, "access": "read"},
                {
                    "kind": "issue-ledger",
                    "id": ".agent/WORKSPACE_ISSUES.json",
                    "access": "read",
                },
                {
                    "kind": "lane",
                    "id": f"workspace-issue/{issue_id}",
                    "access": "write",
                },
                {
                    "kind": "path",
                    "id": "tools/_recoil/commands/fixture.py",
                    "access": "write",
                },
                {"kind": "tracker", "id": "recoil", "access": "read"},
            ]
        )
        packet: dict[str, object] = {
            "id": packet_id,
            "issue_id": issue_id,
            "state": state,
            "handoff_role": "recoil_tool_maintainer",
            "scope": "Repair one bounded issue handoff defect.",
            "next_command": "python -m unittest tests.tools.recoil_cli_tests",
            "allowed_paths": ["tools/_recoil/commands/fixture.py"],
            "forbidden_paths": [
                ".agent/RECONSTRUCTION_PROGRESS.json",
                ".agent/WORKSPACE_ISSUES.json",
                "src",
            ],
            "validation_commands": [
                "python -m unittest tests.tools.recoil_cli_tests",
            ],
            "validation_command_contract_version": 1,
            "required_return_fields": [
                "packet id",
                "changed paths",
                "exact validation results",
            ],
            "resource_claims": claims,
            "reservation_id": reservation_id if state == "active" else None,
            "created": "2026-07-23T00:00:00Z",
            "updated": "2026-07-23T00:00:00Z",
            "semantic_contract_version": 1,
            "scope_versions": [],
            "role_contract_version": 1,
        }
        reservations: list[dict[str, object]] = []
        if state == "active":
            reservations.append(
                {
                    "id": reservation_id,
                    "packet_id": packet_id,
                    "state": "active",
                    "created": "2026-07-23T00:00:00Z",
                    "released": None,
                    "outcome": None,
                    "evidence_ids": [],
                    "resource_claims": deepcopy(claims),
                    "expires": None,
                    "semantic_contract_version": 1,
                    "git_workspace_baseline": {
                        "schema": "recoil-git-workspace-baseline-v2",
                        "packet_id": packet_id,
                        "baseline_commit": baseline_commit,
                        "branch": branch,
                        "writable_paths": ["tools/_recoil/commands/fixture.py"],
                        "status_porcelain_v2": [],
                        "ignored_paths": [],
                        "git_object_ids_are_opaque": True,
                    },
                }
            )
        elif state == "closed":
            packet["outcome"] = "returned"
            reservations.append(
                {
                    "id": reservation_id,
                    "packet_id": packet_id,
                    "state": "released",
                    "created": "2026-07-23T00:00:00Z",
                    "released": "2026-07-23T01:00:00Z",
                    "outcome": "returned",
                    "evidence_ids": [],
                    "resource_claims": deepcopy(claims),
                    "expires": None,
                    "semantic_contract_version": 1,
                }
            )
        ledger["work_packets"] = [packet]
        ledger["reservations"] = reservations
        return ledger

    @staticmethod
    def symbol_classification_document(*, revision: int = 7) -> dict[str, object]:
        data = empty_progress_document()
        data["revision"] = revision
        data["symbols"]["recoil:function:0x401000"] = {
            "binary": "recoil",
            "kind": "function",
            "address": "0x401000",
            "end_exclusive": "0x401010",
            "name": "UnitFunction",
            "pipeline_class": "unresolved",
            "authored_order_role": "unresolved",
            "unrelated": {"preserved": True},
        }
        data["symbols"]["recoil:data:0x401020"] = {
            "binary": "recoil",
            "kind": "data",
            "address": "0x401020",
            "end_exclusive": "0x401024",
            "pipeline_class": "unresolved",
            "authored_order_role": "unresolved",
        }
        return data

    @staticmethod
    def symbol_classification_item(**overrides: object) -> dict[str, object]:
        item: dict[str, object] = {
            "symbol_id": "recoil:function:0x401000",
            "address": "0x401000",
            "reviewed": True,
            "current_pipeline_class": "unresolved",
            "current_authored_order_role": "unresolved",
            "pipeline_class": "non-authored",
            "authored_order_role": "non-authored",
        }
        item.update(overrides)
        return item

    @staticmethod
    def logical_alias_group_document(*, revision: int = 7) -> dict[str, object]:
        data = empty_progress_document()
        data["revision"] = revision
        block_id = "recoil:block:0x401000"
        symbol_id = "recoil:function:0x401000"
        data["physical_blocks"][block_id] = {
            "binary": "recoil",
            "start": "0x401000",
            "end_exclusive": "0x401010",
            "source_path": "sample.cpp",
            "agent_source_path": "sample.cpp",
            "contribution_ids": [symbol_id],
            "semantic_span_ids": [],
            "order": {},
        }
        data["symbols"][symbol_id] = {
            "binary": "recoil",
            "kind": "function",
            "address": "0x401000",
            "end_exclusive": "0x401010",
            "navigation_name": "FoldedBody",
            "pipeline_class": "non-authored",
            "authored_order_role": "compiler-generated-icf-representative",
            "physical_block_id": block_id,
            "semantic_span_ids": [],
            "unrelated": {"preserved": True},
        }
        for evidence_id, summary in (
            ("recoil:evidence:r7:000001", "Retail fold group."),
            ("recoil:evidence:r7:000002", "Winner identity."),
            ("recoil:evidence:r7:000003", "Fold alias identity."),
        ):
            data["evidence"][evidence_id] = {
                "artifacts": [],
                "disposition": "observed",
                "freshness": "historical",
                "gating": False,
                "kind": "legacy-owner",
                "migrated_at_revision": 7,
                "provenance": {},
                "result": "pending",
                "scope_ids": [],
                "summary": summary,
                "validation_mode": "historical-observation",
            }
        for owner_id in ("recoil:owner:fixture.download", "recoil:owner:fixture.api"):
            data["owners"][owner_id] = {
                "binary": "recoil",
                "kind": "class",
                "provider_state": "pending",
                "gates": {"source": "accepted", "owner_linkage": "accepted"},
            }
        return data

    @staticmethod
    def logical_alias_group_payload(**overrides: object) -> dict[str, object]:
        winner_key = "recoil:logical-function:0x401000:download-add-ref"
        alias_key = "recoil:logical-function:0x401000:api-add-ref"
        payload: dict[str, object] = {
            "schema": "recoil-logical-alias-group-v1",
            "reviewed": True,
            "parent_reviewed": True,
            "reason": "Retail vtables prove one folded AddRef address group.",
            "symbol_id": "recoil:function:0x401000",
            "address": "0x401000",
            "current": {
                "pipeline_class": "non-authored",
                "authored_order_role": "compiler-generated-icf-representative",
                "physical_block_id": "recoil:block:0x401000",
                "linked_address_group": None,
                "icf_address_group": None,
                "logical_aliases": None,
            },
            "icf_address_group": {
                "winner_status": "selected-winner",
                "winner_identity_key": winner_key,
                "evidence_ids": ["recoil:evidence:r7:000001"],
            },
            "logical_aliases": {
                winner_key: {
                    "object_symbol": "?AddRef@DownloadSink@@UAGKXZ",
                    "original_name": "DownloadSink::AddRef",
                    "original_name_status": "recovered",
                    "source_owner_status": "authored-owner",
                    "owner_id": "recoil:owner:fixture.download",
                    "pipeline_class": "authored",
                    "authored_order_role": "authored-body",
                    "fold_status": "selected-winner",
                    "evidence_ids": ["recoil:evidence:r7:000002"],
                },
                alias_key: {
                    "object_symbol": "?AddRef@ApiSink@@UAGKXZ",
                    "original_name": "ApiSink::AddRef",
                    "original_name_status": "recovered",
                    "source_owner_status": "authored-owner",
                    "owner_id": "recoil:owner:fixture.api",
                    "pipeline_class": "authored",
                    "authored_order_role": "authored-body",
                    "fold_status": "proven-fold-alias",
                    "evidence_ids": ["recoil:evidence:r7:000003"],
                },
            },
        }
        payload.update(overrides)
        return payload

    @staticmethod
    def logical_alias_group_v2_payload(**overrides: object) -> dict[str, object]:
        payload = deepcopy(RecoilCliTests.logical_alias_group_payload())
        payload["schema"] = "recoil-logical-alias-group-v2"
        payload["icf_address_group"].pop("evidence_ids")
        for alias in payload["logical_aliases"].values():
            alias.pop("evidence_ids")
        payload["new_evidence"] = {
            "summary": (
                "Parent review of immutable retail and Binary Ninja facts proves "
                "the exact authored ICF logical-alias membership."
            ),
            "provenance": {
                "candidate_independent": True,
                "reference": "support/Recoil.exe",
                "producer": "parent-reviewed-retail-and-binary-ninja-facts",
            },
            "artifacts": [
                {
                    "path": "support/Recoil.exe",
                    "size": canonical_retail_reference().stat().st_size,
                }
            ],
            "validation_context": {
                "candidate_output_used": False,
                "review_method": "immutable-retail-and-saved-analysis-review",
            },
        }
        payload.update(overrides)
        return payload

    @staticmethod
    def logical_alias_group_v3_document(*, revision: int = 7) -> dict[str, object]:
        data = RecoilCliTests.logical_alias_group_document(revision=revision)
        symbol_id = "recoil:function:0x401000"
        base = RecoilCliTests.logical_alias_group_payload()
        group = deepcopy(base["icf_address_group"])
        group["winner_status"] = "winner-unknown"
        group["winner_identity_key"] = None
        aliases = deepcopy(base["logical_aliases"])
        for alias in aliases.values():
            alias["fold_status"] = "proven-fold-alias"
        aliases[
            "recoil:logical-function:0x401000:api-add-ref"
        ]["original_name_status"] = "provisional"
        row = data["symbols"][symbol_id]
        row["icf_address_group"] = group
        row["logical_aliases"] = aliases
        row["accepted_order_facts"] = {"stale": True}
        target_id = "recoil:vc5-target:fixture-logical-alias-v3"
        rows = [
            {
                "address": "0x401000",
                "authored_order_gate": True,
                "authored_order_role": alias["authored_order_role"],
                "authored_relative_order_gate": False,
                "full_order_gate": False,
                "icf_fold_status": "proven-fold-alias",
                "logical_identity_key": alias_id,
                "name": alias["original_name"],
                "pipeline_class": alias["pipeline_class"],
                "required_presence": True,
                "symbol": alias["object_symbol"],
                "symbol_regex": None,
            }
            for alias_id, alias in aliases.items()
        ]
        data["verification_targets"][target_id] = {
            "binary": "recoil",
            "kind": "vc5",
            "name": "fixture_logical_alias_v3",
            "registered_addresses": ["0x401000"],
            "registration": {
                "binary": "recoil",
                "manifest_path": (
                    "tools/vc5_verify_targets/"
                    "cabout_prelude_provider_order_current_shape.json"
                ),
                "name": "fixture_logical_alias_v3",
                "translation_unit_function_order": [{"functions": rows}],
            },
        }
        return data

    @staticmethod
    def logical_alias_group_v3_payload(
        document: dict[str, object] | None = None,
        **overrides: object,
    ) -> dict[str, object]:
        data = document or RecoilCliTests.logical_alias_group_v3_document()
        row = data["symbols"]["recoil:function:0x401000"]
        payload: dict[str, object] = {
            "schema": "recoil-logical-alias-group-v3",
            "reviewed": True,
            "parent_reviewed": True,
            "reason": (
                "Refresh candidate-independent physical-group evidence without "
                "changing the existing aliases."
            ),
            "symbol_id": "recoil:function:0x401000",
            "address": "0x401000",
            "current": {
                "pipeline_class": row["pipeline_class"],
                "authored_order_role": row["authored_order_role"],
                "physical_block_id": row["physical_block_id"],
                "linked_address_group": deepcopy(row.get("linked_address_group")),
                "icf_address_group": deepcopy(row["icf_address_group"]),
                "logical_aliases": deepcopy(row["logical_aliases"]),
            },
            "new_evidence": deepcopy(
                RecoilCliTests.logical_alias_group_v2_payload()["new_evidence"]
            ),
        }
        payload.update(overrides)
        return payload

    def test_reserved_handoff_and_live_byte_help_are_explicit_and_typed(self) -> None:
        rc, stdout, stderr = self.run_cli("help", "progress", "handoff")

        self.assertEqual(0, rc)
        self.assertEqual("", stderr)
        self.assertIn("real active reservation", stdout)
        self.assertIn(
            "python tools/recoil.py progress handoff --packet-id <packet-id> --json",
            stdout,
        )
        self.assertNotIn("--authored-byte", stdout)

        rc, stdout, stderr = self.run_cli("help", "progress", "advance-live-byte")

        self.assertEqual(0, rc)
        self.assertEqual("", stderr)
        self.assertIn("explicitly matched tracker physical groups", stdout)
        self.assertIn("--lane authored", stdout)

    def test_progress_handoff_renders_exact_active_issue_reservation(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            repository = root / "repo"
            repository.mkdir()
            run_git(repository, "init", "-q", "-b", "test-packet")
            run_git(repository, "config", "user.email", "recoil-tests@example.invalid")
            run_git(repository, "config", "user.name", "Recoil Tests")
            fixture = repository / "tools" / "_recoil" / "commands" / "fixture.py"
            fixture.parent.mkdir(parents=True)
            fixture.write_text("# fixture\n", encoding="utf-8")
            run_git(repository, "add", ".")
            run_git(repository, "commit", "-q", "-m", "packet baseline")
            progress_path = root / "progress.json"
            issue_path = root / "issues.json"
            progress_path.write_text(
                json.dumps(empty_progress_document(), indent=2),
                encoding="utf-8",
            )
            issue_data = self.issue_handoff_ledger(
                baseline_commit=run_git(repository, "rev-parse", "HEAD"),
                branch="test-packet",
            )
            issue_path.write_text(json.dumps(issue_data, indent=2), encoding="utf-8")
            before_progress = progress_path.read_text(encoding="utf-8")
            before_issues = issue_path.read_text(encoding="utf-8")

            with mock.patch.object(progress_cli, "REPO_ROOT", repository):
                rc, stdout, stderr = self.run_progress_cli(
                    "handoff",
                    "--progress",
                    str(progress_path),
                    "--issue-ledger",
                    str(issue_path),
                    "--packet-id",
                    "issue:work:wsi-20260723-007-provider-object-proof",
                    "--json",
                )

            self.assertEqual(0, rc, stderr)
            self.assertEqual("", stderr)
            payload = json.loads(stdout)
            self.assertEqual("workspace-issue", payload["phase"])
            self.assertEqual("workspace-issues", payload["packet_source"])
            self.assertEqual(9, payload["issue_ledger_revision"])
            self.assertEqual(
                "issue:work:wsi-20260723-007-provider-object-proof:attempt:1",
                payload["reservation_id"],
            )
            packet = payload["work_item"]
            self.assertEqual("issue-maintenance-v1", packet["packet_type"])
            self.assertEqual("recoil_tool_maintainer", packet["handoff_role"])
            self.assertEqual(
                ["tools/_recoil/commands/fixture.py"],
                packet["write_paths"],
            )
            self.assertEqual(
                "python -m unittest tests.tools.recoil_cli_tests",
                packet["worker_command"],
            )
            self.assertEqual(1, len(packet["validation_commands"]))
            self.assertEqual(
                run_git(repository, "rev-parse", "HEAD"),
                packet["baseline_commit"],
            )
            self.assertEqual("test-packet", packet["branch"])
            self.assertEqual(str(repository.resolve()), packet["worktree_root"])
            self.assertFalse(packet["worker_acceptance_allowed"])
            self.assertNotIn("git_workspace_baseline", stdout)
            self.assertNotIn("ignored_paths", stdout)
            self.assertEqual(
                before_progress,
                progress_path.read_text(encoding="utf-8"),
            )
            self.assertEqual(before_issues, issue_path.read_text(encoding="utf-8"))

    def test_progress_handoff_reauthenticates_branch_head_and_clean_state(self) -> None:
        packet_id = "issue:work:wsi-20260723-007-provider-object-proof"

        def invoke_case(label: str) -> tuple[int, str, str, str, str]:
            temporary = tempfile.TemporaryDirectory()
            self.addCleanup(temporary.cleanup)
            root = Path(temporary.name)
            repository = root / "repo"
            repository.mkdir()
            run_git(repository, "init", "-q", "-b", "test-packet")
            run_git(repository, "config", "user.email", "recoil-tests@example.invalid")
            run_git(repository, "config", "user.name", "Recoil Tests")
            fixture = repository / "tools" / "_recoil" / "commands" / "fixture.py"
            fixture.parent.mkdir(parents=True)
            fixture.write_text("baseline\n", encoding="utf-8")
            run_git(repository, "add", ".")
            run_git(repository, "commit", "-q", "-m", "packet baseline")
            baseline = run_git(repository, "rev-parse", "HEAD")

            if label == "branch-drift":
                run_git(repository, "switch", "-q", "-c", "other-branch")
            elif label == "head-drift":
                fixture.write_text("new commit\n", encoding="utf-8")
                run_git(repository, "add", ".")
                run_git(repository, "commit", "-q", "-m", "head drift")
            elif label == "staged":
                fixture.write_text("staged\n", encoding="utf-8")
                run_git(repository, "add", ".")
            elif label == "unstaged":
                fixture.write_text("unstaged\n", encoding="utf-8")
            elif label == "untracked":
                (repository / "authored-new.py").write_text("new\n", encoding="utf-8")

            progress_path = root / "progress.json"
            issue_path = root / "issues.json"
            progress_path.write_text(
                json.dumps(empty_progress_document(), indent=2), encoding="utf-8"
            )
            issue_data = self.issue_handoff_ledger(
                baseline_commit=("missing-baseline" if label == "missing" else baseline),
                branch="test-packet",
            )
            issue_path.write_text(json.dumps(issue_data, indent=2), encoding="utf-8")
            before_progress = progress_path.read_text(encoding="utf-8")
            before_issues = issue_path.read_text(encoding="utf-8")
            with mock.patch.object(progress_cli, "REPO_ROOT", repository):
                rc, stdout, stderr = self.run_progress_cli(
                    "handoff", "--progress", str(progress_path),
                    "--issue-ledger", str(issue_path), "--packet-id", packet_id,
                    "--json",
                )
            self.assertEqual(before_progress, progress_path.read_text(encoding="utf-8"))
            self.assertEqual(before_issues, issue_path.read_text(encoding="utf-8"))
            return rc, stdout, stderr, before_progress, before_issues

        expected = {
            "branch-drift": "Git branch changed",
            "head-drift": "Git HEAD changed",
            "missing": "Git command failed",
            "staged": "dirty or unmerged paths",
            "unstaged": "dirty or unmerged paths",
            "untracked": "dirty or unmerged paths",
        }
        for label, message in expected.items():
            with self.subTest(label=label):
                rc, stdout, stderr, _, _ = invoke_case(label)
                self.assertEqual(2, rc)
                self.assertEqual("", stdout)
                self.assertIn(message, stderr)

    def test_progress_handoff_rejects_unmerged_index_without_ledger_mutation(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            repository = root / "repo"
            repository.mkdir()
            run_git(repository, "init", "-q", "-b", "test-packet")
            run_git(repository, "config", "user.email", "recoil-tests@example.invalid")
            run_git(repository, "config", "user.name", "Recoil Tests")
            fixture = repository / "tools" / "_recoil" / "commands" / "fixture.py"
            fixture.parent.mkdir(parents=True)
            fixture.write_text("base\n", encoding="utf-8")
            run_git(repository, "add", ".")
            run_git(repository, "commit", "-q", "-m", "base")
            run_git(repository, "switch", "-q", "-c", "conflicting-side")
            fixture.write_text("side\n", encoding="utf-8")
            run_git(repository, "add", ".")
            run_git(repository, "commit", "-q", "-m", "side")
            run_git(repository, "switch", "-q", "test-packet")
            fixture.write_text("packet\n", encoding="utf-8")
            run_git(repository, "add", ".")
            run_git(repository, "commit", "-q", "-m", "packet baseline")
            baseline = run_git(repository, "rev-parse", "HEAD")
            merge = subprocess.run(
                ["git", "merge", "--no-edit", "conflicting-side"],
                cwd=repository, capture_output=True, text=True, check=False,
            )
            self.assertNotEqual(0, merge.returncode)

            progress_path = root / "progress.json"
            issue_path = root / "issues.json"
            progress_path.write_text(
                json.dumps(empty_progress_document(), indent=2), encoding="utf-8"
            )
            issue_path.write_text(
                json.dumps(self.issue_handoff_ledger(
                    baseline_commit=baseline, branch="test-packet"
                ), indent=2),
                encoding="utf-8",
            )
            before_progress = progress_path.read_text(encoding="utf-8")
            before_issues = issue_path.read_text(encoding="utf-8")
            with mock.patch.object(progress_cli, "REPO_ROOT", repository):
                rc, stdout, stderr = self.run_progress_cli(
                    "handoff", "--progress", str(progress_path),
                    "--issue-ledger", str(issue_path),
                    "--packet-id",
                    "issue:work:wsi-20260723-007-provider-object-proof", "--json",
                )
            self.assertEqual(2, rc)
            self.assertEqual("", stdout)
            self.assertIn("dirty or unmerged paths", stderr)
            self.assertEqual(before_progress, progress_path.read_text(encoding="utf-8"))
            self.assertEqual(before_issues, issue_path.read_text(encoding="utf-8"))

    def test_progress_handoff_issue_packet_states_fail_closed(self) -> None:
        packet_id = "issue:work:wsi-20260723-007-provider-object-proof"
        cases: list[tuple[str, dict[str, object], str]] = []
        for state in ("ready", "closed"):
            cases.append(
                (
                    state,
                    self.issue_handoff_ledger(state=state),
                    "requires exactly one active reservation",
                )
            )
        absent = self.issue_handoff_ledger()
        absent["work_packets"] = []
        absent["reservations"] = []
        cases.append(("absent", absent, "unknown work packet"))
        malformed_role = self.issue_handoff_ledger()
        malformed_role["work_packets"][0]["handoff_role"] = "recoil_source_worker"
        cases.append(("malformed-role", malformed_role, "invalid workspace issue ledger"))
        no_write = self.issue_handoff_ledger()
        read_claims = [
            row
            for row in no_write["work_packets"][0]["resource_claims"]
            if row["access"] == "read"
        ]
        no_write["work_packets"][0]["resource_claims"] = read_claims
        no_write["reservations"][0]["resource_claims"] = deepcopy(read_claims)
        cases.append(("no-write", no_write, "has no write claims"))
        role_mismatch = self.issue_handoff_ledger()
        role_mismatch["reservations"][0]["handoff_role"] = "recoil_verifier"
        cases.append(
            (
                "role-mismatch",
                role_mismatch,
                "reservation role does not match packet role",
            )
        )
        reservation_mismatch = self.issue_handoff_ledger()
        reservation_mismatch["reservations"][0]["packet_id"] = "issue:work:other"
        cases.append(
            (
                "reservation-mismatch",
                reservation_mismatch,
                "invalid workspace issue ledger",
            )
        )
        closure_mismatch = self.issue_handoff_ledger()
        closure_mismatch["reservations"][0]["git_workspace_baseline"][
            "writable_paths"
        ] = ["different.py"]
        cases.append(
            (
                "writable-closure-mismatch",
                closure_mismatch,
                "writable closure changed",
            )
        )
        malformed_baseline = self.issue_handoff_ledger()
        malformed_baseline["reservations"][0]["git_workspace_baseline"][
            "schema"
        ] = "recoil-git-workspace-baseline-v1"
        cases.append(
            (
                "unsupported-baseline",
                malformed_baseline,
                "unsupported Git workspace baseline descriptor",
            )
        )

        for label, issue_data, message in cases:
            with self.subTest(label=label), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                progress_path = root / "progress.json"
                issue_path = root / "issues.json"
                progress_path.write_text(
                    json.dumps(empty_progress_document(), indent=2),
                    encoding="utf-8",
                )
                issue_path.write_text(
                    json.dumps(issue_data, indent=2),
                    encoding="utf-8",
                )
                rc, stdout, stderr = self.run_progress_cli(
                    "handoff",
                    "--progress",
                    str(progress_path),
                    "--issue-ledger",
                    str(issue_path),
                    "--packet-id",
                    packet_id,
                    "--json",
                )
                self.assertEqual(2, rc)
                self.assertEqual("", stdout)
                self.assertIn(message, stderr)



    def test_artifact_help_registers_complete_build_and_session_cleanup_examples(self) -> None:
        rc, stdout, stderr = self.run_cli("help", "audit", "artifacts")

        self.assertEqual(0, rc)
        self.assertEqual("", stderr)
        self.assertIn("empty the complete direct build directory", stdout)
        self.assertIn("retaining build/", stdout)
        self.assertIn("python tools/recoil.py audit artifacts --delete", stdout)
        self.assertIn(
            "python tools/recoil.py audit artifacts --include-vs --older-than-days 30",
            stdout,
        )
        self.assertIn("python tools/recoil.py audit artifacts --session-only", stdout)
        self.assertIn("python tools/recoil.py audit artifacts --session-only --delete", stdout)

    def test_final_build_help_exposes_only_fresh_build_controls(self) -> None:
        result = subprocess.run(
            [
                sys.executable,
                str(REPO_ROOT / "tools" / "recoil.py"),
                "verify",
                "final-build",
                "--",
                "--help",
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

        self.assertEqual(0, result.returncode, result.stderr)
        self.assertIn("--build-dir", result.stdout)
        self.assertIn("--clean", result.stdout)
        self.assertIn("Freshly compile and link", result.stdout)
        retired_option = "--reuse" + "-compile"
        self.assertNotIn(retired_option, result.stdout)

    def test_function_docblocks_handoff_compatibility_alias_translates_scope(self) -> None:
        completed = mock.Mock(returncode=0)
        with mock.patch("recoil.subprocess.run", return_value=completed) as run:
            rc, stdout, stderr = self.run_cli(
                "audit",
                "function-docblocks",
                "--root",
                "src/Battlesport/hud.cpp",
            )

        self.assertEqual(0, rc)
        self.assertEqual("", stdout)
        self.assertEqual("", stderr)
        command = run.call_args.args[0]
        self.assertEqual(
            ["-m", "_recoil.commands.function_docblock_audit"],
            command[1:3],
        )
        self.assertEqual(
            ["--path", "src/Battlesport/hud.cpp"],
            command[3:],
        )

    def test_function_docblocks_scope_translation_is_narrow(self) -> None:
        invocations = (
            (
                ["audit", "docblocks", "--root", "src/Battlesport/hud.cpp"],
                ["--root", "src/Battlesport/hud.cpp"],
            ),
            (
                ["audit", "function-docblocks", "--path", "src/Battlesport/hud.cpp"],
                ["--path", "src/Battlesport/hud.cpp"],
            ),
        )
        for invocation, expected_args in invocations:
            with self.subTest(invocation=invocation):
                completed = mock.Mock(returncode=0)
                with mock.patch("recoil.subprocess.run", return_value=completed) as run:
                    rc, stdout, stderr = self.run_cli(*invocation)

                self.assertEqual(0, rc)
                self.assertEqual("", stdout)
                self.assertEqual("", stderr)
                command = run.call_args.args[0]
                self.assertEqual(expected_args, command[3:])

    def test_guard_handoff_compatibility_aliases_preserve_guard_arguments(self) -> None:
        aliases = {
            "modern-cpp": "_recoil.commands.no_modern_cpp_constructs",
            "source-shape": "_recoil.commands.no_source_shape_scaffolds",
            "source-placement": "_recoil.commands.source_placement_guard",
        }
        for alias, module in aliases.items():
            with self.subTest(alias=alias):
                completed = mock.Mock(returncode=0)
                with mock.patch("recoil.subprocess.run", return_value=completed) as run:
                    rc, stdout, stderr = self.run_cli(
                        "audit",
                        alias,
                        "--root",
                        "src",
                    )

                self.assertEqual(0, rc)
                self.assertEqual("", stdout)
                self.assertEqual("", stderr)
                command = run.call_args.args[0]
                self.assertEqual(["-m", module], command[1:3])
                self.assertEqual(["--root", "src"], command[3:])

    def test_raw_assembly_handoff_alias_translates_only_obsolete_allowlist(self) -> None:
        completed = mock.Mock(returncode=0)
        with mock.patch("recoil.subprocess.run", return_value=completed) as run:
            rc, stdout, stderr = self.run_cli(
                "audit",
                "raw-assembly",
                "--root",
                "src",
                "--allowlist",
                "tools/raw_assembly_allowlist.json",
            )

        self.assertEqual(0, rc)
        self.assertEqual("", stdout)
        self.assertEqual("", stderr)
        command = run.call_args.args[0]
        self.assertEqual(
            ["-m", "_recoil.commands.no_raw_assembly"],
            command[1:3],
        )
        self.assertEqual(
            [
                "--root",
                "src",
                "--allowlist",
                ".agent/RAW_ASSEMBLY_ALLOWLIST.txt",
            ],
            command[3:],
        )

    def test_raw_assembly_compatibility_translation_is_narrow(self) -> None:
        invocations = (
            (
                [
                    "audit",
                    "raw-assembly",
                    "--allowlist",
                    "tools/another_allowlist.json",
                ],
                "tools/another_allowlist.json",
            ),
            (
                [
                    "guard",
                    "raw-assembly",
                    "--allowlist",
                    "tools/raw_assembly_allowlist.json",
                ],
                "tools/raw_assembly_allowlist.json",
            ),
        )
        for invocation, expected_path in invocations:
            with self.subTest(invocation=invocation):
                completed = mock.Mock(returncode=0)
                with mock.patch("recoil.subprocess.run", return_value=completed) as run:
                    rc, stdout, stderr = self.run_cli(*invocation)

                self.assertEqual(0, rc)
                self.assertEqual("", stdout)
                self.assertEqual("", stderr)
                command = run.call_args.args[0]
                self.assertEqual(expected_path, command[-1])

    def test_dispatch_forwards_session_only_artifact_mode(self) -> None:
        completed = mock.Mock(returncode=0)
        with mock.patch("recoil.subprocess.run", return_value=completed) as run:
            rc, stdout, stderr = self.run_cli("audit", "artifacts", "--session-only")

        self.assertEqual(0, rc)
        self.assertEqual("", stdout)
        self.assertEqual("", stderr)
        command = run.call_args.args[0]
        self.assertEqual(["-m", "_recoil.commands.artifact_audit"], command[1:3])
        self.assertEqual(["--session-only"], command[3:])



    def test_vc5_help_describes_multiple_selectors(self) -> None:
        rc, stdout, stderr = self.run_cli("help", "verify", "vc5")

        self.assertEqual(0, rc)
        self.assertEqual("", stderr)
        self.assertIn("Tier S owner byte-gate evidence should use --owner", stdout)
        self.assertIn("python tools/recoil.py verify vc5 --owner source.owner_id --auto-chunk", stdout)
        self.assertIn("python tools/recoil.py verify vc5 0x401000 0x401020 --skip-bn-compare", stdout)
        self.assertIn("python tools/recoil.py verify vc5 --target target_name", stdout)
        self.assertIn("python tools/recoil.py verify vc5 target_name --auto-chunk", stdout)
        self.assertIn("python tools/recoil.py verify vc5 messages_lookup_data --binary messages --auto-chunk", stdout)
        self.assertIn("--targets-json", stdout)







    def test_provider_closure_help_describes_strict_audit(self) -> None:
        rc, stdout, stderr = self.run_cli("help", "audit", "provider-closure")

        self.assertEqual(0, rc)
        self.assertEqual("", stderr)
        self.assertIn("provider/compiler-generated dependency closure", stdout)
        self.assertIn("--owners-only", stdout)
        self.assertIn("python tools/recoil.py audit provider-closure 0x415650 --depth 1 --json --strict", stdout)

    def test_live_progress_routes_are_registered(self) -> None:
        rc, stdout, stderr = self.run_cli("help", "progress", "advance-live-order")

        self.assertEqual(0, rc)
        self.assertEqual("", stderr)
        self.assertIn("complete contiguous block slices", stdout)
        self.assertIn("all slices only on exact PASS", stdout)
        self.assertIn("--target <linked-target-id>", stdout)
        self.assertIn("--object-target <object-target-id>", stdout)
        self.assertIn("linked acceptance separate", stdout)

        rc, claim_stdout, claim_stderr = self.run_cli("help", "progress", "work", "claim-current")
        self.assertEqual(0, rc)
        self.assertEqual("", claim_stderr)
        self.assertIn("Atomically create and reserve", claim_stdout)
        self.assertIn("--lane primary", claim_stdout)

        rc, relocation_stdout, relocation_stderr = self.run_cli(
            "help", "progress", "relocation-exception", "set"
        )
        self.assertEqual(0, rc)
        self.assertEqual("", relocation_stderr)
        self.assertIn("reviewed retail-relocation ambiguity", relocation_stdout)
        self.assertIn("--expected-revision <revision>", relocation_stdout)

        rc, provider_stdout, provider_stderr = self.run_cli(
            "help", "progress", "provider-target", "register"
        )
        self.assertEqual(0, rc)
        self.assertEqual("", provider_stderr)
        self.assertIn("retail-proven named or reviewed ordinal-function IAT slot", provider_stdout)
        self.assertIn("exact import ordinal for #N imports", provider_stdout)
        self.assertIn("four-byte data/storage extent", provider_stdout)
        self.assertIn("one-byte callable provider-function extent", provider_stdout)
        self.assertIn("relocation-target bind remains the separate", provider_stdout)
        self.assertIn("--dry-run --json", provider_stdout)

        rc, metadata_stdout, metadata_stderr = self.run_cli(
            "help", "progress", "current-metadata", "refresh"
        )
        self.assertEqual(0, rc)
        self.assertEqual("", metadata_stderr)
        self.assertIn("live scheduler metadata", metadata_stdout)
        self.assertIn("--expected-revision <revision>", metadata_stdout)

        rc, readme_stdout, readme_stderr = self.run_cli(
            "help", "docs", "readme-progress"
        )
        self.assertEqual(0, rc)
        self.assertEqual("", readme_stderr)
        self.assertIn("deterministic public README progress", readme_stdout)

        rc, symbol_stdout, symbol_stderr = self.run_cli(
            "help", "progress", "symbol", "set-pipeline-class-batch"
        )
        self.assertEqual(0, rc)
        self.assertEqual("", symbol_stderr)
        self.assertIn("exact existing Recoil function-symbol ids and addresses", symbol_stdout)
        self.assertIn("current pipeline_class", symbol_stdout)
        self.assertIn("--expected-revision <revision>", symbol_stdout)

        rc, alias_stdout, alias_stderr = self.run_cli(
            "help", "progress", "symbol", "set-logical-alias-group"
        )
        self.assertEqual(0, rc)
        self.assertEqual("", alias_stderr)
        self.assertIn("Parent-only", alias_stdout)
        self.assertIn("complete current alias-related state", alias_stdout)
        self.assertIn("winner-unknown group", alias_stdout)
        self.assertIn("winner_identity_key is null", alias_stdout)
        self.assertIn("authored-lifecycle/lifecycle-body", alias_stdout)
        self.assertIn("accepted non-provider owners", alias_stdout)
        self.assertIn("V1 preserves its existing tracker-evidence contract", alias_stdout)
        self.assertIn("candidate-independent new_evidence", alias_stdout)
        self.assertIn("predicts one generated evidence id during dry-run", alias_stdout)
        self.assertIn("Caller-supplied evidence ids or scopes", alias_stdout)
        self.assertIn("--payload-file", alias_stdout)
        self.assertIn("workspace build/", alias_stdout)
        self.assertIn("--expected-revision <revision>", alias_stdout)

        rc, storage_stdout, storage_stderr = self.run_cli(
            "help", "progress", "storage", "register-authored-data"
        )
        self.assertEqual(0, rc)
        self.assertEqual("", storage_stderr)
        self.assertIn(
            "existing known-extent authored physical data artifact",
            storage_stdout,
        )
        self.assertIn("exactly one existing primary-data owner", storage_stdout)
        self.assertIn("creates no source edge", storage_stdout)
        self.assertIn("--dry-run --json", storage_stdout)

    def test_authored_storage_dispatch_is_mutating_and_syncs_readme(self) -> None:
        item = recoil.COMMANDS[
            ("progress", "storage", "register-authored-data")
        ]
        self.assertTrue(item.mutates)
        self.assertEqual("progress_cli", item.module)
        self.assertEqual(
            ("storage", "register-authored-data"),
            item.prepend_args,
        )
        completed = mock.Mock(returncode=0)
        with (
            mock.patch("recoil.subprocess.run", return_value=completed) as run,
            mock.patch(
                "recoil.progress_file_signature",
                side_effect=[(True, 1, 1, 10, 10), (True, 1, 2, 11, 11)],
            ),
            mock.patch("recoil.sync_readme_after_progress_mutation") as sync,
        ):
            rc, stdout, stderr = self.run_cli(
                "progress",
                "storage",
                "register-authored-data",
                "--payload-file",
                "build/reviewed-storage.json",
                "--expected-revision",
                "7",
                "--apply",
                "--json",
            )

        self.assertEqual(0, rc)
        self.assertEqual("", stdout)
        self.assertEqual("", stderr)
        self.assertEqual(
            ["-m", "_recoil.commands.progress_cli"],
            run.call_args.args[0][1:3],
        )
        self.assertEqual(
            ["storage", "register-authored-data"],
            run.call_args.args[0][3:5],
        )
        sync.assert_called_once_with()

    def test_authored_storage_stale_revision_is_clean_cli_error(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            progress_path = Path(temporary) / "progress.json"
            document = empty_progress_document()
            document["revision"] = 8
            progress_path.write_text(
                json.dumps(document, indent=2),
                encoding="utf-8",
            )
            payload = {
                "schema": "recoil-authored-data-storage-register-v1",
                "operation": "register-authored-data-storage",
                "reviewed": True,
                "parent_reviewed": True,
                "symbol_id": "recoil:data:0x4e1320",
                "storage_contribution_id": "recoil:storage:va:0x4e1320",
                "owner_id": "recoil:owner:sample.authored_data",
                "expected_symbol": {
                    "binary": "recoil",
                    "kind": "data",
                    "disposition": "authored",
                    "address": "0x4e1320",
                    "extent_state": "known",
                    "size": 32,
                    "end_exclusive": "0x4e1340",
                    "output_section_id": "recoil:section:.data",
                    "storage_contribution_ids": [],
                },
                "expected_owner_relationship": {
                    "kind": "primary-data",
                    "symbol_id": "recoil:data:0x4e1320",
                    "address": "0x4e1320",
                    "name": "SampleData",
                },
            }
            rc, stdout, stderr = self.run_progress_cli(
                "storage",
                "register-authored-data",
                "--progress",
                str(progress_path),
                "--payload-json",
                json.dumps(payload),
                "--expected-revision",
                "7",
                "--dry-run",
                "--json",
            )

        self.assertEqual(2, rc)
        self.assertEqual("", stdout)
        self.assertIn("revision changed: expected 7, found 8", stderr)
        self.assertNotIn("Traceback", stderr)

    def test_symbol_classification_batch_dispatch_is_mutating_and_syncs_readme(self) -> None:
        item = recoil.COMMANDS[("progress", "symbol", "set-pipeline-class-batch")]
        self.assertTrue(item.mutates)
        self.assertEqual("progress_cli", item.module)
        self.assertEqual(("symbol", "set-pipeline-class-batch"), item.prepend_args)
        payload = json.dumps([self.symbol_classification_item()])
        completed = mock.Mock(returncode=0)
        with (
            mock.patch("recoil.subprocess.run", return_value=completed) as run,
            mock.patch(
                "recoil.progress_file_signature",
                side_effect=[(True, 1, 1, 10, 10), (True, 1, 2, 11, 11)],
            ),
            mock.patch("recoil.sync_readme_after_progress_mutation") as sync,
        ):
            rc, stdout, stderr = self.run_cli(
                "progress",
                "symbol",
                "set-pipeline-class-batch",
                "--payload-json",
                payload,
                "--expected-revision",
                "7",
                "--apply",
                "--json",
            )

        self.assertEqual(0, rc)
        self.assertEqual("", stdout)
        self.assertEqual("", stderr)
        self.assertEqual(
            ["-m", "_recoil.commands.progress_cli"],
            run.call_args.args[0][1:3],
        )
        self.assertEqual(
            ["symbol", "set-pipeline-class-batch"],
            run.call_args.args[0][3:5],
        )
        sync.assert_called_once_with()

    def test_logical_alias_group_dispatch_is_mutating_and_syncs_readme(self) -> None:
        item = recoil.COMMANDS[("progress", "symbol", "set-logical-alias-group")]
        self.assertTrue(item.mutates)
        self.assertEqual("progress_cli", item.module)
        self.assertEqual(("symbol", "set-logical-alias-group"), item.prepend_args)
        completed = mock.Mock(returncode=0)
        with (
            mock.patch("recoil.subprocess.run", return_value=completed) as run,
            mock.patch(
                "recoil.progress_file_signature",
                side_effect=[(True, 1, 1, 10, 10), (True, 1, 2, 11, 11)],
            ),
            mock.patch("recoil.sync_readme_after_progress_mutation") as sync,
        ):
            rc, stdout, stderr = self.run_cli(
                "progress",
                "symbol",
                "set-logical-alias-group",
                "--payload-json",
                json.dumps(self.logical_alias_group_payload()),
                "--expected-revision",
                "7",
                "--apply",
                "--json",
            )

        self.assertEqual(0, rc)
        self.assertEqual("", stdout)
        self.assertEqual("", stderr)
        self.assertEqual(
            ["-m", "_recoil.commands.progress_cli"],
            run.call_args.args[0][1:3],
        )
        self.assertEqual(
            ["symbol", "set-logical-alias-group"],
            run.call_args.args[0][3:5],
        )
        sync.assert_called_once_with()

    def test_logical_alias_group_payload_file_dispatch_is_preserved(self) -> None:
        completed = mock.Mock(returncode=0)
        with mock.patch("recoil.subprocess.run", return_value=completed) as run:
            rc, stdout, stderr = self.run_cli(
                "progress",
                "symbol",
                "set-logical-alias-group",
                "--payload-file",
                "build/diagnostic/logical-alias-v4.json",
                "--expected-revision",
                "7",
                "--dry-run",
                "--json",
            )

        self.assertEqual(0, rc)
        self.assertEqual("", stdout)
        self.assertEqual("", stderr)
        self.assertEqual(
            [
                "symbol",
                "set-logical-alias-group",
                "--payload-file",
                "build/diagnostic/logical-alias-v4.json",
                "--expected-revision",
                "7",
                "--dry-run",
                "--json",
            ],
            run.call_args.args[0][3:],
        )

    def test_logical_alias_group_payload_inputs_are_mutually_exclusive(self) -> None:
        stderr = io.StringIO()
        with contextlib.redirect_stderr(stderr), self.assertRaises(SystemExit) as raised:
            progress_cli._parser().parse_args(
                [
                    "symbol",
                    "set-logical-alias-group",
                    "--payload-json",
                    "{}",
                    "--payload-file",
                    "build/logical-alias.json",
                    "--expected-revision",
                    "7",
                    "--dry-run",
                    "--json",
                ]
            )
        self.assertEqual(2, raised.exception.code)
        self.assertIn("not allowed with argument --payload-json", stderr.getvalue())

    def test_logical_alias_group_dry_run_apply_and_revision_cas_are_exact(self) -> None:
        with contextlib.ExitStack() as stack:
            isolated_root = Path(
                stack.enter_context(tempfile.TemporaryDirectory())
            )
            stack.enter_context(mock.patch.object(progress_cli, "REPO_ROOT", isolated_root))
            payload_root = isolated_root / "build"
            payload_root.mkdir()
            progress_path = isolated_root / "progress.json"
            payload_path = payload_root / "logical-alias-v1.json"
            document = self.logical_alias_group_document()
            progress_path.write_text(json.dumps(document, indent=2), encoding="utf-8")
            before_text = progress_path.read_text(encoding="utf-8")
            payload = self.logical_alias_group_payload()
            payload_path.write_text(json.dumps(payload), encoding="utf-8")
            inline_common = (
                "symbol",
                "set-logical-alias-group",
                "--progress",
                str(progress_path),
                "--payload-json",
                json.dumps(payload),
                "--expected-revision",
                "7",
                "--json",
            )
            file_common = (
                "symbol",
                "set-logical-alias-group",
                "--progress",
                str(progress_path),
                "--payload-file",
                str(payload_path),
                "--expected-revision",
                "7",
                "--json",
            )

            rc, stdout, stderr = self.run_progress_cli(*inline_common, "--dry-run")
            self.assertEqual(0, rc, stderr)
            self.assertEqual("", stderr)
            self.assertEqual(before_text, progress_path.read_text(encoding="utf-8"))
            dry = json.loads(stdout)
            self.assertFalse(dry["commit"]["applied"])
            self.assertEqual(7, dry["commit"]["previous_revision"])
            self.assertEqual(8, dry["commit"]["revision"])
            self.assertEqual(
                ["icf_address_group", "logical_aliases"], dry["changed_fields"]
            )
            self.assertEqual(1, dry["proven_fold_alias_count"])

            rc, file_stdout, stderr = self.run_progress_cli(*file_common, "--dry-run")
            self.assertEqual(0, rc, stderr)
            self.assertEqual("", stderr)
            self.assertEqual(dry, json.loads(file_stdout))
            self.assertEqual(before_text, progress_path.read_text(encoding="utf-8"))

            rc, stdout, stderr = self.run_progress_cli(*file_common, "--apply")
            self.assertEqual(0, rc, stderr)
            self.assertEqual("", stderr)
            applied = json.loads(stdout)
            self.assertTrue(applied["commit"]["applied"])
            self.assertEqual(8, applied["commit"]["revision"])
            after = json.loads(progress_path.read_text(encoding="utf-8"))
            row = after["symbols"]["recoil:function:0x401000"]
            self.assertEqual(payload["icf_address_group"], row["icf_address_group"])
            self.assertEqual(payload["logical_aliases"], row["logical_aliases"])
            self.assertEqual({"preserved": True}, row["unrelated"])
            self.assertEqual(8, after["revision"])

            rc, _stdout, stderr = self.run_progress_cli(*file_common, "--apply")
            self.assertEqual(2, rc)
            self.assertIn("revision changed: expected 7, found 8", stderr)

    def test_logical_alias_group_v2_dry_run_predicts_and_apply_atomically_creates_evidence(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            progress_path = Path(temporary) / "progress.json"
            document = self.logical_alias_group_document()
            progress_path.write_text(json.dumps(document, indent=2), encoding="utf-8")
            before_text = progress_path.read_text(encoding="utf-8")
            payload = self.logical_alias_group_v2_payload()
            common = (
                "symbol",
                "set-logical-alias-group",
                "--progress",
                str(progress_path),
                "--payload-json",
                json.dumps(payload),
                "--expected-revision",
                "7",
                "--json",
            )

            rc, stdout, stderr = self.run_progress_cli(*common, "--dry-run")
            self.assertEqual(0, rc, stderr)
            self.assertEqual("", stderr)
            self.assertEqual(before_text, progress_path.read_text(encoding="utf-8"))
            dry = json.loads(stdout)
            evidence_id = "recoil:evidence:r8:000001"
            expected_scope = sorted(
                {
                    "recoil:function:0x401000",
                    *payload["logical_aliases"].keys(),
                    "recoil:owner:fixture.download",
                    "recoil:owner:fixture.api",
                }
            )
            self.assertFalse(dry["commit"]["applied"])
            self.assertEqual(evidence_id, dry["evidence_id"])
            self.assertEqual(expected_scope, dry["evidence_scope_ids"])
            self.assertTrue(dry["evidence_created"])

            rc, stdout, stderr = self.run_progress_cli(*common, "--apply")
            self.assertEqual(0, rc, stderr)
            self.assertEqual("", stderr)
            applied = json.loads(stdout)
            self.assertTrue(applied["commit"]["applied"])
            self.assertEqual(evidence_id, applied["evidence_id"])
            after = json.loads(progress_path.read_text(encoding="utf-8"))
            self.assertEqual(8, after["revision"])
            self.assertEqual(
                {"revision": 8, "next_ordinal": 2},
                after["id_sequences"]["evidence"],
            )
            evidence = after["evidence"][evidence_id]
            self.assertEqual("authored-order-icf-logical-alias-review", evidence["kind"])
            self.assertEqual(expected_scope, evidence["scope_ids"])
            self.assertEqual("passed", evidence["result"])
            self.assertEqual("accepted", evidence["disposition"])
            self.assertEqual("current", evidence["freshness"])
            self.assertTrue(evidence["gating"])
            self.assertEqual("live", evidence["validation_mode"])
            self.assertTrue(evidence["provenance"]["candidate_independent"])
            self.assertFalse(
                evidence["provenance"]["validation_context"]["candidate_output_used"]
            )
            row = after["symbols"]["recoil:function:0x401000"]
            self.assertEqual([evidence_id], row["icf_address_group"]["evidence_ids"])
            self.assertTrue(
                all(
                    alias["evidence_ids"] == [evidence_id]
                    for alias in row["logical_aliases"].values()
                )
            )
            self.assertEqual({"preserved": True}, row["unrelated"])
            for collection in (
                "binaries",
                "physical_blocks",
                "semantic_spans",
                "output_sections",
                "storage_contributions",
                "owners",
                "verification_targets",
                "work_items",
                "blockers",
                "tombstones",
            ):
                self.assertEqual(document[collection], after[collection])

    def test_logical_alias_group_v3_refreshes_only_existing_group_evidence(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            progress_path = Path(temporary) / "progress.json"
            document = self.logical_alias_group_v3_document()
            progress_path.write_text(json.dumps(document, indent=2), encoding="utf-8")
            before_text = progress_path.read_text(encoding="utf-8")
            before_row = deepcopy(document["symbols"]["recoil:function:0x401000"])
            payload = self.logical_alias_group_v3_payload(document)
            target_id = "recoil:vc5-target:fixture-logical-alias-v3"
            synchronized = deepcopy(document["verification_targets"][target_id])
            common = (
                "symbol",
                "set-logical-alias-group",
                "--progress",
                str(progress_path),
                "--payload-json",
                json.dumps(payload),
                "--expected-revision",
                "7",
                "--json",
            )

            with mock.patch(
                "_recoil.lib.verification_targets.vc5_target_registration",
                return_value=(target_id, synchronized),
            ):
                rc, stdout, stderr = self.run_progress_cli(*common, "--dry-run")
            self.assertEqual(0, rc, stderr)
            self.assertEqual("", stderr)
            self.assertEqual(before_text, progress_path.read_text(encoding="utf-8"))
            dry = json.loads(stdout)
            evidence_id = "recoil:evidence:r8:000001"
            self.assertEqual(evidence_id, dry["evidence_id"])
            self.assertEqual(target_id, dry["governed_target_id"])
            self.assertEqual("physical-icf-group-only", dry["authority_scope"])
            self.assertTrue(dry["existing_group_only"])
            self.assertTrue(dry["non_evidence_fields_preserved"])

            with mock.patch(
                "_recoil.lib.verification_targets.vc5_target_registration",
                return_value=(target_id, synchronized),
            ):
                rc, stdout, stderr = self.run_progress_cli(*common, "--apply")
            self.assertEqual(0, rc, stderr)
            self.assertEqual("", stderr)
            applied = json.loads(stdout)
            self.assertTrue(applied["commit"]["applied"])
            after = json.loads(progress_path.read_text(encoding="utf-8"))
            row = after["symbols"]["recoil:function:0x401000"]
            self.assertEqual(
                {
                    key: value
                    for key, value in before_row["icf_address_group"].items()
                    if key != "evidence_ids"
                },
                {
                    key: value
                    for key, value in row["icf_address_group"].items()
                    if key != "evidence_ids"
                },
            )
            self.assertEqual([evidence_id], row["icf_address_group"]["evidence_ids"])
            for alias_id, before_alias in before_row["logical_aliases"].items():
                after_alias = row["logical_aliases"][alias_id]
                self.assertEqual(
                    {
                        key: value
                        for key, value in before_alias.items()
                        if key != "evidence_ids"
                    },
                    {
                        key: value
                        for key, value in after_alias.items()
                        if key != "evidence_ids"
                    },
                )
                self.assertEqual([evidence_id], after_alias["evidence_ids"])
            self.assertEqual(
                "provisional",
                row["logical_aliases"][
                    "recoil:logical-function:0x401000:api-add-ref"
                ]["original_name_status"],
            )
            self.assertEqual({"preserved": True}, row["unrelated"])
            self.assertIsNone(row["accepted_order_facts"])
            evidence = after["evidence"][evidence_id]
            self.assertEqual(
                "existing-winner-unknown-physical-group-refresh-v1",
                evidence["provenance"]["evidence_contract"],
            )
            self.assertEqual(
                target_id,
                evidence["provenance"]["governed_target_id"],
            )
            context = evidence["provenance"]["validation_context"]
            self.assertEqual("physical-icf-group-only", context["authority_scope"])
            self.assertFalse(context["original_name_used_as_authority"])
            self.assertFalse(context["winner_identity_claimed"])

    def test_logical_alias_group_failures_never_mutate(self) -> None:
        winner_key = "recoil:logical-function:0x401000:download-add-ref"
        alias_key = "recoil:logical-function:0x401000:api-add-ref"

        def case_payload(change) -> dict[str, object]:
            value = deepcopy(self.logical_alias_group_payload())
            change(value)
            return value

        cases = {
            "not-parent-reviewed": (
                case_payload(lambda item: item.__setitem__("parent_reviewed", False)),
                lambda data: None,
                "requires reviewed=true and parent_reviewed=true",
            ),
            "stale-current-state": (
                case_payload(
                    lambda item: item["current"].__setitem__("physical_block_id", "recoil:block:stale")
                ),
                lambda data: None,
                "current state is stale",
            ),
            "stale-physical-relationship": (
                self.logical_alias_group_payload(),
                lambda data: data["physical_blocks"]["recoil:block:0x401000"]["contribution_ids"].clear(),
                "is not an exact contribution",
            ),
            "wrong-physical-role": (
                case_payload(
                    lambda item: item["current"].__setitem__("authored_order_role", "authored-body")
                ),
                lambda data: data["symbols"]["recoil:function:0x401000"].__setitem__(
                    "authored_order_role", "authored-body"
                ),
                "must already be classified",
            ),
            "winner-key-mismatch": (
                case_payload(
                    lambda item: item["icf_address_group"].__setitem__(
                        "winner_identity_key", alias_key
                    )
                ),
                lambda data: None,
                "must identify the sole selected-winner alias",
            ),
            "no-proven-alias": (
                case_payload(
                    lambda item: item["logical_aliases"][alias_key].__setitem__(
                        "fold_status", "selected-winner"
                    )
                ),
                lambda data: None,
                "exactly one selected-winner",
            ),
            "undecorated-symbol": (
                case_payload(
                    lambda item: item["logical_aliases"][winner_key].__setitem__(
                        "object_symbol", "DownloadSinkAddRef"
                    )
                ),
                lambda data: None,
                "exact decorated VC5 object symbol",
            ),
            "unknown-evidence": (
                case_payload(
                    lambda item: item["logical_aliases"][alias_key].__setitem__(
                        "evidence_ids", ["recoil:evidence:r7:999999"]
                    )
                ),
                lambda data: None,
                "unknown tracker evidence ids",
            ),
            "unknown-owner": (
                case_payload(
                    lambda item: item["logical_aliases"][alias_key].__setitem__(
                        "owner_id", "recoil:owner:missing"
                    )
                ),
                lambda data: None,
                "references unknown owner",
            ),
            "provider-owner": (
                self.logical_alias_group_payload(),
                lambda data: data["owners"]["recoil:owner:fixture.api"].__setitem__(
                    "provider_state", "accepted"
                ),
                "must be an existing non-provider Recoil owner",
            ),
        }
        for name, (payload, mutate_document, message) in cases.items():
            with self.subTest(name=name), tempfile.TemporaryDirectory() as temporary:
                progress_path = Path(temporary) / "progress.json"
                document = self.logical_alias_group_document()
                mutate_document(document)
                progress_path.write_text(json.dumps(document, indent=2), encoding="utf-8")
                before_text = progress_path.read_text(encoding="utf-8")
                rc, _stdout, stderr = self.run_progress_cli(
                    "symbol",
                    "set-logical-alias-group",
                    "--progress",
                    str(progress_path),
                    "--payload-json",
                    json.dumps(payload),
                    "--expected-revision",
                    "7",
                    "--apply",
                    "--json",
                )
                self.assertEqual(2, rc)
                self.assertIn(message, stderr)
                self.assertEqual(before_text, progress_path.read_text(encoding="utf-8"))

    def test_symbol_classification_batch_dry_run_and_apply_are_exact(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            progress_path = Path(temporary) / "progress.json"
            document = self.symbol_classification_document()
            progress_path.write_text(json.dumps(document, indent=2), encoding="utf-8")
            before_text = progress_path.read_text(encoding="utf-8")
            before_row = dict(document["symbols"]["recoil:function:0x401000"])
            payload = json.dumps([self.symbol_classification_item()])
            common = (
                "symbol",
                "set-pipeline-class-batch",
                "--progress",
                str(progress_path),
                "--payload-json",
                payload,
                "--expected-revision",
                "7",
                "--json",
            )

            rc, stdout, stderr = self.run_progress_cli(*common, "--dry-run")
            self.assertEqual(0, rc, stderr)
            self.assertEqual("", stderr)
            self.assertEqual(before_text, progress_path.read_text(encoding="utf-8"))
            dry_run = json.loads(stdout)
            self.assertFalse(dry_run["commit"]["applied"])
            self.assertEqual(7, dry_run["commit"]["previous_revision"])
            self.assertEqual(8, dry_run["commit"]["revision"])
            self.assertEqual(
                ["pipeline_class", "authored_order_role"], dry_run["changed_fields"]
            )

            rc, stdout, stderr = self.run_progress_cli(*common, "--apply")
            self.assertEqual(0, rc, stderr)
            self.assertEqual("", stderr)
            applied = json.loads(stdout)
            self.assertTrue(applied["commit"]["applied"])
            self.assertEqual(7, applied["commit"]["previous_revision"])
            self.assertEqual(8, applied["commit"]["revision"])
            after = json.loads(progress_path.read_text(encoding="utf-8"))
            self.assertEqual(8, after["revision"])
            after_row = after["symbols"]["recoil:function:0x401000"]
            self.assertEqual("non-authored", after_row["pipeline_class"])
            self.assertEqual("non-authored", after_row["authored_order_role"])
            for field in set(before_row) - {"pipeline_class", "authored_order_role"}:
                self.assertEqual(before_row[field], after_row[field])
            self.assertEqual(
                document["symbols"]["recoil:data:0x401020"],
                after["symbols"]["recoil:data:0x401020"],
            )

    def test_symbol_classification_batch_failures_never_mutate(self) -> None:
        base_item = self.symbol_classification_item()
        cases = {
            "duplicate-symbol": (
                [base_item, dict(base_item)],
                7,
                "duplicate classification symbol_id",
            ),
            "duplicate-address": (
                [
                    base_item,
                    self.symbol_classification_item(
                        symbol_id="recoil:function:0x402000",
                    ),
                ],
                7,
                "duplicate classification address",
            ),
            "stale-address": (
                [self.symbol_classification_item(address="0x401001")],
                7,
                "address is stale",
            ),
            "stale-current-values": (
                [self.symbol_classification_item(current_pipeline_class="authored")],
                7,
                "current values are stale",
            ),
            "unknown-symbol": (
                [self.symbol_classification_item(symbol_id="recoil:function:0x402000")],
                7,
                "unknown function symbol",
            ),
            "wrong-row-kind": (
                [
                    self.symbol_classification_item(
                        symbol_id="recoil:data:0x401020",
                        address="0x401020",
                    )
                ],
                7,
                "must be an existing Recoil function row",
            ),
            "incompatible-pair": (
                [
                    self.symbol_classification_item(
                        pipeline_class="authored",
                        authored_order_role="non-authored",
                    )
                ],
                7,
                "is incompatible with pipeline_class",
            ),
            "no-op": (
                [
                    self.symbol_classification_item(
                        pipeline_class="unresolved",
                        authored_order_role="unresolved",
                    )
                ],
                7,
                "already has the requested current values",
            ),
            "revision-drift": ([base_item], 6, "revision changed: expected 6, found 7"),
        }
        for name, (items, expected_revision, message) in cases.items():
            with self.subTest(name=name), tempfile.TemporaryDirectory() as temporary:
                progress_path = Path(temporary) / "progress.json"
                progress_path.write_text(
                    json.dumps(self.symbol_classification_document(), indent=2),
                    encoding="utf-8",
                )
                before_text = progress_path.read_text(encoding="utf-8")
                rc, _stdout, stderr = self.run_progress_cli(
                    "symbol",
                    "set-pipeline-class-batch",
                    "--progress",
                    str(progress_path),
                    "--payload-json",
                    json.dumps(items),
                    "--expected-revision",
                    str(expected_revision),
                    "--apply",
                    "--json",
                )
                self.assertEqual(2, rc)
                self.assertIn(message, stderr)
                self.assertEqual(before_text, progress_path.read_text(encoding="utf-8"))

    def test_provider_target_registration_dispatch_is_mutating_and_typed(self) -> None:
        item = recoil.COMMANDS[("progress", "provider-target", "register")]
        self.assertTrue(item.mutates)
        self.assertEqual("provider_target_mutation", item.module)
        self.assertEqual(("register",), item.prepend_args)

        rc, stdout, stderr = self.run_cli(
            "progress",
            "provider-target",
            "register",
            "--show-command",
            "--address",
            "0x4cc5d8",
            "--dry-run",
        )
        self.assertEqual(0, rc)
        self.assertEqual("", stderr)
        self.assertIn("_recoil.commands.provider_target_mutation", stdout)
        self.assertIn(" register ", stdout)

    def test_successful_progress_mutation_silently_synchronizes_readme(self) -> None:
        completed = mock.Mock(returncode=0)
        with (
            mock.patch("recoil.subprocess.run", return_value=completed),
            mock.patch(
                "recoil.progress_file_signature",
                side_effect=[(True, 1, 1, 10, 10), (True, 1, 2, 11, 11)],
            ),
            mock.patch("recoil.sync_readme_after_progress_mutation") as sync,
        ):
            rc, stdout, stderr = self.run_cli(
                "progress", "current-metadata", "refresh", "--apply"
            )

        self.assertEqual(0, rc)
        self.assertEqual("", stdout)
        self.assertEqual("", stderr)
        sync.assert_called_once_with()

    def test_unchanged_progress_command_does_not_synchronize(self) -> None:
        for returncode, signatures in (
            (0, [(True, 1, 1, 10, 10), (True, 1, 1, 10, 10)]),
            (2, [(True, 1, 1, 10, 10), (True, 1, 1, 10, 10)]),
        ):
            with self.subTest(returncode=returncode):
                completed = mock.Mock(returncode=returncode)
                with (
                    mock.patch("recoil.subprocess.run", return_value=completed),
                    mock.patch(
                        "recoil.progress_file_signature", side_effect=signatures
                    ),
                    mock.patch("recoil.sync_readme_after_progress_mutation") as sync,
                ):
                    rc, _stdout, _stderr = self.run_cli(
                        "progress", "current-metadata", "refresh", "--dry-run"
                    )
                self.assertEqual(returncode, rc)
                sync.assert_not_called()

    def test_diverged_progress_command_with_committed_prefix_syncs_readme(self) -> None:
        completed = mock.Mock(returncode=2)
        with (
            mock.patch("recoil.subprocess.run", return_value=completed),
            mock.patch(
                "recoil.progress_file_signature",
                side_effect=[(True, 1, 1, 10, 10), (True, 1, 2, 11, 11)],
            ),
            mock.patch("recoil.sync_readme_after_progress_mutation") as sync,
        ):
            rc, stdout, stderr = self.run_cli(
                "progress", "current-metadata", "refresh", "--apply"
            )

        self.assertEqual(2, rc)
        self.assertEqual("", stdout)
        self.assertIn("command failed with exit code 2", stderr)
        sync.assert_called_once_with()

    def test_diverged_progress_command_and_readme_sync_failure_report_both(self) -> None:
        completed = mock.Mock(returncode=2)
        with (
            mock.patch("recoil.subprocess.run", return_value=completed),
            mock.patch(
                "recoil.progress_file_signature",
                side_effect=[(True, 1, 1, 10, 10), (True, 1, 2, 11, 11)],
            ),
            mock.patch(
                "recoil.sync_readme_after_progress_mutation",
                side_effect=RuntimeError("marker failure"),
            ),
        ):
            rc, stdout, stderr = self.run_cli(
                "progress", "current-metadata", "refresh", "--apply"
            )

        self.assertEqual(3, rc)
        self.assertEqual("", stdout)
        self.assertIn("command failed with exit code 2", stderr)
        self.assertIn("mapped_command=", stderr)
        self.assertIn(
            "tracker changed but README progress synchronization failed: marker failure",
            stderr,
        )
        self.assertIn("docs readme-progress", stderr)

    def test_readme_sync_failure_preserves_tracker_success_and_returns_distinct_code(self) -> None:
        completed = mock.Mock(returncode=0)
        with (
            mock.patch("recoil.subprocess.run", return_value=completed),
            mock.patch(
                "recoil.progress_file_signature",
                side_effect=[(True, 1, 1, 10, 10), (True, 1, 2, 11, 11)],
            ),
            mock.patch(
                "recoil.sync_readme_after_progress_mutation",
                side_effect=RuntimeError("marker failure"),
            ),
        ):
            rc, stdout, stderr = self.run_cli(
                "progress", "current-metadata", "refresh", "--apply"
            )

        self.assertEqual(3, rc)
        self.assertEqual("", stdout)
        self.assertIn("tracker changed", stderr)
        self.assertIn("docs readme-progress", stderr)

        rc, child_stdout, child_stderr = self.run_cli("help", "progress", "advance-live-byte")
        self.assertEqual(0, rc)
        self.assertEqual("", child_stderr)
        self.assertIn("progress advance-live-byte", child_stdout)

    def test_dispatch_runs_live_order_advance_command(self) -> None:
        completed = mock.Mock(returncode=0)
        with mock.patch("recoil.subprocess.run", return_value=completed) as run:
            rc, stdout, stderr = self.run_cli(
                "progress",
                "advance-live-order",
                "--target",
                "recoil:vc5-target:cabout",
                "--object-target",
                "recoil:vc5-target:cabout",
                "--build-root",
                "build/live-validation/order/cabout-r726",
                "--expected-revision",
                "725",
                "--dry-run",
                "--json",
            )

        self.assertEqual(0, rc)
        self.assertEqual("", stdout)
        self.assertEqual("", stderr)
        command = run.call_args.args[0]
        self.assertEqual(["-m", "_recoil.commands.progress_cli"], command[1:3])
        self.assertEqual(
            [
                "advance-live-order",
                "--target",
                "recoil:vc5-target:cabout",
                "--object-target",
                "recoil:vc5-target:cabout",
                "--build-root",
                "build/live-validation/order/cabout-r726",
                "--expected-revision",
                "725",
                "--dry-run",
                "--json",
            ],
            command[3:],
        )

    def test_dispatch_exposes_parent_only_source_policy_bootstrap_sync(self) -> None:
        completed = mock.Mock(returncode=0)
        with mock.patch("recoil.subprocess.run", return_value=completed) as run:
            rc, stdout, stderr = self.run_cli(
                "progress",
                "verification-target",
                "sync",
                "--target",
                "camera_449ba0_44d990_authored_order",
                "--source-policy-bootstrap",
                "--expected-revision",
                "910",
                "--dry-run",
                "--json",
            )

        self.assertEqual(0, rc)
        self.assertEqual("", stdout)
        self.assertEqual("", stderr)
        command = run.call_args.args[0]
        self.assertEqual(["-m", "_recoil.commands.progress_cli"], command[1:3])
        self.assertEqual(
            [
                "verification-target",
                "sync",
                "--target",
                "camera_449ba0_44d990_authored_order",
                "--source-policy-bootstrap",
                "--expected-revision",
                "910",
                "--dry-run",
                "--json",
            ],
            command[3:],
        )

    def test_dispatch_exposes_dry_run_first_verification_target_retirement(self) -> None:
        item = recoil.COMMANDS[("progress", "verification-target", "retire")]
        self.assertTrue(item.mutates)
        self.assertEqual("progress_cli", item.module)
        self.assertEqual(("verification-target", "retire"), item.prepend_args)

        rc, help_stdout, help_stderr = self.run_cli(
            "help",
            "progress",
            "verification-target",
            "retire",
        )
        self.assertEqual(0, rc)
        self.assertEqual("", help_stderr)
        self.assertIn(
            "retirement of exactly one existing verification-target",
            help_stdout,
        )
        self.assertIn("Exact tracker target ids take precedence", help_stdout)
        self.assertIn("preserves block order_targets", help_stdout)
        self.assertIn("--dry-run --json", help_stdout)

        completed = mock.Mock(returncode=0)
        with mock.patch("recoil.subprocess.run", return_value=completed) as run:
            rc, stdout, stderr = self.run_cli(
                "progress",
                "verification-target",
                "retire",
                "--target",
                "recoil:vc5-target:stale",
                "--expected-revision",
                "911",
                "--dry-run",
                "--json",
            )

        self.assertEqual(0, rc)
        self.assertEqual("", stdout)
        self.assertEqual("", stderr)
        self.assertEqual(
            [
                "verification-target",
                "retire",
                "--target",
                "recoil:vc5-target:stale",
                "--expected-revision",
                "911",
                "--dry-run",
                "--json",
            ],
            run.call_args.args[0][3:],
        )

    def test_verification_target_retirement_dry_run_apply_and_revision_cas_are_exact(self) -> None:
        target_id = "recoil:vc5-target:stale"
        symbol_id = "recoil:function:0x401000"
        block_id = "recoil:block:0x401000"
        with tempfile.TemporaryDirectory() as temporary:
            progress_path = Path(temporary) / "progress.json"
            document = empty_progress_document()
            document["revision"] = 7
            document["verification_targets"][target_id] = {
                "binary": "recoil",
                "kind": "vc5",
                "name": "stale",
                "registration": {"name": "stale"},
            }
            document["symbols"][symbol_id] = {
                "verification_target_ids": [target_id],
                "binary_state": {},
            }
            document["physical_blocks"][block_id] = {
                "order_targets": {"object": target_id, "linked": ""},
                "order": {},
            }
            progress_path.write_text(json.dumps(document, indent=2), encoding="utf-8")
            before_text = progress_path.read_text(encoding="utf-8")
            common = (
                "verification-target",
                "retire",
                "--progress",
                str(progress_path),
                "--target",
                target_id,
                "--expected-revision",
                "7",
                "--json",
            )

            rc, stdout, stderr = self.run_progress_cli(*common, "--dry-run")
            self.assertEqual(0, rc, stderr)
            self.assertEqual("", stderr)
            self.assertEqual(before_text, progress_path.read_text(encoding="utf-8"))
            dry = json.loads(stdout)
            self.assertFalse(dry["commit"]["applied"])
            self.assertEqual(7, dry["commit"]["previous_revision"])
            self.assertEqual(8, dry["commit"]["revision"])
            self.assertEqual(target_id, dry["retired_target_id"])
            self.assertEqual([symbol_id], dry["detached_symbol_ids"])

            rc, stdout, stderr = self.run_progress_cli(*common, "--apply")
            self.assertEqual(0, rc, stderr)
            self.assertEqual("", stderr)
            applied = json.loads(stdout)
            self.assertTrue(applied["commit"]["applied"])
            self.assertEqual(8, applied["commit"]["revision"])
            after = json.loads(progress_path.read_text(encoding="utf-8"))
            self.assertNotIn(target_id, after["verification_targets"])
            self.assertEqual([], after["symbols"][symbol_id]["verification_target_ids"])
            self.assertEqual(
                {"object": target_id, "linked": ""},
                after["physical_blocks"][block_id]["order_targets"],
            )

            rc, _stdout, stderr = self.run_progress_cli(*common, "--apply")
            self.assertEqual(2, rc)
            self.assertIn("revision changed: expected 7, found 8", stderr)

    def test_dispatch_registers_parent_only_physical_block_replace_mutation(self) -> None:
        rc, help_stdout, help_stderr = self.run_cli(
            "help",
            "progress",
            "block",
            "replace",
        )

        self.assertEqual(0, rc)
        self.assertEqual("", help_stderr)
        self.assertIn("Dry-run-first parent route", help_stdout)
        self.assertIn("complete replacement semantic spans", help_stdout)
        self.assertIn("--payload-file", help_stdout)
        self.assertIn("zero-symbol padding spans", help_stdout)
        self.assertTrue(recoil.COMMANDS[("progress", "block", "replace")].mutates)

        completed = mock.Mock(returncode=0)
        with mock.patch("recoil.subprocess.run", return_value=completed) as run:
            rc, stdout, stderr = self.run_cli(
                "progress",
                "block",
                "replace",
                "--payload-json",
                "{}",
                "--expected-revision",
                "845",
                "--dry-run",
                "--json",
            )

        self.assertEqual(0, rc)
        self.assertEqual("", stdout)
        self.assertEqual("", stderr)
        command = run.call_args.args[0]
        self.assertEqual(["-m", "_recoil.commands.progress_cli"], command[1:3])
        self.assertEqual(
            [
                "block",
                "replace",
                "--payload-json",
                "{}",
                "--expected-revision",
                "845",
                "--dry-run",
                "--json",
            ],
            command[3:],
        )

        completed = mock.Mock(returncode=0)
        with mock.patch("recoil.subprocess.run", return_value=completed) as run:
            rc, stdout, stderr = self.run_cli(
                "progress",
                "block",
                "replace",
                "--payload-file",
                "build/diagnostic/replace.json",
                "--expected-revision",
                "845",
                "--dry-run",
                "--json",
            )

        self.assertEqual(0, rc)
        self.assertEqual("", stdout)
        self.assertEqual("", stderr)
        command = run.call_args.args[0]
        self.assertEqual(["-m", "_recoil.commands.progress_cli"], command[1:3])
        self.assertEqual(
            [
                "block",
                "replace",
                "--payload-file",
                "build/diagnostic/replace.json",
                "--expected-revision",
                "845",
                "--dry-run",
                "--json",
            ],
            command[3:],
        )

    def test_dispatch_registers_parent_only_function_padding_correction(self) -> None:
        rc, help_stdout, help_stderr = self.run_cli(
            "help",
            "progress",
            "symbol",
            "replace-padding",
        )

        self.assertEqual(0, rc)
        self.assertEqual("", help_stderr)
        self.assertIn("Dry-run-first parent route", help_stdout)
        self.assertIn("immutable support/Recoil.exe", help_stdout)
        self.assertIn("authoritative tracker CAS/README synchronization", help_stdout)
        self.assertTrue(
            recoil.COMMANDS[("progress", "symbol", "replace-padding")].mutates
        )

        completed = mock.Mock(returncode=0)
        with mock.patch("recoil.subprocess.run", return_value=completed) as run:
            rc, stdout, stderr = self.run_cli(
                "progress",
                "symbol",
                "replace-padding",
                "--payload-json",
                "{}",
                "--expected-revision",
                "1264",
                "--dry-run",
                "--json",
            )

        self.assertEqual(0, rc)
        self.assertEqual("", stdout)
        self.assertEqual("", stderr)
        command = run.call_args.args[0]
        self.assertEqual(["-m", "_recoil.commands.progress_cli"], command[1:3])
        self.assertEqual(
            [
                "symbol",
                "replace-padding",
                "--payload-json",
                "{}",
                "--expected-revision",
                "1264",
                "--dry-run",
                "--json",
            ],
            command[3:],
        )

    def test_dispatch_registers_parent_only_owner_replace_batch_mutation(self) -> None:
        rc, help_stdout, help_stderr = self.run_cli(
            "help", "progress", "owner", "replace-batch"
        )

        self.assertEqual(0, rc)
        self.assertEqual("", help_stderr)
        self.assertIn("Dry-run-first parent route", help_stdout)
        self.assertIn("exact current owner records", help_stdout)
        self.assertIn("currently-unowned function bootstraps", help_stdout)
        self.assertIn("primary-data reassignments", help_stdout)
        self.assertIn("optional exact primary-function detachments", help_stdout)
        self.assertIn(
            "non-authored/compiler-generated-icf-representative", help_stdout
        )
        self.assertIn("preserves the row's established ownership_state", help_stdout)
        self.assertIn("data-only migration", help_stdout)
        self.assertIn("--payload-file", help_stdout)
        self.assertTrue(recoil.COMMANDS[("progress", "owner", "replace-batch")].mutates)

        completed = mock.Mock(returncode=0)
        with mock.patch("recoil.subprocess.run", return_value=completed) as run:
            rc, stdout, stderr = self.run_cli(
                "progress", "owner", "replace-batch", "--payload-json", "{}",
                "--expected-revision", "1307", "--dry-run", "--json",
            )

        self.assertEqual(0, rc)
        self.assertEqual("", stdout)
        self.assertEqual("", stderr)
        command = run.call_args.args[0]
        self.assertEqual(["-m", "_recoil.commands.progress_cli"], command[1:3])
        self.assertEqual(
            [
                "owner", "replace-batch", "--payload-json", "{}",
                "--expected-revision", "1307", "--dry-run", "--json",
            ],
            command[3:],
        )

    def test_dispatch_registers_parent_only_owner_downgrade_mutation(self) -> None:
        rc, help_stdout, help_stderr = self.run_cli(
            "help", "progress", "owner", "downgrade"
        )

        self.assertEqual(0, rc)
        self.assertEqual("", help_stderr)
        self.assertIn("Dry-run-first parent route", help_stdout)
        self.assertIn("exact current non-provider owner", help_stdout)
        self.assertIn("strict S/A/B/C-to-lower", help_stdout)
        self.assertTrue(recoil.COMMANDS[("progress", "owner", "downgrade")].mutates)

        completed = mock.Mock(returncode=0)
        with mock.patch("recoil.subprocess.run", return_value=completed) as run:
            rc, stdout, stderr = self.run_cli(
                "progress",
                "owner",
                "downgrade",
                "--payload-json",
                "{}",
                "--expected-revision",
                "1330",
                "--dry-run",
                "--json",
            )

        self.assertEqual(0, rc)
        self.assertEqual("", stdout)
        self.assertEqual("", stderr)
        command = run.call_args.args[0]
        self.assertEqual(["-m", "_recoil.commands.progress_cli"], command[1:3])
        self.assertEqual(
            [
                "owner",
                "downgrade",
                "--payload-json",
                "{}",
                "--expected-revision",
                "1330",
                "--dry-run",
                "--json",
            ],
            command[3:],
        )

    def test_dispatch_registers_parent_only_authored_non_gating_block_acceptance(
        self,
    ) -> None:
        rc, help_stdout, help_stderr = self.run_cli(
            "help", "progress", "block", "accept-authored-non-gating"
        )

        self.assertEqual(0, rc)
        self.assertEqual("", help_stderr)
        self.assertIn("Dry-run-first parent route", help_stdout)
        self.assertIn("complete exact current block snapshots", help_stdout)
        self.assertIn("compiler-generated lifecycle", help_stdout)
        self.assertIn("five authored-order block dimensions", help_stdout)
        self.assertIn("--payload-file", help_stdout)
        self.assertTrue(
            recoil.COMMANDS[
                ("progress", "block", "accept-authored-non-gating")
            ].mutates
        )

        completed = mock.Mock(returncode=0)
        with mock.patch("recoil.subprocess.run", return_value=completed) as run:
            rc, stdout, stderr = self.run_cli(
                "progress",
                "block",
                "accept-authored-non-gating",
                "--payload-json",
                "{}",
                "--expected-revision",
                "1344",
                "--dry-run",
                "--json",
            )

        self.assertEqual(0, rc)
        self.assertEqual("", stdout)
        self.assertEqual("", stderr)
        command = run.call_args.args[0]
        self.assertEqual(["-m", "_recoil.commands.progress_cli"], command[1:3])
        self.assertEqual(
            [
                "block",
                "accept-authored-non-gating",
                "--payload-json",
                "{}",
                "--expected-revision",
                "1344",
                "--dry-run",
                "--json",
            ],
            command[3:],
        )

    def test_dispatch_registers_parent_only_provider_block_reclassification(
        self,
    ) -> None:
        rc, help_stdout, help_stderr = self.run_cli(
            "help", "progress", "block", "reclassify-provider"
        )

        self.assertEqual(0, rc)
        self.assertEqual("", help_stderr)
        self.assertIn("Dry-run-first parent route", help_stdout)
        self.assertIn("complete exact current block snapshot", help_stdout)
        self.assertIn("exactly one accepted provider-boundary owner", help_stdout)
        self.assertIn("provisional_original_path", help_stdout)
        self.assertIn("complete derived scheduler", help_stdout)
        self.assertTrue(
            recoil.COMMANDS[("progress", "block", "reclassify-provider")].mutates
        )

        completed = mock.Mock(returncode=0)
        with mock.patch("recoil.subprocess.run", return_value=completed) as run:
            rc, stdout, stderr = self.run_cli(
                "progress",
                "block",
                "reclassify-provider",
                "--payload-json",
                "{}",
                "--expected-revision",
                "1351",
                "--dry-run",
                "--json",
            )

        self.assertEqual(0, rc)
        self.assertEqual("", stdout)
        self.assertEqual("", stderr)
        command = run.call_args.args[0]
        self.assertEqual(["-m", "_recoil.commands.progress_cli"], command[1:3])
        self.assertEqual(
            [
                "block",
                "reclassify-provider",
                "--payload-json",
                "{}",
                "--expected-revision",
                "1351",
                "--dry-run",
                "--json",
            ],
            command[3:],
        )

    def test_bn_data_evidence_help_describes_read_only_probe(self) -> None:
        rc, stdout, stderr = self.run_cli("help", "audit", "bn-data-evidence")

        self.assertEqual(0, rc)
        self.assertEqual("", stderr)
        self.assertIn("Read-only Binary Ninja data-owner recovery probe", stdout)
        self.assertIn("explicit unsupported relocation/global-search fields", stdout)
        self.assertIn("0x4e5954 --size 0xfc --constants float --nearby 0x60 --json", stdout)







    def test_group_help_lists_subcommands(self) -> None:
        rc, stdout, stderr = self.run_cli("help", "guard")

        self.assertEqual(0, rc)
        self.assertEqual("", stderr)
        self.assertIn("source-shape", stdout)
        self.assertIn("raw-offset", stdout)





    def test_binja_data_overlap_help_marks_read_only_interior_diagnostic(self) -> None:
        rc, stdout, stderr = self.run_cli("help", "binja", "data-overlap")

        self.assertEqual(0, rc)
        self.assertEqual("", stderr)
        self.assertIn("Read-only diagnostic", stdout)
        self.assertIn("interior Binary Ninja data variables", stdout)
        self.assertIn("never edits BN state", stdout)
        self.assertIn("0x4d21d8 0x4d22d4 0x4d22d8", stdout)


    def test_incomplete_group_reports_valid_subcommands(self) -> None:
        rc, stdout, stderr = self.run_cli("verify")

        self.assertEqual(2, rc)
        self.assertIn("Available subcommands", stdout)
        self.assertIn("vc5", stdout)
        self.assertIn("incomplete command group: verify", stderr)














    def test_dispatch_runs_functional_batch_subcommand(self) -> None:
        completed = mock.Mock(returncode=0)
        with mock.patch("recoil.subprocess.run", return_value=completed) as run:
            rc, stdout, stderr = self.run_cli(
                "verify",
                "functional-batch",
                "--dry-run",
                "0x401060",
                "0x401180",
            )

        self.assertEqual(0, rc)
        self.assertEqual("", stdout)
        self.assertEqual("", stderr)
        command = run.call_args.args[0]
        self.assertEqual(["-m", "_recoil.commands.functional_verify"], command[1:3])
        self.assertEqual(["batch", "--dry-run", "0x401060", "0x401180"], command[3:])




    def test_dispatch_runs_binja_data_overlap_command(self) -> None:
        completed = mock.Mock(returncode=0)
        with mock.patch("recoil.subprocess.run", return_value=completed) as run:
            rc, stdout, stderr = self.run_cli(
                "binja",
                "data-overlap",
                "0x4f52c8",
                "0x4f53ac",
                "--strict",
            )

        self.assertEqual(0, rc)
        self.assertEqual("", stdout)
        self.assertEqual("", stderr)
        command = run.call_args.args[0]
        self.assertEqual(["-m", "_recoil.commands.binja_data_overlap"], command[1:3])
        self.assertEqual(["0x4f52c8", "0x4f53ac", "--strict"], command[3:])

    def test_dispatch_runs_audit_bn_data_evidence_command(self) -> None:
        completed = mock.Mock(returncode=0)
        with mock.patch("recoil.subprocess.run", return_value=completed) as run:
            rc, stdout, stderr = self.run_cli(
                "audit",
                "bn-data-evidence",
                "0x4e5954",
                "--size",
                "0xfc",
                "--constants",
                "float",
                "--json",
            )

        self.assertEqual(0, rc)
        self.assertEqual("", stdout)
        self.assertEqual("", stderr)
        command = run.call_args.args[0]
        self.assertEqual(["-m", "_recoil.commands.bn_data_evidence"], command[1:3])
        self.assertEqual(["0x4e5954", "--size", "0xfc", "--constants", "float", "--json"], command[3:])

    def test_dispatch_runs_guard_provider_summary_command(self) -> None:
        completed = mock.Mock(returncode=0)
        with mock.patch("recoil.subprocess.run", return_value=completed) as run:
            rc, stdout, stderr = self.run_cli(
                "guard",
                "provider",
                "--root",
                "src/GameZRecoil/zVideo",
                "--summary",
            )

        self.assertEqual(0, rc)
        self.assertEqual("", stdout)
        self.assertEqual("", stderr)
        command = run.call_args.args[0]
        self.assertEqual(["-m", "_recoil.commands.provider_boundary_guard"], command[1:3])
        self.assertEqual(["--root", "src/GameZRecoil/zVideo", "--summary"], command[3:])



    def test_double_dash_is_preserved_for_downstream_commands(self) -> None:
        rc, stdout, stderr = self.run_cli("build", "msvc-x86", "--show-command", "--", "ctest")

        self.assertEqual(0, rc)
        self.assertEqual("", stderr)
        self.assertIn("_recoil.commands.msvc_x86_run", stdout)
        self.assertIn("-- ctest", stdout)

    def test_binja_preflight_data_overlap_reports_interior_root(self) -> None:
        result = audit_data_overlaps(
            [
                DataItem(
                    address="0x4f52c8",
                    start=0x4F52C8,
                    size=260,
                    name="g_WestwoodOnlineUpgradeSelectedBootstrapServer",
                    data_type="char[260]",
                    section=".data",
                ),
                DataItem(
                    address="0x4f53ac",
                    start=0x4F53AC,
                    size=260,
                    name="g_WestwoodOnlineUpgradeSelectedBootstrapServer",
                    data_type="char[260]",
                    section=".data",
                ),
            ],
            probe_addresses=("0x4f53ac",),
        )

        self.assertFalse(result.ok)
        self.assertEqual(1, result.finding_count)
        self.assertIn("starts inside", "\n".join(result.messages))
        self.assertIn("do not accept data gates", "\n".join(result.messages))

    def test_binja_preflight_data_overlap_accepts_non_overlapping_items(self) -> None:
        result = audit_data_overlaps(
            [
                DataItem(
                    address="0x4f52c8",
                    start=0x4F52C8,
                    size=260,
                    name="g_WestwoodOnlineUpgradeSelectedBootstrapServer",
                    data_type="char[260]",
                    section=".data",
                ),
                DataItem(
                    address="0x4f53d0",
                    start=0x4F53D0,
                    size=100,
                    name="g_WestwoodOnlineUpgradeInitState",
                    data_type="struct",
                    section=".data",
                ),
            ],
            probe_addresses=("0x4f53d0",),
        )

        self.assertTrue(result.ok)
        self.assertEqual(0, result.finding_count)

    def test_progress_audit_fails_closed_without_strict_flag(self) -> None:
        data = empty_progress_document()
        data["evidence"]["recoil:evidence:r0:000001"] = {
            "freshness": "current-unhashed",
            "validation_mode": "reviewed-non-gating-observation",
        }
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "progress.json"
            path.write_text(json.dumps(data), encoding="utf-8")
            rc, stdout, stderr = self.run_progress_cli(
                "audit",
                "--progress",
                str(path),
                "--json",
            )

        self.assertEqual(1, rc)
        self.assertEqual("", stderr)
        payload = json.loads(stdout)
        self.assertFalse(payload["passed"])
        self.assertTrue(
            any(
                item["code"] == "evidence.freshness"
                for item in payload["findings"]
            )
        )

    def test_call_contract_readiness_audit_is_a_non_mutating_public_route(self) -> None:
        item = recoil.COMMANDS[("audit", "call-contract-readiness")]
        self.assertEqual("call_contract_readiness_audit", item.module)
        self.assertFalse(item.mutates)

    def test_call_contract_repair_continuation_is_a_narrow_parent_route(self) -> None:
        item = recoil.COMMANDS[
            ("progress", "call-contract", "prepare-repair-continuation")
        ]
        self.assertEqual("progress_cli", item.module)
        self.assertEqual(
            ("call-contract", "prepare-repair-continuation"),
            item.prepend_args,
        )
        self.assertTrue(item.mutates)
        self.assertTrue(item.needs_binja)

        parsed = progress_cli._parser().parse_args(
            [
                "call-contract",
                "prepare-repair-continuation",
                "--producer-packet",
                "recoil:work:call-contract-continuation-producer:fixture",
                "--returned-work-item",
                "recoil:work:call-contract:fixture",
                "--build-root",
                "build/repair-continuation-fixture",
                "--expected-revision",
                "17",
                "--dry-run",
                "--json",
            ]
        )
        self.assertEqual("call-contract", parsed.command)
        self.assertEqual(
            "prepare-repair-continuation", parsed.call_contract_command
        )
        self.assertEqual(
            "recoil:work:call-contract:fixture", parsed.returned_work_item
        )
        self.assertEqual(
            "recoil:work:call-contract-continuation-producer:fixture",
            parsed.producer_packet,
        )
        self.assertEqual(17, parsed.expected_revision)
        self.assertTrue(parsed.dry_run)
        self.assertFalse(parsed.apply)
        for forbidden in ("target", "jobs", "slice", "scope"):
            self.assertFalse(hasattr(parsed, forbidden), forbidden)

        stderr = io.StringIO()
        with contextlib.redirect_stderr(stderr), self.assertRaises(SystemExit):
            progress_cli._parser().parse_args(
                [
                    "call-contract",
                    "prepare-repair-continuation",
                    "--producer-packet",
                    "recoil:work:call-contract-continuation-producer:fixture",
                    "--returned-work-item",
                    "recoil:work:call-contract:fixture",
                    "--build-root",
                    "build/repair-continuation-fixture",
                    "--expected-revision",
                    "17",
                    "--dry-run",
                    "--target",
                    "recoil:vc5-target:forbidden",
                ]
            )

        full_closeout = progress_cli._parser().parse_args(
            [
                "call-contract",
                "prepare-live-convergence",
                "--packet-id",
                "recoil:explicit-work:full-convergence-fixture",
                "--build-root",
                "build/full-convergence-fixture",
                "--jobs",
                "2",
                "--expected-revision",
                "17",
                "--dry-run",
            ]
        )
        self.assertEqual(
            Path(progress_cli.DEFAULT_ISSUE_LEDGER).resolve(),
            Path(full_closeout.issue_ledger).resolve(),
        )
        self.assertFalse(hasattr(full_closeout, "target"))

    def test_workspace_worktree_public_routes_are_exactly_registered(self) -> None:
        expected = {
            "status": False,
            "create": True,
            "validate": False,
            "integrate": True,
            "retire": True,
            "hygiene": False,
        }
        for operation, mutates in expected.items():
            item = recoil.COMMANDS[("workspace", "worktree", operation)]
            self.assertEqual("worktree_control", item.module)
            self.assertEqual((operation,), item.prepend_args)
            self.assertEqual(mutates, item.mutates)


if __name__ == "__main__":
    unittest.main()
