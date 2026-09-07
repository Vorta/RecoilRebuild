"""Recoil call-contract receiver fields evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract import listing as _cc_listing
from _recoil.call_contract import receiver_candidate as _cc_receiver_candidate
from _recoil.call_contract import receiver_instructions as _cc_receiver_instructions
from _recoil.call_contract import receiver_retail as _cc_receiver_retail
from _recoil.call_contract import receiver_saved_this as _cc_receiver_saved_this
from _recoil.call_contract import receiver_storage as _cc_receiver_storage
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import IdentityIndexes

import re
import struct
from collections import Counter
from typing import Mapping, NoReturn, Sequence

from _recoil.commands.asm_verify import Instruction
from _recoil.lib.binja import BinaryNinjaBridge, BridgeError
from _recoil.lib.progress import address_value, normalize_address


def _exact_candidate_cfg_repeated_saved_entry_this_field_vptr_proofs(
    instructions: Sequence[Instruction],
    *,
    invocation_indices: Sequence[int],
    addresses: Sequence[int | None],
    caller_start: int,
    caller_end: int,
    local_control_flow_indices: frozenset[int],
    local_control_flow_targets: Mapping[int, tuple[int, ...]],
) -> dict[int, str]:
    """Prove repeated direct field dispatches on a saved entry-this root.

    The complete caller may contain other indirect dispatches reached through
    objects loaded from the governed field.  Only the exact alias-field -> ECX
    -> vptr -> CALL rows receive the field token; a nested receiver remains an
    ordinary load lineage.  Each branch is bounded by its own proven lifetime
    endpoint rather than by the earliest POP in another branch.
    """

    lifetime = _cc_receiver_saved_this._exact_candidate_cfg_saved_entry_this_alias_lifetime(
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
    reachable_after_lifetime: set[int] = set()
    for endpoint in lifetime_endpoints:
        reachable_after_lifetime.update(
            _cc_cfg._reachable_cfg_indices(successors, (endpoint,))
        )

    indirect_invocations = tuple(
        index
        for index in invocation_indices
        if _cc_targets._exact_indirect_register_call_slot(
            instructions[index], allow_zero=True
        )
        is not None
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

    # Repetition is the disambiguating evidence for this sibling proof.  The
    # single-dispatch grammar remains intentionally strict, and every direct
    # candidate in this caller must agree on the exact field coordinate.
    if len(candidates) < 3 or len(set(candidates.values())) != 1:
        return {}
    field_displacement = next(iter(candidates.values()))
    marker = (
        "load(exact-receiver-field("
        f"this,+0x{field_displacement:x}))"
    )
    return {index: marker for index in candidates}


def _exact_direct_saved_member_receiver_seam(
    instructions: Sequence[Instruction],
    *,
    invocation_indices: Sequence[int],
    call_index: int,
    assignment_index: int,
    receiver_alias: str,
    vptr_register: str,
) -> tuple[int, int] | None:
    """Prove one saved member receiver across a direct result call.

    VC5 may retain a rebound member receiver and its nonvolatile vptr across
    one direct helper call, then push the helper result followed by the
    receiver before the virtual dispatch.  This predicate proves only that
    exact byte/render seam; the caller-wide CFG and receiver lifetime remain
    the responsibility of the enclosing saved-entry-this proof.
    """

    if (
        receiver_alias not in {"ebx", "ebp", "esi", "edi"}
        or vptr_register not in {"ebx", "ebp", "esi", "edi"}
        or receiver_alias == vptr_register
        or call_index < 3
    ):
        return None
    direct_index = call_index - 3
    if (
        direct_index not in invocation_indices
        or max(
            (index for index in invocation_indices if index < call_index),
            default=-1,
        )
        != direct_index
        or _cc_receiver_instructions._exact_register_stack_transfer(instructions[call_index - 2])
        != ("push", "eax")
        or _cc_receiver_instructions._exact_register_stack_transfer(instructions[call_index - 1])
        != ("push", receiver_alias)
    ):
        return None
    direct_call = instructions[direct_index]
    try:
        direct_body = bytes(int(item, 16) for item in direct_call.bytes)
    except (TypeError, ValueError):
        return None
    if not (
        _cc_cfg._instruction_mnemonic(direct_call) == "call"
        and len(direct_body) == 5
        and direct_body[0] == 0xE8
        and _cc_cfg._exact_invocation_encoding(direct_call, mnemonic="call")
    ):
        return None

    prior_boundary = max(
        (
            index + 1
            for index in invocation_indices
            if index < direct_index
        ),
        default=assignment_index + 1,
    )
    vptr_loads = [
        index
        for index in range(prior_boundary, direct_index)
        if _cc_receiver_candidate._exact_register_memory_load(instructions[index])
        == (vptr_register, receiver_alias, 0)
    ]
    vptr_saves = [
        index
        for index, instruction in enumerate(instructions)
        if _cc_receiver_instructions._exact_register_stack_transfer(instruction)
        == ("push", vptr_register)
    ]
    vptr_restores = [
        index
        for index, instruction in enumerate(instructions)
        if _cc_receiver_instructions._exact_register_stack_transfer(instruction)
        == ("pop", vptr_register)
    ]
    if not (
        len(vptr_loads) == len(vptr_saves) == 1
        and vptr_saves[0] < assignment_index < vptr_loads[0]
        and vptr_restores
        and any(index > call_index for index in vptr_restores)
    ):
        return None
    vptr_index = vptr_loads[0]
    for index in range(vptr_index + 1, call_index):
        instruction = instructions[index]
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        move = _cc_receiver_instructions._exact_register_move(instruction)
        if (
            (mnemonic.startswith("j") or mnemonic.startswith("loop"))
            or (mnemonic in {"call", "jmp"} and index != direct_index)
            or (
                index != direct_index
                and _cc_cfg._instruction_may_clobber_register(
                    instruction, vptr_register
                )
            )
            or (
                move is not None
                and move[1] == vptr_register
            )
            or _cc_receiver_instructions._exact_register_stack_transfer(instruction)
            == ("push", vptr_register)
            or (
                mnemonic == "mov"
                and re.search(
                    rf",\s*{re.escape(vptr_register)}\s*$",
                    instruction.raw_text.strip(),
                    flags=re.IGNORECASE,
                )
                is not None
                and move is None
            )
        ):
            return None
    return vptr_index, direct_index


def _exact_candidate_cfg_saved_entry_this_field_rebind_vptr_proofs(
    instructions: Sequence[Instruction],
    *,
    invocation_indices: Sequence[int],
    addresses: Sequence[int | None],
    caller_start: int,
    caller_end: int,
    local_control_flow_indices: frozenset[int],
    local_control_flow_targets: Mapping[int, tuple[int, ...]],
) -> dict[int, str]:
    """Prove an exact saved entry-this alias rebound to one receiver field."""

    if (
        not instructions
        or not invocation_indices
        or len(addresses) != len(instructions)
    ):
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

    packages: list[dict[int, str]] = []
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
        entry_saves = [
            index
            for index in saves
            if len(assignments) == 1 and index < assignments[0]
        ]
        restores = frozenset(
            index
            for index, instruction in enumerate(instructions)
            if _cc_receiver_instructions._exact_register_stack_transfer(instruction) == ("pop", alias)
        )
        field_rebinds = [
            (index, load[2])
            for index, instruction in enumerate(instructions)
            if (load := _cc_receiver_candidate._exact_register_memory_load(instruction)) is not None
            and load[0] == alias
            and load[1] == alias
            and load[2] > 0
        ]
        if not (
            len(assignments) == len(entry_saves) == len(field_rebinds) == 1
            and restores
            and entry_saves[0] < assignments[0] < field_rebinds[0][0]
        ):
            continue
        assignment_index = assignments[0]
        rebind_index, field_displacement = field_rebinds[0]
        if any(
            _cc_cfg._instruction_mnemonic(instructions[index]).startswith("j")
            or _cc_cfg._instruction_mnemonic(instructions[index]).startswith("loop")
            or _cc_cfg._instruction_mnemonic(instructions[index]) in {"call", "jmp"}
            or _cc_cfg._instruction_may_clobber_register(instructions[index], "ecx")
            for index in range(assignment_index)
            if index != entry_saves[0]
        ):
            continue
        reachable = _cc_cfg._reachable_cfg_indices(successors, (assignment_index,))
        if (
            unresolved & reachable
            or rebind_index not in reachable
            or not restores <= reachable
        ):
            continue

        active: set[int] = set()
        finished: set[int] = set()

        def has_cycle(index: int) -> bool:
            if index in active:
                return True
            if index in finished or index not in reachable:
                return False
            active.add(index)
            if any(has_cycle(target) for target in successors.get(index, ())):
                return True
            active.remove(index)
            finished.add(index)
            return False

        if has_cycle(assignment_index):
            continue

        reachable_after_restores: set[int] = set()
        for restore in restores:
            reachable_after_restores.update(
                _cc_cfg._reachable_cfg_indices(successors, (restore,))
            )

        call_candidates: dict[int, tuple[int, int]] = {}
        for call_index in invocation_indices:
            if (
                call_index <= rebind_index
                or call_index not in reachable
                or call_index in reachable_after_restores
            ):
                continue
            call = instructions[call_index]
            exact_slot = _cc_targets._exact_indirect_register_call_slot(call)
            if exact_slot is None or exact_slot[1] <= 0:
                continue
            vptr_register, slot = exact_slot
            prior_invocation_boundary = max(
                (
                    index + 1
                    for index in invocation_indices
                    if index < call_index
                ),
                default=assignment_index + 1,
            )
            receiver_moves = [
                index
                for index in range(prior_invocation_boundary, call_index)
                if _cc_receiver_instructions._exact_register_move(instructions[index])
                == ("ecx", alias)
            ]
            receiver_pushes = [
                index
                for index in range(prior_invocation_boundary, call_index)
                if _cc_receiver_instructions._exact_register_stack_transfer(instructions[index])
                == ("push", alias)
            ]
            vptr_loads = [
                index
                for index in range(prior_invocation_boundary, call_index)
                if _cc_receiver_candidate._exact_register_memory_load(instructions[index])
                == (vptr_register, alias, 0)
            ]
            direct_result_call_index: int | None = None
            if len(vptr_loads) != 1:
                direct_seam = _exact_direct_saved_member_receiver_seam(
                    instructions,
                    invocation_indices=invocation_indices,
                    call_index=call_index,
                    assignment_index=assignment_index,
                    receiver_alias=alias,
                    vptr_register=vptr_register,
                )
                if direct_seam is not None:
                    vptr_loads = [direct_seam[0]]
                    direct_result_call_index = direct_seam[1]
            if len(vptr_loads) != 1:
                continue
            vptr_index = vptr_loads[0]
            conventional_receiver = (
                len(receiver_moves) == 1 and not receiver_pushes
            )
            pushed_com_receiver = (
                not receiver_moves
                and receiver_pushes == [call_index - 1]
                and vptr_index < receiver_pushes[0]
            )
            if not (conventional_receiver or pushed_com_receiver):
                continue
            receiver_index = (
                receiver_moves[0]
                if conventional_receiver
                else receiver_pushes[0]
            )
            if not (
                rebind_index < receiver_index < call_index
                and rebind_index < vptr_index < call_index
            ):
                continue
            local_start = min(receiver_index, vptr_index)
            allowed_ecx_writes = (
                {receiver_index}
                if conventional_receiver
                else ({vptr_index} if vptr_register == "ecx" else set())
            )
            if any(
                _cc_cfg._instruction_mnemonic(instructions[index]).startswith("j")
                or _cc_cfg._instruction_mnemonic(instructions[index]).startswith("loop")
                or (
                    _cc_cfg._instruction_mnemonic(instructions[index])
                    in {"call", "jmp"}
                    and index != direct_result_call_index
                )
                or (
                    index != direct_result_call_index
                    and index not in allowed_ecx_writes
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
                for index in range(local_start, call_index)
            ):
                continue
            call_candidates[call_index] = (receiver_index, slot)
        if not call_candidates:
            continue
        allowed_receiver_moves = {
            receiver_index
            for receiver_index, _slot in call_candidates.values()
        }
        # Receiver uses that do not themselves need the positive-field token
        # still belong to the same proven lifetime.  Accept an exact ECX copy
        # only when its next invocation is either a direct E8 member call or
        # an exact alias-rooted vptr call (including slot zero), with no local
        # control transfer or receiver/vptr clobber in between.
        for receiver_index, instruction in enumerate(instructions):
            if _cc_receiver_instructions._exact_register_move(instruction) != ("ecx", alias):
                continue
            later_invocations = [
                index for index in invocation_indices if index > receiver_index
            ]
            if not later_invocations:
                continue
            call_index = min(later_invocations)
            call = instructions[call_index]
            try:
                call_body = bytes(int(item, 16) for item in call.bytes)
            except (TypeError, ValueError):
                continue
            between = range(receiver_index + 1, call_index)
            if any(
                _cc_cfg._instruction_mnemonic(instructions[index]).startswith("j")
                or _cc_cfg._instruction_mnemonic(instructions[index]).startswith(
                    "loop"
                )
                or _cc_cfg._instruction_mnemonic(instructions[index])
                in {"call", "jmp"}
                or _cc_cfg._instruction_may_clobber_register(
                    instructions[index], "ecx"
                )
                for index in between
            ):
                continue
            if (
                _cc_cfg._instruction_mnemonic(call) == "call"
                and len(call_body) == 5
                and call_body[0] == 0xE8
                and _cc_cfg._exact_invocation_encoding(call, mnemonic="call")
            ):
                allowed_receiver_moves.add(receiver_index)
                continue
            exact_slot = _cc_targets._exact_indirect_register_call_slot(call)
            if (
                exact_slot is None
                and _cc_cfg._instruction_mnemonic(call) == "call"
                and _cc_cfg._exact_invocation_encoding(call, mnemonic="call")
            ):
                expression, zero_slot = _cc_targets._memory_slot(
                    _cc_cfg._instruction_operand(call)
                )
                if (
                    zero_slot == 0
                    and re.fullmatch(
                        r"eax|ecx|edx|ebx|esi|edi|ebp",
                        expression,
                    )
                    is not None
                ):
                    exact_slot = (expression, 0)
            if exact_slot is None:
                continue
            vptr_register, slot = exact_slot
            prior_invocation_boundary = max(
                (
                    index + 1
                    for index in invocation_indices
                    if index < call_index
                ),
                default=assignment_index + 1,
            )
            indirect_window = range(
                prior_invocation_boundary,
                call_index,
            )
            vptr_loads = [
                index
                for index in indirect_window
                if _cc_receiver_candidate._exact_register_memory_load(instructions[index])
                == (vptr_register, alias, 0)
            ]
            if (
                slot >= 0
                and len(vptr_loads) == 1
                and not any(
                    _cc_cfg._instruction_mnemonic(instructions[index]).startswith(
                        "j"
                    )
                    or _cc_cfg._instruction_mnemonic(
                        instructions[index]
                    ).startswith("loop")
                    or _cc_cfg._instruction_mnemonic(instructions[index])
                    in {"call", "jmp"}
                    for index in indirect_window
                )
                and not any(
                    index != vptr_loads[0]
                    and _cc_cfg._instruction_may_clobber_register(
                        instructions[index], vptr_register
                    )
                    for index in indirect_window
                )
            ):
                allowed_receiver_moves.add(receiver_index)

        # A saved nonvolatile register may be deliberately reused after its
        # receiver-field lifetime ends.  Admit only the exact VC5 result-move
        # shape: one EAX-to-alias move with a unique sequential predecessor
        # that is an exact direct E8 CALL.  No governed field-rooted call may
        # be reachable after that move.  This keeps the earlier receiver proof
        # bounded while the later value remains ordinary, unrelated dataflow.
        predecessors: dict[int, set[int]] = {
            index: set() for index in range(len(instructions))
        }
        for source_index, targets in successors.items():
            for target_index in targets:
                predecessors.setdefault(target_index, set()).add(source_index)
        repurpose_indexes: list[int] = []
        for index, instruction in enumerate(instructions):
            if _cc_receiver_instructions._exact_register_move(instruction) != (alias, "eax"):
                continue
            if predecessors.get(index) != {index - 1} or index <= 0:
                continue
            producer = instructions[index - 1]
            try:
                producer_body = bytes(int(item, 16) for item in producer.bytes)
            except (TypeError, ValueError):
                continue
            if not (
                _cc_cfg._instruction_mnemonic(producer) == "call"
                and len(producer_body) == 5
                and producer_body[0] == 0xE8
                and _cc_cfg._exact_invocation_encoding(producer, mnemonic="call")
                and not (
                    _cc_cfg._reachable_cfg_indices(successors, (index,))
                    & set(call_candidates)
                )
            ):
                continue
            repurpose_indexes.append(index)
        # The rebound receiver may also become dead before the epilogue when
        # VC5 reuses its saved register for the vptr of a distinct preserved
        # nonvolatile receiver.  Admit only one exact zero-offset load after
        # every governed call, and only when no governed call is reachable
        # from the reuse.  Subsequent uses remain unrelated ordinary lineage.
        for index, instruction in enumerate(instructions):
            load = _cc_receiver_candidate._exact_register_memory_load(instruction)
            if (
                load is None
                or load[0] != alias
                or load[1] not in {"ebx", "ebp", "esi", "edi"}
                or load[1] == alias
                or load[2] != 0
                or index <= max(call_candidates)
                or (
                    _cc_cfg._reachable_cfg_indices(successors, (index,))
                    & set(call_candidates)
                )
            ):
                continue
            repurpose_indexes.append(index)
        if len(repurpose_indexes) > 1:
            continue
        repurpose_index = (
            repurpose_indexes[0] if repurpose_indexes else None
        )

        # 0: the alias still denotes entry this; 1: it denotes this+field;
        # 2: the receiver lifetime ended at an exact direct-call result move;
        # 3: the matching POP restored the caller value and the alias is dead.
        work: list[tuple[int, int]] = [
            (target, 0)
            for target in successors.get(assignment_index, ())
        ]
        visited: set[tuple[int, int]] = set()
        reached_calls: set[int] = set()
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
                if phase != 3:
                    valid = False
                else:
                    reached_returns.add(index)
                continue
            if _cc_cfg._instruction_mnemonic(instruction) in {"ret", "retn"}:
                valid = False
                break
            if index == rebind_index:
                if phase != 0:
                    valid = False
                    break
                next_phase = 1
            elif index in restores:
                if phase not in {0, 1, 2}:
                    valid = False
                    break
                next_phase = 3
            elif index == repurpose_index:
                if phase not in {0, 1}:
                    valid = False
                    break
                next_phase = 2
            elif phase == 3:
                if _cc_receiver_retail._dead_saved_alias_is_referenced_or_redefined(
                    instruction,
                    alias,
                ):
                    valid = False
                    break
                next_phase = 3
            elif phase == 2:
                if _cc_receiver_instructions._exact_register_stack_transfer(instruction) == (
                    "push",
                    alias,
                ):
                    valid = False
                    break
                next_phase = 2
            else:
                if (
                    _cc_cfg._instruction_may_clobber_register(instruction, alias)
                    or (
                        alias_escapes(instruction, alias)
                        and index not in allowed_receiver_moves
                    )
                ):
                    valid = False
                    break
                next_phase = phase
            if index in call_candidates:
                if phase != 1:
                    valid = False
                    break
                reached_calls.add(index)
            targets = successors.get(index, ())
            if not targets:
                valid = False
                break
            work.extend((target, next_phase) for target in targets)
        if (
            valid
            and reached_returns
            and reached_calls == set(call_candidates)
            and all(
                any(state[0] == restore for state in visited)
                for restore in restores
            )
        ):
            marker = (
                "load(exact-receiver-field("
                f"this,+0x{field_displacement:x}))"
            )
            packages.append(
                {call_index: marker for call_index in call_candidates}
            )
    return packages[0] if len(packages) == 1 else {}


def _exact_candidate_cfg_saved_entry_this_distinct_field_alias_vptr_proofs(
    instructions: Sequence[Instruction],
    *,
    invocation_indices: Sequence[int],
    addresses: Sequence[int | None],
    caller_start: int,
    caller_end: int,
    local_control_flow_indices: frozenset[int],
    local_control_flow_targets: Mapping[int, tuple[int, ...]],
) -> dict[int, str]:
    """Prove an entry-this alias loading one distinct saved field alias."""

    if (
        not instructions
        or not invocation_indices
        or len(addresses) != len(instructions)
    ):
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

    packages: list[dict[int, str]] = []
    nonvolatile = ("ebx", "ebp", "esi", "edi")
    for root_alias in nonvolatile:
        for receiver_alias in nonvolatile:
            if receiver_alias == root_alias:
                continue
            assignments = [
                index
                for index, instruction in enumerate(instructions)
                if _cc_receiver_instructions._exact_register_move(instruction)
                == (root_alias, "ecx")
            ]
            if any(
                _cc_receiver_instructions._exact_register_move(instruction)
                == (receiver_alias, "ecx")
                for instruction in instructions
            ):
                continue
            root_saves = [
                index
                for index, instruction in enumerate(instructions)
                if _cc_receiver_instructions._exact_register_stack_transfer(instruction)
                == ("push", root_alias)
            ]
            receiver_saves = [
                index
                for index, instruction in enumerate(instructions)
                if _cc_receiver_instructions._exact_register_stack_transfer(instruction)
                == ("push", receiver_alias)
            ]
            root_restores = frozenset(
                index
                for index, instruction in enumerate(instructions)
                if _cc_receiver_instructions._exact_register_stack_transfer(instruction)
                == ("pop", root_alias)
            )
            receiver_restores = frozenset(
                index
                for index, instruction in enumerate(instructions)
                if _cc_receiver_instructions._exact_register_stack_transfer(instruction)
                == ("pop", receiver_alias)
            )
            transfers = [
                (index, load[2])
                for index, instruction in enumerate(instructions)
                if (load := _cc_receiver_candidate._exact_register_memory_load(instruction))
                is not None
                and load[0] == receiver_alias
                and load[1] == root_alias
                and load[2] > 0
            ]
            all_restores = root_restores | receiver_restores
            if not (
                len(assignments)
                == len(root_saves)
                == len(receiver_saves)
                == len(transfers)
                == 1
                and root_restores
                and receiver_restores
                and max(root_saves[0], receiver_saves[0]) < assignments[0]
                and assignments[0] < transfers[0][0] < min(all_restores)
            ):
                continue
            assignment_index = assignments[0]
            transfer_index, field_displacement = transfers[0]
            if any(
                _cc_cfg._instruction_mnemonic(instructions[index]).startswith("j")
                or _cc_cfg._instruction_mnemonic(instructions[index]).startswith("loop")
                or _cc_cfg._instruction_mnemonic(instructions[index])
                in {"call", "jmp"}
                or _cc_cfg._instruction_may_clobber_register(
                    instructions[index], "ecx"
                )
                for index in range(assignment_index)
                if index not in {root_saves[0], receiver_saves[0]}
            ):
                continue
            reachable = _cc_cfg._reachable_cfg_indices(
                successors,
                (assignment_index,),
            )
            if (
                unresolved & reachable
                or transfer_index not in reachable
                or not all_restores <= reachable
            ):
                continue

            active: set[int] = set()
            finished: set[int] = set()

            def has_cycle(index: int) -> bool:
                if index in active:
                    return True
                if index in finished or index not in reachable:
                    return False
                active.add(index)
                if any(
                    has_cycle(target)
                    for target in successors.get(index, ())
                ):
                    return True
                active.remove(index)
                finished.add(index)
                return False

            if has_cycle(assignment_index):
                continue

            call_candidates: dict[int, tuple[int, int]] = {}
            for call_index in invocation_indices:
                if not transfer_index < call_index < min(all_restores):
                    continue
                call = instructions[call_index]
                exact_slot = _cc_targets._exact_indirect_register_call_slot(call)
                if exact_slot is None or exact_slot[1] <= 0:
                    continue
                vptr_register, slot = exact_slot
                prior_invocation_boundary = max(
                    (
                        index + 1
                        for index in invocation_indices
                        if index < call_index
                    ),
                    default=assignment_index + 1,
                )
                receiver_moves = [
                    index
                    for index in range(
                        prior_invocation_boundary,
                        call_index,
                    )
                    if _cc_receiver_instructions._exact_register_move(instructions[index])
                    == ("ecx", receiver_alias)
                ]
                vptr_loads = [
                    index
                    for index in range(
                        prior_invocation_boundary,
                        call_index,
                    )
                    if _cc_receiver_candidate._exact_register_memory_load(instructions[index])
                    == (vptr_register, receiver_alias, 0)
                ]
                if len(receiver_moves) != 1 or len(vptr_loads) != 1:
                    continue
                receiver_index = receiver_moves[0]
                vptr_index = vptr_loads[0]
                if not (
                    transfer_index < receiver_index < call_index
                    and transfer_index < vptr_index < call_index
                ):
                    continue
                local_start = min(receiver_index, vptr_index)
                if any(
                    _cc_cfg._instruction_mnemonic(instructions[index]).startswith("j")
                    or _cc_cfg._instruction_mnemonic(instructions[index]).startswith(
                        "loop"
                    )
                    or _cc_cfg._instruction_mnemonic(instructions[index])
                    in {"call", "jmp"}
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
                    for index in range(local_start, call_index)
                ):
                    continue
                call_candidates[call_index] = (receiver_index, slot)
            if not call_candidates:
                continue
            allowed_receiver_moves = {
                receiver_index
                for receiver_index, _slot in call_candidates.values()
            }

            # Receiver phase 0 is the saved caller value before the exact
            # field transfer, 1 is this+field, and 2 is restored/dead.
            work: list[tuple[int, bool, int]] = [
                (target, True, 0)
                for target in successors.get(assignment_index, ())
            ]
            visited: set[tuple[int, bool, int]] = set()
            reached_calls: set[int] = set()
            reached_returns: set[int] = set()
            valid = True
            while work and valid:
                index, root_live, receiver_phase = work.pop()
                state = (index, root_live, receiver_phase)
                if state in visited:
                    continue
                visited.add(state)
                instruction = instructions[index]
                if index in unresolved:
                    valid = False
                    break
                if _cc_cfg._exact_return_terminates(instruction):
                    if root_live or receiver_phase != 2:
                        valid = False
                    else:
                        reached_returns.add(index)
                    continue
                if _cc_cfg._instruction_mnemonic(instruction) in {"ret", "retn"}:
                    valid = False
                    break
                next_root_live = root_live
                next_receiver_phase = receiver_phase
                if index == transfer_index:
                    if not root_live or receiver_phase != 0:
                        valid = False
                        break
                    next_receiver_phase = 1
                elif index in root_restores:
                    if not root_live or receiver_phase == 0:
                        valid = False
                        break
                    next_root_live = False
                elif index in receiver_restores:
                    if receiver_phase != 1:
                        valid = False
                        break
                    next_receiver_phase = 2
                else:
                    if root_live:
                        if (
                            _cc_cfg._instruction_may_clobber_register(
                                instruction, root_alias
                            )
                            or alias_escapes(instruction, root_alias)
                        ):
                            valid = False
                            break
                    elif _cc_receiver_retail._dead_saved_alias_is_referenced_or_redefined(
                        instruction,
                        root_alias,
                    ):
                        valid = False
                        break
                    if receiver_phase == 0:
                        if _cc_receiver_retail._dead_saved_alias_is_referenced_or_redefined(
                            instruction,
                            receiver_alias,
                        ):
                            valid = False
                            break
                    elif receiver_phase == 1:
                        if (
                            _cc_cfg._instruction_may_clobber_register(
                                instruction, receiver_alias
                            )
                            or (
                                alias_escapes(instruction, receiver_alias)
                                and index not in allowed_receiver_moves
                            )
                        ):
                            valid = False
                            break
                    elif _cc_receiver_retail._dead_saved_alias_is_referenced_or_redefined(
                        instruction,
                        receiver_alias,
                    ):
                        valid = False
                        break
                if index in call_candidates:
                    if not root_live or receiver_phase != 1:
                        valid = False
                        break
                    reached_calls.add(index)
                targets = successors.get(index, ())
                if not targets:
                    valid = False
                    break
                work.extend(
                    (
                        target,
                        next_root_live,
                        next_receiver_phase,
                    )
                    for target in targets
                )
            if (
                valid
                and reached_returns
                and reached_calls == set(call_candidates)
                and all(
                    any(state[0] == restore for state in visited)
                    for restore in all_restores
                )
            ):
                marker = (
                    "load(exact-receiver-field("
                    f"this,+0x{field_displacement:x}))"
                )
                packages.append(
                    {call_index: marker for call_index in call_candidates}
                )
    return packages[0] if len(packages) == 1 else {}


def _exact_candidate_cfg_saved_entry_this_multi_field_vptr_proofs(
    instructions: Sequence[Instruction],
    *,
    invocation_indices: Sequence[int],
    addresses: Sequence[int | None],
    caller_start: int,
    caller_end: int,
    local_control_flow_indices: frozenset[int],
    local_control_flow_targets: Mapping[int, tuple[int, ...]],
) -> dict[int, str]:
    """Prove several fields from one saved entry-this root until rebind."""

    if (
        not instructions
        or not invocation_indices
        or len(addresses) != len(instructions)
    ):
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

    packages: list[dict[int, str]] = []
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
        terminal_rebinds = frozenset(
            index
            for index, instruction in enumerate(instructions)
            if (load := _cc_receiver_candidate._exact_register_memory_load(instruction)) is not None
            and load[0] == alias
            and load[1] == alias
            and load[2] > 0
        )
        if not (
            len(assignments) == len(saves) == 1
            and restores
            and terminal_rebinds
            and saves[0] < assignments[0] < min(terminal_rebinds)
            and min(terminal_rebinds) < min(restores)
        ):
            continue
        assignment_index = assignments[0]
        if any(
            _cc_cfg._instruction_mnemonic(instructions[index]).startswith("j")
            or _cc_cfg._instruction_mnemonic(instructions[index]).startswith("loop")
            or _cc_cfg._instruction_mnemonic(instructions[index]) in {"call", "jmp"}
            or _cc_cfg._instruction_may_clobber_register(
                instructions[index], "ecx"
            )
            for index in range(assignment_index)
            if index != saves[0]
        ):
            continue
        reachable = _cc_cfg._reachable_cfg_indices(successors, (assignment_index,))
        if (
            unresolved & reachable
            or not restores <= reachable
            or not terminal_rebinds <= reachable
        ):
            continue

        call_candidates: dict[int, tuple[int, int]] = {}
        for call_index in invocation_indices:
            call = instructions[call_index]
            exact_slot = _cc_targets._exact_indirect_register_call_slot(call)
            if exact_slot is None or exact_slot[1] <= 0:
                continue
            vptr_register, slot = exact_slot
            prior_invocation_boundary = max(
                (
                    index + 1
                    for index in invocation_indices
                    if index < call_index
                ),
                default=assignment_index + 1,
            )
            vptr_loads = [
                index
                for index in range(prior_invocation_boundary, call_index)
                if _cc_receiver_candidate._exact_register_memory_load(instructions[index])
                == (vptr_register, "ecx", 0)
            ]
            if len(vptr_loads) != 1:
                continue
            vptr_index = vptr_loads[0]
            receiver_definition_index = next(
                (
                    index
                    for index in range(
                        vptr_index - 1,
                        prior_invocation_boundary - 1,
                        -1,
                    )
                    if _cc_cfg._instruction_may_clobber_register(
                        instructions[index], "ecx"
                    )
                ),
                -1,
            )
            receiver_definition = (
                _cc_receiver_candidate._exact_register_memory_load(
                    instructions[receiver_definition_index]
                )
                if receiver_definition_index >= prior_invocation_boundary
                else None
            )
            if not (
                receiver_definition is not None
                and receiver_definition[0] == "ecx"
                and receiver_definition[1] == alias
                and receiver_definition[2] > 0
                and assignment_index < receiver_definition_index < call_index
            ):
                continue
            if any(
                _cc_cfg._instruction_mnemonic(instructions[index]).startswith("j")
                or _cc_cfg._instruction_mnemonic(instructions[index]).startswith(
                    "loop"
                )
                or _cc_cfg._instruction_mnemonic(instructions[index])
                in {"call", "jmp"}
                or _cc_cfg._instruction_may_clobber_register(
                    instructions[index], "ecx"
                )
                or (
                    index != vptr_index
                    and _cc_cfg._instruction_may_clobber_register(
                        instructions[index], vptr_register
                    )
                )
                for index in range(
                    receiver_definition_index + 1,
                    call_index,
                )
            ):
                continue
            call_candidates[call_index] = (
                receiver_definition[2],
                slot,
            )
        if len({field for field, _slot in call_candidates.values()}) < 3:
            continue

        # Phase 0 retains the exact entry-this root, phase 1 begins at one
        # terminal self-field rebind on each exit path, and phase 2 begins at
        # the matching POP.  Resolved loops are permitted while phase 0 is
        # invariant; unknown edges and every root clobber/escape still fail.
        work: list[tuple[int, int]] = [
            (target, 0)
            for target in successors.get(assignment_index, ())
        ]
        visited: set[tuple[int, int]] = set()
        reached_calls: set[int] = set()
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
            if index in terminal_rebinds:
                if phase != 0:
                    valid = False
                    break
                next_phase = 1
            elif index in restores:
                # A branch that bypasses the governed calls may retain the
                # original root all the way to its matching restore.  Paths
                # that retire the root first restore from phase 1.
                if phase not in {0, 1}:
                    valid = False
                    break
                next_phase = 2
            elif phase == 0:
                if (
                    _cc_cfg._instruction_may_clobber_register(instruction, alias)
                    or alias_escapes(instruction, alias)
                ):
                    valid = False
                    break
                next_phase = 0
            elif phase == 1:
                if _cc_receiver_instructions._exact_register_stack_transfer(instruction) == (
                    "push",
                    alias,
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
            if index in call_candidates:
                if phase != 0:
                    valid = False
                    break
                reached_calls.add(index)
            targets = successors.get(index, ())
            if not targets:
                valid = False
                break
            work.extend((target, next_phase) for target in targets)
        if (
            valid
            and reached_returns
            and reached_calls == set(call_candidates)
            and all(
                any(state[0] == row for state in visited)
                for row in terminal_rebinds | restores
            )
        ):
            packages.append(
                {
                    call_index: (
                        "load(exact-receiver-field("
                        f"this,+0x{field:x}))"
                    )
                    for call_index, (field, _slot) in call_candidates.items()
                }
            )
    return packages[0] if len(packages) == 1 else {}


def _exact_candidate_saved_entry_this_alias_vptr_proofs(
    instructions: Sequence[Instruction],
    *,
    invocation_indices: Sequence[int],
) -> dict[int, str]:
    """Prove the exact VC5 saved-nonvolatile entry-this receiver shape.

    This is deliberately a whole-function proof.  A qualifying nonvolatile
    register is saved before its unique assignment from entry ECX, remains
    unmodified and unescaped through the complete pair of receiver-field
    virtual calls, and is restored on the sole straight-line epilogue.  The
    structural slot sequence distinguishes the reviewed member operation from
    an arbitrary alias-shaped virtual dispatch.
    """

    lifetime = _cc_receiver_retail._exact_candidate_saved_entry_this_alias_lifetime(
        instructions,
        invocation_indices=invocation_indices,
    )
    if lifetime is None:
        return {}
    alias, assignment_index, restore_index = lifetime
    call_rows: list[tuple[int, int]] = []
    for call_index in invocation_indices:
        call = instructions[call_index]
        expression, slot = _cc_targets._memory_slot(_cc_cfg._instruction_operand(call))
        base_match = re.fullmatch(
            r"(?P<base>eax|ecx|edx|ebx|esi|edi|ebp)"
            r"(?:\+(?:0x[0-9a-f]+|\d+))?",
            expression,
        )
        if (
            _cc_cfg._instruction_mnemonic(call) != "call"
            or not _cc_cfg._exact_invocation_encoding(call, mnemonic="call")
            or base_match is None
            or slot not in {0x20, 0x60}
            or not assignment_index < call_index < restore_index
        ):
            continue
        base = base_match.group("base")
        prior_invocation_boundary = max(
            (index + 1 for index in invocation_indices if index < call_index),
            default=assignment_index + 1,
        )
        vptr_rows = [
            (index, load)
            for index in range(prior_invocation_boundary, call_index)
            if (load := _cc_receiver_candidate._exact_register_memory_load(instructions[index])) is not None
            and load == (base, "ecx", 0)
        ]
        if len(vptr_rows) != 1:
            continue
        vptr_index = vptr_rows[0][0]
        receiver_rows = [
            (index, load)
            for index in range(prior_invocation_boundary, vptr_index)
            if (load := _cc_receiver_candidate._exact_register_memory_load(instructions[index])) is not None
            and load == ("ecx", alias, 4)
        ]
        if len(receiver_rows) != 1:
            continue
        receiver_index = receiver_rows[0][0]
        if any(
            _cc_cfg._instruction_mnemonic(instructions[index]).startswith("j")
            or _cc_cfg._instruction_mnemonic(instructions[index]).startswith("loop")
            or _cc_cfg._instruction_mnemonic(instructions[index]) in {"call", "jmp"}
            or (
                index != vptr_index
                and _cc_cfg._instruction_may_clobber_register(
                    instructions[index], base
                )
            )
            or (
                index != receiver_index
                and _cc_cfg._instruction_may_clobber_register(
                    instructions[index], "ecx"
                )
            )
            for index in range(receiver_index, call_index)
        ):
            continue
        call_rows.append((call_index, slot))

    if (
        tuple(slot for _index, slot in call_rows) != (0x60, 0x20)
        or tuple(index for index, _slot in call_rows)
        != tuple(invocation_indices)
    ):
        return {}
    return {
        index: "load(exact-receiver-field(this,+0x4))"
        for index, _slot in call_rows
    }


def _exact_retail_zvid_dd_com_vptr_call_proofs(
    instructions: Sequence[Instruction],
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge: BinaryNinjaBridge,
    local_control_flow_indices: frozenset[int],
    local_control_flow_targets: Mapping[int, tuple[int, ...]],
) -> dict[int, str]:
    """Prove zVid's finite targetless DirectDraw surface-vptr caller.

    The selected body is installed as a callback and reached through one
    callback-storage tail dispatch.  Its four COM implementation targets remain
    runtime-selected, while its two ReportError calls have exact authored target
    identity.  This proof therefore publishes only exact receiver-vptr storage
    and slots after validating the complete caller, CFG, six-call population,
    inbound population, cleanup, and the two independent member/global receiver
    lineages.
    """

    expected_identity = "symbol:recoil:function:0x4a8220"
    start = address_value(caller_start)
    end = address_value(caller_end_exclusive)
    if caller_identity != expected_identity and start != 0x4A8220:
        return {}

    label = "zVid DD COM-vtable lineage"

    def reject(detail: str) -> NoReturn:
        raise ValueError(f"{label} rejects {detail}")

    if caller_identity != expected_identity or start != 0x4A8220:
        reject("a caller identity/address collision")
    if end != 0x4A82F0:
        reject("the exact [0x4a8220,0x4a82f0) registered extent")

    addresses = _cc_cfg._instruction_runtime_addresses(
        instructions,
        source="bn",
        caller_start=start,
    )
    if len(addresses) != len(instructions) or not instructions:
        reject("an empty or mismatched instruction population")
    counts = Counter(address for address in addresses if address is not None)
    if any(address is None or counts[address] != 1 for address in addresses):
        reject("missing or duplicate runtime instruction coordinates")
    selected: list[tuple[int, int, bytes]] = []
    for index, (instruction, address) in enumerate(zip(instructions, addresses)):
        assert address is not None
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            body = b""
        if not body:
            reject("missing complete retail instruction bytes")
        selected.append((index, address, body))
    if (
        selected[0][1] != start
        or selected[-1][1] + len(selected[-1][2]) != end
        or any(
            address + len(body) != selected[position + 1][1]
            for position, (_index, address, body) in enumerate(selected[:-1])
        )
    ):
        reject("the exact [0x4a8220,0x4a82f0) complete body extent")

    index_by_address = {
        address: index for index, address, _body in selected
    }
    successors, unresolved = _cc_cfg._exact_invocation_cfg(
        instructions,
        instruction_addresses=addresses,
        instruction_index_by_address=index_by_address,
        source="bn",
        caller_start=start,
        caller_end=end,
        local_control_flow_indices=local_control_flow_indices,
        local_control_flow_targets=dict(local_control_flow_targets or {}),
    )
    reachable = _cc_cfg._reachable_cfg_indices(successors, (0,))
    if unresolved & reachable or reachable != frozenset(range(len(instructions))):
        reject("the complete unambiguous entry-reachable CFG")

    invocation_specs = (
        (0x4A8258, "com", 0x14, 0x18, "member"),
        (0x4A826C, "com", 0x6C, 0x04, "member"),
        (0x4A827F, "direct", None, None, "report-error"),
        (0x4A82B4, "com", 0x14, 0x18, "global"),
        (0x4A82CA, "com", 0x6C, 0x04, "global"),
        (0x4A82DD, "direct", None, None, "report-error"),
    )
    try:
        invocation_spec_indices = tuple(
            index_by_address[address]
            for address, _kind, _slot, _cleanup, _root in invocation_specs
        )
    except KeyError:
        reject("the complete six-call runtime-address population")
    try:
        short_jump_index = index_by_address[0x4A829B]
        short_jump_target_index = index_by_address[0x4A82A2]
    except KeyError:
        reject("the exact 0x4a829b-to-0x4a82a2 local short-jump coordinates")
    short_jump = instructions[short_jump_index]
    try:
        short_jump_body = bytes(
            int(item, 16) for item in short_jump.bytes
        )
    except (TypeError, ValueError):
        short_jump_body = b""
    rendered_short_targets = _cc_catalog.ADDRESS_RE.findall(
        _cc_cfg._instruction_operand(short_jump)
    )
    if (
        short_jump_body != bytes.fromhex("eb 05")
        or _cc_cfg._instruction_mnemonic(short_jump) != "jmp"
        or not _cc_cfg._exact_invocation_encoding(short_jump, mnemonic="jmp")
        or 0x4A829B + 2 + struct.unpack_from("<b", short_jump_body, 1)[0]
        != 0x4A82A2
        or not rendered_short_targets
        or normalize_address(rendered_short_targets[-1]) != "0x4a82a2"
        or successors.get(short_jump_index) != (short_jump_target_index,)
    ):
        reject("the exact EB05 local CFG edge from 0x4a829b to 0x4a82a2")
    if (
        short_jump_index in local_control_flow_indices
        or short_jump_index in invocation_spec_indices
    ):
        reject("a local-short-jump/switch/external address collision")
    encoded_transfer_indices = tuple(
        index
        for index, instruction in enumerate(instructions)
        if _cc_cfg._instruction_mnemonic(instruction) in {"call", "jmp"}
        and _cc_cfg._exact_invocation_encoding(
            instruction,
            mnemonic=_cc_cfg._instruction_mnemonic(instruction),
        )
    )
    local_transfer_indices = frozenset(
        index
        for index in encoded_transfer_indices
        if index in local_control_flow_indices
    )
    if local_transfer_indices != local_control_flow_indices:
        reject("the exact already-proven local CFG transfer partition")
    if local_transfer_indices.intersection(invocation_spec_indices):
        reject("a local-CFG/external-invocation address collision")
    specialized_local_transfer_indices = frozenset(
        {*local_transfer_indices, short_jump_index}
    )
    invocation_indices = tuple(
        index
        for index in encoded_transfer_indices
        if index not in specialized_local_transfer_indices
    )
    if invocation_indices != invocation_spec_indices:
        reject("the exact six-call population without collision or extras")

    report_error_identity = "symbol:recoil:function:0x4ad6a0"
    if indexes.by_address.get("0x4ad6a0") != report_error_identity:
        reject("the reviewed ReportError direct target identity")
    try:
        _report_error_name, report_error_start = _cc_identity._bn_unique_containing_function(
            bridge,
            instruction_address="0x4ae1e6",
        )
        report_error_rows = tuple(
            _cc_listing.parse_assembly(bridge.assembly(report_error_start), source="bn")
        )
    except (BridgeError, OSError, RuntimeError, TypeError, ValueError) as exc:
        raise ValueError(
            f"{label} rejects unavailable ReportError callee cleanup authority"
        ) from exc
    report_error_tails = [
        row
        for row in report_error_rows
        if _cc_cfg._source_instruction_address(row) == "0x4ae1e6"
    ]
    if len(report_error_tails) != 1:
        reject("one exact ReportError callee tail at 0x4ae1e6")
    report_error_tail = report_error_tails[0]
    try:
        report_error_tail_body = bytes(
            int(item, 16) for item in report_error_tail.bytes
        )
    except (TypeError, ValueError):
        report_error_tail_body = b""
    if (
        report_error_tail_body != bytes.fromhex("c2 04 00")
        or _cc_cfg._instruction_mnemonic(report_error_tail) not in {"ret", "retn"}
        or _cc_cfg._parse_unsigned_assembly_integer(
            _cc_cfg._instruction_operand(report_error_tail)
        )
        != 4
        or not _cc_cfg._exact_return_terminates(report_error_tail)
    ):
        reject("the exact callee-cleaned ReportError RET 4 tail")
    report_error_lines = {0x4A827F: 0x267, 0x4A82DD: 0x27F}
    for index, (address, kind, _slot, cleanup, _root) in zip(
        invocation_indices,
        invocation_specs,
    ):
        if kind != "direct":
            continue
        instruction = instructions[index]
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            body = b""
        if (
            len(body) != 5
            or body[0] != 0xE8
            or _cc_cfg._instruction_mnemonic(instruction) != "call"
            or address + 5 + struct.unpack_from("<i", body, 1)[0]
            != 0x4AD6A0
            or _cc_cfg._instruction_operand(instruction) != "zVideo_dd::ReportError"
            or _cc_cfg._cleanup_after(instructions, index) is not cleanup
        ):
            reject(f"the exact ReportError direct call/cleanup at 0x{address:x}")
        setup_addresses = (address - 0xC, address - 7, address - 2)
        try:
            setup_indices = tuple(
                index_by_address[setup_address]
                for setup_address in setup_addresses
            )
        except KeyError:
            reject(f"the exact ReportError setup coordinates at 0x{address:x}")
        push_index, edx_index, ecx_index = setup_indices
        setup_rows = tuple(instructions[item] for item in setup_indices)
        try:
            setup_bodies = tuple(
                bytes(int(item, 16) for item in row.bytes)
                for row in setup_rows
            )
        except (TypeError, ValueError):
            setup_bodies = ()
        line_number = report_error_lines[address]
        if (
            setup_indices != (index - 3, index - 2, index - 1)
            or setup_bodies
            != (
                b"\x68" + struct.pack("<I", line_number),
                bytes.fromhex("ba e8 30 4e 00"),
                bytes.fromhex("8b c8"),
            )
            or _cc_receiver_instructions._exact_register_move(instructions[ecx_index]) != ("ecx", "eax")
            or _cc_receiver_instructions._exact_register_move_immediate(instructions[edx_index])
            != ("edx", 0x4E30E8)
            or _cc_cfg._instruction_mnemonic(instructions[push_index]) != "push"
            or _cc_cfg._parse_unsigned_assembly_integer(
                _cc_cfg._instruction_operand(instructions[push_index])
            )
            != line_number
        ):
            reject(f"the exact ReportError register/line setup at 0x{address:x}")
        exit_rows = tuple(instructions[index + 1 : index + 5])
        try:
            exit_bodies = tuple(
                bytes(int(item, 16) for item in row.bytes)
                for row in exit_rows
            )
        except (TypeError, ValueError):
            exit_bodies = ()
        if (
            len(exit_rows) != 4
            or exit_bodies
            != (
                bytes.fromhex("5f"),
                bytes.fromhex("5e"),
                bytes.fromhex("83 c4 64"),
                bytes.fromhex("c3"),
            )
            or _cc_receiver_instructions._exact_register_stack_transfer(exit_rows[0]) != ("pop", "edi")
            or _cc_receiver_instructions._exact_register_stack_transfer(exit_rows[1]) != ("pop", "esi")
            or _cc_receiver_instructions._exact_register_add_immediate(exit_rows[2]) != ("esp", 0x64)
            or not _cc_cfg._exact_return_terminates(exit_rows[3])
        ):
            reject(
                f"the exact post-ReportError register/frame teardown at 0x{address:x}"
            )

    com_specs = tuple(
        (index, address, slot, cleanup, root)
        for index, (address, kind, slot, cleanup, root) in zip(
            invocation_indices,
            invocation_specs,
        )
        if kind == "com" and slot is not None
    )

    entry_aliases = [
        index
        for index, instruction in enumerate(instructions[: com_specs[0][0]])
        if _cc_receiver_instructions._exact_register_move(instruction) == ("esi", "edx")
    ]
    if len(entry_aliases) != 1 or any(
        index != entry_aliases[0]
        and _cc_cfg._instruction_may_clobber_register(instructions[index], "esi")
        for index in range(entry_aliases[0], com_specs[1][0] + 1)
    ):
        reject("the unique preserved entry EDX-to-ESI alias")

    global_identity = indexes.storage_by_address.get("0x6333f4", "")
    if (
        not global_identity
        or global_identity.startswith("iat:")
        or global_identity in indexes.provider_ids
    ):
        reject("one exact non-provider, non-IAT global 0x6333f4 storage identity")

    member_marker = "load(load(entry-register(edx)+0x1c))"
    global_marker = f"load({global_identity})"
    exact_global_receiver_addresses = {
        0x4A82B4: (0x4A828A, 0x4A829D),
        0x4A82CA: (0x4A82C2,),
    }
    exact_global_vptr_addresses = {
        0x4A82B4: 0x4A82A2,
        0x4A82CA: 0x4A82C8,
    }
    exact_global_push_rows = {
        0x4A82B4: (
            (0x4A82A8, bytes.fromhex("51")),
            (0x4A82A9, bytes.fromhex("68 00 00 00 02")),
            (0x4A82AE, bytes.fromhex("6a 00")),
            (0x4A82B0, bytes.fromhex("6a 00")),
            (0x4A82B2, bytes.fromhex("57")),
            (0x4A82B3, bytes.fromhex("50")),
        ),
        0x4A82CA: ((0x4A82C7, bytes.fromhex("50")),),
    }
    observed_global_a1_addresses = tuple(
        address
        for instruction, address in zip(instructions, addresses)
        if address is not None
        and bytes(int(item, 16) for item in instruction.bytes)
        == bytes.fromhex("a1 f4 33 63 00")
    )
    if observed_global_a1_addresses != (0x4A828A, 0x4A829D, 0x4A82C2):
        reject("the exact three-row A1 global receiver population")
    proof_by_index: dict[int, str] = {}
    for call_index, call_address, slot, callee_cleanup, root_kind in com_specs:
        call_shape = _cc_targets._exact_indirect_register_call_slot(
            instructions[call_index]
        )
        if call_shape is None or call_shape[1] != slot:
            reject(f"the exact +0x{slot:x} virtual call at 0x{call_address:x}")
        if root_kind == "global":
            call_body = bytes(
                int(item, 16) for item in instructions[call_index].bytes
            )
            if call_body != bytes((0xFF, 0x52, slot)):
                reject(
                    f"the exact FF52 +0x{slot:x} global virtual call at "
                    f"0x{call_address:x}"
                )
        if _cc_cfg._cleanup_after(instructions, call_index) is not None:
            reject(
                f"caller cleanup after the callee-cleaned COM call at "
                f"0x{call_address:x}"
            )
        vptr_register = call_shape[0]
        invocation_position = invocation_indices.index(call_index)
        lower = (
            invocation_indices[invocation_position - 1] + 1
            if invocation_position > 0
            else 0
        )
        vptr_rows = [
            (index, load)
            for index in range(lower, call_index)
            if (load := _cc_receiver_candidate._exact_register_memory_load(instructions[index]))
            is not None
            and load[0] == vptr_register
            and load[2] == 0
        ]
        if len(vptr_rows) != 1:
            reject(f"one exact vptr dereference for 0x{call_address:x}")
        vptr_index, (_vptr, receiver_register, _zero) = vptr_rows[0]
        if root_kind == "global":
            expected_vptr_address = exact_global_vptr_addresses[call_address]
            if (
                addresses[vptr_index] != expected_vptr_address
                or bytes(
                    int(item, 16) for item in instructions[vptr_index].bytes
                )
                != bytes.fromhex("8b 10")
                or (vptr_register, receiver_register) != ("edx", "eax")
            ):
                reject(
                    f"the exact EDX-from-EAX vptr row at "
                    f"0x{expected_vptr_address:x}"
                )
        if any(
            _cc_cfg._instruction_may_clobber_register(instructions[index], vptr_register)
            for index in range(vptr_index + 1, call_index)
        ):
            reject(f"the preserved vptr register at 0x{call_address:x}")

        setup_control_transfers = [
            index
            for index in range(lower, call_index)
            if (
                _cc_cfg._instruction_mnemonic(instructions[index]).startswith("j")
                or _cc_cfg._instruction_mnemonic(instructions[index])
                in {"call", "ret", "retn", "iret", "loop", "loope", "loopne", "jecxz"}
            )
        ]
        setup_lower = (
            setup_control_transfers[-1] + 1
            if setup_control_transfers
            else lower
        )
        stack_pushes = [
            index
            for index in range(setup_lower, call_index)
            if (
                _cc_receiver_instructions._exact_register_stack_transfer(instructions[index])
                or (None, None)
            )[0]
            == "push"
            or _cc_identity._exact_unknown_push32(instructions[index])
        ]
        if (
            len(stack_pushes) != callee_cleanup // 4
            or any(
                _cc_cfg._instruction_mnemonic(instructions[index]) == "push"
                and index not in stack_pushes
                for index in range(setup_lower, call_index)
            )
        ):
            reject(
                f"the exact {callee_cleanup // 4}-word callee-cleaned COM "
                f"stack setup at 0x{call_address:x}"
            )

        if root_kind == "member":
            receiver_rows = [
                (index, load)
                for index in range(lower, vptr_index)
                if (load := _cc_receiver_candidate._exact_register_memory_load(instructions[index]))
                == (receiver_register, "esi", 0x1C)
            ]
            marker = member_marker
        else:
            expected_receiver_addresses = exact_global_receiver_addresses[
                call_address
            ]
            receiver_rows = []
            for receiver_address in expected_receiver_addresses:
                index = index_by_address.get(receiver_address, -1)
                if index < lower or index >= vptr_index:
                    reject(
                        f"the exact A1 receiver coordinates for "
                        f"0x{call_address:x}"
                    )
                instruction = instructions[index]
                try:
                    body = bytes(int(item, 16) for item in instruction.bytes)
                except (TypeError, ValueError):
                    body = b""
                operands = _cc_cfg._instruction_operand(instruction).split(",", 1)
                if (
                    body == bytes.fromhex("a1 f4 33 63 00")
                    and receiver_register == "eax"
                    and _cc_cfg._instruction_mnemonic(instruction) == "mov"
                    and len(operands) == 2
                    and operands[0].strip().lower() == "eax"
                    and _cc_targets._exact_memory_expression(operands[1]) in {
                        "0x6333f4",
                        *(
                            name
                            for name, identity in indexes.storage_by_name.items()
                            if identity == global_identity
                        ),
                    }
                ):
                    receiver_rows.append((index, (receiver_register, "", 0)))
            marker = global_marker
        expected_receiver_count = (
            len(exact_global_receiver_addresses[call_address])
            if root_kind == "global"
            else 1
        )
        if len(receiver_rows) != expected_receiver_count:
            reject(
                f"the exact {root_kind} surface receiver definition set at "
                f"0x{call_address:x}"
            )
        receiver_indices = frozenset(index for index, _load in receiver_rows)
        receiver_index = receiver_rows[-1][0]
        if root_kind == "global":
            actual_push_rows = tuple(
                (
                    int(addresses[index]),
                    bytes(int(item, 16) for item in instructions[index].bytes),
                )
                for index in stack_pushes
            )
            if actual_push_rows != exact_global_push_rows[call_address]:
                reject(
                    f"the exact global COM push rows at 0x{call_address:x}"
                )
        if (
            not stack_pushes
            or _cc_receiver_instructions._exact_register_stack_transfer(
                instructions[stack_pushes[-1]]
            )
            != ("push", receiver_register)
        ):
            reject(
                f"the exact COM receiver stack argument at 0x{call_address:x}"
            )
        if root_kind == "member" and any(
            _cc_cfg._instruction_may_clobber_register(
                instructions[index], receiver_register
            )
            for index in range(receiver_index + 1, vptr_index)
        ):
            reject(f"the preserved {root_kind} receiver at 0x{call_address:x}")
        if not _cc_receiver_storage._is_bounded_dynamic_load(
            marker,
            allow_entry_register_root=True,
        ):
            reject(f"the bounded targetless {root_kind} storage marker")
        if not _cc_cfg._exact_register_definition_set_covers_transfer(
            instructions,
            instruction_addresses=addresses,
            instruction_index_by_address=index_by_address,
            definition_indices=receiver_indices,
            transfer_index=vptr_index,
            register=receiver_register,
            source="bn",
            caller_start=start,
            caller_end=end,
            local_control_flow_indices=local_control_flow_indices,
            local_control_flow_targets=dict(local_control_flow_targets),
        ) or not _cc_cfg._exact_register_definition_set_covers_transfer(
            instructions,
            instruction_addresses=addresses,
            instruction_index_by_address=index_by_address,
            definition_indices=frozenset({vptr_index}),
            transfer_index=call_index,
            register=vptr_register,
            source="bn",
            caller_start=start,
            caller_end=end,
            local_control_flow_indices=local_control_flow_indices,
            local_control_flow_targets=dict(local_control_flow_targets),
        ):
            reject(f"the complete receiver/vptr CFG lineage at 0x{call_address:x}")
        proof_by_index[call_index] = marker

    callback_sources: set[str] = set()
    inbound = _cc_identity._complete_bn_inbound_direct_transfers(
        bridge,
        target_address="0x4a8220",
        callback_materializations_out=callback_sources,
    )
    if (
        inbound is None
        or callback_sources != {"0x4a7823"}
        or inbound
    ):
        reject("the sole 0x4a7823 callback installation and zero direct xrefs")

    try:
        storage_xrefs = bridge.get_json(
            "getXrefsTo",
            address="0x6333c8",
            limit=100000,
        )
    except (BridgeError, OSError, RuntimeError, ValueError) as exc:
        raise ValueError(
            f"{label} rejects unavailable callback-storage xref authority"
        ) from exc
    code_rows, data_rows, complete = _cc_identity._bn_inbound_xref_items(storage_xrefs)
    storage_rows = [*code_rows, *data_rows]
    storage_sources = [
        _cc_identity._bn_xref_address(
            row,
            "source_address",
            "source_addr",
            "from_address",
            "from_addr",
            "address",
            "addr",
            "source",
            "from",
        )
        for row in storage_rows
    ]
    if (
        not complete
        or Counter(storage_sources)
        != {"0x4a6835": 1, "0x4a7823": 1}
    ):
        reject("the complete 0x6333c8 callback writer/reader xref population")

    _tail_name, tail_start = _cc_identity._bn_unique_containing_function(
        bridge,
        instruction_address="0x4a6835",
    )
    tail_rows = tuple(
        _cc_listing.parse_assembly(bridge.assembly(tail_start), source="bn")
    )
    matching_tail = [
        instruction
        for instruction in tail_rows
        if _cc_cfg._source_instruction_address(instruction) == "0x4a6835"
    ]
    if len(matching_tail) != 1:
        reject("one exact 0x4a6835 callback-storage tail instruction")
    tail = matching_tail[0]
    try:
        tail_body = bytes(int(item, 16) for item in tail.bytes)
    except (TypeError, ValueError):
        tail_body = b""
    if (
        tail_body != bytes.fromhex("ff 25 c8 33 63 00")
        or _cc_cfg._instruction_mnemonic(tail) != "jmp"
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(tail))
        != "0x6333c8"
        or not _cc_cfg._exact_invocation_encoding(tail, mnemonic="jmp")
    ):
        reject("the exact indirect tail dispatch at 0x4a6835 through 0x6333c8")
    return proof_by_index
