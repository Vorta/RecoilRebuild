"""Native callee effects for the accepted selector's concrete item array."""
from _recoil.call_contract import catalog, cfg, receiver_candidate as rc, receiver_retail as rr, retail_imports, targets
from _recoil.call_contract.receiver_stack_loops import prove_stack_cursor_loop
from _recoil.call_contract.virtual_callees import NativeVirtualEffects, _text_stack_retail_effects, exact_current_table_cleanup


def _ftol_effects(rows, *, addresses, start, candidate, document, indexes, bridge, provider_bridges):
    """The independently authenticated x87 CRT provider has no stack arguments."""
    by_address, by_name = bridge.symbols()
    identity, _ = retail_imports._ftol_import_thunk_retail_bridge(document=document,
        indexes=indexes, bridge_by_address=by_address, bridge_by_name=by_name,
        bridge_data_rows=bridge.data_variables(), bridge=bridge)
    if identity != catalog.MSVC_FTOL_PROVIDER_IDENTITY:
        return {}
    definition = candidate.caller_definition if candidate else None
    if candidate and not rc._candidate_listing_matches_coff(rows,
            addresses=addresses, caller_start=start, definition=definition):
        return {}
    result = {}
    for i, (row, address) in enumerate(zip(rows, addresses)):
        body = bytes.fromhex(' '.join(row.bytes))
        if len(body) != 5 or body[0] != 0xe8 or cfg._instruction_mnemonic(row) != 'call':
            continue
        if candidate is None:
            if address + 5 + int.from_bytes(body[1:], 'little', signed=True) == int(catalog.MSVC_FTOL_PROVIDER_ADDRESS, 16):
                result[i] = 0
            continue
        name = catalog.MSVC_FTOL_CANDIDATE_SYMBOL
        at = address-start
        relocations = [r for r in definition.relocations if at <= r.offset < at+5]
        symbols = [s for s in definition.coff_symbols if s.name == name]
        if (body == b'\xe8\0\0\0\0' and cfg._instruction_operand(row).strip() == name
                and provider_bridges.get(name) == catalog.MSVC_FTOL_IAT_STORAGE_IDENTITY
                and len(relocations) == len(symbols) == 1
                and relocations[0].offset == at+1 and relocations[0].type == 0x14
                and relocations[0].symbol_name == name and relocations[0].symbol_index == symbols[0].index
                and symbols[0].section_number == 0 and symbols[0].value == 0
                and symbols[0].storage_class == 2 and all(definition.relocation_mask[at+1:at+5])):
            result[i] = 0
    return result


def selector_cleanup(rows, *, document, indexes, caller_start, caller_end,
                     known_cleanup, bridge=None, candidate=None, definitions=None, provider_bridges=None):
    if caller_start not in {'0x4b90e0','0x4b8de0'} or bridge is None:
        return {}
    loader = caller_start == '0x4b8de0'
    for owner_id, function in (('recoil:owner:legacy.hud_ui.class_hudcmdbindbuttonbase', int(caller_start,16)),
                              ('recoil:owner:legacy.hud_ui.class_huduilistselectoritem', 0x4b92a0)):
        owner = document.collection('owners').get(owner_id, {})
        if (any(owner.get('gates', {}).get(g) != 'accepted' for g in ('boundary','source','owner_linkage'))
                or not any(r.get('kind') == 'primary-function' and r.get('symbol_id') == 'recoil:function:'+hex(function)
                           for r in owner.get('relationships', ()))):
            return {}
    selected_slots = (24,96,128) if loader else (0,12)
    specs = (('HudUiListSelectorItem', 0x4b92a0, selected_slots),)
    expected = _text_stack_retail_effects(specs, bridge=bridge, document=document, indexes=indexes)
    if not expected or any(v is None for v in expected.values()):
        return {}
    effects = {slot: expected['HudUiListSelectorItem', slot] for slot in selected_slots}
    if candidate is not None:
        symbol = '??_7HudUiListSelectorItem@@6B@'
        units = [(candidate.vftable_definitions, candidate.caller_definition.coff_symbols,
                  candidate.tu_local_function_definitions), *getattr(definitions, 'table_units', ())]
        tables = [(t[symbol], s, f) for t, s, f in units if symbol in t]
        if not tables:
            return {}
        current = {}
        for slot in effects:
            values = {exact_current_table_cleanup(t, slot=slot, symbols=s, definitions=dict(definitions or {}) | f)
                      for t, s, f in tables}
            if len(values) != 1 or None in values:
                return {}
            current[slot] = next(iter(values))
        if current != effects:
            raise ValueError('selector native virtual callee return effects differ from retail')
        effects = current
    start = int(caller_start, 16)
    source = 'cod' if candidate else 'bn'
    addresses = cfg._instruction_runtime_addresses(rows, source=source, caller_start=start)
    end = start+len(candidate.caller_definition.data) if candidate else int(caller_end,16)
    cleanups = dict(known_cleanup)
    if not loader:
        cleanups.update(_ftol_effects(rows, addresses=addresses, start=start,
            candidate=candidate, document=document, indexes=indexes, bridge=bridge, provider_bridges=provider_bridges or {}))
    slots = {i: targets._exact_indirect_register_call_slot(row, allow_zero=True)
             for i, row in enumerate(rows)}
    assumptions = {i: effects[slot[1]] for i, slot in slots.items() if slot and slot[1] in effects}
    if loader:
        if candidate:
            values = rc._exact_candidate_cfg_vptr_proofs(rows, addresses=addresses,
                caller_start=start, indexes=indexes, definition=candidate.caller_definition,
                local_control_flow_indices=candidate.local_control_flow_indices,
                local_control_flow_targets=candidate.local_control_flow_targets,
                call_cleanup_by_instruction_index=cleanups | assumptions)
        else:
            values = {i: rr._exact_retail_cfg_register_provenance(rows, before_index=i, register=slots[i][0],
                addresses=addresses, indexes=indexes, caller_start=start, caller_end=end,
                local_control_flow_indices=frozenset(), local_control_flow_targets={},
                call_cleanup_by_instruction_index=cleanups | assumptions,
                allow_exact_caller_cleanup=True, allow_exact_affine_receiver_roots=True) for i in assumptions}
        allowed = {'load(this+0x16c)'} | {
            'load(affine(load(this+0x418),bounded-stride(bounded-counter('+reg+'),0x2ac)*1))'
            for reg in ('ebx','ebp','esi','edi')}
        proved = {i: value for i, value in values.items() if i in assumptions and value in allowed}
        # Every borrowed effect must be discharged against the concrete item
        # receiver. An unproved earlier slot cannot support a later loop proof.
        if set(proved) != set(assumptions):
            return {}
        return NativeVirtualEffects(assumptions,proved)
    values = prove_stack_cursor_loop(rows, addresses=addresses, source=source, start=start, end=end,
        indexes=indexes, cleanups=cleanups | assumptions,
        definition=candidate.caller_definition if candidate else None,
        local_indices=candidate.local_control_flow_indices if candidate else frozenset(),
        local_targets=candidate.local_control_flow_targets if candidate else {})
    allowed = {'load(cursor(load(this+0x418),+0x2ac))',
        'load(cursor(affine(load(this+0x418),bounded-stride(load(this+0x168),0x2ac)*1),+0x2ac))'}
    proved = {i: value for i, value in values.items() if value in allowed and slots[i][1] == 12}
    # The remaining two item calls use the same accepted concrete type through
    # the stored array pointer and the embedded selected-item member.
    if candidate:
        ordinary = rc._exact_candidate_cfg_vptr_proofs(rows, addresses=addresses,
            caller_start=start, indexes=indexes, definition=candidate.caller_definition,
            local_control_flow_indices=candidate.local_control_flow_indices,
            local_control_flow_targets=candidate.local_control_flow_targets,
            call_cleanup_by_instruction_index=cleanups | {i: assumptions[i] for i in proved})
    else:
        ordinary = {i: rr._exact_retail_cfg_register_provenance(rows, before_index=i, register=slot[0],
            addresses=addresses, indexes=indexes, caller_start=start, caller_end=end,
            local_control_flow_indices=frozenset(), local_control_flow_targets={},
            call_cleanup_by_instruction_index=cleanups | {i: assumptions[i] for i in proved},
            allow_exact_caller_cleanup=True, allow_exact_affine_receiver_roots=True)
            for i, slot in slots.items() if slot and slot[1] in effects}
    for i, value in ordinary.items():
        if slots[i] and (slots[i][1], value) in {(0,'load(load(this+0x418))'), (12,'load(this+0x16c)')}:
            proved[i] = value
    return NativeVirtualEffects({i: effects[slots[i][1]] for i in proved}, proved)
