from __future__ import annotations


def test_aggregate_field_receiver_requires_coff_and_every_cfg_arrival():
    from dataclasses import replace
    from types import SimpleNamespace as Row
    from _recoil.call_contract import cfg, instructions, listing, receiver_candidate
    from _recoil.call_contract.records import CandidateCallerDefinition
    symbol, displacement = '?manager', 0x20

    def prove(register='eax', *, mutation='', bypass=False, clobber='', loop=False):
        code = {'eax': 0, 'edx': 2, 'ebx': 3, 'esi': 6, 'edi': 7}[register]
        operations = []
        if bypass:
            operations += [('entry', '85 ff'), ('bypass', ('74', 'vptr'))]
        operations += [('receiver', '8b 0d 20000000'), ('test', '85 c9'),
            ('guard', ('74', 'end')), ('vptr', bytes((0x8b, 1 | code << 3)).hex())]
        if clobber:
            killed = 1 if clobber == 'receiver' else code
            operations += [('kill', bytes((0x33, 0xc0 | killed << 3 | killed)).hex())]
        operations += [('argument', '6a 01'), ('call', bytes((0xff, 0x50 | code, 0x60)).hex())]
        if loop:
            operations += [('loop', ('75', 'vptr'))]
        # A second legitimate field load after the call must not make the
        # actual call's reaching definition ambiguous, or rescue a bad one.
        operations += [('reload', '8b 0d 20000000'), ('save', '51'), ('end', 'c3')]
        offsets, size = {}, 0
        for label, raw in operations:
            offsets[label] = size
            size += 2 if isinstance(raw, tuple) else len(bytes.fromhex(raw))
        body = b''.join(bytes((int(raw[0], 16), (offsets[raw[1]] - offsets[label] - 2) & 255))
            if isinstance(raw, tuple) else bytes.fromhex(raw) for label, raw in operations)
        text = '\n'.join(f'{i.address:05x} {i.bytes.hex(" ")} {i.mnemonic} {i.op_str}'
            for i in instructions._decoder().disasm(body, 0))
        text = text.replace('dword ptr [0x20]', f'dword {symbol}+32')
        rows = listing.parse_assembly(text, source='cod')
        addresses = cfg._instruction_runtime_addresses(rows, source='cod', caller_start=0x1000)
        refs = [Row(offset=offsets[k]+2, type=6, symbol_name=symbol, symbol_index=1)
            for k in ('receiver', 'reload')]
        mask = [False]*len(body)
        for r in refs:
            mask[r.offset:r.offset+4] = [True]*4
        definition = CandidateCallerDefinition(symbol='caller', data=body, relocations=tuple(refs),
            relocation_mask=tuple(mask), undefined_external_functions=(),
            undefined_external_data=(symbol,), coff_symbols=(Row(index=1, name=symbol),))
        if mutation == 'relocation-type': refs[0].type = 0x14
        if mutation == 'relocation-symbol': refs[0].symbol_name = '?other'
        if mutation == 'relocation-index': refs[0].symbol_index = 2
        if mutation == 'relocation-offset': refs[0].offset += 1
        if mutation == 'duplicate-relocation': definition = replace(definition, relocations=(*refs, refs[0]))
        if mutation == 'duplicate-symbol': definition = replace(definition, coff_symbols=definition.coff_symbols*2)
        if mutation == 'missing-symbol': definition = replace(definition, coff_symbols=())
        if mutation == 'defined-data': definition = replace(definition, defined_external_data=(symbol,))
        if mutation == 'missing-data': definition = replace(definition, undefined_external_data=())
        if mutation == 'short-mask': definition = replace(definition, relocation_mask=definition.relocation_mask[:-1])
        if mutation == 'mask-opcode': mask[offsets['receiver']] = True
        if mutation == 'missing-mask': mask[offsets['receiver']+2] = False
        if mutation in {'mask-opcode', 'missing-mask'}: definition = replace(definition, relocation_mask=tuple(mask))
        if mutation == 'stale-listing': definition = replace(definition, data=body[:-1]+b'\x90')
        if mutation == 'text-register':
            index = addresses.index(0x1000+offsets['receiver'])
            prior = rows[index]
            rows[index] = replace(prior, raw_text=prior.raw_text.replace('ecx,', 'eax,'))
        if mutation == 'wrong-addend':
            index = addresses.index(0x1000+offsets['receiver']);prior=rows[index]
            rows[index] = replace(prior, bytes=('8b','0d','24','00','00','00'))
            at = offsets['receiver']+2
            definition = replace(definition, data=body[:at]+b'\x24'+body[at+1:])
        result = receiver_candidate._candidate_aggregate_field_vptr_calls(rows,
            addresses=addresses, caller_start=0x1000, definition=definition,
            aggregate_symbol=symbol, displacement=displacement, slot_displacement=0x60)
        return result

    for register in ('eax', 'edx', 'ebx', 'esi', 'edi'):
        assert len(prove(register)) == 1
        assert not prove(register, bypass=True)
        assert not prove(register, clobber='receiver')
        assert not prove(register, clobber='vptr')
        assert not prove(register, loop=True)
    for mutation in ('relocation-type', 'relocation-symbol', 'relocation-index',
            'relocation-offset', 'duplicate-relocation', 'duplicate-symbol',
            'missing-symbol', 'defined-data', 'missing-data', 'short-mask',
            'mask-opcode', 'missing-mask', 'stale-listing', 'text-register', 'wrong-addend'):
        assert not prove(mutation=mutation), mutation


def test_receiver_argument_coordinates_require_complete_stack_effects():
    from _recoil.call_contract import cfg, instructions, listing, receiver_proofs, receiver_candidate
    from _recoil.call_contract.records import IdentityIndexes, CandidateCallerDefinition
    indexes = IdentityIndexes(by_address={}, by_candidate_name={}, provider_ids=frozenset(),
        storage_by_address={}, storage_by_name={})
    for source in ('bn', 'cod'):
        for argument in (4, 8):
            for known in (True, False):
                # Save registers, call through an opaque register, then reload
                # one incoming object. Only a known effect restores its exact
                # entry coordinate; a new post-call stack origin cannot do so.
                body = bytes.fromhex('53 56 57 6a 01 ff d0 8b 4c 24') + bytes([argument+12]) + bytes.fromhex('8b 11 6a 00 ff 52 60 c3')
                start = 0x1000 if source == 'bn' else 0
                width = 8 if source == 'bn' else 5
                rows = listing.parse_assembly('\n'.join(f'{i.address:0{width}x} {i.bytes.hex(" ")} {i.mnemonic} {i.op_str.replace(" + ", "+").replace(" - ", "-")}'
                    for i in instructions._decoder().disasm(body, start)), source=source)
                definition = CandidateCallerDefinition(symbol='caller', data=body, relocations=(),
                    relocation_mask=(False,)*len(body), undefined_external_functions=(), section_end=len(body))
                proofs = receiver_proofs._exact_targetless_vptr_call_proofs(rows, source=source,
                    caller_start='0x1000', caller_end_exclusive=hex(0x1000+len(body)), indexes=indexes,
                    candidate_caller_definition=definition if source == 'cod' else None,
                    call_cleanup_by_instruction_index={4:4} if known else {})
                value = proofs.get(8, '')
                if known:
                    assert value == f'load(load(entry-stack+0x{argument:x}))'
                else:
                    assert not receiver_candidate._exact_entry_stack_vptr(value)
    for value in ('load(load(stack+0x8))', 'load(load(entry-stack+0x0))',
                  'load(load(entry-stack+0x6))', 'bounded-stack-receiver-vptr'):
        assert not receiver_candidate._exact_entry_stack_vptr(value)


def test_nested_cursor_reuse_requires_restoring_the_exact_outer_local():
    from _recoil.call_contract import cfg, instructions, listing
    from _recoil.call_contract.receiver_stack_loops import spilled_scalar_seeds, natural_zero_stride_seeds
    def prove(*, restore='8b 5c 24 04', cleanup=4, clobber=False, word=False, opaque=False, inner=False):
        operations=[('entry','8b f1'), ('zero','33 db'), ('frame','83 ec 08'), ('seed','89 5c 24 04'),
            ('outer','6a 01'), ('call','ff 50 0c'), ('check','85 ff'), ('skip',('74','join')),
            ('inner_zero','33 db'), ('inner','90'), ('inner_step','83 c3 18'), ('inner_cmp','83 fb 30'),
            ('inner_tail',('7c','inner')), ('restore',restore), ('join','90'),
            ('step','66 83 c3 18' if word else '83 c3 18'), ('cmp','83 ff 03'), ('save','89 5c 24 04'),
            ('tail',('7c','outer')), ('end','83 c4 08'), ('return','c3')]
        if clobber:
            operations.insert(6,('clobber','c7 44 24 04 00 00 00 00'))
        offsets={};size=0
        for name,code in operations:
            offsets[name]=size;size+=2 if isinstance(code,tuple) else len(bytes.fromhex(code))
        data=b''.join(bytes((int(code[0],16),(offsets[code[1]]-offsets[name]-2)&255))
            if isinstance(code,tuple) else bytes.fromhex(code) for name,code in operations)
        rows=listing.parse_assembly('\n'.join(f'{i.address:08x} {i.bytes.hex(" ")} {i.mnemonic} {i.op_str}'
            for i in instructions._decoder().disasm(data,0x401000)),source='bn')
        addresses=cfg._instruction_runtime_addresses(rows,source='bn',caller_start=0x401000)
        by_address={a:i for i,a in enumerate(addresses)}
        indices={n:by_address[0x401000+at] for n,at in offsets.items()}
        successors,unresolved=cfg._exact_invocation_cfg(rows,instruction_addresses=addresses,
            instruction_index_by_address=by_address,source='bn',caller_start=0x401000,caller_end=0x401000+len(data),
            local_control_flow_indices=frozenset(),local_control_flow_targets={})
        if inner:
            return natural_zero_stride_seeds(rows,before_index=indices['inner'],successors=successors,unresolved=unresolved)
        result=spilled_scalar_seeds(rows,successors=successors,unresolved=unresolved,before_index=indices['call'],
            cleanups={indices['call']:cleanup} if cleanup is not None else {},
            opaque_indices={indices['step']} if opaque else frozenset())
        return result,indices
    result,indices=prove()
    assert result == {indices['seed']:('ebx',4,24,indices['step'])}
    assert len(prove(inner=True)) == 2
    for changes in ({'restore':'8b 5c 24 00'}, {'restore':'90'}, {'cleanup':None},
                    {'cleanup':0}, {'clobber':True}, {'word':True}, {'opaque':True}):
        assert prove(**changes)[0] == {}


def test_native_allocation_stamp_dominates_every_nonnull_returning_call_path():
    from _recoil.call_contract import cfg, instructions, listing
    from _recoil.call_contract.allocation_callees import guarded_stamp_calls
    root = 'allocation-result(provider:allocator,0)'
    value = f'load(nullable({root}))'
    def prove(*, guard='74 08', test='85 f6', zero='null', flags=False, stamp=True, allocation=True, root_match=True, overwrite=False):
        body = bytes.fromhex('e8 00000000 8b f0 '+test+' '+guard+' c7 06 78563412 eb 02 33 f6 8b 06 8b ce ff 50 0c c3')
        if flags:
            body = body[:7]+bytes.fromhex('33 c0')+body[9:]
        if overwrite:
            body = body[:21]+bytes.fromhex('89 06')+body[23:]
        rows = listing.parse_assembly('\n'.join(f'{i.address:08x} {i.bytes.hex(" ")} {i.mnemonic} {i.op_str}'
            for i in instructions._decoder().disasm(body,0x401000)),source='bn')
        addresses = cfg._instruction_runtime_addresses(rows,source='bn',caller_start=0x401000)
        roots = lambda i,r: root if root_match and r=='esi' else zero if r=='ebp' else ''
        return guarded_stamp_calls(rows,addresses=addresses,source='bn',start=0x401000,end=0x401000+len(body),
            root_before=roots,stamp_indices={4} if stamp else set(),
            allocation_indices={root:0} if allocation else {},call_values={9:value})
    assert prove() == {9:value}
    assert prove(test='3b f5') == {9:value}
    assert prove(test='3b ee') == {9:value}
    for changes in ({'guard':'75 08'}, {'guard':'74 07'}, {'flags':True}, {'stamp':False},
                    {'allocation':False}, {'root_match':False}, {'overwrite':True},
                    {'test':'3b f5','zero':''}, {'test':'3b f5','zero':'load(this+0x8)'}):
        assert prove(**changes) == {}


def test_spilled_cursor_induction_requires_exact_stack_cycle_and_unescaped_local():
    from _recoil.call_contract import cfg, instructions, listing
    from _recoil.call_contract.receiver_stack_loops import prove_stack_cursor_loop
    from _recoil.call_contract.records import IdentityIndexes, CandidateCallerDefinition
    indexes = IdentityIndexes(by_address={}, by_candidate_name={}, provider_ids=frozenset(),
        storage_by_address={}, storage_by_name={})

    def prove(*, reload='8b 5c 24 04', cleanup=4, clobber=False, escape=False, bypass=False, word=False,
              source='bn', drift=False):
        codes = ['8b f1', '33 db', '83 ec 08', '89 5c 24 04',
            '8b 56 20', '03 da', '8b 2b', '6a 01', '8b cb', 'ff 55 0c',
            reload, '66 83 c3 18' if word else '83 c3 18', '89 5c 24 04']
        if clobber:
            codes += ['c7 44 24 04 00 00 00 00']
        if escape:
            codes += ['8d 44 24 04']
        if bypass:
            codes[4] = 'eb 02 90'
        codes += ['83 ff 03']
        prefix = bytes.fromhex(' '.join(codes))
        body = prefix + bytes((0x75, (11-len(prefix)-2) & 255)) + bytes.fromhex('83 c4 08 c3')
        width = 8 if source == 'bn' else 5
        rows = listing.parse_assembly('\n'.join(f'{i.address:0{width}x} {i.bytes.hex(" ")} {i.mnemonic} {i.op_str}'
            for i in instructions._decoder().disasm(body, 0x401000 if source=='bn' else 0)), source=source)
        addresses = cfg._instruction_runtime_addresses(rows, source=source, caller_start=0x401000)
        definition = CandidateCallerDefinition(symbol='caller', data=(b'\x90'+body[1:]) if drift else body,
            relocations=(), relocation_mask=(False,)*len(body), undefined_external_functions=(), section_end=len(body))
        return prove_stack_cursor_loop(rows, addresses=addresses, source=source, start=0x401000,
            end=0x401000+len(body), indexes=indexes, cleanups={9: cleanup} if cleanup is not None else {},
            definition=definition if source=='cod' else None)

    assert prove() == {9: 'load(cursor(load(this+0x20),+0x18))'}
    assert prove(source='cod') == prove()
    assert prove(source='cod', drift=True) == {}
    for changes in ({'reload': '8b 5c 24 00'}, {'cleanup': 0}, {'cleanup': None},
                    {'clobber': True}, {'escape': True}, {'bypass': True}, {'word': True}):
        assert prove(**changes) == {}


def test_local_cdecl_import_bridge_requires_exact_call_site_and_coff_population():
    from copy import deepcopy
    from dataclasses import replace
    from types import SimpleNamespace as Row
    from _recoil.call_contract.candidate_imports import _provider_named_import_thunk_candidate_direct_bridges as prove
    from _recoil.call_contract.listing import parse_assembly
    from _recoil.call_contract.records import CandidateAssembly, CandidateCallerDefinition, IdentityIndexes
    name, identity, site = "_tmpfile", "iat:tmpfile", "0x400010"
    route = ("0x401000", identity)
    symbol = Row(index=1, name=name, section_number=0, value=0, storage_class=2, symbol_type=0x20)
    relocation = Row(offset=1, type=0x14, symbol_name=name, symbol_index=1)
    caller = CandidateCallerDefinition(symbol="caller", data=bytes.fromhex("e8 00000000 c3"),
        relocations=(relocation,), relocation_mask=(False, True, True, True, True, False),
        undefined_external_functions=(name,), defined_external_functions=(), coff_symbols=(symbol,),
        section_index=1, section_start=0, section_end=6)
    candidate = CandidateAssembly(instructions=tuple(parse_assembly(
        "00000 e8 00 00 00 00 call _tmpfile\n00005 c3 ret", source="cod")),
        local_control_flow_indices=frozenset(), caller_definition=caller)
    indexes = IdentityIndexes(by_address={}, by_candidate_name={}, provider_ids=frozenset(),
        storage_by_address={}, storage_by_name={},
        reviewed_direct_import_thunk_by_call_site={site: route},
        reviewed_exact_local_import_thunk_by_call_site={site: route})
    expected = [dict(form="call", dispatch="direct", identity_kind="iat", target_identity=identity,
                     storage_identity="", slot_displacement=None)]
    imports = (Row(import_name="tmpfile", dll="MSVCRT.dll", import_ordinal=None),)
    kwargs = dict(thunks=(), indexes=indexes, retail_call_sites=(site,), retail_import_targets=imports)
    assert prove(expected, candidate, **kwargs) == ({name: identity}, {identity: identity})
    for changes in ({"retail_call_sites": ()}, {"retail_call_sites": ("0x400020",)},
                    {"retail_import_targets": imports * 2},
                    {"retail_import_targets": (Row(import_name="tmpfile", dll="other.dll", import_ordinal=None),)},
                    {"indexes": replace(indexes, reviewed_direct_import_thunk_by_call_site={})},
                    {"indexes": replace(indexes, by_candidate_name={name: "symbol:other"})}):
        with pytest.raises(ValueError):
            prove(expected, candidate, **(kwargs | changes))
    for field, value in (("dispatch", "indirect"), ("target_identity", "iat:other"),
                         ("form", "tail"), ("storage_identity", "iat:tmpfile")):
        with pytest.raises(ValueError):
            prove([expected[0] | {field: value}], candidate, **kwargs)
    for field, value in (("type", 6), ("symbol_index", 2), ("offset", 2)):
        bad = deepcopy(relocation)
        setattr(bad, field, value)
        with pytest.raises(ValueError):
            prove(expected, replace(candidate, caller_definition=replace(caller, relocations=(bad,))), **kwargs)
    for changes in ({"data": bytes.fromhex("e8 01000000 c3")},
                    {"coff_symbols": (symbol, symbol)}, {"undefined_external_functions": (name, name)},
                    {"relocation_mask": (False,) * 6}):
        with pytest.raises(ValueError):
            prove(expected, replace(candidate, caller_definition=replace(caller, **changes)), **kwargs)
    _exercise_complete_iat_candidate_composition()
    _exercise_reviewed_storage_registration()


def _exercise_reviewed_storage_registration():
    from types import SimpleNamespace as Row
    from _recoil.call_contract.storage_identity import _reviewed_storage_registration_agrees as agrees
    kwargs = dict(address="0x401000", identity="storage:table", object_symbols=("?table",))
    def prove(addresses, names):
        return agrees(Row(storage_by_address=addresses, storage_by_name=names), **kwargs)
    assert prove({}, {})
    assert prove({"0x401000": "storage:table"}, {})
    assert prove({"0x401000": "storage:table"}, {"?table": "storage:table"})
    assert prove({}, {"?table": "storage:table"})
    for wrong in ("", "storage:other"):
        assert not prove({"0x401000": wrong}, {})
        assert not prove({}, {"?table": wrong})
    assert not prove({}, {"?Table": "storage:table"})
    assert not prove({}, {"?table": "storage:table", "?TABLE": "storage:table"})
    from _recoil.call_contract.storage_identity import _reviewed_registered_or_provisional_storage as select
    def selected(addresses, names):
        return select(Row(storage_by_address=addresses, storage_by_name=names),
            address="0x401000", registered_identity="storage:table",
            provisional_identity="constructor-table:0x401000", object_symbols=("?table",))
    assert selected({}, {}) == "constructor-table:0x401000"
    assert selected({"0x401000": "storage:table"}, {}) == "storage:table"
    assert selected({"0x401000": "storage:table"}, {"?table": "storage:table"}) == "storage:table"
    for wrong in ("", "storage:other", "constructor-table:0x401000"):
        assert selected({"0x401000": wrong}, {}) == ""
    assert selected({}, {"?table": "storage:table"}) == ""
    assert selected({"0x401000": "storage:table"}, {"?table": "constructor-table:0x401000"}) == ""


def _exercise_complete_iat_candidate_composition():
    from copy import deepcopy
    from dataclasses import replace
    from _recoil.call_contract.identity import _publish_current_iat_candidate_names as publish
    from _recoil.call_contract.records import CurrentIatStoragePackage
    package = CurrentIatStoragePackage(address="0x401000", import_dll="CRT.dll", import_name="convert",
        import_ordinal=None, object_symbol="__imp__convert", identity="iat:convert")
    provider = "provider:recoil:function:0x401000"
    original = dict(by_address={package.address:provider}, by_candidate_name={package.object_symbol:provider, "_convert":"provider:thunk"},
        provider_ids={provider, "provider:thunk"}, storage_by_address={package.address:package.identity},
        storage_by_name={package.object_symbol:package.identity, package.import_name:package.identity})
    indexes = deepcopy(original)
    publish((package,), **indexes)
    assert indexes["by_candidate_name"] == {package.object_symbol:package.identity, "_convert":"provider:thunk"}
    assert indexes["by_address"] == original["by_address"]
    assert indexes["provider_ids"] == original["provider_ids"]
    publish((package,), **indexes)
    assert indexes["by_candidate_name"][package.object_symbol] == package.identity
    for key, replacement in (
        ("by_address", {}), ("by_address", {package.address:"provider:other"}), ("provider_ids", set()),
        ("by_candidate_name", {package.object_symbol:"provider:other"}),
        ("by_candidate_name", {package.object_symbol:""}), ("by_candidate_name", {}),
        ("by_candidate_name", {package.object_symbol:provider, package.object_symbol.upper():provider}),
        ("storage_by_address", {package.address:"storage:other"}),
        ("storage_by_name", {package.object_symbol:package.identity}),
        ("storage_by_name", {package.object_symbol:"iat:other", package.import_name:package.identity}),
    ):
        indexes = deepcopy(original); indexes[key] = replacement
        before = deepcopy(indexes)
        publish((package,), **indexes)
        assert indexes == before, key
    for packages in ((), (package, package), (package, replace(package, address="0x402000")),
                     (package, replace(package, object_symbol="__imp__other"))):
        indexes = deepcopy(original)
        publish(packages, **indexes)
        assert indexes == original


def test_private_proof_owners_import_independently():
    import os
    from pathlib import Path
    import subprocess
    import sys
    root = Path(__file__).resolve().parents[2]
    environment = dict(os.environ, PYTHONPATH=str(root / "tools"))
    for path in sorted((root / "tools/_recoil/call_contract").glob("*.py")):
        result = subprocess.run([sys.executable, "-c", f"import _recoil.call_contract.{path.stem}"],
                                cwd=root, env=environment, capture_output=True, text=True)
        assert result.returncode == 0, result.stderr


def test_native_funclet_partition_accounts_for_every_call_and_rejects_location_drift():
    from dataclasses import replace
    from _recoil.call_contract.contributions import InvocationContribution, partition_native_funclets
    from _recoil.call_contract.records import AppFrameRunCatchFuncletProof
    physical = tuple(InvocationContribution("candidate", hex(offset), "call", "direct")
                     for offset in (0x400010, 0x400020, 0x400030, 0x400040))
    proof = AppFrameRunCatchFuncletProof(ranges=((0x20, 0x30), (0x30, 0x40)),
        lifecycle_addresses=("0x401000", "0x402000"), excluded_invocation_indices=(1, 2),
        excluded_invocation_ordinals=(1, 2), excluded_invocation_offsets=(0x20, 0x30), generic_invocation_count=4)
    primary, owned = partition_native_funclets(physical, proof, caller_start="0x400000")
    assert primary == (physical[0], physical[3])
    assert tuple(row.contribution for row in owned) == physical[1:3]
    assert tuple(row.owner for row in owned) == proof.lifecycle_addresses
    with pytest.raises(ValueError, match="locations"):
        partition_native_funclets(physical, replace(proof, excluded_invocation_offsets=(0x21, 0x30)), caller_start="0x400000")
    with pytest.raises(ValueError, match="ambiguous ownership"):
        partition_native_funclets(physical, replace(proof, ranges=((0x20, 0x40), (0x30, 0x40))), caller_start="0x400000")


def test_cod_cpu_rdtsc_keeps_real_instruction_boundaries_and_rejects_bad_directives():
    from _recoil.call_contract.listing import parse_assembly
    from _recoil.call_contract.instructions import instruction_fact
    text = "00000 0f DB 15\n; source comment\n00001 31 DB 49\n00002 89 45 e0 mov dword ptr [ebp-32], eax"
    rows = parse_assembly(text, source="cod")
    assert len(rows) == 2 and instruction_fact(rows[0]).mnemonic == "rdtsc"
    assert rows[1].bytes == ("89", "45", "e0")
    for changed in (text.replace("00001", "00003"), text.replace("00002", "00004")):
        directive = parse_assembly(changed, source="cod")[0]
        assert directive.text == "db 15" and directive.raw_text == "DB 15"
        assert directive.bytes == ("0f",)
    with pytest.raises(ValueError, match="byte directive"):
        parse_assembly(text.replace("DB 49", "DB 48"), source="cod")


def test_retail_negative_load_probe_does_not_publish_a_fact_for_direct_only_code():
    from _recoil.call_contract.dispatch import _retail_register_definition_transfer_addresses
    from _recoil.call_contract.listing import parse_assembly
    rows = parse_assembly("00400000 8b 35 00 10 40 00 mov esi, dword [0x401000]\n00400006 ff 24 85 00 20 40 00 jmp dword [eax*4+0x402000]", source="bn")
    assert _retail_register_definition_transfer_addresses(rows, load_index=0, destination="esi",
        caller_start=0x400000, caller_end=0x40000d, local_control_flow_indices=frozenset({1})) == ()


def test_iat_transfer_population_keeps_copies_and_unanimous_imports_but_not_conflicts():
    from dataclasses import replace
    from _recoil.call_contract.iat import _unique_retail_iat_transfer_proofs
    from _recoil.call_contract.records import CandidateExactIatRegisterLoadProof
    left = CandidateExactIatRegisterLoadProof("0x400000", "esi", "task", "iat:task", ("0x400030",), transfer_registers=("ebx",))
    right = replace(left, definition_offset="0x400010", destination="edi")
    result = _unique_retail_iat_transfer_proofs({left.definition_offset: left, right.definition_offset: right})
    assert len(result) == 2 and all(row.transfer_register("0x400030") == "ebx" for row in result.values())
    assert _unique_retail_iat_transfer_proofs({left.definition_offset: left,
        right.definition_offset: replace(right, identity="iat:other")}) == {}


def test_iat_copy_reaches_extraction_without_changing_its_definition_identity():
    from dataclasses import replace
    from _recoil.call_contract.extraction import extract_invocation_contract
    from _recoil.call_contract.listing import parse_assembly
    from _recoil.call_contract.records import CandidateExactIatRegisterLoadProof, IdentityIndexes
    rows = parse_assembly("00000 8b 35 00 00 00 00 mov esi, dword ptr __imp__task\n"
        "00006 8b fe mov edi, esi\n00008 33 f6 xor esi, esi\n0000a ff d7 call edi\n0000c c3 ret", source="cod")
    proof = CandidateExactIatRegisterLoadProof("0x0", "esi", "__imp__task", "iat:task", ("0xa",), transfer_registers=("edi",))
    indexes = IdentityIndexes(by_address={}, by_candidate_name={}, provider_ids=frozenset(),
        storage_by_address={}, storage_by_name={"__imp__task": "iat:task"})
    kwargs = dict(source="cod", caller_identity="symbol:caller", caller_start="0x400000",
                  caller_end_exclusive="0x40000d", indexes=indexes)
    result = extract_invocation_contract(rows, candidate_exact_iat_register_load_proofs={"0x0": proof}, **kwargs)
    assert len(result) == 1 and result[0]["target_identity"] == "iat:task"
    assert proof.destination == "esi" and proof.transfer_marker("0xa", joined=False).startswith("exact-iat-load(edi,")
    with pytest.raises(ValueError, match="ambiguous"):
        extract_invocation_contract(rows, candidate_exact_iat_register_load_proofs={"0x0": replace(proof, transfer_registers=("esi",))}, **kwargs)
    default = replace(proof, transfer_registers=())
    assert default == replace(proof, transfer_registers=("esi",))
    with pytest.raises(ValueError, match="populations disagree"):
        replace(proof, transfer_offsets=("0xa", "0xc"))


def test_complete_classifier_cfg_accepts_a_backward_case_without_bypassing_the_guard():
    from types import SimpleNamespace
    from _recoil.call_contract.cfg import retail_local_switch_targets
    from _recoil.call_contract.listing import parse_assembly
    text = "00400000 eb 03 jmp 0x400005\n00400002 ff d6 call esi\n00400004 c3 ret\n" \
        "00400005 83 f8 01 cmp eax, 1\n00400008 77 f8 ja 0x400002\n0040000a 33 c9 xor ecx, ecx\n" \
        "0040000c 8a 88 2a 00 40 00 mov cl, byte [eax+0x40002a]\n" \
        "00400012 ff 24 8d 22 00 40 00 jmp dword [ecx*4+0x400022]\n00400019 ff d6 call esi\n0040001b c3 ret"
    data = {0x400022: bytes.fromhex("02004000 19004000"), 0x40002a: b"\0\1"}
    bridge = SimpleNamespace(hexdump=lambda address, count: f"{int(address,16):08x} " + data[int(address,16)][:count].hex(" "))
    rows = parse_assembly(text, source="bn")
    kwargs = dict(caller_start="0x400000", caller_end_exclusive="0x40002c", bridge=bridge)
    assert retail_local_switch_targets(rows, **kwargs) == {}
    assert retail_local_switch_targets(rows, include_backward_targets=True, **kwargs) == {7: (1, 8)}
    bypass = parse_assembly(text.replace("eb 03 jmp 0x400005", "eb 0a jmp 0x40000c"), source="bn")
    assert retail_local_switch_targets(bypass, include_backward_targets=True, **kwargs) == {}
    data[0x400022] = bytes.fromhex("0a004000 19004000")
    assert retail_local_switch_targets(rows, include_backward_targets=True, **kwargs) == {}


def test_machine_arguments_accept_exact_near_ret_alias_but_reject_wrong_opcode():
    from types import SimpleNamespace
    from _recoil.lib.call_argument_bits import ArgumentBits, ArgumentProofError
    row = SimpleNamespace(bytes=bytes.fromhex("c20400"), text="retn 0x4")
    assert ArgumentBits.from_machine([row], {}).parts[0] == ("ret", ("0x4",))
    row.text = "retf 0x4"
    with pytest.raises(ArgumentProofError, match="opcode"):
        ArgumentBits.from_machine([row], {})


def test_checked_call_result_ignores_returned_failure_arms_but_rejects_rejoins():
    from _recoil.call_contract.listing import parse_assembly
    from _recoil.call_contract.retail import _checked_result_reaches_transfer
    rows = parse_assembly("00400000 e8 00 00 00 00 call 0x400005\n00400005 85 c0 test eax, eax\n"
        "00400007 75 06 jne 0x40000f\n00400009 e8 00 00 00 00 call 0x40000e\n"
        "0040000e c3 ret\n0040000f ff d0 call eax\n00400011 c3 ret", source="bn")
    assert _checked_result_reaches_transfer(rows, source="bn", caller_start=0x400000,
        result_index=0, transfer_index=5, register="eax")
    from dataclasses import replace
    rows[4] = replace(rows[4], bytes=("90",), text="nop", raw_text="nop")
    with pytest.raises(ValueError, match="partial or ambiguous"):
        _checked_result_reaches_transfer(rows, source="bn", caller_start=0x400000,
            result_index=0, transfer_index=5, register="eax")


def test_shared_instruction_effects_distinguish_explicit_partial_and_implicit_writes():
    from _recoil.call_contract.instructions import decode_bytes
    cases = (
        ("31db", {("ebx", 0, 32)}),
        ("85db", set()),
        ("b401", {("eax", 8, 8)}),
        ("f7e9", {("eax", 0, 32), ("edx", 0, 32)}),
        ("0fafc1", {("eax", 0, 32)}),
        ("99", {("edx", 0, 32)}),
        ("0fa2", {(name, 0, 32) for name in ("eax", "ebx", "ecx", "edx")}),
        ("dfe0", {("eax", 0, 16)}),
        ("f3a5", {(name, 0, 32) for name in ("ecx", "esi", "edi")}),
        ("e800000000", {(name, 0, 32) for name in ("eax", "ecx", "edx", "esp")}),
    )
    for code, expected in cases:
        assert set(decode_bytes(bytes.fromhex(code)).register_writes) == expected


def test_cod_hexadecimal_db_byte_is_not_a_data_directive():
    from _recoil.call_contract.cfg import _exact_invocation_cfg
    from _recoil.commands.asm_verify import Instruction
    rows = (
        Instruction(text="xor ebx, ebx", raw_text="xor ebx, ebx", bytes=("33", "db"), source_line=" 00000 33 db xor ebx, ebx"),
        Instruction(text="test ebx, ebx", raw_text="test ebx, ebx", bytes=("85", "db"), source_line=" 00002 85 db test ebx, ebx"),
        Instruction(text="ret", raw_text="ret", bytes=("c3",), source_line=" 00004 c3 ret"),
    )
    edges, unresolved = _exact_invocation_cfg(rows, instruction_addresses=(0, 2, 4),
        instruction_index_by_address={0: 0, 2: 1, 4: 2}, source="cod", caller_start=0,
        caller_end=5, local_control_flow_indices=frozenset(), local_control_flow_targets={})
    assert edges == {0: (1,), 1: (2,), 2: ()} and not unresolved


def test_current_stack_root_survives_exact_push_pop_but_not_stack_replacement():
    from types import SimpleNamespace
    from _recoil.call_contract.receiver_storage import _update_register_state
    from _recoil.commands.asm_verify import Instruction
    for code, text, expected_stack, expected_edx in (
        ("6a00", "push 0", "stack", "receiver"),
        ("52", "push edx", "stack", "receiver"),
        ("5a", "pop edx", "stack", ""),
        ("5c", "pop esp", "", "receiver"),
        ("6652", "push dx", "", "receiver"),
    ):
        registers = {"esp": "stack", "edx": "receiver"}
        row = Instruction(text=text, raw_text=text, bytes=tuple(f"{b:02x}" for b in bytes.fromhex(code)), source_line=text)
        _update_register_state(row, registers, assembly_source="bn", indexes=SimpleNamespace(),
                               reviewed_register_storage_bridges={})
        assert registers["esp"] == expected_stack
        assert registers["edx"] == expected_edx


def test_shared_definition_flow_handles_kills_aliases_joins_loops_and_independent_calls():
    from types import SimpleNamespace
    from _recoil.call_contract.flow import ControlFlow, FlowProofError, reaching_definition_uses
    import pytest

    def run(codes, edges=None, unresolved=frozenset()):
        rows = [SimpleNamespace(bytes=bytes.fromhex(code)) for code in codes]
        edges = edges if edges is not None else {i: [i + 1] for i in range(len(rows) - 1)}
        cfg = ControlFlow.from_edges(len(rows), edges, unresolved)
        return reaching_definition_uses(rows, cfg, definition_index=0, register="esi")

    # The unrelated EBX xor and indirect EDI call cannot manufacture, kill, or
    # invalidate the independent ESI definition used by the final call.
    assert run(["8b3500104000", "31db", "ffd7", "ffd6"]) == (3,)
    assert run(["8b3500104000", "89f3", "31f6", "ffd3"]) == (3,)
    assert run(["8b3500104000", "31f6", "ffd6"]) == ()
    assert run(["8b3500104000", "85db", "75fc", "ffd6"], {0: [1], 1: [2], 2: [1, 3]}) == (3,)
    with pytest.raises(FlowProofError, match="partial or ambiguous"):
        run(["8b3500104000", "7502", "31f6", "ffd6"], {0: [1], 1: [2, 3], 2: [3]})
    with pytest.raises(FlowProofError, match="partial or ambiguous"):
        run(["8b3500104000", "6689de", "ffd6"])
    with pytest.raises(FlowProofError, match="unresolved"):
        run(["8b3500104000", "90", "ffd6"], unresolved=frozenset({1}))
    # Attached data is not an executable predecessor. A changed edge that
    # enters it must still block the proof, even when the call is unchanged.
    assert run(["8b3500104000", "ffd6", "c3", "00"],
               {0: [1], 1: [2]}, frozenset({3})) == (1,)
    with pytest.raises(FlowProofError, match="unresolved"):
        run(["8b3500104000", "ffd6", "c3", "00"],
            {0: [3], 3: [1], 1: [2]}, frozenset({3}))


def test_proof_composition_preserves_contradictions_and_retail_obligation_origin():
    import pytest
    from _recoil.call_contract.proofs import Fact, Obligation, ProofMap, ProofConflict, ProofStatus, prove

    expected = Fact("retail", "receiver", ["this", 16], "retail:call:3")
    obligation = Obligation("receiver-vptr-slot", expected)
    candidate = Fact("candidate", "receiver", ["this", 20], "candidate:call:3")
    assert prove(obligation, candidate).status == ProofStatus.CONFLICT
    assert prove(obligation, None).status == ProofStatus.UNRESOLVED
    assert prove(Obligation("cleanup", expected, required=False), None).status == ProofStatus.NOT_APPLICABLE
    with pytest.raises(ValueError, match="only independently recovered retail"):
        Obligation("receiver-vptr-slot", candidate)
    proofs = ProofMap("receiver-vptr-slot", {3: "this+16"})
    proofs.update({3: "this+16"})
    with pytest.raises(ProofConflict, match="instruction| at 3"):
        proofs.update({3: "this+20", 4: "extra"})
    assert proofs == {3: "this+16"}


def test_definition_flow_cannot_reuse_saved_pointers_after_stack_alias_changes():
    from types import SimpleNamespace
    from _recoil.call_contract.flow import ControlFlow, reaching_definition_uses
    for codes in (
        ["8b3500104000", "56", "31f6", "87dc", "5f", "ffd7"],  # XCHG ESP, EBX
        ["8b3500104000", "56", "31f6", "668be0", "5f", "ffd7"],  # partial SP replacement
        ["8b3500104000", "56", "6a00", "8f03", "31f6", "5f", "ffd7"],  # POP through unknown alias
        ["8b3500104000", "897424fd", "6a00", "31f6", "8b7c2401", "ffd7"],  # overlapping PUSH
    ):
        rows = [SimpleNamespace(bytes=bytes.fromhex(code)) for code in codes]
        cfg = ControlFlow.from_edges(len(rows), {i: [i + 1] for i in range(len(rows) - 1)})
        assert reaching_definition_uses(rows, cfg, definition_index=0, register="esi") == ()


def test_definition_flow_rejects_unresolved_predecessors_and_conflicting_seeds():
    from types import SimpleNamespace
    from _recoil.call_contract.flow import ControlFlow, FlowProofError, reaching_definition_uses
    rows = [SimpleNamespace(bytes=bytes.fromhex(code)) for code in ("90", "8b3500104000", "ffd6")]
    cfg = ControlFlow.from_edges(3, {0: [1], 1: [2]}, frozenset({0}))
    with pytest.raises(FlowProofError, match="unresolved"):
        reaching_definition_uses(rows, cfg, definition_index=1, register="esi")
    cfg = ControlFlow.from_edges(3, {0: [1], 1: [2]})
    with pytest.raises(FlowProofError, match="conflicting"):
        reaching_definition_uses(rows, cfg, definition_index=1, register="esi", equivalent_definitions={1: "edi"})


def test_machine_argument_proof_rejects_wrong_operands_and_implicit_stack_changes():
    from types import SimpleNamespace
    from _recoil.lib.call_argument_bits import ArgumentBits, ArgumentProofError
    for specs in (
        [("6650", "push ax"), ("e800000000", "call 0")],
        [("8b4308", "mov eax, dword ptr [ebx+4]"), ("50", "push eax"), ("e800000000", "call 0")],
        [("b805000000", "mov eax"), ("50", "push eax"), ("e800000000", "call 0")],
        [("b805000000", "mov eax, 5"), ("50", "push eax"), ("9c", "pushfd"), ("e800000000", "call 0")],
    ):
        rows = [SimpleNamespace(bytes=bytes.fromhex(code), text=text) for code, text in specs]
        with pytest.raises(ArgumentProofError):
            proof = ArgumentBits.from_machine(rows, {i: [i + 1] for i in range(len(rows) - 1)})
            proof.stack_argument(len(rows) - 1, 0)

import pytest


@pytest.mark.parametrize("element", ["H", "URecord@@"])
def test_value_vector_noop_provider_requires_complete_coff_and_supplier_proof(
    element, monkeypatch, tmp_path,
):
    from copy import deepcopy
    from dataclasses import replace
    from types import SimpleNamespace as Row
    from _recoil.call_contract import candidate as _contract_candidate
    from _recoil.call_contract import catalog as _contract_catalog
    from _recoil.call_contract import providers as _contract_providers
    from _recoil.call_contract import receiver_equivalence as _contract_receiver_equivalence
    from _recoil.call_contract import records as _contract_records
    from _recoil.call_contract import targets as _contract_targets
    from _recoil.commands.asm_verify import Instruction

    name = f"?_Destroy@?$vector@{element}V?$allocator@{element}@std@@@std@@IAEXPA{element}0@Z"
    env = tmp_path / "compiler" / "env.cmd"
    monkeypatch.setattr(_contract_providers, "compiler_env_path", lambda *_: env)
    symbol = Row(index=3, name=name, section_number=2, storage_class=2,
        symbol_type=0x20, section_name=".text", section_size=16, value=0,
        natural_end=16, section_characteristics=0x1020)
    relocation = Row(offset=1, type=0x14, symbol_name=name, symbol_index=3)
    caller = _contract_records.CandidateCallerDefinition(symbol="?Caller@@YAXXZ",
        data=b"\xe8\0\0\0\0\xc3", relocations=(relocation,),
        relocation_mask=(False, True, True, True, True, False),
        undefined_external_functions=(), defined_external_functions=(name,),
        coff_symbols=(symbol,), section_index=1, section_start=0, section_end=6)
    helper = _contract_records.CandidateTuLocalFunctionDefinition(symbol=name,
        data=b"\xc2\x08\0" + b"\x90" * 13, relocations=(),
        relocation_mask=(False,) * 16, section_size=16,
        section_external_functions=(name,), section_is_comdat=True,
        comdat_selection=2,
        source_provenance=str(env.parent / "VC" / "INCLUDE" / "VECTOR"))
    instruction = Instruction(text=f"call {name}", raw_text=f"call {name}",
        bytes=("e8", "00", "00", "00", "00"),
        source_line=f"00000: e8 00 00 00 00 call {name}")
    candidate = _contract_records.CandidateAssembly(instructions=(instruction,),
        local_control_flow_indices=frozenset(), caller_definition=caller,
        tu_local_function_definitions={name: helper})
    supplier = Row(address="0x1000", identity="provider:unit.destroy",
        stack_cleanup_bytes=8, catalog_symbol="?Destroy@Vector@@IAEXPAPAX0@Z")
    indexes = _contract_records.IdentityIndexes(by_address={supplier.address: supplier.identity},
        by_candidate_name={}, provider_ids=frozenset({supplier.identity}),
        storage_by_address={}, storage_by_name={},
        pointer_vector_destroy_providers=(supplier,))
    def prove(value=candidate, identities=indexes):
        return _contract_providers._candidate_local_coff_callable_bridges(value, indexes=identities,
            bridge_names={}, compiler_generated_bridges={})
    assert _contract_targets._pointer_vector_destroy_provider_identity(name, indexes=indexes,
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
    from _recoil.call_contract.cfg import _candidate_exact_coff_switch_targets, _compose_candidate_coff_switch_maps
    from _recoil.call_contract.receiver_candidate import _candidate_exact_static_callback_register
    from _recoil.call_contract.records import CandidateAssembly
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
    from _recoil.call_contract.cfg import _candidate_exact_coff_switch_targets, _compose_candidate_coff_switch_maps
    from _recoil.call_contract.iat import _candidate_iat_load_reached_transfer_offsets
    from _recoil.call_contract.records import CandidateAssembly

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
    from _recoil.call_contract.dispatch import _registered_non_authored_provider_names
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
    from _recoil.call_contract.providers import _canonical_header_provider_catalog_names
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
    from _recoil.call_contract.providers import _require_catalogued_provider_collisions

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
    from _recoil.call_contract import candidate as _contract_candidate
    from _recoil.call_contract import catalog as _contract_catalog
    from _recoil.call_contract import providers as _contract_providers
    from _recoil.call_contract import receiver_equivalence as _contract_receiver_equivalence
    from _recoil.call_contract import records as _contract_records
    from _recoil.call_contract import targets as _contract_targets
    from _recoil.commands import provider_function_mutation as providers

    name = "?Copy@PointerVector@@IAEXPAPAX0@Z"
    primary = "?Copy@IntegerVector@@IAEXPAH0@Z"
    env = tmp_path / "compiler" / "env.cmd"
    monkeypatch.setattr(_contract_providers, "compiler_env_path", lambda *_: env)
    proof_calls = []
    def live_proof(**kwargs):
        proof_calls.append(kwargs)
        return Row(relocations=(), masked_byte_count=0, comdat_selection=2, body_size=4)
    monkeypatch.setattr(providers, "_provider_header_comdat_proof", live_proof)
    helper = _contract_records.CandidateTuLocalFunctionDefinition(symbol=name,
        data=bytes.fromhex("c2 08 00 90"), relocations=(), relocation_mask=(False,) * 4,
        section_size=4, section_external_functions=(name,), section_is_comdat=True,
        comdat_selection=2, source_provenance=str(env.parent / "VC/INCLUDE/vector"))
    symbol = Row(index=3, name=name, section_number=2, storage_class=2,
        value=0, weak_external_tag_index=None)
    caller = _contract_records.CandidateCallerDefinition(symbol="_caller", data=b"\xc3",
        relocations=(), relocation_mask=(False,), undefined_external_functions=(),
        defined_external_functions=(name,), coff_symbols=(symbol,),
        section_index=1, section_start=0, section_end=1)
    candidate = _contract_records.CandidateAssembly(instructions=(), local_control_flow_indices=frozenset(),
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
    identities = _contract_records.IdentityIndexes(by_address={"0x1000": "provider:unit"},
        by_candidate_name={}, provider_ids=frozenset({"provider:unit"}),
        storage_by_address={}, storage_by_name={})
    bridge = Row(hexdump=lambda *_: "00001000  c2 08 00 90")
    def match(value=candidate, rows=records, indexes=identities):
        return _contract_providers._candidate_only_provider_comdat_catalog_matches(name, value,
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


def test_native_header_provider_relocations_use_independent_probe_and_retail_targets(monkeypatch, tmp_path):
    from dataclasses import replace
    from types import SimpleNamespace as Row
    from _recoil.call_contract import providers as contract
    from _recoil.call_contract.records import CandidateAssembly, CandidateTuLocalFunctionDefinition, IdentityIndexes
    from _recoil.commands import provider_function_mutation as registration

    name = "?Destroy@ValueVector@@IAEXPAX0@Z"
    env = tmp_path / "compiler" / "env.cmd"
    monkeypatch.setattr(contract, "compiler_env_path", lambda *_: env)
    expected = {"offset": 1, "type_value": 20, "width": 4,
                "target_symbol": "_element_destroy", "addend": 0}
    proof = Row(relocations=(expected,), masked_byte_count=4, comdat_selection=2, body_size=8)
    monkeypatch.setattr(registration, "_provider_header_comdat_proof", lambda **_: proof)
    helper = CandidateTuLocalFunctionDefinition(symbol=name, data=bytes.fromhex("e8 00000000 c20800"),
        relocations=(Row(offset=1, type=20, symbol_name="_element_destroy"),),
        relocation_mask=(False, True, True, True, True, False, False, False),
        section_size=8, section_external_functions=(name,), section_is_comdat=True,
        comdat_selection=2, source_provenance=str(env.parent / "VC/INCLUDE/vector"))
    candidate = CandidateAssembly(instructions=(), local_control_flow_indices=frozenset())
    provider = {"object_symbol": name, "provider_object_identity": {
        "schema": "recoil-provider-function-object-v2", "proof_mode": "canonical-header-comdat",
        "object_symbol": name, "canonical_header": "VC/INCLUDE/vector", "semantic_provider": "test",
        "probe_recipe": "test", "body_size": 8, "comdat_selection": 2,
        "retail_icf": {"logical_symbols": [name]}}}
    indexes = IdentityIndexes(by_address={"0x3000": "symbol:destructor", "0x4000": "symbol:other"},
        by_candidate_name={"_element_destroy": "symbol:destructor", "_other": "symbol:other"},
        provider_ids=frozenset(), storage_by_address={}, storage_by_name={})
    retail = bytes.fromhex("e8 fb1f0000 c20800")
    def match(value=helper, body=retail, identities=indexes):
        return contract._candidate_canonical_header_provider_matches(value, candidate, provider, body,
            indexes=identities, retail_address="0x1000")
    assert match()
    for changes in (
        {"data": bytes.fromhex("e8 01000000 c20800")},
        {"data": bytes.fromhex("e8 00000000 c20400")},
        {"relocations": ()},
        {"relocations": (Row(offset=2, type=20, symbol_name="_element_destroy"),)},
        {"relocations": (Row(offset=1, type=6, symbol_name="_element_destroy"),)},
        {"relocations": (Row(offset=1, type=20, symbol_name="_other"),)},
        {"relocation_mask": (False,) * 8},
    ):
        assert not match(replace(helper, **changes))
    assert not match(body=bytes.fromhex("e8 fb2f0000 c20800"))
    assert not match(identities=replace(indexes, by_candidate_name={}))
    # Even a candidate and retail operand agreeing with each other cannot
    # replace the canonical probe's independently bound destructor dependency.
    assert not match(replace(helper, relocations=(Row(offset=1, type=20, symbol_name="_other"),)),
                     body=bytes.fromhex("e8 fb2f0000 c20800"))
    for changed in ({**expected, "addend": 1}, {**expected, "target_symbol": "_other"},
                    {**expected, "offset": 2}, {**expected, "type_value": 6}):
        proof.relocations = (changed,)
        assert not match()


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
    folded = {**function, "authored_order_role": "compiler-generated-icf-representative"}
    folded_row, _, _, folded_owner = validate(f=folded)
    assert folded_row == folded and folded_owner == owner
    for changes in ({"size": 17}, {"kind": "function"}, {"pipeline_class": "authored"},
                    {"object_symbol": "_other"}, {"end_exclusive": "0x1020"},
                    {"authored_order_role": "authored-body"},
                    {"authored_order_role": "authored-lifecycle-body"},
                    {"authored_order_role": "unresolved"}, {"authored_order_role": None}):
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


def test_native_header_registration_preserves_existing_provider_census():
    from copy import deepcopy
    from types import SimpleNamespace as Row
    from _recoil.commands import provider_function_mutation as providers
    request = {"owner_id": "provider", "owner_name": "Provider", "proof_mode": "canonical-header-comdat"}
    function = {"binary": "recoil", "kind": "function", "address": "0x1000",
        "end_exclusive": "0x1010", "size": 16, "extent_state": "known",
        "pipeline_class": "non-authored", "authored_order_role": "non-authored",
        "disposition": "unresolved", "ownership_state": "primary-owned",
        "output_section_id": "recoil:section:.text"}
    owner = {"kind": "provider-boundary", "name": "Provider", "source_paths": [],
        "provider_state": "accepted", "lifecycle_state": "accepted",
        "gates": {"boundary": "accepted", "source": "accepted", "owner_linkage": "none", "byte": "deferred"},
        "relationships": [{"kind": "primary-function", "symbol_id": "unit", "address": "0x1000"},
                          {"kind": "primary-function", "symbol_id": "sibling", "address": "0x1010"}],
        "evidence_ids": ["prior"]}
    def validate(o=owner, f=function, extra=None):
        document = Row(collection=lambda key: {"unit": f} if key == "symbols" else {"provider": o, **(extra or {})})
        preserved = providers._existing_header_inventory_owner(document, function_id="unit", address="0x1000", request=request)
        providers._validate_existing_function(document, function_id="unit", address="0x1000", existing_provider_owner_id="provider")
        return preserved
    preserved = validate()
    assert preserved == owner and preserved is not owner
    preserved["evidence_ids"].append("new")
    assert owner["evidence_ids"] == ["prior"]
    located = {**owner, "source_paths": ["provider:vc5-stl"]}
    assert validate(o=located) == located
    for paths in (["provider:vc5-stl", "src/unit.cpp"], ["provider:../unit.cpp"], [None], None):
        with pytest.raises(providers.ProviderFunctionMutationError):
            validate(o={**owner, "source_paths": paths})
    for changes in ({"kind": "class"}, {"name": "Other"}, {"source_paths": ["src/unit.cpp"]},
                    {"provider_state": "unresolved"}, {"lifecycle_state": "unresolved"},
                    {"gates": {"boundary": "accepted", "source": "none"}}, {"relationships": []}):
        with pytest.raises(providers.ProviderFunctionMutationError):
            validate(o={**owner, **changes})
    for field in ("symbol_id", "address"):
        bad = deepcopy(owner)
        bad["relationships"][0][field] = "foreign"
        with pytest.raises(providers.ProviderFunctionMutationError):
            validate(o=bad)
    with pytest.raises(providers.ProviderFunctionMutationError):
        validate(extra={"second-owner": owner})
    with pytest.raises(providers.ProviderFunctionMutationError):
        validate(f={**function, "pipeline_class": "authored"})


def test_spilled_loop_index_is_not_an_unknown_stack_receiver():
    from _recoil.call_contract.receiver_candidate import _is_bounded_stack_vptr
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
    from _recoil.call_contract.candidate import _candidate_comdat_source_provenance
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
    from _recoil.call_contract.candidate import _candidate_unit_header_source_files
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
    from _recoil.call_contract.comparison import compare_call_contracts
    from _recoil.call_contract.receiver_equivalence import _canonical_proven_member_storage
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
        ("load(load(address(this+0x4c)))", "load(load(this+0x4c))"),
        ("load(address(call-result(symbol:recoil:function:0x401000)+0x64))",
         "load(call-result(symbol:recoil:function:0x401000)+0x64)"),
        ("load(address(call-result(symbol:recoil:function:0x401000)+0x60)+0x4)",
         "load(call-result(symbol:recoil:function:0x401000)+0x64)"),
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
                  "load(address(load(this+0x18)+dynamic))",
                  "load(address(call-result(unknown)+0x64))",
                  "load(address(entry-register(ecx)+0x64))",
                  "load(address(this+0x7fffffff)+0x1)"):
        assert _canonical_proven_member_storage(value) == value
    returned = [{**base, "storage_identity": pairs[-1][0]}]
    for value in ("load(call-result(symbol:recoil:function:0x401010)+0x64)",
                  "load(load(call-result(symbol:recoil:function:0x401000)+0x64))",
                  "load(call-result(symbol:recoil:function:0x401000)+0x68)"):
        assert not compare_call_contracts(returned, [{**base, "storage_identity": value}])["passed"]
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
    from _recoil.call_contract.dispatch import _select_registered_symbol_regex_authority

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
    from _recoil.call_contract.callable_identity import _known_authored_direct_target_divergence
    from _recoil.call_contract.records import IdentityIndexes
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
    from _recoil.call_contract.providers import _prove_same_ordinal_compiler_provider_calls

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
    from _recoil.call_contract.receiver_candidate import _candidate_coff_direct_call_identities
    from _recoil.call_contract.receiver_retail import _exact_retail_cfg_register_provenance
    from _recoil.call_contract.records import IdentityIndexes

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

    def prove(authority=indexes, compiler_bridges=None):
        identities = _candidate_coff_direct_call_identities(
            instructions, addresses=addresses, caller_start=0x1000,
            definition=definition, indexes=authority,
            compiler_generated_bridges=compiler_bridges,
        )
        return _exact_retail_cfg_register_provenance(
            instructions, before_index=9, register="ecx", addresses=addresses,
            indexes=authority, caller_start=0x1000, caller_end=0x1000 + offset,
            local_control_flow_indices=frozenset(), local_control_flow_targets={},
            allow_exact_affine_receiver_roots=True, source="cod",
            direct_call_identities=identities,
        )

    expected = "load(load(runtime-object-join(call-result(symbol:factory)+0x8,this+0x4c)))"
    assert prove() == expected
    assert prove(replace(indexes, by_candidate_name={})) == ""
    external = replace(indexes, by_candidate_name={}, provider_ids=frozenset({identity}))
    assert prove(external, {name: identity}) == expected
    for field, value in (("type", 6), ("symbol_index", 2), ("symbol_name", "?Wrong@@YAXXZ")):
        prior = getattr(definition.relocations[0], field)
        setattr(definition.relocations[0], field, value)
        assert prove() == ""
        assert prove(external, {name: identity}) == ""
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
    from _recoil.call_contract.receiver_retail import _exact_retail_cfg_register_provenance
    from _recoil.call_contract.records import IdentityIndexes

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
        "load(load(runtime-object-join(call-result(symbol:factory)+0x8,this+0x4c)))")


@pytest.mark.parametrize("source", ["bn", "cod"])
@pytest.mark.parametrize("mode", ["test", "cmp", "jne", "partial", "wrong-register", "bypass", "zero-edge"])
def test_cfg_nullable_receiver_requires_its_own_whole_register_nonzero_guard(source, mode):
    from _recoil.commands.asm_verify import Instruction
    from _recoil.call_contract.receiver_retail import _exact_retail_cfg_register_provenance
    from _recoil.call_contract.records import IdentityIndexes
    guard = {"cmp": ("83 ff 00", "cmp edi, 0"), "partial": ("66 85 ff", "test di, di"),
             "wrong-register": ("85 c0", "test eax, eax")}.get(mode, ("85 ff", "test edi, edi"))
    rows = [("8b f1", "mov esi, ecx"), ("33 ff", "xor edi, edi"),
            ("85 d2", "test edx, edx"), ("74 00", ""),
            ("8b be 10 01 00 00", "mov edi, [esi+0x110]"), guard, ("74 00", "")]
    if mode in {"jne", "zero-edge"}:
        rows.append(("c3", "ret"))
    load_index = len(rows)
    rows.extend([("8b 07", "mov eax, [edi]"), ("8b cf", "mov ecx, edi"),
                 ("ff 50 60", "call [eax+0x60]"), ("c3", "ret")])
    offsets, cursor = [], 0
    for raw, _ in rows:
        offsets.append(cursor)
        cursor += len(raw.split())
    branches = {3: ("je", 6 if mode == "bypass" else 5),
                6: ("jne" if mode == "jne" else "je",
                    load_index if mode in {"jne", "zero-edge"} else len(rows) - 1)}
    for index, (mnemonic, target) in branches.items():
        raw = bytes((0x75 if mnemonic == "jne" else 0x74, offsets[target] - offsets[index] - 2)).hex(" ")
        operand = hex(0x1000 + offsets[target]) if source == "bn" else f"SHORT $L{target}"
        rows[index] = raw, f"{mnemonic} {operand}"
    instructions = [Instruction(text=text, raw_text=text, bytes=tuple(raw.split()),
                    source_line=f"{0x1000 + offset:08x} {raw} {text}" if source == "bn"
                    else f"{offset:05x} {raw} {text}") for offset, (raw, text) in zip(offsets, rows)]
    indexes = IdentityIndexes(by_address={}, by_candidate_name={}, provider_ids=frozenset(),
                              storage_by_address={}, storage_by_name={})
    actual = _exact_retail_cfg_register_provenance(instructions, before_index=len(rows)-2, register="eax",
        addresses=[0x1000 + offset for offset in offsets], indexes=indexes, caller_start=0x1000,
        caller_end=0x1000 + cursor, local_control_flow_indices=frozenset(), local_control_flow_targets={}, source=source)
    assert actual == ("load(load(this+0x110))" if mode in {"test", "cmp", "jne"} else "")
    if source == "cod":
        from types import SimpleNamespace
        from _recoil.call_contract.receiver_candidate import _exact_candidate_cfg_vptr_proofs
        data = bytes.fromhex(" ".join(raw for raw, _ in rows))
        definition = SimpleNamespace(data=data, relocations=(), relocation_mask=(False,) * len(data))
        result = _exact_candidate_cfg_vptr_proofs(instructions,
            addresses=[0x1000 + offset for offset in offsets], caller_start=0x1000, indexes=indexes,
            definition=definition, local_control_flow_indices=frozenset(), local_control_flow_targets={})
        assert result == ({len(rows)-2: actual} if actual else {})


@pytest.mark.parametrize("source", ["bn", "cod"])
@pytest.mark.parametrize("alternative", ["null", "other-allocation", "unknown"])
def test_cfg_allocation_occurrences_preserve_nullable_vptr_storage(source, alternative):
    from types import SimpleNamespace as Row
    from _recoil.call_contract.receiver_retail import _exact_retail_cfg_register_provenance
    from _recoil.call_contract.receiver_candidate import _exact_candidate_cfg_vptr_proofs

    provider, name = "provider:recoil:function:0x4c5b76", "??2@YAPAXI@Z"
    alternative_row = {"null": ("33 f6", "xor esi, esi"),
                       "other-allocation": ("8b f7", "mov esi, edi"),
                       "unknown": ("8b f2", "mov esi, edx")}[alternative]
    specs = [("e8 00 00 00 00", "call " + name), ("8b f8", "mov edi, eax"),
             ("e8 00 00 00 00", "call " + name), ("8b f0", "mov esi, eax"),
             ("85 f6", "test esi, esi"), ("74 02", "je 0"),
             ("eb 02", "jmp 0"), alternative_row,
             ("89 74 24 04", "mov [esp+0x4], esi"),
             ("8b 4c 24 04", "mov ecx, [esp+0x4]"),
             ("8b 19", "mov ebx, [ecx]"), ("33 c9", "xor ecx, ecx"),
             ("ff 53 74", "call [ebx+0x74]"), ("c3", "ret")]
    offsets, cursor = [], 0
    for raw, _ in specs:
        offsets.append(cursor)
        cursor += len(raw.split())
    for index, target in ((5, 7), (6, 8)):
        raw, text = specs[index]
        specs[index] = raw, text.split()[0] + " " + (hex(0x1000 + offsets[target]) if source == "bn" else f"SHORT $L{target}")
    relocations = []
    for index in (0, 2):
        if source == "bn":
            raw = b"\xe8" + (0x2000 - 0x1000 - offsets[index] - 5).to_bytes(4, "little", signed=True)
            specs[index] = raw.hex(" "), "call 0x2000"
        else:
            relocations.append(Row(offset=offsets[index]+1, type=0x14, symbol_name=name, symbol_index=1))
    rows = [Instruction(text=text, raw_text=text, bytes=tuple(raw.split()),
            source_line=f"{0x1000 + offset:08x}:" if source == "bn" else f"{offset:05x}:")
            for offset, (raw, text) in zip(offsets, specs)]
    addresses = [0x1000 + offset for offset in offsets]
    indexes = IdentityIndexes(by_address={"0x2000": provider}, by_candidate_name={name: provider},
        provider_ids=frozenset({provider}), storage_by_address={}, storage_by_name={})
    expected = f"load(nullable(allocation-result({provider},1)))" if alternative == "null" else ""
    actual = _exact_retail_cfg_register_provenance(rows, before_index=12, register="ebx",
        addresses=addresses, indexes=indexes, caller_start=0x1000, caller_end=0x1000+cursor,
        local_control_flow_indices=frozenset(), local_control_flow_targets={}, source=source,
        direct_call_identities={0: provider, 2: provider} if source == "cod" else None)
    assert actual == expected
    if source == "bn":
        from _recoil.call_contract.receiver_proofs import _exact_targetless_vptr_call_proofs
        proofs = _exact_targetless_vptr_call_proofs(rows, source="bn", caller_start="0x1000",
            caller_end_exclusive=hex(0x1000+cursor), indexes=indexes)
        assert proofs.get(12, "") == expected
    if source == "cod":
        mask = bytearray(cursor)
        for row in relocations:
            mask[row.offset:row.offset+4] = b"\1" * 4
        definition = Row(data=bytes.fromhex(" ".join(raw for raw, _ in specs)),
                         relocation_mask=mask, relocations=relocations,
                         coff_symbols=[Row(index=1, name=name)])
        def prove():
            return _exact_candidate_cfg_vptr_proofs(rows, addresses=addresses, caller_start=0x1000,
                indexes=indexes, definition=definition, local_control_flow_indices=frozenset(),
                local_control_flow_targets={}, bridge_names=bridges)
        bridges = {}
        assert prove() == ({12: expected} if expected else {})
        from dataclasses import replace
        indexes = replace(indexes, by_candidate_name={})
        assert prove() == {}
        bridges = {name: Row(address="0x2000", name=name, raw_name=name, full_name=name)}
        assert prove() == ({12: expected} if expected else {})
        bridges[name] = [bridges[name], Row(address="0x3000", name=name)]
        assert prove() == {}
        bridges[name] = bridges[name][:1]
        definition.relocations[1].type = 6
        assert prove() == {}


def test_candidate_call_result_vptr_requires_current_operand_and_receiver_proofs():
    from types import SimpleNamespace as Row
    from _recoil.call_contract.receiver_candidate import _exact_candidate_cfg_vptr_proofs

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
    from _recoil.call_contract.receiver_candidate import _candidate_coff_direct_call_identities
    assert _candidate_coff_direct_call_identities(instructions, addresses=addresses,
        caller_start=0x1000, definition=definition, indexes=indexes) == {0: identity}
    assert prove() == {}


def test_cfg_receiver_proof_includes_later_arrivals_at_the_same_call():
    from _recoil.call_contract.receiver_retail import _exact_retail_cfg_register_provenance

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
    from _recoil.call_contract.receiver_retail import _exact_retail_cfg_register_provenance

    def prove(field=0x20, stride=0x2c0, initialized=True, clobber=False, contract=False,
              candidate=False, corrupt_coff=False, relocated_cursor=False, initial=0, slot=0x20, member_initial=0):
        specs = [("8b f1", "mov esi, ecx"),
                 ("33 ff" if initialized else "8b fb", "xor edi, edi" if initialized else "mov edi, ebx"),
                 (f"8b 4e {field:02x}", f"mov ecx, [esi+0x{field:x}]"),
                 ("03 cf", "add ecx, edi"), ("8b 11", "mov edx, [ecx]"),
                 (f"ff 52 {slot:02x}" if slot else "ff 12", f"call [edx+0x{slot:x}]" if slot else "call [edx]")]
        if initial:
            specs[1] = ("bf " + initial.to_bytes(4, "little").hex(" "), f"mov edi, 0x{initial:x}")
        if member_initial:
            specs[1] = (f"8b 7e {member_initial:02x}", f"mov edi, [esi+0x{member_initial:x}]")
        loop_start = 0x1000 + sum(len(raw.split()) for raw, _ in specs[:2])
        if clobber:
            specs.append(("8b fb", "mov edi, ebx"))
        specs += [("81 c7 " + stride.to_bytes(4, "little").hex(" "), f"add edi, 0x{stride:x}"),
                  ("81 ff 00 10 00 00", "cmp edi, 0x1000")]
        branch_address = 0x1000 + sum(len(raw.split()) for raw, _ in specs)
        specs += [(f"72 {(loop_start-branch_address-2)&255:02x}",
                   f"jb 0x{loop_start-0x1000 if candidate else loop_start:x}"), ("c3", "ret")]
        instructions, addresses, cursor = [], [], 0x1000
        for raw, text in specs:
            instructions.append(Instruction(text=text, raw_text=text, bytes=tuple(raw.split()),
                source_line=f"{cursor:08x}:"))
            addresses.append(cursor)
            cursor += len(raw.split())
        if candidate:
            from types import SimpleNamespace as Row
            from _recoil.call_contract.receiver_candidate import _exact_candidate_cfg_vptr_proofs
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
            from _recoil.call_contract.extraction import extract_invocation_contract
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
    assert prove(initial=0x150, stride=4) == "load(affine(load(this+0x20),index-offset(bounded-stride(bounded-counter(edi),0x4),0x150)*1))"
    assert prove(candidate=True, initial=0x150, stride=4) == {5: prove(initial=0x150, stride=4)}
    assert prove(initial=0x154, stride=4) != prove(initial=0x150, stride=4)
    assert prove(candidate=True, initial=0x150, stride=4, clobber=True) == {}
    assert prove(candidate=True, slot=0) == {5: prove()}
    assert prove(contract=True, slot=0)[0]["storage_identity"] == prove()
    dynamic = "load(affine(load(this+0x20),cursor(load(this+0x24),+0x2c0)*1))"
    assert prove(member_initial=0x24) == dynamic
    assert prove(candidate=True, member_initial=0x24) == {5: dynamic}
    assert prove(member_initial=0x28) == dynamic.replace("this+0x24", "this+0x28")
    assert prove(member_initial=0x24, clobber=True) == ""
    assert prove(candidate=True, member_initial=0x24, clobber=True) == {}
    assert prove(candidate=True, corrupt_coff=True) == {}
    assert prove(candidate=True, relocated_cursor=True) == {}
    for invalid in ({"initialized": False}, {"clobber": True}, {"stride": 0}):
        assert prove(candidate=True, **invalid) == {}
        with pytest.raises(ValueError):
            prove(contract=True, **invalid)


@pytest.mark.parametrize("seed_form", ["imul", "lea-shl"])
def test_cfg_cursor_preheader_joins_converge_before_loop_publication(seed_form):
    from types import SimpleNamespace as Row
    from _recoil.call_contract.receiver_candidate import _exact_candidate_cfg_vptr_proofs
    from _recoil.call_contract.receiver_retail import _exact_retail_cfg_register_provenance

    def prove(candidate=False, second_argument=8, unknown=False, clobber=False):
        specs = [("entry", "8b f1", "mov esi, ecx"),
            ("", "8b 7c 24 04", "mov edi, [esp+0x4]"),
            ("", "85 ff", "test edi, edi"), ("", "7d 00", "jge seed"),
            ("", "8b fb" if unknown else f"8b 7c 24 {second_argument:02x}",
                "mov edi, ebx" if unknown else f"mov edi, [esp+0x{second_argument:x}]")]
        specs += ([("seed", "69 ff c0 02 00 00", "imul edi, edi, 0x2c0")]
            if seed_form == "imul" else [
                ("seed", "8d 04 bf", "lea eax, [edi+edi*4]"),
                ("", "8d 3c 47", "lea edi, [edi+eax*2]"),
                ("", "c1 e7 06", "shl edi, 0x6")])
        specs += [
            ("loop", "8b 4e 20", "mov ecx, [esi+0x20]"),
            ("", "03 cf", "add ecx, edi"), ("", "8b 11", "mov edx, [ecx]"),
            ("call", "ff 52 20", "call [edx+0x20]")]
        if clobber:
            specs.append(("", "8b fb", "mov edi, ebx"))
        specs += [("", "81 c7 c0 02 00 00", "add edi, 0x2c0"),
            ("", "81 ff 00 10 00 00", "cmp edi, 0x1000"),
            ("", "72 00", "jb loop"), ("", "c3", "ret")]
        offsets, labels, offset = [], {}, 0
        for label, raw, _ in specs:
            offsets.append(offset)
            if label:
                labels[label] = offset
            offset += len(raw.split())
        rows, encoded = [], []
        for (_, raw, text), at in zip(specs, offsets):
            if text in {"jge seed", "jb loop"}:
                mnemonic, label = text.split()
                raw = raw[:3] + f"{(labels[label]-at-2)&255:02x}"
                text = f"{mnemonic} 0x{labels[label] + (0 if candidate else 0x1000):x}"
            encoded.append(bytes.fromhex(raw))
            rows.append(Instruction(text=text, raw_text=text, bytes=tuple(raw.split()), source_line=f"{at:05x}:"))
        addresses = [0x1000 + at for at in offsets]
        indexes = IdentityIndexes(by_address={}, by_candidate_name={}, provider_ids=frozenset(), storage_by_address={}, storage_by_name={})
        if candidate:
            return _exact_candidate_cfg_vptr_proofs(rows, addresses=addresses, caller_start=0x1000,
                indexes=indexes, definition=Row(data=b"".join(encoded), relocations=[], coff_symbols=[], relocation_mask=b"\0"*offset),
                local_control_flow_indices=frozenset(), local_control_flow_targets={})
        return _exact_retail_cfg_register_provenance(rows, before_index=offsets.index(labels["call"]), register="edx",
            addresses=addresses, indexes=indexes, caller_start=0x1000, caller_end=0x1000+offset,
            local_control_flow_indices=frozenset(), local_control_flow_targets={}, allow_exact_affine_receiver_roots=True)

    expected = "load(affine(load(this+0x20),cursor(bounded-stride(scalar-choice(load(entry-stack+0x4),load(entry-stack+0x8)),0x2c0),+0x2c0)*1))"
    assert prove() == expected
    assert prove(candidate=True) == {9 if seed_form == "imul" else 11: expected}
    assert prove(second_argument=12) != expected
    for invalid in ({"unknown": True}, {"clobber": True}):
        assert prove(**invalid) == ""
        assert prove(candidate=True, **invalid) == {}


@pytest.mark.parametrize("source", ["bn", "cod"])
def test_cfg_reflexive_compare_removes_only_its_proven_dead_copy_edge(source):
    from types import SimpleNamespace as Row
    from _recoil.call_contract.receiver_candidate import _exact_candidate_cfg_vptr_proofs
    from _recoil.call_contract.receiver_retail import _exact_retail_cfg_register_provenance

    def prove(mode="equal"):
        specs = [("", "8b f1", "mov esi, ecx"), ("", "8b 7c 24 04", "mov edi, [esp+0x4]"),
            ("", "69 ff c0 02 00 00", "imul edi, edi, 0x2c0"),
            ("", "8d 6e 20", "lea ebp, [esi+0x20]"), ("", "8b 4d 04", "mov ecx, [ebp+0x4]"),
            ("", "03 f9", "add edi, ecx")]
        if mode == "bypass":
            specs += [("", "85 d2", "test edx, edx"), ("", "74 00", "je branch")]
        specs += [("", "3b d8" if mode == "other" else "66 3b db" if mode == "partial" else "3b db",
                   "cmp ebx, eax" if mode in {"other", "drift"} else "cmp bx, bx" if mode == "partial" else "cmp ebx, ebx"),
            ("branch", "74 00", "je seed"), ("", "8b fb", "mov edi, ebx"),
            ("seed", "8b f7", "mov esi, edi"), ("loop", "8b 06", "mov eax, [esi]"),
            ("call", "ff 10", "call [eax]"), ("", "81 c6 c0 02 00 00", "add esi, 0x2c0"),
            ("", "3b f7", "cmp esi, edi"), ("", "75 00", "jne loop"), ("", "c3", "ret")]
        offsets, labels, at = [], {}, 0
        for label, raw, _text in specs:
            offsets.append(at)
            if label:
                labels[label] = at
            at += len(raw.split())
        rows, data = [], b""
        for (_label, raw, text), offset in zip(specs, offsets):
            if text.startswith(("je ", "jne ")):
                mnemonic, label = text.split()
                raw = raw[:3] + f"{(labels[label] - offset - 2) & 255:02x}"
                text = f"{mnemonic} 0x{labels[label] + (0x1000 if source == 'bn' else 0):x}"
            rows.append(Instruction(text=text, raw_text=text, bytes=tuple(raw.split()), source_line=f"{offset:05x}:"))
            data += bytes.fromhex(raw)
        index = offsets.index(labels["call"])
        addresses = [0x1000 + offset for offset in offsets]
        indexes = IdentityIndexes(by_address={}, by_candidate_name={}, provider_ids=frozenset(), storage_by_address={}, storage_by_name={})
        if source == "cod":
            return _exact_candidate_cfg_vptr_proofs(rows, addresses=addresses, caller_start=0x1000, indexes=indexes,
                definition=Row(data=data, relocations=[], coff_symbols=[], relocation_mask=b"\0" * len(data)),
                local_control_flow_indices=frozenset(), local_control_flow_targets={}).get(index, "")
        return _exact_retail_cfg_register_provenance(rows, before_index=index, register="eax", addresses=addresses,
            indexes=indexes, caller_start=0x1000, caller_end=0x1000 + len(data),
            local_control_flow_indices=frozenset(), local_control_flow_targets={}, allow_exact_affine_receiver_roots=True)

    assert prove() == "load(cursor(affine(load(this+0x24),bounded-stride(load(entry-stack+0x4),0x2c0)*1),+0x2c0))"
    for mode in ("other", "partial", "drift", "bypass"):
        assert prove(mode) == ""


def test_cfg_pop_kills_its_encoded_destination_and_unknown_stack_writes():
    from _recoil.call_contract.receiver_retail import _exact_retail_cfg_register_provenance
    indexes = IdentityIndexes(by_address={}, by_candidate_name={}, provider_ids=frozenset(), storage_by_address={}, storage_by_name={})
    def prove(raw, rendered):
        specs = [("8b f1", "mov esi, ecx"), ("6a 00", "push 0"), (raw, rendered),
                 ("8b 4e 20", "mov ecx, [esi+0x20]"), ("8b 01", "mov eax, [ecx]"), ("ff 50 20", "call [eax+0x20]"), ("c3", "ret")]
        rows, addresses, at = [], [], 0x1000
        for raw, text in specs:
            addresses.append(at)
            rows.append(Instruction(text=text, raw_text=text, bytes=tuple(raw.split()), source_line=f"{at:08x}:"))
            at += len(raw.split())
        return _exact_retail_cfg_register_provenance(rows, before_index=5, register="eax", addresses=addresses,
            indexes=indexes, caller_start=0x1000, caller_end=at, local_control_flow_indices=frozenset(), local_control_flow_targets={})
    assert prove("5b", "pop ebx") == "load(load(this+0x20))"
    assert prove("5e", "pop esi") == ""
    assert prove("5e", "pop ebx") == ""
    assert prove("61", "popad") == ""


def test_current_callee_cleanup_requires_current_coff_returns_and_exact_call_operand():
    from types import SimpleNamespace as Row
    from _recoil.call_contract.current_callees import exact_callee_return_cleanup, candidate_direct_cleanup_map, compatible_current_callee_return_effect

    def helper(cleanup=0, mode="single"):
        terminal = ("c3", "ret") if cleanup == 0 else (f"c2 {cleanup:02x} 00", f"ret {cleanup}")
        specs = [terminal]
        if mode in {"joined", "conflict"}:
            specs = [("85 c0", "test eax, eax"), ("74 01", "je SHORT $L1"),
                ("c3", "ret"), terminal if mode == "conflict" else ("c3", "ret")]
        if mode == "tail":
            specs = [("e9 00 00 00 00", "jmp _external")]
        rows, at = [], 0
        for raw, text in specs:
            rows.append(Instruction(text=text, raw_text=text, bytes=tuple(raw.split()), source_line=f"{at:05x} {raw} {text}"))
            at += len(raw.split())
        return Row(symbol="@callee@8", instructions=rows,
            data=b"".join(bytes.fromhex(raw) for raw, _ in specs), relocations=[],
            section_is_comdat=True, comdat_selection=2, section_external_functions=("@callee@8",),
            section_size=at, local_control_flow_indices=frozenset(), local_control_flow_targets={})

    assert exact_callee_return_cleanup(helper()) == 0
    rendered_zero = helper()
    rendered_zero.instructions = [Instruction(text="retn", raw_text="ret 0", bytes=("c3",), source_line="00000 c3 ret 0")]
    assert exact_callee_return_cleanup(rendered_zero) == 0
    rendered_zero.instructions = [Instruction(text="retn", raw_text="ret 4", bytes=("c3",), source_line="00000 c3 ret 4")]
    assert exact_callee_return_cleanup(rendered_zero) is None
    assert exact_callee_return_cleanup(helper(4)) == 4
    assert exact_callee_return_cleanup(helper(mode="joined")) == 0
    assert compatible_current_callee_return_effect(helper(), helper(mode="joined"))
    assert not compatible_current_callee_return_effect(helper(), helper(4))
    assert not compatible_current_callee_return_effect(helper(), helper(mode="tail"))
    assert exact_callee_return_cleanup(helper(4, "conflict")) is None
    assert exact_callee_return_cleanup(helper(mode="tail")) is None
    padded = helper()
    padded.data += b"\x90" * 15
    padded.section_size = 16
    assert exact_callee_return_cleanup(padded) == 0
    padded.data = padded.data[:-1] + b"\xc3"
    assert exact_callee_return_cleanup(padded) is None
    drifted = helper(4)
    drifted.data = bytes.fromhex("c2 08 00")
    assert exact_callee_return_cleanup(drifted) is None
    relocated = helper(4)
    relocated.relocations = [Row(offset=1)]
    assert exact_callee_return_cleanup(relocated) is None

    call = Instruction(text="call @callee@8", raw_text="call @callee@8", bytes=("e8", "00", "00", "00", "00"), source_line="00000 e8 00 00 00 00 call @callee@8")
    reference = Row(offset=1, type=0x14, symbol_name="@callee@8", symbol_index=7)
    caller = Row(data=bytes.fromhex("e8 00 00 00 00"), relocations=[reference],
        coff_symbols=[Row(index=7, name="@callee@8")], relocation_mask=bytes((0, 1, 1, 1, 1)))
    definitions = {"@callee@8": helper()}
    assert candidate_direct_cleanup_map([call], caller, definitions) == {0: 0}
    reference.symbol_index = 8
    assert candidate_direct_cleanup_map([call], caller, definitions) == {}
    reference.symbol_index = 7
    reference.type = 6
    assert candidate_direct_cleanup_map([call], caller, definitions) == {}
    reference.type = 0x14
    caller.data = bytes.fromhex("e8 01 00 00 00")
    assert candidate_direct_cleanup_map([call], caller, definitions) == {}
    assert candidate_direct_cleanup_map([call], caller, {}) == {}


def test_virtual_table_cleanup_requires_exact_slot_target_and_complete_current_callee(tmp_path):
    from copy import deepcopy
    from types import SimpleNamespace as S
    from _recoil.call_contract.virtual_callees import exact_current_table_cleanup as prove
    from _recoil.call_contract.listing import parse_assembly
    table = S(symbol="_table", data=bytes(8), section_name=".rdata", section_size=8,
        section_is_comdat=True, comdat_selection=2, section_external_symbols=("_table",),
        relocation_mask=(True,) * 8, relocations=[
            S(offset=0, type=6, symbol_name="_delete", symbol_index=1),
            S(offset=4, type=6, symbol_name="_other", symbol_index=2)])
    symbols = [S(index=1, name="_delete", storage_class=2, symbol_type=0x20),
               S(index=2, name="_other", storage_class=2, symbol_type=0x20)]
    callee = S(symbol="_delete", data=bytes.fromhex("c2 04 00"), section_size=3,
        section_is_comdat=True, comdat_selection=2, section_external_functions=("_delete",),
        relocations=(), instructions=parse_assembly("00000 c2 04 00 ret 4", source="cod"),
        local_control_flow_indices=frozenset(), local_control_flow_targets={})
    arguments = dict(slot=0, symbols=symbols, definitions={"_delete": callee})
    assert prove(table, **arguments) == 4
    # A native table in one TU may select a body emitted by a freshly compiled
    # companion TU even when no direct call names that body.
    from _recoil.call_contract.current_callees import CurrentCalleeDefinitions
    discovery = CurrentCalleeDefinitions(None, frozenset(), tmp_path, tmp_path, [])
    discovery._routes = {}
    discovery._definitions = {"_delete": callee, "_conflicting": None}
    caller = S(caller_definition=S(symbol="_caller"),
        instructions=parse_assembly("00000 8b 44 24 04 mov eax, [esp+4]\n"
            "00004 ff 10 call [eax]", source="cod"), tu_local_function_definitions={})
    found = discovery.for_caller(caller)
    assert "_conflicting" not in found
    assert prove(table, **(arguments | {"definitions": found})) == 4
    local = deepcopy(callee)
    caller.tu_local_function_definitions = {"_delete": local}
    assert discovery.for_caller(caller)["_delete"] is local
    assert prove(table, **(arguments | {"slot": 4})) is None
    assert prove(table, **(arguments | {"slot": 2})) is None
    assert prove(table, **(arguments | {"symbols": symbols + [symbols[0]]})) is None
    for field, value in (("data", b"\x01" + bytes(7)), ("relocations", table.relocations[:1]),
                         ("relocation_mask", (False,) * 8), ("section_is_comdat", False),
                         ("section_external_symbols", ("_table", "_alias"))):
        altered = deepcopy(table)
        setattr(altered, field, value)
        assert prove(altered, **arguments) is None
    callee.data = bytes.fromhex("c2 08 00")
    assert prove(table, **arguments) is None


def test_retail_return_cleanup_requires_every_reachable_cfg_terminal():
    from _recoil.call_contract.virtual_callees import unanimous_cfg_return_cleanup as prove
    from _recoil.call_contract.listing import parse_assembly
    def effect(text):
        rows = parse_assembly(text, source="bn")
        addresses = [int(row.source_line.split()[0], 16) for row in rows]
        return prove(rows, addresses=addresses, source="bn", start=0x1000,
                     end=addresses[-1] + len(rows[-1].bytes))
    assert effect("00001000 c2 04 00 ret 4") == 4
    assert effect("00001000 85 c0 test eax, eax\n00001002 74 03 je 0x1007\n"
                  "00001004 c2 04 00 ret 4\n00001007 c2 04 00 ret 4") == 4
    for tail in ("c3 ret", "90 nop", "e9 00 10 00 00 jmp 0x200c"):
        assert effect("00001000 85 c0 test eax, eax\n00001002 74 03 je 0x1007\n"
                      "00001004 c2 04 00 ret 4\n00001007 " + tail) is None


def test_native_constructor_table_association_requires_one_dominating_this_stamp():
    from _recoil.call_contract.virtual_callees import _retail_constructor_table
    from _recoil.call_contract.listing import parse_assembly
    indexes = IdentityIndexes(by_address={}, by_candidate_name={}, provider_ids=frozenset(),
        storage_by_address={}, storage_by_name={})
    def prove(text):
        rows = parse_assembly(text, source="bn")
        addresses = [int(row.source_line.split()[0], 16) for row in rows]
        return _retail_constructor_table(rows, addresses, start=0x1000,
            end=addresses[-1]+len(rows[-1].bytes), indexes=indexes)
    good = ("00001000 8b f1 mov esi, ecx\n00001002 c7 06 00 20 00 00 mov dword [esi], 0x2000\n"
            "00001008 43 inc ebx\n00001009 85 d2 test edx, edx\n"
            "0000100b 75 fb jne 0x1008\n0000100d c3 ret")
    assert prove(good) == 0x2000
    assert prove(good.replace("8b f1 mov esi, ecx", "8b f0 mov esi, eax")) is None
    assert prove("00001000 8b f1 mov esi, ecx\n00001002 85 d2 test edx, edx\n"
        "00001004 74 06 je 0x100c\n00001006 c7 06 00 20 00 00 mov dword [esi], 0x2000\n"
        "0000100c c3 ret") is None
    assert prove("00001000 8b f1 mov esi, ecx\n00001002 c7 06 00 20 00 00 mov dword [esi], 0x2000\n"
        "00001008 c7 06 00 30 00 00 mov dword [esi], 0x3000\n0000100e c3 ret") is None
    assert prove(good.replace("75 fb jne 0x1008", "ff e0 jmp eax")) is None


def test_current_callee_definition_compares_relocation_meaning_across_objects():
    from types import SimpleNamespace as R
    from _recoil.call_contract.current_callees import same_current_callee_definition
    def body(index=3, target="callee", offset=1, kind=20, data=b"\xe8\0\0\0\0\xc3"):
        return R(symbol="caller", data=data,
            relocations=(R(symbol_index=index, symbol_name=target, offset=offset, type=kind),))
    assert same_current_callee_definition(body(), body(index=27))
    for changed in (body(target="other"), body(offset=2), body(kind=6), body(data=b"\xe8\1\0\0\0\xc3"), None):
        assert not same_current_callee_definition(body(), changed)


def test_current_callee_routes_use_accepted_targets_and_live_source_dependencies(monkeypatch, tmp_path):
    from types import SimpleNamespace as Row
    from _recoil.call_contract import current_callees
    function = Row(symbol="@callee@8", address="0x401000")
    target = Row(name="active", target_binary="recoil", source_from="src/active.cpp",
                 functions=(function,), translation_unit_function_order=())
    registered = {"active": {"kind": "vc5", "registration": {"manifest_path": "active.json"}},
                  "retired": {"kind": "vc5", "registration": {"manifest_path": "absent-retired.json"}}}
    collections = {"physical_blocks": {"block": {"accepted_order_facts": {"target_id": "active"}}},
                   "verification_targets": registered}
    document = Row(collection=lambda name: collections[name])
    loaded = []
    def load(path):
        loaded.append(path.name)
        assert path.name == "active.json"
        return target
    monkeypatch.setattr(current_callees, "load_manifest", load)
    inside = current_callees.CurrentCalleeDefinitions(document, frozenset({"src/active.cpp"}), tmp_path, tmp_path, [])
    inside._index_routes()
    assert inside._routes == {"@callee@8": target}
    assert loaded == ["active.json"]
    outside = current_callees.CurrentCalleeDefinitions(document, frozenset({"src/other.cpp"}), tmp_path, tmp_path, [])
    outside._index_routes()
    assert outside._routes == {}
    target.functions = ()
    target.translation_unit_function_order = (Row(source_from="src/active.cpp", functions=(function,)),)
    inside._index_routes()
    assert inside._routes == {"@callee@8": target}
    target.translation_unit_function_order += (Row(source_from="src/other.cpp", functions=(function,)),)
    ambiguous = current_callees.CurrentCalleeDefinitions(document,
        frozenset({"src/active.cpp", "src/other.cpp"}), tmp_path, tmp_path, [])
    ambiguous._index_routes()
    assert ambiguous._routes == {}


def test_cfg_stack_object_and_inline_array_keep_argument_member_and_stride():
    from dataclasses import replace
    from types import SimpleNamespace as Row
    from _recoil.call_contract.receiver_candidate import _exact_candidate_cfg_vptr_proofs
    from _recoil.call_contract.receiver_retail import _exact_retail_cfg_register_provenance

    indexes = IdentityIndexes(by_address={}, by_candidate_name={}, provider_ids=frozenset(),
        storage_by_address={}, storage_by_name={})

    def prove(argument=4, member=0x20, inline=False, corrupt=False, clobber=False, candidate=False, symbolic=False, prefix=()):
        if inline:
            specs = [("8b 41 20", "mov eax, [ecx+0x20]"),
                ("8d 14 c0", "lea edx, [eax+eax*8]"),
                ("8d 14 50", "lea edx, [eax+edx*2]"),
                ("8d 74 91 70", "lea esi, [ecx+edx*4+0x70]")]
        else:
            specs = [("56", "push esi"),
                (f"8b 74 24 {argument+4:02x}", f"mov esi, [esp+0x{argument+4:x}]"),
                (f"8b 46 {member:02x}", f"mov eax, [esi+0x{member:x}]"),
                ("69 c0 c0 02 00 00", "imul eax, eax, 0x2c0"),
                ("8b 76 40", "mov esi, [esi+0x40]"),
                ("03 f0", "add esi, eax")]
        if clobber:
            specs.append(("66 8b f3", "mov si, bx"))
        if symbolic and not inline:
            specs[1] = (specs[1][0], "mov esi, dword _this$[esp+80]")
        if prefix and not inline:
            slot = argument + 4 + 4 * len(prefix)
            specs[1] = (f"8b 74 24 {slot:02x}", f"mov esi, [esp+0x{slot:x}]")
            specs = list(prefix) + specs
        specs += [("8b 0e", "mov ecx, [esi]"), ("ff 51 20", "call [ecx+0x20]"), ("c3", "ret")]
        instructions, addresses, cursor = [], [], 0x1000
        for raw, text in specs:
            instructions.append(Instruction(text=text, raw_text=text, bytes=tuple(raw.split()), source_line=f"{cursor:08x}:"))
            addresses.append(cursor)
            cursor += len(raw.split())
        if corrupt:
            instructions[2] = replace(instructions[2], bytes=("8d", "14", "90") if inline else ("8b", "46", "24"))
        if candidate:
            data = bytes.fromhex(" ".join(raw for raw, _ in specs))
            return _exact_candidate_cfg_vptr_proofs(instructions, addresses=addresses,
                caller_start=0x1000, indexes=indexes,
                definition=Row(data=data, relocations=[], relocation_mask=b"\0"*len(data), coff_symbols=[]),
                local_control_flow_indices=frozenset(), local_control_flow_targets={})
        return _exact_retail_cfg_register_provenance(instructions, before_index=len(instructions)-2,
            register="ecx", addresses=addresses, indexes=indexes, caller_start=0x1000, caller_end=cursor,
            local_control_flow_indices=frozenset(), local_control_flow_targets={}, allow_exact_affine_receiver_roots=True)

    expected = "load(affine(load(load(entry-stack+0x4)+0x40),bounded-stride(load(load(entry-stack+0x4)+0x20),0x2c0)*1))"
    assert prove() == expected and prove(candidate=True) == {7: expected}
    assert prove(candidate=True, symbolic=True) == {7: expected}
    assert prove(symbolic=True) == ""
    prefix = (("6a ff", "push -1"), ("68 00 00 00 00", "push $L123"))
    assert prove(candidate=True, prefix=prefix) == {9: expected}
    assert prove(prefix=prefix) == ""
    assert prove(prefix=(("6a ff", "push -1"),)) == expected
    assert prove(prefix=(("68 ff ff ff ff", "push -1"),)) == expected
    for invalid_push in (("6a ff", "push -2"), ("6a ff", "push --1"),
                         ("66 6a ff", "push -1"), ("60", "pushad")):
        assert prove(prefix=(invalid_push,)) == ""
        assert prove(candidate=True, prefix=(invalid_push,)) == {}
    assert prove(argument=8) == expected.replace("entry-stack+0x4", "entry-stack+0x8")
    assert prove(member=0x24) == expected.replace("+0x20)", "+0x24)")
    inline_expected = "load(affine(this,bounded-stride(load(this+0x20),0x4c)*1,+0x70))"
    assert prove(inline=True) == inline_expected
    assert prove(inline=True, candidate=True) == {5: inline_expected}
    for invalid in ({"argument": 0}, {"argument": 5}, {"clobber": True}, {"corrupt": True}, {"inline": True, "corrupt": True}):
        assert prove(**invalid) == ""
        assert prove(candidate=True, **invalid) == {}


def test_cfg_indexed_receiver_preserves_index_source_and_rejects_unknowns():
    from _recoil.call_contract.receiver_retail import _exact_retail_cfg_register_provenance

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
    from _recoil.call_contract import candidate as _contract_candidate
    from _recoil.call_contract import catalog as _contract_catalog
    from _recoil.call_contract import providers as _contract_providers
    from _recoil.call_contract import receiver_equivalence as _contract_receiver_equivalence
    from _recoil.call_contract import records as _contract_records
    from _recoil.call_contract import targets as _contract_targets

    for field, value in (
        ("HUD_NET_GAME_SETUP_NUMERIC_CONSTRUCTOR_SOURCE_PATH", "base.cpp"),
        ("HUD_NET_GAME_SETUP_NUMERIC_CONSTRUCTOR_SYMBOL", "number"),
        ("HUD_NET_GAME_SETUP_CYCLE_SELECTOR_CONSTRUCTOR_SYMBOL", "cycle"),
        ("HUD_CONFIRM_QUIT_BASE_CONSTRUCTOR_SYMBOL", "widget"),
    ):
        monkeypatch.setattr(_contract_catalog, field, value)
    definitions = {name: Row(symbol=name) for name in ("number", "cycle", "widget")}
    requested = []

    def extract(cod, obj, names):
        requested.append(names)
        return definitions.copy()

    monkeypatch.setattr(_contract_candidate, "_candidate_selected_constructor_definitions", extract)
    target = Row()
    unit = (Row(source_from="base.cpp"), Path("base.cod"), Row())
    unrelated = (Row(source_from="other.cpp"), Path("other.cod"), Row())

    def acquire(units):
        return _contract_candidate._compile_hud_numeric_constructor_authority(
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
from _recoil.call_contract.comparison import _normalize_call_contract_row
from _recoil.call_contract.receiver_proofs import _exact_targetless_vptr_call_proofs
from _recoil.call_contract.records import IdentityIndexes


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
    from _recoil.call_contract.comparison import compare_call_contracts

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
    from _recoil.call_contract.recoil_lifecycle import _explicit_lifecycle_target_identity

    assert _explicit_lifecycle_target_identity("symbol:unit", {"symbol:unit"}) == "symbol:unit"
    for prior, candidates in (
        (None, {"symbol:unit"}), ("", {"symbol:unit"}),
        ("symbol:other", {"symbol:unit"}),
        ("symbol:unit", {"symbol:unit", "symbol:other"}),
        ("symbol:unit", set()),
    ):
        assert _explicit_lifecycle_target_identity(prior, candidates) == ""


def test_invocation_comparison_does_not_expand_deletion_or_fold_volume_calls() -> None:
    from _recoil.call_contract.comparison import compare_call_contracts

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
    from _recoil.call_contract.receiver_equivalence import _identical_relocated_call_body
    from _recoil.call_contract.records import CandidateCallerDefinition

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
    from _recoil.call_contract import candidate as _contract_candidate
    from _recoil.call_contract import callable_identity as _contract_callable_identity
    from _recoil.call_contract import catalog as _contract_catalog
    from _recoil.call_contract import providers as _contract_providers
    from _recoil.call_contract import receiver_equivalence as _contract_receiver_equivalence
    from _recoil.call_contract import records as _contract_records
    from _recoil.call_contract import targets as _contract_targets

    body = bytes.fromhex("8b 01 ff 50 60 c3")
    class Handle:
        def __enter__(self):
            return self
        def __exit__(self, *_):
            return False
        def read(self):
            return body
    monkeypatch.setattr(_contract_receiver_equivalence, "StableReadHandle", lambda _: Handle())
    monkeypatch.setattr(_contract_receiver_equivalence, "parse_pe_headers", lambda *_a, **_k:
        Row(image_base=0x1000, size_of_image=0x1000, sections=()))
    monkeypatch.setattr(_contract_receiver_equivalence, "rva_to_offset", lambda *_: 0)
    monkeypatch.setattr(_contract_callable_identity, "_candidate_complete_instruction_offsets", lambda _: (0, 2, 5))
    instructions = tuple(Instruction(text=text, raw_text=text,
        bytes=tuple(encoded.split()), source_line=text) for text, encoded in (
            ("mov eax, dword ptr [ecx]", "8b 01"),
            ("call dword ptr [eax+0x60]", "ff 50 60"), ("ret", "c3")))
    definition = _contract_records.CandidateCallerDefinition(symbol="_caller", data=body,
        relocations=(), relocation_mask=(False,) * len(body), undefined_external_functions=())
    candidate = Row(caller_definition=definition, instructions=instructions)
    expected = {"ordinal": 0, "form": "call", "dispatch": "indirect",
        "identity_kind": "virtual-slot", "storage_identity": "load(this)",
        "target_identity": "", "slot_displacement": 0x60, "cleanup_bytes": None}
    indexes = _contract_records.IdentityIndexes(by_address={}, by_candidate_name={},
        provider_ids=frozenset(), storage_by_address={}, storage_by_name={})

    def prove(value=candidate, row=expected):
        return _contract_receiver_equivalence._exact_body_vptr_candidate_bridges(value, caller_start="0x1000",
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
    from _recoil.call_contract.storage_identity import _index_reviewed_pooled_data_names

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
    from _recoil.call_contract.storage_identity import _pooled_data_manifest_supplier
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
    from _recoil.call_contract.callable_identity import _candidate_complete_instruction_offsets
    from _recoil.call_contract.listing import parse_assembly

    first = "00000 8b 81 ec 01 00"
    second = " 00 mov eax, DWORD PTR [ecx+492]"
    ending = "00006 c3 ret"
    def offsets(lines):
        return _candidate_complete_instruction_offsets(Row(instructions=
            parse_assembly("\n".join(lines), source="cod")))
    assert offsets((first, second, ending)) == (0, 6)
    assert offsets(("00000 8b 05 00 00 00", " 00 mov eax, DWORD PTR _global", ending)) == (0, 6)
    seh_first, seh_second = "00000 64 a1 00 00 00", " 00 mov eax, DWORD PTR fs:__except_list"
    assert offsets((seh_first, seh_second, ending)) == (0, 6)
    assert offsets((seh_first, seh_second.replace("eax", "ecx"), ending))[0] is None
    assert offsets((seh_first.replace("64", "65"), seh_second, ending))[0] is None
    assert offsets((seh_first, "; gap", seh_second, ending))[0] is None
    assert offsets((first, "; intervening row", second, ending))[0] is None
    assert offsets(("00000 8b 84 ec 01 00", second, ending))[0] is None
    assert offsets((first, second, "00000 c3 ret"))[0] is None
    assert offsets((first, " 00 add eax, DWORD PTR [ecx+492]", ending))[0] is None


def test_raw_call_count_guard_precedes_helper_projections_and_checks_retail_census() -> None:
    from types import SimpleNamespace as Row
    from _recoil.call_contract.identity import _raw_call_count_divergence
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
    from _recoil.call_contract.cfg import _instruction_may_clobber_register
    from _recoil.commands.asm_verify import Instruction

    text = f"{mnemonic} ebx, 1"
    encoding = {"bts": "0f ba eb 01", "btr": "0f ba f3 01", "btc": "0f ba fb 01"}[mnemonic]
    instruction = Instruction(text=text, raw_text=text, bytes=tuple(encoding.split()), source_line=text)
    assert _instruction_may_clobber_register(instruction, "ebx")
    assert not _instruction_may_clobber_register(instruction, "esi")
    assert _instruction_may_clobber_register(
        Instruction(text=text, raw_text=text, bytes=(), source_line=text), "esi")


def test_dynamic_probe_coordinates_allow_only_uniform_translation() -> None:
    from _recoil.call_contract.identity import _translated_dynamic_probe_setup

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


def test_dynamic_probe_setup_authenticates_coff_and_local_esp_save() -> None:
    from types import SimpleNamespace as Row
    from _recoil.call_contract.callable_identity import _candidate_dynamic_probe_setup_matches
    from _recoil.commands.asm_verify import Instruction

    setup = ((0, b'\xb8\x10\0\0\0'), (10, b'\x89\x65\xe8'), (13, b'\xb8\x20\0\0\0'))
    offsets = (0, 5, 10, 13, 18, 23)
    def candidate(spill=b'\x89\x65\xe8', first=b'\xb8\x10\0\0\0'):
        bodies = (first, b'\xe8\0\0\0\0', spill, setup[2][1], b'\xe8\0\0\0\0', b'\xc3')
        data = b''.join(bodies)
        mask = tuple(6 <= i < 10 or 19 <= i < 23 for i in range(len(data)))
        instructions = tuple(Instruction(text='', raw_text='',
            bytes=tuple(f'{value:02x}' for value in body), source_line='') for body in bodies)
        return Row(instructions=instructions, caller_definition=Row(data=data,
            relocation_mask=mask, relocations=(Row(offset=6), Row(offset=19)),
            section_index=17, section_start=0, section_end=len(data)))

    for displacement in (0x80, 0xe4, 0xe8, 0xfc):
        current = candidate(bytes((0x89, 0x65, displacement)))
        current.caller_definition.section_index = 900
        assert _candidate_dynamic_probe_setup_matches(current, offsets, setup)
    for spill in (b'\x89\x65\0', b'\x89\x65\x04', b'\x89\x65\xe7',
            b'\x89\x45\xe8', b'\x8b\x65\xe8', b'\x89\x25\xe8'):
        assert not _candidate_dynamic_probe_setup_matches(candidate(spill), offsets, setup)
    assert not _candidate_dynamic_probe_setup_matches(candidate(first=b'\xb8\x11\0\0\0'), offsets, setup)
    for changes in (
        {'section_index': 0}, {'section_start': 1}, {'section_end': 25},
        {'data': b'\x90' * 24}, {'relocation_mask': (False,) * 23},
        {'relocation_mask': tuple(i == 10 for i in range(24))},
        {'relocations': (Row(offset=9),)},
    ):
        current = candidate()
        current.caller_definition.__dict__.update(changes)
        assert not _candidate_dynamic_probe_setup_matches(current, offsets, setup)
    for invalid_offsets in ((0, 5, 10, 13, 18), (0, 5, 10, 13, 18, None), (0, 5, 10, 10, 18, 23)):
        assert not _candidate_dynamic_probe_setup_matches(candidate(), invalid_offsets, setup)
    assert not _candidate_dynamic_probe_setup_matches(candidate(), offsets, ())


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
    from _recoil.call_contract.extraction import extract_invocation_contract

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


@pytest.mark.parametrize("kind,symbol", [
    ("copy-constructor", "??0ProofValue@@QAE@ABU0@@Z"),
    ("copy-assignment", "??4ProofValue@@QAEAAU0@ABU0@@Z"),
])
def test_implicit_copy_binding_keeps_authored_gates_and_requires_expanded_origin(tmp_path, kind, symbol):
    from dataclasses import replace
    from types import SimpleNamespace as Row
    from _recoil.commands.vc5_verify import VerifyFunction
    from _recoil.lib.source_emission_markers import EmissionAnchor
    from _recoil.lib.implicit_copy_members import validate_member_metadata, validate_source_binding, validate_expanded_source

    path = "tests/tools/proof_value.cpp"
    anchor = EmissionAnchor(path, "type-definition", "ProofValue")
    function = VerifyFunction("0x401000", symbol, "ProofValue copy", pipeline_class="authored",
        authored_order_role="authored-body", emission_anchor=anchor, implicit_member_kind=kind)
    artifact = Row(relation="emits", section=".text", direct=True, construct=Row(kind="type", name="ProofValue"),
                   path=path, anchor_id="recoil:anchor:proof-value", artifact_id="recoil:function:0x401000")
    trace = {"state": "resolved", "source_edges": [{"relation": "emits", "anchor_id": artifact.anchor_id,
              "emission_context": {"translation_unit": path}}]}
    binding = validate_source_binding(function=function, artifact=artifact, translation_unit=path, tracker_source_trace=trace)
    expanded = '#line 1 "probe.cpp"\nstruct ProofValue { int value; ProofValue(int); };\n'
    kwargs = dict(compilation_directory=tmp_path, compiled_source=tmp_path / "probe.cpp")
    assert validate_expanded_source(expanded, binding, **kwargs)["explicit_selected_member"] is False
    explicit = ("ProofValue(const ProofValue &);" if kind == "copy-constructor"
                else "ProofValue &operator=(const ProofValue &);")
    with pytest.raises(ValueError, match="explicitly declares"):
        validate_expanded_source(expanded.replace("int value;", explicit), binding, **kwargs)
    with pytest.raises(ValueError, match="source anchor"):
        validate_expanded_source(expanded.replace('"probe.cpp"', '"unrelated.h"'), binding, **kwargs)
    with pytest.raises(ValueError, match="compiler source-origin"):
        validate_expanded_source(expanded.split("\n", 1)[1], binding, **kwargs)
    for change in ({"required_presence": False}, {"source_order_gate": False}, {"full_order_gate": False},
                   {"pipeline_class": "non-authored"}, {"symbol_regex": ".*"}, {"symbol": symbol.replace("ProofValue", "Unrelated")},
                   {"provenance": "provider-boundary"}, {"implicit_member_kind": "arbitrary-member"}):
        with pytest.raises(ValueError):
            validate_member_metadata(replace(function, **change))
    trace["source_edges"][0]["emission_context"] = {"translation_unit": "wrong.cpp"}
    with pytest.raises(ValueError, match="class/TU emission edge"):
        validate_source_binding(function=function, artifact=artifact, translation_unit=path, tracker_source_trace=trace)


def test_implicit_copy_native_emission_rejects_wrong_tu_comdat_and_relocations():
    from types import SimpleNamespace as Row
    from _recoil.lib.implicit_copy_members import ImplicitCopyBinding, validate_native_emission
    from _recoil.lib.source_emission_markers import EmissionAnchor
    binding = ImplicitCopyBinding("copy-constructor", "??0ProofValue@@QAE@ABU0@@Z", "recoil:function:0x401000",
                                 EmissionAnchor("proof.cpp", "type-definition", "ProofValue"), "proof.cpp")
    symbol = Row(name=binding.symbol, section_number=1, storage_class=2, value=0, type=0x20,
                 section_definition_selection=None, section_definition_association=None)
    section_symbol = Row(name=".text", section_number=1, section_definition_selection=2,
                         section_definition_association=None)
    section = Row(index=1, name=".text", raw_data=b"\xe8\0\0\0\0\xc3", characteristics=0x1020)
    relocation = Row(offset=1, type=20, symbol_index=2, symbol_name="_base_copy")
    coff = Row(symbols=[symbol, section_symbol], sections=[section], symbols_by_index={2: Row(name="_base_copy")},
               relocations_by_section={1: [relocation]})
    listing = binding.symbol + " PROC NEAR\n" + binding.symbol + " ENDP\n"
    args = dict(binding=binding, coff=coff, cod_text=listing, source_from="proof.cpp")
    assert validate_native_emission(**args)["native_emission"] is True
    with pytest.raises(ValueError, match="wrong translation unit"):
        validate_native_emission(**{**args, "source_from": "other.cpp"})
    section_symbol.section_definition_selection = 3
    with pytest.raises(ValueError, match="ANY COMDAT"):
        validate_native_emission(**args)
    section_symbol.section_definition_selection = 2
    relocation.offset = 4
    with pytest.raises(ValueError, match="relocation"):
        validate_native_emission(**args)


def test_physical_contributions_preserve_count_tail_and_indirect_dispatch():
    from _recoil.call_contract.contributions import InvocationContribution, contribution_results, require_projection_accounting
    from _recoil.call_contract.proofs import ProofStatus
    expected = (InvocationContribution("retail", "0x401000", "call", "indirect"),)
    candidate = (InvocationContribution("candidate", "0x0", "tail", "direct"),)
    results = contribution_results(expected, candidate)
    assert [row.status for row in results] == [ProofStatus.PROVEN, ProofStatus.CONFLICT, ProofStatus.CONFLICT]
    with pytest.raises(ValueError, match="population"):
        require_projection_accounting(candidate, [], side="candidate")
    with pytest.raises(ValueError, match="form/dispatch"):
        require_projection_accounting(candidate, [{"form": "call", "dispatch": "indirect"}], side="candidate")


def test_keyboard_callback_resolution_preserves_physical_call_order(monkeypatch):
    from types import SimpleNamespace
    from _recoil.call_contract import recoil_input
    from _recoil.call_contract.contributions import InvocationContribution, require_projection_accounting
    sites = ("0x46f6c0", "0x46f6da", "0x46f7e8", "0x46f7f5", "0x46f870", "0x46f89d")
    dispatches = ("indirect", "indirect", "direct", "indirect", "indirect", "direct")
    rows = [{"ordinal": i, "form": "call", "dispatch": dispatch, "target_identity": site}
            for i, (site, dispatch) in enumerate(zip(sites, dispatches))]
    monkeypatch.setattr(recoil_input, "_zinput_runtime_dispatch_storage_identities",
                        lambda *args, **kwargs: {"raw": "storage:recoil:data:0x565bc4"})
    kwargs = dict(caller_start="0x46f690", indexes=None)
    resolved, resolved_sites = recoil_input._zinput_runtime_dispatch_normalize_retail_contract(rows, sites, **kwargs)
    assert resolved_sites == list(sites)
    physical = tuple(InvocationContribution("retail", site, "call", dispatch)
                     for site, dispatch in zip(sites, dispatches))
    require_projection_accounting(physical, resolved, side="retail")
    assert resolved[:3] == rows[:3] and resolved[4:] == rows[4:]
    assert resolved[3]["storage_identity"] == "storage:recoil:data:0x565bc4"
    assert rows[3]["target_identity"] == sites[3]
    with pytest.raises(ValueError, match="population"):
        recoil_input._zinput_runtime_dispatch_normalize_retail_contract(rows, sites[:-1], **kwargs)
    wrong_form = [dict(row) for row in rows]
    wrong_form[3]["dispatch"] = "direct"
    with pytest.raises(ValueError, match="exact memory call"):
        recoil_input._zinput_runtime_dispatch_normalize_retail_contract(wrong_form, sites, **kwargs)
    monkeypatch.setattr(recoil_input, "_zinput_runtime_dispatch_storage_identities",
                        lambda *args, **kwargs: {"raw": "raw-callback", "key-callback": "combo-callback"})
    monkeypatch.setattr(recoil_input, "_zinput_exact_aggregate_leaf_member_vptr_candidate_bridges",
                        lambda *args, **kwargs: kwargs)
    identities = SimpleNamespace(by_address={"0x472490": "report", "0x46fba0": "translate"}, provider_ids=set())
    proof = recoil_input._zinput_keyboard_aggregate_leaf_candidate_bridges(
        [], None, document=None, caller_identity="symbol:recoil:function:0x46f690",
        caller_start="0x46f690", caller_end_exclusive="0x46f970", indexes=identities)
    assert tuple(chain[0] for chain in proof["leaf_chains"]) == (0, 1)
    additional = dict(proof["expected_additional_rows"])
    assert dict(additional[2])["target_identity"] == "translate"
    assert dict(additional[3])["storage_identity"] == "raw-callback"
    assert dict(additional[4])["storage_identity"] == "combo-callback"
    assert dict(additional[5])["target_identity"] == "report"


def test_retail_vptr_storage_keeps_global_and_fastcall_argument_roots():
    from _recoil.call_contract.extraction import extract_invocation_contract
    from _recoil.call_contract.listing import parse_assembly
    from _recoil.call_contract.records import IdentityIndexes
    indexes = IdentityIndexes(by_address={}, by_candidate_name={}, provider_ids=frozenset(),
        storage_by_address={"0x56aaf0": "storage:recoil:data:0x56aaf0"}, storage_by_name={})
    cases = (
        ("00400000 a1 f0 aa 56 00 mov eax, dword [0x56aaf0]\n"
         "00400005 8b 08 mov ecx, dword [eax]\n00400007 ff 51 60 call dword [ecx+0x60]\n0040000a c3 ret",
         "load(storage:recoil:data:0x56aaf0)"),
        ("00400000 8b f2 mov esi, edx\n00400002 8b 46 08 mov eax, dword [esi+0x8]\n"
         "00400005 8b 08 mov ecx, dword [eax]\n00400007 ff 51 60 call dword [ecx+0x60]\n0040000a c3 ret",
         "load(load(entry-register(edx)+0x8))"),
    )
    for assembly, storage in cases:
        rows = extract_invocation_contract(parse_assembly(assembly, source="bn"), source="bn",
            caller_identity="symbol:caller", caller_start="0x400000", caller_end_exclusive="0x40000b", indexes=indexes)
        assert len(rows) == 1 and rows[0]["storage_identity"] == storage
        assert rows[0]["target_identity"] == "" and rows[0]["slot_displacement"] == 0x60
    unknown = cases[1][0].replace("8b f2 mov esi, edx", "8b f0 mov esi, eax")
    bypass = ("00400000 85 c0 test eax, eax\n00400002 74 07 je 0x40000b\n"
              "00400004 8b f2 mov esi, edx\n00400006 8b 46 08 mov eax, dword [esi+0x8]\n"
              "00400009 8b 08 mov ecx, dword [eax]\n0040000b ff 51 60 call dword [ecx+0x60]\n"
              "0040000e c3 ret")
    for assembly, end in ((unknown, "0x40000b"), (bypass, "0x40000f")):
        with pytest.raises(ValueError, match="unresolved indirect register storage"):
            extract_invocation_contract(parse_assembly(assembly, source="bn"), source="bn",
                caller_identity="symbol:caller", caller_start="0x400000", caller_end_exclusive=end, indexes=indexes)


def test_relocated_cfg_calls_keep_separate_effects_and_data_remains_opaque():
    from types import SimpleNamespace as Row
    from _recoil.call_contract.allocation_callees import relocated_cfg_writes

    def check(raw, text):
        rows = [Instruction(text=text, raw_text=text, bytes=tuple(raw.split()), source_line="00000:")]
        return relocated_cfg_writes(rows, addresses=[0x1000], start=0x1000,
            definition=Row(relocations=[Row(offset=len(raw.split())-4)]))

    assert check("e8 00 00 00 00", "call callee") == {}
    assert check("ff 15 00 00 00 00", "call dword [__imp_callee]") == {}
    assert check("b8 00 00 00 00", "mov eax, table") == {0: frozenset({"eax"})}
    assert check("c7 06 00 00 00 00", "mov dword [esi], table") == {0: frozenset()}
    assert check("81 c0 00 00 00 00", "add eax, table") is None
    assert check("e9 00 00 00 00", "jmp target") is None


def test_pair_comparison_cannot_borrow_the_other_sides_receiver_or_dispatch():
    from _recoil.call_contract.comparison import compare_call_contracts
    expected = [{"ordinal": 0, "form": "call", "dispatch": "indirect", "identity_kind": "virtual-slot",
                 "target_identity": "", "storage_identity": "load(entry-register(ecx))", "slot_displacement": 4,
                 "cleanup_bytes": None}]
    candidate = [{**expected[0], "storage_identity": "load(entry-register(edx))"}]
    result = compare_call_contracts(expected, candidate)
    assert result["passed"] is False
    assert any(row["dimension"] == "storage_identity" and row["status"] == "conflict" for row in result["proof_results"])
    candidate = [{**expected[0], "dispatch": "direct"}]
    assert compare_call_contracts(expected, candidate)["passed"] is False


@pytest.mark.parametrize("candidate", [False, True])
def test_cfg_stack_spill_after_unknown_cleanup_uses_only_its_new_origin(candidate):
    from types import SimpleNamespace as Row
    from _recoil.call_contract.receiver_retail import _exact_retail_cfg_register_provenance
    from _recoil.call_contract.receiver_candidate import _exact_candidate_cfg_vptr_proofs
    from _recoil.call_contract.receiver_proofs import _exact_targetless_vptr_call_proofs

    def prove(extra=(), displacement="14", store_before_call=False, complete_object=False, merged_origin=False):
        initial = [("8b f1", "mov esi, ecx"), ("ff d0", "call eax")]
        cleanup = {}
        if merged_origin:
            initial = [("8b f1", "mov esi, ecx"), ("85 d2", "test edx, edx"),
                ("74 02", "je 0x8" if candidate else "je 0x1008"),
                ("ff d0", "call eax"), ("ff d1", "call ecx")]
            cleanup = {4: 0}
        spill = [("8b 46 18", "mov eax, [esi+0x18]"), ("89 44 24 10", "mov [esp+0x10], eax")]
        if complete_object:
            spill[0] = ("8b 46 00", "mov eax, [esi+0]")
        specs = initial[:1]+spill+initial[1:] if store_before_call else initial+spill
        specs += list(extra)+[("6a 01", "push 1"),
            ("8b 44 24 "+displacement, "mov eax, [esp+0x"+displacement+"]"),
            ("ff 50 0c", "call [eax+0xc]"), ("c3", "ret")]
        rows, addresses, offset = [], [], 0
        for raw, text in specs:
            rows.append(Instruction(text=text, raw_text=text, bytes=tuple(raw.split()), source_line=f"{offset:05x}:"))
            addresses.append(0x1000+offset)
            offset += len(raw.split())
        indexes = IdentityIndexes(by_address={}, by_candidate_name={}, provider_ids=frozenset(), storage_by_address={}, storage_by_name={})
        if candidate:
            data = bytes.fromhex(" ".join(raw for raw, _ in specs))
            return _exact_candidate_cfg_vptr_proofs(rows, addresses=addresses, caller_start=0x1000,
                indexes=indexes, definition=Row(data=data, relocations=[], relocation_mask=b"\0"*len(data), coff_symbols=[]),
                local_control_flow_indices=frozenset(), local_control_flow_targets={},
                call_cleanup_by_instruction_index=cleanup).get(len(rows)-2, "")
        retail_rows = [Instruction(text=row.text, raw_text=row.raw_text, bytes=row.bytes,
                       source_line=f"{address:08x}:") for row, address in zip(rows, addresses)]
        return _exact_targetless_vptr_call_proofs(retail_rows, source="bn", caller_start="0x1000",
            caller_end_exclusive=hex(0x1000+offset), indexes=indexes,
            call_cleanup_by_instruction_index=cleanup).get(len(rows)-2, "")

    assert prove() == "load(this+0x18)"
    assert prove(merged_origin=True) == "load(this+0x18)"
    if candidate:
        assert prove(complete_object=True) == "load(this)"
    assert prove([("c6 44 24 18 02", "mov byte [esp+0x18], 2")]) == "load(this+0x18)"
    assert prove(displacement="18") == ""
    assert prove(store_before_call=True) == ""
    for extra in (
        [("ff d1", "call ecx")],
        [("c6 44 24 11 00", "mov byte [esp+0x11], 0")],
        [("89 5c 24 11", "mov [esp+0x11], ebx")],
        [("85 db", "test ebx, ebx"), ("74 02", "je 0x11" if candidate else "je 0x1011"), ("ff d1", "call ecx")],
    ):
        assert prove(extra) == ""


@pytest.mark.parametrize("candidate", [False, True])
def test_cfg_backward_inline_panel_cursor_retains_initial_member_and_signed_step(candidate):
    from types import SimpleNamespace as Row
    from _recoil.call_contract.receiver_retail import _exact_retail_cfg_register_provenance
    from _recoil.call_contract.receiver_candidate import _exact_candidate_cfg_vptr_proofs

    def prove(raw_step="81 ef a4 02 00 00", rendered_step="sub edi, 0x2a4", clobber=False):
        specs = [("8b f1", "mov esi, ecx"), ("8d be 58 05 00 00", "lea edi, [esi+0x558]"),
                 ("8b 07", "mov eax, [edi]"), ("ff 50 60", "call [eax+0x60]")]
        if clobber:
            specs.append(("8b fb", "mov edi, ebx"))
        specs += [(raw_step, rendered_step), ("3b fe", "cmp edi, esi")]
        offset = sum(len(raw.split()) for raw, _ in specs)
        specs += [(f"73 {(8-offset-2)&255:02x}", "jae 0x8" if candidate else "jae 0x1008"), ("c3", "ret")]
        rows, addresses, offset = [], [], 0
        for raw, text in specs:
            rows.append(Instruction(text=text, raw_text=text, bytes=tuple(raw.split()), source_line=f"{offset:05x}:"))
            addresses.append(0x1000 + offset)
            offset += len(raw.split())
        indexes = IdentityIndexes(by_address={}, by_candidate_name={}, provider_ids=frozenset(), storage_by_address={}, storage_by_name={})
        if candidate:
            data = bytes.fromhex(" ".join(raw for raw, _ in specs))
            return _exact_candidate_cfg_vptr_proofs(rows, addresses=addresses, caller_start=0x1000,
                indexes=indexes, definition=Row(data=data, relocations=[], relocation_mask=b"\0"*len(data), coff_symbols=[]),
                local_control_flow_indices=frozenset(), local_control_flow_targets={}).get(3, "")
        return _exact_retail_cfg_register_provenance(rows, before_index=3, register="eax", addresses=addresses,
            indexes=indexes, caller_start=0x1000, caller_end=0x1000+offset,
            local_control_flow_indices=frozenset(), local_control_flow_targets={}, allow_exact_affine_receiver_roots=True)

    expected = "load(cursor(this+0x558,-0x2a4))"
    assert prove() == expected
    assert prove("81 c7 a4 02 00 00", "add edi, 0x2a4") == expected.replace("-0x2a4", "+0x2a4")
    assert prove(rendered_step="sub edi, 0x2ac") == ""
    assert prove(raw_step="66 81 ef a4 02") == ""
    assert prove(clobber=True) == ""
