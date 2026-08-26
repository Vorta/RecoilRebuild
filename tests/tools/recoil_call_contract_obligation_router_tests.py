from __future__ import annotations

from copy import deepcopy
from pathlib import Path
import inspect
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import patch

REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands import progress_cli
from _recoil.lib.call_contract_generations import (
    CALL_CONTRACT_VERIFIER_GENERATION,
    EXPECTED_FACT_SCHEMA_VERSION,
    NORMALIZER_REGISTRY_GENERATION,
)
from _recoil.lib.progress import (
    resource_claim_conflicts,
    work_resource_claims,
)


class _Document:
    revision = 50
    path = Path(".agent/RECONSTRUCTION_PROGRESS.sqlite3")

    def __init__(self) -> None:
        self.data = {"migration": {}, "work_items": {}}

    def collection(self, name: str):
        return self.data.get(name, {})

    def _fresh_root(self, kind: str, label: str, revision: int) -> str:
        return f"build/{kind}/{label}/r{revision}"

    def pipeline(self, binary: str, *, resolve_order_target: bool = False):
        self.assert_binary = binary
        self.assert_resolve_order_target = resolve_order_target
        return {
            "phase": "authored-call-contract",
            "primary_lane": "call-contract",
        }

    def show(self, cursor: str):
        return {"id": cursor}

    def scheduler_output(self, value):
        return value


def _descriptor(identity: str, kind: str, write_path: str) -> dict:
    return {
        "obligation_id": identity,
        "obligation_kind": kind,
        "target_id": "recoil:verification-target:mixed",
        "cursor": "0x401000",
        "symbol_ids": ["recoil:function:0x401000"],
        "physical_block_ids": ["recoil:block:0x401000"],
        "write_paths": [write_path],
        "dependency_paths": [write_path, "tools/recoil.py"],
        "validation_commands": [
            "python tools/recoil.py verify call-contract --target mixed "
            "--build-root {packet_build_root} --progress {progress_path} --json"
        ],
        "objective": f"Repair {kind} obligation.",
        "stop_condition": "PASS or return the scoped contradiction.",
    }


class CallContractObligationRouterTests(unittest.TestCase):
    def _candidates(self, descriptors: list[dict]):
        document = _Document()
        generation = {
            "generation_id": "recoil:call-contract-convergence:r50",
            "obligation_descriptors": descriptors,
        }
        with (
            patch.object(
                progress_cli,
                "convergence_generation_state",
                return_value={"current": True, "generation": generation},
            ),
            patch.object(
                progress_cli,
                "bind_work_packet_contract",
                side_effect=lambda _document, value: value,
            ),
            patch.object(
                progress_cli,
                "_candidate_active_conflicts",
                return_value=([], []),
            ),
        ):
            return progress_cli._call_contract_mixed_obligation_candidates(
                document,
                progress_path=document.path,
                issue_ledger=Path(".agent/WORKSPACE_ISSUES.sqlite3"),
            )

    def test_same_target_disjoint_source_and_verifier_packets_coexist(self) -> None:
        candidates, blockers = self._candidates(
            [
                _descriptor("obligation:source", "source", "src/a.cpp"),
                _descriptor(
                    "obligation:verifier",
                    "verifier",
                    "tools/_recoil/commands/audit.py",
                ),
            ]
        )
        self.assertEqual([], blockers)
        self.assertEqual(2, len(candidates))
        packets = [packet for _work_id, packet in candidates]
        self.assertEqual(
            [
                "call-contract-source-obligation-v1",
                "call-contract-verifier-obligation-v1",
            ],
            [packet["packet_type"] for packet in packets],
        )
        self.assertEqual(
            {"recoil:verification-target:mixed"},
            {packet["target_id"] for packet in packets},
        )
        first_claims, first_complete, _ = work_resource_claims(packets[0])
        second_claims, second_complete, _ = work_resource_claims(packets[1])
        self.assertTrue(first_complete)
        self.assertTrue(second_complete)
        self.assertEqual(
            [],
            resource_claim_conflicts(
                second_claims,
                candidates[0][0],
                first_claims,
            ),
        )

    def test_overlapping_same_target_writes_conflict_by_resource_not_target(self) -> None:
        candidates, blockers = self._candidates(
            [
                _descriptor("obligation:source", "source", "src/shared.h"),
                _descriptor("obligation:profile", "profile", "src/shared.h"),
            ]
        )
        self.assertEqual([], blockers)
        self.assertEqual(2, len(candidates))
        left_claims, left_complete, _ = work_resource_claims(candidates[0][1])
        right_claims, right_complete, _ = work_resource_claims(candidates[1][1])
        self.assertTrue(left_complete)
        self.assertTrue(right_complete)
        conflicts = resource_claim_conflicts(
            right_claims,
            candidates[0][0],
            left_claims,
        )
        self.assertTrue(conflicts)
        self.assertTrue(
            any("src/shared.h" in conflict.get("paths", []) for conflict in conflicts)
        )

    def test_all_mixed_obligations_survive_exact_reserved_handoff(self) -> None:
        descriptors = [
            _descriptor("obligation:source", "source", "src/a.cpp"),
            _descriptor(
                "obligation:profile",
                "profile",
                "tools/_recoil/config/profile.json",
            ),
            _descriptor(
                "obligation:verifier",
                "verifier",
                "tools/_recoil/commands/call_contract_verify.py",
            ),
            _descriptor(
                "obligation:linker",
                "linker",
                "tools/_recoil/commands/vc5_final_build.py",
            ),
        ]
        candidates, blockers = self._candidates(descriptors)
        self.assertEqual([], blockers)
        document = _Document()
        expected_roles = {
            "source": "recoil_source_worker",
            "profile": "recoil_tool_maintainer",
            "verifier": "recoil_tool_maintainer",
            "linker": "recoil_tool_maintainer",
        }
        for ordinal, (work_id, work) in enumerate(candidates, 1):
            work["state"] = "active"
            work["reservation"] = {
                "id": f"{work_id}:attempt:1",
                "state": "active",
            }
            document.data["work_items"][work_id] = work
            handoff = progress_cli._handoff(
                document,
                SimpleNamespace(
                    packet_id=work_id,
                    issue_ledger=Path(".agent/WORKSPACE_ISSUES.sqlite3"),
                ),
            )["work_item"]
            descriptor = descriptors[ordinal - 1]
            kind = descriptor["obligation_kind"]
            self.assertEqual(descriptor["obligation_id"], handoff["obligation_id"])
            self.assertEqual(kind, handoff["obligation_kind"])
            self.assertEqual(expected_roles[kind], handoff["handoff_role"])
            self.assertEqual(descriptor["write_paths"], handoff["write_paths"])
            self.assertEqual(["tools/recoil.py"], handoff["read_paths"])
            self.assertTrue(handoff["build_root"].startswith("build/"))
            self.assertTrue(handoff["nonaccepting"])
            self.assertFalse(handoff["acceptance_eligible"])
            self.assertFalse(handoff["worker_acceptance_allowed"])

    def test_reserved_handoff_rejects_role_or_path_contract_drift(self) -> None:
        candidates, blockers = self._candidates(
            [
                _descriptor(
                    "obligation:verifier",
                    "verifier",
                    "tools/_recoil/commands/call_contract_verify.py",
                )
            ]
        )
        self.assertEqual([], blockers)
        work_id, work = candidates[0]
        work["state"] = "active"
        work["reservation"] = {"id": f"{work_id}:attempt:1", "state": "active"}
        work["handoff_role"] = "recoil_source_worker"
        with self.assertRaisesRegex(progress_cli.ProgressError, "kind/role"):
            progress_cli._compact_reserved_packet(work_id, work)
        work["handoff_role"] = "recoil_tool_maintainer"
        work["source_edit_paths"] = list(work["allowed_paths"])
        with self.assertRaisesRegex(progress_cli.ProgressError, "source edit paths"):
            progress_cli._compact_reserved_packet(work_id, work)

    def test_cli_exposes_domain_only_acceptance_and_closeout_guards(self) -> None:
        parser = progress_cli._parser()
        advance = parser.parse_args(
            [
                "advance-live-call-contract",
                "--slice",
                "recoil:slice:0001",
                "--packet-id",
                "recoil:explicit-work:leaf",
                "--build-root",
                "build/call-contract",
                "--expected-semantic-revision",
                "9",
                "--expected-evidence-generation-revision",
                "10",
                "--dry-run",
            ]
        )
        self.assertIsNone(advance.expected_revision)
        self.assertEqual(9, advance.expected_semantic_revision)
        self.assertEqual(10, advance.expected_evidence_generation_revision)
        closeout = parser.parse_args(
            [
                "call-contract",
                "prepare-live-convergence",
                "--packet-id",
                "recoil:explicit-work:closeout",
                "--build-root",
                "build/closeout",
                "--jobs",
                "4",
                "--closeout",
                "--expected-semantic-revision",
                "11",
                "--expected-evidence-generation-revision",
                "12",
                "--dry-run",
            ]
        )
        self.assertTrue(closeout.closeout)
        self.assertIsNone(closeout.expected_revision)
        self.assertEqual("recoil:explicit-work:leaf", advance.packet_id)
        self.assertEqual("recoil:explicit-work:closeout", closeout.packet_id)

    def test_status_projects_call_contract_routes_with_domain_guards(self) -> None:
        document = _Document()
        document.path = Path("tracker.sqlite3")
        vector = SimpleNamespace(
            semantic_revision=21,
            evidence_generation_revision=34,
            scheduler_revision=55,
        )
        sqlite = SimpleNamespace(read_revision_vector=lambda: vector)
        payload = {
            "next_command": (
                "python tools/recoil.py progress call-contract "
                "prepare-live-convergence --packet-id recoil:explicit-work:closeout "
                "--build-root build/live --jobs 4 "
                "--expected-revision 4735 --apply --json"
            ),
            "followup": {
                "command": (
                    "python tools/recoil.py progress advance-live-call-contract "
                    "--slice recoil:slice:1 --packet-id recoil:explicit-work:leaf "
                    "--build-root build/leaf "
                    "--expected-revision 4735 --apply --json"
                )
            },
        }
        with patch.object(progress_cli, "ProgressSQLiteStore", return_value=sqlite):
            projected = progress_cli._scheduler_domain_guarded_call_contract_commands(
                document, payload
            )
        for command in (
            projected["next_command"],
            projected["followup"]["command"],
        ):
            self.assertNotIn("--expected-revision", command)
            self.assertIn("--expected-semantic-revision 21", command)
            self.assertIn("--expected-evidence-generation-revision 34", command)
            self.assertIn("--packet-id recoil:explicit-work:", command)
        malformed = {
            "next_command": (
                "python tools/recoil.py progress call-contract "
                "prepare-live-convergence --packet-id recoil:explicit-work:closeout "
                "--expected-semantic-revision 21 "
                "--expected-revision 55 --apply --json"
            )
        }
        with (
            patch.object(progress_cli, "ProgressSQLiteStore", return_value=sqlite),
            self.assertRaisesRegex(progress_cli.ProgressError, "mixed or incomplete"),
        ):
            progress_cli._scheduler_domain_guarded_call_contract_commands(
                document, malformed
            )

    def test_policy_requires_fresh_parent_direct_comparison(self) -> None:
        policy = progress_cli.CALL_CONTRACT_VERIFICATION_ACCEPTANCE_POLICY
        self.assertEqual("fresh-parent-direct-retail", policy["acceptance_authority"])
        self.assertEqual(
            "explicit-invalidation-and-generations", policy["identity_role"]
        )
        self.assertFalse(policy["worker_acceptance_allowed"])
        self.assertTrue(policy["partial_body_acceptance"])
        self.assertTrue(policy["phase_closeout_no_reuse"])
        self.assertTrue(progress_cli.CALL_CONTRACT_VERIFICATION_ACCEPTANCE_ENABLED)

    def test_partial_result_keeps_failed_sibling_pending(self) -> None:
        slice_row = {
            "id": "recoil:slice:partial",
            "symbol_ids": ["recoil:function:a", "recoil:function:b"],
            "addresses": ["0x401000", "0x401010"],
            "target_ids": ["recoil:target:mixed"],
            "physical_block_ids": ["recoil:block:mixed"],
            "body_count": 2,
        }
        dependency_states = [
            {
                "path": "src/a.cpp",
                "exists": True,
                "size": 1,
                "mtime_ns": 1,
                "volume_serial": 7,
                "file_id": "fixture:source-a",
            }
        ]
        result = {
            "kind": "authored-call-contract-live-result",
            "contract_version": progress_cli.CALL_CONTRACT_CONTRACT_VERSION,
            "packet_id": "recoil:explicit-work:acceptance",
            "slice_id": slice_row["id"],
            "symbol_ids": slice_row["symbol_ids"],
            "target_ids": slice_row["target_ids"],
            "physical_block_ids": slice_row["physical_block_ids"],
            "body_count": 2,
            "candidate_expected_truth": False,
            "source_write_paths": ["src/a.cpp"],
            "source_edit_paths": ["src/a.cpp"],
            "definition_source_paths": [],
            "definition_compile_results": [],
            "dependency_paths": ["src/a.cpp"],
            "dependency_states_before": dependency_states,
            "dependency_states_after": dependency_states,
            "source_changed_during_validation": False,
            "passed": False,
            "first_divergence": {
                "kind": "mismatch",
                "symbol_id": "recoil:function:b",
            },
            "binary_ninja_session": {
                "snapshot_equal": True,
                "begin": {"provider_revision": "fixture:1"},
                "end": {"provider_revision": "fixture:1"},
                "exact_fact_transcript": [],
            },
            "all_caller_divergences_collected": True,
            "body_results": [
                {
                    "symbol_id": "recoil:function:a",
                    "address": "0x401000",
                    "status": "passed",
                    "comparison_passed": True,
                    "expected_contract": [],
                    "candidate_contract": [],
                    "call_contract_verifier_generation": (
                        CALL_CONTRACT_VERIFIER_GENERATION
                    ),
                    "normalizer_registry_generation": (
                        NORMALIZER_REGISTRY_GENERATION
                    ),
                    "expected_fact_schema_version": EXPECTED_FACT_SCHEMA_VERSION,
                },
                {
                    "symbol_id": "recoil:function:b",
                    "address": "0x401010",
                    "status": "divergent",
                    "comparison_passed": False,
                    "expected_contract": [],
                    "candidate_contract": [{"kind": "call"}],
                    "call_contract_verifier_generation": (
                        CALL_CONTRACT_VERIFIER_GENERATION
                    ),
                    "normalizer_registry_generation": (
                        NORMALIZER_REGISTRY_GENERATION
                    ),
                    "expected_fact_schema_version": EXPECTED_FACT_SCHEMA_VERSION,
                },
            ],
        }
        validated = progress_cli._validate_call_contract_result(
            result,
            expected_slice=slice_row,
            expected_source_write_paths=["src/a.cpp"],
            expected_definition_source_paths=[],
            expected_compiled_definition_sources=[],
            expected_dependency_paths=["src/a.cpp"],
            expected_packet_id="recoil:explicit-work:acceptance",
        )
        self.assertEqual(["recoil:function:a"], validated["passing_symbol_ids"])
        self.assertEqual(
            ["recoil:function:b"],
            [
                row["symbol_id"]
                for row in validated["body_results"]
                if row["status"] != "passed"
            ],
        )
        self.assertEqual(
            ["passed", "divergent"],
            [row["status"] for row in validated["body_results"]],
        )
        self.assertFalse(validated["passed"])

        stale_result = deepcopy(result)
        stale_result["body_results"][0].update(
            {
                "call_contract_verifier_generation": 5,
                "normalizer_registry_generation": 5,
                "expected_fact_schema_version": 5,
            }
        )
        with self.assertRaisesRegex(
            progress_cli.ProgressError,
            "call-contract direct body result is incomplete for recoil:function:a",
        ):
            progress_cli._validate_call_contract_result(
                stale_result,
                expected_slice=slice_row,
                expected_source_write_paths=["src/a.cpp"],
                expected_definition_source_paths=[],
                expected_compiled_definition_sources=[],
                expected_dependency_paths=["src/a.cpp"],
                expected_packet_id="recoil:explicit-work:acceptance",
            )

    def test_parent_advance_runs_one_fresh_direct_verification_after_preflight(self) -> None:
        source = inspect.getsource(progress_cli.advance_live_call_contract)
        self.assertIn("_preflight_call_contract_expensive_operation", source)
        self.assertIn("_run_json_process", source)
        self.assertIn("_validate_call_contract_result", source)
        self.assertIn('"fresh_build": True', source)
        self.assertIn('"reuse": False', source)


if __name__ == "__main__":
    unittest.main()
