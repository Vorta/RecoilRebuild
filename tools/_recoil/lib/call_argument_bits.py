"""Fail-closed, bit-exact argument provenance on an authenticated x86 CFG.

This small proof kernel handles register/constant bit operations and pushed
arguments. It is not an emulator: unsupported writes, cycles, unresolved edges,
and disagreeing reaching definitions provide no proof. The caller supplies the
live decoded instructions and exact CFG, never candidate-derived expectations.
"""
from __future__ import annotations

from functools import lru_cache
import re
from typing import Mapping, Sequence


class ArgumentProofError(ValueError):
    pass


_REGISTERS = {name: (name, 0, 32) for name in ("eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp")}
for _short, _full in (("ax", "eax"), ("bx", "ebx"), ("cx", "ecx"), ("dx", "edx"), ("si", "esi"), ("di", "edi"), ("bp", "ebp"), ("sp", "esp")):
    _REGISTERS[_short] = (_full, 0, 16)
for _letter in "abcd":
    _REGISTERS[_letter + "l"] = ("e" + _letter + "x", 0, 8)
    _REGISTERS[_letter + "h"] = ("e" + _letter + "x", 8, 8)


def _number(text: str) -> int:
    text = text.strip().lower()
    if re.fullmatch(r"-?\d+", text):
        return int(text)
    if re.fullmatch(r"0x[0-9a-f]+", text):
        return int(text, 16)
    if re.fullmatch(r"[0-9][0-9a-f]*h", text):
        return int(text[:-1], 16)
    raise ArgumentProofError(f"not an integer operand: {text}")


class ArgumentBits:
    def __init__(self, instructions: Sequence[str], successors: Mapping[int, Sequence[int]], unresolved: frozenset[int] = frozenset()):
        self.instructions = tuple(instructions)
        self.machine_facts = None
        self.parts = tuple(self._parts(text) for text in instructions)
        self.successors = successors
        self.unresolved = unresolved
        self.predecessors: dict[int, list[int]] = {i: [] for i in range(len(instructions))}
        for source, targets in successors.items():
            for target in targets:
                if source not in self.predecessors or target not in self.predecessors:
                    raise ArgumentProofError("CFG edge outside instruction body")
                self.predecessors[target].append(source)
        self.active: set[tuple[int, str, int]] = set()
        self.reachable: set[int] = set()
        pending = [0]
        while pending:
            index = pending.pop()
            if index in self.reachable:
                continue
            self.reachable.add(index)
            pending.extend(successors.get(index, ()))

    @classmethod
    def from_machine(cls, instructions, successors, unresolved=frozenset()):
        """Live proofs use byte-derived effects; text construction is algebra only."""
        from _recoil.call_contract.instructions import instruction_fact
        result = cls([instruction.text for instruction in instructions], successors, unresolved)
        result.machine_facts = {index: instruction_fact(instructions[index]) for index in result.reachable}
        for index, fact in result.machine_facts.items():
            mnemonic, operands = result.parts[index]
            if mnemonic != fact.mnemonic:
                raise ArgumentProofError(f"argument instruction rendering disagrees with machine opcode at {index}: {mnemonic!r} versus {fact.mnemonic!r}")
            if len(operands) != len(fact.operands):
                raise ArgumentProofError("argument instruction rendering has the wrong operand count")
            if mnemonic in {"push", "pop"} and fact.operands[0].size != 4:
                raise ArgumentProofError("argument proof requires a 32-bit stack operation")
            # Register and immediate operands must agree as well. Relocated
            # addresses are outside this selected scalar-argument proof.
            for spelling, operand in zip(operands, fact.operands):
                if operand.kind == "register" and spelling != operand.register:
                    raise ArgumentProofError("argument register rendering disagrees with machine operand")
                if operand.kind == "memory":
                    memory = re.fullmatch(r"(?:(byte|word|dword)(?: ptr)? )?\[(e(?:ax|bx|cx|dx|si|di|bp|sp))(?:(\+|-)(0x[0-9a-f]+|[0-9]+))?\]", spelling)
                    if memory:
                        displacement = _number(memory[4]) * (-1 if memory[3] == "-" else 1) if memory[4] else 0
                        if (operand.base != memory[2] or operand.displacement != displacement
                                or operand.index or operand.segment
                                or memory[1] and operand.size != {"byte": 1, "word": 2, "dword": 4}[memory[1]]):
                            raise ArgumentProofError("argument memory rendering disagrees with machine operand")
                if operand.kind == "immediate" and not (fact.is_call or fact.is_jump):
                    try:
                        rendered_value = _number(spelling)
                    except ArgumentProofError:
                        # A symbolic relocated address may be irrelevant to the
                        # selected scalar. operand_bit rejects it if selected.
                        continue
                    if rendered_value & 0xffffffff != operand.immediate & 0xffffffff:
                        raise ArgumentProofError("argument immediate rendering disagrees with machine operand")
        return result

    @staticmethod
    def _parts(text: str) -> tuple[str, tuple[str, ...]]:
        text = text.split(";", 1)[0].strip().lower()
        words = text.split(None, 1)
        mnemonic = {"retn": "ret", "sal": "shl", "jz": "je", "jnz": "jne"}.get(words[0], words[0])
        return mnemonic, tuple(part.strip() for part in words[1].split(",")) if len(words) > 1 else ()

    def _incoming(self, index: int) -> tuple[int, ...]:
        if index not in self.reachable:
            raise ArgumentProofError("argument site is not entry-reachable")
        return tuple(p for p in self.predecessors[index] if p in self.reachable)

    @staticmethod
    def _unanimous(values: Sequence[object]) -> object:
        if not values or any(value != values[0] for value in values[1:]):
            raise ArgumentProofError("argument has unknown or conflicting reaching definitions")
        return values[0]

    def operand_bit(self, before: int, operand: str, bit: int) -> object:
        if operand in _REGISTERS:
            register, low, width = _REGISTERS[operand]
            if bit >= width:
                return 0
            return self.register_bit(before, register, low + bit)
        memory = re.fullmatch(r"(byte|word|dword)(?: ptr)? \[(e(?:ax|bx|cx|dx|si|di|bp|sp))(?:(\+|-)(0x[0-9a-f]+|[0-9]+))?\]", operand)
        if memory:
            width = {"byte": 8, "word": 16, "dword": 32}[memory[1]]
            if bit >= width:
                return 0
            base = tuple(self.register_bit(before, memory[2], i) for i in range(32))
            displacement = _number(memory[4] or "0") * (-1 if memory[3] == "-" else 1)
            return ("load-bit", base, displacement, bit)
        return (_number(operand) >> bit) & 1

    @lru_cache(maxsize=None)
    def register_bit(self, before: int, register: str, bit: int) -> object:
        key = (before, register, bit)
        if key in self.active:
            raise ArgumentProofError("cyclic argument provenance")
        if before == 0:
            return ("entry-bit", register, bit)
        self.active.add(key)
        try:
            return self._unanimous([self._after(p, register, bit) for p in self._incoming(before)])
        finally:
            self.active.remove(key)

    def _after(self, index: int, register: str, bit: int) -> object:
        if index in self.unresolved:
            raise ArgumentProofError("unresolved CFG edge in argument provenance")
        mnemonic, operands = self.parts[index]
        if self.machine_facts is not None:
            fact = self.machine_facts[index]
            if not fact.writes(register, bit, 1):
                return self.register_bit(index, register, bit)
            destination = _REGISTERS.get(operands[0]) if operands else None
            if destination is None or destination[0] != register or not destination[1] <= bit < destination[1] + destination[2]:
                raise ArgumentProofError(f"unproved implicit argument register write: {mnemonic}")
        if mnemonic == "call":
            if register in {"eax", "ecx", "edx", "esp"}:
                raise ArgumentProofError("volatile argument value crosses a call")
            return self.register_bit(index, register, bit)
        if mnemonic in {"cmp", "test", "nop"} or mnemonic.startswith("j"):
            return self.register_bit(index, register, bit)
        if mnemonic == "push":
            if register == "esp":
                raise ArgumentProofError("stack-pointer arithmetic is outside argument bit proof")
            return self.register_bit(index, register, bit)
        if not operands:
            raise ArgumentProofError(f"unsupported implicit register effect: {mnemonic}")
        destination = _REGISTERS.get(operands[0])
        if destination is None:
            if mnemonic not in {"mov", "and", "or", "xor", "add", "sub", "inc", "dec"}:
                raise ArgumentProofError(f"unsupported memory instruction: {mnemonic}")
            return self.register_bit(index, register, bit)
        dest_register, low, width = destination
        if dest_register != register or not low <= bit < low + width:
            if mnemonic not in {"mov", "movzx", "movsx", "and", "or", "xor", "shl", "shr", "sal", "sar", "add", "sub", "lea", "inc", "dec", "pop"}:
                raise ArgumentProofError(f"unsupported implicit register effect: {mnemonic}")
            return self.register_bit(index, register, bit)
        relative = bit - low
        if mnemonic in {"mov", "movzx"} and len(operands) == 2:
            return self.operand_bit(index, operands[1], relative)
        if mnemonic == "xor" and operands[0] == operands[1]:
            return 0
        if mnemonic in {"and", "or"}:
            constant = (_number(operands[1]) >> relative) & 1
            if mnemonic == "and" and not constant:
                return 0
            if mnemonic == "or" and constant:
                return 1
            return self.register_bit(index, register, bit)
        if mnemonic in {"shl", "sal", "shr"}:
            shift = _number(operands[1]) & 31
            source_bit = relative - shift if mnemonic in {"shl", "sal"} else relative + shift
            return self.register_bit(index, register, low + source_bit) if 0 <= source_bit < width else 0
        raise ArgumentProofError(f"unsupported definition of argument: {self.instructions[index]}")

    def stack_argument(self, call_index: int, argument_index: int) -> tuple[object, ...]:
        """Return 32 exact bits; argument zero is top-of-stack before CALL."""
        if argument_index < 0 or self.parts[call_index][0] != "call":
            raise ArgumentProofError("invalid call argument selection")
        reaching: set[int] = set()
        pending = [call_index]
        while pending:
            index = pending.pop()
            if index in reaching:
                continue
            reaching.add(index)
            pending.extend(self.predecessors[index])
        if self.unresolved & self.reachable & reaching:
            raise ArgumentProofError("unresolved CFG edge reaching argument site")
        active: set[tuple[int, int]] = set()

        @lru_cache(maxsize=None)
        def seek(before: int, slot: int) -> tuple[object, ...]:
            key = (before, slot)
            if key in active or before == 0:
                raise ArgumentProofError("argument push is missing or cyclic")
            active.add(key)
            try:
                values = []
                for index in self._incoming(before):
                    if index in self.unresolved:
                        raise ArgumentProofError("unresolved argument stack edge")
                    mnemonic, operands = self.parts[index]
                    if mnemonic == "push":
                        values.append(tuple(self.operand_bit(index, operands[0], bit) for bit in range(32)) if slot == 0 else seek(index, slot - 1))
                    elif mnemonic in {"call", "pop", "enter", "leave", "pushad", "popad"} or (operands and operands[0] in {"esp", "sp"}):
                        raise ArgumentProofError("unproven stack change before argument")
                    elif operands and "[" in operands[0] and mnemonic not in {"cmp", "test"}:
                        raise ArgumentProofError("stack argument overwritten through memory")
                    elif self.machine_facts is not None and (
                            self.machine_facts[index].writes("esp") or self.machine_facts[index].writes_memory):
                        raise ArgumentProofError("unproven implicit stack or memory change before argument")
                    else:
                        values.append(seek(index, slot))
                return self._unanimous(values)
            finally:
                active.remove(key)
        return seek(call_index, argument_index)
