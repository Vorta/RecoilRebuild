"""Narrow x87 operand-exchange proof, conditional on an explicit FP contract.

No opcode sorting, arithmetic reassociation, register erasure, or candidate-
derived table boundaries. Callers additionally prove relocations and linkage.
"""
from __future__ import annotations

from typing import Any, Mapping

from _recoil.call_contract.instructions import _decoder

COMMUTATIVE_VERSION = 1
BODY_PROOF_SCOPE = "normalized-function-body-only"
PENDING_OBLIGATIONS = ("relocation-type-target-addend-semantics", "linked-presence-and-identity",
                       "corresponding-linked-body-proof", "current-source-compiler-and-Pro-review")
COMMUTATIVE_CONTRACT = {
    "name": "finite-binary32-masked-x87",
    "inputs": "finite binary32 values at each affected read in valid ordinary stable memory",
    "environment": "same fixed supported x87 control word with all exceptions masked and enough free push slots for each proved region",
    "observations": "identical stored floating-point representations including signed zero, integer state, control flow and ABI",
    "continuations": "excluded x87 status, saved environment and dead physical registers cannot influence included observations through callers, callees or asynchronous inspection",
    "entry_model": "ordinary function entry and ABI call/return flow; no external interior entries or return-address manipulation",
}


def valid_contract(value: Any) -> bool:
    return isinstance(value, Mapping) and dict(value) == COMMUTATIVE_CONTRACT


def _memory(instruction):
    from capstone.x86_const import X86_OP_MEM
    if (len(instruction.operands) != 1 or instruction.operands[0].type != X86_OP_MEM
            or instruction.operands[0].size != 4 or any(instruction.prefix)):
        raise ValueError("only unprefixed binary32 memory operands are supported")
    operand = instruction.operands[0]
    memory = operand.mem
    return (instruction.reg_name(memory.segment), instruction.reg_name(memory.base),
            instruction.reg_name(memory.index), memory.scale, memory.disp, operand.size)


def _stack_index(instruction):
    from capstone.x86_const import X86_OP_REG
    operands = list(instruction.operands)
    if (instruction.mnemonic == "fxch" and len(operands) == 2
            and operands[0].type == X86_OP_REG and instruction.reg_name(operands[0].reg) == "st(0)"):
        operands.pop(0)
    if len(operands) != 1 or operands[0].type != X86_OP_REG:
        raise ValueError("unsupported x87 stack operand")
    name = instruction.reg_name(operands[0].reg)
    if not name.startswith("st(") or not name.endswith(")"):
        raise ValueError("unsupported x87 stack register")
    return int(name[3:-1])


def _region(left, right, begin, entries):
    """A local replacement above an opaque, untouched incoming x87 stack.

    No GPR writes, branches, calls or stores intervene before the final pop.
    Thus address tokens name the same memory epoch in both executions, even
    when input addresses alias. A branch into the region interior blocks it.
    """
    stacks = ([], [])
    swaps = []
    maximum_depth = 0
    for index in range(begin, len(left)):
        a, b = left[index], right[index]
        if index != begin and a.address != left[index - 1].address + left[index - 1].size:
            raise ValueError("noncode gap interrupts an x87 region")
        if index != begin and a.address in entries:
            raise ValueError("control flow enters an operand-exchange region")
        op = a.mnemonic
        if op not in {"fld", "fmul", "fxch", "fadd", "faddp", "fstp"}:
            raise ValueError("unsupported or intervening effect in x87 region")
        if op not in {"fld", "fmul"} and a.bytes != b.bytes:
            raise ValueError("only FLD/FMUL memory operand roles may differ")
        if op == "fld":
            for stack, ins in zip(stacks, (a, b)):
                stack.insert(0, ("memory", _memory(ins), int(ins.address)))
            maximum_depth = max(maximum_depth, len(stacks[0]))
            if maximum_depth > 8:
                raise ValueError("x87 region exceeds stack capacity")
        elif op == "fmul":
            if not stacks[0] or not stacks[1]:
                raise ValueError("multiplication reads an unproved incoming stack value")
            pairs = []
            for stack, ins in zip(stacks, (a, b)):
                value = stack[0]
                if value[0] != "memory":
                    raise ValueError("only a loaded binary32 factor times a binary32 memory factor is supported")
                pairs.append((value[1], _memory(ins)))
            if pairs[0] == pairs[1]:
                value = ("multiply", pairs[0], int(a.address))
            elif pairs[0] == tuple(reversed(pairs[1])):
                if stacks[0][0][2] != stacks[1][0][2]:
                    raise ValueError("operand exchange changes the originating load instruction")
                swaps.append({"load_offset": stacks[0][0][2], "multiply_offset": int(a.address),
                              "retail_factors": [list(x) for x in pairs[0]],
                              "candidate_factors": [list(x) for x in pairs[1]]})
                value = ("multiply", pairs[0], int(a.address))
            else:
                raise ValueError("multiplication factors are neither identical nor exchanged")
            stacks[0][0] = stacks[1][0] = value
        elif op == "fxch":
            slot = _stack_index(a)
            if not 0 < slot < len(stacks[0]):
                raise ValueError("exchange touches an unproved incoming stack value")
            for stack in stacks:
                stack[0], stack[slot] = stack[slot], stack[0]
        elif op in {"fadd", "faddp"}:
            if not stacks[0] or stacks[0] != stacks[1]:
                raise ValueError("ordered addition inputs differ")
            if op == "fadd":
                value = ("add", stacks[0][0], ("memory", _memory(a)), int(a.address))
                stacks[0][0] = stacks[1][0] = value
            else:
                slot = _stack_index(a)
                if not 0 < slot < len(stacks[0]):
                    raise ValueError("addition touches an unproved incoming stack value")
                value = ("add", stacks[0][slot], stacks[0][0], int(a.address))
                for stack in stacks:
                    stack[slot] = value
                    stack.pop(0)
        else:
            _memory(a)
            if len(stacks[0]) != 1 or stacks[0] != stacks[1]:
                raise ValueError("store value differs or region does not preserve the incoming stack")
            return index + 1, swaps, maximum_depth
    raise ValueError("unterminated x87 operand-exchange region")


def compare_commutative(retail: bytes, candidate: bytes, *, function_address: int,
                        contract: Mapping[str, Any] | None = None) -> dict[str, Any]:
    """Prove FLD m32/FMUL m32 factor exchanges with all other effects exact.

    This first proof supports no simultaneous GPR reassignment. It is explicitly
    conditional, not an all-input floating-point or architectural-state match.
    Numerical input validity and unobserved FP diagnostics are reviewed caller
    obligations; the machine proof does not infer them from a function name.
    """
    def result(**values):
        return {"version": COMMUTATIVE_VERSION, "scope": BODY_PROOF_SCOPE,
                "accepts_function_match": False, "accepts_exact_bytes": False,
                "pending_obligations": list(PENDING_OBLIGATIONS), **values}

    def fail(reason, offset=None):
        return result(passed=False, reason=reason, offset=offset)

    if len(retail) != len(candidate):
        return fail("commutative matching requires identical body extents")
    if retail == candidate:
        return result(passed=True, exact=True, normalized_buffers_equal=True, differences=[])
    if not valid_contract(contract):
        return fail("missing or unsupported explicit floating-point contract")
    try:
        from capstone import CS_GRP_CALL, CS_GRP_JUMP, CS_GRP_RET, CS_GRP_IRET, CS_GRP_INT
        from capstone.x86_const import X86_OP_IMM, X86_GRP_MMX, X86_GRP_BRANCH_RELATIVE
        from _recoil.commands.relocation_expectations import decode_x86_operand_sites

        spans = []
        control_targets = []
        sites, unknown = decode_x86_operand_sites(retail, function_address=function_address,
            instruction_spans=spans, control_flow_targets=control_targets)
        if unknown:
            return fail("retail instruction/table partition is unresolved: " + str(unknown[0]))
        decoder = _decoder()
        left, right, differences = [], [], []
        code_offsets = set()
        # The independent decoder owns all direct targets, including E0..E3
        # LOOP/JCXZ forms that Capstone does not uniformly classify as JUMP.
        entries = {0, *(target for _, target in control_targets if 0 <= target < len(retail))}
        encoded_control_targets = {(source, target & 0xffffffff) for source, target in control_targets}
        dispatches = set()
        for site in sites:
            if site.kind == "switch-table-entry":
                entries.add(int.from_bytes(retail[site.offset:site.offset + 4], "little") - function_address)
                dispatches.add(site.instruction_offset)
        for offset, size in spans:
            pair = [list(decoder.disasm(data[offset:offset + size], offset)) for data in (retail, candidate)]
            if any(len(items) != 1 or items[0].size != size for items in pair):
                return fail("candidate instruction boundary or length differs", offset)
            a, b = pair[0][0], pair[1][0]
            if a.group(X86_GRP_MMX):
                return fail("MMX can observe aliased physical x87 registers outside the numerical stack proof", offset)
            if a.mnemonic.startswith(("xsave", "xrstor")) or a.mnemonic in {"fnstsw", "fstsw", "fnstenv", "fstenv", "fnsave", "fsave", "fxsave",
                              "fxsave64", "fldenv", "frstor", "fxrstor", "fxrstor64", "fldcw",
                              "finit", "fninit", "fclex", "fnclex"}:
                return fail("function observes or changes the assumed x87 environment", offset)
            if (a.mnemonic, list(a.prefix), list(a.opcode), a.modrm_offset, a.disp_offset,
                    a.disp_size, a.imm_offset, a.imm_size) != (
                    b.mnemonic, list(b.prefix), list(b.opcode), b.modrm_offset, b.disp_offset,
                    b.disp_size, b.imm_offset, b.imm_size):
                return fail("instruction operation, form or encoding layout differs", offset)
            if a.bytes != b.bytes:
                if a.mnemonic not in {"fld", "fmul"}:
                    return fail("difference outside supported commutative operand roles", offset)
                _memory(a); _memory(b)
                # ModRM addressing mode and opcode extension stay exact. Only
                # the memory address selector/displacement may change roles.
                if a.modrm & 0xf8 != b.modrm & 0xf8:
                    return fail("memory addressing form or opcode extension differs", offset)
                differences.append({"offset": offset, "retail": bytes(a.bytes).hex(), "candidate": bytes(b.bytes).hex(),
                                    "retail_instruction": f"{a.mnemonic} {a.op_str}",
                                    "candidate_instruction": f"{b.mnemonic} {b.op_str}"})
            if a.group(CS_GRP_IRET) or a.group(CS_GRP_INT) or (a.group(CS_GRP_RET) and a.mnemonic != "ret"):
                return fail("control transfer is outside the ordinary ABI entry/return model", offset)
            if a.group(CS_GRP_JUMP) or a.group(CS_GRP_CALL) or a.group(X86_GRP_BRANCH_RELATIVE):
                if a.operands[0].type == X86_OP_IMM:
                    target = int(a.operands[0].imm)
                    if (offset, target & 0xffffffff) not in encoded_control_targets:
                        return fail("direct control transfer lacks an independent decoded target", offset)
                elif a.group(CS_GRP_CALL) or offset not in dispatches:
                    return fail("unproved indirect control flow", offset)
            left.append(a); right.append(b)
            code_offsets.update(range(offset, offset + size))
        starts = {int(x.address) for x in left}
        if not entries <= starts:
            return fail("control-flow target is not a decoded instruction boundary")
        if any(a != b for i, (a, b) in enumerate(zip(retail, candidate)) if i not in code_offsets):
            return fail("typed embedded data or padding differs")
        changed = {x["offset"] for x in differences}
        covered, exchanges, regions = set(), [], []
        for begin, instruction in enumerate(left):
            if instruction.mnemonic != "fld" or instruction.address in covered:
                continue
            try:
                end, swaps, depth = _region(left, right, begin, entries)
            except ValueError:
                continue  # An exact region needs no relaxation; every changed byte must still be covered.
            region_offsets = {int(x.address) for x in left[begin:end]}
            if swaps and region_offsets & changed:
                covered.update(region_offsets)
                exchanges.extend(swaps)
                regions.append({"start": int(instruction.address),
                                "end_exclusive": int(left[end - 1].address + left[end - 1].size),
                                "extra_stack_depth": depth})
        exchange_offsets = {offset for pair in exchanges for offset in (pair["load_offset"], pair["multiply_offset"])}
        if not changed or not changed <= covered or not changed <= exchange_offsets or not exchanges:
            return fail("not every changed operand has a balanced, single-entry x87 value-flow proof",
                        min(changed - covered) if changed - covered else None)
        return result(passed=True, exact=False, normalized_buffers_equal=False,
                proof="paired-x87-binary32-factor-exchange", conditional=True,
                contract=dict(contract), differences=differences, exchanges=exchanges,
                regions=regions, opaque_incoming_stack_preserved=True,
                stack_capacity_is_precondition=True, runtime_contract_proven=False,
                instruction_span_count_including_alignment=len(left),
                exact_noncode_bytes=len(retail) - len(code_offsets))
    except (ValueError, IndexError, KeyError, TypeError) as exc:
        return fail(str(exc))
