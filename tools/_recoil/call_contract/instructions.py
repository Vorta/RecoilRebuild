"""One byte-derived x86 instruction/effect model for Recoil proofs."""

from __future__ import annotations

from dataclasses import dataclass
from functools import lru_cache
from importlib.metadata import version
from typing import Any

import capstone
from capstone import CS_AC_WRITE, CS_ARCH_X86, CS_MODE_32, Cs
from capstone.x86 import X86_OP_IMM, X86_OP_MEM, X86_OP_REG

DECODER_VERSION = "5.0.9"
GPRS = ("eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi")
REGISTER_BITS = {name: (name, 0, 32) for name in GPRS}
REGISTER_BITS.update({short: (full, 0, 16) for short, full in
                      zip(("ax", "cx", "dx", "bx", "sp", "bp", "si", "di"), GPRS)})
REGISTER_BITS.update({letter + part: ("e" + letter + "x", bit, 8)
                      for letter in "acdb" for part, bit in (("l", 0), ("h", 8))})


class InstructionProofError(ValueError):
    pass


@dataclass(frozen=True)
class Operand:
    kind: str
    size: int
    register: str = ""
    immediate: int = 0
    segment: str = ""
    base: str = ""
    index: str = ""
    scale: int = 1
    displacement: int = 0
    access: int = 0


@dataclass(frozen=True)
class InstructionFact:
    data: bytes
    mnemonic: str
    operands: tuple[Operand, ...]
    register_reads: tuple[str, ...]
    register_writes: tuple[tuple[str, int, int], ...]
    writes_memory: bool
    writes_flags: bool
    is_call: bool
    is_jump: bool
    is_return: bool

    def writes(self, register: str, low: int = 0, width: int = 32) -> bool:
        family, base, register_width = REGISTER_BITS.get(register, (register, 0, width))
        low += base
        width = min(width, register_width)
        return any(name == family and start < low + width and low < start + count
                   for name, start, count in self.register_writes)


@lru_cache(maxsize=1)
def _decoder() -> Cs:
    if version("capstone") != DECODER_VERSION or capstone.cs_version()[:2] != (5, 0):
        raise InstructionProofError(f"call-contract decoder requires capstone {DECODER_VERSION}")
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    return decoder


@lru_cache(maxsize=32768)
def decode_bytes(data: bytes) -> InstructionFact:
    if not data or len(data) > 15:
        raise InstructionProofError("expected one nonempty x86 instruction of at most 15 bytes")
    rows = tuple(_decoder().disasm(data, 0))
    if len(rows) != 1 or rows[0].size != len(data):
        raise InstructionProofError("instruction bytes do not decode to exactly one complete instruction")
    row = rows[0]
    operands = []
    for operand in row.operands:
        if operand.type == X86_OP_REG:
            operands.append(Operand("register", operand.size, register=row.reg_name(operand.reg), access=operand.access))
        elif operand.type == X86_OP_IMM:
            operands.append(Operand("immediate", operand.size, immediate=operand.imm, access=operand.access))
        elif operand.type == X86_OP_MEM:
            memory = operand.mem
            operands.append(Operand("memory", operand.size, segment=row.reg_name(memory.segment) or "",
                base=row.reg_name(memory.base) or "", index=row.reg_name(memory.index) or "",
                scale=memory.scale, displacement=memory.disp, access=operand.access))
        else:
            raise InstructionProofError("decoder returned an unsupported operand kind")
    reads, writes = row.regs_access()
    names = {row.reg_name(register) for register in writes}
    # The 5.0 binding reports the accumulator as written by sign extension.
    # These exact encodings only write the high half of the dividend.
    if data in {b"\x99", b"\x66\x99"}:
        names.discard("eax")
        names.discard("ax")
    is_call = row.group(capstone.CS_GRP_CALL)
    # CALL's machine effect does not include callee effects. All Recoil call
    # proofs obey the Windows x86 caller-saved boundary unless separately
    # proving a complete callee's outputs. Stack cleanup is a separate fact.
    if is_call:
        names.update(("eax", "ecx", "edx", "esp"))
    register_writes = tuple(sorted({REGISTER_BITS[name] for name in names if name in REGISTER_BITS}))
    mnemonic = row.mnemonic
    # Capstone reports explicit memory writes. Stack and string operations also
    # write memory implicitly; unknown callee stores invalidate memory lineage.
    writes_memory = (is_call or any(op.kind == "memory" and op.access & CS_AC_WRITE for op in operands)
                     or mnemonic.split()[-1] in {"push", "pushal", "pushaw", "pushfd", "pushf", "enter",
                                                "stosb", "stosw", "stosd", "movsb", "movsw", "movsd",
                                                "insb", "insw", "insd", "fstenv", "fnstenv", "fnsave", "fsave"})
    return InstructionFact(data, mnemonic, tuple(operands), tuple(sorted(row.reg_name(r) for r in reads)),
        register_writes, writes_memory, "eflags" in names or is_call,
        is_call, row.group(capstone.CS_GRP_JUMP), row.group(capstone.CS_GRP_RET))


def instruction_fact(instruction: Any) -> InstructionFact:
    try:
        data = (bytes(instruction.bytes) if isinstance(instruction.bytes, (bytes, bytearray))
                else bytes(int(item, 16) for item in instruction.bytes))
    except (AttributeError, TypeError, ValueError) as exc:
        raise InstructionProofError("instruction lacks exact bytes") from exc
    return decode_bytes(data)


def may_clobber_register(instruction: Any, register: str) -> bool:
    try:
        return instruction_fact(instruction).writes(register)
    except InstructionProofError:
        return True


def written_registers(instruction: Any) -> frozenset[str]:
    """Whole register families touched by any explicit or implicit write."""
    try:
        return frozenset(name for name, _low, _width in instruction_fact(instruction).register_writes)
    except InstructionProofError:
        return frozenset(GPRS)
