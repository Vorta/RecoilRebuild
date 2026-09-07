"""Recoil call-contract receiver cursor evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import instructions as _cc_instructions
from _recoil.call_contract import receiver_instructions as _cc_receiver_instructions
from _recoil.call_contract import receiver_storage as _cc_receiver_storage

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateExactIatRegisterLoadProof,
        IdentityIndexes,
    )

import re
from dataclasses import dataclass, replace
from typing import Mapping, Sequence

from _recoil.commands.asm_verify import Instruction


@dataclass(frozen=True)
class _AddressCursor:
    anchor: str
    delta: int = 0


@dataclass
class _CursorFlowState:
    registers: dict[str, str]
    cursor_registers: dict[str, _AddressCursor]
    cursor_slots: dict[int, _AddressCursor]


def _copy_cursor_flow_state(state: _CursorFlowState) -> _CursorFlowState:
    return _CursorFlowState(
        _cc_cfg._register_state_snapshot(state.registers),
        dict(state.cursor_registers),
        dict(state.cursor_slots),
    )


def _merge_cursor_flow_states(
    states: Sequence[_CursorFlowState],
) -> _CursorFlowState:
    registers = _cc_cfg._merge_register_states([state.registers for state in states])
    cursor_registers: dict[str, _AddressCursor] = {}
    for register in _cc_catalog.REGISTER_STATE_NAMES:
        values = {state.cursor_registers.get(register) for state in states}
        if len(values) == 1:
            value = next(iter(values))
            if value is not None:
                cursor_registers[register] = value
    slots = (
        set.intersection(*(set(state.cursor_slots) for state in states))
        if states
        else set()
    )
    cursor_slots: dict[int, _AddressCursor] = {}
    for slot in slots:
        values = {state.cursor_slots.get(slot) for state in states}
        if len(values) == 1:
            value = next(iter(values))
            if value is not None:
                cursor_slots[slot] = value
    return _CursorFlowState(registers, cursor_registers, cursor_slots)


def _address_cursor_seed(abstract: str) -> _AddressCursor | None:
    if (
        re.fullmatch(
            r"address\((?:this|storage:[^()]+)"
            r"(?:[+-]0x[0-9a-f]+)*\)",
            abstract,
        )
        is None
    ):
        return None
    return _AddressCursor(abstract)


def _render_address_cursor(cursor: _AddressCursor) -> str:
    if cursor.delta == 0:
        return cursor.anchor
    match = re.fullmatch(r"address\((.+)\)", cursor.anchor)
    if match is None:
        return ""
    inner = match.group(1)
    base = re.sub(r"(?:[+-]0x[0-9a-f]+)+$", "", inner)
    displacement = _cc_receiver_storage._additive_provenance_displacement(inner) + cursor.delta
    return f"address({_cc_receiver_instructions._abstract_with_displacement(base, displacement)})"


def _transparent_address_cursor_stack_loads(
    instructions: Sequence[Instruction],
    *,
    source: str,
    caller_start: int,
    caller_end: int,
    indexes: IdentityIndexes,
    reviewed_register_storage_bridges: Mapping[str, str],
    reviewed_iat_register_definition_offsets: frozenset[str] = frozenset(),
    candidate_exact_iat_register_load_proofs: Mapping[
        str, CandidateExactIatRegisterLoadProof
    ] | None = None,
) -> dict[int, str]:
    """Prove transparent, non-escaped local address-cursor spills."""
    candidate_slots = {
        store[0]
        for instruction in instructions
        if (store := _cc_receiver_instructions._exact_stack_slot_store(instruction)) is not None
    }
    if not candidate_slots:
        return {}
    persistent_unsafe_slots: set[int] = set()
    killed_slots_by_index: dict[int, set[int]] = {}
    for instruction_index, instruction in enumerate(instructions):
        load = _cc_receiver_instructions._exact_stack_slot_load(instruction)
        store = _cc_receiver_instructions._exact_stack_slot_store(instruction)
        exact_slot = (
            load[1] if load is not None else
            store[0] if store is not None else None
        )
        for operand_part in _cc_cfg._instruction_operand(instruction).split(","):
            expressions = _cc_catalog.MEMORY_RE.findall(operand_part)
            adjusted_displacement = _cc_receiver_instructions._exact_stack_operand_displacement(
                operand_part
            )
            displacements = (
                (adjusted_displacement,)
                if adjusted_displacement is not None and len(expressions) == 1
                else tuple(
                    _cc_receiver_instructions._exact_stack_expression_displacement(expression)
                    for expression in expressions
                )
            )
            for expression, displacement in zip(expressions, displacements):
                if displacement is None:
                    if re.search(
                        r"\besp\b", expression, flags=re.IGNORECASE
                    ):
                        persistent_unsafe_slots.update(candidate_slots)
                    continue
                for slot in candidate_slots:
                    if abs(displacement - slot) < 4 and exact_slot != slot:
                        if _cc_cfg._instruction_mnemonic(instruction) == "lea":
                            persistent_unsafe_slots.add(slot)
                        else:
                            killed_slots_by_index.setdefault(
                                instruction_index, set()
                            ).add(slot)

    state = _CursorFlowState(
        {
            **_cc_cfg._empty_register_state(),
            "ecx": "this",
            "esp": "stack",
            "ebp": "frame",
        },
        {},
        {},
    )
    addresses = _cc_cfg._instruction_runtime_addresses(
        instructions, source=source, caller_start=caller_start
    )
    address_counts: dict[int, int] = {}
    for address in addresses:
        if address is not None and caller_start <= address < caller_end:
            address_counts[address] = address_counts.get(address, 0) + 1
    index_by_address = {
        address: index
        for index, address in enumerate(addresses)
        if address is not None and address_counts.get(address) == 1
    }
    branches: list[tuple[int, int, str]] = []
    for index, instruction in enumerate(instructions):
        branch = _cc_cfg._exact_local_direct_branch(
            instruction,
            instruction_index=index,
            instruction_addresses=addresses,
            instruction_index_by_address=index_by_address,
            source=source,
            caller_start=caller_start,
            caller_end=caller_end,
        )
        if branch is not None:
            branches.append((index, branch[1], branch[0]))
    exact_branch_indices = {index for index, _target, _kind in branches}
    if any(
        (
            (
                _cc_cfg._instruction_mnemonic(instruction).startswith("j")
                or _cc_cfg._instruction_mnemonic(instruction)
                in {"loop", "loope", "loopne", "jecxz"}
            )
            and index not in exact_branch_indices
        )
        or (
            _cc_cfg._instruction_mnemonic(instruction) in {"ret", "retn"}
            and not _cc_cfg._exact_return_terminates(instruction)
        )
        for index, instruction in enumerate(instructions)
    ):
        return {}

    incoming: dict[int, list[_CursorFlowState]] = {}
    fallthrough_live = True
    entry_states: dict[int, _CursorFlowState] = {}
    backedge_states: list[tuple[int, int, _CursorFlowState]] = []
    substitutions: dict[int, tuple[int, _AddressCursor]] = {}
    for index, instruction in enumerate(instructions):
        if index in incoming:
            predecessors = incoming.pop(index)
            if fallthrough_live:
                predecessors.append(_copy_cursor_flow_state(state))
            state = _merge_cursor_flow_states(predecessors)
            fallthrough_live = True
        elif not fallthrough_live:
            state = _CursorFlowState(_cc_cfg._empty_register_state(), {}, {})
        for killed_slot in killed_slots_by_index.get(index, set()):
            state.cursor_slots.pop(killed_slot, None)
        entry_states[index] = _copy_cursor_flow_state(state)

        branch = next(
            (
                (target, kind)
                for branch_index, target, kind in branches
                if branch_index == index
            ),
            None,
        )
        if branch is not None and fallthrough_live:
            target, kind = branch
            if target > index:
                incoming.setdefault(target, []).append(
                    _copy_cursor_flow_state(state)
                )
            else:
                backedge_states.append(
                    (index, target, _copy_cursor_flow_state(state))
                )

        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        if mnemonic in {"call", "jmp"}:
            if mnemonic == "call":
                for volatile in ("eax", "ecx", "edx"):
                    state.registers[volatile] = ""
                    state.cursor_registers.pop(volatile, None)
            elif branch is not None and branch[1] == "unconditional":
                fallthrough_live = False
            continue

        before_cursor_registers = dict(state.cursor_registers)
        load = _cc_receiver_instructions._exact_stack_slot_load(instruction)
        store = _cc_receiver_instructions._exact_stack_slot_store(instruction)
        register_move = _cc_receiver_instructions._exact_register_move(instruction)
        register_add = _cc_receiver_instructions._exact_register_add_immediate(instruction)
        register_lea = _cc_receiver_instructions._exact_register_lea(instruction)
        _cc_receiver_storage._update_register_state(
            instruction,
            state.registers,
            assembly_source=source,
            indexes=indexes,
            reviewed_register_storage_bridges=(
                reviewed_register_storage_bridges
            ),
            reviewed_instruction_address=(
                _cc_cfg._reviewed_cod_register_definition_address(
                    instructions,
                    instruction_index=index,
                    instruction_addresses=addresses,
                    instruction_address_counts=address_counts,
                    caller_start=caller_start,
                    caller_end=caller_end,
                    reviewed_absolute_storage_load_bridges={},
                    reviewed_member_vptr_load_bridges={},
                    reviewed_iat_register_definition_offsets=(
                        reviewed_iat_register_definition_offsets
                    ),
                )
                if source == "cod"
                else None
            ),
            candidate_exact_iat_register_load_proofs=(
                candidate_exact_iat_register_load_proofs
            ),
            consumed_candidate_exact_iat_register_load_proofs=set(),
        )
        for written in _cc_instructions.written_registers(instruction):
            state.cursor_registers.pop(written, None)
        if register_move is not None:
            destination, source_register = register_move
            cursor = before_cursor_registers.get(source_register)
            if cursor is not None:
                state.cursor_registers[destination] = cursor
        elif register_add is not None:
            destination, delta = register_add
            cursor = before_cursor_registers.get(destination)
            if cursor is not None:
                state.cursor_registers[destination] = replace(
                    cursor, delta=cursor.delta + delta
                )
                state.registers[destination] = _render_address_cursor(
                    state.cursor_registers[destination]
                )
        elif register_lea is not None:
            destination, base, delta = register_lea
            cursor = before_cursor_registers.get(base)
            if cursor is not None:
                state.cursor_registers[destination] = replace(
                    cursor, delta=cursor.delta + delta
                )
            else:
                seed = _address_cursor_seed(
                    state.registers.get(destination, "")
                )
                if seed is not None:
                    state.cursor_registers[destination] = seed
        elif load is not None:
            destination, slot = load
            cursor = state.cursor_slots.get(slot)
            if (
                slot not in persistent_unsafe_slots
                and state.registers.get("esp") == "stack"
                and cursor is not None
            ):
                state.cursor_registers[destination] = cursor
                state.registers[destination] = _render_address_cursor(cursor)
                substitutions[index] = (slot, cursor)
        if store is not None:
            slot, source_register = store
            cursor = before_cursor_registers.get(source_register)
            if (
                slot in persistent_unsafe_slots
                or state.registers.get("esp") != "stack"
            ):
                state.cursor_slots.pop(slot, None)
            elif cursor is None:
                state.cursor_slots.pop(slot, None)
            else:
                state.cursor_slots[slot] = cursor
        if _cc_cfg._exact_return_terminates(instruction):
            fallthrough_live = False

    valid_slots = set(candidate_slots) - persistent_unsafe_slots
    for source_index, target_index, tail in backedge_states:
        header = entry_states.get(target_index)
        if header is None:
            valid_slots.clear()
            break
        if any(
            branch_source < target_index
            and target_index < branch_target <= source_index
            for branch_source, branch_target, _kind in branches
        ):
            valid_slots.clear()
            break
        for slot in tuple(valid_slots):
            if not any(
                mapped_slot == slot and target_index <= index <= source_index
                for index, (mapped_slot, _cursor) in substitutions.items()
            ):
                continue
            head_cursor = header.cursor_slots.get(slot)
            tail_cursor = tail.cursor_slots.get(slot)
            if (
                head_cursor is None
                or tail_cursor is None
                or head_cursor.anchor != tail_cursor.anchor
                or not 0 <= tail_cursor.delta - head_cursor.delta <= 0x10000
                or (tail_cursor.delta - head_cursor.delta) % 4
            ):
                valid_slots.discard(slot)
                continue
            for register, head_register_cursor in (
                header.cursor_registers.items()
            ):
                if head_register_cursor.anchor != head_cursor.anchor:
                    continue
                tail_register_cursor = tail.cursor_registers.get(register)
                if tail_register_cursor is None:
                    continue
                if (
                    tail_register_cursor.anchor
                    != head_register_cursor.anchor
                    or not 0
                    <= tail_register_cursor.delta - head_register_cursor.delta
                    <= 0x10000
                    or (
                        tail_register_cursor.delta
                        - head_register_cursor.delta
                    )
                    % 4
                ):
                    valid_slots.discard(slot)
                    break
    return {
        index: _render_address_cursor(cursor)
        for index, (slot, cursor) in substitutions.items()
        if slot in valid_slots
    }


def _is_exact_self_xor_zero(
    instruction: Instruction,
    *,
    destination: str,
) -> bool:
    """Require one unprefixed encoded ``XOR r32, r32`` zero idiom."""
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return False
    if len(body) != 2 or body[0] not in {0x31, 0x33}:
        return False
    modrm = body[1]
    if modrm >> 6 != 3:
        return False
    registers = (
        "eax",
        "ecx",
        "edx",
        "ebx",
        "esp",
        "ebp",
        "esi",
        "edi",
    )
    register = registers[modrm & 0x07]
    return (
        registers[(modrm >> 3) & 0x07] == register
        and destination == register
    )


def _exact_test_same_register(instruction: Instruction) -> str | None:
    """Return the register for one exact unprefixed ``TEST r32, r32``.

    This recognizes only the VC5 null-check form whose taken JE/JZ edge can
    refine a proven call result to ``null``.  Both rendered operands and the
    ModRM encoding must name the same register.
    """
    if _cc_cfg._instruction_mnemonic(instruction) != "test":
        return None
    operands = [
        item.strip().lower()
        for item in _cc_cfg._instruction_operand(instruction).split(",")
    ]
    if len(operands) != 2 or operands[0] != operands[1]:
        return None
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    if len(body) != 2 or body[0] != 0x85:
        return None
    modrm = body[1]
    if modrm >> 6 != 3:
        return None
    registers = (
        "eax",
        "ecx",
        "edx",
        "ebx",
        "esp",
        "ebp",
        "esi",
        "edi",
    )
    register = registers[modrm & 0x07]
    if registers[(modrm >> 3) & 0x07] != register:
        return None
    return register if operands[0] == register else None


def _exact_cmp_register_pair(
    instruction: Instruction,
) -> tuple[str, str] | None:
    """Return one exact unprefixed ``CMP r32, r32`` operand pair.

    VC5 also spells null checks as ``cmp value, zero_register`` after an
    independently proved self-XOR.  The caller's flow state decides whether
    either encoded operand is actually null; this helper proves only the
    rendered and ModRM register pair.
    """

    if _cc_cfg._instruction_mnemonic(instruction) != "cmp":
        return None
    operands = tuple(
        item.strip().lower()
        for item in _cc_cfg._instruction_operand(instruction).split(",")
    )
    if len(operands) != 2:
        return None
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    if len(body) != 2 or body[0] not in {0x39, 0x3B}:
        return None
    modrm = body[1]
    if modrm >> 6 != 3:
        return None
    registers = (
        "eax",
        "ecx",
        "edx",
        "ebx",
        "esp",
        "ebp",
        "esi",
        "edi",
    )
    reg = registers[(modrm >> 3) & 0x07]
    rm = registers[modrm & 0x07]
    encoded = (rm, reg) if body[0] == 0x39 else (reg, rm)
    return encoded if operands == encoded else None
