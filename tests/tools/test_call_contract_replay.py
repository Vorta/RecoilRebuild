from __future__ import annotations

import json
from pathlib import Path
import subprocess
import sys

import pytest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from _recoil.commands import call_contract_replay, progress_cli, vc5_build  # noqa: E402
from _recoil.commands.progress_cli import (  # noqa: E402
    CALL_CONTRACT_FINAL_BUILD_MANIFEST,
    CALL_CONTRACT_LINKABILITY_SCHEMA,
    _call_contract_replay_payload,
    _call_contract_replay_root,
    _validate_call_contract_linkability_summary,
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


def linkability_summary(build_root: Path) -> dict[str, object]:
    for name in ("Recoil.exe", "Recoil.map", "Recoil.res"):
        (build_root / name).write_bytes(b"fresh")
    return {
        "dry_run": False,
        "kind": "final-build-diagnostic",
        "diagnostic_kind": "whole-program-linkability",
        "binary": "recoil",
        "success": True,
        "validation_mode": "live",
        "fresh_build": True,
        "reuse": False,
        "candidate_expected_truth": False,
        "linked_order_evaluation_suppressed": True,
        "playtest_deployment_suppressed": True,
        "required_order_targets": [],
        "effective_order_targets": [],
        "diagnostic_only": True,
        "final_image_validation": "not-run",
        "compiler_profile": "VC5SP3",
        "canonical_mfc_include_trace": {"ok": True},
        "compile_succeeded": True,
        "coff_alias_sources_succeeded": True,
        "resource_succeeded": True,
        "link_succeeded": True,
        "candidate_available": True,
        "authored_byte_eligible": False,
        "linked_order_passed": False,
        "accepts_linked_order": False,
        "accepts_bytes": False,
        "accepts_final_image": False,
        "build_root": str(build_root.resolve()),
        "candidate_path": str((build_root / "Recoil.exe").resolve()),
        "map_path": str((build_root / "Recoil.map").resolve()),
        "resource_path": str((build_root / "Recoil.res").resolve()),
        "config_path": str(CALL_CONTRACT_FINAL_BUILD_MANIFEST.resolve()),
        "playtest_deploy": {
            "attempted": False,
            "updated": False,
            "destination": str((ROOT / "playground" / "Recoil-rebuild.exe").resolve()),
            "error": None,
            "suppression_reason": "whole-program-linkability",
        },
    }


def test_closeout_linkability_summary_proves_only_a_fresh_no_deploy_link(
    tmp_path: Path,
) -> None:
    summary = linkability_summary(tmp_path)
    accepted = _validate_call_contract_linkability_summary(
        summary,
        build_root=tmp_path,
    )
    assert accepted["schema"] == CALL_CONTRACT_LINKABILITY_SCHEMA
    assert accepted["whole_program_linked"] is True
    assert accepted["playtest_deployment_suppressed"] is True
    assert accepted["accepts_linked_order"] is False
    assert accepted["accepts_bytes"] is False
    assert accepted["accepts_final_image"] is False


@pytest.mark.parametrize(
    ("field", "value"),
    (
        ("link_succeeded", False),
        ("authored_byte_eligible", True),
        ("accepts_linked_order", True),
    ),
)
def test_closeout_linkability_summary_rejects_failure_or_scope_expansion(
    tmp_path: Path,
    field: str,
    value: object,
) -> None:
    summary = linkability_summary(tmp_path)
    summary[field] = value
    with pytest.raises(ProgressError, match="acceptance boundary"):
        _validate_call_contract_linkability_summary(summary, build_root=tmp_path)


def test_closeout_linkability_summary_rejects_playtest_deployment(
    tmp_path: Path,
) -> None:
    summary = linkability_summary(tmp_path)
    summary["playtest_deploy"]["attempted"] = True  # type: ignore[index]
    with pytest.raises(ProgressError, match="deployment suppression"):
        _validate_call_contract_linkability_summary(summary, build_root=tmp_path)


def test_final_build_exposes_explicit_linkability_only_mode() -> None:
    args = vc5_build.build_parser().parse_args(
        [
            "--build-dir",
            "build/live-validation/unit-linkability",
            "--clean",
            "--linkability-only",
        ]
    )
    assert args.linkability_only is True


def test_closeout_runner_requests_one_isolated_linkability_build(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    closeout_root = tmp_path / "closeout"
    closeout_root.mkdir()
    progress_path = tmp_path / "progress.sqlite3"
    progress_path.write_bytes(b"fixture")
    observed: list[list[str]] = []

    def fake_run(command: list[str], **_kwargs: object) -> subprocess.CompletedProcess[str]:
        observed.append(command)
        linkability_root = closeout_root / "whole-program-linkability"
        linkability_root.mkdir()
        summary = linkability_summary(linkability_root)
        (linkability_root / "summary.json").write_text(
            json.dumps(summary),
            encoding="utf-8",
        )
        return subprocess.CompletedProcess(command, 0, stdout="pass", stderr="")

    monkeypatch.setattr(progress_cli.subprocess, "run", fake_run)
    result = progress_cli._run_call_contract_linkability_gate(
        build_root=closeout_root,
        progress_path=progress_path,
    )
    assert len(observed) == 1
    assert "--linkability-only" in observed[0]
    assert "--clean" in observed[0]
    assert result["whole_program_linked"] is True
    assert result["playtest_deployment_suppressed"] is True
