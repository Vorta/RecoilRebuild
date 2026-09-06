from __future__ import annotations

import pytest


@pytest.mark.parametrize("element", ["H", "URecord@@"])
def test_value_vector_noop_provider_requires_complete_coff_and_supplier_proof(
    element, monkeypatch, tmp_path,
):
    from copy import deepcopy
    from dataclasses import replace
    from types import SimpleNamespace as Row
    from _recoil.commands import call_contract_verify as cc
    from _recoil.commands.asm_verify import Instruction

    name = f"?_Destroy@?$vector@{element}V?$allocator@{element}@std@@@std@@IAEXPA{element}0@Z"
    env = tmp_path / "compiler" / "env.cmd"
    monkeypatch.setattr(cc, "compiler_env_path", lambda *_: env)
    symbol = Row(index=3, name=name, section_number=2, storage_class=2,
        symbol_type=0x20, section_name=".text", section_size=16, value=0,
        natural_end=16, section_characteristics=0x1020)
    relocation = Row(offset=1, type=0x14, symbol_name=name, symbol_index=3)
    caller = cc.CandidateCallerDefinition(symbol="?Caller@@YAXXZ",
        data=b"\xe8\0\0\0\0\xc3", relocations=(relocation,),
        relocation_mask=(False, True, True, True, True, False),
        undefined_external_functions=(), defined_external_functions=(name,),
        coff_symbols=(symbol,), section_index=1, section_start=0, section_end=6)
    helper = cc.CandidateTuLocalFunctionDefinition(symbol=name,
        data=b"\xc2\x08\0" + b"\x90" * 13, relocations=(),
        relocation_mask=(False,) * 16, section_size=16,
        section_external_functions=(name,), section_is_comdat=True,
        comdat_selection=2,
        source_provenance=str(env.parent / "VC" / "INCLUDE" / "VECTOR"))
    instruction = Instruction(text=f"call {name}", raw_text=f"call {name}",
        bytes=("e8", "00", "00", "00", "00"),
        source_line=f"00000: e8 00 00 00 00 call {name}")
    candidate = cc.CandidateAssembly(instructions=(instruction,),
        local_control_flow_indices=frozenset(), caller_definition=caller,
        tu_local_function_definitions={name: helper})
    supplier = Row(address="0x1000", identity="provider:unit.destroy",
        stack_cleanup_bytes=8, catalog_symbol="?Destroy@Vector@@IAEXPAPAX0@Z")
    indexes = cc.IdentityIndexes(by_address={supplier.address: supplier.identity},
        by_candidate_name={}, provider_ids=frozenset({supplier.identity}),
        storage_by_address={}, storage_by_name={},
        pointer_vector_destroy_providers=(supplier,))
    def prove(value=candidate, identities=indexes):
        return cc._candidate_local_coff_callable_bridges(value, indexes=identities,
            bridge_names={}, compiler_generated_bridges={})
    assert cc._pointer_vector_destroy_provider_identity(name, indexes=indexes,
        bridge_names={}, compiler_generated_bridges={}) == ""
    assert prove() == {name: supplier.identity}
    for changes in (
        {"data": b"\xc2\x04\0" + b"\x90" * 13},
        {"data": helper.data[:-1] + b"\xcc"},
        {"relocations": (relocation,)}, {"relocation_mask": (True,) * 16},
        {"comdat_selection": 1}, {"section_external_functions": (name, "alias")},
        {"source_provenance": str(tmp_path / "VECTOR")},
    ):
        with pytest.raises(ValueError):
            prove(replace(candidate,
                tu_local_function_definitions={name: replace(helper, **changes)}))
    for field, value in (("type", 6), ("symbol_index", 4), ("offset", 2)):
        bad = deepcopy(relocation)
        setattr(bad, field, value)
        with pytest.raises(ValueError):
            prove(replace(candidate, caller_definition=replace(caller, relocations=(bad,))))
    with pytest.raises(ValueError):
        prove(identities=replace(indexes, pointer_vector_destroy_providers=()))
    with pytest.raises(ValueError):
        prove(identities=replace(indexes, pointer_vector_destroy_providers=(supplier, supplier)))
    with pytest.raises(ValueError):
        prove(identities=replace(indexes, by_candidate_name={name: "provider:conflict"}))


def test_classifier_switch_and_global_callback_require_complete_live_coff_cfg():
    from copy import deepcopy
    from dataclasses import replace
    from types import SimpleNamespace as Row
    from _recoil.commands.asm_verify import Instruction
    from _recoil.commands.call_contract_verify import (
        CandidateAssembly, _candidate_exact_coff_switch_targets,
        _candidate_exact_static_callback_register,
        _compose_candidate_coff_switch_maps,
    )
    specs = [
        (0, "jmp SHORT $Lguard", "eb 01"),
        (2, "ret", "c3"),
        (3, "cmp eax, 2", "83 f8 02"),
        (6, "ja SHORT $Ldefault", "77 fa"),
        (8, "xor ecx, ecx", "33 c9"),
        (10, "mov cl, byte $Lmap[eax]", "8a 88 00 00 00 00"),
        (16, "jmp dword $Ltable[ecx*4]", "ff 24 8d 00 00 00 00"),
        (23, "mov eax, dword _callback", "a1 00 00 00 00"),
        (28, "call eax", "ff d0"),
        (30, "ret", "c3"),
        (31, "ret", "c3"),
    ]
    instructions = tuple(Instruction(text=text, raw_text=text, bytes=tuple(body.split()),
        source_line=f"{offset:05x}: {body} {text}") for offset, text, body in specs)
    data = b"".join(bytes.fromhex(body) for _, _, body in specs) + b"\0" * 8 + b"\0\1\0"
    symbols = [Row(index=index, name=name, value=value, section_number=section)
        for index, name, value, section in (
            (1, "$Lmap", 40, 1), (2, "$Ltable", 32, 1),
            (3, "$Lcase", 23, 1), (4, "$Ldefault", 2, 1),
            (5, "_callback", 0, 0),
        )]
    relocations = [Row(offset=at, symbol_index=index, symbol_name=symbols[index - 1].name, type=6)
        for at, index in ((12, 1), (19, 2), (32, 3), (36, 4), (24, 5))]
    mask = tuple(any(row.offset <= byte < row.offset + 4 for row in relocations)
                 for byte in range(len(data)))
    definition = Row(data=data, relocations=relocations, relocation_mask=mask,
        section_index=1, start=0, coff_symbols=symbols)
    parsed = CandidateAssembly(instructions=instructions, local_control_flow_indices=frozenset())
    coff = Row(symbols=symbols)
    expected = {6: (7, 1)}
    assert _candidate_exact_coff_switch_targets(parsed, definition, coff) == expected
    composed = _compose_candidate_coff_switch_maps(parsed, expected)
    assert composed.local_control_flow_indices == frozenset(expected)
    assert composed.local_control_flow_targets == {}  # not merely the forward case
    assert composed.classification_only_local_control_flow_targets == expected
    bad = deepcopy(definition)
    bad.relocation_mask = ()
    assert _candidate_exact_coff_switch_targets(parsed, bad, coff) == {}
    for field, value in (("symbol_index", 4), ("type", 0x14), ("symbol_name", "$Lwrong")):
        bad = deepcopy(definition)
        setattr(bad.relocations[0], field, value)
        assert _candidate_exact_coff_switch_targets(parsed, bad, coff) == {}
    bad = deepcopy(definition)
    bad.data = data[:-1] + b"\2"
    assert _candidate_exact_coff_switch_targets(parsed, bad, coff) == {}
    # A coherent branch into XOR bypasses the range check even though every
    # table byte and relocation is otherwise correct.
    bad = deepcopy(definition)
    bad.data = b"\xeb\6" + data[2:]
    bypass = replace(parsed, instructions=(replace(instructions[0], bytes=("eb", "06")), *instructions[1:]))
    assert _candidate_exact_coff_switch_targets(bypass, bad, coff) == {}
    indexes = Row(storage_by_name={"callback": "storage:unit.callback"})
    kwargs = dict(transfer_index=8, register="eax", definition=definition, indexes=indexes,
        addresses=tuple(offset for offset, _, _ in specs), caller_start=0, caller_end=len(data),
        local_control_flow_indices=composed.local_control_flow_indices,
        local_control_flow_targets={**composed.local_control_flow_targets,
                                   **composed.classification_only_local_control_flow_targets})
    assert _candidate_exact_static_callback_register(instructions, **kwargs) == "storage:unit.callback"
    bad = deepcopy(definition)
    bad.relocation_mask = ()
    assert _candidate_exact_static_callback_register(instructions, **{
        **kwargs, "definition": bad}) == ""
    assert _candidate_exact_static_callback_register(instructions, **{
        **kwargs, "local_control_flow_targets": {}}) == ""
    # A second route straight into the call must not inherit the load on the
    # first route, and a conflicting symbol index cannot acquire its identity.
    assert _candidate_exact_static_callback_register(instructions, **{
        **kwargs, "local_control_flow_targets": {6: (7, 8, 1)}}) == ""
    bad = deepcopy(definition)
    bad.relocations[-1].symbol_index = 3
    assert _candidate_exact_static_callback_register(instructions, **{
        **kwargs, "definition": bad}) == ""


@pytest.mark.parametrize("classifier", [False, True])
@pytest.mark.parametrize("scheduled_stores", [False, True])
@pytest.mark.parametrize("repeated_cases", [False, True])
def test_coff_switch_edges_feed_iat_lineage_only_after_complete_guard_proof(
    classifier, scheduled_stores, repeated_cases,
):
    from copy import deepcopy
    from dataclasses import replace
    from types import SimpleNamespace as Row
    from _recoil.commands.asm_verify import Instruction
    from _recoil.commands.call_contract_verify import (
        CandidateAssembly, _candidate_exact_coff_switch_targets,
        _candidate_iat_load_reached_transfer_offsets,
        _compose_candidate_coff_switch_maps,
    )

    specs, offset = [], 0

    def emit(text, body):
        nonlocal offset
        specs.append((offset, text, body))
        offset += len(bytes.fromhex(body))

    emit("mov ebx, dword __imp__task", "8b 1d 00 00 00 00")
    emit("cmp eax, 1", "83 f8 01")
    if scheduled_stores:
        emit("mov dword [esp+4], edx", "89 54 24 04")
        emit("mov dword [esp+8], 1", "c7 44 24 08 01 00 00 00")
    branch_index = len(specs)
    emit("ja SHORT $Ldefault", "77 00")
    if classifier:
        emit("xor ecx, ecx", "33 c9")
        load_index = len(specs)
        emit("mov cl, byte $Lmap[eax]", "8a 88 00 00 00 00")
    jump_index = len(specs)
    emit("jmp dword $Ltable[ecx*4]" if classifier else "jmp dword $Ltable[eax*4]",
         "ff 24 8d 00 00 00 00" if classifier else "ff 24 85 00 00 00 00")
    call_index = len(specs)
    emit("call ebx", "ff d3")
    emit("ret", "c3")
    default_index = len(specs)
    default_offset = offset
    emit("ret", "c3")
    at, text, _ = specs[branch_index]
    specs[branch_index] = (at, text, f"77 {default_offset - at - 2:02x}")
    instructions = tuple(Instruction(text=text, raw_text=text, bytes=tuple(raw.split()),
        source_line=f"{at:05x}: {raw} {text}") for at, text, raw in specs)
    table_offset = offset
    data = b"".join(bytes.fromhex(raw) for _, _, raw in specs) + b"\0" * 8
    symbols = [Row(index=i, name=name, value=value, section_number=section)
        for i, name, value, section in (
            (1, "__imp__task", 0, 0), (2, "$Ltable", table_offset, 1),
            (3, "$Lcase", specs[call_index][0], 1),
            (4, "$Ldefault", default_offset, 1),
            (5, "$Lmap", table_offset + 8, 1),
        )]
    relocations = [Row(offset=at, symbol_index=i, symbol_name=symbols[i - 1].name, type=6)
        for at, i in ((2, 1), (specs[jump_index][0] + 3, 2),
                      (table_offset, 3), (table_offset + 4, 3 if repeated_cases else 4))]
    if classifier:
        data += b"\0\1"
        relocations.append(Row(offset=specs[load_index][0] + 2,
                               symbol_index=5, symbol_name="$Lmap", type=6))
    definition = Row(data=data, section_index=1, start=0, coff_symbols=symbols,
        relocations=relocations, relocation_mask=tuple(
            any(row.offset <= at < row.offset + 4 for row in relocations)
            for at in range(len(data))))
    parsed = CandidateAssembly(instructions=instructions, local_control_flow_indices=frozenset())
    coff = Row(symbols=symbols)
    # A case map retains table order and multiplicity; CFG consumers may
    # deduplicate successors only after deriving their own edge view.
    expected = {jump_index: (call_index, call_index if repeated_cases else default_index)}
    assert _candidate_exact_coff_switch_targets(parsed, definition, coff) == expected
    prove = dict(load_index=0, destination="ebx", offsets=tuple(at for at, _, _ in specs))
    with pytest.raises(ValueError, match="unresolved downstream"):
        _candidate_iat_load_reached_transfer_offsets(parsed, **prove)
    complete = _compose_candidate_coff_switch_maps(parsed, expected)
    assert complete.local_control_flow_targets == expected
    assert complete.classification_only_local_control_flow_targets == {}
    for inconsistent in ((call_index,), tuple(reversed(expected[jump_index]))):
        if inconsistent == expected[jump_index]:
            continue
        with pytest.raises(ValueError, match="conflict"):
            _compose_candidate_coff_switch_maps(replace(parsed,
                local_control_flow_targets={jump_index: inconsistent}), expected)
    for invalid in ((), (-1,), (len(instructions),)):
        with pytest.raises(ValueError, match="invalid local targets"):
            _compose_candidate_coff_switch_maps(parsed, {jump_index: invalid})
    assert _candidate_iat_load_reached_transfer_offsets(complete, **prove) == (
        hex(specs[call_index][0]),)

    # A valid local table target must still not enter after the bound check.
    bad = deepcopy(definition)
    bad.coff_symbols[2].value = specs[jump_index][0]
    assert _candidate_exact_coff_switch_targets(parsed, bad, Row(symbols=bad.coff_symbols)) == {}
    bad = deepcopy(definition)
    bad.data = data[:table_offset] + b"\1" + data[table_offset + 1:]
    assert _candidate_exact_coff_switch_targets(parsed, bad, coff) == {}
    if scheduled_stores:
        # ADD looks like the allowed store except that it destroys CMP flags.
        changed = replace(instructions[2], text="add dword [esp+4], edx",
                          raw_text="add dword [esp+4], edx", bytes=("01", "54", "24", "04"))
        bad = deepcopy(definition)
        bad.data = data[:9] + b"\x01" + data[10:]
        variant = replace(parsed, instructions=(*instructions[:2], changed, *instructions[3:]))
        assert _candidate_exact_coff_switch_targets(variant, bad, coff) == {}


def test_provider_collision_names_require_literal_non_authored_registration():
    from types import SimpleNamespace
    from _recoil.commands.call_contract_verify import _registered_non_authored_provider_names
    row = {"address": "0x1000", "symbol": "_registered",
        "pipeline_class": "non-authored", "authored_order_role": "non-authored"}
    document = SimpleNamespace(collection=lambda _: {"target": {"registration": {
        "translation_unit_function_order": [{"functions": [row,
            {**row, "symbol": "_wrong", "address": "0x2000"},
            {**row, "symbol": "_authored", "pipeline_class": "authored"},
            {**row, "symbol": "", "symbol_regex": "_inferred"},
        ]}]}}})
    assert _registered_non_authored_provider_names(document, "0x1000") == {"_registered"}


def test_provider_collision_names_require_explicit_canonical_catalog():
    from copy import deepcopy
    from _recoil.commands.call_contract_verify import _canonical_header_provider_catalog_names
    record = {"object_symbol": "_catalog", "provider_object_identity": {
        "schema": "recoil-provider-function-object-v2",
        "proof_mode": "canonical-header-comdat", "object_symbol": "_catalog",
        "retail_icf": {"logical_symbols": ["_catalog"]}}}
    assert _canonical_header_provider_catalog_names(record) == ("_catalog",)
    assert _canonical_header_provider_catalog_names({}) == ()
    for names in (None, [], ["_catalog", "_catalog"], ["_inferred"], [None]):
        bad = deepcopy(record)
        bad["provider_object_identity"]["retail_icf"]["logical_symbols"] = names
        with pytest.raises(ValueError, match="malformed logical names"):
            _canonical_header_provider_catalog_names(bad)
    bad = deepcopy(record)
    bad["object_symbol"] = "_other"
    with pytest.raises(ValueError, match="malformed logical names"):
        _canonical_header_provider_catalog_names(bad)


def test_provider_collision_subset_does_not_require_unused_catalog_aliases():
    from _recoil.commands.call_contract_verify import _require_catalogued_provider_collisions

    catalog = {"_one": "provider:unit", "_two": "provider:unit"}
    for actual in ({}, {"_one": "provider:unit"}, catalog):
        _require_catalogued_provider_collisions(actual, catalog, helper_name="_current")
    for actual in ({"_unknown": "provider:unit"}, {"_one": "provider:foreign"}):
        with pytest.raises(ValueError, match="identity collisions"):
            _require_catalogued_provider_collisions(actual, catalog, helper_name="_current")


def test_candidate_provider_catalog_requires_live_typed_header_proof(monkeypatch, tmp_path):
    from copy import deepcopy
    from dataclasses import replace
    from types import SimpleNamespace as Row
    from _recoil.commands import call_contract_verify as cc
    from _recoil.commands import provider_function_mutation as providers

    name = "?Copy@PointerVector@@IAEXPAPAX0@Z"
    primary = "?Copy@IntegerVector@@IAEXPAH0@Z"
    env = tmp_path / "compiler" / "env.cmd"
    monkeypatch.setattr(cc, "compiler_env_path", lambda *_: env)
    proof_calls = []
    def live_proof(**kwargs):
        proof_calls.append(kwargs)
        return Row(relocations=(), masked_byte_count=0, comdat_selection=2, body_size=4)
    monkeypatch.setattr(providers, "_provider_header_comdat_proof", live_proof)
    helper = cc.CandidateTuLocalFunctionDefinition(symbol=name,
        data=bytes.fromhex("c2 08 00 90"), relocations=(), relocation_mask=(False,) * 4,
        section_size=4, section_external_functions=(name,), section_is_comdat=True,
        comdat_selection=2, source_provenance=str(env.parent / "VC/INCLUDE/vector"))
    symbol = Row(index=3, name=name, section_number=2, storage_class=2,
        value=0, weak_external_tag_index=None)
    caller = cc.CandidateCallerDefinition(symbol="_caller", data=b"\xc3",
        relocations=(), relocation_mask=(False,), undefined_external_functions=(),
        defined_external_functions=(name,), coff_symbols=(symbol,),
        section_index=1, section_start=0, section_end=1)
    candidate = cc.CandidateAssembly(instructions=(), local_control_flow_indices=frozenset(),
        caller_definition=caller, tu_local_function_definitions={name: helper})
    record = {"object_symbol": primary, "address": "0x1000", "size": 4,
        "kind": "provider-function", "pipeline_class": "non-authored",
        "authored_order_role": "non-authored", "extent_state": "known",
        "provider_object_identity": {
            "schema": "recoil-provider-function-object-v2",
            "proof_mode": "canonical-header-comdat", "object_symbol": primary,
            "canonical_header": "VC/INCLUDE/vector", "semantic_provider": "test",
            "probe_recipe": "test-primary", "body_size": 4, "comdat_selection": 2,
            "retail_icf": {"logical_symbols": [primary, name]}}}
    records = {"unit": record}
    identities = cc.IdentityIndexes(by_address={"0x1000": "provider:unit"},
        by_candidate_name={}, provider_ids=frozenset({"provider:unit"}),
        storage_by_address={}, storage_by_name={})
    bridge = Row(hexdump=lambda *_: "00001000  c2 08 00 90")
    def match(value=candidate, rows=records, indexes=identities):
        return cc._candidate_only_provider_comdat_catalog_matches(name, value,
            document=Row(collection=lambda _: rows), indexes=indexes, bridge=bridge)
    assert match() == ("provider:unit",)
    assert proof_calls[-1]["request"]["retail_icf_logical_symbols"] == [primary, name]
    for changes in (
        {"data": bytes.fromhex("c2 04 00 90")}, {"relocations": (Row(),)},
        {"relocation_mask": (True,) * 4}, {"comdat_selection": 1},
        {"source_provenance": str(tmp_path / "vector")},
        {"section_external_functions": (name, "alias")}, {"section_is_comdat": False},
    ):
        assert match(replace(candidate,
            tu_local_function_definitions={name: replace(helper, **changes)})) == ()
    bad = deepcopy(record)
    bad["provider_object_identity"]["retail_icf"]["logical_symbols"] = [primary]
    assert match(rows={"unit": bad}) == ()  # Identical bytes do not invent a name.
    alias = deepcopy(symbol)
    alias.index, alias.name = 4, "_alias"
    assert match(replace(candidate,
        caller_definition=replace(caller, coff_symbols=(symbol, alias)))) == ()
    second = {**record, "address": "0x2000"}
    assert match(rows={**records, "second": second}, indexes=replace(identities,
        by_address={"0x1000": "provider:unit", "0x2000": "provider:second"},
        provider_ids=frozenset({"provider:unit", "provider:second"}))) == (
            "provider:second", "provider:unit")  # Caller must reject non-unique matches.
    def failed_proof(**_):
        raise providers.ProviderFunctionMutationError("unproven alias")
    monkeypatch.setattr(providers, "_provider_header_comdat_proof", failed_proof)
    with pytest.raises(ValueError, match="unproven alias"):
        match()


def test_header_registration_proves_every_logical_specialization(monkeypatch, tmp_path):
    from types import SimpleNamespace as Row
    from _recoil.commands import provider_function_mutation as providers
    recipes = {key: {"object_symbol": name, "canonical_header": "VC/INCLUDE/header",
        "semantic_provider": "test", "comdat_selection": 2, "retail_body_size": 1}
        for key, name in (("primary", "_primary"), ("alias", "_alias"))}
    monkeypatch.setattr(providers, "HEADER_PROBE_RECIPES", recipes)
    header = tmp_path / "header"
    header.write_text("provider", encoding="ascii")
    monkeypatch.setattr(providers, "_resolve_library", lambda *_: header)
    compiled, compared = [], []
    def compile_probe(**kwargs):
        compiled.append(kwargs["recipe_id"])
        return kwargs["recipe_id"].encode(), ()
    def compare(**kwargs):
        compared.append(kwargs["object_symbol"])
        assert kwargs["retail_body"] == b"\xc3"
        return Row(object_symbol=kwargs["object_symbol"], relocations=(), masked_byte_count=0)
    monkeypatch.setattr(providers, "_compile_header_probe_object", compile_probe)
    monkeypatch.setattr(providers, "_coff_provider_object_proof", compare)
    request = {"object_symbol": "_primary", "probe_recipe": "primary",
        "canonical_header": "VC/INCLUDE/header", "semantic_provider": "test",
        "physical_emitter_state": "winner-unknown", "retail_icf_winner_status": "winner-unknown",
        "retail_icf_logical_symbols": ["_primary", "_alias"]}
    def prove(value=request):
        return providers._provider_header_comdat_proof(vc5_root=tmp_path,
            request=value, body_size=1, retail_body=b"\xc3")
    assert prove().object_symbol == "_primary"
    assert compiled == ["primary", "alias"] and compared == ["_primary", "_alias"]
    for names in (["_primary", "_unknown"], ["_primary", "_primary"], ["_alias"]):
        with pytest.raises(providers.ProviderFunctionMutationError):
            prove({**request, "retail_icf_logical_symbols": names})
    recipes["ambiguous"] = dict(recipes["alias"])
    with pytest.raises(providers.ProviderFunctionMutationError, match="independent registered probe"):
        prove()
    del recipes["ambiguous"]
    def failed_alias(**kwargs):
        if kwargs["object_symbol"] == "_alias":
            raise providers.ProviderFunctionMutationError("alias bytes differ")
        return compare(**kwargs)
    monkeypatch.setattr(providers, "_coff_provider_object_proof", failed_alias)
    with pytest.raises(providers.ProviderFunctionMutationError, match="alias bytes differ"):
        prove()
    monkeypatch.setattr(providers, "_coff_provider_object_proof",
        lambda **_: Row(relocations=({"offset": 0},), masked_byte_count=4))
    with pytest.raises(providers.ProviderFunctionMutationError, match="typed relocation-target"):
        prove()


def test_header_catalog_extension_preserves_identity_extent_and_owner():
    from copy import deepcopy
    from types import SimpleNamespace as Row
    from _recoil.commands import provider_function_mutation as providers
    request = {"owner_id": "provider", "owner_name": "Provider", "object_symbol": "_primary",
        "proof_mode": "canonical-header-comdat", "canonical_header": "VC/INCLUDE/header",
        "probe_recipe": "primary", "semantic_provider": "test",
        "retail_icf_logical_symbols": ["_primary", "_alias"]}
    function = {"binary": "recoil", "kind": "provider-function", "address": "0x1000",
        "end_exclusive": "0x1010", "size": 16, "extent_state": "known",
        "pipeline_class": "non-authored", "authored_order_role": "non-authored",
        "disposition": "provider", "ownership_state": "primary-owned",
        "output_section_id": "recoil:section:.text", "object_symbol": "_primary",
        "provider_object_identity": {"schema": "recoil-provider-function-object-v2",
            **{key: request[key] for key in ("proof_mode", "object_symbol", "canonical_header",
                "probe_recipe", "semantic_provider")}, "body_size": 16,
            "retail_icf": {"logical_symbols": ["_primary"]}}}
    owner = {"kind": "provider-boundary", "name": "Provider", "source_paths": [],
        "relationships": [{"kind": "primary-function", "symbol_id": "unit", "address": "0x1000"}],
        "gates": {"byte": "deferred"}, "evidence_ids": ["prior"]}
    def validate(f=function, o=owner, r=request):
        document = Row(collection=lambda key: {"unit": f} if key == "symbols" else {"provider": o})
        return providers._validate_header_provider_extension(document,
            function_id="unit", address="0x1000", request=r)
    row, start, end, preserved = validate()
    assert (start, end) == (0x1000, 0x1010) and row == function and preserved == owner
    assert row is not function and preserved is not owner
    for changes in ({"size": 17}, {"kind": "function"}, {"pipeline_class": "authored"},
                    {"object_symbol": "_other"}, {"end_exclusive": "0x1020"}):
        with pytest.raises(providers.ProviderFunctionMutationError):
            validate(f={**function, **changes})
    for changes in ({"kind": "class"}, {"name": "renamed"}, {"source_paths": ["src/unit.cpp"]},
                    {"relationships": []}):
        with pytest.raises(providers.ProviderFunctionMutationError):
            validate(o={**owner, **changes})
    for changes in ({"probe_recipe": "other"}, {"object_symbol": "_alias"},
                    {"retail_icf_logical_symbols": ["_alias"]}):
        with pytest.raises(providers.ProviderFunctionMutationError):
            validate(r={**request, **changes})
    bad = deepcopy(function)
    bad["provider_object_identity"]["retail_icf"] = None
    with pytest.raises(providers.ProviderFunctionMutationError):
        validate(f=bad)


def test_spilled_loop_index_is_not_an_unknown_stack_receiver():
    from _recoil.commands.call_contract_verify import _is_bounded_stack_vptr
    expression = (
        "load(load(load(affine(load(call-result(symbol:unit)+0x84),"
        "bounded-stack-counter+0x18*4))+0x8))"
    )
    assert _is_bounded_stack_vptr(expression)
    for old, new in (
        ("bounded-stack-counter+0x18", "load(stack+0x18)"),
        ("bounded-stack-counter+0x18", "unknown"),
        ("call-result(symbol:unit)", "stack"),
        ("call-result(symbol:unit)", "entry-register(eax)"),
        ("symbol:unit", "iat:unit"),
        ("*4", "*3"),
        ("+0x8", "+unknown"),
    ):
        assert not _is_bounded_stack_vptr(expression.replace(old, new))


def test_deferred_comdat_requires_unique_exact_observed_header_rows(tmp_path):
    from _recoil.commands.call_contract_verify import _candidate_comdat_source_provenance
    source = tmp_path / "unit.cpp"
    source.write_text("unrelated\n", encoding="utf-8")
    listing = tmp_path / "unit.cod"
    listing.write_text(
        f"; File {source}\n_helper PROC NEAR\n; 2 : return;\n"
        "  00000 c3 ret 0\n_helper ENDP\n", encoding="utf-8",
    )
    header = str(tmp_path / "provider.h")
    assert _candidate_comdat_source_provenance(listing, "_helper") == str(source)
    assert _candidate_comdat_source_provenance(
        listing, "_helper", header_source_files={header: ("declaration", "return;")},
    ) == header
    assert _candidate_comdat_source_provenance(
        listing, "_helper", header_source_files={header: ("declaration", "return 1;")},
    ) == str(source)
    with pytest.raises(ValueError, match="ambiguous exact COD source-row"):
        _candidate_comdat_source_provenance(
            listing, "_helper", header_source_files={
                header: ("declaration", "return;"),
                str(tmp_path / "other.h"): ("different declaration", "return;"),
            },
        )


def test_comdat_header_pool_requires_exact_current_tu_observation(tmp_path):
    from copy import deepcopy
    from _recoil.commands.call_contract_verify import _candidate_unit_header_source_files
    header = tmp_path / "provider.h"
    header.write_text("declaration\nreturn;\n", encoding="utf-8")
    observation = {
        "verification_eligible": True, "source_from": "src/unit.cpp",
        "compiler": {"observed_cwd": str(tmp_path)},
        "toolchain": {"header_inputs": [{"path": str(header),
            "role": "toolchain-header", "physical_identity": {"unit-test": True}}]},
    }
    row = {"target_id": "unit", "source_from": "src/unit.cpp",
        "verifier_receipt": {"verification_eligible": True, "post_observation": observation},
        "parent_receipt": {"verification_eligible": True,
            "pre_observation": observation, "post_observation": observation}}
    kwargs = dict(target_name="unit", source_from="src/unit.cpp", build_dir=tmp_path)
    assert _candidate_unit_header_source_files([row], **kwargs) == {
        str(header): ("declaration", "return;")}
    auxiliary = deepcopy(row)
    auxiliary["parent_receipt"]["post_observation"]["compiler"]["observed_cwd"] = str(tmp_path / "auxiliary")
    auxiliary["parent_receipt"]["post_observation"]["toolchain"]["header_inputs"][0]["path"] = str(tmp_path / "not-opened.h")
    # An independent fresh compilation of the same target/source does not
    # contribute headers to this COD. Its files need not be opened at all.
    assert _candidate_unit_header_source_files([auxiliary, row], **kwargs) == {
        str(header): ("declaration", "return;")}
    assert _candidate_unit_header_source_files([row, auxiliary], **kwargs) == {
        str(header): ("declaration", "return;")}
    for invalid in ([], [row, row]):
        with pytest.raises(ValueError, match="one fresh TU"):
            _candidate_unit_header_source_files(invalid, **kwargs)
    invalid = deepcopy(row)
    invalid["parent_receipt"]["verification_eligible"] = False
    with pytest.raises(ValueError, match="stale or conflicting"):
        _candidate_unit_header_source_files([invalid], **kwargs)
    with pytest.raises(ValueError, match="one fresh TU"):
        _candidate_unit_header_source_files([row], **{**kwargs, "build_dir": tmp_path / "other"})
    for cwd in (None, "", "relative/build"):
        invalid = deepcopy(row)
        invalid["parent_receipt"]["post_observation"]["compiler"]["observed_cwd"] = cwd
        with pytest.raises(ValueError, match="absolute TU compilation context"):
            _candidate_unit_header_source_files([invalid, row], **kwargs)
    invalid = deepcopy(row)
    invalid["parent_receipt"]["pre_observation"] = {"verification_eligible": False}
    with pytest.raises(ValueError, match="stale or conflicting"):
        _candidate_unit_header_source_files([auxiliary, invalid], **kwargs)


def test_member_storage_rendering_preserves_load_depth_roots_offsets_and_call_shape():
    from copy import deepcopy
    from _recoil.commands.call_contract_verify import (
        _canonical_proven_member_storage, compare_call_contracts,
    )
    base = dict(ordinal=0, form="call", dispatch="indirect",
        identity_kind="virtual-slot", target_identity="", slot_displacement=12,
        cleanup_bytes=4)
    pairs = (
        ("load(exact-receiver-field(this,+0x24))", "load(load(this+0x24))"),
        ("load(address(load(this+0x18)+0x40))", "load(load(this+0x18)+0x40)"),
        ("load(exact-receiver-field(load(load(this+0x10+0x8)),+0x4))",
         "load(load(load(load(this+0x18))+0x4))"),
        ("load(load(this+0x10+0x8)+0x4+0x8)",
         "load(load(this+0x18)+0xc)"),
    )
    for left, right in pairs:
        expected, candidate = [{**base, "storage_identity": left}], [{**base, "storage_identity": right}]
        original = deepcopy((expected, candidate))
        assert compare_call_contracts(expected, candidate)["passed"]
        assert compare_call_contracts(candidate, expected)["passed"]
        assert (expected, candidate) == original
        for key, value in (("ordinal", 1), ("form", "tail"), ("dispatch", "direct"),
                           ("slot_displacement", 16), ("cleanup_bytes", 8),
                           ("target_identity", "different")):
            assert not compare_call_contracts(expected, [{**candidate[0], key: value}])["passed"]
        for dispatch_kind in ("iat", "direct"):
            assert not compare_call_contracts(
                [{**expected[0], "identity_kind": dispatch_kind}],
                [{**candidate[0], "identity_kind": dispatch_kind}],
            )["passed"]
    expected = [{**base, "storage_identity": pairs[0][0]}]
    for value in ("load(this+0x24)", "load(load(this+0x28))",
                  "load(load(eax+0x24))", "load(load(load(this+0x24)))",
                  "load(address(this+0x24))"):
        assert not compare_call_contracts(expected, [{**base, "storage_identity": value}])["passed"]
    for value in ("load(exact-receiver-field(eax,+0x24))",
                  "load(exact-receiver-field(this,-0x24))",
                  "load(exact-receiver-field(this,+0x80000000))",
                  "load(exact-receiver-field(this,+0x24))+0x4",
                  "load(exact-receiver-field(load(eax+0x10),+0x4))",
                  "load(load(this+0x7fffffff+0x1))",
                  "load(exact-receiver-field(load(this+0x10),+0x80000000))",
                  "load(load(this+0x10)+0x4",
                  "load(address(load(this+0x18)+0x80000000))",
                  "load(address(load(this+0x18)+dynamic))"):
        assert _canonical_proven_member_storage(value) == value
    nested = [{**base, "storage_identity": pairs[2][0]}]
    for value in ("load(load(load(this+0x18)+0x4))",
                  "load(load(load(load(this+0x1c))))",
                  "load(load(load(load(this+0x18))+0x8))",
                  "load(load(load(load(this+0x1c))+0x4))"):
        assert not compare_call_contracts(nested, [{**base, "storage_identity": value}])["passed"]


def test_scalar_initializer_proves_stores_not_allocator_defaults():
    import pytest
    from _recoil.lib.initializer_contract import constant_member_bytes

    # mov edx,ecx; xor eax,eax; mov [edx+4],eax; mov eax,edx; ret
    body = bytes.fromhex("8b d1 33 c0 89 42 04 8b c2 c3")
    expected = {i: 0 for i in range(4, 8)}
    assert constant_member_bytes(body, object_size=16) == expected
    assert constant_member_bytes(body[:4] + body[7:], object_size=16) != expected
    assert constant_member_bytes(bytes.fromhex("8b d1 b8 01 00 00 00 89 42 04 8b c2 c3"), object_size=16) != expected
    # A subsequent partial overwrite must not be hidden by an earlier zero.
    overwrite = body[:-3] + bytes.fromhex("c6 42 05 01") + body[-3:]
    assert constant_member_bytes(overwrite, object_size=16)[5] == 1
    for invalid in (b"\xeb\x02" + body, body[:-1], body + b"\xc3",
                    bytes.fromhex("33 c0 89 42 04 8b c2 c3"),
                    bytes.fromhex("8b d1 33 c0 89 42 0f 8b c2 c3")):
        with pytest.raises(ValueError):
            constant_member_bytes(invalid, object_size=16)


def test_scalar_initializer_equates_byte_stores_and_bounded_rep():
    from _recoil.lib.initializer_contract import constant_member_bytes
    direct = bytes.fromhex("8b d1 33 c0 89 42 04 8b c2 c3")
    bytewise = bytes.fromhex("8b d1 33 c0 88 42 04 88 42 05 88 42 06 88 42 07 8b c2 c3")
    repeat = bytes.fromhex("57 8b d1 8d 7a 04 33 c0 b9 01 00 00 00 f3 ab 8b c2 5f c3")
    assert constant_member_bytes(direct, object_size=16) == constant_member_bytes(bytewise, object_size=16)
    assert constant_member_bytes(direct, object_size=16) == constant_member_bytes(repeat, object_size=16)


def test_reviewed_target_authority_is_not_duplicated_by_diagnostics():
    import pytest
    from _recoil.lib.authored_icf import (
        reviewed_authority_targets, exact_selected_target_membership,
        exact_required_target_membership,
    )
    contract = "existing-winner-unknown-physical-group-refresh-v1"
    explicit = {"evidence_contract": contract, "governed_target_id": "reviewed"}
    assert reviewed_authority_targets([explicit], "older") == {"reviewed"}
    assert reviewed_authority_targets([{}], "accepted") == {"accepted"}
    assert exact_selected_target_membership(["reviewed", "diagnostic"], "reviewed")
    assert not exact_selected_target_membership(["diagnostic"], "reviewed")
    assert not exact_selected_target_membership(["reviewed", "reviewed"], "reviewed")
    assert exact_required_target_membership(["diagnostic", "second", "reviewed"], ["reviewed", "second"])
    for registered, required in (
        (["reviewed", "diagnostic"], ["reviewed", "second"]),
        (["reviewed", "diagnostic", "diagnostic"], ["reviewed"]),
        (["reviewed"], ["reviewed", "reviewed"]), (["reviewed"], []),
        (None, ["reviewed"]), ([{}], ["reviewed"]), (["reviewed"], [None]),
    ):
        assert not exact_required_target_membership(registered, required)
    for rows in ([explicit, {**explicit, "governed_target_id": "other"}],
                 [{"evidence_contract": contract}]):
        with pytest.raises(ValueError):
            reviewed_authority_targets(rows, "accepted")


def test_regex_authority_uses_unique_same_address_accepted_order_target():
    from copy import deepcopy
    import pytest
    from _recoil.commands.call_contract_verify import _select_registered_symbol_regex_authority

    address = "0x401000"
    governing = {
        (target, address, "row"): (target, {}, {}, address)
        for target in ("authored", "linked-diagnostic")
    }
    symbols = {"body": {"kind": "function", "address": address, "physical_block_id": "block"}}
    facts = dict(phase="authored-function-order", validation_mode="live",
                 target_id="authored", matched_identities=["body"])
    blocks = {"block": {"accepted_order_facts": facts}}

    def select(matches=governing, physical=symbols, owners=blocks):
        return _select_registered_symbol_regex_authority(
            matches, symbols=physical, blocks=owners, candidate_name="?helper@@YAXXZ",
        )

    assert select()[0] == "authored"
    assert select(dict(reversed(list(governing.items()))))[0] == "authored"
    for field, value in (("phase", "full-function-order"), ("validation_mode", "historical"),
                         ("target_id", "absent"), ("matched_identities", []),
                         ("matched_identities", ["body", "body"])):
        bad = deepcopy(blocks)
        bad["block"]["accepted_order_facts"][field] = value
        with pytest.raises(ValueError, match="ambiguous"):
            select(owners=bad)
    for physical in ({}, {**symbols, "alias": symbols["body"]}):
        with pytest.raises(ValueError, match="ambiguous"):
            select(physical=physical)
    for extra in (
        {("authored", address, "other-row"): ("authored", {}, {"name": "other"}, address)},
        {("other", "0x402000", "row"): ("other", {}, {}, "0x402000")},
    ):
        with pytest.raises(ValueError, match="ambiguous"):
            select({**governing, **extra})


def test_known_authored_targets_cannot_be_substituted_by_legacy_projection():
    from dataclasses import replace
    from types import SimpleNamespace as Row
    from _recoil.commands.call_contract_verify import (
        IdentityIndexes, _known_authored_direct_target_divergence,
    )
    from _recoil.commands.asm_verify import Instruction

    name = "?Build@Different@@QAEXXZ"
    actual, required = "symbol:different", "symbol:required"
    indexes = IdentityIndexes(
        by_address={"0x2000": actual, "0x3000": required},
        by_candidate_name={name: actual}, provider_ids=frozenset(),
        storage_by_address={}, storage_by_name={},
    )
    def instruction(offset, text, encoded):
        return Instruction(text=text, raw_text=text, bytes=tuple(encoded.split()),
                           source_line=f"{offset:05x}: {encoded} {text}")
    definition = Row(data=bytes.fromhex("e8 00 00 00 00 c3"),
                     relocations=[Row(offset=1, type=0x14, symbol_name=name)])
    candidate = Row(
        caller_definition=definition,
        instructions=(instruction(0, "call " + name, "e8 00 00 00 00"),
                      instruction(5, "ret", "c3")),
        local_control_flow_indices=frozenset(), local_control_flow_targets={},
    )
    expected = [{"form": "call", "dispatch": "direct", "identity_kind": "direct",
                 "target_identity": required}]
    def check(authority=indexes, rows=expected):
        return _known_authored_direct_target_divergence(rows, candidate, indexes=authority)
    mismatch = check()
    assert mismatch["kind"] == "verifier-blocked"
    assert mismatch["ordinal"] == 0
    assert mismatch["candidate"]["target_identity"] == actual
    assert mismatch["expected"]["target_identity"] == required
    assert check(rows=[{**expected[0], "target_identity": actual}]) is None
    assert check(replace(indexes, reviewed_icf_group_by_logical_identity={
        actual: "icf:group", required: "icf:group",
    })) is None
    assert check(replace(indexes, reviewed_authored_icf_physical_by_logical_identity={
        actual: required,
    })) is None
    # No rejection-only result may qualify unknown/malformed evidence as PASS.
    assert check(replace(indexes, by_candidate_name={})) is None
    assert check(replace(indexes, by_candidate_name={name: ""},
                         by_address={"0x2000": "", "0x3000": required})) is None
    assert check(replace(indexes, provider_ids=frozenset({actual}))) is None
    assert check(rows=[]) is None
    definition.relocations[0].symbol_name = "?Other@@YAXXZ"
    assert check() is None
    definition.relocations[0].symbol_name = name
    definition.data = bytes.fromhex("e8 01 00 00 00 c3")
    assert check() is None


def test_compiler_provider_identity_uses_live_sites_and_exact_retail_ordinals():
    from copy import deepcopy
    from types import SimpleNamespace as Row
    import pytest
    from _recoil.commands.asm_verify import Instruction
    from _recoil.commands.call_contract_verify import (
        _prove_same_ordinal_compiler_provider_calls,
    )

    name, identity = "__runtime_helper", "provider:runtime-helper"
    expected = [dict(ordinal=i, form="call", dispatch="direct",
                     identity_kind="provider", target_identity=identity,
                     storage_identity="", slot_displacement=None, cleanup_bytes=None)
                for i in range(2)]

    def fixture(second_offset):
        data = bytearray(b"\x90" * (second_offset + 6))
        mask = bytearray(len(data))
        instructions = []
        for offset in (0, second_offset):
            data[offset:offset + 5] = bytes.fromhex("e8 00 00 00 00")
            mask[offset + 1:offset + 5] = b"\x01" * 4
            text = "call " + name
            instructions.append(Instruction(
                text=text, raw_text=text, bytes=("e8", "00", "00", "00", "00"),
                source_line=f"{offset:05x}: e8 00 00 00 00 {text}",
            ))
        data[-1] = 0xc3
        definition = Row(
            data=bytes(data), relocation_mask=bytes(mask),
            coff_symbols=[Row(name=name, index=7, section_number=0, symbol_type=0x20,
                              storage_class=2, aux_count=0)],
            undefined_external_functions=(name,), defined_external_functions=(),
            undefined_external_data=(), defined_external_data=(),
            relocations=[Row(offset=offset + 1, type=0x14, symbol_name=name, symbol_index=7)
                         for offset in (0, second_offset)],
        )
        return Row(caller_definition=definition, instructions=tuple(instructions),
                   local_control_flow_indices=frozenset(), local_control_flow_targets={})

    def prove(candidate, rows=expected):
        return _prove_same_ordinal_compiler_provider_calls(
            rows, candidate, name=name, provider_identity=identity,
        )

    # Instruction movement is immaterial to provider identity; each invocation
    # must still match its independent retail ordinal and live COFF reference.
    for offset in (5, 16, 39):
        assert prove(fixture(offset)) == {name: identity}
    for field, value in (("target_identity", "provider:different"),
                         ("identity_kind", "direct"), ("form", "tail"),
                         ("dispatch", "indirect"), ("ordinal", 9)):
        rows = deepcopy(expected)
        rows[1][field] = value
        with pytest.raises(ValueError):
            prove(fixture(16), rows)
    with pytest.raises(ValueError):
        prove(fixture(16), expected[:1])
    mutations = (
        lambda d: setattr(d.relocations[1], "type", 6),
        lambda d: setattr(d.relocations[1], "offset", 18),
        lambda d: setattr(d.relocations[1], "symbol_index", 8),
        lambda d: setattr(d.relocations[1], "symbol_name", "__different"),
        lambda d: d.relocations.append(deepcopy(d.relocations[1])),
        lambda d: setattr(d.coff_symbols[0], "section_number", 1),
        lambda d: setattr(d, "defined_external_functions", (name,)),
        lambda d: setattr(d, "undefined_external_data", (name,)),
        lambda d: setattr(d, "data", d.data[:17] + b"\x01" + d.data[18:]),
        lambda d: setattr(d, "relocation_mask", b"\x00" * len(d.data)),
    )
    for mutate in mutations:
        candidate = fixture(16)
        mutate(candidate.caller_definition)
        with pytest.raises(ValueError):
            prove(candidate)


def test_candidate_cfg_receiver_join_uses_coff_identity_not_zero_operand_address():
    from dataclasses import replace
    from types import SimpleNamespace as Row
    from _recoil.commands.asm_verify import Instruction
    from _recoil.commands.call_contract_verify import (
        IdentityIndexes, _candidate_coff_direct_call_identities,
        _exact_retail_cfg_register_provenance,
    )

    name, identity = "?Acquire@@YAPAXXZ", "symbol:factory"
    encoded = [
        ("8b f1", "mov esi, ecx"), ("e8 00 00 00 00", "call " + name),
        ("8b f8", "mov edi, eax"), ("85 ff", "test edi, edi"),
        ("75 03", "jne SHORT $Ljoin"), ("8d 7e 44", "lea edi, [esi+68]"),
        ("8b 47 08", "mov eax, [edi+8]"), ("8b 08", "mov ecx, [eax]"),
        ("50", "push eax"), ("ff 51 24", "call [ecx+36]"), ("c3", "ret"),
    ]
    instructions, addresses, offset = [], [], 0
    for raw, text in encoded:
        instructions.append(Instruction(text=text, raw_text=text, bytes=tuple(raw.split()),
                                        source_line=f"{offset:05x}: {raw} {text}"))
        addresses.append(0x1000 + offset)
        offset += len(raw.split())
    definition = Row(
        data=bytes.fromhex(" ".join(raw for raw, _ in encoded)),
        relocation_mask=bytes([0] * 3 + [1] * 4 + [0] * (offset - 7)),
        relocations=[Row(offset=3, type=0x14, symbol_name=name, symbol_index=1)],
        coff_symbols=[Row(index=1, name=name)],
    )
    indexes = IdentityIndexes(
        by_address={"0x1007": "symbol:not-the-factory"},
        by_candidate_name={name: identity}, provider_ids=frozenset(),
        storage_by_address={}, storage_by_name={},
    )

    def prove(authority=indexes):
        identities = _candidate_coff_direct_call_identities(
            instructions, addresses=addresses, caller_start=0x1000,
            definition=definition, indexes=authority,
        )
        return _exact_retail_cfg_register_provenance(
            instructions, before_index=9, register="ecx", addresses=addresses,
            indexes=authority, caller_start=0x1000, caller_end=0x1000 + offset,
            local_control_flow_indices=frozenset(), local_control_flow_targets={},
            allow_exact_affine_receiver_roots=True, source="cod",
            direct_call_identities=identities,
        )

    expected = "load(load(runtime-object-join(call-result(symbol:factory),this+0x44)+0x8))"
    assert prove() == expected
    assert prove(replace(indexes, by_candidate_name={})) == ""
    for field, value in (("type", 6), ("symbol_index", 2), ("symbol_name", "?Wrong@@YAXXZ")):
        prior = getattr(definition.relocations[0], field)
        setattr(definition.relocations[0], field, value)
        assert prove() == ""
        setattr(definition.relocations[0], field, prior)
    prior = instructions[7]
    instructions[7] = replace(prior, text="mov ecx, [edx]", raw_text="mov ecx, [edx]",
                              bytes=("8b", "0a"))
    assert prove() == ""
    instructions[7] = prior
    definition.data = definition.data[:3] + b"\x01" + definition.data[4:]
    assert prove() == ""


@pytest.mark.parametrize("clobber", [False, True])
def test_cfg_receiver_join_converges_through_a_loop_without_losing_paths(clobber):
    from _recoil.commands.asm_verify import Instruction
    from _recoil.commands.call_contract_verify import (
        IdentityIndexes, _exact_retail_cfg_register_provenance,
    )

    encoded = [
        ("8b f1", "mov esi, ecx"), ("e8 00 00 00 00", "call factory"),
        ("8b f8", "mov edi, eax"), ("85 ff", "test edi, edi"),
        ("75 03", "jne SHORT $Ljoin"), ("8d 7e 44", "lea edi, [esi+68]"),
        ("8b 5f 08", "mov ebx, [edi+8]"),
        ("8b da" if clobber else "8b c2", "mov ebx, edx" if clobber else "mov eax, edx"),
        ("49", "dec ecx"), ("75 fb", "jne SHORT $Lloop"),
        ("8b 03", "mov eax, [ebx]"), ("ff 50 30", "call [eax+48]"),
        ("c3", "ret"),
    ]
    instructions, addresses, offset = [], [], 0
    for raw, text in encoded:
        instructions.append(Instruction(text=text, raw_text=text, bytes=tuple(raw.split()),
                                        source_line=f"{offset:05x}: {raw} {text}"))
        addresses.append(0x1000 + offset)
        offset += len(raw.split())
    indexes = IdentityIndexes(by_address={}, by_candidate_name={},
                              provider_ids=frozenset(), storage_by_address={}, storage_by_name={})
    actual = _exact_retail_cfg_register_provenance(
        instructions, before_index=11, register="eax", addresses=addresses,
        indexes=indexes, caller_start=0x1000, caller_end=0x1000 + offset,
        local_control_flow_indices=frozenset(), local_control_flow_targets={},
        allow_exact_affine_receiver_roots=True, source="cod",
        direct_call_identities={1: "symbol:factory"},
    )
    assert actual == ("" if clobber else
        "load(load(runtime-object-join(call-result(symbol:factory),this+0x44)+0x8))")


def test_candidate_call_result_vptr_requires_current_operand_and_receiver_proofs():
    from types import SimpleNamespace as Row
    from _recoil.commands.call_contract_verify import _exact_candidate_cfg_vptr_proofs

    name, identity = "?Acquire@@YAPAXXZ", "symbol:factory"
    specs = [("e8 00 00 00 00", "call " + name), ("8b f8", "mov edi, eax"),
             ("8b 17", "mov edx, [edi]"), ("8b cf", "mov ecx, edi"),
             ("ff 52 20", "call [edx+0x20]"), ("c3", "ret")]
    instructions, addresses, cursor = [], [], 0x1000
    for raw, text in specs:
        instructions.append(Instruction(text=text, raw_text=text, bytes=tuple(raw.split()),
            source_line=f"{cursor-0x1000:05x}:"))
        addresses.append(cursor)
        cursor += len(raw.split())
    data = bytes.fromhex(" ".join(raw for raw, _ in specs))
    definition = Row(data=data + b"\0" * 32,
        relocation_mask=bytes([0] + [1]*4 + [0]*(len(data)+32-5)),
        relocations=[Row(offset=1, type=0x14, symbol_name=name, symbol_index=1)],
        coff_symbols=[Row(index=1, name=name)])
    indexes = IdentityIndexes(by_address={}, by_candidate_name={name: identity},
        provider_ids=frozenset(), storage_by_address={}, storage_by_name={})
    def prove():
        return _exact_candidate_cfg_vptr_proofs(instructions, addresses=addresses,
            caller_start=0x1000, indexes=indexes, definition=definition,
            local_control_flow_indices=frozenset(), local_control_flow_targets={})
    assert prove() == {4: "load(call-result(symbol:factory))"}
    instructions.append(Instruction(text="$Ltable", raw_text="$Ltable",
        bytes=("00", "00", "00", "00", "dd"),
        source_line=f"{len(data):05x} 00 00 00 00 DD $Ltable"))
    addresses.append(0x1000 + len(data))
    assert prove() == {4: "load(call-result(symbol:factory))"}
    definition.relocations[0].type = 6
    assert prove() == {}
    definition.relocations[0].type = 0x14
    definition.data = b"\x90" + definition.data[1:]
    assert prove() == {}
    definition.data = data + b"\0" * 32
    original = instructions[3]
    instructions[3] = Instruction(text="mov ecx, esi", raw_text="mov ecx, esi",
        bytes=("8b", "ce"), source_line=original.source_line)
    definition.data = definition.data[:9] + b"\x8b\xce" + definition.data[11:]
    assert prove() == {}
    # An exact data row after RET is harmless to operand lookup. The same
    # row on the path to the virtual call must block CFG receiver proof.
    instructions.clear()
    addresses.clear()
    data_parts, offset = [], 0
    for raw, text in [*specs[:2], ("00 00 00 00 dd", "$Ltable"), *specs[2:]]:
        data_row = text == "$Ltable"
        encoded = bytes.fromhex(raw[:-3] if data_row else raw)
        instructions.append(Instruction(text=text, raw_text=text, bytes=tuple(raw.split()),
            source_line=(f"{offset:05x} 00 00 00 00 DD $Ltable" if data_row else f"{offset:05x}:")))
        addresses.append(0x1000 + offset)
        data_parts.append(encoded)
        offset += len(encoded)
    definition.data = b"".join(data_parts)
    definition.relocation_mask = bytes([0] + [1]*4 + [0]*(offset-5))
    from _recoil.commands.call_contract_verify import _candidate_coff_direct_call_identities
    assert _candidate_coff_direct_call_identities(instructions, addresses=addresses,
        caller_start=0x1000, definition=definition, indexes=indexes) == {0: identity}
    assert prove() == {}


def test_cfg_receiver_proof_includes_later_arrivals_at_the_same_call():
    from _recoil.commands.call_contract_verify import _exact_retail_cfg_register_provenance

    specs = [("8b f1", "mov esi, ecx"), ("8b 7e 20", "mov edi, [esi+0x20]"),
             ("8b 17", "mov edx, [edi]"), ("8b cf", "mov ecx, edi"),
             ("ff 52 20", "call [edx+0x20]"), ("8b 7e 24", "mov edi, [esi+0x24]"),
             ("eb f4", "jmp 0x1005")]
    instructions, addresses, cursor = [], [], 0x1000
    for raw, text in specs:
        instructions.append(Instruction(text=text, raw_text=text, bytes=tuple(raw.split()),
            source_line=f"{cursor:08x}:"))
        addresses.append(cursor)
        cursor += len(raw.split())
    result = _exact_retail_cfg_register_provenance(instructions, before_index=4,
        register="edx", addresses=addresses,
        indexes=IdentityIndexes(by_address={}, by_candidate_name={}, provider_ids=frozenset(),
            storage_by_address={}, storage_by_name={}),
        caller_start=0x1000, caller_end=cursor, local_control_flow_indices=frozenset(),
        local_control_flow_targets={})
    assert result != "load(load(this+0x20))"
    assert result == "load(load(runtime-object-join(this+0x20,this+0x24)))"


def test_cfg_array_cursor_requires_zero_entry_and_exact_recurrence():
    from _recoil.commands.call_contract_verify import _exact_retail_cfg_register_provenance

    def prove(field=0x20, stride=0x2c0, initialized=True, clobber=False, contract=False,
              candidate=False, corrupt_coff=False, relocated_cursor=False):
        specs = [("8b f1", "mov esi, ecx"),
                 ("33 ff" if initialized else "8b fb", "xor edi, edi" if initialized else "mov edi, ebx"),
                 (f"8b 4e {field:02x}", f"mov ecx, [esi+0x{field:x}]"),
                 ("03 cf", "add ecx, edi"), ("8b 11", "mov edx, [ecx]"),
                 ("ff 52 20", "call [edx+0x20]")]
        if clobber:
            specs.append(("8b fb", "mov edi, ebx"))
        specs += [("81 c7 " + stride.to_bytes(4, "little").hex(" "), f"add edi, 0x{stride:x}"),
                  ("81 ff 00 10 00 00", "cmp edi, 0x1000")]
        branch_address = 0x1000 + sum(len(raw.split()) for raw, _ in specs)
        specs += [(f"72 {(0x1004-branch_address-2)&255:02x}",
                   "jb 0x4" if candidate else "jb 0x1004"), ("c3", "ret")]
        instructions, addresses, cursor = [], [], 0x1000
        for raw, text in specs:
            instructions.append(Instruction(text=text, raw_text=text, bytes=tuple(raw.split()),
                source_line=f"{cursor:08x}:"))
            addresses.append(cursor)
            cursor += len(raw.split())
        if candidate:
            from types import SimpleNamespace as Row
            from _recoil.commands.call_contract_verify import _exact_candidate_cfg_vptr_proofs
            data = bytes.fromhex(" ".join(raw for raw, _ in specs))
            if corrupt_coff:
                data = b"\x90" + data[1:]
            relocations = [Row(offset=6, type=6)] if relocated_cursor else []
            return _exact_candidate_cfg_vptr_proofs(instructions, addresses=addresses,
                caller_start=0x1000,
                indexes=IdentityIndexes(by_address={}, by_candidate_name={}, provider_ids=frozenset(),
                    storage_by_address={}, storage_by_name={}),
                definition=Row(data=data, relocations=relocations, relocation_mask=b"\0" * len(data),
                    coff_symbols=[]), local_control_flow_indices=frozenset(),
                local_control_flow_targets={})
        if contract:
            from _recoil.commands.call_contract_verify import extract_invocation_contract
            return extract_invocation_contract(instructions, source="bn", caller_identity="symbol:unit",
                caller_start="0x1000", caller_end_exclusive=hex(cursor),
                indexes=IdentityIndexes(by_address={}, by_candidate_name={}, provider_ids=frozenset(),
                    storage_by_address={}, storage_by_name={}))
        return _exact_retail_cfg_register_provenance(instructions, before_index=5,
            register="edx", addresses=addresses,
            indexes=IdentityIndexes(by_address={}, by_candidate_name={}, provider_ids=frozenset(),
                storage_by_address={}, storage_by_name={}),
            caller_start=0x1000, caller_end=cursor, local_control_flow_indices=frozenset(),
            local_control_flow_targets={}, allow_exact_affine_receiver_roots=True)

    assert prove() == "load(affine(load(this+0x20),bounded-stride(bounded-counter(edi),0x2c0)*1))"
    assert prove(field=0x24) == "load(affine(load(this+0x24),bounded-stride(bounded-counter(edi),0x2c0)*1))"
    assert prove(stride=0x2ac) == "load(affine(load(this+0x20),bounded-stride(bounded-counter(edi),0x2ac)*1))"
    assert prove(initialized=False) == ""
    assert prove(clobber=True) == ""
    assert prove(stride=0) == ""
    assert prove(contract=True)[0]["storage_identity"] == prove()
    assert prove(candidate=True) == {5: prove()}
    assert prove(candidate=True, field=0x24) == {5: prove(field=0x24)}
    assert prove(candidate=True, stride=0x2ac) == {5: prove(stride=0x2ac)}
    assert prove(candidate=True, corrupt_coff=True) == {}
    assert prove(candidate=True, relocated_cursor=True) == {}
    for invalid in ({"initialized": False}, {"clobber": True}, {"stride": 0}):
        assert prove(candidate=True, **invalid) == {}
        with pytest.raises(ValueError):
            prove(contract=True, **invalid)


def test_cfg_indexed_receiver_preserves_index_source_and_rejects_unknowns():
    from _recoil.commands.call_contract_verify import _exact_retail_cfg_register_provenance

    indexes = IdentityIndexes(by_address={}, by_candidate_name={}, provider_ids=frozenset(),
        storage_by_address={}, storage_by_name={})

    def prove(field=0x20, array=0x40, index_register="eax", clobber=None, known=True,
              from_entry=False):
        index_code = 0 if index_register == "eax" else 2
        specs = [("8b f1", "mov esi, ecx")]
        if known:
            if from_entry:
                specs.append((f"8b {0x44 + (index_code << 3):02x} 24 {field:02x}",
                              f"mov {index_register}, [esp+0x{field:x}]"))
            else:
                specs.append((f"8b {0x46 + (index_code << 3):02x} {field:02x}",
                              f"mov {index_register}, [esi+0x{field:x}]"))
        if clobber:
            specs.append(clobber)
        specs += [(f"8b 4c {0x86 + (index_code << 3):02x} {array:02x}",
                   f"mov ecx, [esi+{index_register}*4+0x{array:x}]"),
                  ("8b 11", "mov edx, [ecx]"),
                  ("ff 52 20", "call [edx+0x20]"), ("c3", "ret")]
        instructions, addresses, cursor = [], [], 0x1000
        for raw, text in specs:
            instructions.append(Instruction(text=text, raw_text=text, bytes=tuple(raw.split()),
                source_line=f"{cursor:08x}:"))
            addresses.append(cursor)
            cursor += len(raw.split())
        return _exact_retail_cfg_register_provenance(
            instructions, before_index=len(instructions)-2, register="edx", addresses=addresses,
            indexes=indexes, caller_start=0x1000, caller_end=cursor,
            local_control_flow_indices=frozenset(), local_control_flow_targets={})

    assert prove() == "load(load(affine(this,load(this+0x20)*4,+0x40)))"
    assert prove(index_register="edx") == prove()
    assert prove(field=0x24) == "load(load(affine(this,load(this+0x24)*4,+0x40)))"
    assert prove(array=0x44) == "load(load(affine(this,load(this+0x20)*4,+0x44)))"
    assert prove(from_entry=True, field=4) == "load(load(affine(this,load(entry-stack+0x4)*4,+0x40)))"
    assert prove(from_entry=True, field=8) == "load(load(affine(this,load(entry-stack+0x8)*4,+0x40)))"
    assert prove(from_entry=True, field=0) == ""
    assert prove(from_entry=True, field=1) == ""
    assert prove(known=False) == ""
    for clobber in (("b0 00", "mov al, 0"), ("8b c3", "mov eax, ebx"),
                    ("33 c0", "xor eax, eax")):
        assert prove(clobber=clobber) == ""


def test_constructor_authority_requires_each_live_definition_without_retired_wrapper(monkeypatch):
    from pathlib import Path
    from types import SimpleNamespace as Row
    import pytest
    from _recoil.commands import call_contract_verify as contract

    for field, value in (
        ("HUD_NET_GAME_SETUP_NUMERIC_CONSTRUCTOR_SOURCE_PATH", "base.cpp"),
        ("HUD_NET_GAME_SETUP_NUMERIC_CONSTRUCTOR_SYMBOL", "number"),
        ("HUD_NET_GAME_SETUP_CYCLE_SELECTOR_CONSTRUCTOR_SYMBOL", "cycle"),
        ("HUD_CONFIRM_QUIT_BASE_CONSTRUCTOR_SYMBOL", "widget"),
    ):
        monkeypatch.setattr(contract, field, value)
    definitions = {name: Row(symbol=name) for name in ("number", "cycle", "widget")}
    requested = []

    def extract(cod, obj, names):
        requested.append(names)
        return definitions.copy()

    monkeypatch.setattr(contract, "_candidate_selected_constructor_definitions", extract)
    target = Row()
    unit = (Row(source_from="base.cpp"), Path("base.cod"), Row())
    unrelated = (Row(source_from="other.cpp"), Path("other.cod"), Row())

    def acquire(units):
        return contract._compile_hud_numeric_constructor_authority(
            build_root=Path("unused"), vc5_env=Path("unused"), precompiled=(target, units),
        )

    actual = acquire((unit, unrelated))
    assert actual[0] is target
    assert [set(group) for group in actual[1:]] == [{"number"}, {"cycle"}, {"widget"}]
    assert requested == [{"number", "cycle", "widget"}]
    for units in ((unrelated,), (unit, unit)):
        with pytest.raises(ValueError):
            acquire(units)
    saved = definitions.pop("number")
    with pytest.raises(ValueError):
        acquire((unit,))
    definitions["number"] = saved
    definitions["unexpected"] = Row(symbol="unexpected")
    with pytest.raises(ValueError):
        acquire((unit,))

from pathlib import Path
import sys

import pytest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from _recoil.lib.call_contract_evidence import (  # noqa: E402
    CallContractEvidenceError,
    json_evidence_value,
)
from _recoil.lib.call_contract_generations import (  # noqa: E402
    current_generations,
    evidence_generations_current,
    generation_increment_findings,
    required_call_contract_verifier_component_findings,
    required_call_contract_verifier_component_graph,
)
from _recoil.lib.zeroarg_abi import (  # noqa: E402
    SourceSymbolPair,
    SymbolPair,
    ZeroArgAbiTarget,
    eligibility_gates,
    manifest_symbol_normalization,
)
from _recoil.commands.asm_verify import Instruction  # noqa: E402
from _recoil.commands.call_contract_verify import (  # noqa: E402
    IdentityIndexes,
    _exact_targetless_vptr_call_proofs,
    _normalize_call_contract_row,
)


def test_json_evidence_boundary_returns_an_alias_free_native_copy() -> None:
    source = {"calls": ({"ordinal": 0, "cleanup": None},), "passed": True}
    copied = json_evidence_value(source)
    assert copied == {"calls": [{"ordinal": 0, "cleanup": None}], "passed": True}
    assert copied is not source
    assert copied["calls"] is not source["calls"]


def test_argument_bits_compare_equivalent_operations_and_reject_wrong_flags() -> None:
    from _recoil.lib.call_argument_bits import ArgumentBits, ArgumentProofError

    def prove(rows: list[str]) -> tuple[object, ...]:
        instructions = ["mov ebx, ecx", *rows, "push edx", "call eax"]
        cfg = {i: (i + 1,) for i in range(len(instructions) - 1)}
        return ArgumentBits(instructions, cfg).stack_argument(len(instructions) - 1, 0)

    first = prove(["mov dl, byte [ebx+9]", "and edx, 2", "shl edx, 6", "or edx, 64"])
    second = prove(["mov dl, byte [ebx+9]", "and edx, 2", "or edx, 1", "shl edx, 6"])
    assert first == second
    assert first != prove(["mov dl, byte [ebx+9]", "and edx, 2", "shl edx, 6", "or edx, 32"])
    assert first != prove(["mov dl, byte [ebx+8]", "and edx, 2", "shl edx, 6", "or edx, 64"])
    with pytest.raises(ArgumentProofError, match="volatile"):
        prove(["mov edx, 1", "call eax"])


def test_argument_bits_require_unanimous_paths_and_reject_unknown_stack_effects() -> None:
    from _recoil.lib.call_argument_bits import ArgumentBits, ArgumentProofError

    instructions = ["test eax, eax", "je somewhere", "mov edx, 1", "jmp join", "mov edx, 2", "push edx", "call eax"]
    cfg = {0: (1,), 1: (2, 4), 2: (3,), 3: (5,), 4: (5,), 5: (6,)}
    with pytest.raises(ArgumentProofError, match="conflicting"):
        ArgumentBits(instructions, cfg).stack_argument(6, 0)
    instructions[4] = "mov edx, 1"
    assert ArgumentBits(instructions, cfg).stack_argument(6, 0)[0] == 1
    with pytest.raises(ArgumentProofError, match="unresolved"):
        ArgumentBits(instructions, cfg, frozenset({1})).stack_argument(6, 0)
    bad = ["push 1", "mov dword [esp], 2", "call eax"]
    with pytest.raises(ArgumentProofError, match="overwritten"):
        ArgumentBits(bad, {0: (1,), 1: (2,)}).stack_argument(2, 0)


def test_constructor_dispatch_requires_a_this_store_and_complete_relocated_table() -> None:
    from types import SimpleNamespace
    from _recoil.lib.constructor_dispatch import exact_table_slots, leaf_constructor_vptr_write

    body = [bytes.fromhex("8b c1"), bytes.fromhex("c7 00 00 10 40 00"), bytes.fromhex("c7 40 04 00 00 00 00"), b"\xc3"]
    assert leaf_constructor_vptr_write(body) == (4, 0x401000)
    for invalid in (body[:1] + body[2:], [bytes.fromhex("8b c3"), *body[1:]], body[:-1], [*body, b"\xc3"]):
        with pytest.raises(ValueError):
            leaf_constructor_vptr_write(invalid)
    relocations = [SimpleNamespace(offset=0, type=6, symbol_name="_first"), SimpleNamespace(offset=4, type=6, symbol_name="_second")]
    assert exact_table_slots(bytes(8), relocations, 2) == ("_first", "_second")
    for invalid in (relocations[:1], [relocations[0], relocations[0]], [SimpleNamespace(offset=0, type=20, symbol_name="_first"), relocations[1]]):
        with pytest.raises(ValueError):
            exact_table_slots(bytes(8), invalid, 2)
    with pytest.raises(ValueError, match="addends"):
        exact_table_slots(b"\x01" + bytes(7), relocations, 2)


@pytest.mark.parametrize("value", [b"bytes", object(), {1: "bad"}, float("nan"), float("inf")])
def test_json_evidence_boundary_rejects_implicit_or_nonfinite_values(value: object) -> None:
    with pytest.raises(CallContractEvidenceError):
        json_evidence_value(value)


def test_generation_contract_has_only_reviewed_integer_coordinates() -> None:
    generations = current_generations()
    assert set(generations) == {
        "call_contract_verifier_generation",
        "expected_fact_schema_version",
    }
    assert all(type(value) is int and value > 0 for value in generations.values())
    assert evidence_generations_current(generations)
    assert not evidence_generations_current({**generations, "expected_fact_schema_version": 0})


def test_constructor_body_padding_cannot_hide_code_or_relocations() -> None:
    from types import SimpleNamespace
    from _recoil.lib.constructor_dispatch import validate_leaf_listing_body

    instructions = [bytes.fromhex("c7 01 00 00 00 00"), b"\xc3"]
    body = b"".join(instructions)
    refs = [SimpleNamespace(offset=2)]
    validate_leaf_listing_body(instructions, body, refs)
    validate_leaf_listing_body(instructions, body + b"\x90" * 9, refs)
    for data, relocations in (
        (body + b"\xc3" + b"\x90" * 8, refs),
        (body + b"\x90" * 25, refs),
        (body + b"\x90", refs),
        (body + b"\x90" * 9, [*refs, SimpleNamespace(offset=8)]),
        (b"\x90" + body[1:], refs),
    ):
        with pytest.raises(ValueError):
            validate_leaf_listing_body(instructions, data, relocations)


def test_dispatch_weak_default_requires_exact_coff_chain() -> None:
    from copy import deepcopy
    from types import SimpleNamespace as S
    from _recoil.lib.constructor_dispatch import resolve_table_weak_target

    relocation = S(symbol_name="_weak", symbol_index=1)
    symbols = [
        S(index=1, name="_weak", storage_class=105, section_number=0, value=0,
          symbol_type=0x20, aux_count=1, weak_external_tag_index=3,
          weak_external_characteristics=2),
        S(index=3, name="_actual", storage_class=2, section_number=0, value=0,
          symbol_type=0x20, aux_count=0),
        S(index=4, name="_actual", storage_class=2, section_number=5, value=0,
          symbol_type=0x20, aux_count=0, section_name=".text", section_characteristics=0x20),
    ]
    assert resolve_table_weak_target(relocation, symbols) == "_actual"
    for index, field, value in (
        (0, "name", "_other"), (0, "weak_external_tag_index", 4),
        (0, "weak_external_characteristics", 3), (0, "aux_count", 0),
        (1, "storage_class", 105), (2, "section_name", ".data"),
    ):
        changed = deepcopy(symbols)
        setattr(changed[index], field, value)
        with pytest.raises(ValueError):
            resolve_table_weak_target(relocation, changed)
    with pytest.raises(ValueError):
        resolve_table_weak_target(relocation, symbols[:2])
    with pytest.raises(ValueError):
        resolve_table_weak_target(relocation, [*symbols, symbols[2]])


def test_constructor_member_store_preserves_only_proven_saved_this() -> None:
    from _recoil.lib.constructor_dispatch import straight_constructor_member_store

    rows = [bytes.fromhex(value) for value in (
        "56", "8b f1", "e8 00 00 00 00", "c7 46 20 00 10 40 00", "5e", "c3",
    )]
    assert straight_constructor_member_store(rows, 0x20) == (11, 0x401000)
    for altered in (
        [rows[0], bytes.fromhex("8b f3"), *rows[2:]],
        [rows[0], bytes.fromhex("8b c1"), rows[2], bytes.fromhex("c7 40 20 00 10 40 00"), *rows[4:]],
        [*rows[:3], bytes.fromhex("74 07"), *rows[3:]],
        [*rows[:4], rows[3], *rows[4:]],
        [*rows[:3], bytes.fromhex("c7 46 24 00 10 40 00"), *rows[4:]],
        rows[:-1], [*rows, b"\xc3"],
    ):
        with pytest.raises(ValueError):
            straight_constructor_member_store(altered, 0x20)


def test_constant_callback_return_cannot_hide_calls_or_wrong_value() -> None:
    from types import SimpleNamespace
    from _recoil.lib.constructor_dispatch import constant_return

    one = bytes.fromhex("b8 01 00 00 00 c3")
    assert constant_return(one) == constant_return(one + b"\x90" * 10) == 1
    assert constant_return(bytes.fromhex("33 c0 c3")) == 0
    for body in (one + b"\xc3", one + b"\x90", b"\xe8\x00\x00\x00\x00" + one, one[:-1]):
        with pytest.raises(ValueError):
            constant_return(body)
    with pytest.raises(ValueError):
        constant_return(one, [SimpleNamespace(offset=1)])


def test_constructor_cfg_requires_the_same_this_stamp_on_every_return() -> None:
    from _recoil.lib.constructor_dispatch import constructor_vptr_store

    rows = [bytes.fromhex(value) for value in (
        "56", "8b f1", "e8 00 00 00 00", "c7 06 00 10 40 00",
        "85 c0", "74 02", "8b ce", "5e", "c3",
    )]
    assert constructor_vptr_store(rows) == (10, 0x401000)
    for altered in (
        [rows[0], bytes.fromhex("8b f3"), *rows[2:]],  # not entry this
        [*rows[:3], bytes.fromhex("74 06"), *rows[3:]],  # stamp bypass
        [*rows[:4], rows[3], *rows[4:]],  # overwritten vptr
        [*rows[:4], bytes.fromhex("c7 46 fe 00 20 40 00"), *rows[4:]],
        [*rows[:4], bytes.fromhex("c7 00 00 20 40 00"), *rows[4:]],
        [*rows[:4], bytes.fromhex("eb fe"), *rows[4:]],  # unbounded loop
        rows[:-1],
    ):
        with pytest.raises(ValueError):
            constructor_vptr_store(altered)


def test_constructor_cfg_kills_volatile_and_partial_register_aliases() -> None:
    from _recoil.lib.constructor_dispatch import constructor_vptr_store

    for prefix in (
        ("8b c1", "e8 00 00 00 00"),
        ("8b c1", "8a 44 24 04"),
    ):
        with pytest.raises(ValueError):
            constructor_vptr_store([bytes.fromhex(row) for row in
                                    (*prefix, "c7 00 00 10 40 00", "c3")])


def test_global_virtual_receiver_window_rejects_bypass_and_wrong_provenance() -> None:
    from _recoil.lib.constructor_dispatch import global_vptr_call_window

    rows = [bytes.fromhex(row) for row in (
        "8b 0d 00 20 40 00", "8b 15 00 30 40 00", "52", "8b 01", "ff 10",
    )]
    assert global_vptr_call_window(rows) == (4, 2, 0x402000)
    for altered in (
        [bytes.fromhex("8b 15 00 20 40 00"), *rows[1:]],
        [rows[0], bytes.fromhex("8b 0d 00 30 40 00"), b"\x51", *rows[3:]],
        [*rows[:3], bytes.fromhex("8b 03"), rows[4]],
        [*rows[:4], bytes.fromhex("ff 50 04")],
        [bytes.fromhex("74 06"), *rows],
        [*rows, *rows],
    ):
        with pytest.raises(ValueError):
            global_vptr_call_window(altered)


def test_invocation_comparison_retains_dependency_and_argument_obligations() -> None:
    from copy import deepcopy
    from _recoil.commands.call_contract_verify import compare_call_contracts

    base = {"ordinal": 0, "form": "call", "dispatch": "direct", "identity_kind": "direct",
            "target_identity": "symbol:unit", "storage_identity": "",
            "slot_displacement": None, "cleanup_bytes": 0}
    for field, facts, changed in (
        ("constructor_dispatch", {"slots": ["symbol:first", "symbol:second"]}, {"slots": ["symbol:second", "symbol:first"]}),
        ("member_dispatch_return", {"return_value": 1}, {"return_value": 0}),
        ("concrete_update_dispatch", {"target": "symbol:override"}, {"target": "symbol:base"}),
        ("argument_bits", {"4": [0, 1]}, {"4": [1, 0]}),
    ):
        expected = {**base, field: facts}
        assert compare_call_contracts([expected], [deepcopy(expected)])["passed"]
        assert not compare_call_contracts([expected], [base])["passed"]
        assert not compare_call_contracts([expected], [{**base, field: changed}])["passed"]


def test_explicit_lifecycle_target_cannot_be_replaced_by_ambiguous_fallback() -> None:
    from _recoil.commands.call_contract_verify import _explicit_lifecycle_target_identity

    assert _explicit_lifecycle_target_identity("symbol:unit", {"symbol:unit"}) == "symbol:unit"
    for prior, candidates in (
        (None, {"symbol:unit"}), ("", {"symbol:unit"}),
        ("symbol:other", {"symbol:unit"}),
        ("symbol:unit", {"symbol:unit", "symbol:other"}),
        ("symbol:unit", set()),
    ):
        assert _explicit_lifecycle_target_identity(prior, candidates) == ""


def test_invocation_comparison_does_not_expand_deletion_or_fold_volume_calls() -> None:
    from _recoil.commands.call_contract_verify import compare_call_contracts

    base = {"ordinal": 0, "form": "call", "dispatch": "direct", "identity_kind": "direct",
            "target_identity": "symbol:destructor", "storage_identity": "",
            "slot_displacement": None, "cleanup_bytes": None}
    expected = [base, {**base, "ordinal": 1, "target_identity": "provider:delete", "cleanup_bytes": 4}]
    helper = {**base, "target_identity": "candidate-local-coff:scalar-delete", "cleanup_bytes": 4}
    assert not compare_call_contracts(expected, [helper])["passed"]
    volume = {**base, "dispatch": "indirect", "identity_kind": "virtual-slot",
              "target_identity": "", "storage_identity": "load(this)", "slot_displacement": 0x3c}
    assert not compare_call_contracts([volume, {**volume, "ordinal": 1}], [volume])["passed"]
    assert not compare_call_contracts([volume], [{**volume, "slot_displacement": 0xf4}])["passed"]
    # Identical indirect encodings do not prove the same receiver or table.
    fixed = {**volume, "target_identity": "symbol:virtual-method",
             "storage_identity": "constructor-table:0x2000"}
    assert compare_call_contracts([fixed], [dict(fixed)])["passed"]
    for changed in (
        {"target_identity": ""},
        {"target_identity": "symbol:other-method"},
        {"storage_identity": "load(this+0x20)"},
        {"storage_identity": "constructor-table:0x3000"},
        {"dispatch": "direct", "identity_kind": "direct", "storage_identity": "",
         "slot_displacement": None},
    ):
        assert not compare_call_contracts([fixed], [{**fixed, **changed}])["passed"]
    # Native leaf sequences require no reconstruction-specific helper graph.
    assert compare_call_contracts(expected, [dict(row) for row in expected])["passed"]
    assert compare_call_contracts([], [])["passed"]
    for candidate in (
        [expected[1], expected[0]],
        [{**base, "form": "tail"}, expected[1]],
        [{**base, "dispatch": "indirect", "identity_kind": "callback"}, expected[1]],
        [{**base, "target_identity": "candidate-local-coff:inline-wrapper"}, expected[1]],
        [base, {**expected[1], "cleanup_bytes": 8}],
        [*expected, {**base, "ordinal": 2}],
    ):
        assert not compare_call_contracts(expected, candidate)["passed"]
    assert not compare_call_contracts([], [helper])["passed"]


def test_relocated_body_equivalence_requires_all_bytes_and_exact_target_semantics():
    from dataclasses import replace
    from types import SimpleNamespace as Row
    from _recoil.commands.call_contract_verify import (
        CandidateCallerDefinition, _identical_relocated_call_body,
    )

    start, target = 0x1000, 0x5000
    prefix = bytes.fromhex("8b 41 20 8b 10 8b c8 ff 52 60 e8")
    retail = prefix + (target - start - 15).to_bytes(4, "little", signed=True) + b"\xc3"
    candidate = prefix + bytes(4) + b"\xc3"
    relocation = Row(offset=11, type=20, symbol_name="_callee")
    definition = CandidateCallerDefinition(
        symbol="_caller", data=candidate, relocations=(relocation,),
        relocation_mask=tuple(11 <= offset < 15 for offset in range(16)),
        undefined_external_functions=("_callee",),
    )
    indexes = IdentityIndexes(
        by_address={hex(target): "symbol:callee"},
        by_candidate_name={"_callee": "symbol:callee", "_other": "symbol:other"},
        provider_ids=frozenset(), storage_by_address={}, storage_by_name={},
    )

    def equal(value=definition, reference=retail, identities=indexes):
        return _identical_relocated_call_body(reference, value, retail_start=start,
            image_start=0x1000, image_end=0x10000, indexes=identities)

    assert equal()
    # Same byte length and relocation count must not hide changed receivers,
    # slots, opcodes, targets, cleanup, or addends.
    for offset in (0, 2, 8, 9, 11, 15):
        body = bytearray(candidate)
        body[offset] ^= 1
        assert not equal(replace(definition, data=bytes(body)))
    for changes in (
        {"relocations": ()},
        {"relocations": (relocation, relocation)},
        {"relocations": (Row(offset=11, type=6, symbol_name="_callee"),)},
        {"relocations": (Row(offset=11, type=20, symbol_name="_other"),)},
        {"relocations": (Row(offset=10, type=20, symbol_name="_callee"),)},
        {"relocation_mask": (False,) * 16},
        {"data": candidate + b"\x90"},
    ):
        assert not equal(replace(definition, **changes))
    assert not equal(identities=replace(indexes,
        by_address={hex(target): "symbol:callee", "0x6000": "symbol:callee"}))
    assert not equal(reference=retail[:11] + (target + 1 - start - 15).to_bytes(4, "little") + b"\xc3")

    absolute = bytes.fromhex("a1 00 20 00 00 ff 50 60 c3")
    absolute_candidate = b"\xa1" + bytes(4) + absolute[5:]
    absolute_definition = replace(definition, data=absolute_candidate,
        relocations=(Row(offset=1, type=6, symbol_name="_global"),),
        relocation_mask=(False, True, True, True, True, False, False, False, False))
    absolute_indexes = replace(indexes,
        storage_by_address={"0x2000": "storage:global"},
        storage_by_name={"_global": "storage:global"})
    assert equal(absolute_definition, absolute, absolute_indexes)
    assert not equal(absolute_definition, absolute, indexes)
    assert not equal(replace(absolute_definition, relocations=(),
        data=absolute, relocation_mask=(False,) * len(absolute)), absolute, absolute_indexes)


def test_exact_body_virtual_lineage_checks_candidate_listing_and_slot(monkeypatch):
    from dataclasses import replace
    from types import SimpleNamespace as Row
    from _recoil.commands import call_contract_verify as contract

    body = bytes.fromhex("8b 01 ff 50 60 c3")
    class Handle:
        def __enter__(self):
            return self
        def __exit__(self, *_):
            return False
        def read(self):
            return body
    monkeypatch.setattr(contract, "StableReadHandle", lambda _: Handle())
    monkeypatch.setattr(contract, "parse_pe_headers", lambda *_a, **_k:
        Row(image_base=0x1000, size_of_image=0x1000, sections=()))
    monkeypatch.setattr(contract, "rva_to_offset", lambda *_: 0)
    monkeypatch.setattr(contract, "_candidate_complete_instruction_offsets", lambda _: (0, 2, 5))
    instructions = tuple(Instruction(text=text, raw_text=text,
        bytes=tuple(encoded.split()), source_line=text) for text, encoded in (
            ("mov eax, dword ptr [ecx]", "8b 01"),
            ("call dword ptr [eax+0x60]", "ff 50 60"), ("ret", "c3")))
    definition = contract.CandidateCallerDefinition(symbol="_caller", data=body,
        relocations=(), relocation_mask=(False,) * len(body), undefined_external_functions=())
    candidate = Row(caller_definition=definition, instructions=instructions)
    expected = {"ordinal": 0, "form": "call", "dispatch": "indirect",
        "identity_kind": "virtual-slot", "storage_identity": "load(this)",
        "target_identity": "", "slot_displacement": 0x60, "cleanup_bytes": None}
    indexes = contract.IdentityIndexes(by_address={}, by_candidate_name={},
        provider_ids=frozenset(), storage_by_address={}, storage_by_name={})

    def prove(value=candidate, row=expected):
        return contract._exact_body_vptr_candidate_bridges(value, caller_start="0x1000",
            caller_end_exclusive="0x1006", indexes=indexes, expected=(row,),
            retail_call_sites=("0x1002",))
    proof = prove()["0x2"]
    assert (proof.storage_identity, proof.target_identity, proof.slot_displacement) == ("load(this)", "", 0x60)
    assert not prove(row={**expected, "slot_displacement": 0x64})
    assert not prove(row={**expected, "dispatch": "direct"})
    assert not prove(Row(caller_definition=replace(definition, data=body[:1] + b"\x09" + body[2:]),
        instructions=instructions))
    assert not prove(Row(caller_definition=definition, instructions=(
        replace(instructions[0], bytes=("8b", "09")), *instructions[1:])))


def test_pooled_data_alias_index_requires_unique_physical_storage_and_evidence():
    from copy import deepcopy
    from types import SimpleNamespace as Row
    from _recoil.commands.call_contract_verify import _index_reviewed_pooled_data_names

    name, key = "??_C@_00A@?$AA@", "data:pooled-literal"
    alias = {"kind": "data", "object_symbol": name, "evidence_ids": ["proof"],
        "pooling": {"mode": "compiler-literal-pooling", "physical_artifact_id": key,
            "evidence_ids": ["proof"]}}
    physical = {"binary": "recoil", "kind": "data", "disposition": "provider",
        "address": "0x2000", "end_exclusive": "0x2001", "size": 1,
        "extent_state": "known", "source_traceability": {"state": "not-applicable",
            "reason_code": "compiler-linker-literal-pooling", "source_edges": []},
        "logical_aliases": {"first": alias, "second": deepcopy(alias)}}
    def index(rows=None, prior=None, addresses=None):
        document = Row(collection=lambda collection:
            ({key: physical} if rows is None else rows) if collection == "symbols" else {"proof": {}})
        result = dict(prior or {})
        _index_reviewed_pooled_data_names(document,
            storage_by_address={"0x2000": f"storage:{key}"} if addresses is None else addresses,
            storage_by_name=result)
        return result.get(name)
    assert index() == f"storage:{key}"
    assert index(prior={name: ""}) == ""
    assert index(prior={name: "storage:other"}) == ""
    assert index(addresses={}) == ""
    for change in ({"size": 2}, {"size": True}, {"extent_state": "unknown"},
                   {"disposition": "authored"}, {"address": "invalid"}):
        assert index({key: {**physical, **change}}) == ""
    for change in ({"mode": "unreviewed"}, {"physical_artifact_id": "other"},
                   {"evidence_ids": []}, {"evidence_ids": ["missing"]}):
        broken = deepcopy(physical)
        broken["logical_aliases"]["first"]["pooling"].update(change)
        assert index({key: broken}) == ""
    competing = deepcopy(physical)
    competing.update(address="0x3000", end_exclusive="0x3001")
    for value in competing["logical_aliases"].values():
        value["pooling"]["physical_artifact_id"] = "other"
    assert index({key: physical, "other": competing}, addresses={
        "0x2000": f"storage:{key}", "0x3000": "storage:other"}) == ""
    from _recoil.commands.call_contract_verify import _pooled_data_manifest_supplier
    document = Row(collection=lambda _: {key: physical})
    def supplier(address="0x2000", size=1, identity=f"storage:{key}"):
        return _pooled_data_manifest_supplier(document, target_id="target:unit",
            row=Row(symbol=name, address=address, byte_length=size), identity=identity)
    assert supplier().storage_identity == f"storage:{key}"
    assert supplier("0x3000") is None
    assert supplier(size=2) is None
    assert supplier(size=True) is None
    assert supplier(identity="") is None
    assert supplier(identity="storage:missing") is None


def test_wrapped_disp32_mov_retains_only_exact_listing_coordinates():
    from types import SimpleNamespace as Row
    from _recoil.commands.call_contract_verify import parse_assembly, _candidate_complete_instruction_offsets

    first = "00000 8b 81 ec 01 00"
    second = " 00 mov eax, DWORD PTR [ecx+492]"
    ending = "00006 c3 ret"
    def offsets(lines):
        return _candidate_complete_instruction_offsets(Row(instructions=
            parse_assembly("\n".join(lines), source="cod")))
    assert offsets((first, second, ending)) == (0, 6)
    assert offsets(("00000 8b 05 00 00 00", " 00 mov eax, DWORD PTR _global", ending)) == (0, 6)
    assert offsets((first, "; intervening row", second, ending))[0] is None
    assert offsets(("00000 8b 84 ec 01 00", second, ending))[0] is None
    assert offsets((first, second, "00000 c3 ret"))[0] is None
    assert offsets((first, " 00 add eax, DWORD PTR [ecx+492]", ending))[0] is None


def test_private_relocation_labels_preserve_partition_and_ordinary_identity():
    from types import SimpleNamespace as Row
    from _recoil.commands.call_contract_verify import _canonical_zui_relocation_names

    def normalize(names):
        return _canonical_zui_relocation_names([Row(symbol_name=name) for name in names])
    expected = ("$L<local-0>", "$T<local-0>", "$L<local-0>", "$L<local-1>", "_ordinary")
    assert normalize(("$L17", "$T92", "$L17", "$L18", "_ordinary")) == expected
    assert normalize(("$L63", "$T12", "$L63", "$L71", "_ordinary")) == expected
    assert normalize(("$L17", "$T92", "$L18", "$L18", "_ordinary")) != expected
    assert normalize(("$Loop", "$T1suffix", "?Member@@", "_ordinary2")) == (
        "$Loop", "$T1suffix", "?Member@@", "_ordinary2")


def test_raw_call_count_guard_precedes_helper_projections_and_checks_retail_census() -> None:
    from types import SimpleNamespace as Row
    from _recoil.commands.call_contract_verify import _raw_call_count_divergence
    from _recoil.commands.asm_verify import Instruction

    def ins(text):
        return Instruction(text=text, raw_text=text, bytes=(), source_line=text)
    retail = [ins("call cleanup"), ins("call delete"), ins("ret")]
    expected = [{"form": "call"}, {"form": "call"}]
    one_helper = Row(instructions=[ins("call deleting_helper"), ins("ret")])
    rejected = _raw_call_count_divergence(expected, one_helper, retail)
    assert rejected["kind"] == "verifier-blocked"
    assert rejected["side"] == "candidate"
    assert (rejected["retail_call_count"], rejected["candidate_call_count"]) == (2, 1)
    exact = Row(instructions=retail)
    assert _raw_call_count_divergence(expected, exact, retail) is None
    assert _raw_call_count_divergence(expected, Row(instructions=retail + [ins("call extra")]), retail)
    assert _raw_call_count_divergence(expected[:1], exact, retail)["side"] == "expected"
    # Equal CALL counts do not accept the body or normalize a tail transfer.
    assert _raw_call_count_divergence(expected, Row(instructions=retail + [ins("jmp other")]), retail) is None
    # The caller must independently prove funclet boundaries before supplying
    # these physical CALL indices. Parent extras still reject, and the complete
    # candidate inventory is not edited by this rejection-only census.
    funclets = Row(instructions=retail + [ins("call catch_a"), ins("call catch_b")])
    assert _raw_call_count_divergence(expected, funclets, retail)
    assert _raw_call_count_divergence(expected, funclets, retail,
        proved_funclet_call_indices=(3, 4)) is None
    extra_parent = Row(instructions=funclets.instructions + [ins("call parent_extra")])
    divergence = _raw_call_count_divergence(expected, extra_parent, retail,
        proved_funclet_call_indices=(3, 4))
    assert (divergence["candidate_call_count"], divergence["candidate_total_call_count"],
            divergence["candidate_funclet_call_count"]) == (3, 5, 2)
    for invalid in ((3, 3), (4, 3), (-1,), (2,), (5,)):
        with pytest.raises(ValueError, match="invalid funclet partition"):
            _raw_call_count_divergence(expected, funclets, retail,
                proved_funclet_call_indices=invalid)
    assert len(funclets.instructions) == 5


@pytest.mark.parametrize("mnemonic", ["bts", "btr", "btc"])
def test_bit_modify_instructions_kill_register_provenance(mnemonic) -> None:
    from _recoil.commands.call_contract_verify import _instruction_may_clobber_register
    from _recoil.commands.asm_verify import Instruction

    text = f"{mnemonic} ebx, 1"
    instruction = Instruction(text=text, raw_text=text, bytes=(), source_line=text)
    assert _instruction_may_clobber_register(instruction, "ebx")
    assert not _instruction_may_clobber_register(instruction, "esi")


def test_dynamic_probe_coordinates_allow_only_uniform_translation() -> None:
    from _recoil.commands.call_contract_verify import _translated_dynamic_probe_setup

    setup = ((8, b"\x24\xfc"), (18, b"\x24\xfc"))
    for delta in (-1, 0, 3):
        assert _translated_dynamic_probe_setup((10, 20), (10 + delta, 20 + delta), setup) == tuple(
            (offset + delta, body) for offset, body in setup
        )
    for reviewed, observed, rows in (
        ((), (), setup), ((10, 20), (10,), setup),
        ((10, 20), (10, None), setup), ((10, 20), (10, 10), setup),
        ((20, 10), (20, 10), setup), ((10, 20), (20, 10), setup),
        ((10, 20), (9, 20), setup), ((10, 20), (0, 10), setup),
        ((10, 20), (10, 20), ()), ((10, 20), (10, 20), ((8, b""),)),
        ((10, 20), (True, 20), setup), ((-1, 9), (0, 10), setup),
    ):
        with pytest.raises(ValueError):
            _translated_dynamic_probe_setup(reviewed, observed, rows)


def test_generation_component_graph_is_complete_and_operational() -> None:
    paths = {row["path"] for row in required_call_contract_verifier_component_graph()}
    assert "tools/_recoil/lib/call_contract_evidence.py" in paths
    assert "tools/_recoil/lib/zeroarg_abi.py" in paths
    assert required_call_contract_verifier_component_findings(ROOT) == []
    assert generation_increment_findings(
        ["tools/_recoil/lib/call_contract_evidence.py"], current_generations()
    ) == [
        "call_contract_verifier_generation: component changes require an increment; touched tools/_recoil/lib/call_contract_evidence.py",
        "expected_fact_schema_version: component changes require an increment; touched tools/_recoil/lib/call_contract_evidence.py",
    ]


def test_zeroarg_abi_policy_requires_every_semantic_gate() -> None:
    evidence = {
        "free_or_static": True,
        "explicit_argument_count": 0,
        "direct_calls": [{
            "address": "0x1000",
            "dispatch": "direct",
            "explicit_argument_count": 0,
            "callee_return": "plain-ret",
        }],
        **{name: False for name in (
            "hidden_this", "hidden_sret", "lifecycle", "variadic", "address_taken",
            "callback", "vtable", "export", "import", "function_pointer",
        )},
    }
    target = ZeroArgAbiTarget(
        target_id="unit", identity="unit-identity", callee_source=Path("callee.cpp"),
        callee=SymbolPair("_callee", "@callee@0"),
        callers=(SourceSymbolPair(Path("caller.cpp"), "_caller", "@caller@0"),),
        return_category="integral32", retail_evidence=evidence,
        eh_policy={"kind": "none", "retail_proven": True}, st0_policy=None,
    )
    assert all(row["passed"] for row in eligibility_gates(target))
    assert manifest_symbol_normalization((target,))["@callee@0"] == "unit-identity"


def _targetless_branch_receiver_instructions(
    *, member_receiver: bool
) -> tuple[Instruction, ...]:
    def row(address: int, raw_text: str, encoded: str) -> Instruction:
        return Instruction(
            text=raw_text,
            raw_text=raw_text,
            bytes=tuple(encoded.split()),
            source_line=f"{address:08x}:",
        )

    return (
        row(
            0x1000,
            "lea ebp, [ecx+0x20]" if member_receiver else "lea ebp, [esp]",
            "8d 69 20" if member_receiver else "8d 2c 24",
        ),
        row(0x1003, "test eax, eax", "85 c0"),
        row(0x1005, "je 0x1010", "74 09"),
        row(0x1007, "call 0x2000", "e8 f4 0f 00 00"),
        row(0x100C, "mov ebp, eax", "8b e8"),
        row(0x100E, "jmp 0x1018", "eb 08"),
        row(0x1010, "mov edx, [ebp]", "8b 55 00"),
        row(0x1013, "mov ecx, ebp", "8b cd"),
        row(0x1015, "call dword ptr [edx+0x78]", "ff 52 78"),
        row(0x1018, "ret", "c3"),
    )


def _targetless_branch_receiver_proofs(
    *,
    member_receiver: bool,
    source: str = "bn",
    allow_exact_this_member: bool = True,
) -> dict[int, str]:
    return _exact_targetless_vptr_call_proofs(
        _targetless_branch_receiver_instructions(
            member_receiver=member_receiver
        ),
        source=source,
        caller_start="0x1000",
        caller_end_exclusive="0x1019",
        indexes=IdentityIndexes(
            by_address={"0x2000": "symbol:dead-factory"},
            by_candidate_name={},
            provider_ids=frozenset(),
            storage_by_address={},
            storage_by_name={},
        ),
        allow_exact_this_member=allow_exact_this_member,
    )


def test_targetless_vptr_prefers_cfg_unanimous_this_member_over_dead_branch() -> None:
    proofs = _targetless_branch_receiver_proofs(member_receiver=True)

    assert proofs[8] == "load(this+0x20)"


def test_targetless_vptr_rejects_dead_branch_factory_without_live_member() -> None:
    proofs = _targetless_branch_receiver_proofs(member_receiver=False)

    assert 8 not in proofs


def test_targetless_vptr_does_not_replace_reviewed_retail_member_provenance() -> None:
    proofs = _targetless_branch_receiver_proofs(
        member_receiver=True,
        source="bn",
        allow_exact_this_member=False,
    )

    assert 8 not in proofs


def test_runtime_vptr_does_not_inherit_an_unrelated_construction_table():
    from _recoil.commands.call_contract_verify import extract_invocation_contract

    indexes = IdentityIndexes(by_address={"0x3000": "symbol:unrelated-method"},
        by_candidate_name={}, provider_ids=frozenset(),
        storage_by_address={"0x2000": "storage:unrelated-construction-table"},
        storage_by_name={})
    def extract(field=0x20, clobber=None):
        specs = [("mov ebx, ecx", "8b d9"),
            ("mov dword [edi], 0x2000", "c7 07 00 20 00 00"),
            (f"mov esi, dword [ebx+0x{field:x}]", f"8b 73 {field:02x}"),
            ("mov edx, dword [esi]", "8b 16"),
            ("mov ecx, esi", "8b ce")]
        if clobber:
            specs.append(clobber)
        specs += [("call dword [edx+0x60]", "ff 52 60"), ("ret", "c3")]
        rows, address = [], 0x1000
        for text, body in specs:
            rows.append(Instruction(text=text, raw_text=text, bytes=tuple(body.split()),
                source_line=f"{address:08x}:"))
            address += len(body.split())
        return extract_invocation_contract(rows, source="bn", caller_identity="symbol:unit",
            caller_start="0x1000", caller_end_exclusive=hex(address), indexes=indexes)
    first, changed = extract(), extract(field=0x24)
    assert len(first) == len(changed) == 1
    assert first[0]["target_identity"] == changed[0]["target_identity"] == ""
    assert first[0]["storage_identity"] == "load(exact-receiver-field(this,+0x20))"
    assert changed[0]["storage_identity"] == "load(exact-receiver-field(this,+0x24))"
    assert first[0]["slot_displacement"] == changed[0]["slot_displacement"] == 0x60
    for clobber in (("xor edx, edx", "33 d2"), ("mov dl, 0", "b2 00"),
                    ("xchg eax, edx", "92"), ("cdq", "99")):
        try:
            result = extract(clobber=clobber)
        except ValueError:
            continue
        pytest.fail(f"Clobber {clobber[0]} retained a callable lineage: {result}")


def test_path_resource_cleanup_distinguishes_exclusive_and_sequential_calls() -> None:
    from _recoil.lib.path_contract import path_depths
    call = (bytes.fromhex("e8 00 00 00 00"), "call")
    ret = (b"\xc3", "ret")
    # Same static call population: one acquire and two release sites.
    exclusive = [call, (b"\x74\x06", "je"), call, ret, call, ret]
    assert path_depths(exclusive, b"".join(x[0] for x in exclusive), {0: 1, 7: -1, 13: -1}) == {
        "return_depths": [0], "peak_depth": 1,
    }
    sequential = [call, call, call, ret]
    with pytest.raises(ValueError, match="underflow"):
        path_depths(sequential, b"".join(x[0] for x in sequential), {0: 1, 5: -1, 10: -1})
    leak = [call, ret]
    assert path_depths(leak, b"".join(x[0] for x in leak), {0: 1})["return_depths"] == [1]
    for rows, effects in (
        ([call, ret], {}),
        ([(b"\xeb\x01", "jmp"), ret], {}),
        ([(b"\xff\xe0", "jmp")], {}),
        ([call, (b"\xeb\xf9", "jmp")], {0: 1}),
        ([ret, call], {1: 0}),
    ):
        with pytest.raises(ValueError):
            path_depths(rows, b"".join(x[0] for x in rows), effects)
    with pytest.raises(ValueError, match="listing"):
        path_depths([ret], b"\x90", {})
    # A zero-effect loop converges; its conditional exit is still checked.
    loop = [(b"\x75\xfe", "jne"), ret]
    assert path_depths(loop, b"\x75\xfe\xc3", {})["return_depths"] == [0]
    assert path_depths([ret], b"\xc3\x90", {})["return_depths"] == [0]
    with pytest.raises(ValueError, match="listing"):
        path_depths([ret], b"\xc3\xcc", {})
    with pytest.raises(ValueError, match="boundaries"):
        path_depths([(b"\xeb\x00", "jmp")], b"\xeb\x00\x90", {})
    indirect = [(b"\xff\x10", "call"), ret]
    with pytest.raises(ValueError, match="effect"):
        path_depths(indirect, b"\xff\x10\xc3", {})
    assert path_depths(indirect, b"\xff\x10\xc3", {0: 0})["return_depths"] == [0]


def test_image_path_listing_preserves_wrapped_instruction_bytes() -> None:
    from _recoil.commands.startup_contract import parse_image_listing
    listing = "  00001000: C7 44 24 18 00 00  mov dword ptr [esp+18h],0\n            00 00\n  00001008: C3                 ret\n"
    rows = parse_image_listing(listing, 0x1000, 9)
    assert rows == [(bytes.fromhex("c7 44 24 18 00 00 00 00"), "mov"), (b"\xc3", "ret")]
    assert parse_image_listing("  401000: c3   ret\n", 0x401000, 1) == [(b"\xc3", "ret")]
    with pytest.raises(ValueError, match="noncontiguous"):
        parse_image_listing(listing.replace("00001008", "00001007"), 0x1000, 9)


def test_virtual_slot_normalization_folds_exact_this_member_lea_spelling() -> None:
    row = {
        "ordinal": 2,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": "load(address(this+0x3c))",
        "slot_displacement": 0x60,
        "cleanup_bytes": None,
    }

    normalized = _normalize_call_contract_row(row, candidate_side=True)

    assert normalized["storage_identity"] == "load(this+0x3c)"
