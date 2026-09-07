"""Finite reaching-definition proof shared by retail and candidate lineages."""

from __future__ import annotations

from collections import deque
from dataclasses import dataclass
from typing import Any, Mapping, Sequence

from _recoil.call_contract import instructions as _cc_instructions


class FlowProofError(ValueError):
    pass


@dataclass(frozen=True)
class ControlFlow:
    successors: tuple[tuple[int, ...], ...]
    unresolved: frozenset[int] = frozenset()

    @classmethod
    def from_edges(cls, count: int, successors: Mapping[int, Sequence[int]],
                   unresolved: frozenset[int] = frozenset()) -> "ControlFlow":
        if count < 1 or any(index not in range(count) for index in successors) or any(
                target not in range(count) for targets in successors.values() for target in targets):
            raise FlowProofError("CFG contains an out-of-body edge")
        if any(index not in range(count) for index in unresolved):
            raise FlowProofError("CFG contains an out-of-body unresolved site")
        return cls(tuple(tuple(sorted(set(successors.get(index, ())))) for index in range(count)), unresolved)

    def reachable(self, roots: Sequence[int] = (0,)) -> frozenset[int]:
        pending = list(roots)
        result: set[int] = set()
        while pending:
            index = pending.pop()
            if index in result:
                continue
            if index not in range(len(self.successors)):
                raise FlowProofError("CFG root is outside the body")
            result.add(index)
            pending.extend(self.successors[index])
        return frozenset(result)


# Each pair records possible and definite *original-position* bits of the one
# selected definition. Partial writes cannot preserve a whole pointer. Joins
# union possibilities and intersect certainties, so a may-definition cannot
# become a must-definition by visiting one predecessor first.
Bits = tuple[int, int]
EMPTY: Bits = (0, 0)
WHOLE: Bits = (0xffffffff, 0xffffffff)


@dataclass(frozen=True)
class DefinitionState:
    registers: tuple[Bits, ...] = (EMPTY,) * 8
    stack: tuple[tuple[int, Bits], ...] = ()
    stack_pointer: int | None = 0
    frame_pointer: int | None = None

    def register(self, name: str) -> Bits:
        family, low, width = _cc_instructions.REGISTER_BITS.get(name, ("", 0, 0))
        if family not in _cc_instructions.GPRS:
            return EMPTY
        mask = ((1 << width) - 1) << low
        value = self.registers[_cc_instructions.GPRS.index(family)]
        return value[0] & mask, value[1] & mask


def _join(left: DefinitionState, right: DefinitionState) -> DefinitionState:
    registers = tuple((a[0] | b[0], a[1] & b[1]) for a, b in zip(left.registers, right.registers))
    sp = left.stack_pointer if left.stack_pointer == right.stack_pointer else None
    fp = left.frame_pointer if left.frame_pointer == right.frame_pointer else None
    a, b = dict(left.stack), dict(right.stack)
    stack = tuple((offset, (a.get(offset, EMPTY)[0] | b.get(offset, EMPTY)[0],
                           a.get(offset, EMPTY)[1] & b.get(offset, EMPTY)[1]))
                  for offset in sorted(a.keys() | b.keys())) if sp is not None else ()
    return DefinitionState(registers, stack, sp, fp)


def _transfer(fact: _cc_instructions.InstructionFact, state: DefinitionState, cleanup: int | None) -> DefinitionState:
    registers = list(state.registers)
    stack = dict(state.stack)
    sp, fp = state.stack_pointer, state.frame_pointer
    mnemonic = fact.mnemonic
    ops = fact.operands
    for name, low, width in fact.register_writes:
        mask = ~(((1 << width) - 1) << low) & 0xffffffff
        old = registers[_cc_instructions.GPRS.index(name)]
        registers[_cc_instructions.GPRS.index(name)] = old[0] & mask, old[1] & mask

    def slot(operand: Any) -> int | None:
        base = sp if operand.base == "esp" else fp if operand.base == "ebp" else None
        return base + operand.displacement if (base is not None and not operand.segment and not operand.index) else None

    def forget_overlapping(offset: int, size: int) -> None:
        for saved in tuple(stack):
            if saved < offset + size and offset < saved + 4:
                del stack[saved]

    if mnemonic == "mov" and len(ops) == 2:
        destination, source = ops
        value = EMPTY
        if source.kind == "register":
            value = state.register(source.register)
        elif source.kind == "memory" and source.size == 4:
            offset = slot(source)
            value = stack.get(offset, EMPTY)
        if destination.kind == "register" and destination.register in _cc_instructions.REGISTER_BITS:
            family, low, width = _cc_instructions.REGISTER_BITS[destination.register]
            # A partial move is tracked only when source and destination bits
            # retain their original positions; shifted AH/AL copies are not an
            # intact reaching definition of the imported pointer.
            source_low = _cc_instructions.REGISTER_BITS.get(source.register, ("", 0, 0))[1]
            if source.kind != "register" or source_low == low:
                mask = ((1 << width) - 1) << low
                old = registers[_cc_instructions.GPRS.index(family)]
                registers[_cc_instructions.GPRS.index(family)] = old[0] | (value[0] & mask), old[1] | (value[1] & mask)
        elif destination.kind == "memory":
            offset = slot(destination)
            if offset is None:
                stack.clear()  # Unknown alias may overwrite a saved pointer.
            else:
                forget_overlapping(offset, destination.size)
                if destination.size == 4 and value[0]:
                    stack[offset] = value
        if destination.kind == "register" and destination.register == "ebp":
            fp = sp if source.kind == "register" and source.register == "esp" else None
        if destination.kind == "register" and destination.register == "esp":
            sp = fp if source.kind == "register" and source.register == "ebp" else None
        if fact.writes("esp") and destination.register != "esp":
            sp = None
        if fact.writes("ebp") and destination.register != "ebp":
            fp = None
    elif mnemonic == "xchg" and len(ops) == 2 and all(op.kind == "register" and op.size == 4 for op in ops):
        for destination, source in ((ops[0], ops[1]), (ops[1], ops[0])):
            if destination.register in _cc_instructions.GPRS:
                registers[_cc_instructions.GPRS.index(destination.register)] = state.register(source.register)
        if fact.writes("esp"):
            sp = None
        if fact.writes("ebp"):
            fp = None
    elif mnemonic == "push" and len(ops) == 1 and ops[0].size == 4 and sp is not None:
        sp -= 4
        forget_overlapping(sp, 4)
        stack[sp] = state.register(ops[0].register) if ops[0].kind == "register" else EMPTY
    elif mnemonic == "pop" and len(ops) == 1 and ops[0].size == 4 and sp is not None:
        value = stack.pop(sp, EMPTY)
        sp += 4
        if ops[0].kind == "register" and ops[0].register in _cc_instructions.GPRS:
            registers[_cc_instructions.GPRS.index(ops[0].register)] = value
            if ops[0].register == "ebp":
                fp = None
            if ops[0].register == "esp":
                sp = None
        elif ops[0].kind == "memory":
            stack.clear()
    elif mnemonic in {"add", "sub"} and len(ops) == 2 and ops[0].register == "esp" and ops[1].kind == "immediate":
        if sp is not None:
            previous = sp
            sp += ops[1].immediate * (1 if mnemonic == "add" else -1)
            stack = {offset: value for offset, value in stack.items() if not previous <= offset < sp}
    elif fact.is_call:
        # A called function can mutate address-taken locals. Keeping cached
        # registers is valid; retaining saved memory needs a separate escape
        # proof, so this shared kernel forgets the stack at the call boundary.
        stack.clear()
        sp = sp + cleanup if sp is not None and cleanup is not None else None
    else:
        if fact.writes_memory:
            stack.clear()
        if fact.writes("esp"):
            sp = None
        if fact.writes("ebp"):
            fp = None
    if sp is None or abs(sp) > 0x100000 or len(stack) > 1024:
        sp, stack = None, {}
    return DefinitionState(tuple(registers), tuple(sorted(stack.items())), sp, fp)


def reaching_definition_uses(instructions: Sequence[Any], cfg: ControlFlow, *,
                             definition_index: int, register: str,
                             call_cleanup: Mapping[int, int] | None = None,
                             equivalent_definitions: Mapping[int, str] | None = None) -> tuple[int, ...]:
    """Return only indirect transfers must-reached by this exact definition.

    Unrelated indirect transfers are analyzed by their own obligations. They
    cannot borrow this definition merely because its register remains live.
    """
    count = len(instructions)
    if count != len(cfg.successors) or definition_index not in range(count) or register not in _cc_instructions.GPRS:
        raise FlowProofError("definition or CFG does not belong to this instruction body")
    definitions = dict(equivalent_definitions or {})
    if definition_index in definitions and definitions[definition_index] != register:
        raise FlowProofError("conflicting register for the selected definition")
    definitions[definition_index] = register
    if any(index not in range(count) or name not in _cc_instructions.GPRS for index, name in definitions.items()):
        raise FlowProofError("equivalent definition is outside the instruction body")
    cleanup_by_index = dict(call_cleanup or {})
    if any(index not in range(count) or type(value) is not int or value < 0 or value % 4
           for index, value in cleanup_by_index.items()):
        raise FlowProofError("malformed call cleanup evidence")
    # Data directives outside the reachable CFG do not masquerade as machine
    # instructions. Decode only actual reachable instruction boundaries.
    reachable = cfg.reachable()
    if cfg.unresolved & reachable:
        raise FlowProofError("unresolved CFG edge may introduce an unmodelled predecessor")
    facts = {}
    for index in reachable:
        try:
            facts[index] = _cc_instructions.instruction_fact(instructions[index])
        except ValueError as exc:
            raise FlowProofError(f"instruction {index} ({getattr(instructions[index], 'raw_text', '')!r}): {exc}") from exc
    incoming: dict[int, DefinitionState] = {0: DefinitionState()}
    pending = deque((0,))
    while pending:
        index = pending.popleft()
        state = incoming[index]
        outgoing = _transfer(facts[index], state, cleanup_by_index.get(index))
        if index in definitions:
            registers = list(outgoing.registers)
            registers[_cc_instructions.GPRS.index(definitions[index])] = WHOLE
            outgoing = DefinitionState(tuple(registers), outgoing.stack, outgoing.stack_pointer, outgoing.frame_pointer)
        for target in cfg.successors[index]:
            combined = _join(incoming[target], outgoing) if target in incoming else outgoing
            if combined != incoming.get(target):
                incoming[target] = combined
                pending.append(target)
    uses = []
    for index, state in sorted(incoming.items()):
        fact = facts[index]
        if not (fact.is_call or fact.is_jump) or len(fact.operands) != 1:
            continue
        operand = fact.operands[0]
        if operand.kind != "register":
            continue
        value = state.register(operand.register)
        if value[0] and value != WHOLE:
            raise FlowProofError(f"indirect transfer at instruction {index} has a partial or ambiguous reaching definition")
        if value == WHOLE:
            uses.append(index)
    return tuple(uses)
