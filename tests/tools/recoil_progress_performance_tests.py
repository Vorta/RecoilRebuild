from __future__ import annotations

from copy import deepcopy
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands import live_byte_verify  # noqa: E402
from _recoil.lib.call_contract_generations import current_generations  # noqa: E402
from _recoil.lib.progress import (  # noqa: E402
    AUTHORED_ORDER_DIMENSIONS,
    CALL_CONTRACT_DIMENSION,
    CALL_CONTRACT_CONTRACT_VERSION,
    CALL_CONTRACT_EXPECTED_TRUTH,
    ProgressDocument,
    ProgressStore,
    empty_progress_document,
    state_record,
)
from _recoil.lib.progress_sqlite import ProgressSQLiteStore  # noqa: E402


def accepted_state() -> dict[str, object]:
    return state_record(
        "passed",
        "accepted",
        "current",
        [],
        validation_mode="live",
    )


def one_authored_body_document() -> ProgressDocument:
    data = empty_progress_document()
    data["binaries"] = {
        "recoil": {
            "text": {"start": "0x401000", "end_exclusive": "0x401010"}
        }
    }
    block_id = "recoil:block:0x401000"
    symbol_id = "recoil:function:0x401000"
    target_id = "recoil:vc5-target:unit"
    data["physical_blocks"] = {
        block_id: {
            "binary": "recoil",
            "start": "0x401000",
            "end_exclusive": "0x401010",
            "agent_source_path": "src/unit.cpp",
            "order": {
                "authored": {
                    dimension: accepted_state()
                    for dimension in AUTHORED_ORDER_DIMENSIONS
                }
            },
            "accepted_order_facts": {
                "phase": "authored-function-order",
                "target_id": target_id,
                "matched_identities": [symbol_id],
            },
        }
    }
    data["symbols"] = {
        symbol_id: {
            "binary": "recoil",
            "kind": "function",
            "address": "0x401000",
            "end_exclusive": "0x401010",
            "pipeline_class": "authored",
            "authored_order_role": "authored-body",
            "physical_block_id": block_id,
            "binary_state": {},
        }
    }
    data["verification_targets"] = {
        target_id: {
            "name": "unit",
            "registered_addresses": ["0x401000"],
            "registration": {
                "function_addresses": ["0x401000"],
                "order_edit_paths": ["src/unit.cpp"],
            },
        }
    }
    return ProgressDocument(data)


def accept_first_call_contract_body(
    document: ProgressDocument,
) -> tuple[dict[str, object], list[dict[str, object]]]:
    slice_row = document.authored_call_contract_slices()[0]
    symbol_id = str(slice_row["symbol_ids"][0])
    symbol = document.collection("symbols")[symbol_id]
    evidence_id = "recoil:evidence:r0:000001"
    symbol["binary_state"][CALL_CONTRACT_DIMENSION] = state_record(
        "passed",
        "accepted",
        "current",
        [evidence_id],
        validation_mode="live",
    )
    transcript = [
        {
            "symbol_id": symbol_id,
            "address": "0x401000",
            "expected_fact_row": {
                **current_generations(),
                "symbol_id": symbol_id,
                "address": "0x401000",
                "calls": [],
            },
        }
    ]
    document.collection("evidence")[evidence_id] = {
        "kind": "live-authored-call-contract-validation",
        "result": "passed",
        "disposition": "accepted",
        "freshness": "current",
        "validation_mode": "live",
        "gating": True,
        "scope_ids": [symbol_id],
        "provenance": {
            **current_generations(),
            "symbol_id": symbol_id,
            "address": "0x401000",
            "physical_block_id": "recoil:block:0x401000",
            "comparison_passed": True,
            "expected_contract": [],
            "candidate_contract": [],
            "exact_fact_transcript": transcript,
        },
    }
    return slice_row, transcript


class RecoilProgressPerformanceTests(unittest.TestCase):
    def test_owned_sqlite_loads_skip_redundant_progress_document_copy(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "progress.sqlite3"
            ProgressSQLiteStore.create_from_mapping(
                path,
                empty_progress_document(),
                cutover_pair_id="owned-load-test",
            )
            with patch(
                "_recoil.lib.progress.deepcopy",
                side_effect=AssertionError("fresh decode was copied"),
            ):
                direct = ProgressDocument.load(path)
                stored = ProgressStore(path).load()
            self.assertEqual(0, direct.revision)
            self.assertEqual(0, stored.revision)

    def test_public_constructor_and_clone_remain_defensive(self) -> None:
        source = empty_progress_document()
        document = ProgressDocument(source)
        source["revision"] = 7
        self.assertEqual(0, document.revision)

        cloned = document.clone()
        document.data["revision"] = 9
        self.assertEqual(0, cloned.revision)

    def test_request_indexes_and_derived_rows_are_copy_safe(self) -> None:
        document = one_authored_body_document()

        blocks = document._blocks_for_binary("recoil")
        symbols = document._symbols_for_binary("recoil")
        groups = document._physical_groups("recoil", gating_only=True)
        slices = document.authored_call_contract_slices()

        blocks.clear()
        symbols.clear()
        groups[0]["scope_ids"].clear()
        groups.clear()
        slices[0]["symbol_ids"].clear()
        slices.clear()

        self.assertEqual(1, len(document._blocks_for_binary("recoil")))
        self.assertEqual(1, len(document._symbols_for_binary("recoil")))
        self.assertEqual(
            ["recoil:function:0x401000"],
            document._physical_groups("recoil", gating_only=True)[0]["scope_ids"],
        )
        self.assertEqual(
            ["recoil:function:0x401000"],
            document.authored_call_contract_slices()[0]["symbol_ids"],
        )

    def test_pending_call_contract_slice_skips_exact_source_closure(self) -> None:
        document = one_authored_body_document()
        slice_row = document.authored_call_contract_slices()[0]
        with patch(
            "_recoil.commands.call_contract_verify.source_dependency_paths",
            side_effect=AssertionError("pending slice derived exact closure"),
        ):
            self.assertFalse(document._call_contract_slice_current(slice_row))

    def test_current_slice_uses_accepted_state_without_source_rescan(self) -> None:
        document = one_authored_body_document()
        slice_row, _transcript = accept_first_call_contract_body(document)

        with patch(
            "_recoil.commands.call_contract_verify.source_dependency_paths",
            side_effect=AssertionError("currentness rescanned source closure"),
        ):
            self.assertTrue(document._call_contract_slice_current(slice_row))
            status = document._call_contract_slice_status(slice_row)
            self.assertEqual(
                "accepted-state-explicit-invalidation", status["storage_mode"]
            )
            self.assertEqual(1, status["accepted_body_count"])
            self.assertEqual(0, status["remaining_body_count"])
            self.assertEqual(
                "accepted-and-not-invalidated",
                status["body_statuses"][0]["reason"],
            )

    def test_phase_closeout_requires_v3_ordered_fact_transcript_scan(self) -> None:
        document = one_authored_body_document()
        slice_row, transcript = accept_first_call_contract_body(document)
        closeout = {
            "schema": "recoil-call-contract-fresh-closeout-v3",
            "ordered_symbol_ids": list(slice_row["symbol_ids"]),
            "slice_ids": [slice_row["id"]],
            "complete_no_reuse_zero_divergence": True,
            "fresh_build": True,
            "reuse": False,
            "candidate_expected_truth": False,
            "scan_rows": [
                {
                    "slice_id": slice_row["id"],
                    "body_count": slice_row["body_count"],
                    "build_root": "build/live-validation/unit",
                    "exact_fact_transcript": deepcopy(transcript),
                }
            ],
            **current_generations(),
        }
        document.data.setdefault("migration", {})[
            "authored_call_contract_fresh_closeout_v3"
        ] = closeout

        self.assertTrue(
            document._call_contract_phase_closeout_status([slice_row])[
                "current"
            ]
        )

        missing_transcript = document.clone()
        missing_transcript.data["migration"][
            "authored_call_contract_fresh_closeout_v3"
        ]["scan_rows"][0].pop("exact_fact_transcript")
        self.assertFalse(
            missing_transcript._call_contract_phase_closeout_status(
                [slice_row]
            )["current"]
        )

        legacy = document.clone()
        legacy_closeout = legacy.data["migration"].pop(
            "authored_call_contract_fresh_closeout_v3"
        )
        legacy_closeout["schema"] = "recoil-call-contract-fresh-closeout-v2"
        legacy.data["migration"][
            "authored_call_contract_fresh_closeout_v2"
        ] = legacy_closeout
        self.assertFalse(
            legacy._call_contract_phase_closeout_status([slice_row])["current"]
        )

    def test_object_rows_accept_scheduler_prefix_without_pipeline_rederivation(self) -> None:
        document = one_authored_body_document()
        with patch.object(
            document,
            "pipeline",
            side_effect=AssertionError("object rows rederived scheduler pipeline"),
        ):
            rows = live_byte_verify._rows(
                document,
                "object",
                None,
                authored_order_prefix_end="0x401010",
            )
        self.assertEqual(["recoil:function:0x401000"], rows[0]["scope_ids"])

    def test_current_task_loads_only_the_progress_ledger_once(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            progress_path = root / "progress.sqlite3"
            ProgressSQLiteStore.create_from_mapping(
                progress_path,
                one_authored_body_document().data,
                cutover_pair_id="current-task-test",
            )
            progress_load = ProgressStore.load
            with patch.object(
                ProgressStore, "load", side_effect=progress_load, autospec=True
            ) as tracker_decode:
                payload = ProgressStore(progress_path).load().current_task()
            self.assertIn("task_id", payload)
            self.assertIn("stage", payload)
            self.assertEqual(1, tracker_decode.call_count)


if __name__ == "__main__":
    unittest.main()
