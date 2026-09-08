"""Recoil call-contract dispatch evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import flow as _cc_flow
from _recoil.call_contract import iat as _cc_iat
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract import instructions as _cc_instructions
from _recoil.call_contract import lifecycle as _cc_lifecycle
from _recoil.call_contract import receiver_instructions as _cc_receiver_instructions
from _recoil.call_contract import receiver_storage as _cc_receiver_storage
from _recoil.call_contract import recoil_lifecycle as _cc_recoil_lifecycle
from _recoil.call_contract import source as _cc_source
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        IdentityIndexes,
        ReviewedInboundEntryRegisterTargetBridge,
        ReviewedMemberVptrStorageBridge,
        _ReviewedMemberVptrBridgeIndexes,
    )

import json
import re
import struct
from pathlib import Path
from typing import Any, Callable, Mapping, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_REL32,
    IMAGE_SYM_CLASS_EXTERNAL,
    Instruction,
)
from _recoil.lib.binja import BinaryNinjaBridge
from _recoil.lib.progress import (
    AUTHORED_ORDER_DIMENSIONS,
    ProgressDocument,
    ProgressError,
    address_value,
    is_current_accepted_state,
    normalize_address,
)
from _recoil.lib.tooling import REPO_ROOT


def _retail_register_definition_transfer_addresses(
    instructions: Sequence[Instruction], *, load_index: int, destination: str,
    caller_start: int, caller_end: int, allow_exact_zero_guard: bool = False,
    local_control_flow_indices: frozenset[int] = frozenset(),
    local_control_flow_targets: Mapping[int, Sequence[int]] | None = None,
    call_cleanup_by_instruction_index: Mapping[int, int] | None = None,
    equivalent_absolute_load_address: str = "",
    bridge: BinaryNinjaBridge | None = None,
    _diagnostic_metrics: dict[str, int] | None = None,
    _trace_overflow: Callable[[Mapping[str, Any]], None] | None = None,
) -> tuple[str, ...]:
    """Retail and COFF definitions use the same finite bit-precise flow proof."""
    addresses = _cc_cfg._instruction_runtime_addresses(instructions, source="bn", caller_start=caller_start)
    if load_index not in range(len(instructions)) or addresses[load_index] is None:
        raise ValueError("retail definition lacks its exact instruction address")
    counts = {address: addresses.count(address) for address in set(addresses) if address is not None}
    indices = {address: index for index, address in enumerate(addresses)
               if address is not None and caller_start <= address < caller_end and counts[address] == 1}
    successors, unresolved = _cc_cfg._exact_invocation_cfg(instructions, instruction_addresses=addresses,
        instruction_index_by_address=indices, source="bn", caller_start=caller_start, caller_end=caller_end,
        local_control_flow_indices=local_control_flow_indices,
        local_control_flow_targets=dict(local_control_flow_targets or {}))
    if unresolved and bridge is not None:
        # A complete must-flow consumer can handle a backward default/case;
        # the older single-pass receiver extractor deliberately cannot.
        targets = dict(local_control_flow_targets or {})
        complete = _cc_cfg.retail_local_switch_targets(instructions,
            caller_start=normalize_address(caller_start), caller_end_exclusive=normalize_address(caller_end),
            bridge=bridge, include_backward_targets=True)
        for index, edges in complete.items():
            if index in targets and tuple(targets[index]) != edges:
                raise ValueError("complete retail switch conflicts with supplied CFG edges")
            targets[index] = edges
        successors, unresolved = _cc_cfg._exact_invocation_cfg(instructions, instruction_addresses=addresses,
            instruction_index_by_address=indices, source="bn", caller_start=caller_start, caller_end=caller_end,
            local_control_flow_indices=local_control_flow_indices | frozenset(targets),
            local_control_flow_targets=targets)
    cfg = _cc_flow.ControlFlow.from_edges(len(instructions), successors, unresolved)
    # This is also a negative classification probe for ordinary address loads.
    # No register invocation means there is no definition/use fact to publish;
    # an unrelated switch cannot make that empty population ambiguous.
    if not any(_cc_cfg._instruction_mnemonic(row) in {"call", "jmp"}
               and _cc_catalog.REGISTER_RE.fullmatch(_cc_cfg._instruction_operand(row).strip().lower())
               for row in instructions):
        return ()
    if load_index not in cfg.reachable():
        return ()
    equivalents = {}
    if equivalent_absolute_load_address:
        slot = int(equivalent_absolute_load_address, 16)
        for index in cfg.reachable():
            fact = _cc_instructions.instruction_fact(instructions[index])
            if fact.mnemonic == "mov" and len(fact.operands) == 2:
                target, source = fact.operands
                if (target.kind == "register" and target.size == 4 and source.kind == "memory"
                        and source.size == 4 and not source.segment and not source.base and not source.index
                        and source.displacement & 0xffffffff == slot):
                    equivalents[index] = target.register
        if equivalents.get(load_index) != destination:
            raise ValueError("equivalent retail definition is not the governed absolute load")
    try:
        uses = _cc_flow.reaching_definition_uses(instructions, cfg, definition_index=load_index, register=destination,
            call_cleanup=call_cleanup_by_instruction_index, equivalent_definitions=equivalents)
    except _cc_flow.FlowProofError as exc:
        sites = [(normalize_address(addresses[index]), instructions[index].raw_text)
                 for index in sorted(unresolved) if addresses[index] is not None]
        raise ValueError(f"{exc}; definition={normalize_address(addresses[load_index])}; unresolved={sites!r}") from exc
    if _diagnostic_metrics is not None:
        _diagnostic_metrics.update(instruction_count=len(instructions), reached_transfer_count=len(uses))
    return tuple(normalize_address(addresses[index]) for index in uses)


def _entry_register_lineage_calls(
    instructions: Sequence[Instruction],
    *,
    source: str = "bn",
    caller_start: str,
    caller_end_exclusive: str,
    entry_registers: frozenset[str],
    cleanup_by_ordinal: Mapping[int, int] | None = None,
) -> list[tuple[int, str, str, str]]:
    """Trace entry-register callback values through exact local CFG edges."""

    cleanups = dict(cleanup_by_ordinal or {})
    if any(
        type(ordinal) is not int
        or ordinal < 0
        or type(cleanup) is not int
        or cleanup < 0
        or cleanup > 0x10000
        or cleanup % 4
        for ordinal, cleanup in cleanups.items()
    ):
        raise ValueError("entry-register lineage cleanup evidence is malformed")
    start = address_value(caller_start)
    end = address_value(caller_end_exclusive)
    addresses = _cc_cfg._instruction_runtime_addresses(
        instructions,
        source=source,
        caller_start=start,
    )
    counts: dict[int, int] = {}
    for address in addresses:
        if address is not None and start <= address < end:
            counts[address] = counts.get(address, 0) + 1
    if any(count != 1 for count in counts.values()):
        raise ValueError("entry-register lineage has duplicate instruction addresses")
    index_by_address = {
        address: index
        for index, address in enumerate(addresses)
        if address is not None and counts.get(address) == 1
    }
    successors, unresolved = _cc_cfg._exact_invocation_cfg(
        instructions,
        instruction_addresses=addresses,
        instruction_index_by_address=index_by_address,
        source=source,
        caller_start=start,
        caller_end=end,
        local_control_flow_indices=frozenset(),
        local_control_flow_targets={},
    )
    ordinal_by_index: dict[int, int] = {}
    ordinal = 0
    for index, instruction in enumerate(instructions):
        address = addresses[index]
        if address is None or not start <= address < end:
            continue
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        if mnemonic not in {"call", "jmp"}:
            continue
        local_branch = _cc_cfg._exact_local_direct_branch(
            instruction,
            instruction_index=index,
            instruction_addresses=addresses,
            instruction_index_by_address=index_by_address,
            source=source,
            caller_start=start,
            caller_end=end,
        )
        if mnemonic == "jmp" and local_branch is not None:
            continue
        ordinal_by_index[index] = ordinal
        ordinal += 1

    # This interpreter produces observations only for exact register-indirect
    # invocations.  Restrict its state space to the reverse CFG closure of
    # those observations.  A later direct-only loop (including one whose
    # callee performs RET n cleanup) cannot affect an already-observed
    # register lineage, and following it would require inventing interprocedural
    # stack-cleanup facts that this local proof does not own.  Ambiguous or
    # unbounded state on every path that can still reach a relevant invocation
    # remains fail-closed below.
    register_invocation_indices = {
        index
        for index in ordinal_by_index
        if _cc_catalog.REGISTER_RE.fullmatch(
            re.sub(
                r"^(?:near|far)\s+(?:ptr\s+)?",
                "",
                _cc_cfg._instruction_operand(instructions[index]).strip().lower(),
            )
        )
        is not None
        and _cc_cfg._exact_invocation_encoding(
            instructions[index],
            mnemonic=_cc_cfg._instruction_mnemonic(instructions[index]),
        )
    }
    predecessors: dict[int, set[int]] = {}
    for predecessor, successor_rows in successors.items():
        for successor in successor_rows:
            predecessors.setdefault(successor, set()).add(predecessor)
    relevant_indices = set(register_invocation_indices)
    pending_relevant = list(register_invocation_indices)
    while pending_relevant:
        successor = pending_relevant.pop()
        for predecessor in predecessors.get(successor, ()):
            if predecessor in relevant_indices:
                continue
            relevant_indices.add(predecessor)
            pending_relevant.append(predecessor)

    initial_registers = tuple(
        sorted(
            (register, f"entry-register({register})")
            for register in entry_registers
        )
    )
    work: list[
        tuple[
            int,
            tuple[tuple[str, str], ...],
            int,
            tuple[tuple[int, str], ...],
            tuple[tuple[int, str, int], ...],
        ]
    ] = [
        (0, initial_registers, 0, (), ())
    ] if instructions and 0 in relevant_indices else []
    visited: set[
        tuple[
            int,
            tuple[tuple[str, str], ...],
            int,
            tuple[tuple[int, str], ...],
            tuple[tuple[int, str, int], ...],
        ]
    ] = set()
    call_lineages: dict[int, set[str]] = {}
    call_shapes: dict[int, tuple[int, str, str]] = {}
    register_names = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    while work:
        (
            index,
            register_rows,
            stack_delta,
            slot_rows,
            symbolic_slot_rows,
        ) = work.pop()
        state = (
            index,
            register_rows,
            stack_delta,
            slot_rows,
            symbolic_slot_rows,
        )
        if state in visited or not 0 <= index < len(instructions):
            continue
        visited.add(state)
        registers = dict(register_rows)
        slots = dict(slot_rows)
        symbolic_slots = {
            slot_address: (symbol, symbolic_delta)
            for slot_address, symbol, symbolic_delta in symbolic_slot_rows
        }
        if index in unresolved and (
            any(registers.values()) or any(slots.values())
        ):
            raise ValueError(
                "entry-register lineage has an unresolved local CFG edge"
            )
        instruction = instructions[index]
        address = addresses[index]
        if address is None or not start <= address < end:
            continue
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            body = b""
        next_stack_delta = stack_delta
        move = _cc_receiver_instructions._exact_register_move(instruction)
        if move is not None:
            destination, source_register = move
            registers[destination] = registers.get(source_register, "")
        elif len(body) == 1 and 0x50 <= body[0] <= 0x57 and mnemonic == "push":
            source_register = register_names[body[0] - 0x50]
            if _cc_cfg._instruction_operand(instruction).strip().lower() != source_register:
                raise ValueError("entry-register PUSH rendering drifted")
            next_stack_delta -= 4
            lineage = registers.get(source_register, "")
            if lineage:
                slots[next_stack_delta] = lineage
            else:
                slots.pop(next_stack_delta, None)
            symbolic_slots.pop(next_stack_delta, None)
        elif _cc_identity._exact_unknown_push32(instruction):
            next_stack_delta -= 4
            slots.pop(next_stack_delta, None)
            symbolic_slots.pop(next_stack_delta, None)
        elif len(body) == 1 and 0x58 <= body[0] <= 0x5F and mnemonic == "pop":
            destination = register_names[body[0] - 0x58]
            if _cc_cfg._instruction_operand(instruction).strip().lower() != destination:
                raise ValueError("entry-register POP rendering drifted")
            registers[destination] = slots.pop(next_stack_delta, "")
            symbolic_slots.pop(next_stack_delta, None)
            next_stack_delta += 4
        elif _cc_identity._exact_unknown_pop32(instruction):
            slots.pop(next_stack_delta, None)
            symbolic_slots.pop(next_stack_delta, None)
            next_stack_delta += 4
        elif (stack_store := _cc_receiver_instructions._exact_stack_slot_store(instruction)) is not None:
            displacement, source_register = stack_store
            slot_address = next_stack_delta + displacement
            lineage = registers.get(source_register, "")
            if lineage:
                slots[slot_address] = lineage
            else:
                slots.pop(slot_address, None)
            symbolic_slots.pop(slot_address, None)
        elif (
            source == "cod"
            and (
                symbolic_store := _cc_receiver_instructions._exact_vc5_symbolic_stack_slot_store(
                    instruction
                )
            )
            is not None
        ):
            (
                displacement,
                source_register,
                symbol,
                rendered_displacement,
            ) = symbolic_store
            symbolic_delta = rendered_displacement - displacement
            if not -0x10000 <= symbolic_delta <= 0x10000:
                raise ValueError(
                    "entry-register lineage symbolic stack coordinate is "
                    "unbounded"
                )
            slot_address = next_stack_delta + displacement
            lineage = registers.get(source_register, "")
            if lineage:
                slots[slot_address] = lineage
                symbolic_slots[slot_address] = (symbol, symbolic_delta)
            else:
                slots.pop(slot_address, None)
                symbolic_slots.pop(slot_address, None)
        elif (stack_load := _cc_receiver_instructions._exact_stack_slot_load(instruction)) is not None:
            destination, displacement = stack_load
            registers[destination] = slots.get(
                next_stack_delta + displacement,
                "",
            )
        elif (
            source == "cod"
            and (
                symbolic_load := _cc_receiver_instructions._exact_vc5_symbolic_stack_slot_load(
                    instruction
                )
            )
            is not None
        ):
            (
                displacement,
                destination,
                symbol,
                rendered_displacement,
            ) = symbolic_load
            slot_address = next_stack_delta + displacement
            symbolic_delta = rendered_displacement - displacement
            registers[destination] = (
                slots.get(slot_address, "")
                if symbolic_slots.get(slot_address)
                == (symbol, symbolic_delta)
                else ""
            )
        elif (
            mnemonic in {"add", "sub"}
            and re.fullmatch(
                r"(?:add|sub)\s+esp\s*,\s*(?:0x[0-9a-f]+|\d+)",
                instruction.raw_text.strip(),
                flags=re.IGNORECASE,
            )
            is not None
        ):
            raw_immediate = _cc_cfg._instruction_operand(instruction).split(",", 1)[1]
            immediate = _cc_cfg._parse_unsigned_assembly_integer(raw_immediate.strip())
            if not _cc_receiver_storage._is_exact_safe_stack_root_adjustment(
                instruction,
                operation=mnemonic,
                parsed_immediate=immediate,
            ):
                if slots:
                    raise ValueError(
                        "entry-register lineage has an inexact ESP adjustment"
                    )
            else:
                prior_delta = next_stack_delta
                next_stack_delta += immediate if mnemonic == "add" else -immediate
                if mnemonic == "add":
                    for slot_address in tuple(slots):
                        if prior_delta <= slot_address < next_stack_delta:
                            slots.pop(slot_address, None)
                            symbolic_slots.pop(slot_address, None)
        elif index in ordinal_by_index:
            operand = re.sub(
                r"^(?:near|far)\s+(?:ptr\s+)?",
                "",
                _cc_cfg._instruction_operand(instruction).strip().lower(),
            )
            if _cc_catalog.REGISTER_RE.fullmatch(operand) and _cc_cfg._exact_invocation_encoding(
                instruction,
                mnemonic=mnemonic,
            ):
                lineage = registers.get(operand, "")
                call_lineages.setdefault(index, set()).add(lineage)
                call_shapes[index] = (
                    ordinal_by_index[index],
                    normalize_address(address),
                    operand,
                )
            if mnemonic == "call":
                cleanup = cleanups.get(ordinal_by_index[index], 0)
                if cleanup:
                    prior_delta = next_stack_delta
                    next_stack_delta += cleanup
                    for slot_address in tuple(slots):
                        if prior_delta <= slot_address < next_stack_delta:
                            slots.pop(slot_address, None)
                            symbolic_slots.pop(slot_address, None)
                for volatile in ("eax", "ecx", "edx"):
                    registers[volatile] = ""
        else:
            destination_operand = _cc_cfg._instruction_operand(instruction).split(",", 1)[0]
            if "[esp" in re.sub(r"\s+", "", destination_operand.lower()):
                # Any stack write not admitted by the exact full-width decoder
                # may overlap a tracked spill.  Lose all local-slot lineage.
                slots.clear()
                symbolic_slots.clear()
            if (
                mnemonic == "lea"
                and "[esp" in re.sub(
                    r"\s+",
                    "",
                    _cc_cfg._instruction_operand(instruction).lower(),
                )
            ):
                copied_range = _cc_identity._exact_bounded_forward_stack_copy_range(
                    instructions,
                    lea_index=index,
                    stack_delta=next_stack_delta,
                    successors=successors,
                    source=source,
                )
                if copied_range is None:
                    receiver_start = _cc_identity._exact_stack_receiver_escape_start(
                        instructions,
                        lea_index=index,
                        stack_delta=next_stack_delta,
                        successors=successors,
                        source=source,
                    )
                    if receiver_start is None:
                        # Address escape makes later aliasing writes unknowable.
                        slots.clear()
                        symbolic_slots.clear()
                    else:
                        # The exact ECX receiver establishes the lower bound of
                        # the stack object exposed to the invocation.  Preserve
                        # only complete spill slots strictly below that object.
                        for slot_address in tuple(slots):
                            if slot_address + 4 > receiver_start:
                                slots.pop(slot_address, None)
                                symbolic_slots.pop(slot_address, None)
                else:
                    copy_start, copy_end = copied_range
                    for slot_address in tuple(slots):
                        if (
                            slot_address < copy_end
                            and slot_address + 4 > copy_start
                        ):
                            slots.pop(slot_address, None)
                            symbolic_slots.pop(slot_address, None)
            for written in _cc_instructions.written_registers(instruction):
                registers[written] = ""
                if written == "esp":
                    slots.clear()
                    symbolic_slots.clear()
                    next_stack_delta = 0
        if not -0x10000 <= next_stack_delta <= 0x10000 or len(slots) > 64:
            raise ValueError("entry-register lineage stack state is unbounded")
        next_rows = tuple(sorted((key, value) for key, value in registers.items() if value))
        next_slot_rows = tuple(sorted(slots.items()))
        next_symbolic_slot_rows = tuple(
            sorted(
                (slot_address, symbol, symbolic_delta)
                for slot_address, (symbol, symbolic_delta) in (
                    symbolic_slots.items()
                )
            )
        )
        for successor in successors.get(index, ()):
            if successor not in relevant_indices:
                continue
            work.append(
                (
                    successor,
                    next_rows,
                    next_stack_delta,
                    next_slot_rows,
                    next_symbolic_slot_rows,
                )
            )
    calls: list[tuple[int, str, str, str]] = []
    for index, lineages in call_lineages.items():
        if len(lineages) != 1:
            continue
        lineage = next(iter(lineages))
        match = re.fullmatch(r"entry-register\((ecx|edx)\)", lineage)
        if match is None:
            continue
        ordinal_value, call_address, call_register = call_shapes[index]
        calls.append(
            (ordinal_value, call_address, call_register, match.group(1))
        )
    return sorted(
        calls,
        key=lambda row: (row[0], address_value(row[1]), row[2], row[3]),
    )


def _retail_inbound_entry_register_lineage_roots(
    retail_instructions: Sequence[Instruction],
    *,
    caller_start: str,
    bridge: BinaryNinjaBridge,
) -> frozenset[str]:
    """Prove a runtime ECX root from one exact direct caller assignment.

    This deliberately selects no static target.  The complete inbound-xref
    population must contain exactly one direct CALL, its immediately preceding
    instruction must be an exact contiguous ``MOV ECX, r32`` from a
    nonvolatile caller register, and the callee must copy entry ECX into one
    nonvolatile register before its first invocation.  The returned marker is
    then consumed by both retail and candidate abstract interpretation.
    """

    aliases = [
        move
        for instruction in retail_instructions
        if (move := _cc_receiver_instructions._exact_register_move(instruction)) is not None
        and move[0] in {"ebx", "esi", "edi", "ebp"}
        and move[1] == "ecx"
    ]
    if len(aliases) != 1:
        return frozenset()
    inbound = _cc_identity._complete_bn_inbound_direct_transfers(
        bridge,
        target_address=normalize_address(caller_start),
    )
    if inbound is None or len(inbound) != 1 or inbound[0][3] != "call":
        return frozenset()
    source_address, _source_function, source_rows, _mnemonic = inbound[0]
    addresses = _cc_cfg._instruction_runtime_addresses(
        source_rows,
        source="bn",
        caller_start=0,
    )
    call_indices = [
        index
        for index, address in enumerate(addresses)
        if address == address_value(source_address)
    ]
    if len(call_indices) != 1 or call_indices[0] == 0:
        return frozenset()
    call_index = call_indices[0]
    definition = source_rows[call_index - 1]
    definition_address = addresses[call_index - 1]
    move = _cc_receiver_instructions._exact_register_move(definition)
    try:
        definition_size = len(bytes(int(item, 16) for item in definition.bytes))
    except (TypeError, ValueError):
        return frozenset()
    if (
        move is None
        or move[0] != "ecx"
        or move[1] not in {"ebx", "esi", "edi", "ebp"}
        or definition_address is None
        or definition_address + definition_size != address_value(source_address)
    ):
        return frozenset()
    if call_index >= 2:
        prior = source_rows[call_index - 2]
        try:
            prior_body = bytes(int(item, 16) for item in prior.bytes)
        except (TypeError, ValueError):
            return frozenset()
        if (
            _cc_cfg._instruction_mnemonic(prior) == "push"
            and len(prior_body) in {1, 2, 5}
        ):
            return frozenset()
    return frozenset({"ecx"})


def _retail_inbound_entry_register_target_bridges(
    retail_instructions: Sequence[Instruction],
    *,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge: BinaryNinjaBridge,
) -> dict[int, ReviewedInboundEntryRegisterTargetBridge]:
    """Derive unanimous entry targets from the complete inbound transfer set."""
    from _recoil.call_contract.records import ReviewedInboundEntryRegisterTargetBridge

    potential_entry_lineage: dict[str, frozenset[str]] = {
        "ecx": frozenset({"ecx"}),
        "edx": frozenset({"edx"}),
    }
    potential_stack_lineage: dict[int, frozenset[str]] = {}
    has_potential_entry_transfer = False
    for instruction in retail_instructions:
        move = _cc_receiver_instructions._exact_register_move(instruction)
        if move is not None:
            potential_entry_lineage[move[0]] = potential_entry_lineage.get(
                move[1],
                frozenset(),
            )
            continue
        stack_store = _cc_receiver_instructions._exact_stack_slot_store(instruction)
        if stack_store is not None:
            displacement, source_register = stack_store
            potential_stack_lineage[displacement] = (
                potential_entry_lineage.get(source_register, frozenset())
            )
            continue
        stack_load = _cc_receiver_instructions._exact_stack_slot_load(instruction)
        if stack_load is not None:
            destination, displacement = stack_load
            potential_entry_lineage[destination] = (
                potential_stack_lineage.get(displacement, frozenset())
            )
            continue
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        operand = re.sub(
            r"^(?:near|far)\s+(?:ptr\s+)?",
            "",
            _cc_cfg._instruction_operand(instruction).strip().lower(),
        )
        if (
            mnemonic in {"call", "jmp"}
            and _cc_cfg._exact_invocation_encoding(instruction, mnemonic=mnemonic)
            and _cc_catalog.REGISTER_RE.fullmatch(operand)
        ):
            if potential_entry_lineage.get(operand):
                has_potential_entry_transfer = True
                break
        if mnemonic == "call":
            for volatile in ("eax", "ecx", "edx"):
                potential_entry_lineage[volatile] = frozenset()
            continue
        if mnemonic in {"cmp", "test"}:
            continue
        for written in _cc_instructions.written_registers(instruction):
            potential_entry_lineage[written] = frozenset()
    if not has_potential_entry_transfer:
        return {}
    # Complete inbound authority is relevant only when the callee actually
    # consumes an entry-register value as a callable.  Ordinary constructors
    # and table-dispatched methods can have pointer-bearing .rdata xrefs or a
    # callback-address materialization adjacent to an IAT call without any
    # entry-register call contract.  Querying those xrefs first incorrectly
    # turns unrelated storage provenance into a direct-transfer requirement.
    inbound = _cc_identity._complete_bn_inbound_direct_transfers(
        bridge,
        target_address=normalize_address(caller_start),
    )
    if not inbound:
        return {}
    direct_cleanup_by_ordinal = _cc_identity._retail_direct_call_cleanup_by_ordinal(
        retail_instructions,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        bridge=bridge,
    )
    lineage_calls = _entry_register_lineage_calls(
        retail_instructions,
        source="bn",
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        entry_registers=frozenset({"ecx", "edx"}),
        cleanup_by_ordinal=direct_cleanup_by_ordinal,
    )
    by_entry: dict[str, list[tuple[int, str, str, str]]] = {}
    for row in lineage_calls:
        by_entry.setdefault(row[3], []).append(row)
    callable_registers = frozenset(by_entry)
    if not callable_registers:
        return {}
    inbound_definitions = [
        _cc_iat._immediate_entry_register_targets(
            source_rows,
            call_address=source_address,
            indexes=indexes,
            callable_registers=callable_registers,
        )
        for source_address, _source_function, source_rows, _mnemonic in inbound
    ]
    incoming = {
        register: next(iter(identities))
        for register in callable_registers
        if all(register in definitions for definitions in inbound_definitions)
        and len(
            identities := {
                definitions[register]
                for definitions in inbound_definitions
            }
        )
        == 1
    }
    if not incoming:
        return {}
    result: dict[int, ReviewedInboundEntryRegisterTargetBridge] = {}
    for entry_register, target_identity in incoming.items():
        for ordinal, call_address, call_register, _entry in by_entry[
            entry_register
        ]:
            if ordinal in result:
                raise ValueError(
                    "BN inbound entry-register call ordinal is ambiguous"
                )
            result[ordinal] = ReviewedInboundEntryRegisterTargetBridge(
                ordinal=ordinal,
                retail_call_address=call_address,
                call_register=call_register,
                entry_register=entry_register,
                target_identity=target_identity,
                lineage_cleanup_by_ordinal=tuple(
                    sorted(direct_cleanup_by_ordinal.items())
                ),
            )
    return result


def _exact_vc5_register_indirect_invocation(
    instruction: Instruction,
) -> tuple[str, str] | None:
    """Decode one exact VC5 ``CALL/JMP r32`` invocation."""

    match = re.fullmatch(
        r"(?P<form>call|jmp)\s+"
        r"(?:(?:near|far|dword)\s+(?:ptr\s+)?)?"
        r"(?P<register>e(?:ax|bx|cx|dx|si|di|bp))",
        instruction.raw_text.strip(),
        flags=re.IGNORECASE,
    )
    if match is None:
        return None
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    if len(body) != 2 or body[0] != 0xFF:
        return None
    form = match.group("form").lower()
    register = match.group("register").lower()
    registers = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    modrm = body[1]
    expected_operation = 2 if form == "call" else 4
    if (
        modrm >> 6 != 3
        or (modrm >> 3) & 0x07 != expected_operation
        or registers[modrm & 0x07] != register
    ):
        return None
    return form, register


def _index_reviewed_member_vptr_storage_bridges(
    reviewed_bridges: Mapping[
        str, ReviewedMemberVptrStorageBridge
    ],
) -> _ReviewedMemberVptrBridgeIndexes:
    from _recoil.call_contract.records import _ReviewedMemberVptrBridgeIndexes
    by_load_address: dict[str, ReviewedMemberVptrStorageBridge] = {}
    by_call_address: dict[str, ReviewedMemberVptrStorageBridge] = {}
    for raw_load_address, bridge in reviewed_bridges.items():
        load_address = normalize_address(raw_load_address)
        call_address = normalize_address(bridge.call_address)
        if (
            load_address in by_load_address
            and by_load_address[load_address] != bridge
        ):
            raise ValueError(
                "conflicting reviewed member-vptr load bridges at "
                f"{load_address}"
            )
        if (
            call_address in by_call_address
            and by_call_address[call_address] != bridge
        ):
            raise ValueError(
                "conflicting reviewed member-vptr call bridges at "
                f"{call_address}"
            )
        by_load_address[load_address] = bridge
        by_call_address[call_address] = bridge
    return _ReviewedMemberVptrBridgeIndexes(
        by_load_address=by_load_address,
        by_call_address=by_call_address,
    )


def _select_registered_symbol_regex_authority(
    governing: Mapping[
        tuple[str, str, str], tuple[str, Mapping[str, Any], Mapping[str, Any], str]
    ],
    *,
    symbols: Mapping[str, Any],
    blocks: Mapping[str, Any],
    candidate_name: str,
) -> tuple[str, Mapping[str, Any], Mapping[str, Any], str]:
    """Select the accepted authored authority, not a second diagnostic view.

    Only same-address registrations can be disambiguated this way. The caller
    still proves synchronization, current order state, exact physical identity,
    membership, and COFF provenance after selection.
    """
    original = governing
    addresses = {value[3] for value in governing.values()}
    if len(governing) > 1 and len(addresses) == 1:
        address = next(iter(addresses))
        physical = []
        for symbol_id, symbol in symbols.items():
            if not isinstance(symbol, Mapping) or symbol.get("kind") not in {
                "function", "provider-function", "compiler-function",
            }:
                continue
            raw_address = symbol.get("address", symbol.get("start"))
            try:
                same_address = isinstance(raw_address, str) and normalize_address(raw_address) == address
            except ProgressError:
                same_address = False
            if same_address:
                physical.append((symbol_id, symbol))
        if len(physical) == 1:
            symbol_id, symbol = physical[0]
            block = blocks.get(symbol.get("physical_block_id"))
            facts = block.get("accepted_order_facts") if isinstance(block, Mapping) else None
            if (
                isinstance(facts, Mapping)
                and facts.get("phase") == "authored-function-order"
                and facts.get("validation_mode") == "live"
                and isinstance(facts.get("matched_identities"), list)
                and facts["matched_identities"].count(symbol_id) == 1
            ):
                governing = {
                    key: value for key, value in governing.items()
                    if value[0] == facts.get("target_id")
                }
    if len(governing) != 1:
        authorities = sorted(f"{target_id}@{address}" for target_id, address, _ in original)
        raise ValueError(
            "ambiguous active registered VC5 symbol-regex authority for "
            f"{candidate_name!r}: " + ", ".join(authorities)
        )
    return next(iter(governing.values()))


def _registered_target_symbol_regex_direct_candidate_bridges(
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    compiler_generated_bridges: Mapping[str, str],
) -> dict[str, str]:
    """Resolve exact candidate calls through one synchronized authored row.

    This is a candidate-only identity join.  Retail call order, form, target,
    and cleanup remain independently derived from Binary Ninja and compared by
    the ordinary call-contract verifier.
    """
    caller = candidate.caller_definition
    if caller is None:
        return {}

    direct_instructions_by_name: dict[
        str, list[tuple[int, Instruction, int, int]]
    ] = {}
    absolute_tu_local_names: set[str] = set()
    for instruction_index, instruction in enumerate(candidate.instructions):
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        if mnemonic not in {"call", "jmp"}:
            continue
        operand = _cc_cfg._instruction_operand(instruction).strip()
        absolute_tu_local_name = _cc_targets._cod_space_bearing_direct_target(
            operand,
            instruction.source_line,
        )
        candidate_name = absolute_tu_local_name or operand
        if (
            absolute_tu_local_name is None
            and _cc_catalog.DECORATED_RE.fullmatch(operand) is None
        ):
            continue
        if absolute_tu_local_name is not None:
            absolute_tu_local_names.add(candidate_name)
        opcode = 0xE8 if mnemonic == "call" else 0xE9
        direct_instructions_by_name.setdefault(candidate_name, []).append(
            (
                instruction_index,
                instruction,
                0,
                opcode,
            )
        )

    if not direct_instructions_by_name:
        return {}

    symbols = document.collection("symbols")
    targets = document.collection("verification_targets")
    resolved: dict[str, str] = {}
    for candidate_name, instructions in direct_instructions_by_name.items():
        absolute_tu_local_bridge = candidate_name in absolute_tu_local_names
        selected_target_name = str(
            getattr(candidate.target, "name", "") or ""
        )
        selected_target_id = (
            f"recoil:vc5-target:{selected_target_name}"
            if selected_target_name
            else ""
        )
        # Existing exact-name authorities always retain priority.  In
        # particular, an explicit ambiguous/empty candidate index must not be
        # repaired by a broader regex row.
        if (
            candidate_name in indexes.by_candidate_name
            or candidate_name in bridge_names
            or candidate_name in compiler_generated_bridges
        ):
            continue
        folded_name = candidate_name.casefold()
        collision_sources = (
            ("candidate index", indexes.by_candidate_name),
            ("Binary Ninja name", bridge_names),
            ("compiler/provider bridge", compiler_generated_bridges),
            ("candidate storage", indexes.storage_by_name),
        )
        for label, names in collision_sources:
            collisions = sorted(
                name
                for name in names
                if name.casefold() == folded_name
            )
            if collisions:
                raise ValueError(
                    "registered target symbol-regex bridge has a "
                    f"case-folded {label} collision for {candidate_name!r}: "
                    + ", ".join(repr(name) for name in collisions)
                )

        proven_instructions: list[tuple[int, Instruction, int, int]] = []
        for instruction_index, instruction, _unused_offset, opcode in instructions:
            raw_offset = _cc_cfg._source_instruction_address(instruction)
            if (
                instruction_index in candidate.local_control_flow_indices
                or not raw_offset
                or tuple(value.lower() for value in instruction.bytes[:1])
                != (f"{opcode:02x}",)
            ):
                raise ValueError(
                    "registered target symbol-regex bridge requires exact "
                    f"nonlocal E8/E9 COD evidence for {candidate_name!r}"
                )
            proven_instructions.append(
                (
                    instruction_index,
                    instruction,
                    address_value(raw_offset),
                    opcode,
                )
            )
        instructions = proven_instructions

        governing: dict[
            tuple[str, str, str], tuple[str, Mapping[str, Any], Mapping[str, Any], str]
        ] = {}
        absolute_selected_regex_matches: set[tuple[str, str, str]] = set()
        for target_id, target in targets.items():
            if (
                not isinstance(target, Mapping)
                or target.get("binary") != "recoil"
                or target.get("kind") != "vc5"
            ):
                continue
            registration = target.get("registration")
            registered_addresses = target.get("registered_addresses")
            if (
                not isinstance(registration, Mapping)
                or not isinstance(registered_addresses, list)
            ):
                continue
            for _view, row in _cc_identity._mapping_target_function_rows_with_views(target):
                symbol_regex = row.get("symbol_regex")
                reviewed_symbol = row.get("symbol")
                if (
                    absolute_tu_local_bridge
                    and str(target_id) == selected_target_id
                    and target.get("name") == selected_target_name
                    and isinstance(symbol_regex, str)
                    and symbol_regex
                ):
                    try:
                        if re.fullmatch(symbol_regex, candidate_name) is not None:
                            raw_selected_address = row.get("address")
                            if isinstance(raw_selected_address, str):
                                selected_address = normalize_address(
                                    raw_selected_address
                                )
                                selected_row_key = json.dumps(
                                    dict(row),
                                    sort_keys=True,
                                    separators=(",", ":"),
                                )
                                absolute_selected_regex_matches.add(
                                    (
                                        str(target_id),
                                        selected_address,
                                        selected_row_key,
                                    )
                                )
                    except re.error:
                        pass
                    except ProgressError:
                        pass
                if (
                    not isinstance(symbol_regex, str)
                    or not symbol_regex
                    or (
                        absolute_tu_local_bridge
                        and (
                            str(target_id) != selected_target_id
                            or target.get("name") != selected_target_name
                            or not isinstance(reviewed_symbol, str)
                            or not reviewed_symbol
                            or not _cc_identity._current_absolute_tu_local_matches_reviewed_symbol(
                                candidate_name,
                                reviewed_symbol,
                            )
                        )
                    )
                    or (
                        not absolute_tu_local_bridge
                        and reviewed_symbol not in {None, ""}
                    )
                ):
                    continue
                try:
                    matches = re.fullmatch(symbol_regex, candidate_name)
                except re.error:
                    continue
                if matches is None:
                    continue
                pipeline_class = row.get("pipeline_class")
                expected_role = (
                    "authored-body"
                    if pipeline_class == "authored"
                    else "authored-lifecycle-body"
                    if pipeline_class == "authored-lifecycle"
                    else ""
                )
                if (
                    row.get("required_presence") is not True
                    or not expected_role
                    or row.get("authored_order_role") != expected_role
                    or row.get("authored_order_gate") is not True
                    or row.get("authored_relative_order_gate") is not True
                    or row.get("full_order_gate") is not True
                    or row.get("logical_identity_key") not in {None, ""}
                    or row.get("icf_fold_status") not in {None, ""}
                ):
                    continue
                raw_address = row.get("address")
                if not isinstance(raw_address, str):
                    continue
                try:
                    address = normalize_address(raw_address)
                except ProgressError:
                    continue
                registered_at_address = [
                    item
                    for item in registered_addresses
                    if isinstance(item, str)
                    and normalize_address(item) == address
                ]
                if len(registered_at_address) != 1:
                    raise ValueError(
                        "registered target symbol-regex bridge target "
                        f"{target_id!r} has missing or duplicate active address "
                        f"{address}"
                    )
                row_key = json.dumps(
                    dict(row),
                    sort_keys=True,
                    separators=(",", ":"),
                )
                governing[(str(target_id), address, row_key)] = (
                    str(target_id),
                    target,
                    row,
                    address,
                )

        if absolute_tu_local_bridge and absolute_selected_regex_matches:
            if len(absolute_selected_regex_matches) != 1:
                raise ValueError(
                    "registered target absolute TU-local bridge requires "
                    f"exactly one selected full-regex row for {candidate_name!r}"
                )
            if not governing:
                raise ValueError(
                    "registered target absolute TU-local bridge regex match "
                    "does not preserve the exact reviewed basename and "
                    f"callable signature for {candidate_name!r}"
                )
        if not governing:
            continue
        target_id, target, row, address = _select_registered_symbol_regex_authority(
            governing,
            symbols=symbols,
            blocks=document.collection("physical_blocks"),
            candidate_name=candidate_name,
        )
        reviewed_symbol = row.get("symbol")
        _cc_identity._synchronized_finite_symbol_regex_target(
            target_id,
            target,
            address=address,
            row=row,
        )
        row_pipeline_class = row.get("pipeline_class")
        row_name = row.get("name")
        destructor_row_name = None
        parameterized_constructor_row_name = None
        if row_pipeline_class == "authored-lifecycle":
            lifecycle_row_name = _cc_recoil_lifecycle._exact_zeroarg_lifecycle_row_name(
                candidate_name
            )
            destructor_row_name = _cc_recoil_lifecycle._exact_zeroarg_destructor_row_name(
                candidate_name
            )
            parameterized_constructor_row_name = (
                _cc_lifecycle._exact_parameterized_constructor_row_name(candidate_name)
            )
            if (
                not isinstance(row_name, str)
                or lifecycle_row_name != row_name
            ):
                raise ValueError(
                    "registered target symbol-regex lifecycle bridge requires "
                    "an exact ordinary constructor or zero-argument "
                    "destructor decoration, or an exact named zero-argument "
                    "Constructor self-return method, whose class and lifecycle "
                    "kind map to the synchronized Class::Class, "
                    "Class::Constructor, or Class::~Class row name for "
                    f"{candidate_name!r}"
                )

        identity = indexes.by_address.get(address, "")
        if not identity or identity in indexes.provider_ids:
            raise ValueError(
                "registered target symbol-regex bridge requires one reviewed "
                f"non-provider identity at {address} for {candidate_name!r}"
            )
        physical_symbols: list[tuple[str, Mapping[str, Any]]] = []
        for symbol_id, symbol in symbols.items():
            if (
                not isinstance(symbol, Mapping)
                or symbol.get("kind")
                not in {"function", "provider-function", "compiler-function"}
            ):
                continue
            raw_symbol_address = symbol.get(
                "address",
                symbol.get("start"),
            )
            if not isinstance(raw_symbol_address, str):
                continue
            try:
                symbol_address = normalize_address(raw_symbol_address)
            except ProgressError:
                continue
            if symbol_address == address:
                physical_symbols.append((str(symbol_id), symbol))
        if len(physical_symbols) != 1:
            raise ValueError(
                "registered target symbol-regex bridge requires one exact "
                f"physical symbol at {address} for {candidate_name!r}"
            )
        symbol_id, symbol = physical_symbols[0]
        end_exclusive = symbol.get("end_exclusive")
        size = symbol.get("size")
        verification_target_ids = symbol.get("verification_target_ids")
        block_id = symbol.get("physical_block_id")
        block = document.collection("physical_blocks").get(str(block_id))
        accepted_order_facts = (
            block.get("accepted_order_facts")
            if isinstance(block, Mapping)
            else None
        )
        accepted_occurrences = 0
        for candidate_block in document.collection(
            "physical_blocks"
        ).values():
            candidate_facts = (
                candidate_block.get("accepted_order_facts")
                if isinstance(candidate_block, Mapping)
                else None
            )
            if (
                not isinstance(candidate_facts, Mapping)
                or candidate_facts.get("phase")
                != "authored-function-order"
                or not isinstance(
                    candidate_facts.get("matched_identities"),
                    list,
                )
            ):
                continue
            accepted_occurrences += candidate_facts[
                "matched_identities"
            ].count(symbol_id)
        authored_order = (
            block.get("order", {}).get("authored", {})
            if isinstance(block, Mapping)
            and isinstance(block.get("order"), Mapping)
            else None
        )
        symbol_navigation_name = symbol.get("navigation_name")
        expected_symbol_roles = (
            {None, "authored-body"}
            if row_pipeline_class == "authored"
            else {"authored-lifecycle-body"}
        )
        lifecycle_navigation_current = (
            row_pipeline_class != "authored-lifecycle"
            or symbol_navigation_name == row_name
            # An exact complete-destructor decoration joined to the current
            # synchronized target row is stronger identity evidence than the
            # provisional physical navigation label.  Constructor behavior
            # retains its existing tracker/BN semantic-name requirement except
            # for the equally exact parameterized-constructor bridge below;
            # named Constructor methods retain that semantic-name requirement.
            or destructor_row_name == row_name
            or parameterized_constructor_row_name == row_name
            or (
                isinstance(row_name, str)
                and _cc_identity._has_exact_bn_semantic_function(
                    bridge_names,
                    semantic_name=row_name,
                    address=address,
                )
            )
        )
        authored_navigation_current = (
            row_pipeline_class != "authored"
            or symbol_navigation_name == row_name
            or (
                absolute_tu_local_bridge
                and isinstance(reviewed_symbol, str)
                and bool(reviewed_symbol)
                and _cc_identity._current_absolute_tu_local_matches_reviewed_symbol(
                    candidate_name,
                    reviewed_symbol,
                )
            )
        )
        if (
            _cc_identity._symbol_identity(symbol_id, symbol) != identity
            or symbol.get("binary") != "recoil"
            or symbol.get("kind") != "function"
            or symbol.get("pipeline_class") != row_pipeline_class
            or symbol.get("disposition") != "unresolved"
            or symbol.get("ownership_state") != "primary-owned"
            or symbol.get("output_section_id") != "recoil:section:.text"
            or symbol.get("authored_order_role") not in expected_symbol_roles
            or symbol.get("logical_aliases") not in (None, {})
            or symbol.get("icf_address_group") is not None
            or symbol.get("linked_provider_binding") is not None
            or symbol.get("lifecycle_variant_of") is not None
            or symbol.get("extent_state") != "known"
            or (
                row_pipeline_class == "authored"
                and not authored_navigation_current
            )
            or not lifecycle_navigation_current
            or not isinstance(end_exclusive, str)
            or address_value(normalize_address(end_exclusive))
            <= address_value(address)
            or size
            != address_value(normalize_address(end_exclusive))
            - address_value(address)
            or not isinstance(verification_target_ids, list)
            or verification_target_ids.count(target_id) != 1
            or not isinstance(block_id, str)
            or not isinstance(block, Mapping)
            or not isinstance(accepted_order_facts, Mapping)
            or accepted_order_facts.get("phase")
            != "authored-function-order"
            or accepted_order_facts.get("validation_mode") != "live"
            or accepted_order_facts.get("target_id") != target_id
            or not isinstance(
                accepted_order_facts.get("covered_block_ids"),
                list,
            )
            or accepted_order_facts["covered_block_ids"].count(block_id)
            != 1
            or not isinstance(
                accepted_order_facts.get("matched_identities"),
                list,
            )
            or accepted_order_facts["matched_identities"].count(symbol_id)
            != 1
            or accepted_occurrences != 1
            or not isinstance(authored_order, Mapping)
            or any(
                not is_current_accepted_state(
                    authored_order.get(dimension)
                )
                for dimension in AUTHORED_ORDER_DIMENSIONS
            )
        ):
            raise ValueError(
                "registered target symbol-regex bridge lacks the exact "
                "authored identity, extent, block occurrence, order state, "
                "role, synchronized lifecycle name, or target membership at "
                f"{address} for {candidate_name!r}"
            )

        folded_relocations = [
            relocation
            for relocation in caller.relocations
            if relocation.symbol_name.casefold() == folded_name
        ]
        if (
            len(folded_relocations) != len(instructions)
            or any(
                relocation.symbol_name != candidate_name
                for relocation in folded_relocations
            )
        ):
            raise ValueError(
                "registered target symbol-regex bridge requires one exact "
                f"caller-object relocation per COD invocation of "
                f"{candidate_name!r}"
            )
        folded_defined_names = [
            name
            for name in caller.defined_external_functions
            if name.casefold() == folded_name
        ]
        folded_undefined_names = [
            name
            for name in caller.undefined_external_functions
            if name.casefold() == folded_name
        ]
        same_tu_defined = (
            folded_defined_names == [candidate_name]
            and not folded_undefined_names
        )
        cross_tu_undefined = (
            not folded_defined_names
            and folded_undefined_names == [candidate_name]
        )
        absolute_tu_local_definition_index: int | None = None
        if absolute_tu_local_bridge:
            defined_coff_rows = [
                definition
                for definition in caller.coff_symbols
                if definition.name.casefold() == folded_name
            ]
            if (
                len(defined_coff_rows) != 1
                or defined_coff_rows[0].name != candidate_name
                or defined_coff_rows[0].section_number <= 0
                or defined_coff_rows[0].symbol_type != 0x20
                or defined_coff_rows[0].storage_class
                != IMAGE_SYM_CLASS_EXTERNAL
                or not same_tu_defined
                or cross_tu_undefined
            ):
                raise ValueError(
                    "registered target absolute TU-local bridge requires one "
                    "exact same-TU defined COFF function symbol, with no "
                    "duplicate, undefined, case-folded, or alias row for "
                    f"{candidate_name!r}"
                )
            absolute_definition = defined_coff_rows[0]
            weak_or_coincident = [
                definition
                for definition in caller.coff_symbols
                if (
                    definition.storage_class == 105
                    and (
                        definition.name.casefold() == folded_name
                        or definition.weak_external_tag_index
                        == absolute_definition.index
                    )
                )
                or (
                    definition.index != absolute_definition.index
                    and definition.section_number
                    == absolute_definition.section_number
                    and definition.value == absolute_definition.value
                    and definition.symbol_type == 0x20
                )
            ]
            if weak_or_coincident:
                raise ValueError(
                    "registered target absolute TU-local bridge requires one "
                    "exact unaliased same-TU defined COFF function symbol"
                )
            absolute_tu_local_definition_index = absolute_definition.index
            if any(
                relocation.symbol_index
                == absolute_tu_local_definition_index
                and relocation.symbol_name != candidate_name
                for relocation in caller.relocations
            ):
                raise ValueError(
                    "registered target absolute TU-local bridge has a "
                    "differently named relocation alias for the exact "
                    f"same-TU defined function {candidate_name!r}"
                )
        if not same_tu_defined and not cross_tu_undefined:
            raise ValueError(
                "registered target symbol-regex bridge requires either one "
                "exact same-TU defined COFF external or one exact cross-TU "
                "undefined COFF external, with no duplicate, mixed, "
                "case-folded, or alias row for "
                f"{candidate_name!r}"
            )
        if cross_tu_undefined:
            registration = target.get("registration")
            target_row_key = json.dumps(
                dict(row),
                sort_keys=True,
                separators=(",", ":"),
            )
            callee_source_rows: list[
                tuple[str, Mapping[str, Any]]
            ] = []
            if isinstance(registration, Mapping):
                for contribution in registration.get(
                    "translation_unit_function_order",
                    [],
                ):
                    if not isinstance(contribution, Mapping):
                        continue
                    for contribution_row in contribution.get(
                        "functions",
                        [],
                    ):
                        if (
                            isinstance(contribution_row, Mapping)
                            and json.dumps(
                                dict(contribution_row),
                                sort_keys=True,
                                separators=(",", ":"),
                            )
                            == target_row_key
                        ):
                            callee_source_rows.append(
                                (
                                    str(
                                        contribution.get(
                                            "source_from",
                                            "",
                                        )
                                    ),
                                    contribution,
                                )
                            )

            source_trace = symbol.get("source_traceability")
            source_edges = (
                source_trace.get("source_edges")
                if isinstance(source_trace, Mapping)
                else None
            )
            callee_source = (
                callee_source_rows[0][0]
                if len(callee_source_rows) == 1
                else ""
            )
            callee_contribution = (
                callee_source_rows[0][1]
                if len(callee_source_rows) == 1
                else None
            )
            callee_manifest = (
                str(registration.get("manifest_path", ""))
                if isinstance(registration, Mapping)
                else ""
            )
            if (
                not callee_source
                or not isinstance(callee_contribution, Mapping)
                or registration.get("check_translation_unit_function_order")
                is not True
                or callee_contribution.get("order_scope") != "authored"
                or callee_contribution.get("inventory_only") is not False
                or callee_contribution.get("candidate_only_extras") != []
                or not isinstance(
                    callee_contribution.get("function_addresses"),
                    list,
                )
                or callee_contribution["function_addresses"].count(address)
                != 1
                or registration.get("source_from") != callee_source
                or not callee_manifest
                or not isinstance(source_trace, Mapping)
                or source_trace.get("state") != "resolved"
                or source_trace.get("reason_code") not in {None, ""}
                or not isinstance(source_edges, list)
                or len(source_edges) != 1
                or not isinstance(source_edges[0], Mapping)
                or source_edges[0].get("relation") != "defines"
                or not source_edges[0].get("anchor_id")
                or source_edges[0].get("emission_context")
                != {"translation_unit": callee_source}
                or block.get("agent_source_path") != callee_source
                or block.get("original_source_path") != callee_source
                or block.get("source_path") != callee_source
            ):
                raise ValueError(
                    "registered target symbol-regex cross-TU bridge lacks "
                    "one exact synchronized callee source signature for "
                    f"{candidate_name!r}"
                )

            candidate_target = candidate.target
            caller_manifest = str(
                getattr(candidate_target, "manifest_path", "")
            )
            caller_rows: list[tuple[str, Any, Any]] = []
            for contribution in getattr(
                candidate_target,
                "translation_unit_function_order",
                (),
            ):
                for candidate_row in getattr(
                    contribution,
                    "functions",
                    (),
                ):
                    candidate_symbol = str(
                        getattr(candidate_row, "symbol", "")
                    )
                    candidate_regex = getattr(
                        candidate_row,
                        "symbol_regex",
                        None,
                    )
                    exact_match = candidate_symbol == caller.symbol
                    regex_match = False
                    if (
                        not candidate_symbol
                        and isinstance(candidate_regex, str)
                        and candidate_regex
                    ):
                        try:
                            regex_match = (
                                re.fullmatch(
                                    candidate_regex,
                                    caller.symbol,
                                )
                                is not None
                            )
                        except re.error:
                            regex_match = False
                    if exact_match or regex_match:
                        caller_rows.append(
                            (
                                str(
                                    getattr(
                                        contribution,
                                        "source_from",
                                        "",
                                    )
                                ),
                                contribution,
                                candidate_row,
                            )
                        )
            caller_source = (
                caller_rows[0][0] if len(caller_rows) == 1 else ""
            )
            caller_row = (
                caller_rows[0][2] if len(caller_rows) == 1 else None
            )
            caller_contribution = (
                caller_rows[0][1] if len(caller_rows) == 1 else None
            )
            target_source_from = str(
                getattr(candidate_target, "source_from", "") or ""
            )
            raw_source_files = getattr(
                candidate_target,
                "source_files",
                (),
            )
            source_files = (
                tuple(raw_source_files)
                if isinstance(raw_source_files, (list, tuple))
                else ()
            )
            source_files_well_formed = (
                isinstance(raw_source_files, (list, tuple))
                and all(
                    isinstance(source_file, str) and bool(source_file)
                    for source_file in source_files
                )
                and len(set(source_files)) == len(source_files)
            )
            has_source_from = bool(target_source_from)
            has_source_files = bool(source_files)
            caller_source_authority_converges = (
                (has_source_from or has_source_files)
                and (not has_source_from or target_source_from == caller_source)
                and (
                    not has_source_files
                    or source_files.count(caller_source) == 1
                )
            )
            if (
                candidate_target is None
                or getattr(candidate_target, "target_binary", "") != "recoil"
                or not bool(
                    getattr(
                        candidate_target,
                        "check_translation_unit_function_order",
                        False,
                    )
                )
                or not caller_manifest
                or Path(caller_manifest).resolve()
                == (REPO_ROOT / callee_manifest).resolve()
                or not caller_source
                or caller_source == callee_source
                or not source_files_well_formed
                or not caller_source_authority_converges
                or caller.defined_external_functions.count(caller.symbol)
                != 1
                or caller.symbol in caller.undefined_external_functions
                or caller_contribution is None
                or getattr(caller_contribution, "source_from", "")
                != caller_source
                or getattr(caller_contribution, "order_scope", "")
                != "authored"
                or getattr(caller_contribution, "inventory_only", None)
                is not False
                or tuple(
                    getattr(
                        caller_contribution,
                        "candidate_only_extras",
                        (),
                    )
                )
                != ()
                or caller_row is None
                or getattr(caller_row, "required_presence", False)
                is not True
                or getattr(caller_row, "pipeline_class", "")
                not in {"authored", "authored-lifecycle"}
                or getattr(caller_row, "source_order_gate", False)
                is not True
                or getattr(caller_row, "authored_order_role", "")
                != (
                    "authored-body"
                    if getattr(caller_row, "pipeline_class", "")
                    == "authored"
                    else "authored-lifecycle-body"
                )
                or getattr(caller_row, "full_order_gate", False)
                is not True
                or getattr(caller_row, "logical_identity_key", "")
                not in {None, ""}
                or getattr(caller_row, "icf_fold_status", "")
                not in {None, ""}
            ):
                raise ValueError(
                    "registered target symbol-regex cross-TU bridge lacks "
                    "one exact distinct compiled caller source signature for "
                    f"{candidate_name!r}"
                )

        expected_relocation_offsets = {
            instruction_offset + 1
            for _index, _instruction, instruction_offset, _opcode in instructions
        }
        actual_relocation_offsets = {
            relocation.offset for relocation in folded_relocations
        }
        if (
            len(expected_relocation_offsets) != len(instructions)
            or actual_relocation_offsets != expected_relocation_offsets
            or len(caller.data) != len(caller.relocation_mask)
        ):
            raise ValueError(
                "registered target symbol-regex bridge COD/COFF callsite "
                f"population does not correspond for {candidate_name!r}"
            )
        relocation_by_offset = {
            relocation.offset: relocation for relocation in folded_relocations
        }
        for (
            _instruction_index,
            instruction,
            instruction_offset,
            opcode,
        ) in instructions:
            relocation = relocation_by_offset[instruction_offset + 1]
            field_end = relocation.offset + 4
            relocations_at_offset = [
                item
                for item in caller.relocations
                if item.offset == relocation.offset
            ]
            exact_instruction_name = (
                _cc_targets._cod_space_bearing_direct_target(
                    _cc_cfg._instruction_operand(instruction).strip(),
                    instruction.source_line,
                )
                if absolute_tu_local_bridge
                else _cc_cfg._instruction_operand(instruction).strip()
            )
            if (
                exact_instruction_name != candidate_name
                or relocation.type != IMAGE_REL_I386_REL32
                or (
                    absolute_tu_local_definition_index is not None
                    and relocation.symbol_index
                    != absolute_tu_local_definition_index
                )
                or len(relocations_at_offset) != 1
                or relocation.offset < 1
                or field_end > len(caller.data)
                or field_end > len(caller.relocation_mask)
                or caller.data[relocation.offset - 1] != opcode
                or struct.unpack_from("<I", caller.data, relocation.offset)[0]
                != 0
                or not all(
                    caller.relocation_mask[index]
                    for index in range(relocation.offset, field_end)
                )
            ):
                raise ValueError(
                    "registered target symbol-regex bridge requires exact "
                    f"zero-addend fully masked E8/E9 REL32 evidence for "
                    f"{candidate_name!r}"
                )

        resolved[candidate_name] = identity
    return resolved


def _registered_decorated_target_direct_candidate_bridges(
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    compiler_generated_bridges: Mapping[str, str],
) -> dict[str, str]:
    """Join exact unresolved decorated calls to registered target identities.

    Some focused VC5 targets publish their literal decorated callable only in
    the governed manifest.  The synchronized tracker projection still carries
    the target, exact address, and physical symbol membership, but deliberately
    omits a function-row spelling when the target is not an order target.  This
    candidate-scoped bridge rejoins those two independent halves without using
    candidate output as expected truth: COD/COFF proves only the exact external
    call spelling and callsites, while the manifest/tracker join supplies the
    existing callable identity.  Every ambiguity or stale relationship fails
    closed.
    """

    caller = candidate.caller_definition
    if caller is None:
        return {}

    invocations_by_name: dict[
        str, list[tuple[int, Instruction, int, int]]
    ] = {}
    for instruction_index, instruction in enumerate(candidate.instructions):
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        if mnemonic not in {"call", "jmp"}:
            continue
        name = _cc_cfg._instruction_operand(instruction).strip()
        if _cc_catalog.DECORATED_RE.fullmatch(name) is None:
            continue
        if (
            name in indexes.by_candidate_name
            or name in bridge_names
            or name in compiler_generated_bridges
        ):
            continue
        raw_offset = _cc_cfg._source_instruction_address(instruction)
        opcode = 0xE8 if mnemonic == "call" else 0xE9
        if (
            instruction_index in candidate.local_control_flow_indices
            or not raw_offset
            or len(instruction.bytes) != 5
            or tuple(value.lower() for value in instruction.bytes)
            != (f"{opcode:02x}", "00", "00", "00", "00")
        ):
            raise ValueError(
                "registered decorated-target bridge requires exact nonlocal "
                f"zero-addend E8/E9 COD evidence for {name!r}"
            )
        invocations_by_name.setdefault(name, []).append(
            (
                instruction_index,
                instruction,
                address_value(raw_offset),
                opcode,
            )
        )

    if not invocations_by_name:
        return {}

    root = REPO_ROOT.resolve()
    manifest_root = (root / "tools" / "vc5_verify_targets").resolve()
    symbols = document.collection("symbols")
    targets = document.collection("verification_targets")
    resolved: dict[str, str] = {}
    for name, invocations in invocations_by_name.items():
        folded_name = name.casefold()
        for label, names in (
            ("candidate index", indexes.by_candidate_name),
            ("Binary Ninja name", bridge_names),
            ("compiler/provider bridge", compiler_generated_bridges),
            ("candidate storage", indexes.storage_by_name),
        ):
            collisions = sorted(
                value for value in names if value.casefold() == folded_name
            )
            if collisions:
                raise ValueError(
                    "registered decorated-target bridge has a case-folded "
                    f"{label} collision for {name!r}: "
                    + ", ".join(repr(value) for value in collisions)
                )

        suppliers: list[
            tuple[str, str, str, Mapping[str, Any], Any]
        ] = []
        for target_id, target_row in targets.items():
            if (
                not isinstance(target_row, Mapping)
                or target_row.get("binary") != "recoil"
                or target_row.get("kind") != "vc5"
            ):
                continue
            registration = target_row.get("registration")
            manifest_value = (
                registration.get("manifest_path")
                if isinstance(registration, Mapping)
                else None
            )
            if not isinstance(manifest_value, str) or not manifest_value:
                continue
            manifest_path = Path(manifest_value)
            if not manifest_path.is_absolute():
                manifest_path = root / manifest_path
            try:
                manifest_path = manifest_path.resolve()
                manifest_path.relative_to(manifest_root)
                before = manifest_path.stat()
                raw_manifest = json.loads(
                    manifest_path.read_text(encoding="utf-8")
                )
                after = manifest_path.stat()
            except (OSError, ValueError, json.JSONDecodeError):
                continue
            if (
                int(before.st_size) != int(after.st_size)
                or int(before.st_mtime_ns) != int(after.st_mtime_ns)
                or not isinstance(raw_manifest, Mapping)
            ):
                raise ValueError(
                    "registered decorated-target manifest changed during "
                    f"identity routing: {manifest_value}"
                )
            raw_name_rows = [
                row
                for row in raw_manifest.get("functions", [])
                if isinstance(row, Mapping)
                and isinstance(row.get("symbol"), str)
                and row["symbol"].casefold() == folded_name
            ]
            if not raw_name_rows:
                continue
            if (
                len(raw_name_rows) != 1
                or raw_name_rows[0].get("symbol") != name
            ):
                raise ValueError(
                    "registered decorated-target manifest has duplicate or "
                    f"case-conflicting identity for {name!r}: {manifest_value}"
                )

            try:
                live_target = _cc_source._call_contract_cached_manifest(
                    document, manifest_path
                )
            except (OSError, ProgressError, ValueError) as exc:
                raise ValueError(
                    "registered decorated-target manifest cannot supply an "
                    f"exact live target for {name!r}: {manifest_value}: {exc}"
                ) from exc
            live_rows = [
                row
                for row in tuple(getattr(live_target, "functions", ()))
                if str(getattr(row, "symbol", "")).casefold()
                == folded_name
            ]
            if (
                len(live_rows) != 1
                or str(getattr(live_rows[0], "symbol", "")) != name
                or getattr(live_rows[0], "symbol_regex", None) is not None
            ):
                raise ValueError(
                    "registered decorated-target live manifest has missing, "
                    f"duplicate, or conflicting identity for {name!r}"
                )
            live_row = live_rows[0]
            try:
                address = normalize_address(str(getattr(live_row, "address", "")))
            except ProgressError as exc:
                raise ValueError(
                    "registered decorated-target live manifest has an invalid "
                    f"address for {name!r}"
                ) from exc
            symbol_id = f"recoil:function:{address}"
            function_addresses = registration.get("function_addresses")
            target_symbol_ids = target_row.get("symbol_ids")
            unresolved_addresses = target_row.get("unresolved_addresses")
            try:
                registered_count = sum(
                    1
                    for raw_address in function_addresses or []
                    if isinstance(raw_address, str)
                    and normalize_address(raw_address) == address
                )
            except ProgressError as exc:
                raise ValueError(
                    "registered decorated-target tracker projection has an "
                    f"invalid function address for {name!r}"
                ) from exc
            if (
                str(target_id)
                != f"recoil:vc5-target:{getattr(live_target, 'name', '')}"
                or target_row.get("name")
                != str(getattr(live_target, "name", ""))
                or str(getattr(live_target, "target_binary", "")) != "recoil"
                or Path(str(getattr(live_target, "manifest_path", ""))).resolve()
                != manifest_path
                or raw_manifest.get("name") != target_row.get("name")
                or registered_count != 1
                or not isinstance(target_symbol_ids, list)
                or target_symbol_ids.count(symbol_id) != 1
                or unresolved_addresses != []
                or getattr(live_row, "required_presence", None) is not True
            ):
                raise ValueError(
                    "registered decorated-target identity has stale target, "
                    f"address, presence, or symbol membership for {name!r}"
                )
            identity = indexes.by_address.get(address, "")
            if not identity:
                raise ValueError(
                    "registered decorated-target identity has no unique "
                    f"tracker callable at {address} for {name!r}"
                )
            suppliers.append(
                (str(target_id), address, identity, target_row, live_row)
            )

        if not suppliers:
            continue
        physical_identities = {
            (address, identity) for _target_id, address, identity, _target, _row in suppliers
        }
        if len(physical_identities) != 1:
            raise ValueError(
                "ambiguous registered decorated-target identity for "
                f"{name!r}: "
                + ", ".join(
                    sorted(
                        f"{target_id}@{address}={identity}"
                        for target_id, address, identity, _target, _row in suppliers
                    )
                )
            )
        address, identity = next(iter(physical_identities))
        physical_symbols: list[tuple[str, Mapping[str, Any]]] = []
        for symbol_id, symbol in symbols.items():
            if not isinstance(symbol, Mapping):
                continue
            raw_address = symbol.get("address", symbol.get("start"))
            if not isinstance(raw_address, str):
                continue
            try:
                symbol_address = normalize_address(raw_address)
            except ProgressError:
                continue
            if symbol_address == address:
                physical_symbols.append((str(symbol_id), symbol))
        if len(physical_symbols) != 1:
            raise ValueError(
                "registered decorated-target bridge requires one exact "
                f"physical callable at {address} for {name!r}"
            )
        symbol_id, symbol = physical_symbols[0]
        end_exclusive = symbol.get("end_exclusive")
        size = symbol.get("size")
        verification_target_ids = symbol.get("verification_target_ids")
        supplier_target_ids = {row[0] for row in suppliers}
        if (
            symbol_id != f"recoil:function:{address}"
            or _cc_identity._symbol_identity(symbol_id, symbol) != identity
            or symbol.get("binary") != "recoil"
            or symbol.get("kind")
            not in {"function", "provider-function", "compiler-function"}
            or symbol.get("ownership_state") != "primary-owned"
            or symbol.get("extent_state") != "known"
            or symbol.get("output_section_id") != "recoil:section:.text"
            or not isinstance(end_exclusive, str)
            or address_value(normalize_address(end_exclusive))
            <= address_value(address)
            or size
            != address_value(normalize_address(end_exclusive))
            - address_value(address)
            or not isinstance(verification_target_ids, list)
            or any(
                verification_target_ids.count(target_id) != 1
                for target_id in supplier_target_ids
            )
            or (identity in indexes.provider_ids) != identity.startswith("provider:")
        ):
            raise ValueError(
                "registered decorated-target bridge lacks the exact physical "
                f"identity, extent, provider class, or target membership at "
                f"{address} for {name!r}"
            )

        folded_relocations = [
            relocation
            for relocation in caller.relocations
            if relocation.symbol_name.casefold() == folded_name
        ]
        folded_defined = [
            value
            for value in caller.defined_external_functions
            if value.casefold() == folded_name
        ]
        folded_undefined = [
            value
            for value in caller.undefined_external_functions
            if value.casefold() == folded_name
        ]
        expected_offsets = {
            instruction_offset + 1
            for _index, _instruction, instruction_offset, _opcode in invocations
        }
        if (
            len(folded_relocations) != len(invocations)
            or any(row.symbol_name != name for row in folded_relocations)
            or folded_defined
            or folded_undefined != [name]
            or len(expected_offsets) != len(invocations)
            or {row.offset for row in folded_relocations} != expected_offsets
            or len(caller.data) != len(caller.relocation_mask)
        ):
            raise ValueError(
                "registered decorated-target bridge requires one exact "
                "undefined COFF external and one exact caller relocation per "
                f"COD invocation of {name!r}"
            )
        relocation_by_offset = {
            relocation.offset: relocation for relocation in folded_relocations
        }
        for _index, instruction, instruction_offset, opcode in invocations:
            relocation = relocation_by_offset[instruction_offset + 1]
            field_end = relocation.offset + 4
            if (
                _cc_cfg._instruction_operand(instruction).strip() != name
                or relocation.type != IMAGE_REL_I386_REL32
                or sum(
                    1
                    for row in caller.relocations
                    if row.offset == relocation.offset
                )
                != 1
                or relocation.offset < 1
                or field_end > len(caller.data)
                or field_end > len(caller.relocation_mask)
                or caller.data[relocation.offset - 1] != opcode
                or struct.unpack_from("<I", caller.data, relocation.offset)[0]
                != 0
                or caller.relocation_mask[relocation.offset - 1]
                or not all(
                    caller.relocation_mask[index]
                    for index in range(relocation.offset, field_end)
                )
            ):
                raise ValueError(
                    "registered decorated-target bridge requires exact "
                    f"zero-addend fully masked E8/E9 REL32 evidence for {name!r}"
                )
        resolved[name] = identity
    return resolved


def _registered_non_authored_provider_names(
    document: ProgressDocument, address: str,
) -> frozenset[str]:
    """Collect literal registered names, not regex guesses or new ICF aliases."""
    names: set[str] = set()
    for target in document.collection("verification_targets").values():
        registration = target.get("registration") if isinstance(target, Mapping) else None
        if not isinstance(registration, Mapping):
            continue
        groups = [registration]
        for key in ("translation_unit_function_order", "linked_function_intervals"):
            groups.extend(row for row in registration.get(key, []) if isinstance(row, Mapping))
        for group in groups:
            for row in group.get("functions", []):
                if (
                    isinstance(row, Mapping)
                    and row.get("address") == address
                    and row.get("pipeline_class") == "non-authored"
                    and row.get("authored_order_role") == "non-authored"
                    and isinstance(row.get("symbol"), str)
                    and row["symbol"]
                ):
                    names.add(row["symbol"])
    return frozenset(names)
