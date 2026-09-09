"""Live, conservative x86 register-allocation equivalence and match policy.

This proof never treats a comment, a Pro answer, or a saved candidate as code
evidence. Callers must additionally prove relocations, identity and placement.
"""
from __future__ import annotations

from collections import deque
from dataclasses import dataclass
import re
from typing import Any, Mapping

from _recoil.call_contract.instructions import (
    GPRS, REGISTER_BITS, InstructionProofError, _decoder, decode_bytes,
)

MATCH_VERSION = 1
MATCH_LEVELS = frozenset({"byte", "instruction"})
INSTRUCTION_DIMENSIONS = ("object_instruction", "linked_body_instruction", "linked_instruction")
_ALL = (1 << 256) - 1
_FLAGS = 256


def _bits(register: str) -> tuple[int, ...]:
    if register not in REGISTER_BITS:
        raise InstructionProofError(f"unsupported register {register}")
    family, low, width = REGISTER_BITS[register]
    start = GPRS.index(family) * 32 + low
    return tuple(range(start, start + width))


def _related(state: tuple[int, ...], first: str, second: str) -> bool:
    if first not in REGISTER_BITS or second not in REGISTER_BITS:
        return first == second
    a, b = _bits(first), _bits(second)
    return len(a) == len(b) and all(state[x] & (1 << y) for x, y in zip(a, b))


def _meet(states: list[tuple[int, ...]]) -> tuple[int, ...]:
    result = list(states[0])
    for state in states[1:]:
        result = [a & b for a, b in zip(result, state)]
    return tuple(result)


def _return_registers(symbol: str) -> tuple[str, ...]:
    # Only the unambiguous primitive portion of VC5's decoration is used.
    match = re.search(r"@@(?:[A-V][AB][AEGI]|Y[AGI])([XDEFGHIJKMN])", symbol)
    if match:
        return () if match[1] in "XMN" else ("eax",)
    return ("eax", "edx")


def _call_inputs(symbol: str) -> tuple[str, ...]:
    value = symbol.removeprefix("__imp_")
    if symbol.startswith("__imp__") and re.fullmatch(r"_[A-Za-z_][A-Za-z_0-9]*(?:@[0-9]+)?", value):
        return ()  # canonical cdecl/stdcall decorated callable identity
    if re.fullmatch(r"@[A-Za-z_][A-Za-z_0-9]*@[0-9]+", value):
        return ("ecx", "edx")
    # Unknown conventions are safe only with identical incoming GPR state.
    return GPRS


@dataclass(frozen=True)
class _State:
    equality: tuple[int, ...]
    targets: tuple[str, ...]


def compare_instructions(retail: bytes, candidate: bytes, *,
                         relocations: list[Mapping[str, Any]] | tuple = (),
                         symbol: str = "") -> dict[str, Any]:
    """Prove equal effects under register reassignment; reject unknown cases.

    Inputs have already had *verified relocation fields* normalized by the
    caller. Equality is a cross-execution bit relation, intersected at CFG
    joins. It follows values through copies and separate temporary lifetimes.
    Memory equality is inductive: every store and effective address is checked.
    No memory value, branch, or unspecified instruction effect is guessed.
    """
    def fail(reason: str, offset: int | None = None) -> dict[str, Any]:
        return {"passed": False, "version": MATCH_VERSION, "reason": reason, "offset": offset}

    if len(retail) != len(candidate):
        return fail("instruction matching requires identical body extents")
    if retail == candidate:
        return {"passed": True, "version": MATCH_VERSION, "differences": [], "exact": True}
    try:
        left = list(_decoder().disasm(retail, 0))
        right = list(_decoder().disasm(candidate, 0))
        if sum(x.size for x in left) != len(retail) or sum(x.size for x in right) != len(candidate):
            return fail("body is not a complete instruction stream; embedded data needs an exact typed proof")
        if len(left) != len(right):
            return fail("instruction count differs")
        facts = []
        differences = []
        starts = {int(row.address): i for i, row in enumerate(left)}
        successors: list[list[int]] = []
        # Keep the supported effect set explicit. Identical but unknown
        # instructions cannot launder an earlier register divergence.
        supported = {"mov", "movzx", "movsx", "lea", "push", "pop", "add", "sub", "and", "or", "xor",
                     "cmp", "test", "inc", "dec", "neg", "not", "shl", "sal", "shr", "sar", "imul",
                     "mul", "div", "idiv", "cdq", "cwde", "xchg", "nop", "int3", "call", "ret", "jmp"}
        for i, (a, b) in enumerate(zip(left, right)):
            offset = int(a.address)
            af, bf = decode_bytes(bytes(a.bytes)), decode_bytes(bytes(b.bytes))
            if a.address != b.address or a.size != b.size or af.mnemonic != bf.mnemonic:
                return fail("instruction boundary, length or operation differs", offset)
            if list(a.prefix) != list(b.prefix) or len(af.operands) != len(bf.operands):
                return fail("instruction prefixes or operand count differ", offset)
            if af.mnemonic not in supported and not (af.is_jump or af.mnemonic.startswith("set")):
                return fail(f"unsupported instruction effect: {af.mnemonic}", offset)
            for x, y in zip(af.operands, bf.operands):
                if (x.kind, x.size, x.access) != (y.kind, y.size, y.access):
                    return fail("operand kind, width or access differs", offset)
                if x.kind == "immediate" and x.immediate != y.immediate:
                    return fail("immediate or control-flow target differs", offset)
                if x.kind == "register":
                    if (x.register in REGISTER_BITS) != (y.register in REGISTER_BITS):
                        return fail("register class differs", offset)
                    if x.register not in REGISTER_BITS and x.register != y.register:
                        return fail("non-GPR register differs", offset)
                    if "esp" in (x.register, y.register) and x.register != y.register:
                        return fail("stack pointer reassignment is forbidden", offset)
                if x.kind == "memory":
                    if (x.segment, x.displacement, x.scale) != (y.segment, y.displacement, y.scale):
                        return fail("memory segment, displacement or scale differs", offset)
                    if bool(x.base) != bool(y.base) or bool(x.index) != bool(y.index):
                        return fail("memory addressing shape differs", offset)
                    if "esp" in (x.base, y.base) and x.base != y.base:
                        return fail("stack addressing differs", offset)
            # Decoder equality alone must not accept alternate opcode forms,
            # redundant encodings or changes outside actual register selectors.
            allowed = [0] * a.size
            if a.modrm_offset != b.modrm_offset or a.disp_offset != b.disp_offset or a.imm_offset != b.imm_offset:
                return fail("operand encoding layout differs", offset)
            if a.modrm_offset:
                m = a.modrm_offset
                # Group opcodes use ModRM.reg as an opcode extension, never
                # as a register selector. That field must remain exact.
                group_opcodes = {0x80, 0x81, 0x82, 0x83, 0x8f, 0xc0, 0xc1, 0xc6, 0xc7,
                                 0xd0, 0xd1, 0xd2, 0xd3, 0xf6, 0xf7, 0xfe, 0xff}
                allowed[m] = 7 if a.bytes[m - 1] in group_opcodes else 0x3f
                if (a.modrm & 0xc0) != (b.modrm & 0xc0):
                    return fail("ModRM addressing mode differs", offset)
                if (a.modrm & 7) == 4 and (a.modrm & 0xc0) != 0xc0:
                    if (b.modrm & 7) != 4:
                        return fail("SIB addressing shape differs", offset)
                    allowed[m + 1] = 0x3f
            # Register-in-opcode forms: push/pop, inc/dec, mov immediate, xchg.
            opcode_index = next((j for j, x in enumerate(a.bytes) if x not in {0x66, 0x67, 0x26, 0x2e, 0x36, 0x3e, 0x64, 0x65, 0xf0, 0xf2, 0xf3}), 0)
            opcode = a.bytes[opcode_index]
            if 0x40 <= opcode <= 0x5f or 0xb0 <= opcode <= 0xbf or 0x91 <= opcode <= 0x97:
                allowed[opcode_index] = 7
            if any((x ^ y) & ~mask for x, y, mask in zip(a.bytes, b.bytes, allowed)):
                return fail("bytes differ outside register encoding fields", offset)
            if bytes(a.bytes) != bytes(b.bytes):
                differences.append({"offset": offset, "retail": bytes(a.bytes).hex(), "candidate": bytes(b.bytes).hex(),
                                    "retail_instruction": f"{a.mnemonic} {a.op_str}", "candidate_instruction": f"{b.mnemonic} {b.op_str}"})
            edges = []
            if af.is_jump:
                if not af.operands or af.operands[0].kind != "immediate":
                    return fail("indirect control flow requires an additional typed CFG proof", offset)
                target = int(a.operands[0].imm)
                if target not in starts:
                    return fail("branch target is outside the proved instruction graph", offset)
                edges.append(starts[target])
            if not af.is_return and af.mnemonic not in {"jmp", "int3"} and i + 1 < len(left):
                edges.append(i + 1)
            successors.append(edges)
            facts.append((af, bf))

        sites = {int(x["offset"]): str(x.get("target_symbol", "")) for x in relocations
                 if ":function:" in str(x.get("target_symbol_id", ""))}
        initial = _State(tuple([1 << i for i in range(256)] + [1]), ("",) * 8)
        incoming: dict[int, _State] = {0: initial}
        outgoing: dict[int, _State] = {}
        predecessors = [[] for _ in left]
        for i, edges in enumerate(successors):
            for j in edges:
                predecessors[j].append(i)
        queue = deque([0])
        errors: dict[int, str] = {}
        visits = 0

        def transfer(i: int, state: _State) -> _State:
            af, bf = facts[i]
            a, b = left[i], right[i]
            eq = state.equality
            ok = True
            reason = "register value or implicit operand is not equivalent"
            pairs: list[tuple[str, str]] = []
            explicit_a: set[str] = set()
            explicit_b: set[str] = set()
            zero = (af.mnemonic in {"xor", "sub"} and len(af.operands) == 2
                    and af.operands[0].kind == af.operands[1].kind == "register"
                    and bf.operands[0].kind == bf.operands[1].kind == "register"
                    and af.operands[0].register == af.operands[1].register
                    and bf.operands[0].register == bf.operands[1].register)
            for x, y in zip(af.operands, bf.operands):
                if x.kind == "register":
                    explicit_a.add(x.register); explicit_b.add(y.register)
                    if x.access & 1 and not zero:
                        ok &= _related(eq, x.register, y.register)
                    if x.access & 2 and x.register in REGISTER_BITS:
                        pairs.append((x.register, y.register))
                elif x.kind == "memory":
                    for r, c in ((x.base, y.base), (x.index, y.index)):
                        if r:
                            explicit_a.add(r); explicit_b.add(c)
                            ok &= _related(eq, r, c)
            implicit_a = set(af.register_reads) - explicit_a
            implicit_b = set(bf.register_reads) - explicit_b
            if implicit_a != implicit_b:
                ok = False
            for register in implicit_a:
                if register == "eflags":
                    ok &= bool(eq[_FLAGS])
                elif register in REGISTER_BITS:
                    ok &= _related(eq, register, register)
            targets = list(state.targets)
            call_target = ""
            if af.is_call:
                call_target = next((sites[p] for p in range(a.address, a.address + a.size) if p in sites), "")
                if af.operands[0].kind == "register":
                    call_target = targets[GPRS.index(REGISTER_BITS[af.operands[0].register][0])]
                for register in _call_inputs(call_target):
                    ok &= _related(eq, register, register)
                # Equal memory and ABI input values imply equal observable
                # callee effects. Unspecified scratch outputs remain unrelated.
                pairs.append(("eax", "eax"))
            if af.is_return:
                for register in (*_return_registers(symbol), "ebx", "ebp", "esi", "edi", "esp"):
                    ok &= _related(eq, register, register)
                reason = "return values, stack or preserved registers differ"
            written_a = set()
            written_b = set()
            for family, low, width in af.register_writes:
                written_a.update(range(GPRS.index(family) * 32 + low, GPRS.index(family) * 32 + low + width))
                targets[GPRS.index(family)] = ""
            for family, low, width in bf.register_writes:
                written_b.update(range(GPRS.index(family) * 32 + low, GPRS.index(family) * 32 + low + width))
            covered_a = {p for r, _ in pairs for p in _bits(r)}
            covered_b = {p for _, c in pairs for p in _bits(c)}
            implicit_writes_a = written_a - covered_a
            implicit_writes_b = written_b - covered_b
            if af.is_call:
                volatile = set(_bits("ecx")) | set(_bits("edx"))
                implicit_writes_a -= volatile; implicit_writes_b -= volatile
            if implicit_writes_a != implicit_writes_b:
                ok = False
            clear_mask = _ALL ^ sum(1 << x for x in written_b)
            result = [0 if p in written_a else value & clear_mask for p, value in enumerate(eq[:256])]
            if ok:
                for r, c in pairs:
                    for x, y in zip(_bits(r), _bits(c)):
                        result[x] |= 1 << y
                for p in implicit_writes_a & implicit_writes_b:
                    result[p] |= 1 << p
                # MOV copies retain cross-register equalities, essential when
                # a value is copied and then its old register is reused.
                if af.mnemonic == "mov" and len(af.operands) == 2 and af.operands[0].kind == "register":
                    x, y = af.operands[1], bf.operands[1]
                    rd, cd = _bits(af.operands[0].register), _bits(bf.operands[0].register)
                    if x.kind == y.kind == "register":
                        for dest, src in zip(rd, _bits(x.register)):
                            result[dest] |= eq[src] & clear_mask
                        for dest, src in zip(cd, _bits(y.register)):
                            for p in range(256):
                                if p not in written_a and eq[p] & (1 << src):
                                    result[p] |= 1 << dest
                    if len(rd) == 32:
                        target = next((sites[p] for p in range(a.address, a.address + a.size) if p in sites), "")
                        if x.kind == "register":
                            target = state.targets[GPRS.index(REGISTER_BITS[x.register][0])]
                        targets[GPRS.index(REGISTER_BITS[af.operands[0].register][0])] = target
            flags = eq[_FLAGS]
            if af.writes_flags:
                # INC/DEC preserve carry; a zero-count shift preserves all
                # flags. Never restore flag equality after an unknown call
                # merely because one of these partial flag writers executes.
                defines_branch_flags = af.mnemonic in {"add", "sub", "cmp", "neg", "and", "or", "xor", "test"}
                flags = int(ok and (defines_branch_flags or bool(eq[_FLAGS])))
                if af.mnemonic in {"mul", "imul", "div", "idiv"}:
                    flags = 0  # some condition flags are architecturally undefined
            if af.is_call:
                flags = 0
            result.append(flags)
            if ok:
                errors.pop(i, None)
            else:
                errors[i] = reason
            return _State(tuple(result), tuple(targets))

        while queue:
            i = queue.popleft()
            visits += 1
            if visits > max(10000, len(left) * 1024):
                return fail("register-equivalence fixed point did not converge")
            inputs = [outgoing[p] for p in predecessors[i] if p in outgoing]
            if i == 0:
                inputs.append(initial)
            if not inputs:
                continue
            eq = _meet([s.equality for s in inputs])
            targets = tuple(inputs[0].targets[j] if all(s.targets[j] == inputs[0].targets[j] for s in inputs) else "" for j in range(8))
            incoming[i] = _State(eq, targets)
            result = transfer(i, incoming[i])
            if outgoing.get(i) != result:
                outgoing[i] = result
                queue.extend(successors[i])
        if errors:
            i = min(errors)
            return fail(errors[i], int(left[i].address))
        if any(starts[x["offset"]] not in outgoing for x in differences):
            return fail("changed instruction has no proved reachable entry")
        return {"passed": True, "version": MATCH_VERSION, "exact": False,
                "differences": differences, "checked_instructions": len(outgoing),
                "proof": "paired-register-bit-dataflow"}
    except (InstructionProofError, ValueError, KeyError, IndexError) as exc:
        return fail(str(exc))
