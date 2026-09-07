from __future__ import annotations

import argparse
import json
from pathlib import Path
import subprocess
import sys
import threading
import time

import pytest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from _recoil.commands import call_contract_replay, progress_cli, vc5_build  # noqa: E402
from _recoil.commands.progress_cli import (  # noqa: E402
    CALL_CONTRACT_CLOSEOUT_DEFAULT_MAX_WORKERS,
    CALL_CONTRACT_CLOSEOUT_MAX_WORKERS,
    CALL_CONTRACT_FINAL_BUILD_MANIFEST,
    CALL_CONTRACT_LINKABILITY_SCHEMA,
    _call_contract_replay_payload,
    _call_contract_replay_root,
    _require_call_contract_closeout_vector_unchanged,
    _run_parallel_call_contract_closeout_scan,
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


def test_slice_evidence_batch_preserves_shared_and_external_references():
    from copy import deepcopy
    from _recoil.commands.progress_v2 import accept_live_call_contract_symbols
    from _recoil.lib.progress import CALL_CONTRACT_DIMENSION

    kind = "live-authored-call-contract-validation"
    for protected in ("none", "unselected-body", "owner", "other-dimension", "other-kind"):
        original = {
            "symbols": {
                name: {"evidence_ids": ["old"], "binary_state": {
                    CALL_CONTRACT_DIMENSION: {"evidence_ids": ["old"]},
                    "bytes": {"result": "unresolved"},
                }} for name in ("first", "second")
            },
            "evidence": {name: {"kind": kind, "scope_ids": []}
                         for name in ("old", "new-first", "new-second")},
            "source_owners": {"owner": {"tier": "C"}},
        }
        if protected == "unselected-body":
            original["symbols"]["untouched"] = deepcopy(original["symbols"]["first"])
        elif protected == "owner":
            original["source_owners"]["owner"]["evidence_ids"] = ["old"]
        elif protected == "other-dimension":
            original["symbols"]["first"]["binary_state"]["bytes"]["evidence_ids"] = ["old"]
        elif protected == "other-kind":
            original["evidence"]["old"]["kind"] = "live-authored-order-validation"
        batch, serial = deepcopy(original), deepcopy(original)
        bindings = {"first": "new-first", "second": "new-second"}
        assert accept_live_call_contract_symbols(batch, evidence_by_symbol=bindings) == list(bindings)
        for name, evidence_id in bindings.items():
            accept_live_call_contract_symbols(serial, evidence_by_symbol={name: evidence_id})
        assert batch == serial
        assert ("old" in batch["evidence"]) == (protected != "none")
        assert batch["source_owners"] == original["source_owners"]
        for name, evidence_id in bindings.items():
            state = batch["symbols"][name]["binary_state"]
            assert state[CALL_CONTRACT_DIMENSION]["evidence_ids"] == [evidence_id]
            assert state["bytes"] == original["symbols"][name]["binary_state"]["bytes"]


def test_slice_commit_batches_retirement_but_keeps_per_body_transcripts(monkeypatch, tmp_path):
    from copy import deepcopy
    from types import SimpleNamespace as Row
    from _recoil.commands import progress_v2

    names = ["recoil:function:first", "recoil:function:second"]
    data = {"revision": 1, "symbols": {name: {} for name in names}, "evidence": {}}
    document = Row(data=data, revision=1,
        call_contract_body_currentness=lambda _: {"current": False})
    result = {"passed": True, "passing_symbol_ids": names, "first_divergence": None,
        "body_results": [{"symbol_id": name, "address": hex(0x1000 + index * 16),
            "target_id": "unit", "expected_contract": [], "candidate_contract": []}
            for index, name in enumerate(names)],
        "exact_fact_transcript": [{"symbol_id": name, "unit": index}
                                  for index, name in enumerate(names)]}
    calls, proposals = [], []
    def accept(value, *, evidence_by_symbol):
        calls.append(dict(evidence_by_symbol))
        return progress_v2.accept_live_call_contract_symbols(value, evidence_by_symbol=evidence_by_symbol)
    def plan(current, transform):
        proposed = deepcopy(current.data)
        transform(proposed)
        proposals.append(proposed)
        return proposed, {}, {}
    monkeypatch.setattr(progress_cli, "accept_live_call_contract_symbols", accept)
    monkeypatch.setattr(progress_cli, "_require_current_call_contract_action", lambda *a, **k: None)
    monkeypatch.setattr(progress_cli, "_call_contract_scoped_patch_plan", plan)
    monkeypatch.setattr(progress_cli, "ProgressSQLiteStore", lambda _: Row(
        persist_scoped_changes=lambda **k: Row(applied=False)))
    monkeypatch.setattr(progress_cli, "_commit_payload", lambda commit, details: details)
    status, details, _ = progress_cli._commit_validated_call_contract_slice(
        args=Row(progress=tmp_path / "unused.sqlite3", apply=False), document=document,
        slice_row={"id": "unit-slice"}, result=result, build_root=tmp_path,
        expected_domains={"semantic": 1, "evidence_generation": 1})
    assert status == 0 and len(calls) == 1
    assert list(calls[0]) == names and len(set(calls[0].values())) == 2
    assert details["evidence_ids"] == calls[0]
    for index, name in enumerate(names):
        evidence = proposals[0]["evidence"][calls[0][name]]
        assert evidence["scope_ids"] == [name]
        assert evidence["provenance"]["exact_fact_transcript"] == [
            {"symbol_id": name, "unit": index}]
    assert data["evidence"] == {}  # The original document remains untouched.


def validate(slice_row: dict[str, object], result: dict[str, object]) -> dict[str, object]:
    return _validate_call_contract_result(
        result,
        expected_slice=slice_row,
        expected_source_write_paths=[],
        expected_definition_source_paths=[],
        expected_compiled_definition_sources=[],
        expected_dependency_paths=[],
    )


def test_early_rejection_projects_as_blocked_without_fabricating_a_transcript() -> None:
    from types import SimpleNamespace
    from _recoil.call_contract.reporting import _call_contract_body_results

    slice_row, result = direct_fixture()
    divergence = {
        "symbol_id": slice_row["symbol_ids"][0], "address": "0x1000",
        "kind": "verifier-blocked", "side": "candidate", "ordinal": 0,
        "reason": "known-authored-direct-target-before-projection",
        "expected": {"target_identity": "symbol:required"},
        "candidate": {"target_identity": "symbol:different"},
    }
    result.update(passed=False, first_divergence=divergence, exact_fact_transcript=[])
    result["body_results"] = _call_contract_body_results(
        document=None, slice_row=slice_row,
        candidate_session=SimpleNamespace(target_id=lambda address: "target"),
        expected_by_symbol={}, candidate_by_symbol={},
        caller_divergences=[divergence], evaluated_symbol_ids=(),
    )
    assert result["body_results"][0]["status"] == "blocked"
    assert result["body_results"][0]["expected_fact_row"] is None
    assert validate(slice_row, result)["passing_symbol_ids"] == []
    result["body_results"][0]["status"] = "divergent"
    with pytest.raises(ProgressError, match="lacks an exact fact transcript"):
        validate(slice_row, result)


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


def closeout_scan_jobs(tmp_path: Path, count: int) -> list[dict[str, object]]:
    return [
        {
            "index": index,
            "slice_row": {
                "id": f"recoil:call-contract-slice:unit-{index}",
                "body_count": 1,
            },
            "closure": object(),
            "slice_root": tmp_path / f"slice-{index + 1:02d}",
        }
        for index in range(count)
    ]


def closeout_scan_outcome(
    *,
    index: int,
    slice_row: dict[str, object],
    slice_root: Path,
    status: str = "passed",
) -> dict[str, object]:
    slice_id = str(slice_row["id"])
    outcome: dict[str, object] = {
        "index": index,
        "slice_id": slice_id,
        "status": status,
        "body_count": 1,
        "build_root": str(slice_root),
        "elapsed_ms": float(index + 1),
        "first_divergence": None,
        "detail": "",
    }
    if status == "passed":
        outcome["scan_row"] = {
            "slice_id": slice_id,
            "body_count": 1,
            "build_root": str(slice_root),
            "exact_fact_transcript": [],
        }
    else:
        outcome["first_divergence"] = {
            "kind": "unit-divergence",
            "slice_id": slice_id,
        }
    return outcome


def test_closeout_parser_defaults_to_eight_and_bounds_configuration() -> None:
    prefix = [
        "call-contract",
        "close-live",
        "--build-root",
        "build/live-validation/unit-closeout",
        "--expected-semantic-revision",
        "1",
        "--expected-evidence-generation-revision",
        "1",
        "--dry-run",
    ]
    args = progress_cli._parser().parse_args(prefix)
    assert args.max_workers == CALL_CONTRACT_CLOSEOUT_DEFAULT_MAX_WORKERS == 8
    configured = progress_cli._parser().parse_args(prefix + ["--max-workers", "1"])
    assert configured.max_workers == 1
    for invalid in ("0", str(CALL_CONTRACT_CLOSEOUT_MAX_WORKERS + 1)):
        with pytest.raises(SystemExit):
            progress_cli._parser().parse_args(prefix + ["--max-workers", invalid])


def test_parallel_closeout_scan_is_bounded_and_returns_retail_order(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    lock = threading.Lock()
    pair_barrier = threading.Barrier(2)
    active = 0
    peak = 0
    completion_order: list[int] = []

    def fake_slice(**kwargs: object) -> dict[str, object]:
        nonlocal active, peak
        index = int(kwargs["index"])
        with lock:
            active += 1
            peak = max(peak, active)
        pair_barrier.wait(timeout=2.0)
        time.sleep(0.01 if index % 2 else 0.03)
        with lock:
            active -= 1
            completion_order.append(index)
        return closeout_scan_outcome(
            index=index,
            slice_row=kwargs["slice_row"],  # type: ignore[arg-type]
            slice_root=kwargs["slice_root"],  # type: ignore[arg-type]
        )

    monkeypatch.setattr(
        progress_cli,
        "_run_call_contract_closeout_slice",
        fake_slice,
    )
    rows, execution = _run_parallel_call_contract_closeout_scan(
        progress_path=tmp_path / "progress.sqlite3",
        jobs=closeout_scan_jobs(tmp_path, 4),  # type: ignore[arg-type]
        max_workers=2,
    )
    assert peak == 2
    assert completion_order != list(range(4))
    assert [row["slice_id"] for row in rows] == [
        f"recoil:call-contract-slice:unit-{index}" for index in range(4)
    ]
    assert execution["effective_max_workers"] == 2
    assert execution["completed_slice_count"] == 4


def test_parallel_closeout_scan_supports_explicit_serial_fallback(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    lock = threading.Lock()
    active = 0
    peak = 0

    def fake_slice(**kwargs: object) -> dict[str, object]:
        nonlocal active, peak
        with lock:
            active += 1
            peak = max(peak, active)
        time.sleep(0.005)
        with lock:
            active -= 1
        return closeout_scan_outcome(
            index=int(kwargs["index"]),
            slice_row=kwargs["slice_row"],  # type: ignore[arg-type]
            slice_root=kwargs["slice_root"],  # type: ignore[arg-type]
        )

    monkeypatch.setattr(
        progress_cli,
        "_run_call_contract_closeout_slice",
        fake_slice,
    )
    _rows, execution = _run_parallel_call_contract_closeout_scan(
        progress_path=tmp_path / "progress.sqlite3",
        jobs=closeout_scan_jobs(tmp_path, 3),  # type: ignore[arg-type]
        max_workers=1,
    )
    assert peak == 1
    assert execution["effective_max_workers"] == 1


def test_parallel_closeout_reports_earliest_retail_failure(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    def fake_slice(**kwargs: object) -> dict[str, object]:
        index = int(kwargs["index"])
        if index == 1:
            time.sleep(0.03)
        status = "diverged" if index in {1, 3} else "passed"
        return closeout_scan_outcome(
            index=index,
            slice_row=kwargs["slice_row"],  # type: ignore[arg-type]
            slice_root=kwargs["slice_root"],  # type: ignore[arg-type]
            status=status,
        )

    monkeypatch.setattr(
        progress_cli,
        "_run_call_contract_closeout_slice",
        fake_slice,
    )
    with pytest.raises(
        ProgressError,
        match="recoil:call-contract-slice:unit-1",
    ):
        _run_parallel_call_contract_closeout_scan(
            progress_path=tmp_path / "progress.sqlite3",
            jobs=closeout_scan_jobs(tmp_path, 5),  # type: ignore[arg-type]
            max_workers=4,
        )


class UnitRevisionVector:
    def __init__(self, revision: int = 7) -> None:
        self.revision = revision

    def to_dict(self) -> dict[str, int]:
        return {
            "transaction_revision": self.revision,
            "semantic_revision": self.revision,
            "evidence_generation_revision": self.revision,
        }


class UnitCloseoutStore:
    def __init__(self) -> None:
        self.vector = UnitRevisionVector()

    def materialize_with_revision_vector(
        self,
    ) -> tuple[dict[str, object], UnitRevisionVector]:
        return {}, self.vector

    def read_revision_vector(self) -> UnitRevisionVector:
        return self.vector


class UnitCloseoutDocument:
    slices = [
        {
            "id": "recoil:call-contract-slice:unit",
            "body_count": 1,
            "symbol_ids": ["recoil:function:unit"],
        }
    ]

    def authored_call_contract_slices(self) -> list[dict[str, object]]:
        return self.slices

    def call_contract_body_currentness(self, _symbol_id: str) -> dict[str, bool]:
        return {"current": True}

    def pipeline(
        self,
        _binary: str,
        *,
        resolve_order_target: bool,
    ) -> dict[str, object]:
        assert resolve_order_target is False
        return {
            "phase": "authored-call-contract",
            "authored_call_contract_slice_id": "",
            "authored_call_contract_closeout": {"current": False},
        }


class UnitCloseoutCommit:
    def to_dict(self) -> dict[str, object]:
        return {"applied": True, "revision": 8}


def prepare_closeout_orchestration(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> tuple[argparse.Namespace, UnitCloseoutStore]:
    progress_path = tmp_path / "progress.sqlite3"
    progress_path.write_bytes(b"fixture")
    build_root = tmp_path / "closeout"
    store = UnitCloseoutStore()
    document = UnitCloseoutDocument()
    closure = type(
        "UnitClosure",
        (),
        {
            "dependency_paths": (),
            "source_edit_paths": (),
            "definition_source_paths": (),
        },
    )()
    monkeypatch.setattr(
        progress_cli,
        "ProgressSQLiteStore",
        lambda _path: store,
    )
    monkeypatch.setattr(
        progress_cli.ProgressDocument,
        "_from_owned_data",
        lambda _data, *, path: document,
    )
    monkeypatch.setattr(
        progress_cli,
        "_absolute_fresh_build_root",
        lambda _path: build_root,
    )
    monkeypatch.setattr(
        progress_cli,
        "load_repository_path_inventory",
        lambda _root: object(),
    )
    monkeypatch.setattr(
        progress_cli,
        "_call_contract_verifier_component_state",
        lambda _inventory: {},
    )
    monkeypatch.setattr(
        progress_cli,
        "_resolve_phase_all_authored_bodies",
        lambda _document: {"id": "recoil:call-contract-phase:unit"},
    )
    monkeypatch.setattr(
        progress_cli,
        "call_contract_source_closure",
        lambda *_args, **_kwargs: closure,
    )
    monkeypatch.setattr(
        progress_cli,
        "file_dependency_states",
        lambda *_args, **_kwargs: [],
    )
    monkeypatch.setattr(
        progress_cli,
        "_require_call_contract_proof_inputs_unchanged",
        lambda *_args, **_kwargs: None,
    )
    return (
        argparse.Namespace(
            progress=progress_path,
            build_root=build_root,
            expected_revision=None,
            expected_semantic_revision=7,
            expected_evidence_generation_revision=7,
            max_workers=8,
            dry_run=False,
            apply=True,
            json=True,
        ),
        store,
    )


def test_closeout_scan_failure_prevents_link_and_commit(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    args, _store = prepare_closeout_orchestration(tmp_path, monkeypatch)
    calls = {"link": 0, "commit": 0}

    def fail_scan(**_kwargs: object) -> tuple[list[object], dict[str, object]]:
        raise ProgressError("unit scan failure")

    def unexpected_link(**_kwargs: object) -> dict[str, object]:
        calls["link"] += 1
        return {}

    def unexpected_commit(**_kwargs: object) -> UnitCloseoutCommit:
        calls["commit"] += 1
        return UnitCloseoutCommit()

    monkeypatch.setattr(
        progress_cli,
        "_run_parallel_call_contract_closeout_scan",
        fail_scan,
    )
    monkeypatch.setattr(
        progress_cli,
        "_run_call_contract_linkability_gate",
        unexpected_link,
    )
    monkeypatch.setattr(
        progress_cli,
        "_call_contract_scoped_patch_commit",
        unexpected_commit,
    )
    with pytest.raises(ProgressError, match="unit scan failure"):
        progress_cli.close_live_call_contract(args)
    assert calls == {"link": 0, "commit": 0}


def test_complete_parallel_closeout_links_and_commits_exactly_once(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    args, _store = prepare_closeout_orchestration(tmp_path, monkeypatch)
    calls = {"scan": 0, "link": 0, "commit": 0}

    def pass_scan(**kwargs: object) -> tuple[list[dict[str, object]], dict[str, object]]:
        calls["scan"] += 1
        jobs = kwargs["jobs"]
        assert isinstance(jobs, list) and len(jobs) == 1
        return (
            [
                {
                    "slice_id": "recoil:call-contract-slice:unit",
                    "body_count": 1,
                    "build_root": str(tmp_path / "closeout" / "slice-01"),
                    "exact_fact_transcript": [],
                }
            ],
            {
                "mode": "parallel-isolated-slices",
                "requested_max_workers": 8,
                "effective_max_workers": 1,
                "completed_slice_count": 1,
                "scan_elapsed_ms": 1.0,
                "slice_timings": [],
            },
        )

    def pass_link(**_kwargs: object) -> dict[str, object]:
        calls["link"] += 1
        return {"whole_program_linked": True}

    def pass_commit(**_kwargs: object) -> UnitCloseoutCommit:
        calls["commit"] += 1
        return UnitCloseoutCommit()

    monkeypatch.setattr(
        progress_cli,
        "_run_parallel_call_contract_closeout_scan",
        pass_scan,
    )
    monkeypatch.setattr(
        progress_cli,
        "_run_call_contract_linkability_gate",
        pass_link,
    )
    monkeypatch.setattr(
        progress_cli,
        "_call_contract_scoped_patch_commit",
        pass_commit,
    )
    result = progress_cli.close_live_call_contract(args)
    assert calls == {"scan": 1, "link": 1, "commit": 1}
    assert result["scan_execution"]["requested_max_workers"] == 8
    assert result["whole_program_linkability"]["whole_program_linked"] is True


def test_closeout_vector_drift_fails_closed() -> None:
    store = UnitCloseoutStore()
    expected = store.vector.to_dict()
    _require_call_contract_closeout_vector_unchanged(store, expected)  # type: ignore[arg-type]
    store.vector = UnitRevisionVector(8)
    with pytest.raises(ProgressError, match="tracker changed"):
        _require_call_contract_closeout_vector_unchanged(store, expected)  # type: ignore[arg-type]
