"""Recoil call-contract receiver saved this evidence and checks."""

from __future__ import annotations

import re
from collections import Counter
from typing import Iterator, Mapping, Sequence

from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import receiver_candidate as _cc_receiver_candidate
from _recoil.call_contract import receiver_instructions as _cc_receiver_instructions
from _recoil.call_contract import receiver_retail as _cc_receiver_retail
from _recoil.call_contract import targets as _cc_targets
from _recoil.commands.asm_verify import Instruction


def _exact_candidate_cfg_saved_entry_this_alias_lifetime(
    instructions: Sequence[Instruction],
    *,
    invocation_indices: Sequence[int],
    addresses: Sequence[int | None],
    caller_start: int,
    caller_end: int,
    local_control_flow_indices: frozenset[int],
    local_control_flow_targets: Mapping[int, tuple[int, ...]],
) -> tuple[str, int, frozenset[int]] | None:
    """Prove one saved entry-this alias across an exact acyclic CFG."""

    if (
        not instructions
        or not invocation_indices
        or len(addresses) != len(instructions)
    ):
        return None
    counts: dict[int, int] = {}
    for address in addresses:
        if address is not None:
            counts[address] = counts.get(address, 0) + 1
    index_by_address = {
        address: index
        for index, address in enumerate(addresses)
        if address is not None and counts.get(address) == 1
    }
    successors, unresolved = _cc_cfg._exact_invocation_cfg(
        instructions,
        instruction_addresses=addresses,
        instruction_index_by_address=index_by_address,
        source="cod",
        caller_start=caller_start,
        caller_end=caller_end,
        local_control_flow_indices=local_control_flow_indices,
        local_control_flow_targets=local_control_flow_targets,
    )

    def alias_escapes(instruction: Instruction, alias: str) -> bool:
        move = _cc_receiver_instructions._exact_register_move(instruction)
        if move is not None and move[1] == alias:
            return True
        if _cc_receiver_instructions._exact_register_stack_transfer(instruction) == ("push", alias):
            return True
        return (
            _cc_cfg._instruction_mnemonic(instruction) == "mov"
            and re.search(
                rf",\s*{re.escape(alias)}\s*$",
                instruction.raw_text.strip(),
                flags=re.IGNORECASE,
            )
            is not None
            and move is None
        )

    aliases: list[tuple[str, int, frozenset[int]]] = []
    for alias in ("ebx", "ebp", "esi", "edi"):
        assignments = [
            index
            for index, instruction in enumerate(instructions)
            if _cc_receiver_instructions._exact_register_move(instruction) == (alias, "ecx")
        ]
        saves = [
            index
            for index, instruction in enumerate(instructions)
            if _cc_receiver_instructions._exact_register_stack_transfer(instruction) == ("push", alias)
        ]
        restores = frozenset(
            index
            for index, instruction in enumerate(instructions)
            if _cc_receiver_instructions._exact_register_stack_transfer(instruction) == ("pop", alias)
        )
        if not (
            len(assignments) == len(saves) == 1
            and restores
            and saves[0] < assignments[0] < min(restores)
        ):
            continue
        assignment_index = assignments[0]
        if any(
            _cc_cfg._instruction_mnemonic(instructions[index]).startswith("j")
            or _cc_cfg._instruction_mnemonic(instructions[index]).startswith("loop")
            or _cc_cfg._instruction_mnemonic(instructions[index]) in {"call", "jmp"}
            or _cc_cfg._instruction_may_clobber_register(instructions[index], "ecx")
            for index in range(assignment_index)
            if index != saves[0]
        ):
            continue
        reachable = _cc_cfg._reachable_cfg_indices(successors, (assignment_index,))
        if unresolved & reachable or not restores <= reachable:
            continue

        # Keep the fail-closed three-colour cycle proof iterative.  Player's
        # current candidate has a long prefix before a late directed cycle;
        # recursive generator frames exhaust Python's host recursion limit
        # before reaching that gray edge.  Host recursion depth is not CFG
        # evidence.  Each frame records the next successor to visit so a gray
        # edge still rejects exactly the same cycles as recursive DFS.
        color: dict[int, int] = {}
        cycle_found = False
        if assignment_index in reachable:
            color[assignment_index] = 1
            stack: list[tuple[int, Iterator[int]]] = [
                (
                    assignment_index,
                    iter(successors.get(assignment_index, ())),
                )
            ]
            while stack and not cycle_found:
                index, targets = stack[-1]
                try:
                    target = next(targets)
                except StopIteration:
                    color[index] = 2
                    stack.pop()
                    continue
                if target not in reachable:
                    continue
                state = color.get(target, 0)
                if state == 1:
                    cycle_found = True
                    break
                if state == 2:
                    continue
                color[target] = 1
                stack.append((target, iter(successors.get(target, ()))))

        if cycle_found:
            continue

        predecessors: dict[int, set[int]] = {
            index: set() for index in range(len(instructions))
        }
        for source_index, targets in successors.items():
            for target_index in targets:
                predecessors.setdefault(target_index, set()).add(
                    source_index
                )
        terminal_repurposes: list[int] = []
        for index, instruction in enumerate(instructions):
            if _cc_receiver_instructions._exact_register_move(instruction) != (alias, "eax"):
                continue
            if (
                index <= 0
                or index not in reachable
                or predecessors.get(index) != {index - 1}
            ):
                continue
            producer = instructions[index - 1]
            try:
                producer_body = bytes(
                    int(item, 16) for item in producer.bytes
                )
            except (TypeError, ValueError):
                continue
            if (
                index - 1 in invocation_indices
                and _cc_cfg._instruction_mnemonic(producer) == "call"
                and len(producer_body) == 5
                and producer_body[0] == 0xE8
                and _cc_cfg._exact_invocation_encoding(producer, mnemonic="call")
            ):
                terminal_repurposes.append(index)
        if len(terminal_repurposes) > 1:
            continue
        terminal_repurpose = (
            terminal_repurposes[0] if terminal_repurposes else None
        )

        # Phase 0 is the exact entry-this alias.  A path may either restore it
        # directly, or retire that root at one exact direct-CALL-result move
        # before later using the register for unrelated data.  Phase 1 is that
        # unrelated value and phase 2 begins at the matching POP.  The
        # returned lifetime endpoints include the repurpose, so no later field
        # load can inherit the entry-this marker.
        work: list[tuple[int, int]] = [
            (target, 0)
            for target in successors.get(assignment_index, ())
        ]
        visited: set[tuple[int, int]] = set()
        reached_invocations: set[int] = set()
        reached_returns: set[int] = set()
        valid = True
        while work and valid:
            index, phase = work.pop()
            state = (index, phase)
            if state in visited:
                continue
            visited.add(state)
            instruction = instructions[index]
            if index in unresolved:
                valid = False
                break
            if _cc_cfg._exact_return_terminates(instruction):
                if phase != 2:
                    valid = False
                else:
                    reached_returns.add(index)
                continue
            if _cc_cfg._instruction_mnemonic(instruction) in {"ret", "retn"}:
                valid = False
                break
            if index in restores:
                if phase not in {0, 1}:
                    valid = False
                    break
                next_phase = 2
            elif index == terminal_repurpose:
                if phase != 0:
                    valid = False
                    break
                next_phase = 1
            elif phase == 0:
                if (
                    _cc_cfg._instruction_may_clobber_register(instruction, alias)
                    or alias_escapes(instruction, alias)
                ):
                    valid = False
                    break
                next_phase = 0
            elif phase == 1:
                if (
                    _cc_receiver_instructions._exact_register_stack_transfer(instruction)
                    == ("push", alias)
                    or _cc_receiver_instructions._exact_register_move(instruction) == (alias, "ecx")
                ):
                    valid = False
                    break
                next_phase = 1
            else:
                if _cc_receiver_retail._dead_saved_alias_is_referenced_or_redefined(
                    instruction,
                    alias,
                ):
                    valid = False
                    break
                next_phase = 2
            if index in invocation_indices:
                reached_invocations.add(index)
            targets = successors.get(index, ())
            if not targets:
                valid = False
                break
            work.extend((target, next_phase) for target in targets)
        if (
            valid
            and reached_returns
            and reached_invocations == set(invocation_indices)
            and all(
                any(state[0] == restore for state in visited)
                for restore in restores
            )
        ):
            lifetime_endpoints = set(restores)
            if terminal_repurpose is not None:
                lifetime_endpoints.add(terminal_repurpose)
            aliases.append(
                (
                    alias,
                    assignment_index,
                    frozenset(lifetime_endpoints),
                )
            )
    return aliases[0] if len(aliases) == 1 else None


def _exact_candidate_cfg_single_saved_entry_this_field_vptr_proofs(
    instructions: Sequence[Instruction],
    *,
    invocation_indices: Sequence[int],
    addresses: Sequence[int | None],
    caller_start: int,
    caller_end: int,
    local_control_flow_indices: frozenset[int],
    local_control_flow_targets: Mapping[int, tuple[int, ...]],
) -> dict[int, str]:
    """Prove exactly one field-vptr dispatch from a saved entry-this root."""

    lifetime = _exact_candidate_cfg_saved_entry_this_alias_lifetime(
        instructions,
        invocation_indices=invocation_indices,
        addresses=addresses,
        caller_start=caller_start,
        caller_end=caller_end,
        local_control_flow_indices=local_control_flow_indices,
        local_control_flow_targets=local_control_flow_targets,
    )
    if lifetime is None:
        return {}
    alias, assignment_index, lifetime_endpoints = lifetime
    indirect_invocations = tuple(
        index
        for index in invocation_indices
        if _cc_targets._exact_indirect_register_call_slot(
            instructions[index], allow_zero=True
        )
        is not None
    )
    if len(indirect_invocations) != 1:
        return {}
    counts: dict[int, int] = {}
    for address in addresses:
        if address is not None:
            counts[address] = counts.get(address, 0) + 1
    index_by_address = {
        address: index
        for index, address in enumerate(addresses)
        if address is not None and counts.get(address) == 1
    }
    successors, unresolved = _cc_cfg._exact_invocation_cfg(
        instructions,
        instruction_addresses=addresses,
        instruction_index_by_address=index_by_address,
        source="cod",
        caller_start=caller_start,
        caller_end=caller_end,
        local_control_flow_indices=local_control_flow_indices,
        local_control_flow_targets=local_control_flow_targets,
    )
    reachable_from_assignment = _cc_cfg._reachable_cfg_indices(
        successors, (assignment_index,)
    )
    if unresolved & reachable_from_assignment:
        return {}
    reachable_after_lifetime = set().union(
        *(
            _cc_cfg._reachable_cfg_indices(successors, (endpoint,))
            for endpoint in lifetime_endpoints
        )
    )

    candidates: dict[int, int] = {}
    for call_index in indirect_invocations:
        if (
            call_index not in reachable_from_assignment
            or call_index in reachable_after_lifetime
        ):
            continue
        exact_slot = _cc_targets._exact_indirect_register_call_slot(
            instructions[call_index]
        )
        if exact_slot is None or exact_slot[1] < 0:
            continue
        vptr_register, _slot = exact_slot
        prior_invocation_boundary = max(
            (
                index + 1
                for index in invocation_indices
                if index < call_index
            ),
            default=assignment_index + 1,
        )
        vptr_rows = [
            index
            for index in range(prior_invocation_boundary, call_index)
            if _cc_receiver_candidate._exact_register_memory_load(instructions[index])
            == (vptr_register, "ecx", 0)
        ]
        receiver_rows = [
            (index, load[2])
            for index in range(prior_invocation_boundary, call_index)
            if (load := _cc_receiver_candidate._exact_register_memory_load(instructions[index]))
            is not None
            and load[0] == "ecx"
            and load[1] == alias
            and load[2] > 0
        ]
        if len(vptr_rows) != 1 or len(receiver_rows) != 1:
            continue
        vptr_index = vptr_rows[0]
        receiver_index, field_displacement = receiver_rows[0]
        if not (
            assignment_index < receiver_index < vptr_index < call_index
        ):
            continue
        if any(
            _cc_cfg._instruction_mnemonic(instructions[index]).startswith("j")
            or _cc_cfg._instruction_mnemonic(instructions[index]).startswith("loop")
            or _cc_cfg._instruction_mnemonic(instructions[index]) in {"call", "jmp"}
            or (
                index != receiver_index
                and _cc_cfg._instruction_may_clobber_register(
                    instructions[index], "ecx"
                )
            )
            or (
                index != vptr_index
                and _cc_cfg._instruction_may_clobber_register(
                    instructions[index], vptr_register
                )
            )
            for index in range(receiver_index, call_index)
        ):
            continue
        candidates[call_index] = field_displacement
    if len(candidates) != 1:
        return {}
    call_index, field_displacement = next(iter(candidates.items()))
    return {
        call_index: (
            "load(exact-receiver-field("
            f"this,+0x{field_displacement:x}))"
        )
    }


def _exact_candidate_zsnd_a3d_entry_this_receiver_field_vptr_proofs(
    instructions: Sequence[Instruction],
    *,
    invocation_indices: Sequence[int],
    addresses: Sequence[int | None],
    caller_start: int,
    caller_end: int,
    local_control_flow_indices: frozenset[int],
    local_control_flow_targets: Mapping[int, tuple[int, ...]],
) -> dict[int, str]:
    """Prove the exact zSnd backend entry-ECX receiver/argument seams.

    VC5 keeps entry ``this`` in ECX until it loads the backend receiver field
    at +0x4c.  The following argument pushes obscure that direct lineage in
    the generic expression model even though candidate and retail instruction
    sequences are identical.  Each authority row stays finite to one reviewed
    caller, extent, invocation ordinal, slot, and complete byte sequence.
    """

    # end, invocation ordinal, receiver-row ordinal, vptr-row ordinal, call
    # slot, exact rows.
    specs: Mapping[
        int,
        tuple[int, int, int, int, int, tuple[tuple[int, bytes], ...]],
    ] = {
        0x4A34E0: (
            0x4A3590,
            0,
            4,
            7,
            0x2C,
            (
                (0x1B, bytes.fromhex("8b 74 24 18")),
                (0x1F, bytes.fromhex("6a 00")),
                (0x21, bytes.fromhex("56")),
                (0x22, bytes.fromhex("8b 74 24 18")),
                (0x26, bytes.fromhex("8b 49 4c")),
                (0x29, bytes.fromhex("56")),
                (0x2A, bytes.fromhex("8b 74 24 20")),
                (0x2E, bytes.fromhex("8b 01")),
                (0x30, bytes.fromhex("56")),
                (0x31, bytes.fromhex("8b 74 24 1c")),
                (0x35, bytes.fromhex("56")),
                (0x36, bytes.fromhex("8b 74 24 1c")),
                (0x3A, bytes.fromhex("56")),
                (0x3B, bytes.fromhex("52")),
                (0x3C, bytes.fromhex("51")),
                (0x3D, bytes.fromhex("ff 50 2c")),
            ),
        ),
        0x4A3590: (
            0x4A3620,
            0,
            1,
            4,
            0x30,
            (
                (0x1B, bytes.fromhex("8b 74 24 10")),
                (0x1F, bytes.fromhex("8b 49 4c")),
                (0x22, bytes.fromhex("56")),
                (0x23, bytes.fromhex("8b 74 24 10")),
                (0x27, bytes.fromhex("8b 01")),
                (0x29, bytes.fromhex("56")),
                (0x2A, bytes.fromhex("8b 74 24 10")),
                (0x2E, bytes.fromhex("56")),
                (0x2F, bytes.fromhex("52")),
                (0x30, bytes.fromhex("51")),
                (0x31, bytes.fromhex("ff 50 30")),
            ),
        ),
        0x4A3620: (
            0x4A3690,
            1,
            0,
            4,
            0x10,
            (
                (0x41, bytes.fromhex("8b 49 4c")),
                (0x44, bytes.fromhex("8d 54 24 08")),
                (0x48, bytes.fromhex("52")),
                (0x49, bytes.fromhex("8d 54 24 08")),
                (0x4D, bytes.fromhex("8b 01")),
                (0x4F, bytes.fromhex("52")),
                (0x50, bytes.fromhex("51")),
                (0x51, bytes.fromhex("ff 50 10")),
            ),
        ),
    }
    spec = specs.get(caller_start)
    if (
        spec is None
        or caller_end != spec[0]
        or len(addresses) != len(instructions)
        or not instructions
        or addresses[0] != caller_start
    ):
        return {}
    (
        _end,
        invocation_ordinal,
        receiver_row,
        vptr_row,
        call_slot,
        expected_rows,
    ) = spec
    counts = Counter(address for address in addresses if address is not None)
    index_by_address = {
        address: index
        for index, address in enumerate(addresses)
        if address is not None and counts[address] == 1
    }
    row_indices: list[int] = []
    for offset, expected_body in expected_rows:
        index = index_by_address.get(caller_start + offset)
        if index is None:
            return {}
        try:
            body = bytes(int(item, 16) for item in instructions[index].bytes)
        except (TypeError, ValueError):
            return {}
        if body != expected_body:
            return {}
        row_indices.append(index)
    if row_indices != list(range(row_indices[0], row_indices[-1] + 1)):
        return {}

    receiver_index = row_indices[receiver_row]
    vptr_index = row_indices[vptr_row]
    call_index = row_indices[-1]
    if (
        len(invocation_indices) <= invocation_ordinal
        or invocation_indices[invocation_ordinal] != call_index
        or _cc_receiver_candidate._exact_register_memory_load(instructions[receiver_index])
        != ("ecx", "ecx", 0x4C)
        or _cc_receiver_candidate._exact_register_memory_load(instructions[vptr_index])
        != ("eax", "ecx", 0)
        or _cc_targets._exact_indirect_register_call_slot(instructions[call_index])
        != ("eax", call_slot)
    ):
        return {}

    # Entry ECX must reach the field load without a call or another ECX
    # definition on any path that can actually reach the governed receiver.
    # The live VC5 body has an earlier conditional exit, so flat rejection of
    # every branch disconnects this proof from the active extractor.  Bind the
    # check to the same resolved candidate CFG used by all target/slice/phase
    # extraction modes and ignore only branches that cannot reach the load.
    counts = Counter(address for address in addresses if address is not None)
    instruction_index_by_address = {
        address: index
        for index, address in enumerate(addresses)
        if address is not None and counts[address] == 1
    }
    successors, unresolved = _cc_cfg._exact_invocation_cfg(
        instructions,
        instruction_addresses=addresses,
        instruction_index_by_address=instruction_index_by_address,
        source="cod",
        caller_start=caller_start,
        caller_end=caller_end,
        local_control_flow_indices=local_control_flow_indices,
        local_control_flow_targets=local_control_flow_targets,
    )
    predecessors: dict[int, set[int]] = {
        index: set() for index in range(len(instructions))
    }
    for predecessor, targets in successors.items():
        for target in targets:
            predecessors.setdefault(target, set()).add(predecessor)
    can_reach_receiver = {receiver_index}
    pending = [receiver_index]
    while pending:
        current = pending.pop()
        for predecessor in predecessors.get(current, ()):
            if predecessor not in can_reach_receiver:
                can_reach_receiver.add(predecessor)
                pending.append(predecessor)
    if (
        0 not in can_reach_receiver
        or unresolved & can_reach_receiver
        or any(
            _cc_cfg._instruction_mnemonic(instructions[index]) in {"call", "jmp"}
            or _cc_cfg._instruction_may_clobber_register(
                instructions[index], "ecx"
            )
            for index in can_reach_receiver
            if index < receiver_index
        )
    ):
        return {}
    return {
        call_index: "load(exact-receiver-field(this,+0x4c))"
    }
