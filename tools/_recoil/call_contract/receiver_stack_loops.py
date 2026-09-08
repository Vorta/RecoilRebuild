"""Induction for one spilled byte-offset cursor in a straight-line loop."""
from _recoil.call_contract import cfg, instructions as dec, receiver_retail as rr, receiver_candidate as rc


def prove_stack_cursor_loop(rows, *, addresses, source, start, end, indexes, cleanups,
                            direct_identities=None, local_indices=frozenset(), local_targets=None,
                            definition=None):
    """Prove storage lineage, without claiming array extent or loop termination.

    The preheader seeds a scalar stack local. Each iteration adds the same
    object-array base, uses that element as ECX and as the vptr source, reloads
    the saved scalar, and advances it by the same positive byte stride. The
    exact CFG excludes bypasses. Known callee effects and disjoint local writes
    establish the stack coordinate through the complete iteration.
    """
    if not rows or None in addresses or len(set(addresses)) != len(rows):
        return {}
    # An earlier escape could let a later callee overwrite this local through
    # an alias. Native SEH's FS registration store is not an argument escape.
    for row in rows:
        fact = dec.instruction_fact(row)
        if (fact.mnemonic == 'lea' and any(op.base == 'esp' or op.index == 'esp' for op in fact.operands)
                or fact.mnemonic == 'push' and fact.operands[0].register == 'esp'
                or fact.mnemonic == 'mov' and len(fact.operands) == 2
                   and fact.operands[0].kind == 'register' and fact.operands[1].register == 'esp'):
            return {}
    opaque = {}
    if source == 'cod':
        if definition is None or not rc._candidate_listing_matches_coff(
                rows, addresses=addresses, caller_start=start, definition=definition):
            return {}
        for i, (row, address) in enumerate(zip(rows, addresses)):
            fact = dec.instruction_fact(row)
            if not any(address-start <= r.offset < address-start+len(row.bytes)
                       for r in definition.relocations):
                continue
            if fact.is_jump or fact.mnemonic in {'cmp', 'test'}:
                return {}
            if not fact.is_call:
                opaque[i] = frozenset(r for r in dec.GPRS if fact.writes(r))
    elif source != 'bn':
        return {}
    successors, unresolved=cfg._exact_invocation_cfg(rows,instruction_addresses=addresses,
        instruction_index_by_address={a:i for i,a in enumerate(addresses)},source=source,
        caller_start=start,caller_end=end,local_control_flow_indices=local_indices,
        local_control_flow_targets=local_targets or {})
    predecessors={i:set() for i in range(len(rows))}
    for a,bs in successors.items():
        for b in bs:predecessors[b].add(a)
    facts=[dec.instruction_fact(row) for row in rows]
    def reg_value(index,register):
        return rr._exact_retail_cfg_register_provenance(rows,before_index=index,register=register,
            addresses=addresses,indexes=indexes,caller_start=start,caller_end=end,
            local_control_flow_indices=local_indices,local_control_flow_targets=local_targets or {},
            call_cleanup_by_instruction_index=cleanups,source=source,direct_call_identities=direct_identities,
            allow_exact_caller_cleanup=True,allow_exact_affine_receiver_roots=True,
            allow_coff_symbolic_stack_operands=source=='cod',opaque_register_writes=opaque)
    def stack_operand(op):return op.kind=='memory' and op.base=='esp' and not op.index and not op.segment
    def move(fact):return fact.mnemonic=='mov' and len(fact.operands)==2
    result={}
    for tail,targets in successors.items():
        heads=[h for h in targets if 0<h<tail]
        if len(heads)!=1 or set(targets)!={heads[0],tail+1}:continue
        head=heads[0]
        if any(i in opaque for i in range(head-1,tail+1)):continue
        if any(successors.get(i)!=(i+1,) for i in range(head,tail)):continue
        if any(predecessors[i]!={i-1} for i in range(head+1,tail+1)):continue
        if predecessors[head]!={head-1,tail} or unresolved & set(range(head-1,tail+1)):continue
        seed=facts[head-1]
        if not move(seed):continue
        memory,register=seed.operands
        if not stack_operand(memory) or memory.size!=4 or register.kind!='register' or register.register not in {'ebx','ebp','esi','edi'}:continue
        cursor=register.register;slot=memory.displacement
        initial=reg_value(head-1,cursor)
        if initial!='null' and not (initial.startswith('bounded-stride(') and rc._bounded_stride_components(initial)):continue
        writes=[i for i in range(head,tail) if facts[i].writes(cursor)]
        if len(writes)!=3:continue
        combine,reload,update=writes
        add=facts[combine];load=facts[reload];step=facts[update]
        if not (add.mnemonic=='add' and len(add.operands)==2 and add.operands[0].register==cursor and add.operands[0].size==4
                and add.operands[1].kind=='register' and move(load) and load.operands[0].register==cursor
                and stack_operand(load.operands[1]) and load.operands[1].size==4
                and step.mnemonic=='add' and len(step.operands)==2 and step.operands[0].register==cursor
                and step.operands[0].size==4 and step.operands[1].kind=='immediate' and 0<step.operands[1].immediate<=0x10000):continue
        stride=step.operands[1].immediate
        base=reg_value(combine,add.operands[1].register)
        if not rc._bounded_targetless_pointer_base(base):continue
        calls=[i for i in range(head,tail) if facts[i].is_call]
        indirect=[i for i in calls if facts[i].operands[0].kind=='memory']
        if len(indirect)!=1:continue
        call=indirect[0];operand=facts[call].operands[0]
        if not combine<call<reload or operand.index or operand.segment or operand.base not in {'ebx','ebp','esi','edi'}:continue
        vptr=operand.base
        definitions=[i for i in range(head,call) if facts[i].writes(vptr)]
        if len(definitions)!=1 or not combine<definitions[0]<call:continue
        load_vptr=facts[definitions[0]]
        if not (move(load_vptr) and load_vptr.operands[0].register==vptr
                and load_vptr.operands[1].kind=='memory' and load_vptr.operands[1].base==cursor
                and not load_vptr.operands[1].index and not load_vptr.operands[1].segment
                and load_vptr.operands[1].displacement==0 and load_vptr.operands[1].size==4):continue
        # The caller really passes this same current element to the native slot.
        ecx_defs=[i for i in range(combine,call) if facts[i].writes('ecx')]
        if not ecx_defs:continue
        this=facts[ecx_defs[-1]]
        if not(move(this) and this.operands[0].register=='ecx' and this.operands[1].register==cursor):continue
        delta=0;stores=[];valid=True
        for i in range(head,tail):
            fact=facts[i]
            if fact.is_call:
                effect=cleanups.get(i)
                if effect is None:valid=False;break
                delta+=effect
                continue
            if fact.mnemonic=='push' and fact.operands[0].size==4:
                delta-=4
                if delta<slot+4 and slot<delta+4:valid=False;break
                continue
            if fact.writes('esp'):valid=False;break
            if fact.mnemonic=='lea' and any(op.base=='esp' for op in fact.operands):valid=False;break
            if i==reload and delta+load.operands[1].displacement!=slot:valid=False;break
            if fact.writes_memory:
                dest=fact.operands[0] if fact.operands else None
                if dest is None or not stack_operand(dest):valid=False;break
                at=delta+dest.displacement
                if at<slot+4 and slot<at+dest.size:
                    if not(move(fact) and dest.size==4 and at==slot and fact.operands[1].register==cursor and i>update):valid=False;break
                    stores.append(i)
        if not valid or delta!=0 or len(stores)!=1:continue
        pointer=base if initial=='null' else f'affine({base},{initial}*1)'
        result[call]=f'load(cursor({pointer},{stride:+#x}))'
    return result



def spilled_scalar_seeds(rows, *, successors, unresolved, before_index, cleanups, opaque_indices=frozenset()):
    """Prove a saved byte offset survives every path through a nested loop.

    This only supplies a recurrence; the caller separately proves the seed's
    zero value. Stack effects are supplied by independently verified callees.
    Register reuse is permitted only if every arrival at the outer update
    restores the iteration's saved value from the same unmodified local.
    """
    reachable = cfg._reachable_cfg_indices(successors,(0,))
    try:
        facts={i:dec.instruction_fact(rows[i]) for i in reachable}
    except dec.InstructionProofError:
        return {}
    predecessors={i:set() for i in range(len(rows))}
    for i,targets in successors.items():
        for j in targets:predecessors[j].add(i)
    def stack(op):return op.kind=='memory' and op.base=='esp' and not op.index and not op.segment
    def mov(f):return f.mnemonic=='mov' and len(f.operands)==2
    result={}
    for tail,targets in successors.items():
        if tail not in reachable:continue
        heads=[i for i in targets if 0<i<=before_index<tail]
        if len(heads)!=1 or set(targets)!={heads[0],tail+1}:continue
        head=heads[0]
        if head-1 not in reachable:continue
        seed=facts[head-1]
        if not mov(seed) or not stack(seed.operands[0]) or seed.operands[0].size!=4:continue
        cursor=seed.operands[1].register;slot=seed.operands[0].displacement
        if cursor not in {'ebx','ebp','esi','edi'} or slot<0:continue
        if predecessors[head]!={head-1,tail}:continue
        region=set(range(head,tail+1))
        if not region <= reachable:continue
        if opaque_indices & (region | {head-1}) or unresolved&region or any(predecessors[i]-region for i in range(head+1,tail+1)):continue
        if any(set(successors.get(i,()))-region for i in range(head,tail)):continue
        saved=facts[tail-1]
        if not(mov(saved) and stack(saved.operands[0]) and saved.operands[0].size==4
               and saved.operands[0].displacement==slot and saved.operands[1].register==cursor):continue
        updates=[i for i in range(head,tail-1) if facts[i].writes(cursor)]
        if not updates:continue
        update=updates[-1];step=facts[update]
        if not(step.mnemonic=='add' and len(step.operands)==2 and step.operands[0].register==cursor
               and step.operands[0].size==4 and step.operands[1].kind=='immediate' and 0<step.operands[1].immediate<=0x10000):continue
        if any(successors.get(i)!=(i+1,) for i in range(update,tail)):continue
        # Prove the pre-update cursor is the original iteration value on all
        # arrivals. A nested loop may destroy it, but a reload must restore it.
        incoming={head:({cursor},0)};pending=[head];valid=True
        while pending and valid:
            i=pending.pop();known,delta=incoming[i]
            if i==update:continue
            f=facts[i];out=set(known);next_delta=delta
            if f.is_call:
                effect=cleanups.get(i)
                if effect is None:valid=False;break
                next_delta+=effect;out-={'eax','ecx','edx'}
            elif f.mnemonic=='push' and f.operands[0].size==4:
                next_delta-=4
                if next_delta<slot+4 and slot<next_delta+4:valid=False;break
            elif f.writes('esp'):
                valid=False;break
            else:
                out-={r for r in dec.GPRS if f.writes(r)}
                if mov(f) and f.operands[0].kind=='register':
                    dest,value=f.operands
                    if value.kind=='register' and value.register in known:
                        out.add(dest.register)
                    elif stack(value) and value.size==4 and delta+value.displacement==slot:
                        out.add(dest.register)
                if f.writes_memory:
                    dest=f.operands[0] if f.operands else None
                    if dest is not None and stack(dest):
                        at=delta+dest.displacement
                        if at<slot+4 and slot<at+dest.size:valid=False;break
                if f.mnemonic=='lea' and len(f.operands)==2 and stack(f.operands[1]):
                    at=delta+f.operands[1].displacement
                    if slot<=at<slot+4:valid=False;break
            for target in successors.get(i,()):
                if target not in region or target==head:valid=False;break
                state=(out,next_delta)
                if target in incoming:
                    old,old_delta=incoming[target]
                    if old_delta!=next_delta:valid=False;break
                    state=(old&out,next_delta)
                if incoming.get(target)!=state:
                    incoming[target]=state;pending.append(target)
        if not valid or update not in incoming or incoming[update][1]!=0 or cursor not in incoming[update][0]:continue
        for i in range(update+1,tail-1):
            f=facts[i]
            if f.writes('esp') or f.is_call:valid=False;break
            if f.writes_memory:
                dest=f.operands[0]
                if not stack(dest) or dest.displacement<slot+4 and slot<dest.displacement+dest.size:valid=False;break
        if valid:result[head-1]=(cursor,slot,step.operands[1].immediate,update)
    return result



def natural_zero_stride_seeds(rows,*,before_index,successors,unresolved):
    """A fresh zero preheader can reset an inner cursor on each outer cycle."""
    from _recoil.call_contract import receiver_proofs as ri
    result={}
    pred={i:set() for i in range(len(rows))}
    for i,ts in successors.items():
        for t in ts:pred[t].add(i)
    for tail,ts in successors.items():
        heads=[h for h in ts if 0<h<=before_index<tail]
        if len(heads)!=1 or set(ts)!={heads[0],tail+1}:continue
        head=heads[0];reg=ri._exact_zero_register(rows[head-1])
        if reg not in {'ebx','ebp','esi','edi'} or pred[head]!={head-1,tail}:continue
        region=set(range(head,tail+1))
        if unresolved&region or any(pred[i]-region for i in range(head+1,tail+1)):continue
        writes=[i for i in region if dec.instruction_fact(rows[i]).writes(reg)]
        if len(writes)!=1:continue
        update=writes[0];fact=dec.instruction_fact(rows[update])
        if not(fact.mnemonic=='add' and fact.operands[0].register==reg and fact.operands[0].size==4
                and fact.operands[1].kind=='immediate' and 0<fact.operands[1].immediate<=0x10000):continue
        if tail in cfg._reachable_cfg_indices(successors,(head,),blocked=frozenset({update})):continue
        marker=rc._bounded_stride_provenance('bounded-counter('+reg+')',fact.operands[1].immediate)
        for i in (head-1,update):
            if i in result and result[i] != (reg,marker): return {}
            result[i]=(reg,marker)
    return result
