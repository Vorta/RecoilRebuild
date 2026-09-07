from __future__ import annotations

import json
from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]
TOOLS = ROOT / "tools"
sys.path.insert(0, str(TOOLS))

import recoil  # noqa: E402


RETIRED_ROUTES = {
    "audit pipeline-contracts",
    "progress status",
    "progress owner find",
    "progress owner relationships",
    "audit call-contract-readiness",
    "audit zinterp",
    "binja data-overlap",
    "verify authored-order scaffold",
    "verify authored-order sweep",
    "verify vc5-abi-equivalence",
    "verify zui-inline-context",
}


def test_registry_is_unique_reachable_and_free_of_retired_routes() -> None:
    names = [item.name for item in recoil.COMMAND_SPECS]
    assert len(names) == len(set(names))
    assert not RETIRED_ROUTES.intersection(names)
    for item in recoil.COMMAND_SPECS:
        assert (TOOLS / "_recoil" / "commands" / f"{item.module}.py").is_file()


def test_machine_readable_command_inventory_matches_registry() -> None:
    completed = subprocess.run(
        [sys.executable, str(TOOLS / "recoil.py"), "commands", "--json"],
        cwd=ROOT,
        capture_output=True,
        text=True,
        check=False,
    )
    assert completed.returncode == 0, completed.stderr
    payload = json.loads(completed.stdout)
    assert {row["command"] for row in payload} == {
        item.name for item in recoil.COMMAND_SPECS
    }


def test_dispatch_preserves_prepend_arguments() -> None:
    item = recoil.COMMANDS[("verify", "vc5-order")]
    command = recoil.build_command(item, ["unit", "--build-root", "scratch"])
    assert command[-4:] == ["--order-only", "unit", "--build-root", "scratch"]


def test_vc5_smoke_is_on_the_retained_verifier_route() -> None:
    item = recoil.COMMANDS[("verify", "vc5")]
    assert "--smoke" in " ".join(item.examples)
    assert ("audit", "source-policy") in recoil.COMMANDS


def test_source_path_relocation_is_one_public_global_cas_route() -> None:
    item = recoil.COMMANDS[("progress", "source-path", "relocate")]
    assert item.prepend_args == ("source-path", "relocate")
    assert item.required_revision_domains == ("global",)
    assert item.mutation_scope == "source-path"


def test_completion_routes_report_scoped_missing_acceptance_operations() -> None:
    from _recoil.commands.pipeline_reachability_audit import audit_completion_routes
    from _recoil.lib.progress import OWNER_GATES, STORAGE_DIMENSIONS, TIERS

    result = audit_completion_routes()
    missing = {row["id"] for row in result["obligations"] if not row["producers"]}
    assert missing == (
        {f"authored-storage/{dimension}/accept" for dimension in STORAGE_DIMENSIONS}
        | {f"existing-authored-owner/{gate}/accept" for gate in OWNER_GATES}
        | {f"existing-authored-owner-tier/{tier}/promote" for tier in TIERS[1:]}
    )
    assert result["completion_routes_complete"] is False
    assert all(row["operational"] for row in result["routes"])
    calls = next(row for row in result["obligations"] if row["dimension"] == "call_contract")
    assert set(calls["producers"]) == {
        "progress advance-live-call-contract", "progress call-contract replay-live"}
    assert not recoil.COMMANDS[("progress", "storage", "register-authored-data")].acceptance_effects
    assert not recoil.COMMANDS[("progress", "owner", "replace-batch")].acceptance_effects
    assert not recoil.COMMANDS[("progress", "owner", "downgrade")].acceptance_effects


def test_reachability_rejects_a_registered_but_unimplemented_parser_route(monkeypatch) -> None:
    from dataclasses import replace
    from _recoil.commands.pipeline_reachability_audit import audit_completion_routes

    original = recoil.COMMANDS[("progress", "advance-live-authored-byte")]
    broken = replace(original, prepend_args=("unimplemented-acceptance",))
    monkeypatch.setitem(recoil.COMMANDS, broken.path, broken)
    result = audit_completion_routes([broken])
    assert not result["routes"][0]["operational"]
    assert "backend parser rejected" in result["routes"][0]["findings"][0]
    assert not any(row["producers"] for row in result["obligations"])


def test_acceptance_effects_match_real_order_and_byte_writes_without_storage_promotion() -> None:
    from _recoil.commands.progress_v2 import accept_live_order_block, accept_live_byte_groups
    from _recoil.commands.final_image_coverage import _storage_accepted
    from _recoil.lib.progress import STORAGE_DIMENSIONS

    pending = {"result": "pending", "disposition": "claim", "freshness": "current"}
    storage = {"applicability": {key: True for key in STORAGE_DIMENSIONS},
               "verification": {key: dict(pending) for key in STORAGE_DIMENSIONS}}
    data = {"symbols": {"function": {}}, "physical_blocks": {"block": {}},
            "storage_contributions": {"data": storage}, "owners": {"owner": {"tier": "X"}}}
    for phase, group, subject in (("authored-function-order", "authored", "authored-block"),
                                  ("full-function-order", "full", "full-block")):
        accept_live_order_block(data, block_id="block", phase=phase, evidence_id="proof", facts={})
        written = data["physical_blocks"]["block"]["order"][group]
        declared = recoil.COMMANDS[("progress", "advance-live-order")].acceptance_effects
        assert set(written) == {effect.dimension for effect in declared if effect.subject == subject}
    for mode, route in (("authored", "advance-live-authored-byte"), ("linked", "advance-live-linked-byte")):
        data["symbols"]["function"]["binary_state"] = {}
        accept_live_byte_groups(data, mode=mode, groups=[["function"]], evidence_id="proof", facts={})
        declared = recoil.COMMANDS[("progress", route)].acceptance_effects
        assert set(data["symbols"]["function"]["binary_state"]) == {effect.dimension for effect in declared}
    assert not _storage_accepted(storage)
    assert all(row == pending for row in storage["verification"].values())
    assert data["owners"] == {"owner": {"tier": "X"}}


def test_doctor_runs_combined_completion_audit_once_and_last() -> None:
    from _recoil.commands.doctor import _steps
    steps = _steps()
    assert steps[-1][0] == "serial pipeline reachability"
    assert sum("pipeline-reachability" in command for _, command in steps) == 1
    assert not any("pipeline-contracts" in command for _, command in steps)


def test_task_routes_cover_six_stages_and_closeout_and_reject_stale_or_saved_inputs():
    from _recoil.commands.pipeline_reachability_audit import audit_task_routes, _probe_arguments
    for stage, name in (
        ("authored-function-order", "progress advance-live-order"),
        ("authored-call-contract", "progress advance-live-call-contract"),
        ("authored-call-contract", "progress call-contract close-live"),
        ("authored-byte-match", "progress advance-live-authored-byte"),
        ("full-function-order", "progress advance-live-order"),
        ("linked-byte-match", "progress advance-live-linked-byte"),
        ("final-validation", "verify final-image"),
    ):
        spec = recoil.COMMANDS[tuple(name.split())]
        task = {"schema": "recoil-current-task-v2", "binary": "recoil", "stage": stage,
                "task_id": "proof-task", "state": "ready", "cursor": "0x401000", "scope": {},
                "objective": "proof", "check_command": None, "blocker": None,
                "stage_runner_command": ("python tools/recoil.py progress call-contract replay-live --apply --json"
                                         if stage == "authored-call-contract" else None),
                "acceptance_command": "python tools/recoil.py " + name + " " + " ".join(_probe_arguments(spec)),
                "revision_vector": {key: 0 for key in ("transaction_revision", "semantic_revision", "evidence_generation_revision")}}
        assert not audit_task_routes(task)
        if stage == "final-validation":
            task["acceptance_command"] += " --candidate stale.exe --map stale.map"
        else:
            task["revision_vector"] = {key: 1 for key in task["revision_vector"]}
        assert audit_task_routes(task)


def test_completion_routes_reject_disconnected_backend_main(monkeypatch):
    from _recoil.commands.pipeline_reachability_audit import audit_completion_routes
    from _recoil.commands import progress_cli
    spec = recoil.COMMANDS[("progress", "advance-live-authored-byte")]
    # The parser and evidence handler still exist; the actual entry point
    # no longer dispatches to them.
    monkeypatch.setattr(progress_cli, "main", lambda argv=None: 0)
    result = audit_completion_routes([spec])
    assert not result["routes"][0]["operational"]
    assert any("CLI dispatch" in finding for finding in result["routes"][0]["findings"])


def test_selected_dispatch_inspection_respects_arguments_and_early_returns():
    import argparse, ast
    from _recoil.commands.pipeline_reachability_audit import _selected_dispatch_calls
    tree = ast.parse("if args.command == 'accept':\n    return 0\nelse:\n    verify()\naccept()")
    calls = _selected_dispatch_calls(tree.body, argparse.Namespace(command="accept"))
    assert next(calls) is None
    calls = _selected_dispatch_calls(tree.body, argparse.Namespace(command="check"))
    assert next(calls) == "verify"
    assert next(calls) == "accept"


def test_process_diagnostic_map_keeps_local_aliases_but_rejects_ambiguous_watches(tmp_path) -> None:
    import pytest
    from _recoil.commands.gameplay_diagnose import read_map, resolve_watch

    path = tmp_path / "synthetic.map"
    path.write_text(
        " 0001:00000000 _local 00401000 f one.obj\n"
        " 0001:00000010 _local 00401010 f two.obj\n"
        " 0003:00000000 _state 00402000 one.obj\n", encoding="ascii")
    code, symbols = read_map(path)
    assert len(code) == 2 and code[0][1] != code[1][1]
    assert resolve_watch(symbols, ["_state"]) == {"_state": 0x402000}
    for missing in ("_local", "state", "_absent"):
        with pytest.raises(ValueError, match="exact unambiguous"):
            resolve_watch(symbols, [missing])
    path.write_text("not a map", encoding="ascii")
    with pytest.raises(ValueError, match="no linked"):
        read_map(path)


def test_process_diagnostic_never_consumes_faults_or_application_breakpoints() -> None:
    from _recoil.commands.gameplay_diagnose import exception_disposition, capture_exception_detail

    assert capture_exception_detail(0x80000001, True, 1)
    assert not capture_exception_detail(0x80000001, True, 100)
    assert capture_exception_detail(0x80000001, False, 100)
    assert capture_exception_detail(0xc0000005, True, 100)

    seen = set()
    assert exception_disposition(0x80000003, 0x70000000, True, 0x400000, 0x10000, seen) == 0x10002
    for code, address, first in (
        (0x80000003, 0x70000000, True), (0xc0000005, 0x401000, True),
        (0x80000003, 0x401000, True), (0x4000001f, 0x70000000, False),
    ):
        assert exception_disposition(code, address, first, 0x400000, 0x10000, seen) == 0x80010001


def test_process_diagnostic_context_layout_and_resume_on_read_failure() -> None:
    import ctypes
    from types import SimpleNamespace
    from _recoil.commands.gameplay_diagnose import WindowsDebugger, X86Context

    assert ctypes.sizeof(X86Context) == 716
    assert X86Context.Eip.offset == 184 and X86Context.Esp.offset == 196
    resumed = []
    debugger = object.__new__(WindowsDebugger)
    debugger.threads = {7: 99}
    debugger.watches = {}
    debugger.kernel = SimpleNamespace(
        SuspendThread=lambda handle: 0,
        ResumeThread=lambda handle: resumed.append(handle) or 1,
        Wow64GetThreadContext=lambda *args: (_ for _ in ()).throw(OSError("unreadable context")),
    )
    report = debugger.snapshot(stopped=False)
    assert resumed == [99] and "unreadable context" in report["threads"][0]["error"]
    assert report["stack_candidates_are_not_unwound_frames"] is True
