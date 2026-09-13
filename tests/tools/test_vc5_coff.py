from __future__ import annotations

from pathlib import Path
import struct
import sys

import pytest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from _recoil.commands import vc5_verify  # noqa: E402
from _recoil.lib.coff_alias import (  # noqa: E402
    CoffAlias,
    CoffAliasSource,
    parse_alias_source_text,
    validate_alias_object,
)


def mapped_switch_bytes() -> tuple[int, bytes]:
    # A bounded byte-remapped dispatch followed by a nested direct dispatch.
    base = 0x600000
    code = bytearray.fromhex(
        "83 f8 03 c7 44 24 04 01 00 00 00 77 24 33 c9 8a 88"
    )
    code += struct.pack("<I", base + 68)
    code += bytes.fromhex("ff 24 8d") + struct.pack("<I", base + 56)
    code += bytes.fromhex("83 fa 01 77 10 ff 24 95") + struct.pack("<I", base + 72)
    code += bytes.fromhex("b8 01 00 00 00 c3 33 c0 c3 33 c0 c3 90 90 90 90")
    code += struct.pack("<III", base + 28, base + 40, base + 46)
    code += bytes([0, 1, 0, 2])
    code += struct.pack("<II", base + 40, base + 46)
    return base, bytes(code)


def test_instruction_match_proves_values_and_requires_current_review(tmp_path, monkeypatch):
    from copy import deepcopy
    from _recoil.lib.function_match import compare_instructions, MATCH_VERSION
    from _recoil.lib import match_evidence as evidence
    from _recoil.lib.source_traceability import parse_source_trace_text
    from _recoil.commands.progress_v2 import accept_live_byte_groups
    from _recoil.lib.progress import ProgressError, is_current_accepted_state

    def compare(a, b, **kwargs):
        return compare_instructions(bytes.fromhex(a), bytes.fromhex(b), symbol="?f@@YAHXZ", **kwargs)

    # Temporary values may change registers and later rejoin ABI-fixed EAX.
    pairs = [
        ("ba01000000 42 c1e207 8bc2 c3", "b901000000 41 c1e107 8bc1 c3"),
        ("8b542404 85d2 7402 42 90 8bc2 c3", "8b4c2404 85c9 7402 41 90 8bc1 c3"),
        ("ba03000000 4a 75fd 8bc2 c3", "b903000000 49 75fd 8bc1 c3"),
        ("b201 b402 0fb6c2 c3", "b101 b402 0fb6c1 c3"),
    ]
    for first, second in pairs:
        proof = compare(first, second)
        assert proof["passed"], proof
        assert proof["differences"] and not proof["exact"]
    assert compare("33c0c3", "33c0c3")["exact"]
    rejected = [
        (pairs[0][0], "b901000000 41 c1e107 8bc2 c3"),  # unmatched use
        (pairs[0][0], "b902000000 41 c1e107 8bc1 c3"),  # constant
        (pairs[0][0], "b901000000 41 c1e106 8bc1 c3"),  # shift count
        (pairs[0][0], "b901000000 41 c1e107 8bc1 90 c3"),  # added work
        ("ba01000000 33c0 f7e2 c3", "b901000000 33c0 f7e1 c3 90"),
        ("b201 b402 0fb6c2 c3", "b101 b102 0fb6c1 c3"),  # alias clobber
        ("8bd0 8bc2 c3", "8bd1 8bc2 c3"),  # unequal entry values
        ("ba03000000 4a 75fd 8bc2 c3", "b903000000 49 74fd 8bc1 c3"),  # branch
        ("ba01000000 83ea01 c3", "b901000000 83e101 c3"),  # operation differs
        ("ba01000000 c1e201 8bc2 c3", "b901000000 c1f101 8bc1 c3"),  # /6 not a selector
        ("ba01000000 8bc2 c3", "bb01000000 8bc3 c3"),  # callee-save corruption
    ]
    for first, second in rejected:
        assert not compare(first, second)["passed"], (first, second)
    # A call through an IAT value can use a cdecl contract; a pointer DATA
    # relocation is never permission to assume that callback's convention.
    first = "ba01000000 52 ff1534124000 83c404 c3"
    second = "b901000000 51 ff1534124000 83c404 c3"
    relocation = {"offset": 8, "target_symbol": "__imp__fread", "target_symbol_id": "recoil:function:0x401234"}
    assert compare(first, second, relocations=[relocation])["passed"]
    assert not compare(first, second)["passed"]
    assert not compare(first, second, relocations=[{**relocation, "target_symbol_id": "recoil:data:0x401234"}])["passed"]
    # INC preserves unknown carry returned by the call; it cannot justify JB.
    assert not compare("ba01000000 52 ff1534124000 40 7201 90 c3",
                       "b901000000 51 ff1534124000 40 7201 90 c3", relocations=[relocation])["passed"]
    # Live-range reuse, flags and implicit operands must be proved even for
    # unchanged instructions after a differing definition.
    for first, second in [
        ("b801000000 f7e2 c3", "b901000000 f7e2 c3"),
        ("b001 b202 d2e2 0fb6c2 c3", "b101 b202 d2e2 0fb6c2 c3"),
    ]:
        assert not compare(first, second)["passed"]

    identity = "recoil:function:0x401000"
    text = "/**\r\n * @recoil-anchor recoil:anchor:unit\r\n * @recoil-artifact defines .text " + identity + ": Unit.\r\n *\r\n *\r\n * Purpose: unit.\r\n */\r\nint f() { return 1; }\r\n"
    path = tmp_path / "unit.cpp"
    path.write_bytes(text.encode())
    monkeypatch.setattr(evidence, "REPO_ROOT", tmp_path)
    state = {"version": MATCH_VERSION, "level": "instruction", "validation_mode": "live", "freshness": "current",
             "evidence_ids": ["proof"], "review_evidence_id": "review", "dependencies": evidence.dependency_states(["unit.cpp"])}
    review = {"version": MATCH_VERSION, "evidence_id": "review", "decision": "compiler-register-allocation-only",
              "no_remaining_credible_source_options": True, "context": {"code": "current"}, "differences": [{"offset": 1}]}
    row = {"function_match": state, "instruction_match_review": review}
    assert evidence.current_match_level(row) == "instruction"
    assert evidence.review_current(review, {"code": "current"}, [{"offset": 1}])
    for changes in ({"no_remaining_credible_source_options": False}, {"version": 0}, {"evidence_id": ""},
                    {"decision": "not-approved"}, {"context": {"code": "changed"}}, {"differences": []}):
        assert not evidence.review_current({**review, **changes}, {"code": "current"}, [{"offset": 1}])
    assert not evidence.current_match_level({"function_match": state})
    document = parse_source_trace_text(text, path="unit.cpp")
    edits, _ = evidence.annotation_edits([document], {identity: row})
    assert len(edits) == 1 and edits[0]["before"].count(b"\n") == edits[0]["after"].count(b"\n")
    tagged = edits[0]["after"].decode()
    assert " * @recoil-match instruction\r\n *\r\n * Purpose:" in tagged
    assert tagged.splitlines()[-2] == " */"
    for compact in (text.replace(" *\r\n *\r\n", " *\r\n"), text.replace(" *\r\n", "")):
        path.write_bytes(compact.encode())
        state["dependencies"] = evidence.dependency_states(["unit.cpp"])
        compact_edits, exclusions = evidence.annotation_edits([parse_source_trace_text(compact, path="unit.cpp")], {identity: row})
        assert not compact_edits and "prose separator" in exclusions[0]["reason"]
    path.write_bytes(text.encode())
    state["dependencies"] = evidence.dependency_states(["unit.cpp"])
    parsed = parse_source_trace_text(tagged, path="unit.cpp")
    assert parsed.matches[0].level == "instruction" and not parsed.findings
    path.write_bytes(edits[0]["after"])
    assert evidence.current_match_level(row) is None  # source-stat invalidation
    state["dependencies"] = evidence.dependency_states(["unit.cpp"])
    assert evidence.annotation_edits([parsed], {identity: row})[0] == []
    # A match directive may precede the other annotations in the same group.
    first_tag = tagged.replace(" * @recoil-match instruction\r\n", "").replace("/**\r\n", "/**\r\n * @recoil-match instruction\r\n")
    path.write_bytes(first_tag.encode())
    state["dependencies"] = evidence.dependency_states(["unit.cpp"])
    assert evidence.annotation_edits([parse_source_trace_text(first_tag, path="unit.cpp")], {identity: row})[0] == []
    for indentation in ("    ", "\t", "        "):
        indented = "\r\n".join(indentation + line if line else line for line in text.split("\r\n"))
        path.write_bytes(indented.encode())
        state["dependencies"] = evidence.dependency_states(["unit.cpp"])
        indent_edits, _ = evidence.annotation_edits([parse_source_trace_text(indented, path="unit.cpp")], {identity: row})
        expected = indented.replace(indentation + " *\r\n", indentation + " * @recoil-match instruction\r\n", 1)
        assert indent_edits[0]["after"].decode() == expected
        assert expected.count("\n") == indented.count("\n")
        assert evidence.strip_match_annotations(indentation + " * @recoil-match byte */") == indentation + " */"
    path.write_bytes(tagged.encode())
    state["dependencies"] = evidence.dependency_states(["unit.cpp"])
    for bad in ("BYTE", "near", "byte instruction", ""):
        assert parse_source_trace_text(tagged.replace("@recoil-match instruction", "@recoil-match " + bad), path="unit.cpp").findings
    duplicate = tagged.replace(" * Purpose:", " * @recoil-match byte\r\n * Purpose:")
    assert any(f.code == "duplicate-match-directive" for f in parse_source_trace_text(duplicate, path="unit.cpp").findings)
    assert parse_source_trace_text("/** @recoil-match byte */\nint f();", path="unit.cpp").findings
    assert any("match-directive" in f.code for f in
               parse_source_trace_text(tagged.replace("/**", "/*"), path="unit.cpp").findings)
    removed = evidence.annotation_edits([parsed], {})[0][0]["after"]
    assert b"@recoil-match" not in removed and removed.count(b"\n") == edits[0]["after"].count(b"\n")
    # A failed ledger compare-and-swap must roll back only our comment write;
    # an intervening user edit must survive that rollback.
    from _recoil.commands import match_progress
    monkeypatch.setattr(match_progress, "REPO_ROOT", tmp_path)
    path.write_bytes(edits[0]["before"])
    def reject_commit():
        raise ProgressError("revision changed")
    with pytest.raises(ProgressError, match="revision changed"):
        match_progress.with_annotation_writes(edits, True, reject_commit)
    assert path.read_bytes() == edits[0]["before"]
    def intervening_edit():
        path.write_bytes(b"user edit\n")
        raise ProgressError("revision changed")
    with pytest.raises(ProgressError):
        match_progress.with_annotation_writes(edits, True, intervening_edit)
    assert path.read_bytes() == b"user edit\n"
    path.write_bytes(edits[0]["after"])
    state["dependencies"] = evidence.dependency_states(["unit.cpp"])
    data = {"symbols": {identity: deepcopy(row)}}
    for mode in ("authored", "linked"):
        accept_live_byte_groups(data, mode=mode, groups=[[identity]], evidence_id="proof",
            facts={"validation_mode": "live", "mode": mode, "match_levels": {identity: "instruction"}})
        assert evidence.stage_match_current(data["symbols"][identity], mode, is_current_accepted_state)
    assert data["symbols"][identity]["binary_state"]["linked_byte"]["result"] == "failed"
    data["symbols"][identity]["instruction_match_review"]["evidence_id"] = "replaced"
    assert not evidence.stage_match_current(data["symbols"][identity], "linked", is_current_accepted_state)
    with pytest.raises(ProgressError):
        accept_live_byte_groups(data, mode="linked", groups=[[identity]], evidence_id="proof",
            facts={"validation_mode": "live", "mode": "linked", "match_levels": {identity: "instruction"}})
    # Exact matching needs no Pro review and must restore exact-byte truth.
    accept_live_byte_groups(data, mode="linked", groups=[[identity]], evidence_id="exact",
        facts={"validation_mode": "live", "mode": "linked", "match_levels": {identity: "byte"}})
    assert data["symbols"][identity]["binary_state"]["linked_byte"]["result"] == "passed"


def _exercise_commutative_stack_argument_loads(compare):
    first = "d94104 d84a08 8b442404 d918 c3"
    second = "d94208 d84904 8b442404 d918 c3"
    proof = compare(first, second)
    assert proof["passed"] and proof["conditional"]
    assert not proof["accepts_function_match"] and not proof["accepts_exact_bytes"]
    assert proof["regions"][0]["exact_stack_argument_loads"] == [{
        "offset": 6, "destination": "eax", "width": 4,
        "base": "esp-region-entry", "segment": "ss", "bytes": "8b442404",
        "esp_displacement": 4,
        "precondition": "valid ordinary stable ABI stack argument memory"}]
    assert "memory_accesses" in proof["contract"]
    old_contract = dict(proof["contract"])
    del old_contract["memory_accesses"]
    assert not compare(first, second, contract=old_contract)["passed"]
    for changed in ("8b442408", "8b5c2404", "668b442404"):
        assert not compare(first, second.replace("8b442404", changed))["passed"]
    # Unsupported effects remain blocked even when byte-identical on both sides.
    for operation in ("89442404", "8b4304", "8b447404", "648b442404", "668b442404",
                      "678b442404", "8b642404", "8b6c2404", "8b442405",
                      "8b0424", "8b4c2404", "83c404"):
        assert not compare(first.replace("8b442404", operation),
                           second.replace("8b442404", operation))["passed"], operation
    rejected = [
        ("d94104 8b442404 d84a08 d918 c3", "d94208 8b442404 d84904 d918 c3"),
        (first.replace("d918", "d84004 d918"), second.replace("d918", "d84004 d918")),
        (first.replace("d918", "d94004 dec1 d918"), second.replace("d918", "d94004 dec1 d918")),
        ("d9430c " + first.replace("d918", "d9c9 d84804 dec1 d918"),
         "d9430c " + second.replace("d918", "d9c9 d84804 dec1 d918")),
        ("d94104 d84a08 d9448604 dec1 8b742404 d918 c3",
         "d94208 d84904 d9448604 dec1 8b742404 d918 c3"),
        (first.replace("d918", "8b5c2408 d918"), second.replace("d918", "8b5c2408 d918")),
        ("7406 " + first, "7406 " + second),  # interior entry at the MOV
        # The unequal factor is consumed by the addition, not left untouched.
        ("d94104 d94304 dec1 d84a08 8b442404 d918 c3",
         "d94208 d94304 dec1 d84904 8b442404 d918 c3"),
        ("d94104 d84304 d84a08 8b442404 d918 c3",
         "d94208 d84304 d84904 8b442404 d918 c3"),
        # FADD m32 retains its conservative whole-tracked-stack condition.
        ("d94104 d94304 d84308 d9c9 d84a08 dec1 8b442404 d918 c3",
         "d94208 d94304 d84308 d9c9 d84904 dec1 8b442404 d918 c3"),
    ]
    for a, b in rejected:
        assert not compare(a, b)["passed"], (a, b)
    assert compare("d9430c " + first.replace("d918", "dec1 d918"),
                   "d9430c " + second.replace("d918", "dec1 d918"))["passed"]
    assert compare(first[:-2] + first, second[:-2] + second)["passed"]
    # FADDP reads two equal top operands while the bottom factor stays unequal.
    # That pending value must survive until its own later FMUL establishes equality.
    assert compare("d94104 d94304 d94308 dec1 d9c9 d84a08 dec1 8b442404 d918 c3",
                   "d94208 d94304 d94308 dec1 d9c9 d84904 dec1 8b442404 d918 c3")["passed"]


def test_commutative_match_is_narrow_conditional_and_separate_from_bytes(tmp_path, monkeypatch):
    from copy import deepcopy
    from _recoil.lib.commutative_match import compare_commutative, COMMUTATIVE_CONTRACT, COMMUTATIVE_VERSION
    from _recoil.lib.function_match import MATCH_VERSION, weakest_match_level
    from _recoil.lib import match_evidence as evidence
    from _recoil.lib.source_traceability import parse_source_trace_text
    from _recoil.lib.progress import is_current_accepted_state, ProgressError, AUTHORED_BYTE_DIMENSIONS, EXACT_LINK_DIMENSIONS
    from _recoil.commands.progress_v2 import accept_live_byte_groups

    def compare(a, b, **kwargs):
        return compare_commutative(bytes.fromhex(a), bytes.fromhex(b), function_address=0x600000,
                                   contract=kwargs.pop("contract", COMMUTATIVE_CONTRACT), **kwargs)

    _exercise_commutative_stack_argument_loads(compare)
    first = "d94004 d84a08 d91f c3"
    second = "d94208 d84804 d91f c3"
    proof = compare(first, second)
    assert proof["passed"] and proof["conditional"] and len(proof["exchanges"]) == 1
    assert proof["scope"] == "normalized-function-body-only"
    assert not proof["accepts_function_match"] and not proof["accepts_exact_bytes"]
    assert proof["pending_obligations"] and not proof["runtime_contract_proven"]
    assert proof["regions"] == [{"start": 0, "end_exclusive": 8, "extra_stack_depth": 1}]
    assert compare(first, first)["exact"]
    assert not compare(first, first)["accepts_exact_bytes"]  # equality of supplied buffers only
    # Calls before the function's zero-based decoder origin wrap to unsigned
    # x86 targets in Capstone; the independent signed relative target agrees.
    assert compare("e8f6ffffff " + first, "e8f6ffffff " + second)["passed"]
    # Pro's concrete counterexample: LOOP re-enters FMUL without its originating
    # FLD. First iteration stores 6 in both; second stores 15 versus 10.
    for branch in ("e0", "e1", "e2", "e3"):
        a = "d906 d900 d80a d91b " + branch + "fa c3"
        b = "d906 d902 d808 d91b " + branch + "fa c3"
        assert not compare(a, b)["passed"], branch
    # A loop that repeats the entire balanced region remains admissible.
    assert compare("d906 d900 d80a d91b e2f8 c3", "d906 d902 d808 d91b e2f8 c3")["passed"]
    for contract in (None, {}, {**COMMUTATIVE_CONTRACT, "inputs": "any floating-point bit pattern"}):
        assert not compare(first, second, contract=contract)["passed"]
    # Interleaved loads/FXCH must follow values, not merely compare mnemonics.
    dot = "d94004 d9420c d84b08 d9c9 d84e10 dec1 d91f c3"
    swapped = "d94004 d94308 d84a0c d9c9 d84e10 dec1 d91f c3"
    assert compare(dot, swapped)["passed"]
    rejected = [
        (first, "d94208 d84808 d91f c3"),  # different factor
        (first, "d94208 d84804 d91e c3"),  # store target
        (first, "dd4208 dc4804 d91f c3"),  # m64 instead of m32
        (first, "d94208 d84004 d91f c3"),  # addition instead of multiplication
        (dot, swapped.replace("d9c9", "d9ca")),  # stack operation changed
        (dot, swapped.replace("dec1", "dec2")),  # addition grouping changed
        (first + "90", second + "cc"),  # trailing byte cannot be relaxed
        ("31c0 " + first, "31c9 " + second),  # mixed unproved GPR reassignment
        ("d94004 40 d84a08 d91f c3", "d94208 40 d84804 d91f c3"),  # address clobber
        ("d94004 8900 d84a08 d91f c3", "d94208 8900 d84804 d91f c3"),  # aliased store
        ("d94004 e800100000 d84a08 d91f c3", "d94208 e800100000 d84804 d91f c3"),  # call
        ("7403 " + first, "7403 " + second),  # branch enters after load
        ("ff20 " + first, "ff20 " + second),  # unproved indirect jump
        ("ff10 " + first, "ff10 " + second),  # unproved indirect call
        (first[:-2] + "dfe0 c3", second[:-2] + "dfe0 c3"),  # reads status word
        (first[:-2] + "d92f c3", second[:-2] + "d92f c3"),  # changes control word
        (first[:-2] + "0f7ec0 c3", second[:-2] + "0f7ec0 c3"),  # reads aliased x87/MMX bits
        ("d94004 d8ca d91f c3", "d94208 d8ca d91f c3"),  # unproved entry ST(2)
    ]
    for a, b in rejected:
        assert not compare(a, b)["passed"], (a, b)
    # Same code behind an independently bounded retail switch; its entries
    # remain exact, even when the function otherwise qualifies.
    base = 0x600000
    prefix = bytes.fromhex("83f801 7710 ff2485") + struct.pack("<I", base + 24)
    code = prefix + bytes.fromhex(first) + bytes.fromhex("c3 9090")
    assert len(code) == 24
    code += struct.pack("<II", base + 12, base + 21)
    candidate = bytearray(code); candidate[12:21] = bytes.fromhex(second)
    table_proof = compare_commutative(code, bytes(candidate), function_address=base, contract=COMMUTATIVE_CONTRACT)
    assert table_proof["passed"], table_proof
    candidate[-4:] = struct.pack("<I", base + 12)
    assert not compare_commutative(code, bytes(candidate), function_address=base, contract=COMMUTATIVE_CONTRACT)["passed"]

    identity = "recoil:function:0x401000"
    text = ("/**\r\n * @recoil-anchor recoil:anchor:unit\r\n * @recoil-artifact defines .text "
            + identity + ": Unit.\r\n *\r\n *\r\n * Purpose: unit.\r\n */\r\nvoid f() {}\r\n")
    path = tmp_path / "unit.cpp"; path.write_bytes(text.encode())
    monkeypatch.setattr(evidence, "REPO_ROOT", tmp_path)
    context = {"source": "unit.cpp"}
    review = {"version": MATCH_VERSION, "commutative_version": COMMUTATIVE_VERSION,
              "evidence_id": "review", "decision": "compiler-commutative-operand-selection-only",
              "no_remaining_credible_source_options": True, "context": context,
              "differences": proof["differences"], "contract": deepcopy(COMMUTATIVE_CONTRACT),
              "contract_justification": "ordinary finite inputs; FP diagnostics are outside the reviewed observation domain"}
    state = {"version": MATCH_VERSION, "commutative_version": COMMUTATIVE_VERSION,
             "level": "commutative", "validation_mode": "live", "freshness": "current",
             "evidence_ids": ["proof"], "review_evidence_id": "review",
             "dependencies": evidence.dependency_states(["unit.cpp"])}
    row = {"function_match": state, "commutative_match_review": review}
    assert evidence.current_match_level(row) == "commutative"
    assert evidence.review_current(review, context, proof["differences"], level="commutative")
    assert not evidence.review_current(review, context, proof["differences"])  # never a register-only proof
    for field, value in (("contract", {}), ("contract_justification", ""), ("commutative_version", 0),
                         ("no_remaining_credible_source_options", False), ("evidence_id", "")):
        bad = deepcopy(row); bad["commutative_match_review"][field] = value
        assert evidence.current_match_level(bad) is None
    assert weakest_match_level(["byte", "instruction", "commutative"]) == "commutative"
    assert weakest_match_level(["commutative", None]) is None
    edits, _ = evidence.annotation_edits([parse_source_trace_text(text, path="unit.cpp")], {identity: row})
    tagged = edits[0]["after"]
    assert b" * @recoil-match commutative\r\n *\r\n * Purpose:" in tagged
    assert tagged.count(b"\n") == text.count("\n")
    parsed = parse_source_trace_text(tagged.decode(), path="unit.cpp")
    assert not parsed.findings and parsed.matches[0].level == "commutative"
    data = {"symbols": {identity: deepcopy(row)}}
    obsolete_exact = deepcopy(row)
    obsolete_exact["binary_state"] = {dimension: {"result": "passed", "disposition": "accepted",
        "freshness": "current", "validation_mode": "live", "evidence_ids": ["older-proof"]}
        for dimension in (*AUTHORED_BYTE_DIMENSIONS, *EXACT_LINK_DIMENSIONS)}
    for mode in ("authored", "linked"):
        assert not evidence.stage_match_current(obsolete_exact, mode, is_current_accepted_state)
    for mode in ("authored", "linked"):
        accept_live_byte_groups(data, mode=mode, groups=[[identity]], evidence_id="proof",
            facts={"validation_mode": "live", "mode": mode, "match_levels": {identity: "commutative"}})
        assert evidence.stage_match_current(data["symbols"][identity], mode, is_current_accepted_state)
    binary = data["symbols"][identity]["binary_state"]
    assert all(binary[x]["result"] == "failed" for x in ("object_byte", "linked_body_byte", "linked_byte"))
    assert not any(x.endswith("instruction") for x in binary)
    data["symbols"][identity]["commutative_match_review"]["contract"] = {}
    assert not evidence.stage_match_current(data["symbols"][identity], "linked", is_current_accepted_state)
    with pytest.raises(ProgressError):
        accept_live_byte_groups(data, mode="linked", groups=[[identity]], evidence_id="proof",
            facts={"match_levels": {identity: "commutative"}})
    # Captured Pro advice grants eligibility only. A receipt, positive answer,
    # transcript inclusion and the exact current compiler/source context are
    # all independently required by the command, before any live proof.
    import json
    from types import SimpleNamespace as Row
    from _recoil.commands import match_progress, live_byte_verify
    monkeypatch.setattr(match_progress, "REPO_ROOT", tmp_path)
    monkeypatch.setattr(match_progress, "source_context", lambda *a: context)
    monkeypatch.setattr(match_progress, "add_live_evidence", lambda *a, **k: "new-review")
    monkeypatch.setattr(live_byte_verify, "_rows", lambda *a: [{}])
    monkeypatch.setattr(live_byte_verify, "_bindings", lambda *a: [])
    monkeypatch.setattr(live_byte_verify, "_select_bindings", lambda *a: [Row(source_from="unit.cpp")])
    review_row = {**deepcopy(row), "pipeline_class": "authored", "address": "0x401000"}
    document = Row(collection=lambda name: {identity: review_row})
    def mutate_review(transform, **kwargs):
        preview = {"symbols": {identity: deepcopy(review_row)}}
        transform(preview)
        updated = preview["symbols"][identity]
        assert updated["function_match"]["freshness"] == "changed"
        assert "binary_state" not in updated
        assert updated["commutative_match_review"]["evidence_id"] == "new-review"
        return Row(to_dict=lambda: {"applied": False})
    store = Row(mutate=mutate_review)
    payload = {"symbol_id": identity, "reviewed": True, "decision": review["decision"],
               "no_remaining_credible_source_options": True, "reason": "reviewed compiler operand selection",
               "attempts": ["credible C++ forms failed"], "differences": proof["differences"],
               "source_context": context, "contract": COMMUTATIVE_CONTRACT,
               "contract_justification": review["contract_justification"],
               **{key: key + ".txt" for key in ("prompt", "answer", "transcript", "receipt")}}
    artifacts = {"prompt": "Detailed source/compiler context and failed variants.",
                 "answer": "COMMUTATIVE_MATCH_APPROVED\n", "transcript": "COMMUTATIVE_MATCH_APPROVED\n",
                 "receipt": json.dumps({"submission": {"status": "confirmed"}})}
    args = Row(operation="review-commutative", payload_file=tmp_path / "review.json", expected_revision=1, apply=False)
    def review_command(values=payload, exchange=artifacts):
        args.payload_file.write_text(json.dumps(values))
        for key, value in exchange.items():
            (tmp_path / payload[key]).write_text(value)
        return match_progress._review(args, document, store)
    assert review_command()["advisory_only"]
    for key, value in (("receipt", json.dumps({"submission": {"status": "unknown"}})),
                       ("answer", "INSTRUCTION_MATCH_APPROVED\n"), ("transcript", "missing answer"),
                       ("answer", "COMMUTATIVE_MATCH_APPROVED\nCOMMUTATIVE_MATCH_NOT_APPROVED\n")):
        with pytest.raises(ProgressError):
            review_command(exchange={**artifacts, key: value})
    for key, value in (("source_context", {}), ("contract", {}), ("contract_justification", ""),
                       ("reviewed", False), ("no_remaining_credible_source_options", False)):
        with pytest.raises(ProgressError):
            review_command(values={**payload, key: value})


def test_retail_decoder_proves_remapped_and_direct_tables_in_one_trailing_island():
    from _recoil.commands.relocation_expectations import (
        _bound_preserved_between, _decode_one, decode_x86_operand_sites,
    )

    base, code = mapped_switch_bytes()
    # VC5 may schedule callee-save PUSH instructions after CMP. Their stack
    # writes preserve the bound and flags, except when ESP itself is bounded.
    for register in range(8):
        pushed = code[:3] + bytes([0x50 + register]) + b"\x90" * 7 + code[11:]
        sites, unresolved = decode_x86_operand_sites(pushed, function_address=base)
        assert not unresolved
        assert [site.offset for site in sites if site.kind == "switch-table-entry"] == [56, 60, 64, 72, 76]
        push = bytes([0x50 + register])
        assert not _bound_preserved_between(push, [_decode_one(push, 0)], 4)
    # The last RET may already end at the aligned table boundary.
    adjacent = bytearray(code[:52] + code[56:])
    for field, target in ((17, 64), (24, 52), (36, 68)):
        struct.pack_into("<I", adjacent, field, base + target)
    sites, unresolved = decode_x86_operand_sites(bytes(adjacent), function_address=base)
    assert not unresolved
    assert [site.offset for site in sites if site.kind == "switch-table-entry"] == [52, 56, 60, 68, 72]
    for opcode in (0x89, 0x8B):
        for register in range(8):
            aligned = code[:52] + bytes((opcode, 0xC0 + register * 9, 0x90, 0x90)) + code[56:]
            sites, unresolved = decode_x86_operand_sites(aligned, function_address=base)
            assert not unresolved
            assert [site.offset for site in sites if site.kind == "switch-table-entry"] == [56, 60, 64, 72, 76]
    for tail in (b"", b"\x90\x90"):
        sites, unresolved = decode_x86_operand_sites(code + tail, function_address=base)
        assert not unresolved
        assert [site.offset for site in sites if site.kind == "switch-table-entry"] == [56, 60, 64, 72, 76]
        assert [site.offset for site in sites if site.kind == "absolute32"] == [17, 24, 36]
        assert all(site.relocation_type == 6 for site in sites)
        assert len(sites) == len({(site.offset, site.relocation_type) for site in sites})


def test_retail_decoder_rejects_unproven_remap_flow_extents_and_targets():
    from _recoil.commands.relocation_expectations import decode_x86_operand_sites

    base, code = mapped_switch_bytes()
    mutations = (
        (13, b"\x90\x90"),  # Upper bits of the dispatch index are not cleared.
        (1, b"\xfb"),  # The bound applies to a different register.
        (3, bytes.fromhex("8b 44 24 04 90 90 90 90")),  # Bounded index is overwritten.
        (3, bytes.fromhex("83 c0 01 90 90 90 90 90")),  # Bound flags are clobbered.
        (3, b"\x58" + b"\x90" * 7),  # POP overwrites the bounded EAX.
        (3, b"\x9d" + b"\x90" * 7),  # POPF overwrites the comparison flags.
        (3, b"\x66\x57" + b"\x90" * 6),  # Operand-size variants remain unproved.
        (68, b"\x03"),  # Map selects an entry outside the pointer table.
        (56, struct.pack("<I", base + 41)),  # Target is inside an instruction.
        (56, struct.pack("<I", base + 52)),  # Target is alignment before the data island.
        (36, struct.pack("<I", base + 68)),  # The second table overlaps the map.
        (52, b"\x40"),  # Alignment is executable work, not proven padding.
        (52, bytes.fromhex("8b f7")),  # Different registers change state.
        (52, bytes.fromhex("8b 3f")),  # Memory MOV can fault.
        (52, bytes.fromhex("66 8b ff")),  # Not the unprefixed VC5 alignment form.
        (52, bytes.fromhex("8a ff")),  # Partial-register MOV is not accepted.
        (40, b"\xe9" + struct.pack("<i", 15 - 45)),  # An incoming branch bypasses the bound.
        (80, b"\xcc"),  # Unproven data follows the final table.
    )
    for offset, replacement in mutations:
        changed = code[:offset] + replacement + code[offset + len(replacement):]
        _, unresolved = decode_x86_operand_sites(changed, function_address=base)
        assert unresolved, (offset, replacement.hex())


def test_retail_derivation_screens_arithmetic_lea_but_retains_absolute_symbol_references(tmp_path, monkeypatch):
    from types import SimpleNamespace as Row
    from _recoil.commands import relocation_expectations as relocation

    _exercise_registered_byte_selectors(monkeypatch)
    reference = tmp_path / "retail.exe"
    reference.write_bytes(b"retail fixture")
    # Arithmetic LEA displacements are not necessarily image addresses. The
    # final LEA does reference a registered image object.
    code = bytearray.fromhex(
        "8d 04 cd 08 00 00 00 8d 04 cd 00 20 40 00 c3"
    )
    target = relocation.TargetIdentity("recoil:data:0x402000", 0x402000, 0x402010, ("_table",), "registered-vc5-target")
    document = Row(collection=lambda name: {})
    monkeypatch.setattr(relocation, "parse_pe_headers", lambda *args, **kw: Row(image_base=0x400000, size_of_image=0x10000))
    monkeypatch.setattr(relocation, "_pe_bytes", lambda *args: bytes(code))
    monkeypatch.setattr(relocation, "build_target_identity_state", lambda *args, **kw: ((target,), []))

    def derive(**kwargs):
        return relocation.derive_relocation_expectations(document=document,
            row=dict(address="0x401000", end_exclusive=hex(0x401000+len(code)), scope_ids=["recoil:function:0x401000"]),
            object_symbol="_entry", bindings={}, reference=reference, **kwargs)

    report = derive()
    assert report["passed"]
    assert [(x["offset"], x["target_symbol"]) for x in report["expectations"]] == [(len(code)-5, "_table")]
    state = ((target,), [])
    with monkeypatch.context() as patch:
        def reject_rebuild(*args, **kwargs):
            raise AssertionError("target index rebuilt")
        patch.setattr(relocation, "build_target_identity_state", reject_rebuild)
        assert derive(target_identity_state=state) == report
        with pytest.raises(AssertionError, match="target index rebuilt"):
            derive()
        other = dict(source_symbol_id="recoil:function:0x409000", kind="stale-other")
        current = dict(source_symbol_id="recoil:function:0x401000", kind="stale-current")
        state[1].extend([other, current])
        blocked = derive(target_identity_state=state)
        assert not blocked["passed"] and blocked["unresolved"] == [current]
        assert state[1] == [other, current]
    code[-5:-1] = struct.pack("<I", 0x403000)
    assert derive()["unresolved"][0]["kind"] == "missing-target-identity"
    code[:] = bytes.fromhex("8b 05 00 00 00 00 c3")
    assert derive()["unresolved"][0]["kind"] == "missing-target-identity"
    # A segmented zero may still carry a COFF relocation to an absolute symbol
    # (VC5's __except_list). Segment prefixes must not erase that requirement.
    code[:] = bytes.fromhex("64 a1 00 00 00 00 64 89 25 00 00 00 00 c3")
    assert len(derive()["unresolved"]) == 2
    # Register-relative disp32 can index an image object. Cover a plain
    # ModRM operand, SIB, two-byte opcode and x87, including interior addends.
    # Ordinary positive field and negative frame offsets must stay excluded.
    code[:] = bytes.fromhex(
        "8b 80 01 20 40 00 8b 84 8b 02 20 40 00 "
        "0f b6 88 03 20 40 00 d8 80 04 20 40 00 "
        "8b 80 34 12 00 00 8b 85 00 ff ff ff "
        "0f b6 88 10 00 00 00 8b 84 8b 10 00 00 00 c3"
    )
    indexed = derive()
    assert indexed["passed"], indexed
    assert [(x["offset"], x["target_symbol"], x["coff_addend"])
            for x in indexed["expectations"]] == [
        (2, "_table", 1), (9, "_table", 2),
        (16, "_table", 3), (22, "_table", 4),
    ]
    code[2:6] = struct.pack("<I", 0x403000)
    assert derive()["unresolved"][0]["kind"] == "missing-target-identity"
    _exercise_native_eh_role_witnesses(monkeypatch)
    _exercise_native_eh_source_refresh()
    _exercise_native_array_cleanup(monkeypatch)
    _exercise_native_array_destruction(monkeypatch)
    _exercise_native_import_references(monkeypatch)
    _exercise_native_import_owner_context(monkeypatch)
    _exercise_native_import_source_refresh()


def test_reviewed_one_past_end_pointer_keeps_array_identity(tmp_path, monkeypatch):
    from copy import deepcopy
    from types import SimpleNamespace as Row
    from _recoil.commands import relocation_expectations as expected
    from _recoil.commands import relocation_expectation_mutation as mutation

    reference = tmp_path / "retail.exe"
    reference.write_bytes(b"immutable retail fixture")
    base, array, end = 0x600000, 0x601000, 0x601010
    source_id, target_id = "recoil:function:0x600000", "recoil:data:0x601000"
    code = bytearray(b"\xbe" + struct.pack("<I", array)
                     + b"\x81\xfe" + struct.pack("<I", end) + b"\xc3")
    source = dict(binary="recoil", kind="function", address=hex(base),
                  end_exclusive=hex(base+len(code)), object_symbol="_iterate", scope_ids=[source_id])
    target = dict(binary="recoil", kind="data", address=hex(array), end_exclusive=hex(end),
                  size=16, extent_state="known", object_symbol="_items")
    symbols = {source_id: source, target_id: target}
    document = Row(collection=lambda name: symbols if name == "symbols" else {"proof": {}})
    monkeypatch.setattr(expected, "parse_pe_headers", lambda *a, **k: Row(image_base=base, size_of_image=0x10000))
    monkeypatch.setattr(expected, "_pe_bytes", lambda image, headers, address, size: bytes(code[address-base:address-base+size]))
    identities = (expected.TargetIdentity(target_id, array, end, ("_items",), "registered-vc5-target"),
                  expected.TargetIdentity("recoil:data:0x601010", end, end+16, ("_next",), "registered-vc5-target"))
    monkeypatch.setattr(expected, "build_target_identity_state", lambda *a, **k: (identities, []))
    payload = dict(reviewed=True, object_symbol="_iterate", offset=7, type=6,
                   target_symbol="_items", target_symbol_id=target_id, coff_addend=16,
                   resolved_target_addend=16, retail_target=end, evidence_ids=["proof"],
                   reason="Reviewed end pointer for the existing array.")

    def prepare(request=None):
        return mutation.prepare_reviewed_exception(document=document, bindings={}, source_symbol_id=source_id,
            source_address=hex(base), payload=payload if request is None else request, reference=reference)

    def derive():
        return expected.derive_relocation_expectations(document=document, bindings={}, row={**source, "physical_rows": [source]},
            object_symbol="_iterate", reference=reference)

    # Ordinary lookup remains half-open and chooses the neighboring object.
    assert derive()["expectations"][-1]["target_symbol"] == "_next"
    reviewed, decoded = prepare()
    assert decoded["retail_target"] == end
    source["relocation_expectation_exceptions"] = [reviewed]
    result = derive()
    assert result["passed"], result
    bound = result["expectations"][-1]
    assert (bound["target_symbol"], bound["coff_addend"], bound["resolved_target_addend"]) == ("_items", 16, 16)
    for change in ({"coff_addend": 15}, {"resolved_target_addend": 15}, {"reviewed": False},
                   {"retail_target": end+1}, {"type": 20}):
        with pytest.raises(mutation.RelocationExceptionMutationError):
            prepare({**payload, **change})
    for change in ({"kind": "function"}, {"extent_state": "unknown"}, {"size": 20}):
        saved = deepcopy(target)
        target.update(change)
        with pytest.raises(mutation.RelocationExceptionMutationError):
            prepare()
        assert not derive()["passed"]  # live proof repeats the same boundary obligation
        target.clear(); target.update(saved)
    # A stale extent cannot silently become an interior reference.
    target["end_exclusive"] = hex(end+4)
    assert not derive()["passed"]
    target["end_exclusive"] = hex(end)
    original = bytes(code)
    for replacement, operand_offset in [
        (b"\xbf" + struct.pack("<I", end) + b"\xc3", 1),  # MOV reg, pointer
        (b"\x81\x3e" + struct.pack("<I", end) + b"\xc3", 2),  # CMP [reg], pointer
        (b"\x8b\x05" + struct.pack("<I", end) + b"\xc3", 2),  # dereference end
        (b"\x81\xf6" + struct.pack("<I", end) + b"\xc3", 2),  # XOR reg, pointer
    ]:
        code[:] = replacement
        source["end_exclusive"] = hex(base+len(code))
        with pytest.raises(mutation.RelocationExceptionMutationError):
            prepare({**payload, "offset": operand_offset})
    # The accumulator encoding has the same safe comparison semantics.
    code[:] = b"\x3d" + struct.pack("<I", end) + b"\xc3"
    source["end_exclusive"] = hex(base+len(code))
    assert prepare({**payload, "offset": 1})[1]["opcode"] == "3d"
    code[:] = original
    source["end_exclusive"] = hex(base+len(code))
    # A forged stored addend is rejected during live derivation, not only on write.
    reviewed["coff_addend"] = 15
    assert not derive()["passed"]


def _exercise_registered_byte_selectors(monkeypatch):
    from types import SimpleNamespace as Row
    from _recoil.commands import byte_symbol_selectors as selectors
    from _recoil.commands import relocation_expectations as expected, live_byte_verify as live

    pattern = r"_entry_(?:cdecl|fastcall)"
    function = Row(symbol="", symbol_regex=pattern)
    binding = Row(function=function, source_from="src/unit.cpp", target=Row(source_from="src/unit.cpp", name="order"))
    literal = Row(function=Row(symbol="_entry_cdecl", symbol_regex=None), source_from="src/unit.cpp", target=binding.target)
    assert selectors.object_selector(function) == selectors.PREFIX+pattern
    exact = Row(symbol="", symbol_regex=r"\?Draw@Widget@@UAEXXZ")
    assert selectors.object_selector(exact) == "?Draw@Widget@@UAEXXZ"
    for nonliteral in (r"_entry.*", r"_entry[0-9]", r"^_entry$", r"_entry\d", "_entry\\", r"_entry(?:x|y)"):
        assert selectors.object_selector(Row(symbol="", symbol_regex=nonliteral)) == selectors.PREFIX+nonliteral
    selected = selectors.registered_target_selector([binding, literal], {"_entry_cdecl"})
    assert selected == dict(symbol_regex=pattern, source_from="src/unit.cpp")
    assert selectors.registered_target_selector([binding], {"_unrelated"}) is None
    literal.source_from = "src/other.cpp"
    assert selectors.registered_target_selector([binding, literal], {"_entry_cdecl"}) is None
    literal.source_from = "src/unit.cpp"
    other = Row(function=Row(symbol="", symbol_regex="_different"), source_from="src/unit.cpp", target=binding.target)
    assert selectors.registered_target_selector([binding, other], set()) is None

    # A reviewed static stem and its exact compiler-suffix selector identify
    # the same storage. Keep the stem so the complete reader proof still runs.
    data_id = "recoil:data:0x503000"
    data_binding = Row(function=Row(symbol="_constant", symbol_regex=r"_constant\$S[0-9]+"),
                       source_from="src/unit.cpp", target=binding.target)
    reviewed = dict(reviewed=True, object_symbol="_constant",
                    binding_context={"source_binding": {"symbol_id": "recoil:function:0x401000"}})
    data_row = dict(binary="recoil", address="0x503000", end_exclusive="0x503004",
                    kind="data", ownership_state="primary-owned",
                    output_section_id="recoil:section:.rdata", relocation_target_binding=reviewed)
    data_document = Row(collection=lambda name: {data_id: data_row} if name == "symbols" else {})
    with monkeypatch.context() as patch:
        patch.setattr(expected, "relocation_target_binding_staleness", lambda item, **kw: (item, []))
        def data_state(extra=()):
            return expected.build_target_identity_state(data_document, {data_id: [data_binding, *extra]})
        state, blockers = data_state()
        assert not blockers and state[0].object_symbols == ("_constant",)
        assert state[0].registered_selector is None
        assert "reviewed-relocation-target-binding" in state[0].source
        for target, field, value in (
            (data_row, "kind", "function"), (data_row, "ownership_state", "unresolved"),
            (data_row, "output_section_id", "recoil:section:.data"),
            (reviewed, "object_symbol", "_other"), (reviewed, "reviewed", False)):
            with monkeypatch.context() as changed:
                changed.setitem(target, field, value)
                state, _ = data_state()
                assert state[0].object_symbols != ("_constant",)
        for field, value in (("symbol_regex", r"_constant.*"),
                             ("symbol_regex", r"_other\$S[0-9]+"), ("symbol", "_other"),
                             ("object_offset", 4)):
            with monkeypatch.context() as changed:
                changed.setattr(data_binding.function, field, value, raising=False)
                state, blockers = data_state()
                assert blockers or state[0].object_symbols != ("_constant",)
        other_tu = Row(function=data_binding.function, source_from="src/other.cpp", target=binding.target)
        assert data_state([other_tu])[0][0].object_symbols != ("_constant",)
        patch.setattr(expected, "relocation_target_binding_staleness",
                      lambda item, **kw: (item, [{"field": "source_binding"}]))
        state, blockers = data_state()
        assert blockers and state[0].object_symbols != ("_constant",)
        patch.setattr(expected, "relocation_target_binding_staleness", lambda item, **kw: (item, []))
        # Older data rows record primary ownership only in the reviewed
        # relationship, without a duplicate ownership_state field.
        patch.delitem(data_row, "ownership_state")
        context = reviewed["binding_context"]
        patch.setitem(context, "owner", {"owner_id": "recoil:owner:unit"})
        patch.setitem(context, "target", {**{k:data_row.get(k) for k in
            ("kind", "address", "end_exclusive", "output_section_id", "ownership_state")},
            "symbol_id": data_id, "object_symbol": "_constant"})
        patch.setitem(context, "relationship", {"kind":"primary-data", "symbol_id":data_id,
                                               "address":data_row["address"]})
        assert data_state()[0][0].object_symbols == ("_constant",)
        for target, field, value in ((context["owner"], "owner_id", ""),
            (context["relationship"], "kind", "primary-function"),
            (context["relationship"], "address", "0x503004"),
            (context["target"], "end_exclusive", "0x503008")):
            with monkeypatch.context() as changed:
                changed.setitem(target, field, value)
                assert data_state()[0][0].object_symbols != ("_constant",)
        patch.setattr(expected, "relocation_target_binding_staleness",
                      lambda item, **kw: (item, [{"field":"relationship"}]))
        assert data_state()[1]

    symbol_id = "recoil:function:0x402000"
    row = dict(binary="recoil", address="0x402000", end_exclusive="0x402010", kind="function")
    collections = dict(symbols={symbol_id: row}, owners={}, evidence={})
    document = Row(collection=collections.__getitem__)
    state, blockers = expected.build_target_identity_state(document, {symbol_id: [binding, literal]})
    assert not blockers and len(state) == 1
    assert state[0].object_symbols == (selectors.PREFIX+pattern,)
    assert state[0].registered_selector == (pattern, "src/unit.cpp", "function")
    snapshot = expected.build_object_binding_snapshot(document, {symbol_id: [binding]},
        symbol_id=symbol_id, object_symbol=selectors.object_selector(function))
    assert snapshot["object_symbol"] == selectors.object_selector(function)
    exact_binding = Row(function=exact, source_from="src/unit.cpp", target=binding.target)
    exact_snapshot = expected.build_object_binding_snapshot(document, {symbol_id: [exact_binding]},
        symbol_id=symbol_id, object_symbol="?Draw@Widget@@UAEXXZ")
    assert exact_snapshot["object_symbol"] == "?Draw@Widget@@UAEXXZ"
    competing = expected.TargetIdentity(symbol_id, 0x402000, 0x402010,
        state[0].object_symbols, "other-tu", (pattern, "src/other.cpp", "function"))
    assert expected._resolve_identity(0x402000, [state[0], competing])[0] is None

    # A registered data slice names its containing COFF object plus an offset.
    # The retail field extent remains the only eligible resolution interval.
    field = Row(symbol="_table", symbol_regex=None, object_offset=12)
    field_binding = Row(function=field, source_from="src/unit.cpp", target=binding.target)
    with monkeypatch.context() as patch:
        patch.setitem(row, "kind", "data")
        state, blockers = expected.build_target_identity_state(document, {symbol_id: [field_binding]})
        assert not blockers and state[0].object_offset == 12
        assert expected._resolve_identity(0x402000, state)[1] == 12
        assert expected._resolve_identity(0x402004, state)[1] == 16
        assert expected._resolve_identity(0x401fff, state)[0] is None
        assert expected._resolve_identity(0x402010, state)[0] is None
        base = expected.TargetIdentity("table", 0x401ff4, 0x402010, ("_table",), "registered-vc5-target")
        assert expected._resolve_identity(0x402004, [state[0], base])[1] == 16
        wrong = expected.TargetIdentity("other", 0x402000, 0x402010, ("_table",), "registered-vc5-target")
        assert expected._resolve_identity(0x402004, [state[0], wrong])[0] is None
        for invalid in (-1, True, "12", 0x500000):
            with monkeypatch.context() as changed:
                changed.setattr(field, "object_offset", invalid)
                identities, blockers = expected.build_target_identity_state(document, {symbol_id: [field_binding]})
                assert not identities and blockers[0]["kind"] == "invalid-registered-target-offset"
        zero = Row(function=Row(symbol="_table", symbol_regex=None, object_offset=0),
                   source_from="src/unit.cpp", target=binding.target)
        identities, blockers = expected.build_target_identity_state(document, {symbol_id: [field_binding, zero]})
        assert not identities and blockers
    identities, blockers = expected.build_target_identity_state(document, {symbol_id: [field_binding]})
    assert not identities and blockers  # Nonzero slices cannot describe functions.

    symbol = Row(name="_entry_fastcall", section_number=1, storage_class=2, type=0x20, value=0)
    section = Row(characteristics=0x20, name=".text", index=1)
    obj = Row(symbols=[symbol], section=lambda index: section)
    selector = dict(selected, kind="function")
    assert selectors.resolve_target_definition(obj, selector) == symbol.name
    for field, value in (("section_number", 0), ("storage_class", 6), ("type", 0)):
        prior = getattr(symbol, field)
        setattr(symbol, field, value)
        with pytest.raises(ValueError): selectors.resolve_target_definition(obj, selector)
        setattr(symbol, field, prior)
    obj.symbols.append(Row(**dict(vars(symbol), name="_entry_cdecl")))
    with pytest.raises(ValueError, match="uniquely"): selectors.resolve_target_definition(obj, selector)
    obj.symbols.pop()
    with pytest.raises(ValueError, match="non-data"): selectors.resolve_target_definition(obj, dict(selector, kind="data"))
    section.characteristics = 0x40
    symbol.type = 0
    assert selectors.resolve_target_definition(obj, dict(selector, kind="data")) == symbol.name
    section.characteristics, symbol.type = 0x20, 0x20

    body = Row(start=4)
    relocation = Row(offset=12, type=20, symbol_name=symbol.name)
    canonical = live.CanonicalRelocationTarget(symbol.name, 4, False, "literal")
    catalog = [dict(offset=8, type=20, target_symbol=selectors.PREFIX+pattern,
                    registered_target_selector=selector)]
    def load(source):
        if source != "src/unit.cpp": raise ValueError("wrong TU")
        return obj
    def prove(): return live._canonicalize_registered_target_selectors(body, [(relocation, canonical)], catalog, load)[0][1]
    assert prove().symbol_name == selectors.PREFIX+pattern
    assert prove().registered_symbol_name == symbol.name and prove().coff_addend == 4
    selector["source_from"] = "src/wrong.cpp"
    assert not prove().registered_symbol_name
    selector["source_from"] = "src/unit.cpp"
    relocation.symbol_name = "_entry_cdecl"
    assert not prove().registered_symbol_name
    relocation.symbol_name = symbol.name
    catalog.append(catalog[0])
    assert not prove().registered_symbol_name

    # A switch label must retain its independently proved containing function
    # and body-relative addend when that function uses a registered selector.
    label = Row(name="$L42", index=1, section_number=1, storage_class=6, type=0, value=12)
    symbol.index = 0
    obj.symbols.append(label)
    obj.symbols_by_index = {0: symbol, 1: label}
    obj.function_end = lambda entry, selected: 32
    section.characteristics = 0x1020
    body = Row(start=0, end=32, natural_end=32, section_index=1,
               symbol=symbol.name, data=bytes(32))
    relocation = Row(offset=4, type=6, symbol_name=label.name, symbol_index=1)
    canonical = live._canonicalize_same_comdat_local_label(
        coff_object=obj, function_bytes=body, relocation=relocation, raw_addend=0)
    assert canonical.canonicalized and canonical.coff_addend == 12
    catalog = [dict(offset=4, type=6, target_symbol=selectors.PREFIX+pattern,
                    registered_target_selector=selector)]
    def prove_local(definition=None):
        return live._canonicalize_registered_target_selectors(
            body, [(relocation, canonical)], catalog,
            lambda source: obj if definition is None else definition,
            coff_object=obj)[0][1]
    matched = prove_local()
    assert matched.symbol_name == selectors.PREFIX+pattern
    assert matched.registered_symbol_name == symbol.name and matched.coff_addend == 12
    assert not prove_local(Row(**vars(obj))).registered_symbol_name
    for target, attribute, value in (
        (label, "storage_class", 3), (label, "section_number", 2),
        (label, "value", 32), (section, "characteristics", 0x20),
        (body, "symbol", "_other"), (body, "data", bytes(4)),
        (relocation, "symbol_index", 0), (relocation, "type", 20),
    ):
        with monkeypatch.context() as patch:
            patch.setattr(target, attribute, value)
            assert not prove_local().registered_symbol_name
    canonical = live.CanonicalRelocationTarget(symbol.name, 13, True, "same-comdat-local-label")
    assert not prove_local().registered_symbol_name


def _exercise_native_import_references(monkeypatch):
    from types import SimpleNamespace as Row
    from copy import deepcopy
    from _recoil.commands import native_import_relocations as imports, live_byte_verify as live
    from _recoil.commands import provider_target_mutation as iat

    def symbol(index, name, section, *, type=0, storage=3, aux=0):
        return Row(index=index, name=name, section_number=section, value=0,
                   type=type, storage_class=storage, aux_count=aux)
    symbols = [symbol(0, "_convert", 1, type=32, storage=2),
               symbol(1, "__imp__convert", 2, storage=2), symbol(2, ".idata$6", 4),
               symbol(3, "__IMPORT_DESCRIPTOR_MSVCRT", 0, storage=2)]
    sections = [Row(index=1, name=".text", raw_data=b"\xff\x25"+bytes(4), characteristics=0x60001020),
                Row(index=2, name=".idata$5", raw_data=bytes(4)),
                Row(index=3, name=".idata$4", raw_data=bytes(4)),
                Row(index=4, name=".idata$6", raw_data=b"\0\0convert\0")]
    refs = {1: [Row(offset=2, type=6, symbol_index=1)],
            2: [Row(offset=0, type=7, symbol_index=2)], 3: [Row(offset=0, type=7, symbol_index=2)]}
    obj = Row(symbols=symbols, symbols_by_index={s.index:s for s in symbols}, sections=sections,
              section=lambda index: sections[index-1], relocations_by_section=refs)
    assert imports.prove_import_member(obj, "_convert", "convert")["iat_symbol"] == "__imp__convert"
    for target, attr, value in [(symbols[0], "type", 0), (symbols[0], "value", 1),
        (symbols[1], "section_number", 0), (symbols[1], "storage_class", 3),
        (refs[1][0], "type", 20), (refs[1][0], "offset", 1), (refs[1][0], "symbol_index", 2),
        (refs[2][0], "type", 6), (refs[3][0], "symbol_index", 1),
        (sections[3], "raw_data", b"\0\0other\0"), (sections[1], "raw_data", b"\1\0\0\0"),
        (sections[0], "raw_data", b"\xff\x25\1\0\0\0"),
        (symbols[3], "section_number", 1)]:
        with monkeypatch.context() as patch:
            patch.setattr(target, attr, value)
            with pytest.raises(ValueError): imports.prove_import_member(obj, "_convert", "convert")
    obj.symbols.append(deepcopy(symbols[0]))
    with pytest.raises(ValueError, match="ambiguous"): imports.prove_import_member(obj, "_convert", "convert")
    obj.symbols.pop()

    # Ordinal import members have two literal ordinal words, no name section,
    # and no lookup relocations. Keep their proof distinct from named imports.
    with monkeypatch.context() as patch:
        patch.setattr(obj, "sections", sections[:3])
        patch.setattr(obj, "symbols", [symbols[0], symbols[1], symbols[3]])
        patch.setattr(obj, "relocations_by_section", {1: refs[1]})
        patch.setattr(symbols[3], "name", "__IMPORT_DESCRIPTOR_MFC42")
        for section in sections[1:3]:
            patch.setattr(section, "raw_data", struct.pack("<I", 0x8000002a))
        def ordinal_proof(ordinal=42, name="#42"):
            return imports.prove_import_member(obj, "_convert", name, import_ordinal=ordinal,
                                                descriptor_name="__IMPORT_DESCRIPTOR_MFC42")
        assert ordinal_proof()["import_ordinal"] == 42
        for ordinal, name in ((43, "#43"), (-1, "#-1"), (65536, "#65536"),
                              (True, "#True"), (42, "convert")):
            with pytest.raises(ValueError): ordinal_proof(ordinal, name)
        for target, attr, value in [(sections[1], "raw_data", struct.pack("<I", 42)),
            (sections[2], "raw_data", struct.pack("<I", 0x8000002b)),
            (sections[1], "raw_data", struct.pack("<I", 0x8000002a)+bytes(4)),
            (obj, "sections", sections), (symbols[3], "name", "__IMPORT_DESCRIPTOR_OTHER"),
            (obj, "relocations_by_section", {1: refs[1], 2: refs[2]}),
            (obj, "relocations_by_section", {1: refs[1], 3: refs[3]}),
            (refs[1][0], "symbol_index", 2)]:
            with monkeypatch.context() as changed:
                changed.setattr(target, attr, value)
                with pytest.raises(ValueError): ordinal_proof()

    for identity in (dict(dll="OTHER.dll", name="#42", ordinal=42),
                     dict(dll="MFC42.DLL", name="convert", ordinal=None),
                     dict(dll="MSVCRT.dll", name="#42", ordinal=42),
                     dict(dll="AVIFIL32.dll", name="#42", ordinal=42)):
        with pytest.raises(ValueError, match="requires"):
            imports.canonical_import_proof("_convert", identity)

    # Named Video for Windows imports use the same strict long member proof;
    # their exact canonical archive, DLL member and descriptor still matter.
    from _recoil.commands import provider_function_mutation as archives
    video_identity = dict(dll="AVIFIL32.dll", name="convert", ordinal=None)
    member = Row(name="AVIFIL32.dll", data=b"fixture _convert")
    read_paths = []
    with monkeypatch.context() as patch:
        patch.setattr(Path, "read_bytes", lambda path: read_paths.append(path) or b"archive")
        patch.setattr(archives, "parse_archive_members", lambda data: [member])
        patch.setattr(imports.CoffObject, "from_bytes", lambda data: obj)
        patch.setattr(symbols[3], "name", "__IMPORT_DESCRIPTOR_AVIFIL32")
        proof = imports.canonical_import_proof("_convert", video_identity)
        assert proof["library"] == "VC/LIB/VFW32.LIB" and proof["member"] == "AVIFIL32.dll"
        assert read_paths == [imports.DEFAULT_VC5_ROOT / "VC/LIB/VFW32.LIB"]
        for target, field, value in ((member, "name", "OTHER.dll"),
                                    (symbols[3], "name", "__IMPORT_DESCRIPTOR_MSVCRT"),
                                    (sections[3], "raw_data", b"\0\0other\0")):
            with monkeypatch.context() as changed:
                changed.setattr(target, field, value)
                with pytest.raises(ValueError): imports.canonical_import_proof("_convert", video_identity)
        patch.setattr(archives, "parse_archive_members", lambda data: [member, member])
        with pytest.raises(ValueError, match="ambiguous"):
            imports.canonical_import_proof("_convert", video_identity)

    identity = dict(dll="MSVCRT.dll", name="convert", ordinal=None)
    table = [Row(address="0x408000", dll="MSVCRT.dll", import_name="convert", import_ordinal=None)]
    with monkeypatch.context() as patch:
        patch.setattr(live, "_pe_bytes", lambda *args: b"\xff\x25"+struct.pack("<I", 0x408000))
        patch.setattr(iat, "_retail_import_targets", lambda *args: (table, {}))
        assert imports.import_at(Path("candidate.exe"), 0x407000) == (identity, 0x408000)
        table.append(deepcopy(table[0]))
        with pytest.raises(ValueError, match="ambiguous"): imports.import_at(Path("candidate.exe"), 0x407000)
        table.pop()
        patch.setattr(live, "_pe_bytes", lambda *args: b"\xff\x15"+bytes(4))
        with pytest.raises(ValueError, match="jump"): imports.import_at(Path("candidate.exe"), 0x407000)

    external = symbol(0, "_convert", 0, storage=2, type=32)
    obj = Row(symbols_by_index={0:external})
    body = Row(start=0, data=b"\xe8"+bytes(4))
    ref = Row(type=20, offset=1, symbol_index=0, symbol_name="_convert")
    expected = dict(offset=1, target_symbol="_convert", native_import=dict(identity=identity))
    linked = Row(symbols=[Row(symbol="_convert", address=0x407000)])
    with monkeypatch.context() as patch:
        patch.setattr(imports, "import_at", lambda *args: (identity, 0x408000))
        def prove(): imports.prove_candidate_import(obj, body, ref, expected, linked, Path("candidate.exe"), 0x407000)
        prove()
        for target, attr, value in [(external, "section_number", 1), (external, "type", 0),
            (ref, "type", 6), (body, "data", b"\xe8\1\0\0\0"), (linked.symbols[0], "address", 0x407010)]:
            with monkeypatch.context() as changed:
                changed.setattr(target, attr, value)
                with pytest.raises(ValueError): prove()
        linked.symbols.append(deepcopy(linked.symbols[0]))
        with pytest.raises(ValueError, match="ambiguous"): prove()
        linked.symbols.pop()
        for wrong in (dict(identity, dll="OTHER.dll"), dict(identity, name="other"), dict(identity, ordinal=1)):
            patch.setattr(imports, "import_at", lambda *args: (wrong, 0x408000))
            with pytest.raises(ValueError, match="different DLL/import"): prove()


def _exercise_native_import_owner_context(monkeypatch):
    from copy import deepcopy
    from types import SimpleNamespace as Row
    from _recoil.commands import native_import_relocations as imports

    target_id, owner_id = "recoil:function:0x407000", "recoil:owner:provider.example"
    target = dict(address="0x407000", ownership_state="unresolved")
    owners = {}
    document = Row(collection=lambda name: owners)
    prove = lambda selected=None: imports.import_owner_context(document, target_id, target, selected)
    assert prove() is None
    with pytest.raises(ValueError): prove(owner_id)
    target["ownership_state"] = "primary-owned"
    relationship = dict(kind="primary-function", symbol_id=target_id, address=target["address"])
    owner = dict(binary="recoil", kind="provider-boundary", provider_state="accepted",
                 lifecycle_state="accepted", gates=dict(boundary="accepted", source="accepted", byte="deferred"),
                 relationships=[relationship], evidence_ids=[])
    owners[owner_id] = owner
    with pytest.raises(ValueError): prove()
    baseline = deepcopy((target, owners))
    saved = prove(owner_id)
    assert saved == dict(owner_id=owner_id, owner=owner)
    assert (target, owners) == baseline
    assert saved["owner"] is not owner
    for selected in ("other", "", True, 1):
        with pytest.raises(ValueError): prove(selected)
    for record, key, value in [
        (target, "ownership_state", "unresolved"), (owner, "binary", "messages"),
        (owner, "kind", "class"), (owner, "provider_state", "pending"),
        (owner, "lifecycle_state", "discovered"), (owner["gates"], "boundary", "pending"),
        (owner["gates"], "source", "pending"), (relationship, "symbol_id", "other"),
        (relationship, "address", "0x407010"),
        (owner, "relationships", [relationship, deepcopy(relationship)]),
    ]:
        with monkeypatch.context() as patch:
            patch.setitem(record, key, value)
            with pytest.raises(ValueError): prove(owner_id)
    owners["other"] = deepcopy(owner)
    with pytest.raises(ValueError): prove(owner_id)
    del owners["other"]
    for field in ("object_symbol", "logical_aliases", "relocation_target_binding", "provider_object_identity"):
        with monkeypatch.context() as patch:
            patch.setitem(target, field, "already-bound")
            with pytest.raises(ValueError): prove(owner_id)
    owner["evidence_ids"].append("new-provider-evidence")
    assert saved != prove(owner_id) and saved["owner"]["evidence_ids"] == []
    # Live expectation derivation must re-read the selected owner and reject
    # stale ownership snapshots; no changed context may reuse a stored proof.
    old = dict(source_binding=dict(object_symbol="_caller"), offset=1,
               canonical=dict(symbol="_convert"), evidence_ids=["source-evidence"],
               provider_owner=saved, target_id=target_id, target=0x407000)
    binding = dict(schema=imports.SCHEMA, reviewed=True, context=old)
    symbols = {"caller": {imports.FIELD: [binding]}}
    doc = Row(collection=lambda name: symbols)
    calls = []
    with monkeypatch.context() as patch:
        def context(*args):
            calls.append(args[-1])
            return dict(old, provider_owner=prove(owner_id))
        patch.setattr(imports, "binding_context", context)
        with pytest.raises(ValueError, match="stale"):
            imports.derive_import_expectations(doc, [], dict(scope_ids=["caller"]), "_caller", Path("retail.exe"))
        assert calls == [owner_id]
        owner["evidence_ids"].clear()
        assert imports.derive_import_expectations(doc, [], dict(scope_ids=["caller"]), "_caller", Path("retail.exe"))


def _exercise_native_eh_source_refresh():
    from copy import deepcopy
    from _recoil.commands import native_eh_relocations as eh

    old = dict(schema=eh.SCHEMA, reviewed=True, reason="Retail EH parent",
               context=dict(retail=dict(handler=0x402000, handler_offset=3, max_state=1),
                            runtime=dict(value=0), target=dict(address="0x402000"),
                            target_id="handler", owner_id=None, relationship=None, evidence_ids=["evidence"],
                            source=dict(symbol_id="caller", object_symbol="_caller", address="0x401000",
                                        end_exclusive="0x401030", registration_ids=["vc5:old"])))
    row = dict(unrelated="preserved")
    assert eh.stage_eh_binding(row, old) == "added"
    with pytest.raises(ValueError, match="already"): eh.stage_eh_binding(row, old)
    current = deepcopy(old)
    current["reason"] = "Corrected source selector registrations"
    current["context"]["source"]["registration_ids"] = ["vc5:old", "vc5:corrected"]
    for invalid in ({}, dict(old, reviewed=1), dict(old, schema="unknown"), dict(old, reason=""),
                    dict(old, reason="stale"), None):
        with pytest.raises(ValueError): eh.stage_eh_binding(row, current, invalid)
    with pytest.raises(ValueError, match="missing"): eh.stage_eh_binding({}, current, old)
    for field, value in (("retail", dict(handler=0x402001)), ("runtime", dict(value=1)),
                         ("target", dict(address="0x402001")), ("target_id", "other"),
                         ("owner_id", "owner"), ("relationship", {}), ("evidence_ids", ["new"]),
                         ("provider_lifecycle", dict(state="accepted")), ("unexpected", True)):
        bad = deepcopy(current)
        bad["context"][field] = value
        with pytest.raises(ValueError): eh.stage_eh_binding(row, bad, old)
    for field, value in (("symbol_id", "other"), ("object_symbol", "_other"),
                         ("address", "0x401001"), ("end_exclusive", "0x401031"), ("unexpected", True),
                         ("registration_ids", []), ("registration_ids", ["same", "same"]),
                         ("registration_ids", [None]), ("registration_ids", "bad")):
        bad = deepcopy(current)
        bad["context"]["source"][field] = value
        with pytest.raises(ValueError): eh.stage_eh_binding(row, bad, old)
    for context in (None, {}, dict(old["context"], source=None)):
        malformed = dict(old, context=context)
        with pytest.raises(ValueError): eh.stage_eh_binding({eh.FIELD:malformed}, current, malformed)
    missing = deepcopy(old)
    del missing["context"]["source"]["registration_ids"]
    with pytest.raises(ValueError): eh.stage_eh_binding({eh.FIELD:missing}, current, missing)
    with pytest.raises(ValueError, match="changed"): eh.stage_eh_binding(row, old, old)
    assert row == dict(unrelated="preserved", **{eh.FIELD:old})
    assert eh.stage_eh_binding(row, current, old) == "refreshed-source-registration"
    assert row == dict(unrelated="preserved", **{eh.FIELD:current})
    current["context"]["source"]["registration_ids"].clear()
    assert row[eh.FIELD]["context"]["source"]["registration_ids"] == ["vc5:old", "vc5:corrected"]
    assert old["context"]["source"]["registration_ids"] == ["vc5:old"]


def _exercise_native_import_source_refresh():
    from copy import deepcopy
    from _recoil.commands import native_import_relocations as imports

    old = dict(schema=imports.SCHEMA, reviewed=True, reason="Retail import",
               context=dict(offset=1, opcode="e8", target=0x407000,
                            identity=dict(dll="MSVCRT.dll", name="convert", ordinal=None),
                            evidence_ids=["evidence"], canonical=dict(symbol="_convert"),
                            source_binding=dict(symbol_id="caller", object_symbol="_caller",
                                                address="0x401000", end_exclusive="0x401010",
                                                registration_ids=["vc5:unit:source:src/old.cpp"])))
    entries = []
    assert imports.stage_import_binding(entries, old) == "added"
    with pytest.raises(ValueError, match="already"): imports.stage_import_binding(entries, old)
    current = deepcopy(old)
    current["reason"] = "Refresh current implementation registration"
    current["context"]["source_binding"]["registration_ids"] = ["vc5:unit:source:src/current.cpp"]
    bad_old = deepcopy(old)
    bad_old["reason"] = "stale"
    for expected_old in (bad_old, {}, dict(old, schema="unknown"), dict(old, reviewed=1)):
        with pytest.raises(ValueError): imports.stage_import_binding(entries, current, expected_old)
    with pytest.raises(ValueError, match="duplicated"):
        imports.stage_import_binding([old, deepcopy(old)], current, old)
    with pytest.raises(ValueError): imports.stage_import_binding([], current, old)
    for invalid in (None, {}, [None], [{}]):
        with pytest.raises(ValueError): imports.stage_import_binding(invalid, current, old)
    for field, value in (("offset", 2), ("opcode", "e9"), ("target", 0x407001),
                         ("identity", dict(dll="OTHER.dll", name="convert", ordinal=None)),
                         ("evidence_ids", ["new"]), ("canonical", dict(symbol="_other")),
                         ("provider_owner", dict(owner_id="new-owner", owner={})),
                         ("unexpected", True)):
        bad = deepcopy(current)
        bad["context"][field] = value
        with pytest.raises(ValueError): imports.stage_import_binding(entries, bad, old)
    for field, value in (("symbol_id", "other"), ("object_symbol", "_other"),
                         ("address", "0x401001"), ("end_exclusive", "0x401011"),
                         ("unexpected", True), ("registration_ids", []),
                         ("registration_ids", ["duplicate", "duplicate"]),
                         ("registration_ids", [None]), ("registration_ids", "bad")):
        bad = deepcopy(current)
        bad["context"]["source_binding"][field] = value
        with pytest.raises(ValueError): imports.stage_import_binding(entries, bad, old)
    with pytest.raises(ValueError, match="changed"): imports.stage_import_binding(entries, old, old)
    assert entries == [old]
    assert imports.stage_import_binding(entries, current, old) == "refreshed-source-registration"
    assert entries == [current] and old["context"]["source_binding"]["registration_ids"] == ["vc5:unit:source:src/old.cpp"]


def _exercise_native_array_cleanup(monkeypatch):
    from types import SimpleNamespace as Row
    from _recoil.commands import native_array_cleanup as cleanup, live_byte_verify as live
    from _recoil.lib import source_emission_markers as sources

    header = Row(repo_path="src/classes.h", text="""
struct Base { virtual ~Base() {} };
struct Element : Base { Element(); };
struct Parent { Element children[6]; };
""")
    with monkeypatch.context() as patch:
        patch.setattr(sources, "collect_source_closure", lambda *args, **kwargs: (header,))
        def generation():
            return cleanup.source_generation("src/parent.cpp", "??0Parent@@QAE@XZ", "??0Element@@QAE@XZ", "??_7Base@@6B@", 6)
        assert generation()["destructor_symbol"] == "??1Element@@UAE@XZ"
        original = header.text
        for before, after in (("Element();", "Element(); ~Element() {}"),
                ("Element : Base", "Element : virtual Base"), ("virtual ~Base() {}", "~Base() {}"),
                ("Element children[6];", "Element *children;"), ("children[6]", "children[5]"),
                ("Element children[6];", "struct Nested { Element children[6]; };")):
            header.text = original.replace(before, after)
            with pytest.raises(ValueError):
                generation()
        header.text = original

    destructor = Row(name="??1Element@@UAE@XZ", index=1, storage_class=2, type=0x20, section_number=2)
    definition = Row(section_number=2, section_definition_selection=2)
    relocation = Row(offset=4, type=6, symbol_name=destructor.name, symbol_index=1)
    constructor = Row(offset=9, type=6, symbol_name="??0Element@@QAE@XZ")
    table = Row(offset=2, type=6, symbol_name="??_7Base@@6B@")
    parent = Row(start=0, data=bytes(20), relocations=[relocation, constructor])
    body = Row(symbol=destructor.name, start=0, section_index=2,
               data=b"\xc7\x01"+bytes(4)+b"\xc3"+b"\x90"*9, relocations=[table])
    obj = Row(symbols=[destructor, definition], symbols_by_index={1: destructor},
              function_bytes=lambda name: body, section=lambda index: Row(characteristics=0x1000))
    expected = dict(offset=4, native_cleanup=dict(generation=dict(destructor_symbol=destructor.name),
        dependencies=dict(table=dict(object_symbol=table.symbol_name), constructor=dict(object_symbol=constructor.symbol_name)),
        bound_object_symbols=["??1Base@@UAE@XZ"]))
    # Definition records in real COFF also have names and a null selection on functions.
    definition.name = ".text"
    destructor.section_definition_selection = None
    assert cleanup.prove_object_cleanup(obj, parent, relocation, expected) is body
    for row, field, value in ((relocation, "symbol_name", "??1Other@@UAE@XZ"),
            (constructor, "symbol_name", "??0Other@@QAE@XZ"), (table, "symbol_name", "??_7Other@@6B@"),
            (table, "type", 20), (table, "offset", 3), (definition, "section_definition_selection", 1),
            (destructor, "storage_class", 3), (body, "data", body.data[:-1]+b"\x40")):
        old = getattr(row, field)
        setattr(row, field, value)
        with pytest.raises(ValueError):
            cleanup.prove_object_cleanup(obj, parent, relocation, expected)
        setattr(row, field, old)
    parent.data = bytes(4)+b"\x01"+bytes(15)
    with pytest.raises(ValueError, match="addend"):
        cleanup.prove_object_cleanup(obj, parent, relocation, expected)
    parent.data = bytes(20)
    mapping = Row(symbols=[Row(symbol=destructor.name, address=0x402000),
                           Row(symbol=table.symbol_name, address=0x403000),
                           Row(symbol="??1Base@@UAE@XZ", address=0x402000)])
    linked = bytearray(b"\xc7\x01"+struct.pack("<I", 0x403000)+b"\xc3")
    with monkeypatch.context() as patch:
        patch.setattr(live, "_pe_bytes", lambda *args: bytes(linked))
        def prove():
            return cleanup.prove_linked_cleanup(obj, parent, relocation, expected, mapping, None, 0x402000)
        assert prove() == destructor.name
        linked[2] ^= 4
        with pytest.raises(ValueError, match="table"):
            prove()
        linked[2] ^= 4
        mapping.symbols[2].address += 16
        with pytest.raises(ValueError, match="existing physical"):
            prove()
        mapping.symbols[2].address -= 16
        mapping.symbols.append(mapping.symbols[0])
        with pytest.raises(ValueError, match="ambiguous"):
            prove()
    for cleanup_first in (False, True):
        targets, strict = {}, set()
        assert cleanup.record_physical_mapping(1, 10, cleanup_first, targets, strict)
        assert cleanup.record_physical_mapping(1, 10, not cleanup_first, targets, strict)
        assert not cleanup.record_physical_mapping(1, 11, False, targets, strict)
        assert not cleanup.record_physical_mapping(2, 10, False, targets, strict)
        assert targets == {1: {10}} and strict == {1}
    targets, strict = {1: {10}, 2: {10}}, set()
    assert not cleanup.record_physical_mapping(1, 10, True, targets, strict)


def _exercise_native_eh_role_witnesses(monkeypatch):
    from types import SimpleNamespace as Row
    from _recoil.commands import native_eh_relocations as eh, live_byte_verify as live

    def symbol(index, name, section, value, storage):
        return Row(index=index, name=name, section_number=section, value=value,
                   storage_class=storage, type=0, aux_count=0)

    handler = symbol(1, "$L123", 2, 1, 6)
    info = symbol(2, "$T234", 3, 0, 3)
    dispatcher = symbol(3, "___CxxFrameHandler", 0, 0, 2)
    unwind = symbol(4, "$T235", 3, 32, 3)
    cleanup = symbol(5, "$L124", 2, 0, 6)
    symbols = [handler, info, dispatcher, unwind, cleanup]
    definitions = [Row(name=name, section_number=index, storage_class=3, aux_count=1,
                       section_definition_selection=5, section_definition_association=1)
                   for index, name in ((2, ".text$x"), (3, ".xdata$x"))]
    code = b"\xc3\xb8"+bytes(4)+b"\xe9"+bytes(4)
    data = struct.pack("<10I", 0x19930520, 1, 0, 0, 0, 0, 0, 0, 0xffffffff, 0)
    sections = {2: Row(index=2, name=".text$x", raw_data=code, characteristics=0x1000),
                3: Row(index=3, name=".xdata$x", raw_data=data, characteristics=0x1000)}
    def reloc(offset, kind, target):
        return Row(offset=offset, type=kind, symbol_index=target.index, symbol_name=target.name)
    relocs = {2: [reloc(2, 6, info), reloc(7, 20, dispatcher)],
              3: [reloc(8, 6, unwind), reloc(36, 6, cleanup)]}
    obj = Row(symbols=symbols+definitions, symbols_by_index={s.index: s for s in symbols},
              section=sections.__getitem__, relocations_by_section=relocs)
    prologue = b"\x6a\xff\x68"+bytes(4)+b"\x64\xa1"+bytes(4)+b"\x50\x64\x89\x25"+bytes(4)
    body = Row(start=0, section_index=1, data=prologue)
    parent_reloc = reloc(3, 6, handler)
    expected = dict(native_eh_max_state=1, offset=3)
    assert eh.prove_object_handler(obj, body, parent_reloc, expected) is handler
    body.symbol = '_parent'
    assert eh.prove_object_handler(obj, body, parent_reloc, {**expected, 'native_eh_parent_symbol':'_parent'}) is handler
    with pytest.raises(ValueError, match='exact reviewed parent'):
        eh.prove_object_handler(obj, body, parent_reloc, {**expected, 'native_eh_parent_symbol':'_other'})
    # Ordinal changes are immaterial, but class, association, role, and addend are not.
    for target, field, value in ((handler, "storage_class", 3), (handler, "type", 0x20),
            (handler, "value", 0), (definitions[0], "section_definition_association", 9),
            (definitions[1], "section_definition_selection", 2), (info, "value", 4),
            (dispatcher, "name", "_wrong")):
        old = getattr(target, field)
        setattr(target, field, value)
        if target is dispatcher:
            relocs[2][1].symbol_name = value
        with pytest.raises(ValueError):
            eh.prove_object_handler(obj, body, parent_reloc, expected)
        setattr(target, field, old)
        relocs[2][1].symbol_name = dispatcher.name
    handler.name = parent_reloc.symbol_name = "$L9999"
    assert eh.prove_object_handler(obj, body, parent_reloc, expected) is handler
    body.data = prologue[:3]+b"\x01\x00\x00\x00"+prologue[7:]
    with pytest.raises(ValueError, match="addend"):
        eh.prove_object_handler(obj, body, parent_reloc, expected)
    body.data = prologue
    with pytest.raises(ValueError, match="FuncInfo"):
        eh.prove_object_handler(obj, body, parent_reloc, dict(native_eh_max_state=2, offset=3))
    early_load = b"\x64\xa1"+bytes(4)+b"\x6a\xff\x68"+bytes(4)+b"\x50\x64\x89\x25"+bytes(4)
    body.data = early_load
    parent_reloc.offset = expected["offset"] = 9
    assert eh.prove_object_handler(obj, body, parent_reloc, expected) is handler
    for offset in (0, 2, 6, 8, 13, 17):
        body.data = early_load[:offset]+bytes((early_load[offset]^1,))+early_load[offset+1:]
        with pytest.raises(ValueError): eh.prove_object_handler(obj, body, parent_reloc, expected)
    body.data = prologue
    parent_reloc.offset = expected["offset"] = 3

    linked_code = bytearray(code)
    struct.pack_into("<I", linked_code, 2, 0x409000)
    struct.pack_into("<i", linked_code, 7, 0x408000-(0x407000+11))
    linked_data = bytearray(data)
    struct.pack_into("<I", linked_data, 8, 0x409020)
    struct.pack_into("<I", linked_data, 36, 0x407000)
    images = {0x407000: linked_code, 0x409000: linked_data}
    mapping = Row(symbols=[Row(symbol=dispatcher.name, address=0x408000)])
    with monkeypatch.context() as patch:
        patch.setattr(live, "_pe_bytes", lambda image, address, size: bytes(images[address][:size]))
        patch.setattr(eh, "frame_handler_import", lambda image, address: None)
        def prove():
            eh.prove_linked_packet(obj, body, handler, 0x407001, mapping, None)
        prove()
        for address, offset in ((0x407000, 0), (0x409000, 0), (0x409000, 36), (0x407000, 7)):
            images[address][offset] ^= 1
            with pytest.raises(ValueError):
                prove()
            images[address][offset] ^= 1
        mapping.symbols *= 2
        with pytest.raises(ValueError, match="identity"):
            prove()
        mapping.symbols.pop()
        relocs[3].append(relocs[3][0])
        with pytest.raises(ValueError, match="overlapping"):
            prove()
        relocs[3].pop()
        definitions[1].section_definition_association = 9
        with pytest.raises(ValueError, match="associative"):
            prove()
        definitions[1].section_definition_association = 1
        prove()

    # The site roles require a current reviewed binding. They do not resolve
    # arbitrary zero operands or borrow a binding from another parent.
    from _recoil.commands import relocation_expectations as relocation
    source_id, target_id, owner_id = "recoil:function:0x401000", "recoil:function:0x402000", "recoil:owner:eh"
    source = dict(binary="recoil", kind="function", pipeline_class="authored-lifecycle",
                  address="0x401000", end_exclusive="0x401030")
    target_row = dict(kind="function", pipeline_class="non-authored", address="0x402000",
                      end_exclusive="0x402010", output_section_id="recoil:section:.text")
    relation = dict(kind="primary-function", address="0x402000", symbol_id=target_id)
    owner = dict(kind="provider-boundary", provider_state="accepted", lifecycle_state="accepted",
                 evidence_ids=["evidence"], relationships=[relation])
    collections = dict(symbols={source_id: source, target_id: target_row}, owners={owner_id: owner},
                       evidence={"evidence": {}})
    document = Row(collection=collections.__getitem__)
    facts = dict(handler=0x402000, handler_offset=3, max_state=1, exception_list_offsets=[9, 17, 35])
    with monkeypatch.context() as patch:
        patch.setattr(eh, "retail_roles", lambda *args: facts)
        patch.setattr(eh, "runtime_absolute_proof", lambda: dict(value=0))
        patch.setattr(relocation, "build_object_binding_snapshot", lambda *args, **kwargs:
                      dict(symbol_id=kwargs["symbol_id"], object_symbol=kwargs["object_symbol"], address=source["address"]))
        def context():
            return eh.binding_context(document, {}, source_id, "_parent", owner_id, ["evidence"], None)
        assert eh.derive_native_expectations(document, {}, source, "_parent", None) == {}
        source[eh.FIELD] = dict(schema=eh.SCHEMA, reviewed=True, reason="reviewed role", context=context())
        def derive():
            return eh.derive_native_expectations(document, {}, source, "_parent", None)
        assert set(derive()) == {(3, 6), (9, 6), (17, 6), (35, 6)}
        assert derive()[(9, 6)]["target_symbol"] == "__except_list"
        alias = eh.derive_native_expectations(document, {}, source, '@vc5-symbol-regex:_parent.*', None)
        assert alias[(3, 6)]['native_eh_parent_symbol'] == '_parent'
        with pytest.raises(ValueError, match='stale'):
            eh.derive_native_expectations(document, {}, source, '@vc5-symbol-regex:_other.*', None)
        with pytest.raises(ValueError, match="stale"):
            eh.derive_native_expectations(document, {}, source, "_different", None)
        for row, field, value in ((source[eh.FIELD], "reviewed", False),
                (owner, "provider_state", "unresolved"), (owner, "evidence_ids", []),
                (owner, "relationships", []), (target_row, "pipeline_class", "authored"),
                (target_row, "end_exclusive", "0x402020")):
            old = row[field]
            row[field] = value
            with pytest.raises(ValueError):
                derive()
            row[field] = old
        assert derive()

        accepted_context = source[eh.FIELD]["context"]
        owner["lifecycle_state"] = "discovered"
        with pytest.raises(ValueError, match="boundary"):
            context()
        owner["gates"] = dict(boundary="accepted")
        pending = context()
        assert pending["provider_lifecycle"] == dict(state="discovered", boundary="accepted")
        with pytest.raises(ValueError, match="stale"):
            derive()
        source[eh.FIELD]["context"] = pending
        assert derive()
        owner["gates"]["boundary"] = "unresolved"
        with pytest.raises(ValueError, match="boundary"):
            derive()
        owner["gates"]["boundary"] = "accepted"
        owner["lifecycle_state"] = "accepted"
        with pytest.raises(ValueError, match="stale"):
            derive()
        source[eh.FIELD]["context"] = accepted_context
        assert derive()

        with pytest.raises(ValueError, match="no primary owner"):
            eh.binding_context(document, {}, source_id, "_parent", None, ["evidence"], None)
        owner["relationships"] = []
        target_row["ownership_state"] = "unresolved"
        target_row["authored_order_role"] = "compiler-generated-eh-helper"
        source["evidence_ids"] = ["evidence"]
        unowned = eh.binding_context(document, {}, source_id, "_parent", None, ["evidence"], None)
        assert unowned["owner_id"] is None and unowned["relationship"] is None
        source[eh.FIELD]["context"] = unowned
        assert derive()
        owner["relationships"] = [relation]
        with pytest.raises(ValueError, match="no primary owner"):
            derive()

    from _recoil.commands import provider_function_mutation as provider
    absolute = Row(name="__except_list", section_number=-1, storage_class=2, type=0,
                   value=0, aux_count=0)
    members = [Row(name="build/intel/dll_obj/dllsupp.obj", data=b"__except_list")]
    class RuntimeRoot:
        def __truediv__(self, path):
            assert path == "VC/LIB/MSVCRT.LIB"
            return Row(read_bytes=lambda: b"canonical runtime")
    with monkeypatch.context() as patch:
        patch.setattr(eh, "DEFAULT_VC5_ROOT", RuntimeRoot())
        patch.setattr(provider, "parse_archive_members", lambda data: members)
        patch.setattr(eh, "CoffObject", Row(from_bytes=lambda data: Row(symbols=[absolute])))
        assert eh.runtime_absolute_proof()["value"] == 0
        for field, value in (("section_number", 0), ("section_number", 1), ("value", 4),
                             ("storage_class", 3), ("type", 0x20), ("aux_count", 1)):
            old = getattr(absolute, field)
            setattr(absolute, field, value)
            with pytest.raises(ValueError):
                eh.runtime_absolute_proof()
            setattr(absolute, field, old)
        members *= 2
        with pytest.raises(ValueError, match="ambiguous"):
            eh.runtime_absolute_proof()
        members.pop()
        members[0].name = "unrelated.obj"
        with pytest.raises(ValueError, match="drifted"):
            eh.runtime_absolute_proof()

    from _recoil.commands import provider_target_mutation as imports
    entry = Row(address="0x409000", dll="MSVCRT.dll", import_name="__CxxFrameHandler", import_ordinal=None)
    entries = [entry]
    with monkeypatch.context() as patch:
        patch.setattr(live, "_pe_bytes", lambda *args: b"\xff\x25"+struct.pack("<I", 0x409000))
        patch.setattr(imports, "_retail_import_targets", lambda image: (entries, {}))
        assert eh.frame_handler_import(None, 0x408000) == 0x409000
        for field, value in (("dll", "OTHER.dll"), ("import_name", "_other"),
                             ("import_ordinal", 1), ("address", "0x409004")):
            old = getattr(entry, field)
            setattr(entry, field, value)
            with pytest.raises(ValueError, match="identity"):
                eh.frame_handler_import(None, 0x408000)
            setattr(entry, field, old)
        entries.append(entry)
        with pytest.raises(ValueError, match="identity"):
            eh.frame_handler_import(None, 0x408000)


def test_authored_byte_derivation_uses_exact_folded_call_selectors_and_rejects_drift(tmp_path, monkeypatch):
    from copy import deepcopy
    from types import SimpleNamespace as Row
    from _recoil.commands import relocation_expectations as relocation

    source, target = 0x401000, 0x402000
    target_id = "recoil:function:0x402000"
    alias = dict(object_symbol="?Remove@Camera@@YAHXZ", fold_status="proven-fold-alias",
                 pipeline_class="authored", evidence_ids=["recoil:evidence:fold"],
                 retail_target_selectors={"direct_call_sites": [hex(source)]})
    target_row = dict(binary="recoil", kind="function", address=hex(target), end_exclusive=hex(target+16),
                      logical_aliases={"recoil:logical:camera": alias},
                      icf_address_group={"model": "authored-linker-coalesced-v1", "physical_gate_symbol_id": target_id})
    symbols = {target_id: target_row}
    document = Row(collection=lambda name: symbols if name == "symbols" else {})
    reference = tmp_path / "retail.exe"
    reference.write_bytes(b"retail fixture")
    code = bytearray(b"\xe8" + struct.pack("<i", target-source-5) + b"\xc3")
    monkeypatch.setattr(relocation, "parse_pe_headers", lambda *args, **kw: Row(image_base=0x400000, size_of_image=0x10000))
    monkeypatch.setattr(relocation, "_pe_bytes", lambda *args: bytes(code))
    physical = relocation.TargetIdentity(target_id, target, target+16, ("?Remove@Object@@YAHXZ",), "registered-vc5-target")
    monkeypatch.setattr(relocation, "build_target_identity_state", lambda *args, **kw: ((physical,), []))
    row = dict(address=hex(source), end_exclusive=hex(source+6), scope_ids=["recoil:function:0x401000"])

    def derive():
        return relocation.derive_relocation_expectations(document=document, row=row,
            object_symbol="_entry", bindings={}, reference=reference)

    report = derive()
    assert report["passed"]
    expected = report["expectations"][0]
    assert expected["target_symbol"] == alias["object_symbol"]
    assert expected["target_symbol_id"] == target_id and expected["coff_addend"] == 0
    assert expected["retail_target"] == target
    code[1:5] = struct.pack("<i", target+1-source-5)
    assert derive()["unresolved"][0]["kind"] == "authored-icf-call-selector-retail-drift"
    code[1:5] = struct.pack("<i", target-source-5)
    code[0] = 0xb8  # Same address operand is not a direct call or tail call.
    code[1:5] = struct.pack("<I", target)
    assert derive()["unresolved"][0]["kind"] == "authored-icf-call-selector-retail-drift"
    code[:] = b"\xe8" + struct.pack("<i", target-source-5) + b"\xc3"
    target_row["logical_aliases"]["recoil:logical:window"] = deepcopy(alias)
    with pytest.raises(relocation.RelocationExpectationError, match="conflicting"):
        derive()
    del target_row["logical_aliases"]["recoil:logical:window"]
    alias["fold_status"] = "unresolved"
    with pytest.raises(relocation.RelocationExpectationError, match="invalid"):
        derive()
    alias["fold_status"] = "proven-fold-alias"
    alias["retail_target_selectors"]["direct_call_sites"] = [hex(source+8)]
    assert derive()["expectations"][0]["target_symbol"] == physical.object_symbols[0]


def test_named_static_stem_requires_exact_storage_contents_and_all_readers(monkeypatch, tmp_path):
    from types import SimpleNamespace as Row
    from _recoil.commands import live_byte_verify as byte
    from _recoil.call_contract.listing import _matches_compiler_literal

    literal = dict(name="$T42", section_name=".rdata", section_number=7,
                   value=0x120, symbol_type=0, storage_class=3, aux_count=0,
                   data=struct.pack("<f", 1))
    assert _matches_compiler_literal(Row(**literal), literal["data"])
    assert _matches_compiler_literal(Row(**{**literal, "value": 0x560,
                                           "section_number": 12}), literal["data"])
    for field, value in (("name", "named"), ("section_name", ".data"),
                         ("section_number", 0), ("value", -1), ("symbol_type", 0x20),
                         ("storage_class", 2), ("aux_count", 1), ("data", b"bad")):
        assert not _matches_compiler_literal(Row(**{**literal, field: value}), literal["data"])

    # A virtual-only data range must not read the following section's file
    # bytes. Only the explicit loaded-data path may supply PE zero fill.
    retail_path = tmp_path / "loaded-data.exe"
    retail_path.write_bytes(b"HEAD" + b"ABCD" + b"NEXTSECTION")
    section = Row(virtual_address=0x1000, virtual_size=12, raw_pointer=4, raw_size=4)
    with monkeypatch.context() as pe_patch:
        pe_patch.setattr(byte, "parse_pe_headers", lambda *args, **kwargs:
                         Row(image_base=0x400000, sections=[section]))
        assert byte._pe_bytes(retail_path, 0x401000, 4) == b"ABCD"
        for address, length in ((0x401004, 4), (0x401002, 4)):
            with pytest.raises(byte.LiveByteError, match="not file-backed"):
                byte._pe_bytes(retail_path, address, length)
        assert byte._pe_bytes(retail_path, 0x401004, 4, allow_zero_fill=True) == bytes(4)
        assert byte._pe_bytes(retail_path, 0x401002, 4, allow_zero_fill=True) == b"CD\0\0"
        for address, length in ((0x401008, 8), (0x400fff, 4), (0x401000, -1)):
            with pytest.raises(byte.LiveByteError, match="within one PE section"):
                byte._pe_bytes(retail_path, address, length, allow_zero_fill=True)
        section.raw_pointer = 100
        with pytest.raises(byte.LiveByteError, match="truncated"):
            byte._pe_bytes(retail_path, 0x401000, 4, allow_zero_fill=True)

    target_id = "recoil:data:0x501000"
    target = Row(name="_limit$S12", index=1, value=0, section_number=2, storage_class=3, type=0)
    entry = Row(name="_entry", index=0, value=0, section_number=1, storage_class=2, type=0x20)
    rel = Row(offset=0, type=6, symbol_index=1, symbol_name=target.name)
    sections = {1: Row(index=1, name=".text", characteristics=0x20, raw_data=b"\0" * 8),
                2: Row(index=2, name=".rdata", characteristics=0x40, raw_data=struct.pack("<f", 5))}
    obj = Row(symbols=[entry, target], symbols_by_index={0: entry, 1: target},
              relocations_by_section={1: [rel]}, section=lambda index: sections[index],
              symbol_end=lambda symbol, section: 4, function_end=lambda symbol, section: 8)
    body = Row(start=0, end=8, data=b"\0" * 8, relocations=[rel])
    expected = dict(object_symbol="_entry", offset=0, type=6, target_symbol="_limit",
                    target_symbol_id=target_id, coff_addend=0, resolved_target_addend=0,
                    retail_target=0x501000)
    target_row = dict(binary="recoil", kind="data", extent_state="known", ownership_state="primary-owned",
                      output_section_id="recoil:section:.rdata", address="0x501000", end_exclusive="0x501004",
                      size=4, relocation_target_binding={"object_symbol": "_limit"})
    for derivation in ("reviewed-relocation-target-binding", "registered-vc5-target+reviewed-relocation-target-binding"):
        item = {**expected, "derivation": derivation}
        assert byte._needs_retail_reader_universe(item, target_row, {"_limit"})
        assert not byte._needs_retail_reader_universe(item, target_row, set())
        assert not byte._needs_retail_reader_universe(item, {**target_row, "kind": "function"}, {"_limit"})
    assert byte._needs_retail_reader_universe(
        {"provenance_mode": byte.PHYSICAL_TARGET_UNRESOLVED_VC5_TEMPORARY}, {}, set())
    readers = [byte.RetailObjectReader("recoil:function:0x401000", 0x401000, "_entry", 0, 6, 0, 0x501000)]
    monkeypatch.setattr(byte, "_pe_bytes", lambda *args, **kwargs: struct.pack("<f", 5))

    def check(catalog=None, row=None, universe=None):
        return byte._canonicalize_vc5_local_data_ordinals(
            coff_object=obj, function_bytes=body, relocation_catalog=[catalog or expected],
            target_rows={target_id: row or target_row}, reference=Path("retail.exe"),
            retail_reader_universes={target_id: readers if universe is None else universe})[0][1]

    good = check()
    assert good.symbol_name == "_limit" and good.compiler_local_ordinal_canonicalized
    assert good.expected_target_bytes == struct.pack("<f", 5)
    from copy import deepcopy
    relational_row = deepcopy(target_row)
    del relational_row["ownership_state"]
    context = dict(owner={"owner_id":"recoil:owner:unit"},
        target={**{k:relational_row.get(k) for k in
            ("kind", "address", "end_exclusive", "output_section_id", "ownership_state")},
            "symbol_id":target_id, "object_symbol":"_limit"},
        relationship={"kind":"primary-data", "symbol_id":target_id, "address":"0x501000"})
    relational_row["relocation_target_binding"].update(reviewed=True, binding_context=context)
    assert check(row=relational_row).compiler_local_ordinal_canonicalized
    assert not check(row=relational_row, universe=[]).compiler_local_ordinal_canonicalized
    for changed_row, field, value in ((context["owner"], "owner_id", None),
        (context["relationship"], "symbol_id", "recoil:data:0x501004"),
        (context["relationship"], "kind", "consumes"),
        (context["target"], "object_symbol", "_other"),
        (context["target"], "end_exclusive", "0x501008")):
        with monkeypatch.context() as changed:
            changed.setitem(changed_row, field, value)
            assert not check(row=relational_row).compiler_local_ordinal_canonicalized
    # Registered selectors must be resolved before the all-or-nothing local
    # ordinal proof. Their presence cannot hide a bad selector or bad scalar.
    literal = Row(name='_label123', index=2, value=0, section_number=3, storage_class=2, type=0)
    sections[3] = Row(index=3, name='.data', characteristics=0x40, raw_data=b'text')
    obj.symbols.append(literal); obj.symbols_by_index[2] = literal
    extra = Row(offset=4, type=6, symbol_index=2, symbol_name=literal.name)
    body.relocations.append(extra); obj.relocations_by_section[1].append(extra)
    selector = dict(symbol_regex=r'_label[0-9]+', source_from='src/other.cpp', kind='data')
    selected = dict(object_symbol='_entry', offset=4, type=6,
                    target_symbol='@vc5-symbol-regex:_label[0-9]+', registered_target_selector=selector,
                    coff_addend=0)
    def mixed(loader):
        return byte._canonicalize_vc5_local_data_ordinals(coff_object=obj, function_bytes=body,
            relocation_catalog=[expected,selected], target_rows={target_id:target_row},
            reference=Path('retail.exe'), retail_reader_universes={target_id:readers}, load_definition=loader)
    assert not mixed(None)[0][1].compiler_local_ordinal_canonicalized
    assert mixed(lambda source: obj)[0][1].compiler_local_ordinal_canonicalized
    assert mixed(lambda source: obj)[1][1].canonicalized
    literal.name='_other'
    assert not mixed(lambda source: obj)[0][1].compiler_local_ordinal_canonicalized
    literal.name='_label123'
    sections[2].raw_data=struct.pack('<f',6)
    assert not mixed(lambda source: obj)[0][1].compiler_local_ordinal_canonicalized
    sections[2].raw_data=struct.pack('<f',5)
    obj.symbols.pop(); del obj.symbols_by_index[2]; del sections[3]
    body.relocations.pop(); obj.relocations_by_section[1].pop()
    for changes in ({"target_symbol": "_other"}, {"coff_addend": 1}, {"type": 20}):
        assert not check(catalog={**expected, **changes}).compiler_local_ordinal_canonicalized
    for changes in ({"size": 8}, {"ownership_state": "unresolved"},
                    {"relocation_target_binding": {"object_symbol": "_other"}}):
        assert not check(row={**target_row, **changes}).compiler_local_ordinal_canonicalized
    assert not check(universe=[]).compiler_local_ordinal_canonicalized
    missing = byte.RetailObjectReader("recoil:function:0x402000", 0x402000, "_other", 0, 6, 0, 0x501000)
    assert not check(universe=readers + [missing]).compiler_local_ordinal_canonicalized
    for flags in (0x80000040, 0x80, 0):
        sections[2].characteristics = flags
        assert not check().compiler_local_ordinal_canonicalized
    sections[2].characteristics = 0x40
    sections[2].raw_data = struct.pack("<f", 6)
    assert not check().compiler_local_ordinal_canonicalized
    sections[2].raw_data = struct.pack("<f", 5)
    obj.relocations_by_section[1].append(Row(offset=4, type=6, symbol_index=1, symbol_name=target.name))
    assert not check().compiler_local_ordinal_canonicalized

    # A manifest can register the scalar definition alongside its functions.
    # Data bytes must never enter the x86 decoder; possible data readers block.
    source_id = "recoil:function:0x401000"
    rows = {target_id: target_row, source_id: dict(kind="function", address="0x401000",
            end_exclusive="0x401008", pipeline_class="authored")}
    document = Row(collection=lambda name: rows)
    manifest = Row(name="unit", source_from="src/unit.cpp")
    binding = byte.TargetBinding(manifest, Row(symbol="_entry"))
    bindings = {source_id: [binding], target_id: [byte.TargetBinding(manifest, Row(symbol="_limit"))]}

    def decode(*, row, **kwargs):
        assert row["kind"] == "function"
        return [dict(type=6, offset=0, retail_target=0x501000)]

    monkeypatch.setattr(byte, "decode_retail_target_sites", decode)

    def universe():
        return byte._registered_retail_reader_universe(document=document, bindings=bindings,
            binding=binding, target_symbol_id=target_id, reference=Path("retail.exe"))

    assert universe() == tuple(readers)
    # An incomplete retail decode leaves the complete reader proof unavailable;
    # the full census must receive a scoped failure, never an empty reader set.
    def incomplete_decode(**kwargs):
        raise byte.RelocationExpectationError("unproven indexed jump")

    with monkeypatch.context() as decode_patch:
        decode_patch.setattr(byte, "decode_retail_target_sites", incomplete_decode)
        with pytest.raises(byte.LiveByteError) as unavailable:
            universe()
        assert target_id in str(unavailable.value) and source_id in str(unavailable.value)
        assert "unproven indexed jump" in str(unavailable.value)
        assert isinstance(unavailable.value.__cause__, byte.RelocationExpectationError)
    assert universe() == tuple(readers)
    # Separate manifests for the same TU contribute a single reader census;
    # private function spellings are resolved only as fresh COFF witnesses.
    other_id = "recoil:function:0x402000"
    rows[other_id] = dict(kind="function", address="0x402000",
                         end_exclusive="0x402008", pipeline_class="authored")
    other_manifest = Row(name="whole-unit", source_from="src/unit.cpp")
    bindings[source_id].append(byte.TargetBinding(other_manifest, Row(symbol="_entry")))
    bindings[other_id] = [byte.TargetBinding(other_manifest,
        Row(symbol="", symbol_regex=r"_private[0-9]+"))]
    expanded = universe()
    assert len(expanded) == 2 and expanded[1].object_symbol_regex == r"_private[0-9]+"
    other = Row(name="_private123", index=2, value=0, section_number=3, storage_class=3, type=0x20)
    sections[3] = Row(index=3, name=".text", characteristics=0x20, raw_data=bytes(8))
    obj.symbols.append(other)
    obj.relocations_by_section[1] = [rel]
    obj.relocations_by_section[3] = [Row(offset=0, type=6, symbol_index=1)]
    assert check(universe=expanded).compiler_local_ordinal_canonicalized
    from _recoil.commands.byte_symbol_selectors import PREFIX
    def check_private_reader(pattern=r"_private[0-9]+", selected_offsets=(0,), selected_universe=expanded):
        return byte._static_data_reader_mismatch(coff_object=obj, target=target,
            universe=selected_universe, reviewed_object_symbol=PREFIX+pattern,
            site_offsets=selected_offsets, reviewed_retail_target=0x501000)
    assert check_private_reader() is None
    assert check_private_reader(pattern=r"_private.*") == "physical-witness-reviewed-reader-selector-is-not-retail-proved"
    assert check_private_reader(selected_offsets=(4,)) == "physical-witness-reviewed-reader-is-not-retail-proved"
    other.name = "_different123"
    assert check_private_reader() == "physical-witness-retail-reader-selector-is-not-unique"
    other.name = "_private123"
    same_reader = byte.replace(expanded[1], object_symbol=other.name, object_symbol_regex="")
    assert check(universe=expanded + (same_reader,)).compiler_local_ordinal_canonicalized
    assert not check(universe=expanded + (expanded[1],)).compiler_local_ordinal_canonicalized
    wrong_identity = byte.replace(same_reader, source_address=0x403000)
    assert not check(universe=expanded + (wrong_identity,)).compiler_local_ordinal_canonicalized
    # An unnamed scalar section can be placed through all registered readers,
    # but each reader must have a unique same-object MAP identity and an exact
    # object-to-linked body, and every resolved operand must agree.
    placements = [Row(symbol="_entry", address=0x601000, object="unit.obj"),
                  Row(symbol=other.name, address=0x602000, object="unit.obj")]
    linked_bodies = {row.address: struct.pack("<I", 0x603000) + bytes(4) for row in placements}
    obj.function_bytes = lambda name: Row(start=0, data=bytes(8), relocation_mask=(True,) * 4 + (False,) * 4)
    with monkeypatch.context() as linked_patch:
        linked_patch.setattr(byte, "_pe_bytes", lambda path, address, length: linked_bodies[address])
        def placement():
            return byte._candidate_static_target_from_readers(
                coff_object=obj, obj_path=Path("unit.obj"), parsed_map=Row(symbols=placements),
                image=Path("current.exe"), symbol_name=target.name, candidate_target_base=0x603000,
                universe=expanded, reviewed_object_symbol="_entry", site_offsets=[0],
                reviewed_retail_target=0x501000)
        assert placement().target_bases == frozenset((0x603000,))
        linked_bodies[0x602000] = struct.pack("<I", 0x603004) + bytes(4)
        assert not placement().target_bases
        linked_bodies[0x602000] = struct.pack("<I", 0x603000) + b"\x01" + bytes(3)
        assert not placement().target_bases
        linked_bodies[0x602000] = linked_bodies[0x601000]
        placements[1].object = "foreign.obj"
        assert not placement().target_bases
        placements[1].object = "unit.obj"
        placements.append(placements[1])
        assert not placement().target_bases
        placements.pop()
        placements.pop()
        assert not placement().target_bases
    other.name = "_unknown"
    assert not check(universe=expanded).compiler_local_ordinal_canonicalized
    other.name = "_private123"
    obj.symbols.append(Row(**{**vars(other), "name": "_private456"}))
    assert not check(universe=expanded).compiler_local_ordinal_canonicalized
    obj.symbols.pop()
    other_manifest.source_from = "src/another.cpp"
    assert universe() == tuple(readers)
    del bindings[other_id]
    monkeypatch.setattr(byte, "_pe_bytes", lambda *args, **kwargs: struct.pack("<I", 0x501000))
    with pytest.raises(byte.LiveByteError, match="data-reader semantics are unresolved"):
        universe()
    target_row["end_exclusive"] = target_row["address"]
    with pytest.raises(byte.LiveByteError, match="data extent is invalid"):
        universe()
    # Unknown source extents need not become invented exact data facts just to
    # exclude readers: scan a conservative suffix of the immutable PE section.
    with monkeypatch.context() as bound_patch:
        reference = tmp_path / "bounded-retail.exe"
        reference.write_bytes(b"retail fixture")
        section = Row(name=".rdata", virtual_address=0x1000, virtual_size=32,
                      raw_size=16, characteristics=0x40)
        headers = Row(image_base=0x500000, sections=[section])
        bound_patch.setattr(byte, "parse_pe_headers", lambda *a, **kw: headers)
        suffix = bytearray(28)
        def read_bound(path, start, length, *, allow_zero_fill=False):
            assert path == reference and start == 0x501004 and length == 28 and allow_zero_fill
            return bytes(suffix)
        bound_patch.setattr(byte, "_pe_bytes", read_bound)
        unknown = dict(binary="recoil", kind="data", address="0x501004",
                       extent_state="unknown", output_section_id="recoil:section:.rdata")
        before = dict(unknown)
        assert byte._retail_data_reader_bytes(reference, unknown) == bytes(28)
        assert unknown == before and "end_exclusive" not in unknown
        rows[target_id] = unknown
        def bounded_universe():
            return byte._registered_retail_reader_universe(document=document, bindings=bindings,
                binding=binding, target_symbol_id=target_id, reference=reference)
        assert len(bounded_universe()) == 1
        # Include unaligned potential pointers at the very end of the bound.
        suffix[-4:] = struct.pack("<I", 0x501004)
        with pytest.raises(byte.LiveByteError, match="data-reader semantics are unresolved"):
            bounded_universe()
        for drift in ({"binary":"messages"}, {"output_section_id":"recoil:section:.data"},
                      {"address":"0x501020"}):
            with pytest.raises(byte.LiveByteError, match="data-section bound"):
                byte._retail_data_reader_bytes(reference, {**unknown, **drift})
        headers.sections.append(section)
        with pytest.raises(byte.LiveByteError, match="data-section bound"):
            byte._retail_data_reader_bytes(reference, unknown)
        headers.sections.pop()
        for flags in (0, 0x20, 0x60):
            section.characteristics = flags
            with pytest.raises(byte.LiveByteError, match="data-section bound"):
                byte._retail_data_reader_bytes(reference, unknown)
        rows[target_id] = target_row
    missing_object = tmp_path / "current.obj"
    missing_object.write_bytes(b"fixture")

    def missing_symbol(name):
        raise ValueError("Symbol not found in COFF object: " + name)

    monkeypatch.setattr(byte, "object_path", lambda *args: missing_object)
    monkeypatch.setattr(byte.CoffObject, "from_path", lambda path: Row(function_bytes=missing_symbol))
    result = byte._compare_row(mode="authored", row={**rows[source_id], "symbol_id": source_id},
        binding=binding, config=None, paths=None, reference=Path("retail.exe"), parsed_map=None)
    assert not result["passed"] and result["stage"] == "object-symbol"
    assert result["symbol"] == "_entry" and "Symbol not found" in result["message"]
    _exercise_near_byte_review(tmp_path, monkeypatch)
    _exercise_float_alignment_padding()


def _exercise_float_alignment_padding():
    from types import SimpleNamespace as Row
    from _recoil.commands import live_byte_verify as byte
    sections = {
        1: Row(index=1, name='.text', characteristics=0x20, raw_data=bytes.fromhex('d90500000000c3')),
        2: Row(index=2, name='.text', characteristics=0x20, raw_data=bytes.fromhex('dd0500000000c3')),
        3: Row(index=3, name='.rdata', characteristics=0x40, raw_data=bytes(16)),
    }
    symbols = [Row(index=1,name='$T1',value=0,section_number=3,storage_class=3,type=0),
               Row(index=2,name='$T2',value=8,section_number=3,storage_class=3,type=0),
               Row(index=3,name='_float',value=0,section_number=1,storage_class=2,type=0x20),
               Row(index=4,name='_double',value=0,section_number=2,storage_class=2,type=0x20)]
    relocs = {1:[Row(offset=2,type=6,symbol_index=1)],2:[Row(offset=2,type=6,symbol_index=2)]}
    obj = Row(symbols=symbols, symbols_by_index={s.index:s for s in symbols},
              relocations_by_section=relocs, section=lambda i:sections[i],
              symbol_end=lambda s,section: min([x.value for x in symbols
                  if x.section_number==s.section_number and x.value>s.value]+[len(section.raw_data)]))
    def check(): return byte._vc5_float_alignment_end(obj,symbols[0],sections[3],4,8)
    assert check()==4
    for value in (bytes(4)+b'\x01'+bytes(11),bytes(20)):
        sections[3].raw_data=value
        assert check()==8
    sections[3].raw_data=bytes(16)
    for code in ('dd0500000000c3','d90501000000c3','a10000000090c3','648b0500000000'):
        sections[1].raw_data=bytes.fromhex(code)
        assert check()==8
    sections[1].raw_data=bytes.fromhex('d90500000000c3')
    sections[2].raw_data=bytes.fromhex('d90500000000c3')
    assert check()==8
    sections[2].raw_data=bytes.fromhex('dd0500000000c3')
    symbols[1].name='_named'
    assert check()==8
    symbols[1].name='$T2'
    following=relocs.pop(2)
    assert check()==8
    relocs[2]=following
    for value in (0,4,8):
        symbols.append(Row(index=5,name='_alias',value=value,section_number=3,storage_class=2,type=0))
        assert check()==8
        symbols.pop()
    relocs[3]=[Row(offset=0,type=6,symbol_index=1)]
    sections[3].raw_data=b'\x04'+bytes(15)
    assert check()==8
    del relocs[3]
    sections[3].raw_data=bytes(16)
    assert check()==4


def _exercise_near_byte_review(tmp_path, monkeypatch):
    from types import SimpleNamespace as Row
    from _recoil.commands import live_byte_verify as byte
    reference = tmp_path/"retail.exe"
    candidate = tmp_path/"candidate.exe"
    obj_path = tmp_path/"unit.obj"; obj_path.write_bytes(b"fixture")
    expected, actual = bytes.fromhex("b8 01 00 00 00 c3"), bytes.fromhex("b8 02 00 00 00 c3")
    body = Row(start=0, data=actual, relocation_mask=(False,)*6, relocations=[], section_index=1)
    obj = Row(function_bytes=lambda *a, **kw: body, symbols=[])
    monkeypatch.setattr(byte, "object_path", lambda *a: obj_path)
    monkeypatch.setattr(byte.CoffObject, "from_path", lambda *a: obj)
    linked = [actual]
    monkeypatch.setattr(byte, "_pe_bytes", lambda path, *a, **kw: expected if path == reference else linked[0])
    monkeypatch.setattr(byte, "source_context", lambda *a: {})
    row = dict(address="0x401000", end_exclusive="0x401006", symbol_id="recoil:function:0x401000")
    binding = byte.TargetBinding(Row(source_from="unit.cpp", name="unit"), Row(symbol="_entry", symbol_regex=None), scope_id=row["symbol_id"])
    def compare(allow=False, relocations=()):
        return byte._compare_row(mode="authored", row=row, binding=binding, config=Row(sources=[tmp_path/"unit.cpp"]),
            paths=Row(exe_path=candidate), reference=reference,
            parsed_map=Row(symbols=[Row(symbol="_entry", address=0x402000, is_function=True)]),
            relocation_catalog=relocations, target_rows={}, allow_near_byte_review=allow)
    assert compare()["stage"] == "object-body"
    reviewed = compare(True)
    assert reviewed["structural_comparison_complete"] and not reviewed["passed"] and reviewed["match_level"] is None
    linked[0] = expected
    assert not compare(True)["structural_comparison_complete"]
    linked[0] = actual
    incomplete = compare(True, [{"offset": 1, "type": 6, "target_symbol": "_missing"}])
    assert not incomplete.get("structural_comparison_complete", False) and not incomplete["passed"]

    # Join a real numerical body proof with relocation/link obligations. The
    # COFF/map adapters supply one typed fixture; body equivalence must never
    # bypass a missing relocation, wrong addend/target or absent linked body.
    from _recoil.lib.commutative_match import COMMUTATIVE_CONTRACT, COMMUTATIVE_VERSION
    from _recoil.lib.function_match import MATCH_VERSION
    expected = bytes.fromhex("be00304000 d94004 d84a08 d91f c3")
    actual = bytes.fromhex("be00000000 d94208 d84804 d91f c3")
    linked[0] = bytes.fromhex("be00404000 d94208 d84804 d91f c3")
    relocation = Row(offset=1, type=6, symbol_name="_data")
    body = Row(start=0, data=actual, relocation_mask=tuple(1 <= i < 5 for i in range(len(actual))),
               relocations=[relocation], section_index=1)
    row["end_exclusive"] = hex(0x401000 + len(expected))
    catalog = [{"offset": 1, "type": 6, "target_symbol": "_data", "coff_addend": 0,
                "retail_target": 0x403000, "resolved_target_addend": 0}]
    canonical = byte.CanonicalRelocationTarget("_data", 0, False, "fixture")
    monkeypatch.setattr(byte, "_canonicalize_vc5_local_data_ordinals", lambda **kw:
                        [(rel, canonical) for rel in body.relocations])
    monkeypatch.setattr(byte, "_candidate_target_identity", lambda **kw:
        byte.CandidateTargetIdentity(frozenset({0x404000}), frozenset({"_data"}), "fixture", "typed target"))
    reviews = {}
    mapped = [Row(symbol="_entry", address=0x402000, is_function=True)]
    def compare_commutative_link(relocations=catalog):
        return byte._compare_row(mode="authored", row=row, binding=binding, config=Row(sources=[tmp_path/"unit.cpp"]),
            paths=Row(exe_path=candidate), reference=reference, parsed_map=Row(symbols=mapped),
            relocation_catalog=relocations, target_rows=reviews)
    pending = compare_commutative_link()
    assert pending["commutative_proof"]["passed"] and pending["stage"] == "commutative-review-required"
    review = {"version": MATCH_VERSION, "commutative_version": COMMUTATIVE_VERSION,
              "decision": "compiler-commutative-operand-selection-only", "evidence_id": "review",
              "no_remaining_credible_source_options": True, "context": {},
              "differences": pending["commutative_proof"]["differences"], "contract": COMMUTATIVE_CONTRACT,
              "contract_justification": "reviewed finite ordinary ABI domain and continuation noninterference"}
    reviews[row["symbol_id"]] = {"commutative_match_review": review}
    complete = compare_commutative_link()
    assert complete["passed"] and complete["match_level"] == "commutative"
    assert complete["relocation_expectations_exact"] and not complete["object_body_equal_outside_relocations"]
    for invalid in (None, [], [{**catalog[0], "type": 20}], [{**catalog[0], "target_symbol": "_wrong"}],
                    [{**catalog[0], "coff_addend": 4}], [{**catalog[0], "retail_target": 0x403004}]):
        result = compare_commutative_link(invalid)
        assert result["commutative_proof"]["passed"] and not result["passed"]
    mapped.clear()
    assert compare_commutative_link()["stage"] == "linked-presence"
    mapped.append(Row(symbol="_entry", address=0x402000, is_function=True))
    linked[0] = bytes.fromhex("be04404000 d94208 d84804 d91f c3")
    assert not compare_commutative_link()["passed"]  # resolved operand/addend drift
    linked[0] = bytes.fromhex("be00404000 d94208 d84808 d91f c3")
    assert not compare_commutative_link()["passed"]  # linked body differs from object witness
    linked[0] = bytes.fromhex("be00404000 d94208 d84804 d91f c3")
    review["context"] = {"stale": True}
    assert not compare_commutative_link()["passed"]


def alias_object(alias: str = "_alias", target: str = "_target") -> bytes:
    def name(value: str) -> bytes:
        return value.encode("ascii").ljust(8, b"\0")

    target_row = name(target) + struct.pack("<IhHBB", 0, 0, 0, 2, 0)
    alias_row = name(alias) + struct.pack("<IhHBB", 0, 0, 0, 105, 1)
    alias_aux = struct.pack("<II", 0, 3) + b"\0" * 10
    header = struct.pack("<HHIIIHH", 0x14C, 0, 0, 20, 3, 0, 0)
    return header + target_row + alias_row + alias_aux + struct.pack("<I", 4)


def test_alias_source_accepts_only_noncontributing_weak_alias_directives() -> None:
    rows = ".386\n.model flat\nEXTERN _target:PROC\nALIAS <_alias> = <_target>\nEND\n"
    externs, aliases = parse_alias_source_text(rows, path=Path("unit.asm"))
    assert externs == {"_target"}
    assert aliases == (CoffAlias("_alias", "_target"),)
    with pytest.raises(ValueError, match="forbidden"):
        parse_alias_source_text(".code\nEND\n", path=Path("bad.asm"))


def test_alias_object_must_be_i386_zero_section_and_zero_contribution(tmp_path: Path) -> None:
    source = tmp_path / "unit.asm"
    source.write_text(
        ".386\n.model flat\nEXTERN _target:PROC\nALIAS <_alias> = <_target>\nEND\n",
        encoding="ascii",
    )
    obj = tmp_path / "unit.obj"
    obj.write_bytes(alias_object())
    report = validate_alias_object(
        obj,
        CoffAliasSource(source, (CoffAlias("_alias", "_target"),), Path("unit.cpp")),
    )
    assert report["validated"] is True
    assert report["section_count"] == 0
    assert report["contribution_bytes"] == 0


def test_vc5_parser_exposes_smoke_without_profile_matrix() -> None:
    parser = vc5_verify.build_parser()
    args = parser.parse_args(["--smoke", "--json", "--build-root", "scratch"])
    assert args.smoke is True and args.json is True
    assert "profile_matrix" not in vars(args)


@pytest.mark.parametrize("item_kind", ["function", "data"])
def test_byte_evidence_reports_effective_source_profile(item_kind, capsys) -> None:
    from types import SimpleNamespace as Row

    target = Row(source_from="unit.cpp", compiler_profile="default",
                 compiler_flags=("/Ob0",), manifest_path=Path("target.json"))
    compiled = Row(source_path=Path("unit.cpp"), effective_compiler_profile="inline",
                   effective_compiler_flags=("/Ob1", "/MD"), compiler_env="vc5",
                   compiler_version="11", obj_path=Path("unit.obj"), cod_path=Path("unit.cod"))
    comparison = Row(mask_path=None, relocation_identity_path=None, diff_path=None,
                     triage_path=None, text_diff_path=None, classified_text_path=None)
    result = Row(mismatches=0, target=target, item_kind=item_kind,
                 function=Row(address="0x401000", name="entry", symbol="_entry"),
                 comparison=comparison, vc5_size_or_diff_count=1)
    vc5_verify.print_evidence_block(compiled, result)
    output = capsys.readouterr().out
    assert "Compiler profile: inline" in output
    assert "Compiler flags: /Ob1 /MD" in output
    assert "/Ob0" not in output
    compiled.effective_compiler_profile = ""
    compiled.effective_compiler_flags = ()
    vc5_verify.print_evidence_block(compiled, result)
    fallback = capsys.readouterr().out
    assert "Compiler profile: default" in fallback
    assert "Compiler flags: /Ob0" in fallback


def test_lifecycle_icf_requires_every_fresh_comdat_and_exact_relocation_semantics() -> None:
    from types import SimpleNamespace as Row
    from _recoil.lib.authored_icf import validate_lifecycle_object_members
    from _recoil.lib.progress import ProgressError

    names = ("??1First@@UAE@XZ", "??1Second@@UAE@XZ")
    definitions = [Row(name=name, index=index, section_number=index, storage_class=2, type=0x20,
                       section_definition_selection=None, section_definition_association=None)
                   for index, name in enumerate(names, 1)]
    sections = [Row(name=".text", section_number=index, storage_class=3, type=0,
                    section_definition_selection=2, section_definition_association=0)
                for index in (1, 2)]
    data = b"\xc7\x01\0\0\0\0\xc3"
    relocation = Row(offset=2, type=6, symbol_name="??_7Root@@6B@")
    bodies = {name: Row(start=0, end=7, data=data, relocations=(relocation,)) for name in names}
    obj = Row(symbols=definitions + sections, function_bytes=bodies.__getitem__, relocations_by_section={},
              section=lambda index: Row(index=index, name=".text", characteristics=0x1020, raw_data=data))
    validate_lifecycle_object_members(obj, names)
    for mutation in ("missing", "different-bytes", "different-target", "different-addend", "association", "selection"):
        from copy import deepcopy
        bad = deepcopy(obj)
        bad_bodies = deepcopy(bodies)
        bad.function_bytes = bad_bodies.__getitem__
        if mutation == "missing":
            bad.symbols.pop(1)
        elif mutation == "different-bytes":
            bad_bodies[names[1]].data = b"\xc7\x02\0\0\0\0\xc3"
        elif mutation == "different-target":
            bad_bodies[names[1]].relocations = (Row(offset=2, type=6, symbol_name="??_7Other@@6B@"),)
        elif mutation == "different-addend":
            bad_bodies[names[1]].data = b"\xc7\x01\x04\0\0\0\xc3"
        elif mutation == "association":
            bad.symbols.append(Row(name=".xdata", section_number=3, storage_class=3, type=0,
                                  section_definition_selection=5, section_definition_association=2))
        else:
            bad.symbols[-1].section_definition_selection = 1
        with pytest.raises(ProgressError):
            validate_lifecycle_object_members(bad, names)
    with pytest.raises(ProgressError, match="fresh"):
        validate_lifecycle_object_members(None, names)

    # FPO records must differ only in their exact self-reference relocation.
    from copy import deepcopy
    fpo_obj = deepcopy(obj)
    for index in (1, 2):
        fpo_obj.symbols.append(Row(name=".debug$F", section_number=index + 2, storage_class=3, type=0,
                                  section_definition_selection=5, section_definition_association=index))
        fpo_obj.relocations_by_section[index + 2] = (Row(offset=0, type=7, symbol_name=names[index - 1], symbol_index=index),)
    fpo_obj.section = lambda index: (obj.section(index) if index <= 2 else
                                    Row(index=index, name=".debug$F", characteristics=0x42101048, raw_data=b"\0" * 16))
    validate_lifecycle_object_members(fpo_obj, names)
    fpo_obj.relocations_by_section[4][0].symbol_name = names[0]
    with pytest.raises(ProgressError, match="FPO"):
        validate_lifecycle_object_members(fpo_obj, names)


def test_lifecycle_icf_header_mirrors_bind_generated_and_explicit_destructors(tmp_path: Path) -> None:
    from _recoil.lib.authored_icf import validate_authored_icf_source_mirrors
    from _recoil.lib.progress import ProgressError

    source = tmp_path / "src"
    source.mkdir()
    (source / "unit.cpp").write_text('#include "unit.h"\n', encoding="utf-8")
    header = source / "unit.h"
    identity = "recoil:logical-function:0x401000:unit"
    anchor = "recoil:anchor:unit"
    alias = {
        "object_symbol": "??1Unit@@UAE@XZ",
        "source_traceability": {"source_edges": [{
            "relation": "emits", "anchor_id": anchor,
            "emission_context": {"translation_unit": "src/unit.h"},
        }]},
        "source_generation": {"kind": "implicit-destructor", "class_name": "Unit",
                              "translation_units": ["src/unit.cpp"]},
    }
    comment = f"/**\n * @recoil-anchor {anchor}\n * @recoil-artifact emits .text {identity}: Generated lifetime.\n */\n"
    header.write_text(comment + "struct Unit { virtual void Method(); };\n", encoding="utf-8")
    assert validate_authored_icf_source_mirrors({identity: alias}, repo_root=tmp_path) == {identity: "src/unit.h"}
    for declaration in ("~Unit();", "virtual ~Unit() {}"):
        header.write_text(comment + "struct Unit { " + declaration + " };\n", encoding="utf-8")
        with pytest.raises(ProgressError, match="destructor"):
            validate_authored_icf_source_mirrors({identity: alias}, repo_root=tmp_path)
    alias["source_generation"]["kind"] = "inline-destructor"
    alias["source_traceability"]["source_edges"][0]["relation"] = "defines"
    header.write_text("struct Unit {\n" + comment.replace("emits", "defines") + "virtual ~Unit() {}\n};\n", encoding="utf-8")
    assert validate_authored_icf_source_mirrors({identity: alias}, repo_root=tmp_path)
    alias["source_generation"]["class_name"] = "Other"
    with pytest.raises(ProgressError, match="bind"):
        validate_authored_icf_source_mirrors({identity: alias}, repo_root=tmp_path)


def test_lifecycle_icf_projection_requires_explicit_witness_and_checks_whole_tu(monkeypatch: pytest.MonkeyPatch) -> None:
    from _recoil.lib import authored_icf as icf
    from _recoil.lib.progress import ProgressError

    physical = "recoil:function:0x401000"
    aliases = {"first": {"object_symbol": "??1First@@UAE@XZ"},
               "second": {"object_symbol": "??1Second@@UAE@XZ"}}
    for name, alias in aliases.items():
        alias["source_generation"] = {"translation_units": ["src/unit.cpp"]}
        alias["source_traceability"] = {"source_edges": [{"relation": "emits", "emission_context": {"translation_unit": "src/unit.h"}}]}
    group = {"model": icf.AUTHORED_ICF_GROUP_MODEL, "source_model": icf.AUTHORED_ICF_LIFECYCLE_SOURCE_MODEL}
    data = {"symbols": {physical: {"icf_address_group": group, "logical_aliases": aliases}}}
    monkeypatch.setattr(icf, "audit_authored_icf_groups", lambda data: [])
    checked = []
    monkeypatch.setattr(icf, "validate_lifecycle_object_members", lambda obj, names: checked.append((obj, names)))
    selector = icf.select_authored_icf_translation_unit_object_symbol
    for witness in (None, "??1Absent@@UAE@XZ"):
        with pytest.raises(ProgressError, match="witness"):
            selector(data, physical_symbol_id=physical, translation_unit="src/unit.cpp", object_witness=witness)
    assert selector(data, physical_symbol_id=physical, translation_unit="src/unit.cpp",
                    object_witness=aliases["second"]["object_symbol"], coff_object="fresh") == ("second", aliases["second"]["object_symbol"])
    assert checked == [("fresh", tuple(alias["object_symbol"] for alias in aliases.values()))]
    with pytest.raises(ProgressError):
        selector(data, physical_symbol_id=physical, translation_unit="src/other.cpp",
                 object_witness=aliases["second"]["object_symbol"], coff_object="fresh")


def test_coff_lifecycle_cli_reports_map_observations_without_acceptance(monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]) -> None:
    import json
    from types import SimpleNamespace as Row
    from _recoil.commands import coff_lifecycle

    monkeypatch.setattr(coff_lifecycle, "configure_stdio", lambda: None)
    monkeypatch.setattr(coff_lifecycle, "inventory", lambda path, names: {"acceptance": False, "members": names})
    monkeypatch.setattr(coff_lifecycle, "parse_link_map", lambda path: Row(symbols=(
        Row(symbol="_unit", address=0x402000, is_function=True),
        Row(symbol="_unit", address=0x403000, is_function=False),
    )))
    monkeypatch.setattr(sys, "argv", ["coff-lifecycle", "--object", "unit.obj", "--symbol", "_unit", "--map", "unit.map"])
    assert coff_lifecycle.main() == 0
    report = json.loads(capsys.readouterr().out)
    assert report["acceptance"] is False
    assert report["maps"] == {"unit.map": {"_unit": ["0x402000"]}}


def test_linked_presence_rejects_missing_data_and_ambiguous_function_symbols() -> None:
    from dataclasses import replace
    from _recoil.commands.vc5_build import (
        LinkedMapSymbol, ParsedLinkMap, authored_linked_presence_report,
    )
    from _recoil.commands.vc5_verify import VerifyFunction

    required = (VerifyFunction("0x401000", "_entry", "entry"),)

    def symbol(address: int, name: str = "_entry", function: bool = True) -> LinkedMapSymbol:
        return LinkedMapSymbol(1, 0, name, address, ("f",) if function else (), "unit.obj", "Publics by Value")

    # Different RVA and aliases at the same RVA are valid for presence alone.
    present = ParsedLinkMap(0x400000, (symbol(0x405000), symbol(0x405000, "_alias")))
    assert authored_linked_presence_report(required, present)["passed"]
    patterned = (replace(required[0], symbol="_old_spelling", symbol_regex="_entry"),)
    assert authored_linked_presence_report(patterned, present)["passed"]
    assert not authored_linked_presence_report(
        (replace(required[0], symbol_regex="_other"),), present
    )["passed"]
    local_a = replace(symbol(0x405000), source="Static symbols")
    local_b = replace(local_a, address=0x406000, object="other.obj")
    locals_map = ParsedLinkMap(0x400000, (local_a, local_b))
    assert not authored_linked_presence_report(required, locals_map)["passed"]
    scope = {"0x401000": {"unit.obj"}}
    assert authored_linked_presence_report(required, locals_map, static_object_scopes=scope)["passed"]
    assert not authored_linked_presence_report(
        required, ParsedLinkMap(0x400000, (local_b,)), static_object_scopes=scope
    )["passed"]
    assert not authored_linked_presence_report(
        required, ParsedLinkMap(0x400000, (symbol(0x405000), symbol(0x406000))),
        static_object_scopes=scope,
    )["passed"]
    for symbols in ((), (symbol(0x405000, function=False),), (symbol(0x405000), symbol(0x406000))):
        report = authored_linked_presence_report(required, ParsedLinkMap(0x400000, symbols))
        assert not report["passed"]
        assert report["divergences"][0]["symbol_id"] == "recoil:function:0x401000"
    with pytest.raises(ValueError, match="nonempty"):
        authored_linked_presence_report((), present)


def _check_byte_binding_translation_unit_projection(monkeypatch):
    from dataclasses import replace
    from types import SimpleNamespace as Row
    from _recoil.commands import live_byte_verify as byte

    first = vc5_verify.VerifyFunction("0x401000", "_first", "first")
    second = vc5_verify.VerifyFunction("0x402000", "_second", "second")
    folded = replace(second, symbol="_folded", logical_identity_key="logical:folded")
    other_alias = replace(second, logical_identity_key="logical:other")
    unmapped = vc5_verify.VerifyFunction("0x403000", "_unmapped", "unmapped")
    target = Row(name="units", manifest_path=Path("tools/units.json"),
                 source_from="src/primary.cpp", functions=(first, second, folded, other_alias),
                 translation_unit_function_order=(
                     Row(source_from="src/primary.cpp", functions=(first,)),
                     Row(source_from="src/second.cpp", functions=(second,)),
                     Row(source_from="src/third.cpp", functions=(folded,))),
                 linked_function_intervals=(Row(functions=(second, unmapped)),))
    symbols = {f"function:{address}": dict(address=address, kind="function",
                verification_target_ids=["units"]) for address in
               (first.address, second.address, unmapped.address)}
    collections = {"symbols": symbols, "verification_targets": {
        "units": {"kind": "vc5", "registration": {"name": "units"}}}}
    document = Row(collection=lambda name: collections[name])
    with monkeypatch.context() as patch:
        patch.setattr(byte, "load_manifests", lambda *args, **kwargs: [target])
        def sources():
            bindings = byte._bindings(document, Path("tools"))
            return {(binding.function.address, binding.function.symbol,
                     binding.function.logical_identity_key or "", binding.source_from)
                    for group in bindings.values() for binding in group}
        expected = {
            (first.address, "_first", "", "src/primary.cpp"),
            (second.address, "_second", "", "src/second.cpp"),
            (folded.address, "_folded", "logical:folded", "src/third.cpp"),
            (other_alias.address, "_second", "logical:other", "src/primary.cpp"),
            (unmapped.address, "_unmapped", "", "src/primary.cpp"),
        }
        assert sources() == expected
        # All explicit inline emission sites remain mandatory; a second site
        # is neither discarded nor replaced by the unrelated default TU.
        target.translation_unit_function_order += (
            Row(source_from="src/fourth.cpp", functions=(second,)),)
        expected.add((second.address, "_second", "", "src/fourth.cpp"))
        assert sources() == expected
        # Clearing explicit placements restores ordinary single-TU selection.
        target.translation_unit_function_order = ()
        assert sources() == {(address, symbol, identity, "src/primary.cpp")
                             for address, symbol, identity, source in expected}
        # A registered identity without any source still fails selection.
        target.source_from = ""
        bindings = byte._bindings(document, Path("tools"))
        with pytest.raises(byte.LiveByteError, match="no source-backed byte target"):
            byte._select_bindings(bindings, dict(address=first.address,
                scope_ids=[f"function:{first.address}"]))


def test_linked_presence_census_rejects_stale_or_uncovered_authority(monkeypatch: pytest.MonkeyPatch) -> None:
    from dataclasses import replace
    from types import SimpleNamespace
    from _recoil.commands import vc5_build
    from _recoil.lib import verification_targets

    function = vc5_verify.VerifyFunction("0x401000", "_entry", "entry", pipeline_class="authored", authored_order_role="authored-body")
    registration = {"manifest_path": "tools/unit.json"}
    current = {"registration": dict(registration)}
    target = SimpleNamespace(functions=(function,), translation_unit_function_order=(), linked_function_intervals=())
    collections = {"verification_targets": {"unit": {"registration": registration,
        "binary": "recoil", "kind": "vc5", "registered_addresses": [function.address]}},
        "symbols": {"recoil:function:0x401000": dict(binary="recoil", address=function.address,
            pipeline_class="authored", authored_order_role="authored-body", verification_target_ids=["unit"])}}
    document = SimpleNamespace(
        authored_call_contract_slices=lambda binary: [{"addresses": [function.address], "target_ids": ["unit"]}],
        collection=lambda name: collections[name],
    )
    monkeypatch.setattr(vc5_build, "load_repository_path_inventory", lambda root: None)
    monkeypatch.setattr(vc5_build, "resolve_repository_file", lambda *args, **kwargs: SimpleNamespace(physical_path=Path("unit.json")))
    monkeypatch.setattr(vc5_build, "load_vc5_verify_manifest", lambda path: target)
    monkeypatch.setattr(verification_targets, "vc5_target_registration", lambda path: ("unit", current))
    assert vc5_build.required_authored_linked_functions(document) == (function,)
    # The accepted census, not a second manifest-role filter, owns membership.
    # Compiler lifecycle helpers can remain explicit census obligations.
    lifecycle = replace(function, pipeline_class="non-authored", authored_order_role="non-authored")
    outside = replace(function, address="0x401020", symbol="_outside")
    target.functions = (lifecycle, outside)
    assert vc5_build.required_authored_linked_functions(document) == (lifecycle,)
    target.functions = (replace(lifecycle, required_presence=False),)
    with pytest.raises(ValueError, match="required presence"):
        vc5_build.required_authored_linked_functions(document)
    target.functions = (function,)
    current["registration"] = {"manifest_path": "tools/stale.json"}
    with pytest.raises(ValueError, match="stale"):
        vc5_build.required_authored_linked_functions(document)
    current["registration"] = dict(registration)
    target.functions = ()
    with pytest.raises(ValueError, match="uncovered"):
        vc5_build.required_authored_linked_functions(document)
    target.functions = (replace(function, required_presence=False),)
    with pytest.raises(ValueError, match="required presence"):
        vc5_build.required_authored_linked_functions(document)

    # Playground membership remains complete while order acceptance is pending.
    def pending_order(binary):
        raise ValueError("order proof pending")
    document.authored_call_contract_slices = pending_order
    target.functions = (function,)
    assert vc5_build.required_authored_linked_functions(document, playground_only=True) == (function,)
    with pytest.raises(ValueError, match="order proof pending"):
        vc5_build.required_authored_linked_functions(document)
    target.functions = (function, replace(function, symbol="_other_profile"))
    union = vc5_build.required_authored_linked_functions(document, playground_only=True)
    assert len(union) == 1
    def linked(name, address):
        return vc5_build.LinkedMapSymbol(1, 0, name, address, ("f",), "unit.obj", "Publics by Value")
    assert vc5_build.authored_linked_presence_report(union,
        vc5_build.ParsedLinkMap(0x400000, (linked("_other_profile", 0x405000),)))["passed"]
    assert not vc5_build.authored_linked_presence_report(union,
        vc5_build.ParsedLinkMap(0x400000, (linked("_entry", 0x405000), linked("_other_profile", 0x406000))))["passed"]
    target.functions = (function, replace(function, symbol="_logical_member", logical_identity_key="logical:other"))
    assert len(vc5_build.required_authored_linked_functions(document, playground_only=True)) == 2
    target.functions = (function,)
    member = collections["symbols"]["recoil:function:0x401000"]
    for references in ([], ["unit", "unit"], ["missing"]):
        member["verification_target_ids"] = references
        with pytest.raises(ValueError, match="registration"):
            vc5_build.required_authored_linked_functions(document, playground_only=True)
    member["verification_target_ids"] = ["unit"]
    collections["verification_targets"]["unit"]["registered_addresses"] = []
    with pytest.raises(ValueError, match="absent from its registration"):
        vc5_build.required_authored_linked_functions(document, playground_only=True)
    collections["verification_targets"]["unit"]["registered_addresses"] = [function.address]
    current["registration"] = {"manifest_path": "tools/stale.json"}
    with pytest.raises(ValueError, match="stale"):
        vc5_build.required_authored_linked_functions(document, playground_only=True)
    current["registration"] = dict(registration)
    # A retired overlapping diagnostic cannot select a name or excuse a missing
    # body. A separate current registration must cover the complete population.
    collections["verification_targets"]["old"] = dict(binary="recoil", kind="vc5",
        registration={"manifest_path": "tools/removed.json", "function_addresses": [function.address]})
    member["verification_target_ids"] = ["unit", "old"]
    with monkeypatch.context() as patch:
        def resolve(path, **kwargs):
            if path == "tools/removed.json":
                raise ValueError("retired diagnostic")
            return SimpleNamespace(physical_path=Path("unit.json"))
        patch.setattr(vc5_build, "resolve_repository_file", resolve)
        assert vc5_build.required_authored_linked_functions(document, playground_only=True) == (function,)
        member["verification_target_ids"] = ["old"]
        with pytest.raises(ValueError, match="uncovered"):
            vc5_build.required_authored_linked_functions(document, playground_only=True)
    member["verification_target_ids"] = ["unit"]
    production = replace(function, symbol="_production")
    production_registration = {"manifest_path": "tools/canonical.json"}
    collections["verification_targets"]["canonical"] = dict(binary="recoil", kind="vc5",
        registration=production_registration, registered_addresses=[function.address])
    member["verification_target_ids"] = ["unit", "canonical"]
    with monkeypatch.context() as patch:
        patch.setattr(vc5_build, "resolve_repository_file", lambda path, **kwargs:
            SimpleNamespace(physical_path=Path(path)))
        patch.setattr(verification_targets, "vc5_target_registration", lambda path:
            ("canonical", {"registration": production_registration}) if path.name == "canonical.json" else ("unit", current))
        canonical_target = SimpleNamespace(functions=(production,), translation_unit_function_order=(),
            linked_function_intervals=(), compile_context_from="tools/_recoil/config/vc5_final_build.json")
        patch.setattr(vc5_build, "load_vc5_verify_manifest", lambda path:
            canonical_target if path.name == "canonical.json" else target)
        selected = vc5_build.required_authored_linked_functions(document, playground_only=True)
        assert selected == (production,)
        # A production function missing from the map cannot fall back to a
        # standalone diagnostic's still-present old spelling.
        assert not vc5_build.authored_linked_presence_report(selected,
            vc5_build.ParsedLinkMap(0x400000, (linked("_entry", 0x405000),)))["passed"]
    member["verification_target_ids"] = ["unit"]
    target.functions = ()
    with pytest.raises(ValueError, match="uncovered"):
        vc5_build.required_authored_linked_functions(document, playground_only=True)
    target.functions = (replace(function, required_presence=False),)
    with pytest.raises(ValueError, match="required presence"):
        vc5_build.required_authored_linked_functions(document, playground_only=True)

    _check_byte_binding_translation_unit_projection(monkeypatch)


def test_playground_request_rejects_reuse_profiles_and_partial_modes(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    from types import SimpleNamespace as Row
    from _recoil.commands import vc5_build as build

    monkeypatch.setattr(build, "REPO_ROOT", tmp_path)
    config = Row(
        manifest_path=build.DEFAULT_MANIFEST, output_exe="Recoil.exe",
        playtest_output_exe=build.DEFAULT_PLAYTEST_OUTPUT, diagnostic_only=False,
        diagnostic_kind="", compile_profile="", link_profile="", library_profile="",
        build_dir=tmp_path / "build/live-validation/fresh", build_dir_explicit=True,
    )
    options = dict(clean=False, compile_only=False, linked_order_only=False,
                   linkability_only=False, keep_going=False, order_targets=(),
                   progress_path=build.DEFAULT_PROGRESS)
    build.validate_playground_build_request(config, **options)
    for name, value in (("clean", True), ("compile_only", True), ("linked_order_only", True),
                        ("linkability_only", True), ("keep_going", True),
                        ("order_targets", ("unit",)), ("progress_path", tmp_path / "other.db")):
        with pytest.raises(ValueError):
            build.validate_playground_build_request(config, **{**options, name: value})
    for name, value in (("diagnostic_only", True), ("compile_profile", "probe"),
                        ("link_profile", "probe"), ("library_profile", "probe"),
                        ("build_dir_explicit", False), ("output_exe", "messages.dll"),
                        ("playtest_output_exe", tmp_path / "elsewhere.exe"),
                        ("build_dir", tmp_path / "outside"), ("build_dir", tmp_path / "build/live-validation")):
        with pytest.raises(ValueError):
            build.validate_playground_build_request(Row(**{**vars(config), name: value}), **options)
    config.build_dir.mkdir(parents=True)
    with pytest.raises(ValueError, match="fresh absent"):
        build.validate_playground_build_request(config, **options)


def test_playground_completion_requires_presence_and_deployment_without_acceptance(monkeypatch: pytest.MonkeyPatch) -> None:
    from types import SimpleNamespace as Row
    from _recoil.commands import vc5_build as build
    from _recoil.commands import startup_contract

    config = Row(playtest_output_exe=Path("playground/candidate.exe"))
    paths = Row(map_path=Path("fresh/candidate.map"), exe_path=Path("fresh/candidate.exe"))
    reports, deployments = [], []
    presence, deployed = {"passed": False}, {"attempted": True, "updated": True}
    startup = {"passed": False}
    monkeypatch.setattr(startup_contract, "check_startup_contract", lambda *args: startup)
    def check_presence(*args, playground_only=False):
        assert playground_only
        return presence
    monkeypatch.setattr(build, "required_authored_presence_at_map", check_presence)
    monkeypatch.setattr(build, "compile_profile_rows", lambda config: [])
    monkeypatch.setattr(build, "write_summary", lambda *args, **kwargs: reports.append(kwargs["acceptance"]))
    monkeypatch.setattr(build, "deploy_playtest_candidate", lambda *args: deployments.append(args) or deployed)
    kwargs = dict(progress_path=Path("canonical.db"), required_order_targets=("later-order",), canonical_include_trace=None)
    assert build.finish_playground_build(config, paths, [], **kwargs) == 1
    assert not deployments and reports[-1]["failure_stage"] == "linked-presence"
    presence["passed"] = True
    assert build.finish_playground_build(config, paths, [], **kwargs) == 1
    assert not deployments and reports[-1]["failure_stage"] == "startup-contract"
    startup["passed"] = True
    deployed["updated"] = False
    assert build.finish_playground_build(config, paths, [], **kwargs) == 1
    assert reports[-1]["failure_stage"] == "deployment"
    deployed["updated"] = True
    assert build.finish_playground_build(config, paths, [], **kwargs) == 0
    report = reports[-1]
    assert report["success"] and report["kind"] == "playground-build"
    assert not any(report[key] for key in ("accepts_order", "accepts_bytes", "accepts_final_image", "required_order_targets_passed"))
    assert report["effective_order_targets"] == [] and report["order_reports"] == []


def test_playground_parser_is_explicit_and_presence_authority_errors_fail_closed(monkeypatch: pytest.MonkeyPatch) -> None:
    from _recoil.commands import vc5_build as build

    assert not build.build_parser().parse_args([]).playground_only
    assert build.build_parser().parse_args(["--playground-only"]).playground_only
    assert not build.build_parser().parse_args(["--linkability-only"]).playground_only
    def stale(path: Path) -> None:
        raise ValueError("stale census")
    monkeypatch.setattr(build.ProgressDocument, "load", stale)
    assert build.required_authored_presence_at_map(Path("candidate.map"), Path("canonical.db")) == {
        "passed": False, "error": "stale census",
    }


def test_byte_artifact_build_is_non_deploying_and_requires_complete_success(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch,
) -> None:
    import json
    from types import SimpleNamespace as Row
    from _recoil.commands import live_byte_verify as byte

    good = dict(
        kind="final-build-diagnostic", diagnostic_kind="whole-program-linkability",
        success=True, fresh_build=True, reuse=False, compile_succeeded=True,
        coff_alias_sources_succeeded=True, resource_succeeded=True,
        link_succeeded=True, candidate_available=True,
        linked_order_evaluation_suppressed=True, playtest_deployment_suppressed=True,
        accepts_linked_order=False, accepts_bytes=False, accepts_final_image=False,
        candidate_expected_truth=False, authored_byte_eligible=False,
        playtest_deploy={"attempted": False, "updated": False},
    )
    commands = []

    def invoke(mode: str, report: dict, *, returncode: int = 0, omit: str = "") -> None:
        root = tmp_path / str(len(commands))
        config = Row(sources=(Path("unit.cpp"),))
        paths = Row(summary_path=root / "summary.json", exe_path=root / "unit.exe",
                    map_path=root / "unit.map")
        obj = root / "unit.obj"
        monkeypatch.setattr(byte, "load_config", lambda path: config)
        monkeypatch.setattr(byte, "with_explicit_build_dir", lambda config, path: config)
        monkeypatch.setattr(byte, "build_paths", lambda config: paths)
        monkeypatch.setattr(byte, "object_path", lambda *args: obj)

        def produce(command: list[str], **kwargs: object) -> Row:
            commands.append(command)
            root.mkdir()
            for path in (obj, paths.exe_path, paths.map_path):
                if path.name != omit:
                    path.write_bytes(b"fresh candidate")
            paths.summary_path.write_text(json.dumps(report), encoding="utf-8")
            return Row(returncode=returncode, stdout="", stderr="")

        monkeypatch.setattr(byte.subprocess, "run", produce)
        byte._run_fresh_build(Row(mode=mode, final_config=Path("canonical.json")), root)

    for mode in ("authored", "linked"):
        invoke(mode, good)
        assert "--linkability-only" in commands[-1]
        assert "--clean" in commands[-1]
        assert "--playground-only" not in commands[-1]
        assert "--compile-only" not in commands[-1]
    invoke("object", dict(kind="compile-only-diagnostic", success=True))
    assert "--compile-only" in commands[-1]
    assert "--compile-only-skip-linked-order" in commands[-1]
    assert "--linkability-only" not in commands[-1]

    # These summaries describe artifact production only, never byte acceptance.
    for field in (
        "success", "fresh_build", "compile_succeeded", "coff_alias_sources_succeeded",
        "resource_succeeded", "link_succeeded", "candidate_available",
        "linked_order_evaluation_suppressed", "playtest_deployment_suppressed",
        "reuse", "accepts_linked_order", "accepts_bytes", "accepts_final_image",
        "candidate_expected_truth",
    ):
        with pytest.raises(byte.LiveByteError):
            invoke("authored", {**good, field: not good[field]})
    for changes in (
        {"kind": "final-build"}, {"diagnostic_kind": "other"},
        {"playtest_deploy": {"attempted": True, "updated": False}},
        {"playtest_deploy": {"attempted": False, "updated": True}},
        {"playtest_deploy": None},
    ):
        with pytest.raises(byte.LiveByteError):
            invoke("linked", {**good, **changes})
    with pytest.raises(byte.LiveByteError, match="non-deploying"):
        invoke("authored", {**good, "failure_stage": "linked-order"}, returncode=1)
    for missing in ("unit.obj", "unit.exe", "unit.map"):
        with pytest.raises(byte.LiveByteError, match="missing"):
            invoke("authored", good, omit=missing)

def _exercise_native_array_destruction(monkeypatch):
    from copy import deepcopy
    from types import SimpleNamespace as Row
    from _recoil.commands import native_array_cleanup as cleanup, live_byte_verify as live

    selector = bytearray(bytes.fromhex("68 00204000 6a06 8d86 f0b80000 6a40 50 c644242c08 e8 00000000"))
    row = dict(address="0x401000", end_exclusive="0x401040")
    with monkeypatch.context() as patch:
        patch.setattr(live, "_pe_bytes", lambda image, address, size: bytes(selector[:size]))
        def select(): return cleanup.retail_selector(None, row, 1)
        expected = dict(kind="destruction", offset=1, target=0x402000, count=6,
                        stride=64, member_offset=0xb8f0, helper=0x40101a)
        assert select() == expected
        for index in (0, 5, 7, 8, 13, 15, 16, 17, 18, 21):
            selector[index] ^= 1
            with pytest.raises(ValueError): select()
            selector[index] ^= 1
        for index in (6, 14):
            old = selector[index]
            selector[index] = 0
            with pytest.raises(ValueError): select()
            selector[index] = old
        for offset in (0, -1, True, 40):
            with pytest.raises(ValueError): cleanup.retail_selector(None, row, offset)

    helper_code = bytearray(115)
    helper_code[:3] = bytes.fromhex("55 8b ec")
    helper_code[40:77] = bytes.fromhex(
        "8b4d10 8b7d0c 0fafcf 8b7508 03f1 897508 8945fc "
        "ff4d10 780c 2bf7 897508 8bce ff5514 ebef")
    helper_code[91] = 0xe8
    helper_code[92:96] = struct.pack("<i", 0x402000-0x401060)
    helper_code[112:] = bytes.fromhex("c2 10 00")
    forwarder = bytearray(bytes.fromhex("8b45e4 85c0 750f 8b5514 52 8b4510 50 57 56 e8 00000000 c3"))
    forwarder[18:22] = struct.pack("<i", 0x403000-0x402016)
    checker = bytearray(106)
    checker[:3] = bytes.fromhex("55 8b ec")
    checker[45:64] = bytes.fromhex("ff4d10 781e 8b4d08 2b4d0c 894d08 ff5514 ebed")
    checker[103:] = bytes.fromhex("c2 10 00")
    memory = {0x401000: helper_code, 0x402000: forwarder, 0x403000: checker}
    with monkeypatch.context() as patch:
        patch.setattr(live, "_pe_bytes", lambda image, address, size: bytes(memory.get(address, bytes(size))[:size]))
        def abi(): return cleanup.prove_destructor_helper_abi(None, 0x401000)
        assert abi() == dict(destructor=0x401000, cleanup=0x402000, checker=0x403000,
                            callback_receiver="complete-element-in-ecx", callback_stack_arguments=0)
        for data, offset in ((helper_code, 64), (helper_code, 70), (helper_code, 74),
                             (helper_code, 92), (helper_code, 113), (forwarder, 9),
                             (forwarder, 18), (checker, 52), (checker, 60), (checker, 104)):
            data[offset] ^= 1
            with pytest.raises(ValueError): abi()
            data[offset] ^= 1

    constructor = dict(source=dict(symbol_id="ctor", object_symbol="??0Parent@@QAE@XZ"),
        selector=dict(offset=10, target=0x402000, count=6, stride=64, member_offset=0xb8f0, constructor=0x403000),
        generation=dict(parent_class="Parent"), owner_id="owner", evidence_ids=["evidence"])
    binding = dict(schema=cleanup.SCHEMA, reviewed=True, reason="scoped review", context=constructor)
    rows = {"ctor": {cleanup.FIELD: [binding]}}
    document = Row(collection=lambda name: rows)
    with monkeypatch.context() as patch:
        patch.setattr(cleanup, "binding_context", lambda *args: deepcopy(constructor))
        def pair(selection=expected):
            return cleanup.paired_construction(document, {}, "??1Parent@@UAE@XZ", selection, None, None)
        assert pair() == constructor
        for key in ("target", "count", "stride", "member_offset"):
            with pytest.raises(ValueError, match="unambiguous"):
                pair(dict(expected, **{key: expected[key]+1}))
        rows["ctor"][cleanup.FIELD].append(deepcopy(binding))
        with pytest.raises(ValueError, match="unambiguous"): pair()
        rows["ctor"][cleanup.FIELD].pop()
        binding["reviewed"] = False
        with pytest.raises(ValueError, match="invalid reviewed"): pair()
        binding["reviewed"] = True
        patch.setattr(cleanup, "binding_context", lambda *args: dict(constructor, owner_id="changed"))
        with pytest.raises(ValueError, match="stale"): pair()

    destructor = Row(name="??1Element@@UAE@XZ", index=1, storage_class=2, type=0x20,
                     section_number=2, section_definition_selection=None)
    definition = Row(name=".text", section_number=2, section_definition_selection=2)
    relocation = Row(offset=4, type=6, symbol_name=destructor.name, symbol_index=1)
    helper = Row(offset=25, type=20, symbol_name="??_M@YGXPAXIHP6EX0@Z@Z")
    table = Row(offset=2, type=6, symbol_name="??_7Base@@6B@")
    parent = Row(start=0, data=bytes(32), relocations=[relocation, helper])
    body = Row(symbol=destructor.name, start=0, section_index=2,
               data=b"\xc7\x01"+bytes(4)+b"\xc3", relocations=[table])
    obj = Row(symbols=[destructor, definition], symbols_by_index={1: destructor},
              function_bytes=lambda name: body, section=lambda index: Row(characteristics=0x1000))
    expected = dict(offset=4, native_cleanup=dict(selector=dict(kind="destruction"),
        generation=dict(destructor_symbol=destructor.name), dependencies=dict(
            table=dict(object_symbol=table.symbol_name), helper=dict(object_symbol=helper.symbol_name))))
    def prove(): return cleanup.prove_object_cleanup(obj, parent, relocation, expected)
    assert prove() is body
    for field, value in (("offset", 9), ("type", 6), ("symbol_name", "??_L@YGXPAXIHP6EX0@Z1@Z")):
        old = getattr(helper, field)
        setattr(helper, field, value)
        with pytest.raises(ValueError): prove()
        setattr(helper, field, old)
    parent.data = bytes(25)+b"\x01"+bytes(6)
    with pytest.raises(ValueError): prove()
