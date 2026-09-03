from __future__ import annotations

from pathlib import Path
import sys

import pytest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from _recoil.commands import call_contract_replay  # noqa: E402
from _recoil.commands.progress_cli import (  # noqa: E402
    _call_contract_replay_payload,
    _call_contract_replay_root,
    _validate_call_contract_result,
)
from _recoil.lib.call_contract_generations import current_generations  # noqa: E402
from _recoil.lib.progress import CALL_CONTRACT_CONTRACT_VERSION, ProgressError  # noqa: E402


def direct_fixture() -> tuple[dict[str, object], dict[str, object]]:
    symbol = "recoil:function:unit"
    address = "0x1000"
    fact = {**current_generations(), "symbol_id": symbol, "address": address, "calls": []}
    slice_row = {
        "id": "recoil:call-contract-slice:unit",
        "body_count": 1,
        "symbol_ids": [symbol],
        "addresses": [address],
        "target_ids": ["recoil:vc5-target:unit"],
        "physical_block_ids": ["recoil:block:unit"],
    }
    result = {
        "kind": "authored-call-contract-live-result",
        "contract_version": CALL_CONTRACT_CONTRACT_VERSION,
        "all_caller_divergences_collected": True,
        "slice_id": slice_row["id"],
        "body_count": 1,
        "symbol_ids": slice_row["symbol_ids"],
        "target_ids": slice_row["target_ids"],
        "physical_block_ids": slice_row["physical_block_ids"],
        "candidate_expected_truth": False,
        "source_edit_paths": [],
        "definition_source_paths": [],
        "definition_compile_results": [],
        "dependency_paths": [],
        "dependency_states_before": [],
        "dependency_states_after": [],
        "source_changed_during_validation": False,
        "passed": True,
        "first_divergence": None,
        "exact_fact_transcript": [{
            "symbol_id": symbol, "address": address, "expected_fact_row": fact,
        }],
        "provider_fact_transcript": [],
        "body_results": [{
            **current_generations(),
            "symbol_id": symbol,
            "address": address,
            "status": "passed",
            "comparison_passed": True,
            "divergence": None,
            "expected_fact_row": fact,
            "expected_contract": [],
            "candidate_contract": [],
        }],
    }
    return slice_row, result


def validate(slice_row: dict[str, object], result: dict[str, object]) -> dict[str, object]:
    return _validate_call_contract_result(
        result,
        expected_slice=slice_row,
        expected_source_write_paths=[],
        expected_definition_source_paths=[],
        expected_compiled_definition_sources=[],
        expected_dependency_paths=[],
    )


def test_direct_result_requires_exact_current_typed_evidence() -> None:
    slice_row, result = direct_fixture()
    accepted = validate(slice_row, result)
    assert accepted["passing_symbol_ids"] == slice_row["symbol_ids"]
    result["body_results"][0]["expected_fact_schema_version"] = 0  # type: ignore[index]
    with pytest.raises(ProgressError, match="incomplete"):
        validate(slice_row, result)


def test_direct_result_rejects_candidate_derived_expected_truth() -> None:
    slice_row, result = direct_fixture()
    result["candidate_expected_truth"] = True
    with pytest.raises(ProgressError, match="candidate-derived"):
        validate(slice_row, result)


def test_replay_cli_requires_exactly_one_mode() -> None:
    parser = call_contract_replay.build_parser()
    assert parser.parse_args(["--dry-run"]).dry_run is True
    with pytest.raises(SystemExit):
        parser.parse_args([])
    with pytest.raises(SystemExit):
        parser.parse_args(["--dry-run", "--apply"])


def test_replay_payload_preserves_serial_projection_and_closeout() -> None:
    payload = _call_contract_replay_payload(
        mode="apply",
        status="complete",
        initial_vector={"transaction_revision": 1},
        final_vector={"transaction_revision": 3},
        proof_session={"fresh_build": True, "reuse": False},
        slice_results=[{"passing_body_count": 2}, {"passing_body_count": 1}],
        closeout_command="python tools/recoil.py progress call-contract close-live ...",
    )
    assert payload["slice_count"] == 2
    assert payload["passing_body_count"] == 3
    assert payload["proof_session"] == {"fresh_build": True, "reuse": False}
    assert "close-live" in payload["closeout_command"]


def test_replay_root_must_be_a_fresh_governed_sibling() -> None:
    direct = ROOT / "build" / "live-validation" / "call-contract" / "unit-r1"
    result = _call_contract_replay_root(direct, create=False)
    assert result.name == "unit-r1-replay-001"
    with pytest.raises(ProgressError, match="escaped"):
        _call_contract_replay_root(ROOT / "build" / "elsewhere", create=False)
