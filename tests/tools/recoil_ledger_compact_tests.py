from __future__ import annotations

from copy import deepcopy
from pathlib import Path
import sys
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands.ledger_compact import (  # noqa: E402
    prepare_issue_compaction,
    prepare_progress_compaction,
    require_compaction_apply_allowed,
)
from _recoil.commands.workspace_issues import next_issue_id  # noqa: E402
from _recoil.commands.call_contract_verify import file_dependency_states  # noqa: E402
from _recoil.lib.call_contract_generations import current_generations  # noqa: E402
from _recoil.lib.progress import (  # noqa: E402
    AUTHORED_ORDER_DIMENSIONS,
    FULL_ORDER_DIMENSIONS,
    ProgressDocument,
    empty_progress_document,
    state_record,
)


def accepted_group(dimensions: tuple[str, ...]) -> dict[str, object]:
    return {
        dimension: state_record(
            "passed",
            "accepted",
            "current",
            ["recoil:evidence:r1:000001"],
            gating=True,
            validation_mode="live",
        )
        for dimension in dimensions
    }


def pending_group(dimensions: tuple[str, ...]) -> dict[str, object]:
    return {dimension: state_record() for dimension in dimensions}


def call_contract_progress_fixture() -> tuple[dict[str, object], str, str]:
    data = empty_progress_document()
    symbol_id = "recoil:function:0x401000"
    block_id = "recoil:block:0x401000"
    target_id = "recoil:vc5-target:unit"
    evidence_id = "recoil:evidence:r1:000001"
    superseded_id = "recoil:evidence:r1:000002"
    data["revision"] = 1
    data["binaries"]["recoil"] = {
        "text": {"start": "0x401000", "end_exclusive": "0x401010"}
    }
    data["physical_blocks"][block_id] = {
        "binary": "recoil",
        "start": "0x401000",
        "end_exclusive": "0x401010",
        "agent_source_path": "src/unit.cpp",
        "contribution_ids": [symbol_id],
        "semantic_span_ids": [],
        "order": {
            "authored": accepted_group(AUTHORED_ORDER_DIMENSIONS),
            "full": pending_group(FULL_ORDER_DIMENSIONS),
        },
        "accepted_order_facts": {
            "phase": "authored-function-order",
            "target_id": target_id,
            "matched_identities": [symbol_id],
        },
    }
    data["verification_targets"][target_id] = {
        "binary": "recoil",
        "name": "unit",
        "registered_addresses": ["0x401000"],
        "registration": {
            "source_from": "src/unit.cpp",
            "function_addresses": ["0x401000"],
            "translation_unit_function_order": [],
        },
    }
    slice_id = "recoil:call-contract-slice:0x401000-0x401000"
    data["symbols"][symbol_id] = {
        "binary": "recoil",
        "kind": "function",
        "start": "0x401000",
        "end_exclusive": "0x401010",
        "size": 16,
        "extent_state": "known",
        "physical_block_id": block_id,
        "pipeline_class": "authored",
        "authored_order_role": "authored-body",
        "binary_state": {
            "call_contract": state_record(
                "passed",
                "accepted",
                "current",
                [evidence_id],
                gating=True,
                validation_mode="live",
            )
        },
        "evidence_ids": [evidence_id, superseded_id],
    }
    data["evidence"][evidence_id] = {
        "kind": "live-authored-call-contract-validation",
        "summary": "current",
        "scope_ids": [symbol_id],
        "result": "passed",
        "disposition": "accepted",
        "freshness": "current",
        "gating": True,
        "validation_mode": "live",
        "artifacts": [],
        "provenance": {
            **current_generations(),
            "symbol_id": symbol_id,
            "address": "0x401000",
            "target_id": target_id,
            "physical_block_id": block_id,
            "slice_id": slice_id,
            "expected_truth": (
                "retail-binary-ninja-plus-reviewed-tracker-identities"
            ),
            "fresh_build": True,
            "reuse": False,
            "comparison_passed": True,
            "expected_contract": [{"form": "call", "callee": "provider:unit"}],
            "candidate_contract": [{"form": "call", "callee": "provider:unit"}],
            "normalizers": [],
            "binary_ninja_session": {
                "begin": {
                    "saved_view": "Recoil.bndb",
                    "generation_token": "7",
                    "revision": "11",
                    "schema": "recoil-binja-authenticated-snapshot-v2",
                    "authenticated": True,
                    "provider": "binary-ninja",
                    "capability_version": "2",
                },
                "end": {
                    "saved_view": "Recoil.bndb",
                    "generation_token": "7",
                    "revision": "11",
                    "schema": "recoil-binja-authenticated-snapshot-v2",
                    "authenticated": True,
                    "provider": "binary-ninja",
                    "capability_version": "2",
                },
                "snapshot_equal": True,
                "exact_fact_transcript": [
                    {"symbol_id": symbol_id, "calls": [{"form": "call"}]}
                ],
            },
        },
    }
    data["evidence"][superseded_id] = {
        "kind": "live-authored-call-contract-validation",
        "summary": "superseded",
        "scope_ids": [symbol_id],
        "result": "passed",
        "disposition": "accepted",
        "freshness": "historical",
        "gating": True,
        "validation_mode": "live",
        "artifacts": [],
        "provenance": {},
    }
    data["migration"] = {
        "authored_call_contract_v1": {"keep": True},
        "source_traceability_v1": {"keep": True},
        "schema_v4": {"remove": True},
        "future_v9": {"keep": True},
    }
    data["work_items"]["recoil:work:terminal"] = {"state": "closed"}
    return data, symbol_id, evidence_id


class RecoilLedgerCompactTests(unittest.TestCase):
    def test_progress_compaction_preserves_direct_body_evidence_without_refresh(self) -> None:
        data, symbol_id, evidence_id = call_contract_progress_fixture()
        dependency_path = "tests/tools/recoil_ledger_compact_tests.py"
        dependencies_before = file_dependency_states([dependency_path])
        before_projection = ProgressDocument(deepcopy(data)).next_work("recoil")
        compacted, report = prepare_progress_compaction(data)
        second, second_report = prepare_progress_compaction(compacted)
        compacted_document = ProgressDocument(compacted)
        after_projection = compacted_document.next_work("recoil")
        slice_row = compacted_document.authored_call_contract_slices()[0]
        dependencies_after = file_dependency_states([dependency_path])

        self.assertEqual(dependencies_before, dependencies_after)
        self.assertEqual(dependency_path, dependencies_before[0]["path"])
        self.assertTrue(dependencies_before[0]["exists"])
        self.assertIsInstance(dependencies_before[0]["physical_identity"], dict)
        self.assertGreater(dependencies_before[0]["size"], 0)
        self.assertTrue(compacted_document._call_contract_slice_current(slice_row))

        symbol = compacted["symbols"][symbol_id]
        self.assertNotIn("accepted_call_contract_facts", symbol)
        self.assertEqual([evidence_id], symbol["evidence_ids"])
        self.assertEqual(
            "retail-binary-ninja-plus-reviewed-tracker-identities",
            compacted["evidence"][evidence_id]["provenance"]["expected_truth"],
        )
        provenance = compacted["evidence"][evidence_id]["provenance"]
        self.assertTrue(provenance["comparison_passed"])
        self.assertEqual(
            provenance["expected_contract"], provenance["candidate_contract"]
        )
        self.assertNotIn("recoil:evidence:r1:000002", compacted["evidence"])
        self.assertNotIn("recoil:work:terminal", compacted["work_items"])
        self.assertNotIn("schema_v4", compacted["migration"])
        self.assertIn("future_v9", compacted["migration"])
        self.assertTrue(report["parity"]["passed"])
        self.assertEqual(before_projection, after_projection)
        launchability = after_projection["cursor_launchability"]
        self.assertEqual(
            "contained-disabled",
            launchability["primary"]["reason_code"],
        )
        self.assertEqual(
            "repair-call-contract-readiness",
            launchability["primary"]["required_parent_action"],
        )
        self.assertEqual(
            "byte-preflight-blocked",
            launchability["parallel_authored_byte"]["reason_code"],
        )
        self.assertIn(
            "lacks its canonical address",
            launchability["parallel_authored_byte"]["reason"],
        )
        self.assertEqual(0, report["converted_call_contract_slices"])
        self.assertEqual(0, second_report["converted_call_contract_slices"])
        self.assertEqual(
            0, second_report["removed_counts"]["superseded_call_contract_evidence"]
        )
        self.assertEqual(compacted, second)

    def test_issue_compaction_retains_only_active_semantics_and_high_water(self) -> None:
        data = {
            "version": 2,
            "revision": 8,
            "id_sequences": {},
            "issues": [
                {"id": "WSI-20260805-007", "status": "resolved"},
                {
                    "id": "WSI-20260805-006",
                    "status": "open",
                    "summary": "current",
                    "history": [{"action": "created"}],
                },
            ],
            "work_packets": [
                {"id": "issue:work:closed", "state": "closed"},
                {"id": "issue:work:ready", "state": "ready"},
            ],
            "reservations": [
                {"id": "issue:work:closed:attempt:1", "state": "released"}
            ],
        }

        compacted, report = prepare_issue_compaction(data)
        self.assertEqual(["WSI-20260805-006"], [row["id"] for row in compacted["issues"]])
        self.assertNotIn("history", compacted["issues"][0])
        self.assertEqual(7, compacted["id_sequences"]["issue"]["20260805"])
        self.assertEqual(
            "WSI-20260805-008",
            next_issue_id(
                compacted["issues"],
                date_key="20260805",
                id_sequences=compacted["id_sequences"],
            ),
        )
        self.assertEqual(["issue:work:ready"], [row["id"] for row in compacted["work_packets"]])
        self.assertEqual([], compacted["reservations"])
        self.assertTrue(report["parity"]["passed"])

    def test_issue_compaction_apply_gate_refuses_active_reservation(self) -> None:
        data = {
            "version": 2,
            "revision": 1,
            "id_sequences": {},
            "issues": [],
            "work_packets": [],
            "reservations": [{"id": "active", "state": "active"}],
        }
        _candidate, report = prepare_issue_compaction(data)
        self.assertFalse(report["apply_allowed"])
        with self.assertRaisesRegex(ValueError, "active issue reservation"):
            require_compaction_apply_allowed(report)


if __name__ == "__main__":
    unittest.main()
