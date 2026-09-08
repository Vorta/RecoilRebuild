"""Associate native inline construction with returning virtual calls."""
from _recoil.call_contract import cfg,instructions as dec,receiver_instructions as ri


def relocated_cfg_writes(rows, *, addresses, start, definition):
    """Keep relocated data opaque; calls have their separately bound effects."""
    result = {}
    for i, (row, at) in enumerate(zip(rows, addresses)):
        if not any(at-start <= r.offset < at-start+len(row.bytes) for r in definition.relocations):
            continue
        fact = dec.instruction_fact(row)
        if fact.is_call:
            continue
        if fact.is_jump or fact.writes_flags:
            return None
        result[i] = frozenset(r for r in dec.GPRS if fact.writes(r))
    return result


def guarded_stamp_calls(rows, *, addresses, source, start, end, root_before,
                        stamp_indices, allocation_indices, call_values,
                        local_indices=frozenset(), local_targets=None):
    """Require a native vptr stamp on every nonnull allocation-to-call path.

    The caller supplies separately authenticated allocation identities, native
    stamp operands and complete CFG register roots. The allocation and its
    zero guard must dominate each selected call. Removing the null edge must
    make the stamp dominate it as well. A null vptr cannot reach a returning
    virtual invocation; its alternative remains explicit in the receiver.
    """
    successors, unresolved=cfg._exact_invocation_cfg(rows,instruction_addresses=addresses,
        instruction_index_by_address={a:i for i,a in enumerate(addresses)},source=source,
        caller_start=start,caller_end=end,local_control_flow_indices=local_indices,
        local_control_flow_targets=local_targets or {})
    reachable=cfg._reachable_cfg_indices(successors,(0,))
    if unresolved & reachable:return {}
    predecessors={i:set() for i in range(len(rows))}
    for i in reachable:
        for j in successors.get(i,()):predecessors[j].add(i)
    facts=[dec.instruction_fact(row) for row in rows]
    guards=[]
    for branch in sorted(reachable):
        if facts[branch].mnemonic not in {'je','jne'} or len(successors.get(branch,()))!=2:continue
        test=branch-1
        while test>=0 and not facts[test].writes_flags and predecessors.get(test+1)=={test}:test-=1
        if test<0 or any(predecessors.get(i)!={i-1} for i in range(test+1,branch+1)):continue
        register=ri._exact_whole_register_zero_test(rows[test])
        if register is None and facts[test].mnemonic == 'cmp':
            operands = facts[test].operands
            if len(operands) == 2 and all(o.kind == 'register' and o.size == 4 for o in operands):
                left, right = (o.register for o in operands)
                if root_before(test, right) == 'null':
                    register = left
                elif root_before(test, left) == 'null':
                    register = right
        if register is None:continue
        root=root_before(test,register)
        if root not in allocation_indices:continue
        target=next((i for i in successors[branch] if i!=branch+1),None)
        if target is None:continue
        nonzero=branch+1 if facts[branch].mnemonic=='je' else target
        guards.append((branch,nonzero,root))
    result={}
    for call,value in call_values.items():
        if not(value.startswith('load(') and value.endswith(')')):continue
        root=value[5:-1]
        if root.startswith('nullable(') and root.endswith(')'):root=root[9:-1]
        if root not in allocation_indices:continue
        allocation=allocation_indices[root]
        if call in cfg._reachable_cfg_indices(successors,(0,),blocked=frozenset({allocation})):continue
        matching=[i for i in stamp_indices if i in reachable and root_before(i,facts[i].operands[0].base)==root]
        valid=[]
        for stamp in matching:
            interval = (cfg._reachable_cfg_indices(successors,(stamp,))
                        & cfg._indices_reaching_cfg_target(successors,call)) - {stamp,call}
            overwritten=False
            for i in interval:
                fact=facts[i]
                if not fact.writes_memory or fact.is_call or not fact.operands:continue
                dest=fact.operands[0]
                if (dest.kind=='memory' and not dest.segment and not dest.index
                        and dest.displacement==0 and dest.base!='esp'):
                    base=root_before(i,dest.base)
                    if not base or base in {root,'nullable('+root+')'}:
                        overwritten=True;break
            if overwritten:continue
            for guard,nonzero,tested in guards:
                if tested!=root:continue
                if call in cfg._reachable_cfg_indices(successors,(0,),blocked=frozenset({guard})):continue
                nonnull=dict(successors);nonnull[guard]=(nonzero,)
                if (call in cfg._reachable_cfg_indices(nonnull,(0,))
                        and call not in cfg._reachable_cfg_indices(nonnull,(0,),blocked=frozenset({stamp}))):
                    valid.append(stamp)
        if len(set(valid))==1:result[call]=value
    return result


def label_panel_cleanup(rows, *, document, indexes, caller_start, caller_end,
                   known_cleanup, bridge=None, candidate=None, definitions=None, bridge_names=None,
                   provider_bridges=None):
    """Prove allocated label receivers independently in both ZRD loaders.

    A source helper's frame size is immaterial: allocation occurrence, native
    table stamp, null guard and all intervening stack effects supply the root.
    The accepted caller/class relationships bound where this model applies.
    """
    from _recoil.call_contract import receiver_candidate as rc, receiver_retail as rr, targets
    from _recoil.call_contract.virtual_callees import (NativeVirtualEffects, _text_stack_retail_effects,
        _retail_rows, _retail_constructor_table, exact_current_table_cleanup)
    from _recoil.lib.tooling import REPO_ROOT
    caller_owners = {
        '0x4b7340': 'recoil:owner:legacy.hud_ui.class_huduichecktogglewidget',
        '0x4b59f0': 'recoil:owner:legacy.hud_ui.class_huduizrdwidget',
    }
    if caller_start not in caller_owners or bridge is None:
        return {}
    for owner_id, function in ((caller_owners[caller_start], int(caller_start, 16)),
                              ('recoil:owner:legacy.hud_ui.class_huduitransitiontextpanel', 0x4ba020)):
        owner = document.collection('owners').get(owner_id, {})
        if (any(owner.get('gates', {}).get(g) != 'accepted' for g in ('boundary','source','owner_linkage'))
                or not any(r.get('kind') == 'primary-function' and r.get('symbol_id') == 'recoil:function:'+hex(function)
                           for r in owner.get('relationships', ()))):
            return {}
    name = 'HudUiTransitionTextPanel'
    effects = _text_stack_retail_effects(((name,0x4ba020,(12,96,116,128)),),
        bridge=bridge, document=document, indexes=indexes)
    if not effects or any(v is None for v in effects.values()):
        return {}
    effects = {slot: effect for (_,slot),effect in effects.items()}
    image = (REPO_ROOT/'support/Recoil.exe').read_bytes()
    constructor, at, end = _retail_rows(bridge, document, 0x4ba020, image)
    table = _retail_constructor_table(constructor, at, start=0x4ba020, end=end, indexes=indexes)
    table_name = '??_7'+name+'@@6B@'
    if candidate:
        native = candidate.vftable_definitions.get(table_name)
        if native is None:
            return {}
        current = {slot: exact_current_table_cleanup(native, slot=slot,
            symbols=candidate.caller_definition.coff_symbols,
            definitions=dict(definitions or {}) | candidate.tu_local_function_definitions) for slot in effects}
        if any(v is None for v in current.values()):
            return {}
        if current != effects:
            raise ValueError('label-panel native virtual callee return effects differ from retail')
        effects = current
    start = int(caller_start,16)
    source = 'cod' if candidate else 'bn'
    addresses = cfg._instruction_runtime_addresses(rows, source=source, caller_start=start)
    end = start+len(candidate.caller_definition.data) if candidate else int(caller_end,16)
    definition = candidate.caller_definition if candidate else None
    opaque = {}
    identities = {}
    if candidate:
        if not rc._candidate_listing_matches_coff(rows, addresses=addresses, caller_start=start, definition=definition):
            return {}
        identities = rc._candidate_coff_direct_call_identities(rows, addresses=addresses,
            caller_start=start, definition=definition, indexes=indexes, bridge_names=bridge_names,
            compiler_generated_bridges=provider_bridges)
        opaque = relocated_cfg_writes(rows, addresses=addresses, start=start, definition=definition)
        if opaque is None:
            return {}
    stamps=set();allocations={}
    for i,(row,at) in enumerate(zip(rows,addresses)):
        fact=dec.instruction_fact(row);body=bytes.fromhex(' '.join(row.bytes))
        if fact.is_call and len(body)==5 and body[0]==0xe8:
            target=(identities.get(i) if candidate else indexes.by_address.get(hex(at+5+int.from_bytes(body[1:],'little',signed=True))))
            if target=='provider:recoil:function:0x4c5b76':
                allocations[f'allocation-result({target},{len(allocations)})']=i
        if fact.mnemonic!='mov' or len(fact.operands)!=2:
            continue
        dest,value=fact.operands
        if (dest.kind!='memory' or dest.size!=4 or dest.segment or dest.index or dest.displacement
                or dest.base not in dec.GPRS or value.kind!='immediate'):
            continue
        if candidate:
            offset=at-start
            refs=[r for r in definition.relocations if offset<=r.offset<offset+len(body)]
            symbols=[s for s in definition.coff_symbols if s.name==table_name]
            if (value.immediate!=0 or len(refs)!=len(symbols) or len(refs)!=1
                    or refs[0].offset!=offset+len(body)-4 or refs[0].type!=6
                    or refs[0].symbol_name!=table_name or refs[0].symbol_index!=symbols[0].index
                    or not all(definition.relocation_mask[offset+len(body)-4:offset+len(body)])):
                continue
        elif value.immediate & 0xffffffff != table:
            continue
        stamps.add(i)
    local_indices=candidate.local_control_flow_indices if candidate else frozenset()
    local_targets=candidate.local_control_flow_targets if candidate else {}
    slots={i:targets._exact_indirect_register_call_slot(row,allow_zero=True) for i,row in enumerate(rows)}
    selected={}
    for _ in range(len(rows)):
        cache={}
        def root_before(i,r):
            if (i,r) not in cache:
                cache[i,r]=rr._exact_retail_cfg_register_provenance(rows,before_index=i,register=r,
                    addresses=addresses,indexes=indexes,caller_start=start,caller_end=end,
                    local_control_flow_indices=local_indices,local_control_flow_targets=local_targets,
                    call_cleanup_by_instruction_index=dict(known_cleanup)|selected,source=source,
                    direct_call_identities=identities,opaque_register_writes=opaque,
                    allow_exact_caller_cleanup=True,allow_exact_affine_receiver_roots=True,
                    allow_coff_symbolic_stack_operands=bool(candidate))
            return cache[i,r]
        values={i:root_before(i,slot[0]) for i,slot in slots.items() if slot and slot[1] in effects}
        proofs=guarded_stamp_calls(rows,addresses=addresses,source=source,start=start,end=end,
            root_before=root_before,stamp_indices=stamps,allocation_indices=allocations,
            call_values=values,local_indices=local_indices,local_targets=local_targets)
        new={i:effects[slots[i][1]] for i in proofs}
        if new==selected:
            return NativeVirtualEffects(new,proofs)
        if any(i not in new or new[i]!=v for i,v in selected.items()):
            return {}
        selected=new
    return {}


