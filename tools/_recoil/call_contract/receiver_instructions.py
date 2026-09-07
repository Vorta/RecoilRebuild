"""Recoil call-contract receiver instructions evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import instructions as _cc_instructions
from _recoil.call_contract import receiver_cursor as _cc_receiver_cursor
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import IdentityIndexes

import re
import struct
from typing import Mapping, Sequence

from _recoil.commands.asm_verify import Instruction
from _recoil.lib.progress import normalize_address


def _candidate_iat_import_name(object_symbol: str) -> str:
    """Derive only the import-directory name encoded by an MSVC IAT symbol."""

    if re.fullmatch(r"__imp_[A-Za-z0-9_@$?]+", object_symbol) is None:
        return ""
    decorated = object_symbol[len("__imp_") :]
    if decorated.startswith("?") or decorated.startswith("@"):
        return decorated
    if not decorated.startswith("_") or len(decorated) < 2:
        return ""
    import_name = decorated[1:]
    stdcall = re.fullmatch(r"(?P<name>[A-Za-z_][A-Za-z0-9_]*)@\d+", import_name)
    return stdcall.group("name") if stdcall is not None else import_name


def _exact_direct_iat_storage(
    operand: str,
    *,
    instruction: Instruction,
    source: str,
    indexes: IdentityIndexes,
) -> tuple[str, str, int | None, str] | None:
    """Resolve only exact candidate symbols or retail FF15 IAT operands."""
    call_mnemonic = _cc_cfg._instruction_mnemonic(instruction) == "call"
    if "__imp_" in operand:
        if source != "cod":
            raise ValueError(
                f"candidate IAT symbol is not valid retail truth in {operand!r}"
            )
        exact_name = _cc_targets._exact_memory_expression(operand)
        if not call_mnemonic:
            raise ValueError(
                f"candidate direct-IAT FF15 requires call mnemonic, got "
                f"{_cc_cfg._instruction_mnemonic(instruction)!r}"
            )
        if (
            _cc_targets._exact_ff15_absolute_address(instruction) is None
            or not exact_name.startswith("__imp_")
            or re.fullmatch(r"__imp_[A-Za-z0-9_@$?]+", exact_name)
            is None
            or re.fullmatch(
                rf"(?:dword\s+(?:ptr\s+)?)?{re.escape(exact_name)}",
                operand.strip(),
                flags=re.IGNORECASE,
            )
            is None
        ):
            raise ValueError(
                f"malformed or non-FF15 candidate IAT storage {operand!r}"
            )
        target = indexes.storage_by_name.get(exact_name, "")
        if not target:
            import_name = _candidate_iat_import_name(exact_name)
            target = indexes.storage_by_name.get(import_name, "")
        if not target or not target.startswith("iat:"):
            raise ValueError(
                f"unresolved exact tracker-backed candidate IAT storage "
                f"{exact_name!r}"
            )
        return "iat", target, None, target
    if source != "bn":
        return None

    decoded_address = _cc_targets._exact_ff15_absolute_address(instruction)
    if decoded_address is None:
        exact_expression = _cc_targets._exact_memory_expression(operand)
        possible_iat = indexes.storage_by_name.get(exact_expression, "")
        if _cc_catalog.ADDRESS_RE.fullmatch(exact_expression):
            possible_iat = indexes.storage_by_address.get(
                normalize_address(exact_expression),
                possible_iat,
            )
        if possible_iat.startswith("iat:"):
            raise ValueError(
                "retail direct IAT storage requires an exact unprefixed FF15 "
                f"CALL: {operand!r}"
            )
        return None
    address_identity = indexes.storage_by_address.get(decoded_address, "")
    if not address_identity.startswith("iat:"):
        return None
    if not call_mnemonic:
        raise ValueError(
            f"retail direct-IAT FF15 requires call mnemonic, got "
            f"{_cc_cfg._instruction_mnemonic(instruction)!r}"
        )
    match = re.fullmatch(
        r"dword\s+(?:ptr\s+)?"
        r"\[(?P<expression>[^\[\]\s]+)\]",
        operand.strip(),
        flags=re.IGNORECASE,
    )
    if match is None:
        raise ValueError(
            f"malformed exact retail direct-IAT operand {operand!r}"
        )
    exact_expression = _cc_targets._exact_memory_expression(operand)
    if exact_expression != match.group("expression"):
        raise ValueError(
            f"malformed exact retail direct-IAT expression {operand!r}"
        )
    if _cc_catalog.ADDRESS_RE.fullmatch(exact_expression):
        rendered_address = normalize_address(exact_expression)
        if rendered_address != decoded_address:
            raise ValueError(
                "retail direct-IAT text/address disagreement: "
                f"{rendered_address} != {decoded_address}"
            )
    else:
        named_identity = indexes.storage_by_name.get(exact_expression, "")
        if not named_identity or not named_identity.startswith("iat:"):
            raise ValueError(
                f"unresolved exact tracker-backed retail IAT name "
                f"{exact_expression!r}"
            )
        if named_identity != address_identity:
            raise ValueError(
                f"retail direct-IAT name/address identity disagreement for "
                f"{exact_expression!r} at {decoded_address}"
            )
    return "iat", address_identity, None, address_identity


def _abstract_with_displacement(base: str, displacement: int | None) -> str:
    if not displacement:
        return base
    operator = "+" if displacement > 0 else "-"
    return f"{base}{operator}{hex(abs(displacement))}"


def _encoded_stack_slot_load_displacement(
    instruction: Instruction,
    *,
    parsed_destination: str,
    parsed_source: str,
) -> int | None:
    """Decode an exact unprefixed ``MOV r32, [ESP(+disp)]`` instruction.

    VC5 COD symbolic stack names can retain a stale textual displacement after
    the compiler moves the current stack root.  Only the instruction encoding
    is authoritative for that slot: require opcode 8B, an exact ESP-base SIB
    with no index, one of the three memory ModRM modes, the encoded destination,
    and no prefix or trailing bytes.
    """
    if len(_cc_catalog.MEMORY_RE.findall(parsed_source)) != 1:
        return None
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    if len(body) < 3 or body[0] != 0x8B:
        return None
    modrm = body[1]
    mode = modrm >> 6
    if mode == 3 or (modrm & 0x07) != 0x04:
        return None
    destinations = (
        "eax",
        "ecx",
        "edx",
        "ebx",
        "esp",
        "ebp",
        "esi",
        "edi",
    )
    if parsed_destination.lower() != destinations[(modrm >> 3) & 0x07]:
        return None
    if body[2] != 0x24:
        return None
    if mode == 0 and len(body) == 3:
        return 0
    if mode == 1 and len(body) == 4:
        return struct.unpack("<b", body[3:4])[0]
    if mode == 2 and len(body) == 7:
        return struct.unpack("<i", body[3:7])[0]
    return None


def _encoded_stack_slot_immediate_store_displacement(
    instruction: Instruction,
) -> int | None:
    """Decode exact ``MOV dword [ESP(+disp)], imm32`` local initialization."""

    if _cc_cfg._instruction_mnemonic(instruction) != "mov":
        return None
    operands = _cc_cfg._instruction_operand(instruction).split(",", 1)
    if len(operands) != 2:
        return None
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    if len(body) < 7 or body[0] != 0xC7:
        return None
    modrm = body[1]
    mode = modrm >> 6
    if mode == 3 or ((modrm >> 3) & 7) != 0 or (modrm & 7) != 4:
        return None
    if body[2] != 0x24:
        return None
    if mode == 0 and len(body) == 7:
        displacement = 0
    elif mode == 1 and len(body) == 8:
        displacement = struct.unpack("<b", body[3:4])[0]
    elif mode == 2 and len(body) == 11:
        displacement = struct.unpack("<i", body[3:7])[0]
    else:
        return None
    rendered = _exact_stack_operand_displacement(operands[0])
    return displacement if rendered == displacement else None


def _bounded_initialized_stack_scalar_loads(
    instructions: Sequence[Instruction],
    *,
    source: str,
    caller_start: int,
    caller_end: int,
) -> Mapping[int, str]:
    """Prove exact, dominating, non-escaped initialized stack-local loads."""

    addresses = _cc_cfg._instruction_runtime_addresses(
        instructions,
        source=source,
        caller_start=caller_start,
    )
    counts: dict[int, int] = {}
    for address in addresses:
        if address is not None and caller_start <= address < caller_end:
            counts[address] = counts.get(address, 0) + 1
    by_address = {
        address: index
        for index, address in enumerate(addresses)
        if address is not None and counts.get(address) == 1
    }
    initializers: dict[int, list[int]] = {}
    for index, instruction in enumerate(instructions):
        displacement = _encoded_stack_slot_immediate_store_displacement(
            instruction
        )
        if displacement is not None and displacement >= 0:
            initializers.setdefault(displacement, []).append(index)

    result: dict[int, str] = {}
    for load_index, instruction in enumerate(instructions):
        operands = _cc_cfg._instruction_operand(instruction).split(",", 1)
        if len(operands) != 2:
            continue
        displacement = _encoded_stack_slot_load_displacement(
            instruction,
            parsed_destination=operands[0].strip().lower(),
            parsed_source=operands[1],
        )
        if displacement is None or displacement < 0:
            continue
        prior = [
            index
            for index in initializers.get(displacement, ())
            if index < load_index
        ]
        if not prior:
            continue
        initializer = prior[-1]
        # No entry-side branch may skip the selected initializer and land in
        # its proof interval.  Backedges wholly below the initializer remain
        # valid loop reuse of the same local scalar slot.
        skips_initializer = False
        for branch_index in range(initializer):
            branch = _cc_cfg._exact_local_direct_branch(
                instructions[branch_index],
                instruction_index=branch_index,
                instruction_addresses=addresses,
                instruction_index_by_address=by_address,
                source=source,
                caller_start=caller_start,
                caller_end=caller_end,
            )
            if (
                branch is not None
                and initializer < branch[1] <= load_index
            ):
                skips_initializer = True
                break
        if skips_initializer:
            continue
        interval = instructions[initializer + 1 : load_index]
        if any(
            _cc_cfg._instruction_mnemonic(row) == "lea"
            and re.search(r"\besp\b", _cc_cfg._instruction_operand(row), re.IGNORECASE)
            for row in interval
        ):
            continue
        result[load_index] = (
            f"load(stack-local{_abstract_with_displacement('', displacement)})"
        )
    return result


def _first_entry_stack_affine_index_load(
    instructions: Sequence[Instruction],
) -> bool:
    """Prove that instruction zero's ESP load feeds one exact scale-4 SIB index."""

    if not instructions:
        return False
    operands = _cc_cfg._instruction_operand(instructions[0]).split(",", 1)
    if len(operands) != 2:
        return False
    destination = operands[0].strip().lower()
    displacement = _encoded_stack_slot_load_displacement(
        instructions[0],
        parsed_destination=destination,
        parsed_source=operands[1],
    )
    if displacement is None or displacement < 0:
        return False
    consumers = 0
    for instruction in instructions[1:]:
        indexed = _cc_targets._exact_indexed_affine_load(instruction)
        if indexed is not None and indexed[2] == destination and indexed[3] == 4:
            consumers += 1
            continue
        if _cc_cfg._instruction_may_clobber_register(instruction, destination):
            break
    return consumers == 1


def _exact_stack_expression_displacement(expression: str) -> int | None:
    match = re.fullmatch(
        r"esp(?:(?P<sign>[+-])(?P<value>0x[0-9a-f]+|\d+))?",
        expression.strip().lower(),
    )
    if match is None:
        return None
    if match.group("value") is None:
        return 0
    value = _cc_cfg._parse_unsigned_assembly_integer(match.group("value"))
    return -value if match.group("sign") == "-" else value


def _exact_stack_operand_displacement(operand: str) -> int | None:
    """Resolve one explicit constant-affine ``N+[esp+M]`` COD local."""
    value = re.sub(
        r"^(?:byte|word|dword)\s+(?:ptr\s+)?",
        "",
        operand.strip(),
        flags=re.IGNORECASE,
    )
    match = re.fullmatch(
        r"(?:(?P<prefix>[+-]?(?:0x[0-9a-f]+|\d+))\+)?"
        r"\[(?P<expression>[^\]]+)\]",
        value,
        flags=re.IGNORECASE,
    )
    if match is None:
        return None
    displacement = _exact_stack_expression_displacement(
        match.group("expression")
    )
    if displacement is None:
        return None
    prefix = match.group("prefix")
    if prefix is None:
        return displacement
    sign = -1 if prefix.startswith("-") else 1
    raw_prefix = prefix[1:] if prefix[:1] in {"+", "-"} else prefix
    return displacement + sign * _cc_cfg._parse_unsigned_assembly_integer(raw_prefix)


def _exact_stack_slot_load(instruction: Instruction) -> tuple[str, int] | None:
    match = re.fullmatch(
        r"mov\s+(?P<destination>e(?:ax|bx|cx|dx|si|di|sp|bp))\s*,\s*"
        r"(?P<source>(?:dword\s+(?:ptr\s+)?)?"
        r"(?:(?:[+-]?(?:0x[0-9a-f]+|\d+))\+)?\[[^\]]+\])",
        instruction.raw_text.strip(),
        flags=re.IGNORECASE,
    )
    if match is None:
        return None
    destination = match.group("destination").lower()
    source = match.group("source")
    encoded = _encoded_stack_slot_load_displacement(
        instruction,
        parsed_destination=destination,
        parsed_source=source,
    )
    rendered = _exact_stack_operand_displacement(source)
    if encoded is None or rendered != encoded:
        return None
    return destination, encoded


def _encoded_stack_slot_store(
    instruction: Instruction,
    *,
    parsed_source: str,
) -> tuple[int, str] | None:
    """Decode exact unprefixed ``MOV [ESP(+disp)], r32`` store bytes."""
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    if len(body) < 3 or body[0] != 0x89:
        return None
    modrm = body[1]
    mode = modrm >> 6
    if mode == 3 or (modrm & 0x07) != 0x04 or body[2] != 0x24:
        return None
    registers = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    source = registers[(modrm >> 3) & 0x07]
    if source != parsed_source.lower():
        return None
    if mode == 0 and len(body) == 3:
        displacement = 0
    elif mode == 1 and len(body) == 4:
        displacement = struct.unpack("<b", body[3:4])[0]
    elif mode == 2 and len(body) == 7:
        displacement = struct.unpack("<i", body[3:7])[0]
    else:
        return None
    return displacement, source


def _exact_stack_slot_store(instruction: Instruction) -> tuple[int, str] | None:
    """Decode an exact unprefixed ``MOV [ESP(+disp)], r32`` spill."""
    match = re.fullmatch(
        r"mov\s+(?P<destination>(?:dword\s+(?:ptr\s+)?)?"
        r"(?:(?:[+-]?(?:0x[0-9a-f]+|\d+))\+)?\[[^\]]+\])"
        r"\s*,\s*(?P<source>e(?:ax|bx|cx|dx|si|di|sp|bp))",
        instruction.raw_text.strip(),
        flags=re.IGNORECASE,
    )
    if match is None:
        return None
    encoded = _encoded_stack_slot_store(
        instruction,
        parsed_source=match.group("source"),
    )
    if encoded is None:
        return None
    displacement, source = encoded
    rendered = _exact_stack_operand_displacement(match.group("destination"))
    return (displacement, source) if rendered == displacement else None


def _vc5_symbolic_stack_local(
    operand: str,
    *,
    index_register: str | None,
) -> tuple[str, int] | None:
    """Parse one exact VC5 COD ``local$N[esp(+index*4)+disp]`` operand."""
    value = re.sub(
        r"^(?:byte|word|dword)\s+(?:ptr\s+)?",
        "",
        operand.strip(),
        flags=re.IGNORECASE,
    )
    match = re.fullmatch(
        r"(?P<symbol>[A-Za-z_?$][A-Za-z0-9_@$?]*)"
        r"\[(?P<expression>[^\]]+)\]",
        value,
    )
    if match is None:
        return None
    expression = re.sub(r"\s+", "", match.group("expression").lower())
    if index_register is None:
        displacement = _exact_stack_expression_displacement(expression)
    else:
        indexed = re.fullmatch(
            rf"esp\+{re.escape(index_register.lower())}\*4"
            r"(?:(?P<sign>[+-])(?P<value>0x[0-9a-f]+|\d+))?",
            expression,
        )
        if indexed is None:
            return None
        displacement = 0
        if indexed.group("value") is not None:
            displacement = _cc_cfg._parse_unsigned_assembly_integer(
                indexed.group("value")
            )
            if indexed.group("sign") == "-":
                displacement = -displacement
    if displacement is None:
        return None
    return match.group("symbol"), displacement


def _exact_vc5_symbolic_stack_slot_store(
    instruction: Instruction,
) -> tuple[int, str, str, int] | None:
    """Decode one exact VC5 COD symbolic stack-local pointer store."""
    match = re.fullmatch(
        r"mov\s+(?P<destination>(?:dword\s+(?:ptr\s+)?)?"
        r"[A-Za-z_?$][A-Za-z0-9_@$?]*\[[^\]]+\])"
        r"\s*,\s*(?P<source>e(?:ax|bx|cx|dx|si|di|sp|bp))",
        instruction.raw_text.strip(),
        flags=re.IGNORECASE,
    )
    if match is None:
        return None
    symbolic = _vc5_symbolic_stack_local(
        match.group("destination"),
        index_register=None,
    )
    encoded = _encoded_stack_slot_store(
        instruction,
        parsed_source=match.group("source"),
    )
    if symbolic is None or encoded is None:
        return None
    symbol, rendered_displacement = symbolic
    encoded_displacement, source = encoded
    return encoded_displacement, source, symbol, rendered_displacement


def _exact_vc5_symbolic_stack_slot_load(
    instruction: Instruction,
) -> tuple[int, str, str, int] | None:
    """Decode one exact VC5 COD symbolic scalar stack-local load."""

    match = re.fullmatch(
        r"mov\s+(?P<destination>e(?:ax|bx|cx|dx|si|di|sp|bp))\s*,\s*"
        r"(?P<source>(?:dword\s+(?:ptr\s+)?)?"
        r"[A-Za-z_?$][A-Za-z0-9_@$?]*\[[^\]]+\])",
        instruction.raw_text.strip(),
        flags=re.IGNORECASE,
    )
    if match is None:
        return None
    symbolic = _vc5_symbolic_stack_local(
        match.group("source"),
        index_register=None,
    )
    encoded = _encoded_stack_slot_load_displacement(
        instruction,
        parsed_destination=match.group("destination"),
        parsed_source=match.group("source"),
    )
    if symbolic is None or encoded is None:
        return None
    symbol, rendered_displacement = symbolic
    return encoded, match.group("destination").lower(), symbol, rendered_displacement


def _exact_vc5_symbolic_stack_address_lea(
    instruction: Instruction,
) -> tuple[str, int, str, int] | None:
    """Decode one exact VC5 COD symbolic ``LEA r32, local$[ESP+n]``."""

    match = re.fullmatch(
        r"lea\s+(?P<destination>e(?:ax|bx|cx|dx|si|di|sp|bp))\s*,\s*"
        r"(?P<source>(?:dword\s+(?:ptr\s+)?)?"
        r"[A-Za-z_?$][A-Za-z0-9_@$?]*\[[^\]]+\])",
        instruction.raw_text.strip(),
        flags=re.IGNORECASE,
    )
    if match is None:
        return None
    symbolic = _vc5_symbolic_stack_local(
        match.group("source"),
        index_register=None,
    )
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    if (
        symbolic is None
        or len(body) not in {4, 7}
        or body[0] != 0x8D
        or body[1] & 7 != 4
        or body[2] != 0x24
        or body[1] >> 6 not in {1, 2}
    ):
        return None
    registers = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    destination = registers[(body[1] >> 3) & 7]
    if match.group("destination").lower() != destination:
        return None
    encoded_displacement = (
        struct.unpack("<b", body[3:4])[0]
        if len(body) == 4 and body[1] >> 6 == 1
        else struct.unpack("<i", body[3:7])[0]
        if len(body) == 7 and body[1] >> 6 == 2
        else None
    )
    if encoded_displacement is None:
        return None
    symbol, rendered_displacement = symbolic
    return (
        destination,
        encoded_displacement,
        symbol,
        rendered_displacement,
    )


def _exact_indexed_stack_array_load(
    instruction: Instruction,
) -> tuple[str, str, int] | None:
    """Decode exact ``MOV r32, [ESP + index*4 + disp]`` candidate bytes."""
    match = re.fullmatch(
        r"mov\s+(?P<destination>e(?:ax|bx|cx|dx|si|di|sp|bp))\s*,\s*"
        r"(?:dword\s+(?:ptr\s+)?)?"
        r"(?:(?P<symbol>[A-Za-z_?$][A-Za-z0-9_@$?]*)|"
        r"(?:(?P<prefix>[+-]?(?:0x[0-9a-f]+|\d+))\+))?"
        r"\[(?P<expression>[^\]]+)\]",
        instruction.raw_text.strip(),
        flags=re.IGNORECASE,
    )
    if match is None:
        return None
    expression = re.sub(r"\s+", "", match.group("expression").lower())
    indexed = re.fullmatch(
        r"esp\+(?P<index>e(?:ax|bx|cx|dx|si|di|bp))\*4"
        r"(?:(?P<sign>[+-])(?P<value>0x[0-9a-f]+|\d+))?",
        expression,
    )
    if indexed is None:
        return None
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    if len(body) < 4 or body[0] != 0x8B:
        return None
    modrm = body[1]
    mode = modrm >> 6
    if mode not in {1, 2} or (modrm & 0x07) != 0x04:
        return None
    sib = body[2]
    if (sib >> 6) != 2 or (sib & 0x07) != 0x04:
        return None
    registers = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    destination = registers[(modrm >> 3) & 0x07]
    index_register = registers[(sib >> 3) & 0x07]
    if index_register == "esp":
        return None
    if mode == 1 and len(body) == 4:
        displacement = struct.unpack("<b", body[3:4])[0]
    elif mode == 2 and len(body) == 7:
        displacement = struct.unpack("<i", body[3:7])[0]
    else:
        return None
    rendered_displacement = 0
    if indexed.group("value") is not None:
        rendered_displacement = _cc_cfg._parse_unsigned_assembly_integer(
            indexed.group("value")
        )
        if indexed.group("sign") == "-":
            rendered_displacement = -rendered_displacement
    prefix = match.group("prefix")
    if prefix is not None:
        prefix_sign = -1 if prefix.startswith("-") else 1
        raw_prefix = prefix[1:] if prefix[:1] in {"+", "-"} else prefix
        rendered_displacement += (
            prefix_sign * _cc_cfg._parse_unsigned_assembly_integer(raw_prefix)
        )
    rendered = (
        match.group("destination").lower(),
        indexed.group("index").lower(),
        rendered_displacement,
    )
    actual = (destination, index_register, displacement)
    if match.group("symbol") is not None:
        return actual if rendered[:2] == actual[:2] else None
    return actual if rendered == actual else None


def _exact_vc5_symbolic_indexed_stack_array_load(
    instruction: Instruction,
    *,
    index_register: str,
) -> tuple[str, int] | None:
    match = re.fullmatch(
        r"mov\s+e(?:ax|bx|cx|dx|si|di|sp|bp)\s*,\s*"
        r"(?P<source>(?:dword\s+(?:ptr\s+)?)?"
        r"[A-Za-z_?$][A-Za-z0-9_@$?]*\[[^\]]+\])",
        instruction.raw_text.strip(),
        flags=re.IGNORECASE,
    )
    if match is None:
        return None
    return _vc5_symbolic_stack_local(
        match.group("source"),
        index_register=index_register,
    )


def _is_exact_register_compare_immediate(
    instruction: Instruction,
    *,
    register: str,
) -> bool:
    """Recognize an exact non-writing ``CMP r32, imm8|imm32`` encoding."""
    match = re.fullmatch(
        rf"cmp\s+{re.escape(register)}\s*,\s*"
        r"(?P<value>0x[0-9a-f]+|\d+)",
        instruction.raw_text.strip(),
        flags=re.IGNORECASE,
    )
    if match is None:
        return False
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return False
    registers = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    register_code = registers.index(register.lower())
    expected_modrm = 0xF8 | register_code
    rendered = _cc_cfg._parse_unsigned_assembly_integer(match.group("value"))
    if len(body) == 3 and body[:2] == bytes((0x83, expected_modrm)):
        return rendered == body[2]
    if len(body) == 6 and body[:2] == bytes((0x81, expected_modrm)):
        return rendered == struct.unpack("<I", body[2:6])[0]
    return False


def _bounded_indexed_stack_array_loads(
    instructions: Sequence[Instruction],
    *,
    source: str,
    caller_start: int,
    caller_end: int,
) -> dict[int, str]:
    """Prove finite initialized stack-pointer arrays selected by a loop index."""
    addresses = _cc_cfg._instruction_runtime_addresses(
        instructions,
        source=source,
        caller_start=caller_start,
    )
    counts: dict[int, int] = {}
    for address in addresses:
        if address is not None and caller_start <= address < caller_end:
            counts[address] = counts.get(address, 0) + 1
    index_by_address = {
        address: index
        for index, address in enumerate(addresses)
        if address is not None and counts.get(address) == 1
    }
    result: dict[int, str] = {}
    for load_index, instruction in enumerate(instructions):
        load = _exact_indexed_stack_array_load(instruction)
        if load is None:
            continue
        _destination, index_register, base_slot = load
        symbolic_load = _exact_vc5_symbolic_indexed_stack_array_load(
            instruction,
            index_register=index_register,
        )
        if symbolic_load is not None and source != "cod":
            continue
        symbolic_name = symbolic_load[0] if symbolic_load is not None else ""
        symbolic_delta = (
            symbolic_load[1] - base_slot
            if symbolic_load is not None
            else None
        )
        initialization_indices = [
            index
            for index in range(max(0, load_index - 64), load_index)
            if _cc_cfg._instruction_mnemonic(instructions[index]) == "xor"
            and _cc_cfg._instruction_operand(instructions[index]).replace(" ", "").lower()
            == f"{index_register},{index_register}"
            and _cc_receiver_cursor._is_exact_self_xor_zero(
                instructions[index],
                destination=index_register,
            )
        ]
        if not initialization_indices:
            continue
        initialization_index = initialization_indices[-1]

        backedges: list[tuple[int, int]] = []
        for branch_index in range(
            load_index + 1,
            min(len(instructions), load_index + 128),
        ):
            branch = _cc_cfg._exact_local_direct_branch(
                instructions[branch_index],
                instruction_index=branch_index,
                instruction_addresses=addresses,
                instruction_index_by_address=index_by_address,
                source=source,
                caller_start=caller_start,
                caller_end=caller_end,
            )
            if (
                branch is not None
                and branch[0] == "conditional"
                and branch[1] <= load_index
                and branch[1] > initialization_index
                and _cc_cfg._instruction_mnemonic(instructions[branch_index])
                in {"jb", "jl"}
            ):
                backedges.append((branch_index, branch[1]))
        if len(backedges) != 1:
            continue
        branch_index, branch_target = backedges[0]
        if branch_index < 2:
            continue
        compare = re.fullmatch(
            rf"cmp\s+{re.escape(index_register)}\s*,\s*"
            r"(?P<count>0x[0-9a-f]+|\d+)",
            instructions[branch_index - 1].raw_text.strip(),
            flags=re.IGNORECASE,
        )
        if compare is None:
            continue
        element_count = _cc_cfg._parse_unsigned_assembly_integer(compare.group("count"))
        if not 1 <= element_count <= 16:
            continue
        increment_indices = [
            index
            for index in range(load_index + 1, branch_index - 1)
            if _cc_cfg._instruction_mnemonic(instructions[index]) == "inc"
            and _cc_cfg._instruction_operand(instructions[index]).strip().lower()
            == index_register
        ]
        if len(increment_indices) != 1:
            continue
        increment_index = increment_indices[0]
        written_indices = [
            index
            for index in range(initialization_index + 1, branch_index)
            if _cc_instructions.may_clobber_register(instructions[index], index_register)
            and index not in {increment_index, branch_index - 1}
            and not _is_exact_register_compare_immediate(
                instructions[index],
                register=index_register,
            )
        ]
        if written_indices:
            continue
        expected_slots = {
            base_slot + element_index * 4
            for element_index in range(element_count)
        }
        stores_by_slot: dict[int, list[int]] = {
            slot: [] for slot in expected_slots
        }
        clobbered = False
        for index in range(initialization_index + 1, branch_index):
            store = _exact_stack_slot_store(instructions[index])
            symbolic_store = _exact_vc5_symbolic_stack_slot_store(
                instructions[index]
            )
            encoded_slot = (
                store[0] if store is not None
                else symbolic_store[0] if symbolic_store is not None
                else None
            )
            if encoded_slot in expected_slots:
                store_matches_load = (
                    (
                        symbolic_load is None
                        and store is not None
                        and symbolic_store is None
                    )
                    or (
                        symbolic_load is not None
                        and symbolic_store is not None
                        and symbolic_store[2] == symbolic_name
                        and symbolic_store[3] - symbolic_store[0]
                        == symbolic_delta
                    )
                )
                if store_matches_load:
                    stores_by_slot[encoded_slot].append(index)
                else:
                    clobbered = True
                if index >= load_index:
                    clobbered = True
            if (
                index != load_index
                and _exact_indexed_stack_array_load(instructions[index])
                is not None
            ):
                clobbered = True
        if (
            clobbered
            or branch_target > load_index
            or any(
                len(store_indices) != 1
                or store_indices[0] >= load_index
                for store_indices in stores_by_slot.values()
            )
        ):
            continue
        result[load_index] = (
            f"load({_abstract_with_displacement('stack', base_slot)})"
        )
    return result


def _exact_register_move(instruction: Instruction) -> tuple[str, str] | None:
    match = re.fullmatch(
        r"mov\s+(?P<destination>e(?:ax|bx|cx|dx|si|di|sp|bp))\s*,\s*"
        r"(?P<source>e(?:ax|bx|cx|dx|si|di|sp|bp))",
        instruction.raw_text.strip(),
        flags=re.IGNORECASE,
    )
    if match is None:
        return None
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    if len(body) != 2 or body[0] not in {0x89, 0x8B} or body[1] >> 6 != 3:
        return None
    registers = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    reg = registers[(body[1] >> 3) & 0x07]
    rm = registers[body[1] & 0x07]
    destination, source = (reg, rm) if body[0] == 0x8B else (rm, reg)
    rendered = (
        match.group("destination").lower(),
        match.group("source").lower(),
    )
    return (destination, source) if rendered == (destination, source) else None


def _exact_register_stack_transfer(
    instruction: Instruction,
) -> tuple[str, str] | None:
    """Decode one exact unprefixed PUSH/POP of a 32-bit register."""

    mnemonic = _cc_cfg._instruction_mnemonic(instruction)
    if mnemonic not in {"push", "pop"}:
        return None
    match = re.fullmatch(
        r"e(?:ax|bx|cx|dx|si|di|sp|bp)",
        _cc_cfg._instruction_operand(instruction).strip(),
        flags=re.IGNORECASE,
    )
    if match is None:
        return None
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    opcode_base = 0x50 if mnemonic == "push" else 0x58
    if len(body) != 1 or not opcode_base <= body[0] <= opcode_base + 7:
        return None
    encoded = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )[body[0] - opcode_base]
    rendered = match.group(0).lower()
    return (mnemonic, encoded) if rendered == encoded else None


def _exact_register_add_immediate(
    instruction: Instruction,
) -> tuple[str, int] | None:
    match = re.fullmatch(
        r"add\s+(?P<destination>e(?:ax|bx|cx|dx|si|di|sp|bp))\s*,\s*"
        r"(?P<value>0x[0-9a-f]+|\d+)",
        instruction.raw_text.strip(),
        flags=re.IGNORECASE,
    )
    if match is None:
        return None
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    registers = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    destination = match.group("destination").lower()
    encoded_destination = ""
    encoded_immediate = -1
    if len(body) == 3 and body[0] == 0x83 and body[1] >> 6 == 3:
        if (body[1] >> 3) & 0x07 == 0:
            encoded_destination = registers[body[1] & 0x07]
            encoded_immediate = struct.unpack("<b", body[2:3])[0]
    elif len(body) == 6 and body[0] == 0x81 and body[1] >> 6 == 3:
        if (body[1] >> 3) & 0x07 == 0:
            encoded_destination = registers[body[1] & 0x07]
            encoded_immediate = struct.unpack("<i", body[2:6])[0]
    elif len(body) == 5 and body[0] == 0x05:
        encoded_destination = "eax"
        encoded_immediate = struct.unpack("<i", body[1:5])[0]
    rendered_immediate = _cc_cfg._parse_unsigned_assembly_integer(match.group("value"))
    if (
        destination != encoded_destination
        or rendered_immediate != encoded_immediate
        or not 0 < encoded_immediate <= 0x10000
        or encoded_immediate % 4
    ):
        return None
    return destination, encoded_immediate




def _exact_register_move_immediate(
    instruction: Instruction,
) -> tuple[str, int] | None:
    """Decode one unprefixed positive ``MOV r32, imm32`` exactly."""

    match = re.fullmatch(
        r"mov\s+(?P<destination>e(?:ax|bx|cx|dx|si|di|sp|bp))\s*,\s*"
        r"(?P<value>0x[0-9a-f]+|\d+)",
        instruction.raw_text.strip(),
        flags=re.IGNORECASE,
    )
    if match is None:
        return None
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    registers = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    destination = match.group("destination").lower()
    encoded_destination = ""
    encoded_immediate = -1
    if len(body) == 5 and 0xB8 <= body[0] <= 0xBF:
        encoded_destination = registers[body[0] - 0xB8]
        encoded_immediate = struct.unpack("<I", body[1:5])[0]
    elif (
        len(body) == 6
        and body[0] == 0xC7
        and body[1] >> 6 == 3
        and ((body[1] >> 3) & 7) == 0
    ):
        encoded_destination = registers[body[1] & 7]
        encoded_immediate = struct.unpack("<I", body[2:6])[0]
    rendered = _cc_cfg._parse_unsigned_assembly_integer(match.group("value"))
    if (
        destination != encoded_destination
        or rendered != encoded_immediate
        or not 0 <= encoded_immediate <= 0x7FFFFFFF
    ):
        return None
    return destination, encoded_immediate


def _exact_register_compare_immediate(
    instruction: Instruction,
) -> tuple[str, int] | None:
    """Decode one unprefixed ``CMP r32, imm`` with render/byte agreement."""

    match = re.fullmatch(
        r"cmp\s+(?P<register>e(?:ax|bx|cx|dx|si|di|sp|bp))\s*,\s*"
        r"(?P<value>0x[0-9a-f]+|\d+)",
        instruction.raw_text.strip(),
        flags=re.IGNORECASE,
    )
    if match is None:
        return None
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    registers = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    encoded_register = ""
    encoded_immediate = -1
    if (
        len(body) == 3
        and body[0] == 0x83
        and body[1] >> 6 == 3
        and ((body[1] >> 3) & 7) == 7
    ):
        encoded_register = registers[body[1] & 7]
        encoded_immediate = struct.unpack("<b", body[2:3])[0]
    elif (
        len(body) == 6
        and body[0] == 0x81
        and body[1] >> 6 == 3
        and ((body[1] >> 3) & 7) == 7
    ):
        encoded_register = registers[body[1] & 7]
        encoded_immediate = struct.unpack("<i", body[2:6])[0]
    elif len(body) == 5 and body[0] == 0x3D:
        encoded_register = "eax"
        encoded_immediate = struct.unpack("<i", body[1:5])[0]
    rendered = _cc_cfg._parse_unsigned_assembly_integer(match.group("value"))
    if (
        match.group("register").lower() != encoded_register
        or rendered != encoded_immediate
        or not 0 <= encoded_immediate <= 0x7FFFFFFF
    ):
        return None
    return encoded_register, encoded_immediate


def _exact_register_imul_immediate(
    instruction: Instruction,
) -> tuple[str, str, int] | None:
    """Decode exact two/three-operand ``IMUL r32,r32,imm`` arithmetic."""

    match = re.fullmatch(
        r"imul\s+(?P<destination>e(?:ax|bx|cx|dx|si|di|sp|bp))\s*,\s*"
        r"(?:(?P<source>e(?:ax|bx|cx|dx|si|di|sp|bp))\s*,\s*)?"
        r"(?P<value>0x[0-9a-f]+|\d+)",
        instruction.raw_text.strip(),
        flags=re.IGNORECASE,
    )
    if match is None:
        return None
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    registers = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    if (
        len(body) not in {3, 6}
        or body[0] not in {0x69, 0x6B}
        or body[1] >> 6 != 3
        or (body[0] == 0x69 and len(body) != 6)
        or (body[0] == 0x6B and len(body) != 3)
    ):
        return None
    destination = registers[(body[1] >> 3) & 7]
    source = registers[body[1] & 7]
    encoded = (
        struct.unpack("<i", body[2:6])[0]
        if body[0] == 0x69
        else struct.unpack("<b", body[2:3])[0]
    )
    rendered_destination = match.group("destination").lower()
    rendered_source = (match.group("source") or rendered_destination).lower()
    rendered_immediate = _cc_cfg._parse_unsigned_assembly_integer(match.group("value"))
    if (
        (rendered_destination, rendered_source)
        != (destination, source)
        or rendered_immediate != encoded
        or not 0 < encoded <= _cc_catalog._BOUNDED_TARGETLESS_ARITHMETIC_LIMIT
    ):
        return None
    return destination, source, encoded


def _exact_register_shift_left_immediate(
    instruction: Instruction,
) -> tuple[str, int] | None:
    """Decode exact unprefixed ``SHL r32, imm8`` arithmetic."""

    match = re.fullmatch(
        r"(?:shl|sal)\s+"
        r"(?P<register>e(?:ax|bx|cx|dx|si|di|sp|bp))\s*,\s*"
        r"(?P<value>0x[0-9a-f]+|\d+)",
        instruction.raw_text.strip(),
        flags=re.IGNORECASE,
    )
    if match is None:
        return None
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    registers = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    if (
        len(body) != 3
        or body[0] != 0xC1
        or body[1] >> 6 != 3
        or ((body[1] >> 3) & 7) != 4
    ):
        return None
    register = registers[body[1] & 7]
    shift = body[2]
    if (
        match.group("register").lower() != register
        or _cc_cfg._parse_unsigned_assembly_integer(match.group("value")) != shift
        or not 1 <= shift <= 16
    ):
        return None
    return register, shift


def _exact_register_lea(
    instruction: Instruction,
) -> tuple[str, str, int] | None:
    match = re.fullmatch(
        r"lea\s+(?P<destination>e(?:ax|bx|cx|dx|si|di|sp|bp))\s*,\s*"
        r"(?:dword\s+(?:ptr\s+)?)?(?P<source>\[[^\]]+\])",
        instruction.raw_text.strip(),
        flags=re.IGNORECASE,
    )
    if match is None:
        return None
    expression_match = re.fullmatch(
        r"(?P<base>e(?:ax|bx|cx|dx|si|di|sp|bp))"
        r"(?:(?P<sign>[+-])(?P<value>0x[0-9a-f]+|\d+))?",
        _cc_targets._exact_memory_expression(match.group("source")).lower(),
    )
    if expression_match is None:
        return None
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    if len(body) < 2 or body[0] != 0x8D:
        return None
    modrm = body[1]
    mode = modrm >> 6
    if (
        mode == 3
        or (modrm & 0x07) == 0x04
        or (mode == 0 and (modrm & 0x07) == 0x05)
    ):
        return None
    registers = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    destination = registers[(modrm >> 3) & 0x07]
    base = registers[modrm & 0x07]
    if mode == 0 and len(body) == 2:
        displacement = 0
    elif mode == 1 and len(body) == 3:
        displacement = struct.unpack("<b", body[2:3])[0]
    elif mode == 2 and len(body) == 6:
        displacement = struct.unpack("<i", body[2:6])[0]
    else:
        return None
    rendered = 0
    if expression_match.group("value") is not None:
        rendered = _cc_cfg._parse_unsigned_assembly_integer(
            expression_match.group("value")
        )
        if expression_match.group("sign") == "-":
            rendered = -rendered
    expected = (
        match.group("destination").lower(),
        expression_match.group("base").lower(),
        rendered,
    )
    actual = (destination, base, displacement)
    return actual if actual == expected else None
