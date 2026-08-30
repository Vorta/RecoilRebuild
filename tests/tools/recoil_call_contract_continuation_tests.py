from __future__ import annotations

from copy import deepcopy
from pathlib import Path
from types import SimpleNamespace
import sys
import unittest


ROOT = Path(__file__).resolve().parents[2]
TOOLS = ROOT / "tools"
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

from _recoil.commands import call_contract_continuation as continuation  # noqa: E402
from _recoil.commands import progress_cli  # noqa: E402
from _recoil.lib.progress import ProgressError  # noqa: E402


def _predecessor() -> dict[str, object]:
    work = {
        "state": "returned-tool-blocked",
        "phase": "authored-call-contract",
        "target_id": "recoil:vc5-target:camera",
        "cursor": "0x44b8c0",
        "block_id": "recoil:block:0x44b8c0",
        "covered_block_ids": ["recoil:block:0x44b8c0"],
        "scope_ids": ["recoil:function:0x44b8c0"],
        "target_ids": ["recoil:vc5-target:camera"],
        "original_slice_ids": ["slice:camera"],
        "allowed_paths": ["src/GameZRecoil/zClass/Camera.c"],
        "source_edit_paths": ["src/GameZRecoil/zClass/Camera.c"],
        "definition_source_paths": ["src/GameZRecoil/zMath/zmth_main.c"],
        "dependency_paths": [
            "src/GameZRecoil/zClass/Camera.c",
            "src/GameZRecoil/zMath/zmth_decls.h",
            "src/GameZRecoil/zMath/zmth_main.c",
        ],
    }
    work[continuation.RETURN_PROVENANCE_FIELD] = continuation.returned_tool_blocked_provenance(
        None, "predecessor", work, continuation.LINKED_TOOL_ISSUE
    )
    return work


def _document(predecessor: dict[str, object]) -> SimpleNamespace:
    producer = {
        "packet_type": continuation.CONTINUATION_PRODUCER_TYPE,
        "state": "returned",
    }
    return SimpleNamespace(
        revision=47,
        data={
            "migration": {},
            "work_items": {"predecessor": predecessor, "producer": producer},
        },
    )


class CallContractContinuationTests(unittest.TestCase):
    def test_parser_requires_parent_producer_and_no_operator_route_facts(self) -> None:
        parser = progress_cli._parser()
        parsed = parser.parse_args([
            "call-contract", "prepare-repair-continuation",
            "--producer-packet", "producer",
            "--returned-work-item", "predecessor",
            "--build-root", "build/producer",
            "--expected-revision", "47", "--apply",
        ])
        self.assertEqual("producer", parsed.producer_packet)
        for forbidden in ("caller", "owner", "declaration", "definition"):
            self.assertFalse(hasattr(parsed, forbidden))

    def test_generic_unique_routing_builds_one_nonaccepting_v2_child(self) -> None:
        predecessor = _predecessor()
        document = _document(predecessor)
        divergence = {
            "symbol_id": "recoil:function:0x44b8c0",
            "address": "0x44b8c0",
            "ordinal": 1,
            "kind": "verifier-blocked",
            "expected": {"form": "call", "target_identity": "retail:target"},
            "candidate": {"form": "call", "target_identity": "candidate:abi"},
            "repair_routing": {
                "schema": "call-contract-continuation-routing-evidence-v1",
                "caller_edit_path": "src/GameZRecoil/zClass/Camera.c",
                "controlling_declaration_path": "src/GameZRecoil/zMath/zmth_decls.h",
                "controlling_definition_path": "src/GameZRecoil/zMath/zmth_main.c",
                "caller_symbol_id": "recoil:function:0x44b8c0",
                "caller_physical_block_id": "recoil:block:0x44b8c0",
                "caller_owner_id": "recoil:owner:camera",
                "caller_semantic_span_ids": ["recoil:span:0x44b8c0"],
                "unique_controlling_pair": True,
                "authored_route": True,
                "provider_boundary": False,
                "out_of_policy": False,
                "candidate_expected_truth": False,
            },
        }
        result = {
            "schema": continuation.PRODUCER_RESULT_SCHEMA,
            "packet_id": "producer",
            "all_authored_bodies": True,
            "all_caller_divergences_collected": True,
            "candidate_expected_truth": False,
            "caller_divergences": [divergence],
        }
        preparation = continuation.prepare_repair_continuation(
            document, "predecessor", predecessor,
            {"issues": [{"id": continuation.LINKED_TOOL_ISSUE, "status": "in-progress"}]},
            Path("build/producer"),
            producer_work_item_id="producer", producer_result=result,
        )
        self.assertEqual("descriptor-ready", preparation.checkpoint["state"])
        child = preparation.child_descriptor
        self.assertEqual("call-contract-repair-continuation-edit-v2", child["packet_type"])
        self.assertEqual(3, len(child["allowed_paths"]))
        self.assertFalse(child["candidate_expected_truth"])
        self.assertTrue(child["route_descriptor"]["fresh_parent_acceptance_required"])

    def test_ambiguous_or_provider_route_fails_closed_without_child(self) -> None:
        predecessor = _predecessor()
        document = _document(predecessor)
        for routing in (
            {"unique_controlling_pair": False},
            {"unique_controlling_pair": True, "authored_route": True, "provider_boundary": True},
        ):
            with self.subTest(routing=routing):
                row = {
                    "address": "0x44b8c0", "ordinal": 1, "kind": "verifier-blocked",
                    "repair_routing": {
                        "caller_edit_path": "src/GameZRecoil/zClass/Camera.c",
                        "controlling_declaration_path": "src/GameZRecoil/zMath/zmth_decls.h",
                        "controlling_definition_path": "src/GameZRecoil/zMath/zmth_main.c",
                        "caller_symbol_id": "recoil:function:0x44b8c0",
                        "caller_physical_block_id": "recoil:block:0x44b8c0",
                        "caller_owner_id": "recoil:owner:camera",
                        "caller_semantic_span_ids": ["recoil:span:0x44b8c0"],
                        "out_of_policy": False, **routing,
                    },
                }
                producer_result = {
                    "schema": continuation.PRODUCER_RESULT_SCHEMA,
                    "packet_id": "producer", "all_authored_bodies": True,
                    "all_caller_divergences_collected": True,
                    "candidate_expected_truth": False, "caller_divergences": [row],
                }
                preparation = continuation.prepare_repair_continuation(
                    document, "predecessor", predecessor,
                    {"issues": [{"id": continuation.LINKED_TOOL_ISSUE, "status": "open"}]},
                    Path("build/producer"), producer_work_item_id="producer",
                    producer_result=producer_result,
                )
                self.assertEqual("route-blocked", preparation.checkpoint["state"])
                self.assertIsNone(preparation.child_descriptor)

    def test_snapshot_and_state_transitions_are_exact(self) -> None:
        predecessor = _predecessor()
        document = _document(predecessor)
        snapshot = continuation.capture_continuation_input_snapshot(
            document, predecessor[continuation.RETURN_PROVENANCE_FIELD]
        )
        self.assertTrue(continuation.continuation_snapshots_equal(snapshot, deepcopy(snapshot)))
        self.assertEqual(13, snapshot["generations"]["call_contract_verifier_generation"])
        with self.assertRaises(ProgressError):
            continuation.activate_continuation_child({"state": "descriptor-ready"}, child_work_item_id="child")


if __name__ == "__main__":
    unittest.main()
