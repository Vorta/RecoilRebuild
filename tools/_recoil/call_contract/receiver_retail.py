"""Recoil call-contract receiver retail evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract import instructions as _cc_instructions
from _recoil.call_contract import receiver_candidate as _cc_receiver_candidate
from _recoil.call_contract import receiver_equivalence as _cc_receiver_equivalence
from _recoil.call_contract import receiver_instructions as _cc_receiver_instructions
from _recoil.call_contract import receiver_proofs as _cc_receiver_proofs
from _recoil.call_contract import receiver_storage as _cc_receiver_storage
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import IdentityIndexes

import re
import struct
from typing import Mapping, Sequence

from _recoil.commands.asm_verify import Instruction
from _recoil.lib.progress import normalize_address

_ENCODED_REGISTERS = ("eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi")

def _exact_retail_cfg_register_provenance(
    instructions: Sequence[Instruction],
    *,
    before_index: int,
    register: str,
    addresses: Sequence[int | None],
    indexes: IdentityIndexes,
    caller_start: int,
    caller_end: int,
    local_control_flow_indices: frozenset[int],
    local_control_flow_targets: Mapping[int, tuple[int, ...]],
    call_cleanup_by_instruction_index: Mapping[int, int] | None = None,
    allow_exact_affine_receiver_roots: bool = False,
    allow_exact_caller_cleanup: bool = False,
    source: str = "bn",
    direct_call_identities: Mapping[int, str] | None = None,
    opaque_register_writes: Mapping[int, frozenset[str]] | None = None,
    allow_coff_symbolic_stack_operands: bool = False,
    observe_after: bool = False,
) -> str:
    """Recover one bounded register value over the exact retail or COFF CFG.

    The ordinary invocation walk is intentionally linear and therefore drops
    state at loop and multi-branch joins.  This bounded dataflow is used only
    for targetless vptr storage.  It propagates exact whole-register
    moves, stack loads, direct-call results, and exact memory loads; every CFG
    predecessor must agree on the final value.  Unknown edges, clobbers,
    partial definitions, and conflicting join values publish no proof.
    """

    if source not in {"bn", "cod"} or (source == "bn" and (direct_call_identities or opaque_register_writes or allow_coff_symbolic_stack_operands)):
        return ""
    if (
        not instructions
        or not 0 <= before_index < len(instructions)
        or register not in _cc_catalog.REGISTER_STATE_NAMES
        or len(addresses) != len(instructions)
    ):
        return ""
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
        source=source,
        caller_start=caller_start,
        caller_end=caller_end,
        local_control_flow_indices=local_control_flow_indices,
        local_control_flow_targets=dict(local_control_flow_targets or {}),
    )
    predecessors: dict[int, set[int]] = {
        index: set() for index in range(len(instructions))
    }
    for predecessor, successor_rows in successors.items():
        for successor in successor_rows:
            predecessors.setdefault(successor, set()).add(predecessor)
    # VC5 retains an erase-copy arm behind CMP r32,r32. Prove its dead edge
    # from the exact flags producer, without changing the static call census.
    # A jump into the conditional instruction bypasses those flags and blocks
    # this reduction even when its textual neighbor is a reflexive compare.
    for index in range(1, len(instructions)):
        if predecessors.get(index) != {index - 1}:
            continue
        compare, branch = instructions[index - 1], instructions[index]
        raw = bytes.fromhex(" ".join(compare.bytes))
        if (len(raw) != 2 or raw[0] not in {0x39, 0x3B} or raw[1] >> 6 != 3
                or (raw[1] >> 3) & 7 != raw[1] & 7):
            continue
        register_name = _ENCODED_REGISTERS[raw[1] & 7]
        if (_cc_cfg._instruction_mnemonic(compare) != "cmp"
                or re.fullmatch(rf"{register_name}\s*,\s*{register_name}", _cc_cfg._instruction_operand(compare), re.IGNORECASE) is None):
            continue
        mnemonic = _cc_cfg._instruction_mnemonic(branch)
        if mnemonic not in {"je", "jz", "jne", "jnz"} or len(successors.get(index, ())) != 2:
            continue
        rows = successors[index]
        if index + 1 not in rows:
            continue
        successors[index] = tuple(row for row in rows
            if (row != index + 1) == (mnemonic in {"je", "jz"}))
    entry_reachable = _cc_cfg._reachable_cfg_indices(successors, (0,))
    predecessors = {index: set() for index in range(len(instructions))}
    for predecessor in entry_reachable:
        for successor in successors.get(predecessor, ()):
            predecessors[successor].add(predecessor)
    reaching_target = _cc_cfg._indices_reaching_cfg_target(successors, before_index)
    if (
        before_index not in entry_reachable
        or unresolved & entry_reachable & reaching_target
    ):
        return ""

    stride_seeds = (_cc_receiver_candidate._exact_cfg_zero_stride_seeds(instructions, before_index=before_index,
        successors=successors, unresolved=unresolved) if allow_exact_affine_receiver_roots else {})
    from _recoil.call_contract.receiver_stack_loops import spilled_scalar_seeds
    spill_records = (spilled_scalar_seeds(instructions, successors=successors,
        unresolved=unresolved, before_index=before_index, cleanups=call_cleanup_by_instruction_index or {},
        opaque_indices=frozenset(opaque_register_writes or {}))
        if allow_exact_affine_receiver_roots else {})
    spilled_values = {}
    for seed, (reg, _slot, step, update) in spill_records.items():
        # The independent loop induction says how the local advances. Its
        # initial value still has to follow from the complete incoming CFG.
        initial = _exact_retail_cfg_register_provenance(instructions, before_index=seed, register=reg,
            addresses=addresses, indexes=indexes, caller_start=caller_start, caller_end=caller_end,
            local_control_flow_indices=local_control_flow_indices, local_control_flow_targets=local_control_flow_targets,
            call_cleanup_by_instruction_index=call_cleanup_by_instruction_index, source=source,
            direct_call_identities=direct_call_identities, opaque_register_writes=opaque_register_writes,
            allow_exact_caller_cleanup=allow_exact_caller_cleanup,
            allow_coff_symbolic_stack_operands=allow_coff_symbolic_stack_operands)
        if initial == 'null':
            marker = _cc_receiver_candidate._bounded_stride_provenance('bounded-counter('+reg+')', step)
            spilled_values[seed] = (reg, marker)
            spilled_values[update] = (reg, marker)
    cursor_seeds = (_cc_receiver_candidate._exact_cfg_cursor_recurrences(instructions, before_index=before_index,
        successors=successors, unresolved=unresolved) if allow_exact_affine_receiver_roots else {})
    cursor_initial_values = {}
    for seed, (seed_register, step, is_seed) in cursor_seeds.items():
        if not is_seed or seed in stride_seeds:
            continue
        # Resolve the complete acyclic preheader before feeding a recurrence.
        # Publishing a partial scalar choice while visiting predecessors can
        # otherwise poison the backedge before its other inputs are visited.
        initial = _exact_retail_cfg_register_provenance(
            instructions, before_index=seed, register=seed_register,
            addresses=addresses, indexes=indexes, caller_start=caller_start,
            caller_end=caller_end, local_control_flow_indices=local_control_flow_indices,
            local_control_flow_targets=local_control_flow_targets,
            call_cleanup_by_instruction_index=call_cleanup_by_instruction_index,
            allow_exact_affine_receiver_roots=True,
            allow_exact_caller_cleanup=allow_exact_caller_cleanup, source=source,
            direct_call_identities=direct_call_identities,
            opaque_register_writes=opaque_register_writes,
            allow_coff_symbolic_stack_operands=allow_coff_symbolic_stack_operands,
            observe_after=True)
        if initial and not initial.startswith("cursor("):
            cursor_initial_values[seed] = f"cursor({initial},{step:+#x})"

    # An allocator may occur several times in a caller. Keep each physical
    # occurrence distinct instead of identifying all returned pointers by
    # the allocator's name. This is pointer-value provenance, not a proof of
    # successful allocation, construction, or a concrete virtual target.
    allocation_results = {}
    for index, (row, address) in enumerate(zip(instructions, addresses)):
        raw = bytes.fromhex(" ".join(row.bytes)) if all(re.fullmatch(r"[0-9a-fA-F]{2}", part) for part in row.bytes) else b""
        if _cc_cfg._instruction_mnemonic(row) != "call" or len(raw) != 5 or raw[0] != 0xE8 or address is None:
            continue
        target_identity = ((direct_call_identities or {}).get(index, "") if source == "cod"
            else indexes.by_address.get(normalize_address(address + 5 + int.from_bytes(raw[1:], "little", signed=True)), ""))
        if target_identity == "provider:recoil:function:0x4c5b76":
            allocation_results[index] = f"allocation-result({target_identity},{len(allocation_results)})"

    nonzero_edges: dict[tuple[int, int], str] = {}
    for branch_index in range(1, len(instructions)):
        # A direct predecessor proves that no path bypasses or overwrites
        # the flags from this exact whole-register test.
        if predecessors.get(branch_index) != {branch_index - 1}:
            continue
        tested = _cc_receiver_instructions._exact_whole_register_zero_test(instructions[branch_index - 1])
        if tested is None:
            continue
        branch = instructions[branch_index]
        mnemonic = _cc_cfg._instruction_mnemonic(branch)
        try:
            body = bytes(int(item, 16) for item in branch.bytes)
        except (TypeError, ValueError):
            continue
        zero_jump = mnemonic in {"je", "jz"}
        nonzero_jump = mnemonic in {"jne", "jnz"}
        opcode = 0x74 if zero_jump else 0x75
        if not (zero_jump or nonzero_jump):
            continue
        if len(body) == 2 and body[0] == opcode:
            displacement = struct.unpack_from("<b", body, 1)[0]
        elif len(body) == 6 and body[:2] == bytes((0x0F, opcode + 0x10)):
            displacement = struct.unpack_from("<i", body, 2)[0]
        else:
            continue
        address = addresses[branch_index]
        target = index_by_address.get(address + len(body) + displacement) if address is not None else None
        if target is None or target == branch_index + 1 or set(successors.get(branch_index, ())) != {target, branch_index + 1}:
            continue
        nonzero_edges[(branch_index, branch_index + 1 if zero_jump else target)] = tested

    unknown = frozenset({""})

    def merge_values(
        left: frozenset[str], right: frozenset[str]
    ) -> frozenset[str]:
        values = left | right
        if "" in values or len(values) > 4:
            return unknown
        return values

    def exact_value(values: frozenset[str]) -> str:
        return next(iter(values)) if len(values) == 1 and "" not in values else ""

    def exact_targetless_runtime_value(values: frozenset[str]) -> str:
        """Compose one bounded path-union of exact runtime object roots.

        A conditional factory/result path may join the address of an embedded
        fallback object before both paths load the same receiver field and
        vptr.  The joined object deliberately has no static callee identity.
        Preserve the exact, sorted alternatives only when every predecessor
        contributes one small non-IAT object root; unknown, stack, entry-
        register, null, oversized, or conflicting storage remains unresolved.
        """

        if len(values) < 2 or len(values) > 4 or "" in values:
            return ""
        root_re = re.compile(
            r"(?:"
            r"this"
            r"|call-result\([^()]+\)"
            r"|storage:[A-Za-z0-9_:@?.\\/]+"
            r")(?:[+-]0x[0-9a-f]+)*"
        )
        if any(
            len(value) > 256
            or root_re.fullmatch(value) is None
            or any(
                forbidden in value
                for forbidden in (
                    "iat:",
                    "entry-register(",
                    "entry-stack",
                    "stack",
                    "frame",
                    "null",
                )
            )
            for value in values
        ):
            return ""
        return "runtime-object-join(" + ",".join(sorted(values)) + ")"

    def exact_state_value(values: frozenset[str], *, scalar_operand: bool = False) -> str:
        direct = exact_value(values)
        if direct or not values or "" in values:
            return direct
        if len(values) == 2 and "null" in values:
            allocated = next(iter(values - {"null"}))
            if re.fullmatch(r"allocation-result\(provider:recoil:function:0x4c5b76,[0-9]+\)", allocated):
                # Preserve the null alternative explicitly. In contrast to
                # a nonzero edge, this never discharges a nullable receiver.
                return f"nullable({allocated})"
        # Factor identical loads only when publishing the result. During
        # iteration each path retains its own expression: stringifying a
        # partial join inside a transfer is not monotone across backedges.
        if all(value.startswith("load(") and value.endswith(")") for value in values):
            inner = exact_state_value(frozenset(value[5:-1] for value in values))
            if inner:
                return f"load({inner})"
        displaced = [re.fullmatch(r"(.+)([+-]0x[0-9a-f]+)", value) for value in values]
        if all(displaced) and len({match.group(2) for match in displaced}) == 1:
            inner = exact_state_value(frozenset(match.group(1) for match in displaced))
            if inner:
                return inner + displaced[0].group(2)
        # A scalar phi is eligible only as an operand of exact arithmetic.
        # In particular, do not turn a nullable object pointer into a valid
        # dereference by publishing its null arm as an integer alternative.
        if scalar_operand and all(value == "null" or _cc_receiver_proofs._bounded_scalar_index_provenance(value) for value in values):
            return "scalar-choice(" + ",".join(sorted(values)) + ")"
        return exact_targetless_runtime_value(values)

    def displaced_values(values: frozenset[str], displacement: int) -> frozenset[str]:
        if "" in values:
            return unknown
        # Normalize an address at its own load level, including a biased
        # embedded-this cursor seed. The wrapper introduces no real load.
        return frozenset(_cc_receiver_equivalence._canonical_proven_member_storage(
            "load(" + _cc_receiver_instructions._abstract_with_displacement(value, displacement) + ")")[5:-1]
            for value in values)

    entry = {name: unknown for name in _cc_catalog.REGISTER_STATE_NAMES}
    entry.update(
        {
            "ecx": frozenset({"this"}),
            "edx": frozenset({"entry-register(edx)"}),
            "esp": frozenset({"stack"}),
            "ebp": frozenset({"frame"}),
            "$stack-delta": frozenset({"0"}),
            "$stack-base": frozenset({"entry"}),
        }
    )
    direct_cleanups = dict(call_cleanup_by_instruction_index or {})
    if any(
        type(index) is not int
        or not 0 <= index < len(instructions)
        or type(cleanup) is not int
        or cleanup < 0
        or cleanup % 4
        for index, cleanup in direct_cleanups.items()
    ):
        return ""

    def exact_stack_delta(state: Mapping[str, frozenset[str]]) -> int | None:
        if not exact_value(state.get("$stack-base", unknown)):
            return None
        value = exact_value(state.get("$stack-delta", unknown))
        if re.fullmatch(r"-?\d+", value) is None:
            return None
        return int(value, 10)

    def set_stack_delta(
        state: dict[str, frozenset[str]], value: int | None
    ) -> None:
        state["$stack-delta"] = (
            frozenset({str(value)}) if value is not None else unknown
        )

    def forget_stack_slots(state: dict[str, frozenset[str]], offset: int | None = None, size: int = 4) -> None:
        for key in tuple(state):
            if key.startswith("stack-slot:") and (
                offset is None or offset < int(key.rsplit(":", 1)[1]) + 4
                and int(key.rsplit(":", 1)[1]) < offset + size
            ):
                del state[key]

    incoming: dict[int, dict[str, frozenset[str]]] = {0: entry}
    edge_states: dict[
        tuple[int, int], dict[str, frozenset[str]]
    ] = {}
    pending = [0]
    observed_after = unknown
    while pending:
        index = pending.pop()
        state = incoming[index]
        # Observe the incoming value only after convergence. Stopping at the
        # queried call would omit later arrivals through its own backedges.
        instruction = instructions[index]
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        outgoing = dict(state)
        if mnemonic in {"call", "jmp"}:
            if mnemonic == "call":
                for volatile in ("eax", "ecx", "edx"):
                    outgoing[volatile] = unknown
                try:
                    body = bytes(int(item, 16) for item in instruction.bytes)
                except (TypeError, ValueError):
                    body = b""
                address = addresses[index]
                if len(body) == 5 and body[0] == 0xE8 and address is not None:
                    target = normalize_address(
                        address + 5 + struct.unpack_from("<i", body, 1)[0]
                    )
                    identity = (
                        (direct_call_identities or {}).get(index, "")
                        if source == "cod" else indexes.by_address.get(target, "")
                    )
                    if identity:
                        outgoing["eax"] = frozenset(
                            {allocation_results.get(index, f"call-result({identity})")}
                        )
                delta = exact_stack_delta(state)
                cleanup = direct_cleanups.get(index)
                if (
                    cleanup is None
                    and allow_exact_caller_cleanup
                    and _cc_cfg._cleanup_after(instructions, index) is not None
                ):
                    # One exact caller-side ADD ESP owns the argument cleanup,
                    # so the callee itself must return without an immediate
                    # stack adjustment.  This remains stricter than assuming
                    # cdecl from a symbol spelling: an unknown call without
                    # the bounded caller-cleanup row still destroys the stack
                    # coordinate at this edge.
                    cleanup = 0
                if cleanup is None or delta is None:
                    # The return ESP becomes a new unknown origin. Discard
                    # every old frame slot. Subsequent spills/reloads can be
                    # proved relative to this origin without guessing its
                    # displacement from entry ESP or a callee cleanup amount.
                    # A known callee cannot restore a lost incoming coordinate,
                    # but its own return still supplies this fresh origin.
                    forget_stack_slots(outgoing)
                    outgoing["$stack-base"] = frozenset({f"after-call:{index}"})
                    set_stack_delta(outgoing, 0)
                else:
                    set_stack_delta(outgoing, delta + cleanup if delta is not None else None)
        else:
            destination = ""
            values = unknown
            move = _cc_receiver_instructions._exact_register_move(instruction)
            register_add = _cc_receiver_instructions._exact_register_step_immediate(instruction)
            register_lea = _cc_receiver_instructions._exact_register_lea(instruction)
            stack_store = _cc_receiver_instructions._exact_stack_slot_store(instruction)
            stack_load = _cc_receiver_instructions._exact_stack_slot_load(instruction)
            if allow_coff_symbolic_stack_operands:
                # The candidate entry point has authenticated the complete
                # listing against current COFF. VC5's named stack operands
                # include a symbolic local bias; use their encoded ESP slot.
                symbolic_load = _cc_receiver_instructions._exact_vc5_symbolic_stack_slot_load(instruction)
                symbolic_store = _cc_receiver_instructions._exact_vc5_symbolic_stack_slot_store(instruction)
                if stack_load is None and symbolic_load is not None:
                    stack_load = (symbolic_load[1], symbolic_load[0])
                if stack_store is None and symbolic_store is not None:
                    stack_store = (symbolic_store[0], symbolic_store[1])
            memory_load = _cc_receiver_candidate._exact_register_memory_load(instruction)
            affine_load = _cc_targets._exact_indexed_affine_load(instruction)
            register_sum = _cc_receiver_proofs._exact_register_add_register(instruction)
            shift_left = _cc_receiver_instructions._exact_register_shift_left_immediate(instruction)
            multiply = _cc_receiver_instructions._exact_register_imul_immediate(instruction)
            try:
                body = bytes(int(item, 16) for item in instruction.bytes)
            except (TypeError, ValueError):
                body = b""
            stack_delta = exact_stack_delta(state)
            first_operand = _cc_cfg._instruction_operand(instruction).split(",", 1)[0]
            if (stack_store is None and "[" in first_operand
                    and re.search(r"\besp\b", first_operand, re.IGNORECASE)
                    and mnemonic not in {"cmp", "test", "push", "call", "jmp"}):
                # Invalidate every pointer spill touched by the encoded write.
                # An EH state byte elsewhere in the frame leaves unrelated
                # spills intact; an undecoded write destroys all slot facts.
                fact = _cc_instructions.instruction_fact(instruction)
                operand = fact.operands[0] if fact.operands else None
                if (stack_delta is not None and operand is not None
                        and operand.kind == "memory" and operand.base == "esp"
                        and not operand.index and not operand.segment and operand.size > 0):
                    forget_stack_slots(outgoing, stack_delta + operand.displacement, operand.size)
                else:
                    forget_stack_slots(outgoing)
            register_push = (
                len(body) == 1
                and 0x50 <= body[0] <= 0x57
                and mnemonic == "push"
                and _cc_cfg._instruction_operand(instruction).strip().lower() == _ENCODED_REGISTERS[body[0] - 0x50]
            )
            register_pop = (
                len(body) == 1
                and 0x58 <= body[0] <= 0x5F
                and mnemonic == "pop"
                and _cc_cfg._instruction_operand(instruction).strip().lower() == _ENCODED_REGISTERS[body[0] - 0x58]
            )
            coff_symbol_push = (
                allow_coff_symbolic_stack_operands
                and mnemonic == "push"
                and len(body) == 5 and body[0] == 0x68
                and re.fullmatch(r"(?:OFFSET\s+FLAT:)?[?@$A-Za-z_][?@$A-Za-z_0-9]*",
                    _cc_cfg._instruction_operand(instruction).strip(), re.IGNORECASE) is not None
            )
            if register_push or _cc_identity._exact_unknown_push32(instruction) or coff_symbol_push:
                forget_stack_slots(outgoing, stack_delta - 4 if stack_delta is not None else None)
                set_stack_delta(
                    outgoing,
                    stack_delta - 4 if stack_delta is not None else None,
                )
            elif register_pop or _cc_identity._exact_unknown_pop32(instruction):
                set_stack_delta(
                    outgoing,
                    stack_delta + 4 if stack_delta is not None else None,
                )
                if register_pop:
                    outgoing[_ENCODED_REGISTERS[body[0] - 0x58]] = unknown
                    if body[0] == 0x5C:
                        set_stack_delta(outgoing, None)
            elif mnemonic in {"push", "pop", "pushf", "pushfd", "pusha", "pushad", "popf", "popfd", "popa", "popad", "enter", "leave"}:
                # An unproved implicit ESP write destroys the coordinate;
                # ignoring it would relabel a later argument as a different one.
                set_stack_delta(outgoing, None)
                if mnemonic in {"pop", "popa", "popad"}:
                    for register_name in _ENCODED_REGISTERS:
                        outgoing[register_name] = unknown
            elif (
                mnemonic in {"add", "sub"}
                and re.fullmatch(
                    r"(?:add|sub)\s+esp\s*,\s*(?:0x[0-9a-f]+|\d+)",
                    instruction.raw_text.strip(),
                    flags=re.IGNORECASE,
                )
                is not None
            ):
                immediate = _cc_cfg._parse_unsigned_assembly_integer(
                    _cc_cfg._instruction_operand(instruction).split(",", 1)[1].strip()
                )
                if not _cc_receiver_storage._is_exact_safe_stack_root_adjustment(
                    instruction,
                    operation=mnemonic,
                    parsed_immediate=immediate,
                ):
                    set_stack_delta(outgoing, None)
                else:
                    set_stack_delta(
                        outgoing,
                        (
                            stack_delta
                            + (immediate if mnemonic == "add" else -immediate)
                            if stack_delta is not None
                            else None
                        ),
                    )
            elif stack_store is not None:
                stack_displacement, stack_source = stack_store
                if stack_delta is not None:
                    forget_stack_slots(outgoing, stack_delta + stack_displacement)
                    outgoing[
                        f"stack-slot:{exact_value(state.get('$stack-base', unknown))}:{stack_delta + stack_displacement}"
                    ] = state.get(stack_source, unknown)
                    if index in spilled_values:
                        reg, marker = spilled_values[index]
                        outgoing[reg] = frozenset({marker})
                        outgoing[f"stack-slot:{exact_value(state.get('$stack-base', unknown))}:{stack_delta + stack_displacement}"] = frozenset({marker})
            elif index in spilled_values:
                destination, marker = spilled_values[index]
                values = frozenset({marker})
            elif index in stride_seeds:
                destination, marker = stride_seeds[index]
                initial = _cc_receiver_instructions._exact_register_move_immediate(instruction)
                if (_cc_receiver_proofs._exact_zero_register(instruction) == destination
                        or initial is not None and initial[0] == destination
                        or exact_state_value(state.get(destination, unknown)) == marker):
                    values = frozenset({marker})
            elif (zero_register := _cc_receiver_proofs._exact_zero_register(instruction)) is not None and zero_register != "esp":
                destination, values = zero_register, frozenset({"null"})
            elif move is not None:
                destination = move[0]
                values = state.get(move[1], unknown)
            elif allow_exact_affine_receiver_roots and register_sum is not None:
                destination, source_register = register_sum
                left = exact_state_value(state.get(destination, unknown), scalar_operand=True)
                right = exact_state_value(state.get(source_register, unknown), scalar_operand=True)
                combined = _cc_receiver_candidate._combine_bounded_stride_provenance(left, right)
                if combined:
                    values = frozenset({combined})
                elif _cc_receiver_candidate._bounded_targetless_pointer_base(left) and _cc_receiver_proofs._bounded_index_provenance(right):
                    values = frozenset({f"affine({left},{right}*1)"})
                elif _cc_receiver_candidate._bounded_targetless_pointer_base(right) and _cc_receiver_proofs._bounded_index_provenance(left):
                    values = frozenset({f"affine({right},{left}*1)"})
            elif allow_exact_affine_receiver_roots and (shift_left is not None or multiply is not None):
                if shift_left is not None:
                    destination, shift = shift_left
                    source_register, coefficient = destination, 1 << shift
                else:
                    destination, source_register, coefficient = multiply
                scaled = _cc_receiver_candidate._bounded_stride_provenance(
                    exact_state_value(state.get(source_register, unknown), scalar_operand=True), coefficient)
                if scaled:
                    values = frozenset({scaled})
            elif (
                allow_exact_affine_receiver_roots
                and register_add is not None
                and register_add[0] != "esp"
            ):
                destination = register_add[0]
                values = displaced_values(state.get(destination, unknown), register_add[1])
            elif (
                allow_exact_affine_receiver_roots
                and register_lea is not None
                and register_lea[0] != "esp"
            ):
                destination = register_lea[0]
                values = displaced_values(state.get(register_lea[1], unknown), register_lea[2])
            elif stack_load is not None:
                destination = stack_load[0]
                if stack_delta is not None:
                    absolute_slot = stack_delta + stack_load[1]
                    values = state.get(
                        f"stack-slot:{exact_value(state.get('$stack-base', unknown))}:{absolute_slot}",
                        frozenset(
                            {
                                "load("
                                f"{_cc_receiver_instructions._abstract_with_displacement('entry-stack', absolute_slot)}"
                                ")"
                            }
                        ) if state.get("$stack-base") == frozenset({"entry"}) else unknown,
                    )
            elif memory_load is not None:
                destination, base, displacement = memory_load
                bases = displaced_values(state.get(base, unknown), displacement)
                if "" not in bases:
                    values = frozenset(_cc_receiver_equivalence._canonical_proven_member_storage(
                        f"load({base_value})") for base_value in bases)
            elif affine_load is not None:
                (
                    destination,
                    base,
                    index_register,
                    scale,
                    displacement,
                    _mnemonic,
                ) = affine_load
                base_value = exact_state_value(state.get(base, unknown), scalar_operand=_mnemonic == "lea")
                index_value = exact_state_value(state.get(index_register, unknown), scalar_operand=_mnemonic == "lea")
                # An exact member-sourced index is a value, not its temporary
                # register name. Preserve its load, scale and array offset.
                # This proves address lineage, not an array capacity bound.
                if (
                    _mnemonic == "mov"
                    and
                    base_value == "this"
                    and scale == 4
                    and 0 < displacement <= 0x7FFFFFFF
                    and _cc_receiver_candidate._exact_cfg_index_source(index_value)
                ):
                    values = frozenset(
                        {
                            f"load(affine(this,{index_value}*4,+0x{displacement:x}))"
                        }
                    )
                elif allow_exact_affine_receiver_roots:
                    combined = (_cc_receiver_candidate._combine_bounded_stride_provenance(base_value, index_value,
                        right_scale=scale) if _mnemonic == "lea" and displacement == 0 else "")
                    pointer, scalar, scalar_scale = base_value, index_value, scale
                    if (scale == 1 and _cc_receiver_candidate._bounded_targetless_pointer_base(index_value)
                            and _cc_receiver_proofs._bounded_index_provenance(base_value)):
                        pointer, scalar = index_value, base_value
                    if scalar.startswith("bounded-stride("):
                        scaled_index = _cc_receiver_candidate._bounded_stride_provenance(scalar, scalar_scale)
                        if _cc_receiver_proofs._bounded_index_provenance(scaled_index):
                            scalar, scalar_scale = scaled_index, 1
                    if combined:
                        values = frozenset({combined})
                    elif (_cc_receiver_candidate._bounded_targetless_pointer_base(pointer)
                            and _cc_receiver_proofs._bounded_index_provenance(scalar)):
                        displacement_text = f",{displacement:+#x}" if displacement else ""
                        address = f"affine({pointer},{scalar}*{scalar_scale}{displacement_text})"
                        values = frozenset({f"load({address})" if _mnemonic == "mov" else address})
            else:
                if source == "bn" and mnemonic == "mov" and len(body) == 5 and body[0] == 0xA1:
                    destination = "eax"
                    absolute = normalize_address(struct.unpack_from("<I", body, 1)[0])
                    identity = indexes.storage_by_address.get(absolute, "")
                    if identity and not identity.startswith("iat:"):
                        values = frozenset({identity})
                elif (
                    len(body) == 6
                    and body[0] == 0x8B
                    and body[1] >> 6 == 0
                    and body[1] & 7 == 5
                ):
                    destination = (
                        "eax", "ecx", "edx", "ebx",
                        "esp", "ebp", "esi", "edi",
                    )[(body[1] >> 3) & 7]
                    absolute = normalize_address(
                        struct.unpack_from("<I", body, 2)[0]
                    )
                    identity = indexes.storage_by_address.get(absolute, "")
                    if identity and not identity.startswith("iat:"):
                        values = frozenset({identity})
                if not destination:
                    for candidate in _cc_catalog.REGISTER_STATE_NAMES:
                        if _cc_cfg._instruction_may_clobber_register(
                            instruction, candidate
                        ):
                            outgoing[candidate] = unknown
            if destination:
                cursor = cursor_seeds.get(index)
                if cursor is not None and index not in stride_seeds and cursor[0] == destination:
                    if cursor[2]:
                        values = frozenset({cursor_initial_values[index]}) if index in cursor_initial_values else unknown
                    else:
                        prior = exact_state_value(state.get(destination, unknown))
                        if prior.startswith("cursor(") and prior.endswith(f",{cursor[1]:+#x})"):
                            values = frozenset({prior})
                outgoing[destination] = values

        # A current COFF relocation is not its literal object addend. Kill
        # only its produced registers; unrelated definitions and later exact
        # redefinitions remain usable. No linked target is inferred here.
        for opaque_register in (opaque_register_writes or {}).get(index, ()):
            outgoing[opaque_register] = unknown
            if opaque_register == "esp":
                set_stack_delta(outgoing, None)

        if index == before_index:
            observed_after = outgoing.get(register, unknown)

        for successor in successors.get(index, ()):
            if successor not in reaching_target:
                continue
            edge_state = dict(outgoing)
            tested = nonzero_edges.get((index, successor))
            if tested is not None:
                alternatives = edge_state.get(tested, unknown)
                if alternatives == frozenset({"null"}):
                    continue
                if "" not in alternatives:
                    edge_state[tested] = alternatives - {"null"}
            # The zero edge retains all runtime expressions: a loaded
            # pointer can itself be zero. Only explicit null is excluded
            # on the nonzero edge, and no equal-valued register is refined.
            edge_states[(index, successor)] = edge_state
            contributor_states = [
                edge_states[(predecessor, successor)]
                for predecessor in sorted(predecessors.get(successor, ()))
                if (predecessor, successor) in edge_states
            ]
            if successor == 0:
                contributor_states.insert(0, entry)
            if not contributor_states:
                continue
            merged = dict(contributor_states[0])
            for contributor in contributor_states[1:]:
                state_names = set(merged) | set(contributor)
                merged = {
                    name: merge_values(
                        merged.get(name, unknown),
                        contributor.get(name, unknown),
                    )
                    for name in state_names
                }
            if merged != incoming.get(successor):
                incoming[successor] = merged
                pending.append(successor)

    result_values = observed_after if observe_after else incoming.get(before_index, {}).get(register, unknown)
    return exact_state_value(result_values)


def _dead_saved_alias_is_referenced_or_redefined(
    instruction: Instruction,
    alias: str,
) -> bool:
    """Reject every explicit or implicit alias use after its exact restore."""

    mnemonic = _cc_cfg._instruction_mnemonic(instruction)
    return bool(
        re.search(
            rf"\b{re.escape(alias)}\b",
            instruction.raw_text,
            flags=re.IGNORECASE,
        )
        or _cc_cfg._instruction_may_clobber_register(instruction, alias)
        or mnemonic in {"pusha", "pushad", "popa", "popad"}
    )


def _exact_candidate_saved_entry_this_alias_lifetime(
    instructions: Sequence[Instruction],
    *,
    invocation_indices: Sequence[int],
) -> tuple[str, int, int] | None:
    """Prove one exact saved-nonvolatile entry-this alias lifetime."""

    if not instructions or not invocation_indices:
        return None

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

    aliases: list[tuple[str, int, int]] = []
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
        restores = [
            index
            for index, instruction in enumerate(instructions)
            if _cc_receiver_instructions._exact_register_stack_transfer(instruction) == ("pop", alias)
        ]
        if not (
            len(assignments) == len(saves) == len(restores) == 1
            and saves[0] < assignments[0] < restores[0]
        ):
            continue
        assignment_index = assignments[0]
        restore_index = restores[0]
        if any(
            _cc_cfg._instruction_mnemonic(instructions[index]).startswith("j")
            or _cc_cfg._instruction_mnemonic(instructions[index]).startswith("loop")
            or _cc_cfg._instruction_mnemonic(instructions[index]) in {"call", "jmp"}
            or _cc_cfg._instruction_may_clobber_register(instructions[index], "ecx")
            for index in range(assignment_index)
            if index != saves[0]
        ):
            continue
        if any(
            _cc_cfg._instruction_mnemonic(instructions[index]).startswith("j")
            or _cc_cfg._instruction_mnemonic(instructions[index]).startswith("loop")
            or _cc_cfg._instruction_may_clobber_register(instructions[index], alias)
            or alias_escapes(instructions[index], alias)
            for index in range(assignment_index + 1, restore_index)
        ):
            continue
        returns = [
            index
            for index, instruction in enumerate(instructions)
            if _cc_cfg._instruction_mnemonic(instruction).startswith("ret")
        ]
        if (
            len(returns) != 1
            or restore_index >= returns[0]
            or any(index >= restore_index for index in invocation_indices)
            or any(
                _cc_cfg._instruction_mnemonic(instructions[index]).startswith("j")
                or _cc_cfg._instruction_mnemonic(instructions[index]).startswith("loop")
                or _cc_cfg._instruction_mnemonic(instructions[index]) in {"call", "jmp"}
                or re.search(
                    rf"\b{re.escape(alias)}\b",
                    instructions[index].raw_text,
                    flags=re.IGNORECASE,
                )
                is not None
                for index in range(restore_index + 1, returns[0])
            )
        ):
            continue
        aliases.append((alias, assignment_index, restore_index))
    if len(aliases) != 1:
        return None
    return aliases[0]
