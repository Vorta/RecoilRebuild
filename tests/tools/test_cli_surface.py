from __future__ import annotations


def test_memory_trace_supports_fresh_build_roots_without_creating_them(tmp_path, monkeypatch):
    from _recoil.call_contract import catalog, reporting
    import pytest
    from _recoil.lib.progress import ProgressError
    monkeypatch.setattr(catalog, "CALL_CONTRACT_MEMORY_TRACE_ROOT", tmp_path)
    build_root = tmp_path / "fresh" / "scan"
    trace = tmp_path / "trace.jsonl"
    with reporting._open_call_contract_memory_trace_file(trace, build_root=build_root) as output:
        output.write("diagnostic\n")
    assert trace.read_text() == "diagnostic\n"
    assert not build_root.exists()
    with pytest.raises(ProgressError, match="overwrite"):
        reporting._open_call_contract_memory_trace_file(trace, build_root=build_root)
    build_root.mkdir(parents=True)
    with pytest.raises(ProgressError, match="outside"):
        reporting._open_call_contract_memory_trace_file(build_root / "trace.jsonl", build_root=build_root)

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


def test_json_live_route_keeps_policy_output_on_stderr_and_blocks_failed_policy(monkeypatch, capsys):
    calls = []
    policy_code = 0
    def run(command, **kwargs):
        calls.append(command)
        if command[-1] == "_recoil.commands.source_policy":
            print("policy diagnostic", file=kwargs.get("stdout"))
            return subprocess.CompletedProcess(command, policy_code)
        print('{"status": "checked"}')
        return subprocess.CompletedProcess(command, 0)
    monkeypatch.setattr(recoil.subprocess, "run", run)
    args = ["progress", "call-contract", "replay-live", "--apply", "--json"]
    assert recoil.main(args) == 0
    captured = capsys.readouterr()
    assert json.loads(captured.out) == {"status": "checked"}
    assert "policy diagnostic" in captured.err and len(calls) == 2
    calls.clear()
    policy_code = 1
    assert recoil.main(args) == 1
    captured = capsys.readouterr()
    assert captured.out == "" and "policy diagnostic" in captured.err
    assert len(calls) == 1


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
    assert not missing
    scoped = (
        {f"authored-storage/{dimension}/accept" for dimension in STORAGE_DIMENSIONS}
        | {f"existing-authored-owner/{gate}/accept" for gate in OWNER_GATES}
        | {f"existing-authored-owner-tier/{tier}/promote" for tier in TIERS[1:]}
    )
    assert result["completion_routes_complete"] is True
    routes = [spec for spec in recoil.COMMAND_SPECS if spec.module != "scoped_acceptance"]
    without_scoped = audit_completion_routes(routes)
    assert {row["id"] for row in without_scoped["obligations"] if not row["producers"]} == scoped
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


def test_acceptance_effects_match_real_order_and_byte_writes_without_storage_promotion(tmp_path, monkeypatch) -> None:
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
        # Effects declare both alternatives; an exact match must not be
        # required to write the optional instruction-fallback dimensions.
        assert set(data["symbols"]["function"]["binary_state"]) == {
            effect.dimension for effect in declared if not effect.dimension.endswith("instruction")}
    assert not _storage_accepted(storage)
    assert all(row == pending for row in storage["verification"].values())
    assert data["owners"] == {"owner": {"tier": "X"}}
    _exercise_scoped_acceptance(tmp_path, monkeypatch)


def _exercise_scoped_acceptance(tmp_path, monkeypatch):
    from copy import deepcopy
    from types import SimpleNamespace as Row
    import pytest
    from _recoil.commands import scoped_acceptance as scoped
    from _recoil.lib.progress import OWNER_GATES, STORAGE_DIMENSIONS, ProgressError
    from _recoil.lib.storage_proof import storage_scope

    sid, oid, storage_id = "recoil:data:0x401000", "recoil:owner:unit", "recoil:storage:unit"
    symbol = dict(binary="recoil", kind="data", disposition="authored", address="0x401000",
        end_exclusive="0x401004", size=4, extent_state="known", output_section_id="recoil:section:.data",
        storage_contribution_ids=[storage_id], source_traceability={"state": "resolved", "source_edges": [{}]})
    owner = dict(binary="recoil", kind="data-owner", relationships=[{"kind": "primary-data", "symbol_id": sid}],
        gates={key: "accepted" for key in OWNER_GATES}, reimplementation={"entries": {sid: {"tier": "X", "kind": "data"}}})
    storage = dict(binary="recoil", reference={k: symbol[k] for k in ("address", "end_exclusive", "size", "extent_state")},
        symbol_ids=[sid], owner_ids=[oid], output_section_id=symbol["output_section_id"],
        applicability={key: True for key in STORAGE_DIMENSIONS}, verification={})
    data = dict(symbols={sid: symbol}, owners={oid: owner}, storage_contributions={storage_id: storage})
    from _recoil.commands import owner_entry_repair as repair
    with monkeypatch.context() as patch:
        patch.setattr(repair, "validate_owner_invariants", lambda value: None)
        damaged = deepcopy(data)
        damaged["owners"][oid]["relationships"][0]["address"] = symbol["address"]
        damaged["owners"][oid]["reimplementation"]["entries"].clear()
        before = deepcopy(damaged)
        payload = dict(reviewed=True, reason="repair absent X bookkeeping", current_owners=deepcopy(damaged["owners"]))
        changes = repair.repair(damaged, payload)
        assert changes == [{"owner_id": oid, "symbol_id": sid, "tier": "X"}]
        expected = deepcopy(before)
        expected["owners"][oid]["reimplementation"]["entries"][sid] = {"kind": "data", "tier": "X", "evidence_ids": []}
        assert damaged == expected
        with pytest.raises(ProgressError): repair.repair(damaged, payload)
        with pytest.raises(ProgressError): repair.repair(deepcopy(before), {**payload, "reviewed": False})
    document = Row(data=data, revision=7, collection=lambda name: data[name])
    assert storage_scope(document, storage_id)[1:] == ([sid], 0x401000, 0x401004)
    for field, bad in (("owner_ids", []), ("symbol_ids", [sid, sid]), ("output_section_id", "recoil:section:.rdata")):
        original = storage[field]
        storage[field] = bad
        with pytest.raises(ProgressError): storage_scope(document, storage_id)
        storage[field] = original
    data["storage_contributions"]["unknown"] = dict(binary="recoil", output_section_id=symbol["output_section_id"],
        reference={"address": "0x401002", "extent_state": "unknown"})
    with pytest.raises(ProgressError): storage_scope(document, storage_id)
    del data["storage_contributions"]["unknown"]
    data["symbols"]["extra"] = dict(symbol)
    with pytest.raises(ProgressError, match="reciprocal"): storage_scope(document, storage_id)
    del data["symbols"]["extra"]
    comparison = dict(storage_id=storage_id, symbol_ids=[sid], dimensions={key: True for key in STORAGE_DIMENSIONS})
    for dimension in STORAGE_DIMENSIONS:
        proposal = deepcopy(data)
        scoped.record_storage(proposal, comparison, [dimension], "proof")
        assert set(proposal["storage_contributions"][storage_id]["verification"]) == {dimension}
        assert proposal["owners"] == data["owners"]
        bad = deepcopy(comparison); bad["dimensions"][dimension] = False
        with pytest.raises(ProgressError): scoped.record_storage(proposal, bad, [dimension], "proof")
    with monkeypatch.context() as patch:
        patch.setattr(scoped, "validate_owner_invariants", lambda value: None)
        events = []
        live = Row(document=document, source_graph=lambda members: events.append(("source", members)),
            storage=lambda identity: comparison, providers=lambda owner: {})
        for tier in ("C", "B", "A", "S"):
            report = scoped.verify_owner(live, oid, gates=[], tier=tier)
            proposal = deepcopy(data)
            scoped.record_owner(proposal, report, "proof")
            assert proposal["owners"][oid]["reimplementation"]["entries"][sid]["tier"] == tier
            assert proposal["symbols"] == data["symbols"]
        for gate in OWNER_GATES:
            report = scoped.verify_owner(live, oid, gates=[gate], tier=None)
            proposal = deepcopy(data)
            proposal["owners"][oid]["gates"] = {key: "pending" for key in OWNER_GATES}
            scoped.record_owner(proposal, report, "proof")
            assert [key for key, value in proposal["owners"][oid]["gates"].items() if value == "accepted"] == [gate]
        owner["gates"]["boundary"] = "none"
        with pytest.raises(ProgressError): scoped.verify_owner(live, oid, gates=[], tier="B")
        owner["gates"]["boundary"] = "accepted"
        symbol["source_traceability"]["state"] = "unresolved"
        with pytest.raises(ProgressError): scoped.verify_owner(live, oid, gates=[], tier="C")
        symbol["source_traceability"]["state"] = "resolved"
        assert events
        saved_gates = dict(owner["gates"])
        owner["gates"] = {key: "pending" for key in OWNER_GATES}
        live.data_presence = lambda identity: {"object_definition": identity}
        # C and boundary discovery can bootstrap mutually dependent owners.
        data["owners"]["dependency"] = {"kind": "data-owner", "gates": {"boundary": "pending"}}
        owner["relationships"].append({"kind": "depends-on-owner", "target_owner_id": "dependency"})
        assert scoped.verify_owner(live, oid, gates=[], tier="C")["passed"]
        assert scoped.verify_owner(live, oid, gates=["boundary"], tier=None)["passed"]
        owner["relationships"].pop(); del data["owners"]["dependency"]
        owner["gates"] = saved_gates
        owner["reimplementation"]["entries"][sid]["tier"] = "S"
        comparison["dimensions"]["raw"] = False
        with pytest.raises(ProgressError): scoped.verify_owner(live, oid, gates=[], tier="C")
        comparison["dimensions"]["raw"] = True
        owner["reimplementation"]["entries"][sid]["tier"] = "X"
    from _recoil.lib.storage_proof import preserve_storage_relationships
    def allocation(retail, candidate):
        return {"retail_address": hex(retail), "size": 4, "identities": [{"candidate_address": hex(candidate)}]}
    preserve_storage_relationships([allocation(0x1000, 0x2000), allocation(0x1010, 0x2020)])
    preserve_storage_relationships([allocation(0x1000, 0x2000), allocation(0x1000, 0x2000)])
    for pair in ([allocation(0x1000, 0x2000), allocation(0x1010, 0x2000)],
                 [allocation(0x1000, 0x2000), allocation(0x1000, 0x2020)]):
        with pytest.raises(ProgressError): preserve_storage_relationships(pair)
    context = {"members": {sid: symbol}}
    payload = dict(reviewed=True, context=context, gates=["source"], tier=None, rationale="reviewed source",
        scrutiny={"decision": "ALLOW", "rationale": "complete source review"},
        entries={sid: {"source": "attached definition", "behavior": "retail read semantics"}}, live_comparison={})
    scoped.validate_review(payload, context, gates=["source"])
    for changes in ({"reviewed": False}, {"context": {}}, {"entries": {}}, {"gates": ["byte"]},
                    {"scrutiny": {"decision": "BLOCK", "rationale": "missing proof"}}):
        with pytest.raises(ProgressError): scoped.validate_review({**payload, **changes}, context, gates=["source"])
    fresh = scoped.REPO_ROOT / "build/live-validation/unit-fresh"
    assert scoped.comparison_content({"object_path": fresh.as_posix()+"/unit.obj"}, fresh) == {"object_path": "<fresh-build>/unit.obj"}
    # Currentness rejects both new input paths and mixed output generations.
    live_inputs = object.__new__(scoped.LiveInputs)
    live_inputs.before = [{"path": "source", "size": 1}]
    live_inputs.input_inventory = lambda: list(live_inputs.before)
    live_inputs.root = tmp_path
    output = tmp_path/"one.obj"; output.write_bytes(b"one")
    live_inputs.outputs = {output: b"one"}
    live_inputs.unchanged()
    output.write_bytes(b"two")
    with pytest.raises(ProgressError): live_inputs.unchanged()
    output.write_bytes(b"one")
    extra = tmp_path/"extra.obj"; extra.write_bytes(b"extra")
    with pytest.raises(ProgressError): live_inputs.unchanged()
    extra.unlink()
    live_inputs.input_inventory = lambda: live_inputs.before + [{"path": "new-header", "size": 1}]
    with pytest.raises(ProgressError): live_inputs.unchanged()
    # Exercise the public storage handler through its fresh-build and guarded
    # mutation boundaries, with no native compiler or production ledger fixture.
    with monkeypatch.context() as patch:
        events, committed = [], []
        class Store:
            def __init__(self, path): pass
            def load(self): return document
            def mutate(self, transform, *, expected_revision, apply):
                assert expected_revision == document.revision
                proposal = deepcopy(data)
                transform(proposal)
                if apply: committed.append(proposal)
                return Row(to_dict=lambda: {"applied": apply})
        class Live:
            def __init__(self, doc, root): assert doc is document
            def build(self): events.append("build")
            def unchanged(self): events.append("unchanged")
            def storage(self, identity):
                assert events == ["build"]
                return deepcopy(comparison)
        patch.setattr(scoped, "ProgressStore", Store)
        patch.setattr(scoped, "LiveInputs", Live)
        patch.setattr(scoped, "add_live_evidence", lambda *a, **kw: "proof")
        args = Row(build_root=fresh, progress=tmp_path/"unused.sqlite3", expected_revision=7,
                   storage=storage_id, dimension=["extent"], apply=False)
        scoped.accept_storage(args)
        assert events == ["build", "unchanged", "unchanged"] and not committed
        events.clear(); args.apply = True
        scoped.accept_storage(args)
        assert len(committed) == 1 and committed[0]["owners"] == data["owners"]
        events.clear(); args.expected_revision = 6
        with pytest.raises(ProgressError, match="revision"): scoped.accept_storage(args)
        assert not events and len(committed) == 1
        args.expected_revision = 7
        def drift(self): raise ProgressError("inputs changed")
        patch.setattr(Live, "unchanged", drift)
        with pytest.raises(ProgressError, match="changed"): scoped.accept_storage(args)
        assert len(committed) == 1


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
