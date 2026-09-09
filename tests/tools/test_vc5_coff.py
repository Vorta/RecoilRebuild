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


def test_retail_decoder_proves_remapped_and_direct_tables_in_one_trailing_island():
    from _recoil.commands.relocation_expectations import decode_x86_operand_sites

    base, code = mapped_switch_bytes()
    for tail in (b"", b"\x90\x90"):
        sites, unresolved = decode_x86_operand_sites(code + tail, function_address=base)
        assert not unresolved
        assert [site.offset for site in sites if site.kind == "switch-table-entry"] == [56, 60, 64, 72, 76]
        assert [site.offset for site in sites if site.kind == "absolute32"] == [17, 24, 36]
        assert all(site.relocation_type == 6 for site in sites)


def test_retail_decoder_rejects_unproven_remap_flow_extents_and_targets():
    from _recoil.commands.relocation_expectations import decode_x86_operand_sites

    base, code = mapped_switch_bytes()
    mutations = (
        (13, b"\x90\x90"),  # Upper bits of the dispatch index are not cleared.
        (1, b"\xfb"),  # The bound applies to a different register.
        (3, bytes.fromhex("8b 44 24 04 90 90 90 90")),  # Bounded index is overwritten.
        (3, bytes.fromhex("83 c0 01 90 90 90 90 90")),  # Bound flags are clobbered.
        (68, b"\x03"),  # Map selects an entry outside the pointer table.
        (56, struct.pack("<I", base + 41)),  # Target is inside an instruction.
        (56, struct.pack("<I", base + 52)),  # Target is alignment before the data island.
        (36, struct.pack("<I", base + 68)),  # The second table overlaps the map.
        (52, b"\x40"),  # Alignment is executable work, not proven padding.
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
    _exercise_native_eh_role_witnesses(monkeypatch)
    _exercise_native_array_cleanup(monkeypatch)
    _exercise_native_array_destruction(monkeypatch)
    _exercise_native_import_references(monkeypatch)


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


def test_linked_presence_census_rejects_stale_or_uncovered_authority(monkeypatch: pytest.MonkeyPatch) -> None:
    from dataclasses import replace
    from types import SimpleNamespace
    from _recoil.commands import vc5_build
    from _recoil.lib import verification_targets

    function = vc5_verify.VerifyFunction("0x401000", "_entry", "entry", pipeline_class="authored", authored_order_role="authored-body")
    registration = {"manifest_path": "tools/unit.json"}
    current = {"registration": dict(registration)}
    target = SimpleNamespace(functions=(function,), translation_unit_function_order=(), linked_function_intervals=())
    document = SimpleNamespace(
        authored_call_contract_slices=lambda binary: [{"addresses": [function.address], "target_ids": ["unit"]}],
        collection=lambda name: {"unit": {"registration": registration}},
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
    monkeypatch.setattr(build, "required_authored_presence_at_map", lambda *args: presence)
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
