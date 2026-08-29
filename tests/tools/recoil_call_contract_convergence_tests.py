from __future__ import annotations

from pathlib import Path
from types import SimpleNamespace
import sys
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.commands import call_contract_convergence as convergence
from _recoil.lib.call_contract_generations import (
    CALL_CONTRACT_VERIFIER_GENERATION,
    EXPECTED_FACT_SCHEMA_VERSION,
    NORMALIZER_REGISTRY_GENERATION,
    evidence_generations_current,
)
from _recoil.lib.progress import ProgressError


class _Document:
    revision = 47

    @staticmethod
    def authored_call_contract_slices() -> list[dict[str, object]]:
        return [
            {
                "id": "recoil:call-contract-slice:0001",
                "symbol_ids": [
                    "recoil:function:0x401000",
                    "recoil:function:0x401010",
                ],
            },
            {
                "id": "recoil:call-contract-slice:0002",
                "symbol_ids": ["recoil:function:0x401020"],
            },
        ]


class CallContractConvergenceTests(unittest.TestCase):
    def test_census_is_derived_directly_from_live_slices(self) -> None:
        census = convergence.derive_convergence_census(_Document())

        self.assertEqual(3, convergence.CONVERGENCE_CONTRACT_VERSION)
        self.assertEqual(3, census["contract_version"])
        self.assertEqual(3, census["body_count"])
        self.assertEqual(
            [
                "recoil:function:0x401000",
                "recoil:function:0x401010",
                "recoil:function:0x401020",
            ],
            census["symbol_ids"],
        )
        self.assertEqual(
            [
                "recoil:call-contract-slice:0001",
                "recoil:call-contract-slice:0002",
            ],
            census["slice_ids"],
        )

    def test_compact_census_contains_only_direct_coordinates(self) -> None:
        compact = convergence.compact_convergence_census(
            convergence.derive_convergence_census(_Document())
        )

        self.assertEqual(
            {
                "contract_version": 3,
                "body_count": 3,
                "slice_ids": [
                    "recoil:call-contract-slice:0001",
                    "recoil:call-contract-slice:0002",
                ],
            },
            compact,
        )

    def test_state_is_contained_until_fresh_closeout_producer_exists(self) -> None:
        state = convergence.convergence_generation_state(_Document())

        self.assertFalse(state["current"])
        self.assertEqual("contained-disabled", state["status"])
        self.assertIsNone(state["generation"])
        self.assertEqual(
            CALL_CONTRACT_VERIFIER_GENERATION,
            state["call_contract_verifier_generation"],
        )
        self.assertEqual(
            NORMALIZER_REGISTRY_GENERATION,
            state["normalizer_registry_generation"],
        )
        self.assertEqual(
            EXPECTED_FACT_SCHEMA_VERSION,
            state["expected_fact_schema_version"],
        )
        self.assertIn("fresh complete no-reuse direct comparison", state["reason"])

    def test_scheduler_exposes_no_packetless_action(self) -> None:
        self.assertEqual("contained-disabled", convergence.convergence_scheduler_mode())
        self.assertEqual("", convergence.convergence_next_action())

    def test_reviewed_verifier_identity_is_integer_generations(self) -> None:
        identity = convergence.current_call_contract_verifier_semantic_identity()

        self.assertEqual("call-contract-reviewed-integer-generations", identity["kind"])
        self.assertEqual(10, identity["call_contract_verifier_generation"])
        self.assertEqual(10, identity["normalizer_registry_generation"])
        self.assertEqual(10, identity["expected_fact_schema_version"])

    def test_semantic_projection_uses_revision_and_integer_generations(self) -> None:
        projection = convergence._normalized_semantic_projection(_Document())

        self.assertEqual(
            {
                "revision": 47,
                "call_contract_verifier_generation": (
                    CALL_CONTRACT_VERIFIER_GENERATION
                ),
                "normalizer_registry_generation": NORMALIZER_REGISTRY_GENERATION,
                "expected_fact_schema_version": EXPECTED_FACT_SCHEMA_VERSION,
            },
            projection,
        )

    def test_historical_generation5_evidence_is_explicitly_noncurrent(self) -> None:
        self.assertFalse(
            evidence_generations_current(
                {
                    "call_contract_verifier_generation": 5,
                    "normalizer_registry_generation": 5,
                    "expected_fact_schema_version": 5,
                }
            )
        )

    def test_generation10_is_current_and_generation9_or_mixed_fail_closed(self) -> None:
        current = {
            "call_contract_verifier_generation": 10,
            "normalizer_registry_generation": 10,
            "expected_fact_schema_version": 10,
        }
        self.assertTrue(evidence_generations_current(current))
        generation9 = {
            "call_contract_verifier_generation": 9,
            "normalizer_registry_generation": 9,
            "expected_fact_schema_version": 9,
        }
        self.assertFalse(evidence_generations_current(generation9))
        generation7 = {
            "call_contract_verifier_generation": 7,
            "normalizer_registry_generation": 7,
            "expected_fact_schema_version": 7,
        }
        self.assertFalse(evidence_generations_current(generation7))
        generation6 = {
            "call_contract_verifier_generation": 6,
            "normalizer_registry_generation": 6,
            "expected_fact_schema_version": 6,
        }
        self.assertFalse(evidence_generations_current(generation6))
        missing = dict(current)
        missing.pop("expected_fact_schema_version")
        self.assertFalse(evidence_generations_current(missing))
        mixed = dict(current, normalizer_registry_generation=7)
        self.assertFalse(evidence_generations_current(mixed))

    def test_work_ledger_mutation_never_carries_prior_closeout(self) -> None:
        self.assertFalse(
            convergence.carry_current_generation_across_work_ledger_mutation()
        )

    def test_prepare_fails_before_any_closeout_operation(self) -> None:
        with self.assertRaisesRegex(
            ProgressError, "separately approved active-packet"
        ):
            convergence.prepare_live_convergence(
                SimpleNamespace(build_root="build/not-allocated")
            )

    def test_dependent_owner_route_is_not_launchable(self) -> None:
        result = convergence.dependent_owner_repair_launchability(_Document())

        self.assertFalse(result["launchable"])
        self.assertIn("fresh complete no-reuse direct comparison", result["reason"])

    def test_retail_fact_packet_route_is_empty_and_invalid(self) -> None:
        self.assertEqual([], convergence.retail_fact_packet_scopes(_Document()))
        self.assertFalse(convergence._valid_retail_fact_scope({}))

    def test_prospective_profile_route_has_no_side_effect(self) -> None:
        self.assertIsNone(
            convergence.prospective_wol_profile_convergence_route(_Document())
        )

    def test_closeout_expected_truth_is_direct_retail_comparison(self) -> None:
        self.assertEqual(
            "fresh-complete-direct-retail-comparison",
            convergence.CONVERGENCE_EXPECTED_TRUTH,
        )
        self.assertEqual(
            "authored_call_contract_direct_closeout_v3",
            convergence.CONVERGENCE_MIGRATION_KEY,
        )
        self.assertEqual((), convergence.CONVERGENCE_VERIFIER_SEMANTIC_PATHS)


if __name__ == "__main__":
    unittest.main()
