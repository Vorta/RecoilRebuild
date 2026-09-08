"""Recoil call-contract receiver candidate evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import receiver_equivalence as _cc_receiver_equivalence
from _recoil.call_contract import receiver_instructions as _cc_receiver_instructions
from _recoil.call_contract import receiver_proofs as _cc_receiver_proofs
from _recoil.call_contract import receiver_retail as _cc_receiver_retail
from _recoil.call_contract import receiver_storage as _cc_receiver_storage
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import CandidateCallerDefinition, IdentityIndexes

import re
import struct
from collections import Counter
from typing import Mapping, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_DIR32,
    IMAGE_REL_I386_REL32,
    Instruction,
)
from _recoil.lib.progress import normalize_address


def _split_abstract_arguments(value: str) -> tuple[str, ...] | None:
    """Split a symbolic-provenance argument list without losing nesting."""

    result: list[str] = []
    depth = 0
    start = 0
    for index, character in enumerate(value):
        if character == "(":
            depth += 1
        elif character == ")":
            depth -= 1
            if depth < 0:
                return None
        elif character == "," and depth == 0:
            result.append(value[start:index])
            start = index + 1
    if depth:
        return None
    result.append(value[start:])
    return tuple(result) if all(result) else None


def _bounded_stride_components(abstract: str) -> tuple[str, int] | None:
    """Return the unique scalar root and coefficient of safe arithmetic.

    Intermediate LEA/ADD/SHL products remain internal arithmetic evidence.
    Only the exact reviewed final strides are eligible as memory indexes.
    """

    if _cc_receiver_proofs._bounded_scalar_index_provenance(abstract):
        return abstract, 1
    for marker in ("bounded-stride", "index-scale"):
        prefix = marker + "("
        if not abstract.startswith(prefix) or not abstract.endswith(")"):
            continue
        arguments = _split_abstract_arguments(
            abstract[len(prefix) : -1]
        )
        if arguments is None or len(arguments) != 2:
            return None
        source = _bounded_stride_components(arguments[0])
        if source is None:
            return None
        try:
            coefficient = int(arguments[1], 0)
        except ValueError:
            return None
        total = source[1] * coefficient
        if (
            coefficient <= 0
            or total <= 0
            or total > _cc_catalog._BOUNDED_TARGETLESS_ARITHMETIC_LIMIT
        ):
            return None
        return source[0], total
    return None


def _bounded_stride_provenance(
    abstract: str,
    coefficient: int,
) -> str:
    components = _bounded_stride_components(abstract)
    if components is None:
        return ""
    root, prior = components
    total = prior * coefficient
    if (
        coefficient <= 0
        or total <= 0
        or total > _cc_catalog._BOUNDED_TARGETLESS_ARITHMETIC_LIMIT
    ):
        return ""
    return root if total == 1 else f"bounded-stride({root},0x{total:x})"


def _combine_bounded_stride_provenance(
    left: str,
    right: str,
    *,
    right_scale: int = 1,
) -> str:
    left_components = _bounded_stride_components(left)
    right_components = _bounded_stride_components(right)
    if (
        left_components is None
        or right_components is None
        or left_components[0] != right_components[0]
        or right_scale not in {1, 2, 4, 8}
    ):
        return ""
    total = left_components[1] + right_components[1] * right_scale
    if not 0 < total <= _cc_catalog._BOUNDED_TARGETLESS_ARITHMETIC_LIMIT:
        return ""
    return (
        left_components[0]
        if total == 1
        else f"bounded-stride({left_components[0]},0x{total:x})"
    )


def _bounded_targetless_pointer_base(abstract: str) -> bool:
    """Recognize one non-IAT object/storage pointer before index addition."""

    if _bounded_cfg_cursor(abstract) == "pointer":
        return True
    argument = re.fullmatch(
        r"(?:load\(entry-stack\+0x([0-9a-f]+)\)"
        r"|load\(load\(entry-stack\+0x([0-9a-f]+)\)\+0x([0-9a-f]+)\))"
        r"(?:\+0x([0-9a-f]+))?", abstract,
    )
    if argument is not None:
        slot = int(argument.group(1) or argument.group(2), 16)
        return (4 <= slot <= 0x7FFFFFFF and slot % 4 == 0
                and all(value is None or 0 < int(value, 16) <= 0x7FFFFFFF
                        for value in argument.groups()[2:]))

    if (
        not abstract
        or len(abstract) > 256
        or any(
            token in abstract
            for token in (
                "address(",
                "bounded-stride(",
                "call-result(",
                "frame",
                "iat:",
                "null",
                "stack",
            )
        )
    ):
        return False
    return (
        re.fullmatch(
            r"(?:this|storage:[A-Za-z0-9_:@?.\\/]+"
            r"|load\((?:this|storage:[A-Za-z0-9_:@?.\\/]+)"
            r"(?:[+-]0x[0-9a-f]+)?\))"
            r"(?:[+-]0x[0-9a-f]+)?",
            abstract,
        )
        is not None
    )


def _bounded_cfg_cursor(abstract: str) -> str:
    """Retain the initial value and signed step of one proved recurrence."""
    if len(abstract) > 512 or not abstract.startswith("cursor(") or not abstract.endswith(")"):
        return ""
    parts = _split_abstract_arguments(abstract[7:-1])
    if parts is None or len(parts) != 2 or "cursor(" in parts[0]:
        return ""
    try:
        step = int(parts[1], 0)
    except ValueError:
        return ""
    if abs(step) not in _cc_catalog.BOUNDED_TARGETLESS_VPTR_STRIDES:
        return ""
    initial = parts[0]
    if _cc_receiver_proofs._bounded_index_provenance(initial):
        return "index"
    if (_bounded_targetless_pointer_base(initial)
            or initial.startswith("affine(") and _eligible_retail_cfg_targetless_vptr(f"load({initial})")
            or re.fullmatch(r"call-result\((?:symbol|provider):recoil:function:0x[0-9a-f]+\)", initial) is not None):
        return "pointer"
    return ""


def _is_bounded_stack_vptr(abstract: str) -> bool:
    """Accept only bounded stack-rooted vptr and affine-index provenance."""
    # A proven stack-spilled loop index is not a stack-rooted receiver.
    # Preserve the entire known object -> array -> element -> interface path;
    # only the already-proven scalar recurrence may occupy the index term.
    indexed_field = re.fullmatch(
        r"load\(load\(load\(affine\(load\("
        r"(?:this|storage:[A-Za-z0-9_:@?.\\/]+"
        r"|call-result\((?:symbol|provider):[A-Za-z0-9_:@?.\\/]+\))"
        r"(?:[+-]0x[0-9a-f]+)?\),"
        r"bounded-stack-counter(?:[+-]0x[0-9a-f]+)?\*(?:1|2|4|8)"
        r"\)\)(?:[+-]0x[0-9a-f]+)?\)\)",
        abstract,
    )
    if indexed_field is not None and len(abstract) <= 512 and "iat:" not in abstract:
        return True
    direct = (
        re.fullmatch(
            r"(?:"
            r"load\(load\((?:stack|entry-stack)(?:[+-]0x[0-9a-f]+)?\)\)"
            r"|"
            r"load\(load\(load\((?:stack|entry-stack)(?:[+-]0x[0-9a-f]+)?\)\)\)"
            r")",
            abstract,
        )
        is not None
    )
    if direct:
        return True
    if abstract in {
        "bounded-paired-stack-vptr(+0x8,+0x4)",
        "bounded-stack-receiver-vptr",
    }:
        return True
    if (
        re.fullmatch(
            r"bounded-stack-vptr-sequence\("
            r"\+0x8(?:,\+0x4)+(?:,\+0xc)?\)",
            abstract,
        )
        is not None
    ):
        return True
    legacy_affine = (
        re.fullmatch(
            r"load\(load\(affine\("
            r"(?:this|entry-register\((?:ecx|edx)\))"
            r"(?:[+-]0x[0-9a-f]+)?,"
            r"(?:"
            r"(?:load\((?:entry-stack|stack-local)(?:[+-]0x[0-9a-f]+)?\)"
            r"|bounded-counter\(e(?:ax|bx|cx|dx|si|di|bp)\)"
            r"|bounded-stack-counter(?:[+-]0x[0-9a-f]+)?)"
            r"|index-scale\((?:load\((?:entry-stack|stack-local|this)"
            r"(?:[+-]0x[0-9a-f]+)?\)|bounded-counter\(e(?:ax|bx|cx|dx|si|di|bp)\)"
            r"|bounded-stack-counter(?:[+-]0x[0-9a-f]+)?),3\)"
            r"|index-sum\("
            r"(?:load\((?:entry-stack|stack-local|this)(?:[+-]0x[0-9a-f]+)?\)"
            r"|bounded-counter\(e(?:ax|bx|cx|dx|si|di|bp)\)"
            r"|bounded-stack-counter(?:[+-]0x[0-9a-f]+)?),"
            r"(?:load\((?:entry-stack|stack-local|this)(?:[+-]0x[0-9a-f]+)?\)"
            r"|bounded-counter\(e(?:ax|bx|cx|dx|si|di|bp)\)"
            r"|bounded-stack-counter(?:[+-]0x[0-9a-f]+)?)"
            r"\)"
            r")\*4"
            r"(?:,[+-]0x[0-9a-f]+)?"
            r"\)\)\)",
            abstract,
        )
        is not None
    )
    if legacy_affine:
        return True
    prefix = "load(load(affine("
    if not abstract.startswith(prefix) or not abstract.endswith(")))"):
        return False
    arguments = _split_abstract_arguments(
        abstract[len(prefix) : -3]
    )
    if arguments is None or len(arguments) not in {2, 3}:
        return False
    if (
        re.fullmatch(
            r"(?:this|entry-register\((?:ecx|edx)\))"
            r"(?:[+-]0x[0-9a-f]+)?",
            arguments[0],
        )
        is None
        or (
            len(arguments) == 3
            and re.fullmatch(r"[+-]0x[0-9a-f]+", arguments[2])
            is None
        )
    ):
        return False
    index_term = re.fullmatch(
        r"(?P<index>.+)\*(?P<scale>1|2|4|8)",
        arguments[1],
    )
    return bool(
        index_term is not None
        and _cc_receiver_proofs._bounded_index_provenance(index_term.group("index"))
    )


def _exact_register_memory_load(
    instruction: Instruction,
) -> tuple[str, str, int] | None:
    """Decode one exact unprefixed ``MOV r32,[r32+disp]`` without a SIB."""

    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    if len(body) < 2 or body[0] != 0x8B:
        return None
    modrm = body[1]
    mode = modrm >> 6
    base_code = modrm & 7
    if mode == 3 or base_code in {4, 5} and mode == 0 or base_code == 4:
        return None
    if mode == 0 and len(body) == 2:
        displacement = 0
    elif mode == 1 and len(body) == 3:
        displacement = struct.unpack("<b", body[2:3])[0]
    elif mode == 2 and len(body) == 6:
        displacement = struct.unpack("<i", body[2:6])[0]
    else:
        return None
    registers = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    destination = registers[(modrm >> 3) & 7]
    base = registers[base_code]
    operands = _cc_cfg._instruction_operand(instruction).split(",", 1)
    if (
        _cc_cfg._instruction_mnemonic(instruction) != "mov"
        or len(operands) != 2
        or operands[0].strip().lower() != destination
    ):
        return None
    expression, rendered_displacement = _cc_targets._memory_slot(operands[1])
    if (
        expression
        not in {
            base,
            f"{base}+{displacement}",
            f"{base}+0x{displacement:x}",
        }
        or rendered_displacement not in {0, displacement}
    ):
        return None
    return destination, base, displacement


def _exact_zero_push(instruction: Instruction) -> bool:
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return False
    operand = _cc_cfg._instruction_operand(instruction).strip().lower()
    return bool(
        _cc_cfg._instruction_mnemonic(instruction) == "push"
        and operand in {"0", "0x0"}
        and body in {b"\x6a\x00", b"\x68\x00\x00\x00\x00"}
    )


def _exact_stack_address_lea(
    instruction: Instruction,
) -> tuple[str, int] | None:
    """Decode exact ``LEA r32,[ESP+disp]`` used as an interface receiver."""

    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    if len(body) not in {4, 7} or body[0] != 0x8D or body[1] & 7 != 4:
        return None
    mode = body[1] >> 6
    if body[2] != 0x24 or mode not in {1, 2}:
        return None
    displacement = (
        struct.unpack("<b", body[3:4])[0]
        if mode == 1
        else struct.unpack("<i", body[3:7])[0]
    )
    registers = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    destination = registers[(body[1] >> 3) & 7]
    operands = _cc_cfg._instruction_operand(instruction).split(",", 1)
    expression, rendered = (
        _cc_targets._memory_slot(operands[1]) if len(operands) == 2 else ("", None)
    )
    if (
        _cc_cfg._instruction_mnemonic(instruction) != "lea"
        or len(operands) != 2
        or operands[0].strip().lower() != destination
        or expression not in {
            f"esp+{displacement}", f"esp+0x{displacement:x}",
        }
        or rendered != displacement
    ):
        return None
    return destination, displacement


def _exact_retail_fresh_register_provenance(
    instructions: Sequence[Instruction],
    *,
    before_index: int,
    register: str,
    addresses: Sequence[int | None],
    indexes: IdentityIndexes,
    depth: int = 0,
    visited: frozenset[tuple[int, str]] = frozenset(),
) -> str:
    """Recover one straight-line fresh retail register definition.

    This is a bounded backward slice for targetless receiver/vptr calls whose
    forward state was conservatively discarded at an outer CFG join.  Every
    register and memory edge must have exact agreeing bytes/rendering.  A
    control-flow edge, volatile-register call crossing, competing definition,
    unknown absolute cell, or recursive cycle publishes no proof.
    """

    key = (before_index, register)
    if (
        depth > 8
        or before_index < 0
        or register not in {
            "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"
        }
        or key in visited
    ):
        return ""
    next_visited = visited | {key}
    for index in range(before_index - 1, -1, -1):
        instruction = instructions[index]
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        if mnemonic.startswith("j") or mnemonic.startswith("loop"):
            return ""
        if mnemonic in {"call", "jmp"}:
            if mnemonic == "jmp":
                return ""
            if register == "eax":
                try:
                    body = bytes(int(item, 16) for item in instruction.bytes)
                except (TypeError, ValueError):
                    return ""
                address = addresses[index] if index < len(addresses) else None
                if len(body) != 5 or body[0] != 0xE8 or address is None:
                    return ""
                target = normalize_address(
                    address + 5 + struct.unpack_from("<i", body, 1)[0]
                )
                identity = indexes.by_address.get(target, "")
                return f"call-result({identity})" if identity else ""
            if register in {"ecx", "edx"}:
                return ""
            continue
        if not _cc_cfg._instruction_may_clobber_register(instruction, register):
            continue
        move = _cc_receiver_instructions._exact_register_move(instruction)
        if move is not None and move[0] == register:
            return _exact_retail_fresh_register_provenance(
                instructions,
                before_index=index,
                register=move[1],
                addresses=addresses,
                indexes=indexes,
                depth=depth + 1,
                visited=next_visited,
            )
        stack_load = _cc_receiver_instructions._exact_stack_slot_load(instruction)
        if stack_load is not None and stack_load[0] == register:
            return f"load({_cc_receiver_instructions._abstract_with_displacement('entry-stack', stack_load[1])})"
        memory_load = _exact_register_memory_load(instruction)
        if memory_load is not None and memory_load[0] == register:
            base = _exact_retail_fresh_register_provenance(
                instructions,
                before_index=index,
                register=memory_load[1],
                addresses=addresses,
                indexes=indexes,
                depth=depth + 1,
                visited=next_visited,
            )
            return (
                f"load({_cc_receiver_instructions._abstract_with_displacement(base, memory_load[2])})"
                if base
                else ""
            )
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            return ""
        registers = (
            "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
        )
        if (
            len(body) == 6
            and body[0] == 0x8B
            and body[1] >> 6 == 0
            and body[1] & 7 == 5
            and registers[(body[1] >> 3) & 7] == register
        ):
            absolute = normalize_address(struct.unpack_from("<I", body, 2)[0])
            identity = indexes.storage_by_address.get(absolute, "")
            if not identity or identity.startswith("iat:"):
                return ""
            operands = _cc_cfg._instruction_operand(instruction).split(",", 1)
            expression = (
                _cc_targets._exact_memory_expression(operands[1])
                if len(operands) == 2
                else ""
            )
            if (
                len(operands) != 2
                or operands[0].strip().lower() != register
                or (
                    expression != absolute
                    and indexes.storage_by_name.get(expression) != identity
                )
            ):
                return ""
            return identity
        return ""
    if register == "ecx":
        return "this"
    if register == "edx":
        return "entry-register(edx)"
    return ""


def _eligible_retail_fresh_targetless_vptr(abstract: str) -> bool:
    """Restrict fresh-slice fallback to non-stack, non-entry deep roots."""

    # The object still comes from this; only its array index is an exact
    # incoming scalar argument. Do not admit a stack-rooted object or local.
    argument_array = re.fullmatch(
        r"load\(load\(affine\(this,(load\(entry-stack\+0x[0-9a-f]+\))"
        r"\*4,\+0x([0-9a-f]+)\)\)\)", abstract,
    )
    if argument_array is not None:
        return (_exact_cfg_index_source(argument_array.group(1))
                and 0 < int(argument_array.group(2), 16) <= 0x7FFFFFFF)

    runtime_root = (
        r"(?:this|call-result\([^()]+\)|storage:[A-Za-z0-9_:@?.\\/]+)"
        r"(?:[+-]0x[0-9a-f]+)*"
    )
    exact_runtime_join_vptr = bool(
        re.fullmatch(
            r"load\(load\(runtime-object-join\("
            + runtime_root
            + r"(?:,"
            + runtime_root
            + r"){1,3}\)(?:[+-]0x[0-9a-f]+)*\)\)",
            abstract,
        )
    )
    return bool(
        "stack" not in abstract
        and "entry-register(" not in abstract
        and (
            "call-result(" in abstract
            or abstract.count("load(") >= 2
        )
        and (
            _cc_receiver_storage._is_bounded_dynamic_load(abstract)
            or exact_runtime_join_vptr
        )
    )


def _exact_entry_stack_vptr(abstract: str) -> bool:
    """Retain the exact aligned argument slot of a whole-CFG receiver proof."""
    match = re.fullmatch(r"load\(load\(entry-stack\+0x([0-9a-f]+)\)\)", abstract)
    if match is None:
        return False
    slot = int(match.group(1), 16)
    return 4 <= slot <= 0x7FFFFFFF and slot % 4 == 0


def _eligible_retail_cfg_targetless_vptr(abstract: str) -> bool:
    """Also retain an exact EDX argument's member vptr after CFG convergence.

    COM methods pass their object on the stack, so ECX need not hold that
    object at invocation. The whole-CFG proof still establishes the incoming
    argument, member offset and both loads without guessing a virtual target.
    This extension is deliberately unavailable to the linear fallback.
    """
    if not abstract or len(abstract) > 512:
        return False
    if _exact_allocation_vptr_storage(abstract):
        return True
    if abstract.startswith("load(cursor(") and abstract.endswith("))"):
        return _bounded_cfg_cursor(abstract[5:-1]) == "pointer"
    if _exact_entry_stack_vptr(abstract):
        return True
    member = re.fullmatch(r"load\(load\(entry-register\(edx\)\+0x([0-9a-f]+)\)\)", abstract)
    if abstract.startswith("load(affine(") and abstract.endswith("))"):
        arguments = _split_abstract_arguments(abstract[len("load(affine("):-2])
        if arguments is not None and len(arguments) in {2, 3}:
            index = re.fullmatch(r"(.+)\*(1|2|4|8)", arguments[1])
            if (_bounded_targetless_pointer_base(arguments[0]) and index
                    and _cc_receiver_proofs._bounded_index_provenance(index.group(1))
                    and (len(arguments) == 2 or re.fullmatch(r"[+-]0x[0-9a-f]+", arguments[2]))):
                return True
    return _eligible_retail_fresh_targetless_vptr(abstract) or bool(
        member and 0 < int(member.group(1), 16) <= 0x7FFFFFFF
        and _cc_receiver_storage._is_bounded_dynamic_load(abstract, allow_entry_register_root=True)
    )


def _exact_allocation_vptr_storage(abstract: str) -> bool:
    """A load from one exact allocator occurrence, retaining any null arm."""
    root = r"allocation-result\(provider:recoil:function:0x4c5b76,[0-9]+\)"
    return re.fullmatch(r"load\((?:" + root + r"|nullable\(" + root + r"\))\)", abstract) is not None


def _candidate_listing_matches_coff(
    instructions: Sequence[Instruction],
    *,
    addresses: Sequence[int | None],
    caller_start: int,
    definition: CandidateCallerDefinition,
) -> bool:
    """Require an exact contiguous current-COFF prefix for CFG input."""
    if not instructions or len(instructions) != len(addresses):
        return False
    cursor = 0
    for instruction, address in zip(instructions, addresses):
        encoded_parts = instruction.bytes
        if re.fullmatch(
            r"\s*[0-9a-f]{5,8}\s+(?:[0-9a-f]{2}\s+){4}DD\s+[^;\r\n]+(?:;.*)?",
            instruction.source_line, re.IGNORECASE,
        ):
            # The listing parser retains DD as a fifth hex-looking token.
            # It is a four-byte data directive, not opcode 0xdd. Its bytes
            # still have to agree with COFF; switch target proofs own the
            # table relocation semantics and its non-executable role.
            if len(encoded_parts) != 5 or encoded_parts[-1].lower() != "dd":
                return False
            encoded_parts = encoded_parts[:4]
        try:
            encoded = bytes(int(value, 16) for value in encoded_parts)
        except (TypeError, ValueError):
            return False
        if (not encoded or address != caller_start + cursor
                or definition.data[cursor:cursor + len(encoded)] != encoded):
            return False
        cursor += len(encoded)
    # A COMDAT can end in compiler switch data after the instruction prefix.
    # This helper proves call operands, not whole-contribution byte coverage.
    # Every supplied instruction still matches its own current COFF bytes;
    # the ordinary census and local-switch proofs own noninstruction ranges.
    return True


def _candidate_coff_direct_call_identities(
    instructions: Sequence[Instruction],
    *,
    addresses: Sequence[int | None],
    caller_start: int,
    definition: CandidateCallerDefinition,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, object] | None = None,
    compiler_generated_bridges: Mapping[str, str] | None = None,
) -> dict[int, str]:
    """Acquire registered direct-call identities from exact listing/COFF pairs.

    This supplies candidate CFG call-result lineage, never retail expectations.
    Unknown COFF symbols never become a zero-displacement fallthrough target.
    """
    if not _candidate_listing_matches_coff(instructions, addresses=addresses,
            caller_start=caller_start, definition=definition):
        return {}
    result: dict[int, str] = {}
    for index, (instruction, address) in enumerate(zip(instructions, addresses)):
        if _cc_cfg._instruction_mnemonic(instruction) != "call" or address is None:
            continue
        name = _cc_cfg._instruction_operand(instruction).strip()
        identity = indexes.by_candidate_name.get(name)
        if name in (compiler_generated_bridges or {}) or (not identity and name in (bridge_names or {})):
            try:
                kind, resolved = _cc_targets._canonical_direct_identity(name, source="cod",
                    caller_identity=indexes.by_address.get(hex(caller_start), ""),
                    caller_start=caller_start, caller_end=caller_start + len(definition.data),
                    indexes=indexes, bridge_names=bridge_names or {},
                    compiler_generated_bridges=compiler_generated_bridges or {},
                    call_site_address=address, call_site_unique=True, cod_source_line=instruction.source_line)
                if kind in {"direct", "provider"}:
                    identity = resolved
                else:
                    identity = ""
            except ValueError:
                identity = ""
        if not identity:
            continue
        offset = address - caller_start
        references = [row for row in definition.relocations if row.offset == offset + 1]
        if (
            offset < 0 or offset + 5 > len(definition.data)
            or tuple(instruction.bytes) != ("e8", "00", "00", "00", "00")
            or definition.data[offset:offset + 5] != b"\xe8\0\0\0\0"
            or len(references) != 1
            or references[0].type != IMAGE_REL_I386_REL32
            or references[0].symbol_name != name
            or len(definition.relocation_mask) != len(definition.data)
            or not all(definition.relocation_mask[offset + 1:offset + 5])
        ):
            continue
        symbols = [row for row in definition.coff_symbols
                   if row.index == references[0].symbol_index]
        if len(symbols) != 1 or symbols[0].name != name:
            continue
        result[index] = identity
    return result


def _candidate_exact_static_callback_register(
    instructions: Sequence[Instruction], *, transfer_index: int, register: str,
    definition: CandidateCallerDefinition, indexes: IdentityIndexes,
    addresses: Sequence[int | None], caller_start: int, caller_end: int,
    local_control_flow_indices: frozenset[int],
    local_control_flow_targets: Mapping[int, tuple[int, ...]],
) -> str:
    """Prove a global callback load reaches every visit to a register CALL.

    Only unprefixed A1/8B absolute loads with exact zero-addend DIR32 symbols
    qualify. The complete CFG must retain the same independently indexed
    storage on all paths; calls, partial writes and unknown edges fail closed.
    """
    if register not in {"eax", "ecx", "edx", "ebx", "esi", "edi", "ebp"}:
        return ""
    if (len(definition.relocation_mask) != len(definition.data)
            or len(addresses) != len(instructions)):
        return ""
    counts = Counter(addresses)
    by_address = {address: index for index, address in enumerate(addresses)
                  if address is not None and counts[address] == 1}
    grouped: dict[str, set[int]] = {}
    names = ("eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi")
    for index, instruction in enumerate(instructions):
        address = addresses[index]
        if address is None:
            continue
        body = bytes(int(byte, 16) for byte in instruction.bytes)
        if len(body) == 5 and body[0] == 0xA1 and register == "eax":
            operand_offset = 1
        elif (len(body) == 6 and body[0] == 0x8B and body[1] & 0xC7 == 5
              and names[(body[1] >> 3) & 7] == register):
            operand_offset = 2
        else:
            continue
        offset = address - caller_start
        if offset < 0 or definition.data[offset:offset + len(body)] != body:
            continue
        rows = [row for row in definition.relocations
                if offset <= row.offset < offset + len(body)]
        if (len(rows) != 1 or rows[0].offset != offset + operand_offset
                or rows[0].type != IMAGE_REL_I386_DIR32
                or body[operand_offset:] != b"\0" * 4
                or any(definition.relocation_mask[offset:offset + operand_offset])
                or not all(definition.relocation_mask[offset + operand_offset:offset + len(body)])):
            continue
        relocation = rows[0]
        symbols = [symbol for symbol in definition.coff_symbols
                   if symbol.index == relocation.symbol_index
                   and symbol.name == relocation.symbol_name]
        if len(symbols) != 1:
            continue
        if _cc_cfg._instruction_operand(instruction).strip() != f"{register}, dword {relocation.symbol_name}":
            continue
        identity = _cc_receiver_storage._candidate_c_external_storage_identity(relocation.symbol_name, indexes=indexes)
        if identity.startswith("storage:"):
            grouped.setdefault(identity, set()).add(index)
    proven = [identity for identity, indices in grouped.items()
              if _cc_cfg._exact_register_definition_set_covers_transfer(
                  instructions, instruction_addresses=addresses,
                  instruction_index_by_address=by_address,
                  definition_indices=frozenset(indices), transfer_index=transfer_index,
                  register=register, source="cod", caller_start=caller_start,
                  caller_end=caller_end, local_control_flow_indices=local_control_flow_indices,
                  local_control_flow_targets=local_control_flow_targets)]
    return proven[0] if len(proven) == 1 else ""


def _candidate_aggregate_field_vptr_calls(
    instructions: Sequence[Instruction], *, addresses: Sequence[int | None],
    caller_start: int, definition: CandidateCallerDefinition,
    aggregate_symbol: str, displacement: int, slot_displacement: int,
) -> tuple[tuple[int, int, str], ...]:
    """Prove ECX field loads and member-vptr calls through the complete CFG.

    Every reference to this aggregate field must have matching x86 bytes,
    symbolic operands, and one exact DIR32 relocation. Multiple loads are
    permitted; every arrival at the vptr load and call must retain the field
    receiver. This supplies storage identity, not argument values or behavior.
    """
    if (not 0 <= displacement <= 0x7fffffff
            or not 0 <= slot_displacement <= 0x7f
            or len(definition.data) != len(definition.relocation_mask)
            or not _candidate_listing_matches_coff(instructions, addresses=addresses,
                caller_start=caller_start, definition=definition)):
        return ()
    counts = Counter(addresses)
    if any(address is None or counts[address] != 1 for address in addresses):
        return ()
    if (definition.undefined_external_data.count(aggregate_symbol) != 1
            or aggregate_symbol in definition.defined_external_data):
        return ()
    registers = ('eax', 'ecx', 'edx', 'ebx', 'esp', 'ebp', 'esi', 'edi')
    expression = f'{aggregate_symbol}+{displacement}'
    mention = re.compile(re.escape(expression) + r'(?![0-9a-z_])', re.IGNORECASE)
    field_bytes = struct.pack('<I', displacement)
    receiver_loads = set()
    plain = set()
    vptr_loads = []
    calls = []
    for index, (instruction, address) in enumerate(zip(instructions, addresses)):
        offset = address - caller_start
        body = bytes(int(part, 16) for part in instruction.bytes)
        references = [r for r in definition.relocations if offset <= r.offset < offset + len(body)]
        names_field = bool(mention.search(instruction.raw_text))
        relocates_field = any(r.symbol_name == aggregate_symbol
            and definition.data[r.offset:r.offset + 4] == field_bytes for r in references)
        if names_field or relocates_field:
            expected_text, operand_offset, receiver_load = '', 0, False
            if len(body) == 5 and body[0] in {0xa1, 0xa3}:
                operand_offset = 1
                expected_text = (f'mov eax, dword {expression}' if body[0] == 0xa1
                    else f'mov dword {expression}, eax')
            elif (len(body) == 6 and body[0] in {0x8b, 0x89}
                    and body[1] & 0xc7 == 5):
                operand_offset = 2
                register = registers[(body[1] >> 3) & 7]
                expected_text = (f'mov {register}, dword {expression}' if body[0] == 0x8b
                    else f'mov dword {expression}, {register}')
                receiver_load = body[0] == 0x8b and register == 'ecx'
            elif len(body) == 6 and body[:2] == b'\xff\x35':
                operand_offset = 2
                expected_text = f'push dword {expression}'
            normalized = re.sub(r'\s+', ' ', instruction.raw_text.strip().lower()).replace('dword ptr ', 'dword ')
            if (not expected_text or normalized != expected_text.lower()
                    or len(references) != 1 or references[0].offset != offset + operand_offset
                    or references[0].type != IMAGE_REL_I386_DIR32
                    or references[0].symbol_name != aggregate_symbol
                    or body[operand_offset:] != field_bytes
                    or any(definition.relocation_mask[offset:offset + operand_offset])
                    or not all(definition.relocation_mask[offset + operand_offset:offset + len(body)])):
                return ()
            symbols = [row for row in definition.coff_symbols if row.index == references[0].symbol_index]
            if len(symbols) != 1 or symbols[0].name != aggregate_symbol:
                return ()
            if receiver_load:
                receiver_loads.add(index)
        elif not references and not any(definition.relocation_mask[offset:offset + len(body)]):
            plain.add(index)
            if len(body) == 2 and body[0] == 0x8b and body[1] & 0xc7 == 1:
                register = registers[(body[1] >> 3) & 7]
                if (register not in {'ecx', 'esp'}
                        and _cc_cfg._instruction_mnemonic(instruction) == 'mov'
                        and _cc_cfg._instruction_operand(instruction) == f'{register}, dword [ecx]'):
                    vptr_loads.append((index, register))
            slot = _cc_targets._exact_indirect_register_call_slot(instruction, allow_zero=True)
            if slot is not None and slot[1] == slot_displacement:
                calls.append((index, slot[0]))
    if not receiver_loads:
        return ()
    by_address = {address: index for index, address in enumerate(addresses)}
    def covers(definitions, transfer, register):
        return _cc_cfg._exact_register_definition_set_covers_transfer(
            instructions, instruction_addresses=addresses, instruction_index_by_address=by_address,
            definition_indices=frozenset(definitions), transfer_index=transfer, register=register,
            source='cod', caller_start=caller_start, caller_end=caller_start + len(definition.data),
            local_control_flow_indices=frozenset(), local_control_flow_targets={})
    proven = []
    for call_index, call_register in calls:
        if (call_index not in plain or _cc_cfg._cleanup_after(instructions, call_index) is not None
                or not covers(receiver_loads, call_index, 'ecx')):
            continue
        reaching = [(index, register) for index, register in vptr_loads
            if register == call_register and covers(receiver_loads, index, 'ecx')
            and covers({index}, call_index, register)]
        if len(reaching) == 1:
            index, register = reaching[0]
            proven.append((index, call_index, register))
    return tuple(proven)


def _exact_candidate_cfg_vptr_proofs(
    instructions: Sequence[Instruction],
    *,
    addresses: Sequence[int | None],
    caller_start: int,
    indexes: IdentityIndexes,
    definition: CandidateCallerDefinition | None,
    local_control_flow_indices: frozenset[int],
    local_control_flow_targets: Mapping[int, tuple[int, ...]],
    call_cleanup_by_instruction_index: Mapping[int, int] | None = None,
    bridge_names: Mapping[str, object] | None = None,
) -> dict[int, str]:
    """Prove runtime receivers from current COFF operands and all CFG arrivals.

    Member and array storage retain their exact incoming root, offsets and
    loads after convergence, including loops and verified nonzero guards.
    Factory results additionally require matching ECX and a unique registered
    call identity. Relocated receiver arithmetic is excluded.
    This establishes runtime storage only, never a static virtual target.
    """
    if definition is None or not _candidate_listing_matches_coff(
            instructions, addresses=addresses, caller_start=caller_start,
            definition=definition):
        return {}
    for row, address in zip(instructions, addresses):
        if _cc_cfg._instruction_mnemonic(row) in {"cmp", "test"} and any(
                address - caller_start <= item.offset < address - caller_start + len(row.bytes)
                for item in definition.relocations):
            return {}
    identities = _candidate_coff_direct_call_identities(
        instructions, addresses=addresses, caller_start=caller_start,
        definition=definition, indexes=indexes, bridge_names=bridge_names,
    )
    counts = Counter(identities.values())
    known_results = {f"call-result({identity})" for identity, count in counts.items()
                     if count == 1}
    result = {}
    # The CFG's integer arithmetic is literal, not linked relocation algebra.
    # A relocation on a register-producing arithmetic/load instruction must
    # therefore block this route rather than be interpreted as its addend.
    relocated_register_writes = {}
    for instruction_index, (instruction, address) in enumerate(zip(instructions, addresses)):
        if _cc_cfg._instruction_mnemonic(instruction) not in {
            "mov", "lea", "add", "sub", "imul", "shl", "sal", "xor", "inc",
        }:
            continue
        offset = address - caller_start
        if any(offset <= row.offset < offset + len(instruction.bytes)
               for row in definition.relocations):
            relocated_register_writes[instruction_index] = frozenset(
                register for register in _cc_catalog.REGISTER_STATE_NAMES
                if _cc_cfg._instruction_may_clobber_register(instruction, register))
    for index, instruction in enumerate(instructions):
        slot = _cc_targets._exact_indirect_register_call_slot(instruction, allow_zero=True)
        if slot is None:
            continue
        arguments = dict(instructions=instructions, before_index=index,
            addresses=addresses, indexes=indexes, caller_start=caller_start,
            caller_end=caller_start + len(definition.data),
            local_control_flow_indices=local_control_flow_indices,
            local_control_flow_targets=local_control_flow_targets,
            source="cod", direct_call_identities=identities,
            call_cleanup_by_instruction_index=call_cleanup_by_instruction_index,
            allow_exact_caller_cleanup=True,
            allow_exact_affine_receiver_roots=True,
            allow_coff_symbolic_stack_operands=True,
            opaque_register_writes=relocated_register_writes)
        receiver = _cc_receiver_retail._exact_retail_cfg_register_provenance(register="ecx", **arguments)
        value = _cc_receiver_retail._exact_retail_cfg_register_provenance(register=slot[0], **arguments)
        exact_storage = ("call-result(" not in value
            and (value == "load(this)"
                 or _eligible_retail_cfg_targetless_vptr(value)
                 or _cc_receiver_equivalence._canonical_exact_this_member_vptr_storage(value)))
        cursor = (_split_abstract_arguments(receiver[7:-1])
                  if receiver.startswith("cursor(") and receiver.endswith(")") else None)
        result_cursor = (cursor is not None and len(cursor) == 2
                         and cursor[0] in known_results
                         and _bounded_cfg_cursor(receiver) == "pointer")
        if exact_storage or value == f"load({receiver})" and (receiver in known_results or result_cursor):
            result[index] = value
    return result


def _exact_cfg_index_source(value: str) -> bool:
    """Recognize a proved member scalar or aligned incoming scalar argument."""
    match = re.fullmatch(r"load\((this|entry-stack)\+0x([0-9a-f]+)\)", value)
    if match is None:
        return False
    offset = int(match.group(2), 16)
    return offset <= 0x7FFFFFFF and (
        match.group(1) == "this" or offset >= 4 and offset % 4 == 0
    )


def _exact_cfg_cursor_recurrences(
    instructions: Sequence[Instruction], *, before_index: int,
    successors: Mapping[int, tuple[int, ...]], unresolved: frozenset[int],
) -> dict[int, tuple[str, int, bool]]:
    """Find a dominating value and one exact stride on every loop return.

    The seed's value is computed by dataflow, never invented from its register.
    A seed outside the cycle overwrites every prior value. Each return to the
    query must cross the sole update, whose region has no other register write.
    """
    reaching = _cc_cfg._indices_reaching_cfg_target(successors, before_index)
    after = _cc_cfg._reachable_cfg_indices(successors, successors.get(before_index, ()))
    if before_index not in after:
        return {}
    result = {}
    for seed, instruction in enumerate(instructions):
        if seed in after or seed not in reaching:
            continue
        for register in ("ebx", "ebp", "esi", "edi"):
            if not _cc_cfg._instruction_may_clobber_register(instruction, register):
                continue
            if before_index in _cc_cfg._reachable_cfg_indices(successors, (0,), blocked=frozenset({seed})):
                continue
            relevant = _cc_cfg._reachable_cfg_indices(successors, (seed,)) & reaching
            if relevant & unresolved:
                continue
            writes = [index for index in relevant if index != seed
                      and _cc_cfg._instruction_may_clobber_register(instructions[index], register)]
            if len(writes) != 1:
                continue
            update = writes[0]
            add = _cc_receiver_instructions._exact_register_step_immediate(instructions[update])
            if add is None or add[0] != register or abs(add[1]) not in _cc_catalog.BOUNDED_TARGETLESS_VPTR_STRIDES:
                continue
            if update not in after or before_index in _cc_cfg._reachable_cfg_indices(successors,
                    successors.get(before_index, ()), blocked=frozenset({update})):
                continue
            for index, is_seed in ((seed, True), (update, False)):
                record = (register, add[1], is_seed)
                if index in result and result[index] != record:
                    return {}
                result[index] = record
    return result


def _exact_cfg_zero_stride_seeds(
    instructions: Sequence[Instruction], *, before_index: int,
    successors: Mapping[int, tuple[int, ...]], unresolved: frozenset[int],
) -> dict[int, tuple[str, str]]:
    """Prove constant-entry nonvolatile cursors with one exact update per cycle.

    The zero must dominate the query and remain outside its cycle. Every
    return to the query must cross the unique positive update, and no other
    reaching path may redefine the register. This proves a recurrence, not
    an array extent or a loop termination bound.
    """
    reaching = _cc_cfg._indices_reaching_cfg_target(successors, before_index)
    after_query = _cc_cfg._reachable_cfg_indices(successors, successors.get(before_index, ()))
    if before_index not in after_query:
        return {}
    from _recoil.call_contract.receiver_stack_loops import natural_zero_stride_seeds
    result = natural_zero_stride_seeds(instructions, before_index=before_index,
        successors=successors, unresolved=unresolved)
    for zero, instruction in enumerate(instructions):
        register = _cc_receiver_proofs._exact_zero_register(instruction)
        initial_value = 0
        if register is None:
            initial = _cc_receiver_instructions._exact_register_move_immediate(instruction)
            if initial is None or not 0 < initial[1] <= _cc_catalog._BOUNDED_TARGETLESS_ARITHMETIC_LIMIT:
                continue
            register, initial_value = initial
        if register not in {"ebx", "ebp", "esi", "edi"} or zero not in reaching:
            continue
        if zero in after_query or before_index in _cc_cfg._reachable_cfg_indices(
            successors, (0,), blocked=frozenset({zero}),
        ):
            continue
        relevant = _cc_cfg._reachable_cfg_indices(successors, (zero,)) & reaching
        if relevant & unresolved:
            continue
        writes = [index for index in relevant if index != zero
                  and _cc_cfg._instruction_may_clobber_register(instructions[index], register)]
        if len(writes) != 1:
            continue
        update = writes[0]
        add = _cc_receiver_instructions._exact_register_add_immediate(instructions[update])
        step = (add[1] if add is not None and add[0] == register else
                1 if _cc_receiver_proofs._exact_inc_register(instructions[update]) == register else 0)
        if not 0 < step <= _cc_catalog._BOUNDED_TARGETLESS_ARITHMETIC_LIMIT or update not in after_query:
            continue
        if before_index in _cc_cfg._reachable_cfg_indices(successors,
            successors.get(before_index, ()), blocked=frozenset({update})):
            continue
        marker = _bounded_stride_provenance(f"bounded-counter({register})", step)
        if initial_value:
            marker = f"index-offset({marker},0x{initial_value:x})"
        for index in (zero, update):
            if index in result and result[index] != (register, marker):
                return {}
            result[index] = (register, marker)
    return result
