from __future__ import annotations

import json
from pathlib import Path
import sqlite3
import sys
from types import MappingProxyType

import pytest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))


def _check_relocation_source_name_refresh_preserves_context(monkeypatch):
    from copy import deepcopy
    from types import SimpleNamespace
    from _recoil.commands import relocation_source_names as names

    identity = "recoil:function:0x1000"
    snapshot = dict(symbol_id=identity, object_symbol="?Old@@", address="0x1000", end_exclusive="0x1020")
    native_context = dict(source=deepcopy(snapshot), owner_id=None, evidence_ids=["proof"], retail={"target": 4096})
    data = {"symbols": {
        identity: dict(binary="recoil", kind="function", pipeline_class="authored",
            native_eh_relocation_binding=dict(context=native_context, reviewed=True, reason="prior proof"),
            relocation_expectation_exceptions=[dict(source_binding=deepcopy(snapshot), object_symbol="?Old@@", target=8192)]),
        "recoil:data:0x2000": dict(relocation_target_binding=dict(
            binding_context=dict(source_binding=deepcopy(snapshot)), target=8192)),
    }, "accepted": {"owner": "unchanged"}}
    document = SimpleNamespace(data=data, collection=lambda key: data[key])
    payload = dict(schema="recoil-relocation-source-names-v1", reviewed=True, renames=[dict(
        source_symbol_id=identity, expected_object_symbol="?Old@@", object_symbol="?New@@",
        expected_occurrences=3, reason="Reviewed source identifier rename")])
    before = deepcopy(data)
    with monkeypatch.context() as patch:
        patch.setattr(names.expectations, "build_object_binding_snapshot",
            lambda *a, **kw: dict(snapshot, object_symbol=kw["object_symbol"]))
        patch.setattr(names.expectations, "relocation_target_binding_staleness",
            lambda binding, **kw: (binding, []))
        patch.setattr(names.expectations, "reviewed_exception_staleness", lambda binding, **kw: (binding, []))
        patch.setattr(names.eh, "binding_context", lambda *a: dict(native_context,
            source=dict(snapshot, object_symbol=a[3])))
        proposed, changes = names.plan_refresh(document, payload, bindings={}, reference=Path("unused"))
        assert len(changes) == 3 and data == before
        expected = deepcopy(before)
        expected["symbols"][identity]["native_eh_relocation_binding"]["context"]["source"]["object_symbol"] = "?New@@"
        exception = expected["symbols"][identity]["relocation_expectation_exceptions"][0]
        exception["source_binding"]["object_symbol"] = exception["object_symbol"] = "?New@@"
        expected["symbols"]["recoil:data:0x2000"]["relocation_target_binding"]["binding_context"]["source_binding"]["object_symbol"] = "?New@@"
        assert proposed == expected
        for key, value, error in (("expected_occurrences", 2, "population changed"),
                                  ("expected_object_symbol", "?Missing@@", "population changed"),
                                  ("source_symbol_id", "messages:function:0x1000", "existing authored")):
            invalid = deepcopy(payload)
            invalid["renames"][0][key] = value
            with pytest.raises(names.ProgressError, match=error):
                names.plan_refresh(document, invalid, bindings={}, reference=Path("unused"))
            assert data == before
        patch.setattr(names.expectations, "relocation_target_binding_staleness",
            lambda binding, **kw: (binding, [{"field": "retail_target"}]))
        with pytest.raises(names.ProgressError, match="target context is stale"):
            names.plan_refresh(document, payload, bindings={}, reference=Path("unused"))
        patch.setattr(names.expectations, "build_object_binding_snapshot",
            lambda *a, **kw: dict(snapshot, object_symbol=kw["object_symbol"], end_exclusive="0x1030"))
        with pytest.raises(names.ProgressError, match="differs beyond object_symbol"):
            names.plan_refresh(document, payload, bindings={}, reference=Path("unused"))
        assert data == before


def _check_symbol_name_batch_is_atomic_and_preserves_semantic_facts():
    from copy import deepcopy
    from types import SimpleNamespace
    from _recoil.commands.symbol_names import plan_renames, ProgressError

    data = {"symbols": {
        "recoil:function:one": {"binary": "recoil", "navigation_name": "Old", "accepted": {"bytes": True}},
        "recoil:function:two": {"binary": "recoil", "navigation_name": "Other"},
    }, "owners": {"recoil:owner:one": {"binary": "recoil", "name": "Old owner",
        "gates": {"source": "accepted"}, "relationships": {"primary_functions": ["one"]},
        "address_metadata": {"0x401000": {"name": "Old method", "source_path": "src/a.cpp"}}}}}
    original = deepcopy(data)
    row = dict(symbol_id="recoil:function:one", expected_name="Old", name="ReadLine", reason="reads one line")
    payload = dict(schema="recoil-symbol-names-v1", reviewed=True, binary="recoil", renames=[row])
    document = SimpleNamespace(data=data)
    proposed = plan_renames(document, payload)
    expected = deepcopy(data)
    expected["symbols"][row["symbol_id"]]["navigation_name"] = "ReadLine"
    assert proposed == expected and data == original
    bad_rows = [dict(row, expected_name="stale"), dict(row, symbol_id="recoil:function:missing"),
                dict(row, name="Old"), dict(row, name="bad\nname"), dict(row, name=" padded"),
                dict(row, reason=""), dict(row, address="0x401000")]
    for bad in bad_rows:
        with pytest.raises(ProgressError):
            plan_renames(document, {**payload, "renames": [
                dict(row, symbol_id="recoil:function:two", expected_name="Other"), bad]})
        assert data == original
    owner_row = dict(owner_id="recoil:owner:one", address="0x401000", expected_name="Old method",
                     name="ReadLine", reason="synchronize display name only")
    combined = {**payload, "owner_renames": [owner_row]}
    expected["owners"]["recoil:owner:one"]["address_metadata"]["0x401000"]["name"] = "ReadLine"
    assert plan_renames(document, combined) == expected and data == original
    for bad in (dict(owner_row, address="0x402000"), dict(owner_row, expected_name="stale"),
                dict(owner_row, owner_id="messages:owner:one"), dict(owner_row, gates={}),
                dict(owner_row, name="bad\nname")):
        with pytest.raises(ProgressError):
            plan_renames(document, {**payload, "owner_renames": [bad]})
        assert data == original
    with pytest.raises(ProgressError):
        plan_renames(document, {**payload, "owner_renames": [owner_row, owner_row]})
    for change in ({"renames": [row, row]}, {"renames": []}, {"binary": "messages"},
                   {"reviewed": 1}, {"owners": {}}, {"schema": "unknown"}):
        with pytest.raises(ProgressError):
            plan_renames(document, {**payload, **change})
        assert data == original


def _check_symbol_name_cli_dry_run_and_stale_revision(tmp_path, monkeypatch, capsys):
    from copy import deepcopy
    from types import SimpleNamespace
    from _recoil.commands import symbol_names as command

    data = {"symbols": {"recoil:function:one": {"binary": "recoil", "navigation_name": "Old"}}}
    payload = dict(schema="recoil-symbol-names-v1", reviewed=True, binary="recoil", renames=[
        dict(symbol_id="recoil:function:one", expected_name="Old", name="ReadLine", reason="observed input")])
    build = tmp_path / "build"
    build.mkdir()
    path = build / "names.json"
    path.write_text(json.dumps(payload))
    commits = []
    def commit(proposed, *, expected_revision, apply):
        assert expected_revision == 7
        commits.append(apply)
        if apply:
            data.clear()
            data.update(deepcopy(proposed))
        return SimpleNamespace(to_dict=lambda: {"applied": apply})
    monkeypatch.setattr(command, "REPO_ROOT", tmp_path)
    monkeypatch.setattr(command, "ProgressStore", lambda _: SimpleNamespace(
        load=lambda: SimpleNamespace(data=data, revision=7), commit=commit))
    args = ["--payload-file", str(path), "--expected-revision", "6", "--dry-run", "--json"]
    assert command.main(args) == 2 and commits == []
    args[3] = "7"
    assert command.main(args) == 0 and commits == [False]
    assert data["symbols"]["recoil:function:one"]["navigation_name"] == "Old"
    args[4] = "--apply"
    assert command.main(args) == 0 and commits == [False, True]
    assert data["symbols"]["recoil:function:one"]["navigation_name"] == "ReadLine"
    assert command.main(args) == 2 and commits == [False, True]

from _recoil.lib.progress import ProgressDocument  # noqa: E402
from _recoil.commands.progress_cli import (  # noqa: E402
    OrderTargetRoleGateError,
    _order_row_role_gate,
    _parse_source_path_relocation_payload,
    _relocate_source_paths,
    _source_path_relocation_matches,
)
from _recoil.lib.repository_paths import RepositoryPathInventory  # noqa: E402


OLD_TIME_PREFIX = "src/GameZRecoil/" + "Time"
NEW_TIME_PREFIX = "src/GameZRecoil/zTime"
from _recoil.lib.progress_sqlite import (  # noqa: E402
    APPLICATION_ID,
    DELETE_FACET,
    ConcurrentSQLiteProgressUpdate,
    ProgressSQLiteStore,
)


def test_function_tail_separation_requires_exact_snapshot_and_live_boundary_proof(tmp_path, monkeypatch):
    from copy import deepcopy
    from types import SimpleNamespace as Row
    from _recoil.commands import function_tail_padding as tail

    symbol_id = "recoil:function:0x401000"
    evidence_id = "recoil:evidence:boundary"
    function = dict(binary="recoil", kind="function", pipeline_class="authored", extent_state="known",
        output_section_id="recoil:section:.text", address="0x401000", end_exclusive="0x401010", size=16,
        physical_block_id="recoil:block:unit", evidence_ids=[evidence_id],
        binary_state={"call_contract": {"disposition": "accepted", "result": "passed"}})
    data = dict(symbols={symbol_id: function}, evidence={evidence_id: {}}, owners={"owner": {"preserved": True}},
        physical_blocks={"recoil:block:unit": dict(start="0x401000", end_exclusive="0x401010",
                                                 contribution_ids=[symbol_id])})
    document = Row(data=data, collection=lambda name: data[name])
    payload = dict(schema="recoil-function-tail-padding-v1", reviewed=True, reason="reviewed retail boundary",
                   symbol_id=symbol_id, current_symbol=deepcopy(function), body_end_exclusive="0x401002",
                   evidence_ids=[evidence_id])
    original = deepcopy(data)
    proposed, interval = tail.plan_separation(document, payload)
    assert data == original
    assert proposed["symbols"][symbol_id]["size"] == 2
    assert proposed["symbols"][symbol_id]["binary_state"] == function["binary_state"]
    assert proposed["physical_blocks"] == data["physical_blocks"] and proposed["owners"] == data["owners"]
    assert proposed["symbols"][symbol_id]["tail_padding_separation"]["acceptance"] == "unaccepted"
    for changes in ({"reviewed": 1}, {"reason": " "}, {"evidence_ids": []},
                    {"body_end_exclusive": "0x401010"}, {"body_end_exclusive": "0x401000"},
                    {"current_symbol": {**function, "size": 15}}):
        with pytest.raises(tail.ProgressError):
            tail.plan_separation(document, {**payload, **changes})
    for changes in ({"logical_aliases": {"alias": {}}}, {"storage_contribution_ids": ["storage"]},
                    {"accepted_byte_facts": {"accepted": True}},
                    {"binary_state": {"object_byte": {"disposition": "accepted"}}}):
        function.update(changes)
        with pytest.raises(tail.ProgressError):
            tail.plan_separation(document, {**payload, "current_symbol": deepcopy(function)})
        function.clear()
        function.update(deepcopy(original["symbols"][symbol_id]))
    data["symbols"]["other"] = dict(binary="recoil", address="0x401004", end_exclusive="0x401008")
    with pytest.raises(tail.ProgressError, match="overlaps"):
        tail.plan_separation(document, payload)
    del data["symbols"]["other"]
    retail = b"\x90\xc3" + b"\xcc" * 14
    assembly = "00401000  90               nop\n00401001  c3               retn"
    tail.prove_tail(retail, *interval, assembly)
    # A long opcode fills BN's byte column, leaving one space before the mnemonic.
    tail.prove_tail(bytes.fromhex("f7 45 fc ff ff ff 7f c3") + b"\xcc" * 8,
        0x401000, 0x401008, 0x401010,
        "00401000  f7 45 fc ff ff ff 7f test dword [ebp-4], 0x7fffffff\n00401007  c3 ret")
    for image, listing in ((retail[:-1]+b"\x90", assembly), (retail, assembly.splitlines()[0]),
                           (retail, assembly+"\n00401002  cc               int3"),
                           (b"\x90\x90"+retail[2:], assembly)):
        with pytest.raises(tail.ProgressError):
            tail.prove_tail(image, *interval, listing)
    reference = tmp_path / "retail.exe"
    reference.write_bytes(retail)
    monkeypatch.setattr(tail, "parse_pe_headers", lambda *a, **kw: Row(image_base=0x400000,
        sections=[Row(virtual_address=0x1000, raw_size=16, name=".text")]))
    monkeypatch.setattr(tail, "rva_to_offset", lambda rva, sections: rva-0x1000)
    controls = {}
    statuses = []

    def request(endpoint, **params):
        if endpoint == "status":
            statuses.append(1)
            return dict(loaded=True, filename=tail.reference_image("recoil").bndb_path,
                platform="windows-x86", arch="x86", analysis={"state": "AnalysisState.IdleState"},
                view_identity="view", session_id="session",
                view_revision=len(statuses) if controls.get("drift") else 1)
        if endpoint == "functionAt":
            return dict(address=params["address"], functions=[{}] if controls.get("function") else [])
        return dict(address=params["address"], code_references=[], data_references=[],
                    coverage={"complete": not controls.get("partial"), "errors": []},
                    total=1 if controls.get("reference") else 0, has_more=False, truncated=False, partial=False)

    bridge = Row(get_json=request, assembly=lambda name: assembly)
    assert tail.prove_live(reference, interval, bridge)["tail_size"] == 14
    for flag in ("drift", "function", "partial", "reference"):
        controls[flag] = True
        with pytest.raises(tail.ProgressError):
            tail.prove_live(reference, interval, bridge)
        controls.clear()


def _check_provisional_alias_labels_preserve_identity_and_require_independent_review():
    from copy import deepcopy
    from _recoil.commands.progress_cli import _parse_logical_alias_group_payload, ProgressError

    prefix = "recoil:logical-function:0x401000:"
    def alias(name, status):
        return dict(object_symbol="?" + name + "@@", original_name=name, original_name_status=status,
                    source_owner_status="authored-owner", owner_id="recoil:owner:unit",
                    pipeline_class="authored", authored_order_role="authored-body",
                    fold_status="proven-fold-alias")
    old = {prefix + "first": alias("First", "recovered"), prefix + "second": alias("Second", "provisional")}
    current = dict(pipeline_class="non-authored", authored_order_role="compiler-generated-icf-representative",
                   physical_block_id="recoil:block:0x401000", linked_address_group=None,
                   icf_address_group=dict(winner_status="winner-unknown", winner_identity_key=None, evidence_ids=["prior"]),
                   logical_aliases={key: dict(value, evidence_ids=["prior"]) for key, value in old.items()})
    review = dict(reviewed=True, spelling_kind="reconstruction-only", original_spelling="unknown",
                  identity_basis="Independent retail caller protocol proves a distinct logical callee.")
    payload = dict(schema="recoil-logical-alias-group-v2", reviewed=True, reason="Reviewed logical extension",
                   symbol_id="recoil:function:0x401000", address="0x401000", current=current,
                   icf_address_group=dict(winner_status="winner-unknown", winner_identity_key=None),
                   logical_aliases={**old, prefix + "third": alias("Third", "provisional")},
                   new_evidence=dict(summary="Retail identity review",
                       provenance=dict(candidate_independent=True, provisional_alias_reviews={prefix + "third": review}),
                       artifacts=[dict(path="support/Recoil.exe", size=1)],
                       validation_context=dict(candidate_output_used=False)))
    before = deepcopy(payload)
    parsed = _parse_logical_alias_group_payload(json.dumps(payload))
    assert parsed["logical_aliases"][prefix + "third"]["original_name_status"] == "provisional"
    assert parsed["current"] == current and payload == before
    # A spelling label grants no instruction, linkage, source-location, or owner acceptance.
    assert set(parsed["logical_aliases"][prefix + "third"]) == set(alias("Third", "provisional"))
    invalid = []
    for field, value in (("reviewed", 1), ("spelling_kind", "recovered"), ("original_spelling", "Third"),
                         ("identity_basis", ""), ("unexpected", True)):
        row = deepcopy(payload)
        row["new_evidence"]["provenance"]["provisional_alias_reviews"][prefix + "third"][field] = value
        invalid.append(row)
    for reviews in ({}, {prefix + "third": None}, {prefix + "other": review}):
        row = deepcopy(payload)
        row["new_evidence"]["provenance"]["provisional_alias_reviews"] = reviews
        invalid.append(row)
    for field, value in (("pipeline_class", "authored"), ("logical_aliases", None), ("icf_address_group", None)):
        row = deepcopy(payload)
        row["current"][field] = value
        invalid.append(row)
    row = deepcopy(payload)
    del row["logical_aliases"][prefix + "first"]
    invalid.append(row)
    row = deepcopy(payload)
    row["logical_aliases"][prefix + "second"]["original_name"] = "Renamed"
    invalid.append(row)
    row = deepcopy(payload)
    row["current"]["icf_address_group"]["winner_status"] = "selected-winner"
    invalid.append(row)
    row = deepcopy(payload)
    row["new_evidence"]["validation_context"]["candidate_output_used"] = True
    invalid.append(row)
    row = deepcopy(payload)
    row["logical_aliases"][prefix + "third"]["original_name_status"] = "unknown"
    invalid.append(row)
    for row in invalid:
        with pytest.raises(ProgressError):
            _parse_logical_alias_group_payload(json.dumps(row))
    assert payload == before


def test_exception_removal_requires_complete_typed_match_and_preserves_other_facts(monkeypatch, tmp_path, capsys):
    _check_provisional_alias_labels_preserve_identity_and_require_independent_review()
    _check_relocation_source_name_refresh_preserves_context(monkeypatch)
    _check_symbol_name_batch_is_atomic_and_preserves_semantic_facts()
    _check_symbol_name_cli_dry_run_and_stale_revision(tmp_path, monkeypatch, capsys)
    from copy import deepcopy
    from types import SimpleNamespace as Row
    from _recoil.commands import relocation_expectation_mutation as mutation

    source_id = "recoil:function:0x401000"
    exception = {"reviewed": True, "offsets": [1], "type": 6, "target_symbol_id": "recoil:data:unit",
                 "reason": "reviewed ambiguity", "source_binding": {"object_symbol": "_entry"}}
    other = {**exception, "offsets": [5]}
    data = ProgressDocument.empty().data
    data["symbols"][source_id] = {"binary": "recoil", "kind": "function", "address": "0x401000",
                                  "end_exclusive": "0x401010", "relocation_expectation_exceptions": [exception, other]}
    data["symbols"]["recoil:data:unit"] = {"retained": "target facts"}
    original = deepcopy(data)
    commits = []

    def commit(proposed, *, expected_revision, apply):
        assert expected_revision == 0
        commits.append(deepcopy(proposed))
        if apply:
            data.clear()
            data.update(deepcopy(proposed))
        return Row(to_dict=lambda: {"applied": apply})

    monkeypatch.setattr(mutation, "ProgressStore", lambda path: Row(load=lambda: ProgressDocument(data), commit=commit))
    args = dict(progress=Path("unit.sqlite3"), source_symbol_id=source_id, source_address="0x401000",
                payload=exception, reason="replace superseded exception with reviewed named identity",
                expected_revision=0, apply=False)
    result = mutation.remove_reviewed_exception(**args)
    assert result["operation"] == "remove" and not result["commit"]["applied"]
    assert data == original
    expected = deepcopy(original)
    expected["symbols"][source_id]["relocation_expectation_exceptions"] = [other]
    assert commits == [expected]
    for changes in ({"expected_revision": 1}, {"source_address": "0x401004"}, {"reason": " "},
                    {"payload": {"reviewed": True, "offsets": [1]}},
                    {"payload": {**exception, "offsets": [True]}}):
        with pytest.raises(mutation.RelocationExceptionMutationError):
            mutation.remove_reviewed_exception(**{**args, **changes})
    assert len(commits) == 1
    mutation.remove_reviewed_exception(**{**args, "apply": True})
    assert data == expected
    with pytest.raises(mutation.RelocationExceptionMutationError, match="exactly one"):
        mutation.remove_reviewed_exception(**args)


def make_store(path: Path) -> ProgressSQLiteStore:
    document = ProgressDocument.empty().data
    document["evidence"] = {
        "recoil:evidence:unit": {"kind": "unit-evidence", "scope_ids": []}
    }
    return ProgressSQLiteStore.create_from_mapping(
        path, document, cutover_pair_id="proof-kernel"
    )


def test_repair_created_data_preserves_legacy_owner_and_refuses_acquired_state(monkeypatch):
    from copy import deepcopy
    from types import SimpleNamespace
    from _recoil.commands.relocation_target_mutation import _pending_data_symbol
    from _recoil.commands import relocation_target_repair as repair
    from _recoil.lib.progress import ProgressError

    target_id, owner_id = "recoil:data:0x501000", "recoil:owner:new"
    relation = dict(kind="primary-data", address="0x501000", symbol_id=target_id, name="token")
    target = _pending_data_symbol(address=0x501000, end_exclusive=0x501003,
        name="token", output_section_id="recoil:section:.data", evidence_ids=["recoil:evidence:token"])
    target["relocation_target_binding"] = dict(object_symbol="_token", binding_context=dict(creation_mode="created-data-symbol",
        owner=dict(owner_id=owner_id), relationship=relation))
    owner = dict(relationships=[relation], gates={"source": "accepted"},
        reimplementation=dict(entries={target_id: dict(kind="data", tier="X", evidence_ids=[])}))
    legacy = dict(kind="data-owner", provider_state="pending", lifecycle_state="discovered",
        relationships=[relation], reimplementation=dict(entries={target_id: dict(kind="data", tier="C", evidence_ids=["old"])}))
    data = dict(symbols={target_id: target}, owners={owner_id: owner, "recoil:owner:legacy": legacy})
    monkeypatch.setattr(repair, "_validate_owner_evidence", lambda *args, **kw: legacy)
    monkeypatch.setattr(repair, "normalize_relocation_target_binding", lambda binding: binding)

    def check(current):
        document = SimpleNamespace(data=current, collection=lambda name: current.get(name, {}))
        payload = dict(schema="recoil-repair-created-relocation-owner-v1", reviewed=True,
            reason="Repair an unreviewed duplicate creation", target_symbol_id=target_id,
            current_target=deepcopy(current["symbols"][target_id]), current_owner=deepcopy(current["owners"][owner_id]),
            retained_owner_id="recoil:owner:legacy", current_retained_owner=deepcopy(current["owners"]["recoil:owner:legacy"]),
            evidence_ids=["recoil:evidence:token"])
        return repair.repair_pending_owner(document, payload)

    before = deepcopy(data)
    result, detail = check(data)
    assert data == before and result["owners"]["recoil:owner:legacy"] == legacy
    assert result["symbols"][target_id]["size"] == target["size"]
    assert result["owners"][owner_id]["relationships"] == []
    assert result["owners"][owner_id]["gates"] == owner["gates"]
    assert detail["other_owner_facts_preserved"]
    for mutation in (
        lambda d: d["symbols"][target_id].update(size=9),
        lambda d: d["symbols"][target_id].update(relocation_target_bindings=[]),
        lambda d: d["owners"][owner_id]["reimplementation"]["entries"][target_id].update(tier="C"),
        lambda d: d["owners"].update({"third": {"relationships": [relation]}}),
        lambda d: d["owners"][owner_id]["relationships"].append(deepcopy(relation)),
    ):
        changed = deepcopy(data)
        mutation(changed)
        with pytest.raises(ProgressError):
            check(changed)


def test_missing_typed_data_preserves_exact_owner_facts_and_rejects_conflicts(monkeypatch):
    from copy import deepcopy
    from types import SimpleNamespace
    from _recoil.commands import relocation_target_mutation as mutation

    owner_id, evidence_id = "recoil:owner:unit.reader", "recoil:evidence:unit.reader"
    target_id = "recoil:data:0x501000"
    relationship = {"kind": "primary-data", "address": "0x501000",
                    "symbol_id": target_id, "name": "Reader::token"}
    data = ProgressDocument.empty().data
    data["owners"][owner_id] = {
        "binary": "recoil", "kind": "source-file", "provider_state": "unresolved",
        "lifecycle_state": "active", "evidence_ids": [evidence_id],
        "relationships": [deepcopy(relationship)],
        "reimplementation": {"entries": {target_id: {
            "kind": "data", "tier": "B", "evidence_ids": [evidence_id]}}},
    }
    data["evidence"][evidence_id] = {"scope_ids": [owner_id]}
    original_owner = deepcopy(data["owners"][owner_id])
    commits = []

    def commit(proposed, **kwargs):
        commits.append(deepcopy(proposed))
        return SimpleNamespace(to_dict=lambda: {"applied": kwargs["apply"]})

    monkeypatch.setattr(mutation, "ProgressStore", lambda _: SimpleNamespace(
        load=lambda: ProgressDocument(deepcopy(data)), commit=commit))
    monkeypatch.setattr(mutation, "_source_row", lambda *a, **k: {})
    monkeypatch.setattr(mutation, "build_object_binding_snapshot", lambda *a, **k: {})
    monkeypatch.setattr(mutation, "decode_retail_relocation_at_offset", lambda **k: {
        "offset": 1, "type": 6, "type_name": "DIR32", "retail_target": 0x501000,
        "instruction_offset": 0, "opcode": "68"})
    monkeypatch.setattr(mutation, "_retail_section", lambda *a, **k: "recoil:section:.data")
    monkeypatch.setattr(mutation, "normalize_relocation_target_binding", lambda value: value)
    monkeypatch.setattr(mutation, "relocation_target_binding_staleness", lambda value, **k: (value, []))

    def bind():
        return mutation.bind_relocation_target(
            progress=Path("unused.sqlite3"), reference=Path("retail.exe"), manifest_dir=Path("."),
            source_symbol_id="recoil:function:0x401000", source_address="0x401000",
            expected_revision=0, apply=False, bindings={}, payload={
                "reviewed": True, "source_object_symbol": "_read", "offset": 1,
                "target_object_symbol": "_token", "target_owner_id": owner_id,
                "reason": "complete retail literal and owner evidence", "evidence_ids": [evidence_id],
                "create_missing_data": True, "target_end_exclusive": "0x501004",
                "target_name": "Reader::token"})

    assert bind()["target_created"] is True
    assert commits[-1]["owners"][owner_id] == original_owner
    row = commits[-1]["symbols"][target_id]
    assert row["accepted_byte_facts"] is None and row["accepted_order_facts"] is None
    assert all(state["result"] == "pending" for state in row["binary_state"].values())
    assert target_id not in data["symbols"]

    for changed in ({"name": "different"}, {"symbol_id": "recoil:data:other"},
                    {"address": "0x501001"}, {"kind": "primary-function"}):
        data["owners"][owner_id]["relationships"] = [{**relationship, **changed}]
        with pytest.raises(mutation.RelocationTargetMutationError, match="relationship conflicts"):
            bind()
    data["owners"][owner_id]["relationships"] = [relationship, deepcopy(relationship)]
    with pytest.raises(mutation.RelocationTargetMutationError, match="duplicated"):
        bind()
    data["owners"][owner_id]["relationships"] = [relationship]
    for conflicting_entry in ({"kind": "function", "tier": "B"}, "invalid"):
        data["owners"][owner_id]["reimplementation"]["entries"][target_id] = conflicting_entry
        with pytest.raises(mutation.RelocationTargetMutationError, match="tier entry conflicts"):
            bind()
    data["owners"][owner_id] = deepcopy(original_owner)
    data["owners"]["recoil:owner:existing"] = {"relationships": [deepcopy(relationship)]}
    with pytest.raises(mutation.RelocationTargetMutationError, match="already has a primary relationship"):
        bind()
    del data["owners"]["recoil:owner:existing"]
    data["symbols"][target_id] = dict(binary="recoil", kind="data", address="0x501000", extent_state="unknown")
    with pytest.raises(mutation.RelocationTargetMutationError, match="already exists"):
        bind()
    assert len(commits) == 1


def test_temporary_scalar_creation_preserves_extent_and_pending_acceptance(monkeypatch):
    from copy import deepcopy
    from _recoil.commands import relocation_expectation_mutation as mutation
    from _recoil.commands import relocation_expectations as expectations

    # The independent catalog route deliberately creates no owner relationship
    # or ownership field. Physical scalar snapshots must preserve that state.
    snapshot = dict(symbol_id="recoil:data:0x501000", binary="recoil", kind="data",
                    extent_state="known", output_section_id="recoil:section:.rdata",
                    address="0x501000", end_exclusive="0x501008", size=8,
                    retail_content_hex="00" * 8)
    unowned = expectations._normalize_physical_target_snapshot(snapshot)
    assert unowned["ownership_state"] is None
    assert expectations._normalize_physical_target_snapshot(unowned) == unowned
    owned = expectations._normalize_physical_target_snapshot(
        {**snapshot, "ownership_state": "primary-owned"})
    assert owned != unowned and owned["ownership_state"] == "primary-owned"
    for invalid in ("", False, 0, []):
        with pytest.raises(expectations.RelocationExpectationError, match="ownership_state"):
            expectations._normalize_physical_target_snapshot({**snapshot, "ownership_state": invalid})

    owner_id = "recoil:owner:unit.reader"
    evidence_id = "recoil:evidence:unit.reader"
    target = 0x501000
    target_id = f"recoil:data:0x{target:x}"
    data = ProgressDocument.empty().data
    data["owners"][owner_id] = {
        "binary": "recoil", "kind": "source-file", "provider_state": "unresolved",
        "lifecycle_state": "active", "evidence_ids": [evidence_id], "relationships": [],
        "gates": {"boundary": "accepted", "source": "accepted", "data": "accepted", "owner_linkage": "accepted"},
    }
    data["evidence"][evidence_id] = {"scope_ids": [owner_id]}
    section_requests = []

    def retail_section(document, *, reference, start, end_exclusive):
        section_requests.append((start, end_exclusive))
        return "recoil:section:.rdata"

    monkeypatch.setattr(mutation, "_retail_section", retail_section)

    def stage(size, current=data):
        proposed = deepcopy(current)
        result = mutation._stage_missing_physical_target(
            document=ProgressDocument(current), proposed=proposed, reference=Path("retail.exe"),
            normalized_request={
                "retail_target": target, "target_symbol_id": target_id,
                "evidence_ids": [evidence_id],
                "create_missing_data": {
                    "target_owner_id": owner_id, "target_end_exclusive": hex(target + size),
                    "target_name": "Reader::constant",
                },
            },
        )
        return result, proposed

    for size in (4, 8):
        result, proposed = stage(size)
        row = proposed["symbols"][target_id]
        assert section_requests[-1] == (target, target + size)
        assert result["target_end_exclusive"] == hex(target + size)
        assert row["size"] == size and row["accepted_byte_facts"] is None
        assert row["accepted_order_facts"] is None
        assert all(value["result"] == "pending" for value in row["binary_state"].values())
        assert target_id not in data["symbols"]
        owner = proposed["owners"][owner_id]
        assert owner["reimplementation"]["entries"][target_id] == {"kind": "data", "tier": "X", "evidence_ids": []}
        assert set(owner["gates"].values()) == {"pending"}
        normalized = {"exception_mode": expectations.PHYSICAL_TARGET_UNRESOLVED_VC5_TEMPORARY,
                      "target_symbol_id": target_id, "evidence_ids": [evidence_id],
                      "physical_target_binding": {"ownership_state": "primary-owned", "address": hex(target)}}
        mutation._bind_existing_physical_owner(ProgressDocument(proposed), normalized)
        assert normalized["physical_target_owner_binding"] == result["owner_binding"]
        assert normalized["physical_target_relationship"] == result["relationship"]
        ambiguous = deepcopy(proposed)
        ambiguous["owners"][owner_id + ".duplicate"] = deepcopy(owner)
        with pytest.raises(mutation.RelocationExceptionMutationError, match="exactly one current"):
            mutation._bind_existing_physical_owner(ProgressDocument(ambiguous), normalized)

    for size in (0, 3, 5, 16):
        with pytest.raises(mutation.RelocationExceptionMutationError, match="four or eight"):
            stage(size)
    assert len(section_requests) == 2

    occupied = deepcopy(data)
    occupied["symbols"]["recoil:data:unit.neighbor"] = {
        "binary": "recoil", "address": hex(target + 4), "end_exclusive": hex(target + 8),
    }
    with pytest.raises(mutation.RelocationExceptionMutationError, match="overlaps"):
        stage(8, occupied)


@pytest.mark.parametrize("phase", ["authored-function-order", "full-function-order"])
@pytest.mark.parametrize("registered,tracked", [("non-authored", "authored"), ("authored", "non-authored")])
def test_order_role_gate_rejects_independent_classification_disagreement(phase, registered, tracked):
    with pytest.raises(OrderTargetRoleGateError, match="pipeline_class disagrees"):
        _order_row_role_gate(
            target_id="unit-target", phase=phase,
            row={"pipeline_class": registered, "authored_order_role": "non-authored" if registered == "non-authored" else "authored-body"},
            tracker_row={"pipeline_class": tracked},
            address="0x401000", identity="recoil:function:0x401000",
        )


def test_order_role_gate_rejects_independent_role_disagreement():
    with pytest.raises(OrderTargetRoleGateError, match="authored_order_role disagrees"):
        _order_row_role_gate(
            target_id="unit-target", phase="authored-function-order",
            row={"pipeline_class": "authored", "authored_order_role": "authored-body"},
            tracker_row={"pipeline_class": "authored", "authored_order_role": "compiler-generated-thunk"},
            address="0x401000", identity="recoil:function:0x401000",
        )


def test_order_role_gate_preserves_legacy_missing_role_fallback(monkeypatch):
    _order_row_role_gate(
        target_id="unit-target", phase="authored-function-order",
        row={"pipeline_class": "authored", "authored_order_role": "authored-body"},
        tracker_row={"pipeline_class": "authored"},
        address="0x401000", identity="recoil:function:0x401000",
    )
    from types import SimpleNamespace
    from _recoil.commands import progress_cli as cli

    block = {"start": "0x402000", "end_exclusive": "0x402100", "order_targets": {}}
    targets = {name: {"binary": "recoil", "kind": "vc5", "name": name,
                      "registration": {}, "interval": interval}
               for name, interval in {
                   "before": ("0x401000", "0x402000"),
                   "current": ("0x402000", "0x402100"),
                   "after": ("0x402100", "0x403000"),
               }.items()}
    document = SimpleNamespace(
        pipeline=lambda *a, **k: {"phase": "authored-function-order", "physical_block_id": "block"},
        collection=lambda name: {"block": block} if name == "physical_blocks" else targets)
    visited = []
    monkeypatch.setattr(cli, "_registered_order_interval", lambda target: target["interval"])
    monkeypatch.setattr(cli, "_registered_order_scope", lambda *a: "authored")
    def contract(doc, target_id, **kwargs):
        visited.append(target_id)
        return {"target": targets[target_id], "covered_block_ids": ["block"]}
    monkeypatch.setattr(cli, "_target_order_contract", contract)
    result = cli.resolve_current_order_target(document)
    assert result["status"] == "ready" and result["target_id"] == "current"
    assert visited == ["current"]
    targets["overlapping"] = {**targets["current"], "interval": ("0x401000", "0x402080")}
    visited.clear()
    result = cli.resolve_current_order_target(document)
    assert result["reason_code"] == "order-target-ambiguous"
    assert visited == ["current", "overlapping"]


def test_provider_registration_accepts_only_detached_non_authored_inventory():
    from _recoil.commands.provider_function_mutation import (
        _validate_existing_function, _validate_tracker_ownership,
        ProviderFunctionMutationError,
    )
    document = ProgressDocument.empty()
    symbol_id = "recoil:function:0x401000"
    row = {
        "binary": "recoil", "kind": "function", "address": "0x401000",
        "end_exclusive": "0x401020", "size": 32, "extent_state": "known",
        "pipeline_class": "non-authored", "authored_order_role": "non-authored",
        "ownership_state": "primary-owned", "disposition": "unresolved",
        "output_section_id": "recoil:section:.text",
    }
    document.data["symbols"][symbol_id] = row
    for role in ("non-authored", "compiler-generated-icf-representative"):
        row["authored_order_role"] = role
        assert _validate_existing_function(
            document, function_id=symbol_id, address="0x401000",
        )[1:] == (0x401000, 0x401020)
        _validate_tracker_ownership(
            document, owner_id="new-provider", function_id=symbol_id,
            start=0x401000, end=0x401020,
        )
        for reference in ({"symbol_id": symbol_id}, {"address": "0x401000"}):
            document.data["owners"]["existing-owner"] = {
                "relationships": [{"kind": "primary-function", **reference}],
            }
            with pytest.raises(ProviderFunctionMutationError, match="still claimed"):
                _validate_existing_function(
                    document, function_id=symbol_id, address="0x401000",
                )
            document.data["owners"].clear()
        document.data["owners"]["existing-owner"] = {
            "relationships": [{"kind": "helper", "symbol_id": symbol_id}],
        }
        with pytest.raises(ProviderFunctionMutationError, match="already owned"):
            _validate_tracker_ownership(
                document, owner_id="new-provider", function_id=symbol_id,
                start=0x401000, end=0x401020,
            )
        document.data["owners"].clear()
    row["authored_order_role"] = "authored-body"
    with pytest.raises(ProviderFunctionMutationError, match="existing row is not"):
        _validate_existing_function(document, function_id=symbol_id, address="0x401000")


def test_provider_trace_producer_emits_explicit_empty_edge_population():
    from _recoil.commands.provider_function_mutation import _provider_boundary_source_traceability
    from _recoil.commands.source_trace_progress import normalize_source_traceability
    trace = _provider_boundary_source_traceability()
    assert trace == {"state": "not-applicable", "source_edges": [], "reason_code": "provider-boundary"}
    assert normalize_source_traceability(trace) == trace


def test_data_aggregate_coalescence_preserves_views_and_rejects_incomplete_closure(tmp_path, monkeypatch):
    from copy import deepcopy
    from types import SimpleNamespace
    from _recoil.commands import data_aggregate_progress as aggregate
    data = ProgressDocument.empty().data
    owner_id = "recoil:owner:unit.aggregate"
    section_id = "recoil:section:.data"
    target_id = "recoil:vc5-target:unit_data"
    base, tail = "0x4da010", "0x4da014"
    field_ids = [f"recoil:data:{address}" for address in (base, tail)]
    storage_ids = [f"recoil:storage:va:{address}" for address in (base, tail)]
    data["output_sections"][section_id] = {
        "binary": "recoil", "reference": {"image_address": "0x4da000", "virtual_size": 256},
        "contribution_ids": storage_ids.copy(),
    }
    data["owners"][owner_id] = {
        "binary": "recoil", "kind": "record", "name": "Unit aggregate", "section": "unit",
        "lifecycle_state": "discovered", "source_paths": [], "evidence_ids": [],
        "gates": {g: "pending" for g in ("boundary", "source", "data", "owner_linkage", "byte")},
        "relationships": [{"kind": "primary-data", "symbol_id": sid, "address": a, "name": n}
                          for sid, a, n in zip(field_ids, (base, tail), ("unitTag", "unitValue"))],
        "reimplementation": {"entries": {sid: {"kind": "data", "tier": "X", "evidence_ids": []}
                                         for sid in field_ids}},
    }
    for sid, storage_id, address, name in zip(field_ids, storage_ids, (base, tail), ("unitTag", "unitValue")):
        data["symbols"][sid] = {
            "binary": "recoil", "kind": "data", "disposition": "authored", "address": address,
            "navigation_name": name, "extent_state": "unknown", "output_section_id": section_id,
            "storage_contribution_ids": [storage_id], "verification_target_ids": [target_id],
        }
        data["storage_contributions"][storage_id] = {
            "binary": "recoil", "kind": "data-symbol", "output_section_id": section_id,
            "reference": {"address": address, "extent_state": "unknown"}, "symbol_ids": [sid],
            "owner_ids": [owner_id], "parent_contribution_id": None, "overlap": "none",
        }
    registration = {
        "manifest_path": "tools/vc5_verify_targets/unit_data.json", "source_from": "src/unit.cpp",
        "compiler_profile": "unit", "data_addresses": [base, tail], "function_addresses": [],
        "translation_unit_function_order": [], "linked_function_intervals": [],
        "check_function_order": False, "check_translation_unit_function_order": False,
    }
    data["verification_targets"][target_id] = {"registration": registration}
    replacement = {"registration": {**registration, "data_addresses": [base]}, "registered_addresses": [base]}
    monkeypatch.setattr(aggregate, "vc5_target_registration", lambda path: (target_id, deepcopy(replacement)))
    monkeypatch.setattr(aggregate, "load_manifest", lambda *a, **kw: SimpleNamespace(
        data_symbols=[SimpleNamespace(address=base, byte_length=8, name="unitAggregate")]))
    function_id = "recoil:function:0x401000"
    data["symbols"][function_id] = {"kind": "function", "binary": "recoil", "address": "0x401000",
        "binary_state": {"call_contract": {"result": "passed"}}, "accepted_byte_facts": {"unit": True}}
    data["physical_blocks"]["recoil:block:0x401000"] = {"accepted_order_facts": {"unit": True}}
    payload = {
        "schema": aggregate.SCHEMA, "operation": aggregate.OPERATION, "reviewed": True,
        "artifact_id": field_ids[0], "owner_id": owner_id, "navigation_name": "unitAggregate", "size": 8,
        "fields": [{"artifact_id": sid, "logical_artifact_id": f"recoil:logical-data:{a}:{n}",
                    "field_name": n, "size": s}
                   for sid, a, n, s in zip(field_ids, (base, tail), ("tag", "value"), (1, 4))],
        "padding": [{"offset": 1, "size": 3}],
        "expected_current": {"symbols": {sid: deepcopy(data["symbols"][sid]) for sid in field_ids},
            "storage_contributions": deepcopy(data["storage_contributions"]),
            "owner": deepcopy(data["owners"][owner_id]), "verification_targets": deepcopy(data["verification_targets"])},
        "new_evidence": {"kind": "reviewed-data-artifact-observation",
            "method": "immutable-retail-bn-plus-vc5sp3-listing",
            "summary": "Retail 0x4da010 accesses and native ABI prove the unit aggregate field and padding partition.",
            "scope_ids": [field_ids[0]], "observation": {"artifact_id": field_ids[0], "address": base,
                "size": 8, "end_exclusive": "0x4da018", "output_section_id": section_id},
            "command": f"python tools/recoil.py audit bn-data-evidence {base} --size 8 --binary recoil --json",
            "target_id": target_id, "artifacts": []},
    }
    before = deepcopy(data)
    result = aggregate.plan_coalescence(data, payload, expected_revision=data["revision"], repo_root=tmp_path)
    proposed = result["proposed"]
    assert data == before
    assert field_ids[1] not in proposed["symbols"]
    assert len(proposed["symbols"][field_ids[0]]["logical_aliases"]) == 2
    assert proposed["symbols"][field_ids[0]]["size"] == 8
    assert proposed["symbols"][function_id]["binary_state"]["call_contract"]["result"] == "pending"
    assert "accepted_byte_facts" not in proposed["symbols"][function_id]
    assert proposed["physical_blocks"] == data["physical_blocks"]
    assert proposed["evidence"][result["evidence_id"]]["provenance"]["superseded_records"] == payload["expected_current"]
    assert len(proposed["storage_contributions"]) == 1
    for corrupt, message in (
        (lambda d, p: p["padding"].clear(), "coverage"),
        (lambda d, p: p["fields"][0].update(size=5), "overlap"),
        (lambda d, p: p["fields"][1].update(logical_artifact_id=p["fields"][0]["logical_artifact_id"]), "logical"),
        (lambda d, p: p["expected_current"]["symbols"][field_ids[0]].update(navigation_name="stale"), "stale"),
        (lambda d, p: d["owners"].update({"foreign": {"relationships": [{"address": tail}]}}), "foreign"),
        (lambda d, p: d["symbols"].update({"recoil:data:0x4da012": {"binary": "recoil", "address": "0x4da012"}}), "overlaps"),
        (lambda d, p: d["symbols"].update({"recoil:data:0x4da008": {"binary": "recoil", "address": "0x4da008",
            "extent_state": "known", "size": 16, "end_exclusive": "0x4da018"}}), "overlaps"),
        (lambda d, p: d.update(unit_unhandled_reference={"target": field_ids[1]}), "unhandled current reference"),
        (lambda d, p: d.update(unit_unhandled_reference={"target": field_ids[0]}), "unhandled current base"),
        (lambda d, p: d["symbols"][function_id].update(target_binding=field_ids[0]), "unhandled current field"),
        (lambda d, p: p["expected_current"]["verification_targets"].clear(), "census"),
        (lambda d, p: d["storage_contributions"].update({"unhandled-parent": {"parent_contribution_id": storage_ids[0]}}), "field binding in storage"),
    ):
        changed, request = deepcopy(data), deepcopy(payload)
        corrupt(changed, request)
        with pytest.raises(aggregate.DataAggregateError, match=message):
            aggregate.plan_coalescence(changed, request, expected_revision=data["revision"], repo_root=tmp_path)


def test_source_trace_repair_compares_raw_current_but_validates_replacement():
    from copy import deepcopy
    from _recoil.commands.source_trace_progress import (
        plan_source_traceability_batch, SourceTraceProgressError,
    )
    data = ProgressDocument.empty().data
    symbol = "recoil:function:0x401000"
    malformed = {"state": "not-applicable", "reason_code": "provider-boundary"}
    data["symbols"][symbol] = {"source_traceability": malformed}
    replacement = {**malformed, "source_edges": []}
    payload = {"operation": "replace-batch", "reviewed": True, "updates": [{
        "artifact_id": symbol, "expected_current": malformed,
        "source_traceability": replacement,
    }]}
    plan = plan_source_traceability_batch(data, payload, expected_revision=data["revision"])
    assert plan.proposed["symbols"][symbol]["source_traceability"] == replacement
    assert data["symbols"][symbol]["source_traceability"] == malformed
    stale = deepcopy(payload)
    stale["updates"][0]["expected_current"] = replacement
    with pytest.raises(SourceTraceProgressError, match="expected_current is stale"):
        plan_source_traceability_batch(data, stale, expected_revision=data["revision"])
    invalid = deepcopy(payload)
    invalid["updates"][0]["source_traceability"] = malformed
    with pytest.raises(SourceTraceProgressError, match="missing"):
        plan_source_traceability_batch(data, invalid, expected_revision=data["revision"])


def test_new_store_has_one_atomic_three_domain_revision_vector(tmp_path: Path) -> None:
    path = tmp_path / "progress.sqlite3"
    store = make_store(path)
    document, vector = store.materialize_with_revision_vector()
    assert document["revision"] == 0
    assert vector.to_dict() == {
        "transaction_revision": 0,
        "semantic_revision": 0,
        "evidence_generation_revision": 0,
    }
    with sqlite3.connect(path) as connection:
        assert connection.execute("PRAGMA application_id").fetchone()[0] == APPLICATION_ID


def test_stale_transaction_revision_is_rejected(tmp_path: Path) -> None:
    store = make_store(tmp_path / "progress.sqlite3")
    candidate = store.materialize()
    candidate["id_sequences"] = {"unit": 1}
    store.commit(candidate, expected_revision=0, apply=True)
    with pytest.raises(ConcurrentSQLiteProgressUpdate):
        store.commit(candidate, expected_revision=0, apply=True)


def test_scoped_delete_updates_all_requested_domains_atomically(tmp_path: Path) -> None:
    store = make_store(tmp_path / "progress.sqlite3")
    commit = store.persist_scoped_changes(
        expected_domain_revisions={"semantic": 0, "evidence_generation": 0},
        entity_patches={
            "evidence": {"recoil:evidence:unit": {"": DELETE_FACET}}
        },
        increment_domains={"semantic", "evidence_generation"},
        apply=True,
    )
    assert commit.deleted_entities == 1
    assert store.materialize()["evidence"] == {}
    assert commit.revision_vector.to_dict() == {
        "transaction_revision": 1,
        "semantic_revision": 1,
        "evidence_generation_revision": 1,
    }


def _path_inventory(tmp_path: Path, *paths: str) -> RepositoryPathInventory:
    exact = frozenset(paths)
    return RepositoryPathInventory(
        repository_root=tmp_path,
        exact_paths=exact,
        casefolded_paths=MappingProxyType(
            {path.casefold(): (path,) for path in exact}
        ),
        allowed_roots=("src",),
        allowed_paths=(),
    )


def _source_path_payload(**expected_overrides: list[str]) -> dict[str, object]:
    expected = {
        "physical_block_ids": [],
        "semantic_span_ids": ["recoil:semantic:unit"],
        "owner_ids": ["recoil:owner:unit"],
        "artifact_ids": ["recoil:function:unit"],
        "pre_synced_verification_target_ids": ["recoil:vc5-target:unit"],
    }
    expected.update(expected_overrides)
    return {
        "schema": "recoil-source-path-relocation-v1",
        "reviewed": True,
        "reason": "unit relocation",
        "binary": "recoil",
        "old_prefix": OLD_TIME_PREFIX,
        "new_prefix": NEW_TIME_PREFIX,
        "expected_matches": expected,
    }


def _source_path_document() -> dict[str, object]:
    data = ProgressDocument.empty().data
    data["semantic_spans"] = {
        "recoil:semantic:unit": {
            "binary": "recoil",
            "source_path": OLD_TIME_PREFIX + "/Time.cpp",
        }
    }
    data["owners"] = {
        "recoil:owner:unit": {
            "binary": "recoil",
            "tier": "B",
            "gates": {"source": "accepted"},
            "source_paths": [
                OLD_TIME_PREFIX + "/Time.cpp",
                OLD_TIME_PREFIX + "/Time.h",
            ],
            "address_metadata": {
                "0x401000": {
                    "source_path": OLD_TIME_PREFIX + "/Time.cpp",
                    "anchor_id": "recoil:anchor:unit",
                }
            },
        }
    }
    data["symbols"] = {
        "recoil:function:unit": {
            "binary": "recoil",
            "binary_state": {
                "call_contract": {
                    "result": "passed",
                    "disposition": "accepted",
                    "freshness": "current",
                    "evidence_ids": ["recoil:evidence:unit"],
                }
            },
            "accepted_call_contract_facts": {"unit": True},
            "source_traceability": {
                "state": "resolved",
                "source_edges": [
                    {
                        "anchor_id": "recoil:anchor:unit",
                        "relation": "defines",
                        "evidence_ids": [],
                        "emission_context": {
                            "translation_unit": OLD_TIME_PREFIX + "/Time.cpp"
                        },
                    }
                ],
            },
            "logical_aliases": {
                "recoil:logical:unit": {
                    "source_traceability": {
                        "state": "resolved",
                        "source_edges": [
                            {
                                "anchor_id": "recoil:anchor:logical-unit",
                                "relation": "defines",
                                "evidence_ids": [],
                                "emission_context": {
                                    "translation_unit": OLD_TIME_PREFIX + "/Time.cpp"
                                },
                            }
                        ],
                    }
                }
            },
        }
    }
    data["verification_targets"] = {
        "recoil:vc5-target:unit": {
            "binary": "recoil",
            "registration": {
                "source_from": NEW_TIME_PREFIX + "/Time.cpp",
                "order_edit_paths": [NEW_TIME_PREFIX + "/Time.h"],
            },
        }
    }
    return data


def test_source_path_relocation_is_boundary_aware_and_conservatively_invalidates(
    tmp_path: Path,
) -> None:
    data = _source_path_document()
    data["migration"]["unrelated_path"] = OLD_TIME_PREFIX + "keeper/Clock.cpp"
    owner_before = dict(data["owners"]["recoil:owner:unit"])
    details = _relocate_source_paths(
        data,
        _source_path_payload(),
        inventory=_path_inventory(
            tmp_path,
            NEW_TIME_PREFIX + "/Time.cpp",
            NEW_TIME_PREFIX + "/Time.h",
        ),
    )
    owner = data["owners"]["recoil:owner:unit"]
    symbol = data["symbols"]["recoil:function:unit"]
    assert owner["source_paths"] == [
        NEW_TIME_PREFIX + "/Time.cpp",
        NEW_TIME_PREFIX + "/Time.h",
    ]
    assert owner["tier"] == owner_before["tier"]
    assert owner["gates"] == owner_before["gates"]
    assert owner["address_metadata"]["0x401000"]["anchor_id"] == "recoil:anchor:unit"
    assert symbol["binary_state"]["call_contract"]["result"] == "pending"
    assert "accepted_call_contract_facts" not in symbol
    assert (
        symbol["logical_aliases"]["recoil:logical:unit"]["source_traceability"]
        ["source_edges"][0]["emission_context"]["translation_unit"]
        == NEW_TIME_PREFIX + "/Time.cpp"
    )
    assert data["migration"]["unrelated_path"] == OLD_TIME_PREFIX + "keeper/Clock.cpp"
    assert details["scheduler_before"]["phase"] == details["scheduler_after"]["phase"]
    assert details["preserved"]["acceptance_not_expanded"] is True
    _check_partial_source_extraction(tmp_path / "partial")


def _check_partial_source_extraction(root: Path) -> None:
    from copy import deepcopy
    from _recoil.commands.source_path_extraction import apply_extraction, extraction_snapshot
    from _recoil.lib.progress import state_record

    old_path, new_path = "src/Owner.cpp", "src/Component.cpp"
    (root / "src").mkdir(parents=True)
    (root / old_path).write_text("void Retained() {}\n", encoding="utf-8")
    moved = "recoil:function:0x401000"
    retained = "recoil:function:0x401010"
    block_id, span_id, owner_id = "recoil:block:0x401000", "recoil:semantic:unit", "recoil:owner:unit"
    source = (
        "/**\n * @recoil-anchor recoil:anchor:unit.extracted\n"
        " * @recoil-artifact defines .text recoil:function:0x401000: Component body.\n"
        " *\n * Purpose: Update the component.\n */\nvoid Extracted() {}\n"
    )
    (root / new_path).write_text(source, encoding="utf-8")
    trace = {"state": "resolved", "reason_code": None, "source_edges": [{
        "relation": "defines", "anchor_id": "recoil:anchor:unit.extracted",
        "emission_context": {"translation_unit": old_path}, "evidence_ids": []}]}
    data = ProgressDocument.empty().data
    data["symbols"] = {
        symbol_id: {"binary": "recoil", "kind": "function", "address": address,
                    "pipeline_class": "authored", "source_traceability": deepcopy(trace),
                    "physical_block_id": block_id, "semantic_span_ids": [span_id],
                    "binary_state": {"call_contract": state_record("passed", "accepted", "current", [])},
                    "function_match": {"freshness": "current"},
                    "instruction_match_review": {"sentinel": True},
                    "accepted_byte_facts": {"sentinel": True}}
        for symbol_id, address in ((moved, "0x401000"), (retained, "0x401010"))
    }
    data["owners"] = {owner_id: {"binary": "recoil", "source_paths": [old_path],
        "relationships": [{"kind": "primary-function", "symbol_id": moved}],
        "gates": {"boundary": "accepted", "source": "accepted", "data": "accepted"},
        "address_metadata": {"0x401000": {"name": "Component", "source_path": old_path}},
        "reimplementation": {"entries": {moved: {"tier": "B", "evidence_ids": []}}}}}
    data["physical_blocks"] = {block_id: {"binary": "recoil", "source_path": old_path,
        "original_source_path": "historical.cpp", "contribution_ids": [moved, retained],
        "mapping": {"state": "unresolved", "status": "inferred", "evidence_ids": []}}}
    data["semantic_spans"] = {span_id: {"source_path": old_path, "symbol_ids": [moved, retained]}}
    data["verification_targets"] = {"target": {"binary": "recoil", "source_files": [old_path, new_path]}}
    baseline = deepcopy(data)
    payload = extraction_snapshot(data, old_path, new_path, root=root)
    payload.update(reviewed=True, reason="Independent reviewed compilation boundary")
    with pytest.MonkeyPatch.context() as patch:
        # The existing pipeline kernel tests own scheduler mechanics. Here its
        # observable contract isolates exact source-scope and rollback guards.
        patch.setattr(ProgressDocument, "pipeline", lambda *_args, **_kwargs: {"phase": "authored-byte-match"})
        patch.setattr(ProgressDocument, "audit", lambda *_args, **_kwargs: [])
        result = apply_extraction(data, payload, root=root)
        assert result["accepted"] is False
        assert data["symbols"][moved]["source_traceability"]["source_edges"][0]["emission_context"]["translation_unit"] == new_path
        assert data["symbols"][retained]["source_traceability"] == baseline["symbols"][retained]["source_traceability"]
        assert "accepted_byte_facts" not in data["symbols"][retained]
        assert data["symbols"][retained]["function_match"]["freshness"] == "changed"
        assert "instruction_match_review" not in data["symbols"][retained]
        assert data["owners"][owner_id]["relationships"] == baseline["owners"][owner_id]["relationships"]
        assert data["owners"][owner_id]["gates"] == {"boundary": "accepted", "source": "pending", "data": "accepted"}
        assert data["owners"][owner_id]["reimplementation"]["entries"][moved]["tier"] == "C"
        assert data["physical_blocks"][block_id]["original_source_path"] is None
        assert (root / old_path).read_text() == "void Retained() {}\n"
        synchronized = deepcopy(baseline)
        synchronized["symbols"][moved]["source_traceability"]["source_edges"][0]["emission_context"]["translation_unit"] = new_path
        current_payload = extraction_snapshot(synchronized, old_path, new_path, root=root)
        current_payload.update(reviewed=True, reason="Defining topology was synchronized separately")
        apply_extraction(synchronized, current_payload, root=root)
        assert synchronized["symbols"][moved]["source_traceability"] == data["symbols"][moved]["source_traceability"]
        missing_trace = deepcopy(baseline)
        del missing_trace["symbols"][moved]["source_traceability"]
        with pytest.raises(ValueError, match="register the current defining source edge first"):
            extraction_snapshot(missing_trace, old_path, new_path, root=root)
        for field in ("expected_owners", "expected_blocks", "expected_artifacts", "expected_verification_targets"):
            stale = deepcopy(payload)
            stale[field] = {}
            working = deepcopy(baseline)
            with pytest.raises(ValueError, match="stale or incomplete"):
                apply_extraction(working, stale, root=root)
            assert working == baseline
        protected = deepcopy(baseline)
        protected["physical_blocks"][block_id]["mapping"]["state"] = "accepted"
        protected_payload = extraction_snapshot(protected, old_path, new_path, root=root)
        protected_payload.update(reviewed=True, reason="Must remain blocked")
        before = deepcopy(protected)
        with pytest.raises(ValueError, match="accepted source-file mapping"):
            apply_extraction(protected, protected_payload, root=root)
        assert protected == before
        (root / old_path).write_text(source, encoding="utf-8")
        with pytest.raises(ValueError, match="still occur"):
            extraction_snapshot(baseline, old_path, new_path, root=root)


def test_source_path_relocation_refuses_unsynchronized_or_drifted_scope(
    tmp_path: Path,
) -> None:
    inventory = _path_inventory(
        tmp_path,
        NEW_TIME_PREFIX + "/Time.cpp",
        NEW_TIME_PREFIX + "/Time.h",
    )
    unsynced = _source_path_document()
    unsynced["verification_targets"]["recoil:vc5-target:unit"]["registration"][
        "source_from"
    ] = OLD_TIME_PREFIX + "/Time.cpp"
    with pytest.raises(ValueError, match="verification-target sync first"):
        _relocate_source_paths(unsynced, _source_path_payload(), inventory=inventory)

    drifted = _source_path_document()
    with pytest.raises(ValueError, match="exact match scope changed"):
        _relocate_source_paths(
            drifted,
            _source_path_payload(owner_ids=["recoil:owner:wrong"]),
            inventory=inventory,
        )


def test_source_path_relocation_refuses_missing_new_files_and_protected_survivors(
    tmp_path: Path,
) -> None:
    with pytest.raises(ValueError, match="no authenticated repository files"):
        _relocate_source_paths(
            _source_path_document(),
            _source_path_payload(),
            inventory=_path_inventory(tmp_path, "src/Elsewhere.cpp"),
        )

    protected = _source_path_document()
    protected["physical_blocks"] = {
        "recoil:block:protected": {
            "binary": "other",
            "original_source_path": OLD_TIME_PREFIX + "/Original.cpp",
        }
    }
    with pytest.raises(ValueError, match="outside its allowed"):
        _relocate_source_paths(
            protected,
            _source_path_payload(),
            inventory=_path_inventory(
                tmp_path,
                NEW_TIME_PREFIX + "/Time.cpp",
                NEW_TIME_PREFIX + "/Time.h",
            ),
        )


def test_source_path_relocation_payload_rejects_noop_case_and_nested_moves() -> None:
    payload = _source_path_payload()
    payload["new_prefix"] = "src/GameZRecoil/" + "time"
    with pytest.raises(ValueError, match="differ beyond path case"):
        _parse_source_path_relocation_payload(json.dumps(payload))

    payload = _source_path_payload()
    payload["new_prefix"] = OLD_TIME_PREFIX + "/Runtime"
    with pytest.raises(ValueError, match="nested old/new prefixes"):
        _parse_source_path_relocation_payload(json.dumps(payload))


def test_source_path_match_inventory_ignores_neighboring_prefixes() -> None:
    data = _source_path_document()
    data["owners"]["recoil:owner:neighbor"] = {
        "binary": "recoil",
        "source_paths": [OLD_TIME_PREFIX + "keeper/Clock.cpp"],
    }
    assert _source_path_relocation_matches(
        data,
        binary="recoil",
        old_prefix=OLD_TIME_PREFIX,
        new_prefix=NEW_TIME_PREFIX,
    )["owner_ids"] == ["recoil:owner:unit"]
