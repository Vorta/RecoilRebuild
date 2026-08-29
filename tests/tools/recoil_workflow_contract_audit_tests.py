from __future__ import annotations

from pathlib import Path
from copy import deepcopy
import contextlib
import io
import json
import tempfile
from types import SimpleNamespace
import sys
import unittest
from unittest.mock import patch


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands import progress_cli  # noqa: E402
from _recoil.commands.workflow_contract_audit import (  # noqa: E402
    REQUIRED_COMMANDS,
    _audit_integration_validation_guidance,
    audit_generated_call_contract_commands,
    audit_workflow_contracts,
    build_parser,
)
from _recoil.lib.progress import (
    EXPLICIT_MAINTENANCE_PACKET_TYPE,
    ProgressDocument,
    empty_progress_document,
    explicit_output_allocation_record,
    normalize_resource_claims,
)
from _recoil.lib.progress_sqlite import ProgressSQLiteStore


def valid_specs() -> list[dict[str, object]]:
    return [
        {
            "path": path,
            "module": contract.module,
            "prepend_args": contract.prepend,
            "mutates": contract.mutates,
        }
        for path, contract in REQUIRED_COMMANDS.items()
    ]


class WorkflowContractAuditTests(unittest.TestCase):
    def test_integration_guidance_rejects_post_fast_forward_validation(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            skill = root / ".codex" / "skills" / "recoil-validation" / "SKILL.md"
            skill.parent.mkdir(parents=True)
            skill.write_text(
                "Validation runs before `master` advances and again on canonical `master`.\n"
                "The canonical control root and executing worktree are recorded.\n"
                "Deterministic assertions follow.\n",
                encoding="utf-8",
            )
            failures = _audit_integration_validation_guidance(root)
        self.assertTrue(failures)
        self.assertEqual("integration-validation-order", failures[0]["check"])

    def test_integration_guidance_accepts_pre_fast_forward_only_contract(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            skill = root / ".codex" / "skills" / "recoil-validation" / "SKILL.md"
            skill.parent.mkdir(parents=True)
            skill.write_text(
                "All fallible checks run in the executing worktree before `master` advances.\n"
                "Afterward only deterministic assertions run. Machine-local inputs come "
                "from the canonical control root.\n",
                encoding="utf-8",
            )
            failures = _audit_integration_validation_guidance(root)
        self.assertEqual([], failures)

    def test_explicit_canonical_root_rejects_different_progress_path(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            canonical = root / "canonical"
            expected = canonical / ".agent" / "RECONSTRUCTION_PROGRESS.sqlite3"
            expected.parent.mkdir(parents=True)
            expected.write_bytes(b"expected")
            other = root / "fixture.sqlite3"
            other.write_bytes(b"fixture")
            resolution = SimpleNamespace(
                canonical_control_root=canonical,
                resolution_source="explicit",
            )
            with patch(
                "_recoil.lib.worktree_control.resolve_canonical_control_root",
                return_value=resolution,
            ):
                with self.assertRaisesRegex(Exception, "does not equal"):
                    audit_workflow_contracts(
                        specs=valid_specs(),
                        progress_path=other,
                        canonical_root=canonical,
                    )

    def test_canonical_progress_reauthenticates_when_projection_aborts(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            canonical = Path(temporary)
            progress = canonical / ".agent" / "RECONSTRUCTION_PROGRESS.sqlite3"
            progress.parent.mkdir(parents=True)
            progress.write_bytes(b"fixture")
            resolution = SimpleNamespace(
                canonical_control_root=canonical,
                resolution_source="explicit",
            )
            document = SimpleNamespace(
                pipeline=lambda _binary: (_ for _ in ()).throw(KeyboardInterrupt())
            )
            with (
                patch(
                    "_recoil.lib.worktree_control.resolve_canonical_control_root",
                    return_value=resolution,
                ),
                patch(
                    "_recoil.lib.progress.ProgressDocument.load",
                    return_value=document,
                ),
                patch(
                    "_recoil.lib.worktree_control.reauthenticate_canonical_control_root"
                ) as reauthenticate,
            ):
                with self.assertRaises(KeyboardInterrupt):
                    audit_workflow_contracts(
                        specs=valid_specs(),
                        progress_path=progress,
                        canonical_root=canonical,
                    )
        reauthenticate.assert_called_once_with(resolution)

    def test_workspace_worktree_registry_and_progress_containment(self) -> None:
        expected = {
            "status": False, "create": True, "validate": False,
            "integrate": True, "retire": True, "hygiene": False,
        }
        for operation, mutates in expected.items():
            contract = REQUIRED_COMMANDS[("workspace", "worktree", operation)]
            self.assertEqual("worktree_control", contract.module)
            self.assertEqual((operation,), contract.prepend)
            self.assertEqual(mutates, contract.mutates)
        report = audit_workflow_contracts(specs=valid_specs())
        self.assertEqual(
            "passed", report["checks"]["progress_worktree_adapter_containment"]
        )

    def test_generated_command_projection_rejects_false_launchability(self) -> None:
        document = ProgressDocument(empty_progress_document())
        findings = audit_generated_call_contract_commands(
            document,
            {
                "call_contract_containment": {
                    "launchable": True,
                    "next_command": None,
                }
            },
        )
        self.assertTrue(findings)
        self.assertIn("lacks a nonempty command", findings[0]["message"])

    def test_generated_command_projection_authenticates_output_marker(self) -> None:
        build_dir = REPO_ROOT / "build"
        created_build_dir = not build_dir.exists()
        build_dir.mkdir(exist_ok=True)
        if created_build_dir:
            self.addCleanup(build_dir.rmdir)
        with tempfile.TemporaryDirectory(dir=build_dir) as temporary:
            root = Path(temporary)
            progress = root / "progress.sqlite3"
            data = empty_progress_document()
            ProgressSQLiteStore.create_from_mapping(
                progress, deepcopy(data), cutover_pair_id="generated-command-root"
            )
            packet_id = "recoil:explicit-work:generated-command"
            reservation_id = f"{packet_id}:attempt:1"
            output_root = root / "output"
            output_root.mkdir()
            output_relative = output_root.relative_to(REPO_ROOT).as_posix()
            allocation = explicit_output_allocation_record(
                packet_id=packet_id,
                reservation_id=reservation_id,
                output_root=output_relative,
                progress_path=progress,
                operation_nonce="generated-command",
                issue_ledger_identity={"fixture": "generated-command"},
            )
            claims = normalize_resource_claims(
                [
                    {"kind": "binary-ninja-db", "id": "Recoil.bndb", "access": "read"},
                    {"kind": "reference", "id": "support/Recoil.exe", "access": "read"},
                    {"kind": "tracker", "id": "recoil", "access": "read"},
                    {"kind": "output-root", "id": output_relative, "access": "write"},
                ]
            )
            data["work_items"][packet_id] = {
                "id": packet_id,
                "packet_type": EXPLICIT_MAINTENANCE_PACKET_TYPE,
                "state": "active",
                "resource_claims": claims,
                "reservation": {
                    "id": reservation_id,
                    "state": "active",
                    "resource_claims": deepcopy(claims),
                },
                "explicit_provenance": {"output_allocation": allocation},
            }
            encoded = json.dumps(allocation, sort_keys=True, separators=(",", ":")) + "\n"
            (REPO_ROOT / allocation["ownership_sidecar"]).write_text(encoded, encoding="utf-8")
            command = (
                "python -B tools/recoil.py progress advance-live-call-contract "
                f"--packet-id {packet_id} --expected-semantic-revision 1 "
                "--expected-evidence-generation-revision 1 --apply"
            )
            findings = audit_generated_call_contract_commands(
                ProgressDocument(data, path=progress),
                {
                    "call_contract_containment": {
                        "launchable": True,
                        "next_command": command,
                    }
                },
            )
            self.assertTrue(
                any("unauthenticated output root" in row["message"] for row in findings)
            )

    def test_source_fragment_commands_are_nonmutating_workflow_contracts(self) -> None:
        audit_contract = REQUIRED_COMMANDS[("audit", "source-fragments")]
        guard_contract = REQUIRED_COMMANDS[("guard", "source-fragments")]

        self.assertEqual(("source_fragments", ("--audit",), False), (
            audit_contract.module,
            audit_contract.prepend,
            audit_contract.mutates,
        ))
        self.assertEqual(("source_fragments", (), False), (
            guard_contract.module,
            guard_contract.prepend,
            guard_contract.mutates,
        ))

    def test_current_parser_and_handoff_contract_pass_with_injected_registry(self) -> None:
        report = audit_workflow_contracts(
            specs=valid_specs(),
            progress_module=progress_cli,
            invocation_counts={"advance-live-order": 1, "advance-live-byte": 1, "advance-live-call-contract": 1},
        )
        self.assertTrue(report["passed"], report["failures"])
        self.assertEqual(0, report["failure_count"])

    def test_required_verifier_component_failure_blocks_workflow_audit(self) -> None:
        document = ProgressDocument(empty_progress_document())
        with patch(
            "_recoil.lib.call_contract_generations."
            "required_call_contract_verifier_component_findings",
            return_value=[
                {
                    "kind": "unparseable",
                    "path": "tools/_recoil/lib/binja.py",
                    "detail": "line 1: invalid syntax",
                }
            ],
        ):
            report = audit_workflow_contracts(
                specs=valid_specs(),
                progress_module=progress_cli,
                invocation_counts={
                    "advance-live-order": 1,
                    "advance-live-byte": 1,
                    "advance-live-call-contract": 1,
                },
                generated_command_document=document,
                generated_command_payload={},
            )
        self.assertFalse(report["passed"])
        self.assertTrue(
            any(
                row["check"] == "call-contract-required-component"
                for row in report["failures"]
            )
        )

    def test_missing_public_claim_command_is_a_contract_failure(self) -> None:
        specs = [
            item
            for item in valid_specs()
            if tuple(item["path"]) != ("progress", "work", "claim-current")
        ]
        report = audit_workflow_contracts(
            specs=specs,
            progress_module=progress_cli,
            invocation_counts={"advance-live-order": 1, "advance-live-byte": 1, "advance-live-call-contract": 1},
        )
        self.assertFalse(report["passed"])
        self.assertTrue(
            any("claim-current" in item["message"] for item in report["failures"])
        )

    def test_missing_public_repair_continuation_is_a_contract_failure(self) -> None:
        specs = [
            item
            for item in valid_specs()
            if tuple(item["path"])
            != ("progress", "call-contract", "prepare-repair-continuation")
        ]
        report = audit_workflow_contracts(
            specs=specs,
            progress_module=progress_cli,
            invocation_counts={
                "advance-live-order": 1,
                "advance-live-byte": 1,
                "advance-live-call-contract": 1,
            },
        )
        self.assertFalse(report["passed"])
        self.assertTrue(
            any(
                "prepare-repair-continuation" in item["message"]
                for item in report["failures"]
            )
        )

    def test_verify_command_cannot_be_marked_as_tracker_mutating(self) -> None:
        specs = valid_specs()
        for item in specs:
            if tuple(item["path"]) == ("verify", "vc5-order"):
                item["mutates"] = True
        report = audit_workflow_contracts(
            specs=specs,
            progress_module=progress_cli,
            invocation_counts={"advance-live-order": 1, "advance-live-byte": 1, "advance-live-call-contract": 1},
        )
        self.assertFalse(report["passed"])
        self.assertEqual(
            "failed", report["checks"]["verify_vs_advance_mutation_boundary"]
        )

    def test_handoff_that_accepts_invalid_reservations_is_detected(self) -> None:
        def permissive_handoff(_document, _args):
            return {
                "work_item": {
                    "reservation_id": "recoil:work:sample:attempt:1",
                    "write_paths": ["src/GameZRecoil/sample.cpp"],
                    "worker_command": "python tools/recoil.py verify vc5-order sample",
                }
            }

        facade = SimpleNamespace(**vars(progress_cli))
        facade._handoff = permissive_handoff
        report = audit_workflow_contracts(
            specs=valid_specs(),
            progress_module=facade,
            invocation_counts={"advance-live-order": 1, "advance-live-byte": 1, "advance-live-call-contract": 1},
        )
        self.assertFalse(report["passed"])
        rejected = [
            item
            for item in report["failures"]
            if item["check"] == "compact-reserved-handoff"
        ]
        self.assertGreaterEqual(len(rejected), 4)

    def test_live_acceptance_must_launch_exactly_one_validator(self) -> None:
        report = audit_workflow_contracts(
            specs=valid_specs(),
            progress_module=progress_cli,
            invocation_counts={"advance-live-order": 2, "advance-live-byte": 1, "advance-live-call-contract": 1},
        )
        self.assertFalse(report["passed"])
        self.assertTrue(
            any(
                item["check"] == "single-validator-invocation"
                and "advance-live-order" in item["message"]
                for item in report["failures"]
            )
        )

    def test_direct_call_contract_requires_packet_and_expensive_audits_stay_disabled(self) -> None:
        self.assertTrue(progress_cli.CALL_CONTRACT_VERIFICATION_ACCEPTANCE_ENABLED)
        self.assertFalse(progress_cli.CALL_CONTRACT_CURRENTNESS_AUDIT_ENABLED)
        self.assertTrue(progress_cli.CALL_CONTRACT_CURRENTNESS_AUDIT_DISABLED_REASON)
        self.assertFalse(progress_cli.CALL_CONTRACT_CONVERGENCE_ENABLED)
        self.assertTrue(progress_cli.CALL_CONTRACT_CONVERGENCE_DISABLED_REASON)
        parser = progress_cli._parser()
        with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit):
            parser.parse_args(
                [
                    "advance-live-call-contract",
                    "--slice",
                    "recoil:call-contract-slice:test",
                    "--build-root",
                    "build/test",
                    "--expected-semantic-revision",
                    "1",
                    "--expected-evidence-generation-revision",
                    "1",
                    "--dry-run",
                ]
            )

    def test_cli_accepts_strict_and_json(self) -> None:
        args = build_parser().parse_args(["--strict", "--json"])
        self.assertTrue(args.strict)
        self.assertTrue(args.json)


if __name__ == "__main__":
    unittest.main()
