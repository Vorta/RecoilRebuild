"""Return effects for the accepted, concrete composite-panel value vector.

These effects preserve stack lineage only. They do not turn a virtual call
into a static target, accept a table extent, or change the invocation census.
Retail and current COFF supply their own callee bodies independently.
"""
from __future__ import annotations

from _recoil.call_contract import cfg, identity, instructions as decoded, listing, receiver_candidate, receiver_retail, targets
from _recoil.call_contract.current_callees import exact_callee_return_cleanup
from _recoil.commands.startup_contract import image_bytes
from _recoil.lib.constructor_dispatch import constructor_vptr_store, exact_table_slots, resolve_table_weak_target
from _recoil.lib.tooling import REPO_ROOT
from _recoil.lib.pe import parse_pe_headers, rva_to_offset


class NativeVirtualEffects(dict):
    """Invocation-local effects and their independently proved receiver roots."""

    def __init__(self, effects, receivers):
        super().__init__(effects)
        self.receivers = {index: receivers[index] for index in effects}


def unanimous_cfg_return_cleanup(rows, *, addresses, source, start, end):
    """Every reachable terminal must be a decoded RET with the same effect."""
    if not rows or None in addresses or len(set(addresses)) != len(rows) or addresses[0] != start:
        return None
    successors, unresolved = cfg._exact_invocation_cfg(rows,
        instruction_addresses=addresses,
        instruction_index_by_address={value: index for index, value in enumerate(addresses)},
        source=source, caller_start=start, caller_end=end,
        local_control_flow_indices=frozenset(), local_control_flow_targets={})
    reachable = cfg._reachable_cfg_indices(successors, (0,))
    terminals = [index for index in reachable if not successors.get(index, ())]
    if unresolved & reachable or not terminals:
        return None
    values = {identity._exact_ret_cleanup_bytes(rows[index]) for index in terminals}
    return next(iter(values)) if len(values) == 1 and None not in values else None


def exact_current_table_cleanup(table, *, slot, symbols, definitions):
    """Resolve a complete native table and its exact emitted callee."""
    if (table is None or table.section_name != ".rdata"
            or not table.section_is_comdat or table.comdat_selection != 2
            or table.section_external_symbols != (table.symbol,)
            or table.section_size != len(table.data) or slot < 0 or slot % 4
            or slot + 4 > len(table.data)
            or len(table.relocation_mask) != len(table.data)
            or not all(table.relocation_mask)):
        return None
    try:
        exact_table_slots(table.data, table.relocations, len(table.data) // 4)
        relocation = next(row for row in table.relocations if row.offset == slot)
        target = resolve_table_weak_target(relocation, symbols)
    except (ValueError, StopIteration):
        return None
    definition = definitions.get(target)
    return exact_callee_return_cleanup(definition) if definition is not None else None


def _accepted_composite_model(document, caller_start):
    owners = document.collection("owners")
    composite = owners.get("recoil:owner:legacy.hud_ui.class_huduicompositepanel", {})
    element = owners.get("recoil:owner:legacy.hud_ui.struct_huduicompositepanelentry", {})
    provider_id = "recoil:owner:provider.vc5_stl.ui_composite_panel_vector"
    provider = owners.get(provider_id, {})
    if any(owner.get("gates", {}).get(gate) != "accepted"
           for owner in (composite, element) for gate in ("boundary", "source", "owner_linkage")):
        return False
    relationships = composite.get("relationships", ())
    return (any(row.get("kind") == "primary-function" and row.get("symbol_id") == "recoil:function:" + caller_start
                for row in relationships)
        and any(row.get("kind") == "depends-on-owner" and row.get("target_owner_id") == provider_id
                for row in relationships)
        and provider.get("kind") == "provider-boundary" and provider.get("provider_state") == "accepted"
        and all(provider.get("gates", {}).get(gate) == "accepted" for gate in ("source", "boundary")))


def _retail_rows(bridge, document, address, image):
    symbol = document.collection("symbols").get("recoil:function:" + hex(address), {})
    if symbol.get("extent_state") != "known" or not isinstance(symbol.get("size"), int):
        raise ValueError("virtual callee dependency has no exact retail extent")
    rows = tuple(listing.parse_assembly(bridge.assembly(hex(address)), source="bn"))
    addresses = cfg._instruction_runtime_addresses(rows, source="bn", caller_start=address)
    cursor = address
    for row, at in zip(rows, addresses):
        data = bytes.fromhex(" ".join(row.bytes))
        if at != cursor or image_bytes(image, at, len(data)) != data:
            raise ValueError("virtual callee dependency BN/retail disagreement")
        cursor += len(data)
    end = address + symbol["size"]
    if cursor > end or end - cursor > 15 or any(byte not in (0x90, 0xCC) for byte in image_bytes(image, cursor, end - cursor)):
        raise ValueError("virtual callee dependency has unexplained trailing bytes")
    return rows, addresses, end


def composite_vector_cleanup(instructions, *, document, indexes, caller_start, caller_end,
                             known_cleanup, bridge=None, candidate=None):
    """Bind slot zero only to a proved cursor in the accepted value vector.

    This source model has a concrete 0x2c0-byte element at this+0x2ac.
    A copied range retains that element type; its registered std::copy result
    is another cursor in the same vector. Unknown roots, steps and slots never
    receive an inferred cleanup. No amount is inferred from pushed arguments.
    """
    if caller_start not in {"0x4bb790", "0x4bbca0"} or not _accepted_composite_model(document, caller_start):
        return {}
    insert = document.collection("symbols").get("recoil:function:0x4bbff0", {})
    binding = insert.get("provider_object_identity", {})
    if (insert.get("disposition") != "provider" or binding.get("proof_mode") != "canonical-header-comdat"
            or binding.get("canonical_header") != "VC/INCLUDE/vector"
            or binding.get("probe_recipe") != "vc5-vector-composite-entry-insert-count-ob1-v1"):
        return {}
    source = "cod" if candidate is not None else "bn"
    start = int(caller_start, 16)
    addresses = cfg._instruction_runtime_addresses(instructions, source=source, caller_start=start)
    direct_identities = (receiver_candidate._candidate_coff_direct_call_identities(
        instructions, addresses=addresses, caller_start=start,
        definition=candidate.caller_definition, indexes=indexes) if candidate is not None else {})
    insert_calls = []
    for index, (row, address) in enumerate(zip(instructions, addresses)):
        data = bytes.fromhex(" ".join(row.bytes))
        if cfg._instruction_mnemonic(row) != "call" or len(data) != 5 or data[0] != 0xE8:
            continue
        if ((candidate is None and address + 5 + int.from_bytes(data[1:], "little", signed=True) == 0x4bbff0)
                or (candidate is not None and direct_identities.get(index) == indexes.by_address.get("0x4bbff0"))):
            insert_calls.append(index)
    if len(insert_calls) != 1:
        return {}
    field = receiver_retail._exact_retail_cfg_register_provenance(instructions,
        before_index=insert_calls[0], register="ecx", addresses=addresses, indexes=indexes,
        caller_start=start, caller_end=start + len(candidate.caller_definition.data) if candidate else int(caller_end, 16),
        local_control_flow_indices=candidate.local_control_flow_indices if candidate else frozenset(),
        local_control_flow_targets=candidate.local_control_flow_targets if candidate else {},
        call_cleanup_by_instruction_index=known_cleanup,
        source=source, direct_call_identities=direct_identities,
        allow_exact_affine_receiver_roots=True, allow_exact_caller_cleanup=True,
        allow_coff_symbolic_stack_operands=candidate is not None)
    if field != "this+0x2a8":
        return {}
    if candidate is None:
        values = {}
        for index, row in enumerate(instructions):
            slot = targets._exact_indirect_register_call_slot(row, allow_zero=True)
            if slot is None or slot[1] != 0:
                continue
            values[index] = receiver_retail._exact_retail_cfg_register_provenance(
                instructions, before_index=index, register=slot[0], addresses=addresses, indexes=indexes,
                caller_start=start, caller_end=int(caller_end, 16),
                local_control_flow_indices=frozenset(), local_control_flow_targets={},
                call_cleanup_by_instruction_index=known_cleanup,
                allow_exact_affine_receiver_roots=True, allow_exact_caller_cleanup=True)
    else:
        values = receiver_candidate._exact_candidate_cfg_vptr_proofs(instructions,
            addresses=addresses, caller_start=start, indexes=indexes, definition=candidate.caller_definition,
            local_control_flow_indices=candidate.local_control_flow_indices,
            local_control_flow_targets=candidate.local_control_flow_targets,
            call_cleanup_by_instruction_index=known_cleanup)
    allowed = {
        "load(cursor(affine(load(this+0x2ac),bounded-stride(load(entry-stack+0x4),0x2c0)*1),+0x2c0))",
        "load(cursor(call-result(provider:recoil:function:0x4bc320),+0x2c0))",
    }
    selected = {index for index, value in values.items() if value in allowed
                and targets._exact_indirect_register_call_slot(instructions[index], allow_zero=True)[1] == 0}
    if not selected:
        return {}
    if candidate is None:
        image = (REPO_ROOT / "support/Recoil.exe").read_bytes()
        constructor, _, _ = _retail_rows(bridge, document, 0x4bc410, image)
        _, table = constructor_vptr_store(tuple(bytes.fromhex(" ".join(row.bytes)) for row in constructor))
        pe = parse_pe_headers(image, source="retail virtual table")
        offset = rva_to_offset(table - pe.image_base, pe.sections)
        last = rva_to_offset(table - pe.image_base + 3, pe.sections)
        if last != offset + 3 or offset + 4 > len(image):
            raise ValueError("virtual table slot has no exact file-backed extent")
        target = int.from_bytes(image[offset:offset + 4], "little")
        rows, callee_addresses, end = _retail_rows(bridge, document, target, image)
        cleanup = unanimous_cfg_return_cleanup(rows, addresses=callee_addresses, source="bn", start=target, end=end)
    else:
        name = "??_7HudUiTransitionTextPanel@@6B@"
        cleanup = exact_current_table_cleanup(candidate.vftable_definitions.get(name), slot=0,
            symbols=candidate.caller_definition.coff_symbols, definitions=candidate.tu_local_function_definitions)
        if bridge is None:
            return {}
        image = (REPO_ROOT / "support/Recoil.exe").read_bytes()
        constructor, _, _ = _retail_rows(bridge, document, 0x4bc410, image)
        _, table = constructor_vptr_store(tuple(bytes.fromhex(" ".join(row.bytes)) for row in constructor))
        expected = _retail_slot_cleanup(table, 0, bridge=bridge, document=document, image=image)
        if cleanup is not None and cleanup != expected:
            raise ValueError("composite-vector virtual callee return effect differs from retail")
    return {index: cleanup for index in selected} if cleanup is not None else {}


def _retail_constructor_table(rows, addresses, *, start, end, indexes):
    """Associate a native class with its unique dominating this-vptr stamp.

    The complete authenticated constructor CFG supplies the store and its
    receiver; every normal return must cross it. This establishes the table's
    class association, not the effects of calls made during construction.
    """
    successors, unresolved = cfg._exact_invocation_cfg(rows, instruction_addresses=addresses,
        instruction_index_by_address={address: index for index, address in enumerate(addresses)},
        source="bn", caller_start=start, caller_end=end,
        local_control_flow_indices=frozenset(), local_control_flow_targets={})
    reachable = cfg._reachable_cfg_indices(successors, (0,))
    if unresolved & reachable:
        return None
    stamps = []
    for index in sorted(reachable):
        fact = decoded.instruction_fact(rows[index])
        if fact.mnemonic != "mov" or len(fact.operands) != 2:
            continue
        dest, value = fact.operands
        if (dest.kind != "memory" or dest.size != 4 or dest.segment or dest.index
                or dest.displacement or dest.base not in {"eax", "ecx", "edx", "ebx", "ebp", "esi", "edi"}
                or value.kind != "immediate"):
            continue
        root = receiver_retail._exact_retail_cfg_register_provenance(rows,
            before_index=index, register=dest.base, addresses=addresses, indexes=indexes,
            caller_start=start, caller_end=end, local_control_flow_indices=frozenset(), local_control_flow_targets={})
        if root == "this":
            stamps.append((index, value.immediate & 0xffffffff))
    terminals = {index for index in reachable if not successors.get(index, ())}
    if (len(stamps) != 1 or not terminals
            or any(not decoded.instruction_fact(rows[index]).is_return for index in terminals)
            or terminals & cfg._reachable_cfg_indices(successors, (0,), blocked=frozenset({stamps[0][0]}))):
        return None
    return stamps[0][1]


def _retail_slot_cleanup(table, slot, *, bridge, document, image):
    pe = parse_pe_headers(image, source="retail virtual callee dependency")
    first = rva_to_offset(table + slot - pe.image_base, pe.sections)
    last = rva_to_offset(table + slot + 3 - pe.image_base, pe.sections)
    if last != first + 3 or first + 4 > len(image):
        raise ValueError("virtual callee slot is not an exact file-backed cell")
    target = int.from_bytes(image[first:first+4], "little")
    rows, addresses, end = _retail_rows(bridge, document, target, image)
    return unanimous_cfg_return_cleanup(rows, addresses=addresses, source="bn", start=target, end=end)


def _text_stack_retail_effects(table_specs, *, bridge, document, indexes):
    image = (REPO_ROOT / "support/Recoil.exe").read_bytes()
    effects = {}
    for name, constructor, slots in table_specs:
        rows, at, end = _retail_rows(bridge, document, constructor, image)
        table = _retail_constructor_table(rows, at, start=constructor, end=end, indexes=indexes)
        if table is None:
            return {}
        for slot in slots:
            effects[name, slot] = _retail_slot_cleanup(table, slot, bridge=bridge, document=document, image=image)
    return effects


def text_stack_cleanup(instructions, *, document, indexes, caller_start, caller_end,
                       known_cleanup, bridge=None, candidate=None, definitions=None):
    """Return effects for the accepted four-panel message-stack source model.

    Null or unknown roots, other arrays and different strides are ineligible.
    The complete-object SetEnabled ABI is checked against the container and
    both concrete message-stack tables. Embedded panels use their own table.
    These facts preserve stack coordinates only; dispatch remains targetless.
    """
    if caller_start != "0x4bd160":
        return {}
    owners = document.collection("owners")
    models = (("recoil:owner:legacy.hud_ui.class_huduitextstack4", 0x4bd160),
        ("recoil:owner:legacy.hud_ui.class_huduicontainer", 0x4bc780),
        ("recoil:owner:legacy.hud_ui.class_huduitopmessagestack", 0x4bd020),
        ("recoil:owner:legacy.hud_ui.class_huduichatmessagestack", 0x4bd2d0),
        ("recoil:owner:hud_ui.hud_ui_panel_class", 0x4ba740))
    for owner_id, function in models:
        owner = owners.get(owner_id, {})
        if (any(owner.get("gates", {}).get(gate) != "accepted" for gate in ("boundary", "source", "owner_linkage"))
                or not any(row.get("kind") == "primary-function" and row.get("symbol_id") == "recoil:function:" + hex(function)
                           for row in owner.get("relationships", ()))):
            return {}
    effects = {}
    table_specs = (("HudUiContainer", 0x4bc780, (4,)),
                   ("HudUiTopMessageStack", 0x4bd020, (4,)),
                   ("HudUiChatMessageStack", 0x4bd2d0, (4,)),
                   ("HudUiPanel", 0x4ba740, (0x60, 0x74)))
    if candidate is None:
        effects = _text_stack_retail_effects(table_specs, bridge=bridge, document=document, indexes=indexes)
        if not effects:
            return {}
    else:
        units = [(candidate.vftable_definitions, candidate.caller_definition.coff_symbols,
                  candidate.tu_local_function_definitions), *getattr(definitions, "table_units", ())]
        for name, _constructor, slots in table_specs:
            symbol = "??_7" + name + "@@6B@"
            tables = [(tables[symbol], symbols, functions) for tables, symbols, functions in units if symbol in tables]
            if not tables:
                return {}
            for slot in slots:
                values = {exact_current_table_cleanup(table, slot=slot, symbols=symbols,
                    definitions=dict(definitions or {}) | functions) for table, symbols, functions in tables}
                effects[name, slot] = next(iter(values)) if len(values) == 1 else None
        if bridge is None:
            return {}
        expected = _text_stack_retail_effects(table_specs, bridge=bridge, document=document, indexes=indexes)
        if not expected:
            return {}
        if all(value is not None for value in effects.values()) and effects != expected:
            raise ValueError("message-stack native virtual callee return effects differ from retail")
    if any(value is None for value in effects.values()) or len({effects[name, 4] for name, _, slots in table_specs if slots == (4,)}) != 1:
        return {}
    start = int(caller_start, 16)
    source = "cod" if candidate is not None else "bn"
    addresses = cfg._instruction_runtime_addresses(instructions, source=source, caller_start=start)
    selected = {}
    allowed_panels = {"load(this+0x10)", "load(cursor(this+0x558,-0x2a4))", "load(cursor(this+0x7fc,-0x2a4))"}
    for _ in range(len(instructions)):
        cleanups = dict(known_cleanup) | selected
        if candidate is not None:
            values = receiver_candidate._exact_candidate_cfg_vptr_proofs(instructions,
                addresses=addresses, caller_start=start, indexes=indexes, definition=candidate.caller_definition,
                local_control_flow_indices=candidate.local_control_flow_indices,
                local_control_flow_targets=candidate.local_control_flow_targets,
                call_cleanup_by_instruction_index=cleanups)
        else:
            values = {}
            for index, row in enumerate(instructions):
                slot = targets._exact_indirect_register_call_slot(row, allow_zero=True)
                if slot is not None and slot[1] in {4, 0x60, 0x74}:
                    values[index] = receiver_retail._exact_retail_cfg_register_provenance(instructions,
                        before_index=index, register=slot[0], addresses=addresses, indexes=indexes,
                        caller_start=start, caller_end=int(caller_end, 16),
                        local_control_flow_indices=frozenset(), local_control_flow_targets={},
                        call_cleanup_by_instruction_index=cleanups,
                        allow_exact_affine_receiver_roots=True, allow_exact_caller_cleanup=True)
        new = {}
        for index, value in values.items():
            slot = targets._exact_indirect_register_call_slot(instructions[index], allow_zero=True)[1]
            if slot == 4 and value == "load(this)":
                new[index] = effects["HudUiContainer", 4]
            elif slot in {0x60, 0x74} and value in allowed_panels:
                new[index] = effects["HudUiPanel", slot]
        if new == selected:
            return NativeVirtualEffects(selected, values)
        if any(index not in new or new[index] != value for index, value in selected.items()):
            return {}
        selected = new
    return {}


def native_virtual_cleanup(instructions, *, definitions=None, provider_bridges=None, bridge_names=None, **arguments):
    if arguments['caller_start'] in {'0x4b7340', '0x4b59f0'}:
        from _recoil.call_contract.allocation_callees import label_panel_cleanup
        return label_panel_cleanup(instructions, definitions=definitions, bridge_names=bridge_names,
            provider_bridges=provider_bridges, **arguments)
    if arguments['caller_start'] in {'0x4b90e0','0x4b8de0'}:
        from _recoil.call_contract.selector_callees import selector_cleanup
        return selector_cleanup(instructions, definitions=definitions, provider_bridges=provider_bridges, **arguments)
    return (text_stack_cleanup(instructions, definitions=definitions, **arguments)
            if arguments["caller_start"] == "0x4bd160" else composite_vector_cleanup(instructions, **arguments))
