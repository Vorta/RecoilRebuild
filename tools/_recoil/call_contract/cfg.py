"""Recoil call-contract cfg evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import errors as _cc_errors
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract import instructions as _cc_instructions
from _recoil.call_contract import listing as _cc_listing
from _recoil.call_contract import receiver_cursor as _cc_receiver_cursor
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        CandidateCallerDefinition,
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedMemberVptrStorageBridge,
    )

import re
import struct
from collections import Counter
from dataclasses import replace
from typing import Any, Iterable, Mapping, NoReturn, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_DIR32,
    IMAGE_REL_I386_REL32,
    CoffFunctionBytes,
    CoffObject,
    Instruction,
)
from _recoil.lib.binja import BinaryNinjaBridge, BridgeError
from _recoil.lib.progress import address_value, normalize_address


def _instruction_mnemonic(instruction: Instruction) -> str:
    return instruction.raw_text.split(None, 1)[0].lower() if instruction.raw_text else ""


def _instruction_operand(instruction: Instruction) -> str:
    parts = instruction.raw_text.split(None, 1)
    return parts[1].strip() if len(parts) == 2 else ""


def _exact_invocation_encoding(
    instruction: Instruction,
    *,
    mnemonic: str,
) -> bool:
    """Require the rendered CALL/JMP form to agree with exact x86 bytes.

    Binary Ninja may return a containing function for an overlapping EH
    continuation.  The continuation's first bytes and rendered instruction
    remain authoritative: a TEST (or any other non-invocation encoding) must
    never enter the static invocation census merely because stale/overlapping
    text labels it as a CALL.  Prefixes are intentionally rejected here; no
    reviewed call-contract row depends on a prefixed CALL/JMP form.
    """

    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return False
    def exact_ff_group_length() -> bool:
        if len(body) < 2 or body[0] != 0xFF:
            return False
        modrm = body[1]
        mode = modrm >> 6
        rm = modrm & 0x07
        length = 2
        sib_base: int | None = None
        if mode != 3 and rm == 4:
            if len(body) <= length:
                return False
            sib_base = body[length] & 0x07
            length += 1
        if mode == 0 and (rm == 5 or (rm == 4 and sib_base == 5)):
            length += 4
        elif mode == 1:
            length += 1
        elif mode == 2:
            length += 4
        return len(body) == length

    if mnemonic == "call":
        return (
            (len(body) == 5 and body[0] == 0xE8)
            or (
                exact_ff_group_length()
                and (
                    ((body[1] >> 3) & 0x07) == 2
                    or (
                        ((body[1] >> 3) & 0x07) == 3
                        and body[1] >> 6 != 3
                    )
                )
            )
            or (len(body) == 7 and body[0] == 0x9A)
        )
    if mnemonic == "jmp":
        return (
            (len(body) == 2 and body[0] == 0xEB)
            or (len(body) == 5 and body[0] == 0xE9)
            or (
                exact_ff_group_length()
                and (
                    ((body[1] >> 3) & 0x07) == 4
                    or (
                        ((body[1] >> 3) & 0x07) == 5
                        and body[1] >> 6 != 3
                    )
                )
            )
            or (len(body) == 7 and body[0] == 0xEA)
        )
    return False




def _exact_local_indexed_jump_table_address(
    instruction: Instruction,
    *,
    caller_start: int,
    caller_end: int,
) -> int | None:
    """Prove an encoded absolute indexed JMP table is caller-local.

    This classification is intentionally independent from complete switch
    edge recovery.  An exact ``FF /4`` SIB form with scale four, no base, and
    an absolute table base inside the selected caller extent is local control
    flow even when its cardinality or every successor cannot be recovered.
    Unknown successors terminate forward provenance and remain unresolved;
    the dispatch itself is never reinterpreted as an external tail call.
    """

    if _instruction_mnemonic(instruction) != "jmp":
        return None
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    if (
        len(body) != 7
        or body[0:2] != b"\xff\x24"
        or body[2] >> 6 != 2
        or body[2] & 0x07 != 5
        or ((body[2] >> 3) & 0x07) == 4
    ):
        return None
    table_address = struct.unpack_from("<I", body, 3)[0]
    rendered_addresses = _cc_catalog.ADDRESS_RE.findall(_instruction_operand(instruction))
    memory_operands = _cc_catalog.MEMORY_RE.findall(_instruction_operand(instruction))
    index_registers = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    rendered_index = re.search(
        r"\b(?P<register>e(?:ax|cx|dx|bx|bp|si|di))\s*\*\s*4\b",
        memory_operands[0] if memory_operands else "",
        flags=re.IGNORECASE,
    )
    if (
        len(rendered_addresses) != 1
        or address_value(rendered_addresses[0]) != table_address
        or len(memory_operands) != 1
        or rendered_index is None
        or rendered_index.group("register").lower()
        != index_registers[(body[2] >> 3) & 0x07]
        or not caller_start <= table_address < caller_end
    ):
        return None
    return table_address


def _parse_unsigned_assembly_integer(token: str) -> int:
    """Parse one exact unsigned assembly literal without Python octal rules."""
    if re.fullmatch(r"\d+", token):
        return int(token, 10)
    if re.fullmatch(r"0x[0-9a-f]+", token, flags=re.IGNORECASE):
        return int(token[2:], 16)
    raise ValueError(f"invalid unsigned assembly integer literal {token!r}")


def _source_instruction_address(instruction: Instruction) -> str:
    parts = instruction.source_line.strip().split()
    if not parts:
        return ""
    raw = parts[0].rstrip(":")
    if re.fullmatch(r"[0-9a-fA-F]{5,8}", raw):
        return normalize_address(f"0x{raw}")
    return ""


def _instruction_runtime_address(
    instruction: Instruction,
    *,
    source: str,
    caller_start: int,
) -> int | None:
    raw_address = _source_instruction_address(instruction)
    if not raw_address:
        return None
    value = address_value(raw_address)
    if source == "cod":
        return caller_start + value
    if source == "bn" and value < caller_start:
        # Binary Ninja normally renders absolute virtual addresses, but its
        # bounded body renderers may use a zero-based body coordinate.  Bind
        # both renderings to the one runtime coordinate here; downstream
        # producers and consumers must never independently guess which space
        # an instruction row occupies.
        return caller_start + value
    return value


def _exact_instruction_byte_length(
    instruction: Instruction,
) -> int | None:
    if not instruction.bytes or any(
        re.fullmatch(r"[0-9a-fA-F]{2}", item) is None
        for item in instruction.bytes
    ):
        return None
    return len(instruction.bytes)


def _instruction_runtime_addresses(
    instructions: Sequence[Instruction],
    *,
    source: str,
    caller_start: int,
) -> tuple[int | None, ...]:
    """Resolve instruction starts, including exact VC5 COD continuations.

    VC5 wraps a six-byte instruction after five rendered bytes.  The assembly
    parser joins those bytes to the mnemonic continuation but necessarily
    retains the continuation as ``source_line``, which has no explicit offset.
    Derive that one start only from the immediately preceding resolved
    instruction and its exact encoded length.  An explicit offset remains
    authoritative and resets the local sequence after gaps or malformed rows.
    """
    addresses: list[int | None] = []
    previous_address: int | None = None
    previous_length: int | None = None
    for instruction in instructions:
        explicit_address = _instruction_runtime_address(
            instruction,
            source=source,
            caller_start=caller_start,
        )
        if explicit_address is not None:
            address = explicit_address
        elif (
            source == "cod"
            and previous_address is not None
            and previous_length is not None
        ):
            address = previous_address + previous_length
        else:
            address = None
        addresses.append(address)
        previous_address = address
        previous_length = (
            _exact_instruction_byte_length(instruction)
            if address is not None
            else None
        )
    return tuple(addresses)


def _closed_candidate_invocation_runtime_address(
    instructions: Sequence[Instruction],
    *,
    instruction_index: int,
    caller_start: int,
    caller_end: int,
    caller_definition: CandidateCallerDefinition,
) -> int:
    """Recover one addressless candidate invocation from exact closed bytes.

    VC5 can print an offset-bearing first line for a wrapped invocation and put
    the mnemonic on an addressless continuation retained by the shared parser.
    Bind that invocation to a runtime coordinate only when the containing COFF
    body, an explicit neighboring coordinate, cumulative instruction lengths,
    exact invocation bytes, and any relocation field all agree.  This is a
    coordinate proof only; it grants no target/provider identity.
    """

    def reject(detail: str) -> NoReturn:
        raise ValueError(
            "candidate invocation has no exact byte-closed runtime coordinate: "
            + detail
        )

    if (
        not 0 <= instruction_index < len(instructions)
        or caller_definition.section_start != 0
        or caller_definition.section_end != len(caller_definition.data)
        or len(caller_definition.relocation_mask)
        != len(caller_definition.data)
        or caller_start >= caller_end
        or _source_instruction_address(instructions[instruction_index])
    ):
        reject("instruction index, caller extent, or explicit-coordinate drift")

    explicit = tuple(
        _instruction_runtime_address(
            instruction,
            source="cod",
            caller_start=caller_start,
        )
        for instruction in instructions
    )
    previous_explicit = next(
        (
            index
            for index in range(instruction_index - 1, -1, -1)
            if explicit[index] is not None
        ),
        None,
    )
    next_explicit = next(
        (
            index
            for index in range(instruction_index + 1, len(instructions))
            if explicit[index] is not None
        ),
        None,
    )
    terminal_split_parts = (
        instructions[instruction_index]
        .source_line.split(";", 1)[0]
        .strip()
        .split()
    )
    if (
        next_explicit is not None
        and tuple(
            item.casefold()
            for item in instructions[instruction_index].bytes
        )
        == ("ff", "25", "00", "00", "00", "00")
        and len(terminal_split_parts) >= 2
        and terminal_split_parts[0].casefold() == "00"
        and terminal_split_parts[1].casefold() == "jmp"
    ):
        reject("nonterminal wrapped FF25 continuation")
    terminal_wrapped = False
    inferred: dict[int, int] = {}
    if next_explicit is None:
        instruction = instructions[instruction_index]
        marker_parts = instruction.source_line.rsplit(
            _cc_catalog._COD_TERMINAL_WRAP_MARKER,
            1,
        )
        continuation_parts = (
            marker_parts[0].split(";", 1)[0].strip().split()
            if len(marker_parts) == 2
            else []
        )
        first_parts = (
            marker_parts[1].strip().split()
            if len(marker_parts) == 2
            else []
        )
        if (
            instruction_index != len(instructions) - 1
            or len(marker_parts) != 2
            or instruction.source_line.count(_cc_catalog._COD_TERMINAL_WRAP_MARKER) != 1
            or len(continuation_parts) != 5
            or continuation_parts[0].casefold() != "00"
            or continuation_parts[1].casefold() != "jmp"
            or continuation_parts[2].casefold() != "dword"
            or continuation_parts[3].casefold() != "ptr"
            or re.fullmatch(
                r"[A-Za-z_?$@][A-Za-z0-9_?$@]*",
                continuation_parts[4],
            )
            is None
            or _instruction_operand(instruction).strip().casefold()
            != f"dword {continuation_parts[4]}".casefold()
            or len(first_parts) != 6
            or re.fullmatch(r"[0-9A-Fa-f]{5}", first_parts[0]) is None
            or tuple(item.casefold() for item in first_parts[1:])
            != ("ff", "25", "00", "00", "00")
            or caller_definition.section_index <= 0
            or caller_end - caller_start != len(caller_definition.data)
        ):
            reject("unclosed trailing row")
        invocation_address = caller_start + int(first_parts[0], 16)
        terminal_wrapped = True
    else:
        run_start = 0 if previous_explicit is None else previous_explicit + 1
        if any(
            explicit[index] is not None
            for index in range(run_start, next_explicit)
        ):
            reject("ambiguous explicit coordinate inside inferred run")
        if previous_explicit is None:
            cursor = caller_start
        else:
            previous_length = _exact_instruction_byte_length(
                instructions[previous_explicit]
            )
            if previous_length is None:
                reject("unparseable preceding instruction bytes")
            cursor = int(explicit[previous_explicit]) + previous_length

        for index in range(run_start, next_explicit):
            length = _exact_instruction_byte_length(instructions[index])
            if length is None:
                reject("unparseable inferred instruction bytes")
            inferred[index] = cursor
            cursor += length
        if cursor != explicit[next_explicit]:
            reject("next explicit coordinate does not close cumulative bytes")
        invocation_address = inferred.get(instruction_index)
    invocation_length = _exact_instruction_byte_length(
        instructions[instruction_index]
    )
    if invocation_address is None or invocation_length is None:
        reject("invocation is outside the closed inferred run")

    all_coordinates = [
        int(address)
        for address in explicit
        if address is not None
    ] + list(inferred.values())
    body_offset = invocation_address - caller_start
    if (
        len(all_coordinates) != len(set(all_coordinates))
        or not caller_start <= invocation_address < caller_end
        or invocation_address + invocation_length > caller_end
        or body_offset < 0
        or body_offset + invocation_length > len(caller_definition.data)
    ):
        reject("duplicate coordinate or caller/COFF bounds drift")

    instruction = instructions[instruction_index]
    mnemonic = _instruction_mnemonic(instruction)
    if mnemonic not in {"call", "jmp"} or not _exact_invocation_encoding(
        instruction, mnemonic=mnemonic
    ):
        reject("opcode/rendered invocation drift")
    try:
        invocation_body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        reject("unparseable invocation bytes")
    if (
        caller_definition.data[
            body_offset : body_offset + invocation_length
        ]
        != invocation_body
    ):
        reject("COD/COFF invocation bytes drift")

    relocation_offset: int | None = None
    relocation_type: int | None = None
    operand_symbol = ""
    operand = _instruction_operand(instruction).strip()
    if len(invocation_body) == 5 and invocation_body[0] in {0xE8, 0xE9}:
        if invocation_body[1:] == b"\0" * 4:
            relocation_offset = body_offset + 1
            relocation_type = IMAGE_REL_I386_REL32
            operand_symbol = (
                _cc_targets._cod_space_bearing_direct_target(operand, instruction.source_line)
                or operand
            )
    elif (
        len(invocation_body) == 6
        and invocation_body[:2] in {b"\xff\x15", b"\xff\x25"}
        and invocation_body[2:] == b"\0" * 4
    ):
        relocation_offset = body_offset + 2
        relocation_type = IMAGE_REL_I386_DIR32
        operand_symbol = re.sub(
            r"^(?:(?:byte|word|dword|qword)\s+(?:ptr\s+)?)",
            "",
            operand,
            flags=re.IGNORECASE,
        ).strip()
        if operand_symbol.startswith("[") and operand_symbol.endswith("]"):
            operand_symbol = operand_symbol[1:-1].strip()

    overlapping_relocations = [
        relocation
        for relocation in caller_definition.relocations
        if body_offset
        <= relocation.offset
        < body_offset + invocation_length
    ]
    if relocation_offset is not None:
        matching_relocations = [
            relocation
            for relocation in overlapping_relocations
            if relocation.offset == relocation_offset
            and relocation.type == relocation_type
            and relocation.symbol_name == operand_symbol
        ]
        field_end = relocation_offset + 4
        nonfield_offsets = {
            offset
            for offset in range(body_offset, body_offset + invocation_length)
            if not relocation_offset <= offset < field_end
        }
        if (
            not operand_symbol
            or len(matching_relocations) != 1
            or len(overlapping_relocations) != 1
            or caller_definition.data[relocation_offset:field_end] != b"\0" * 4
            or not all(
                caller_definition.relocation_mask[offset]
                for offset in range(relocation_offset, field_end)
            )
            or any(
                caller_definition.relocation_mask[offset]
                for offset in nonfield_offsets
            )
        ):
            reject("opcode operand/COFF relocation agreement drift")
        if terminal_wrapped:
            body_end = body_offset + invocation_length
            if (
                len(caller_definition.relocations) != 1
                or any(
                    byte != 0x90
                    for byte in caller_definition.data[body_end:]
                )
                or any(caller_definition.relocation_mask[body_end:])
            ):
                reject("terminal section padding/relocation population drift")
            symbol_population = (
                caller_definition.undefined_external_functions.count(
                    operand_symbol
                )
                + caller_definition.defined_external_functions.count(
                    operand_symbol
                )
                + caller_definition.undefined_external_data.count(
                    operand_symbol
                )
                + caller_definition.defined_external_data.count(
                    operand_symbol
                )
            )
            if symbol_population != 1:
                reject("terminal relocation symbol population drift")
    elif overlapping_relocations or any(
        caller_definition.relocation_mask[offset]
        for offset in range(body_offset, body_offset + invocation_length)
    ):
        reject("unexpected relocation in nonrelocated invocation")
    return invocation_address


def _exact_cod_split_absolute_mov_instruction_address(
    instructions: Sequence[Instruction],
    *,
    instruction_index: int,
    instruction_addresses: Sequence[int | None],
    instruction_address_counts: Mapping[int, int],
    caller_start: int,
    caller_end: int,
) -> str:
    """Recover one unique, contiguous VC5 split absolute-MOV offset.

    VC5 renders a six-byte ``8B /r disp32`` instruction as five bytes on an
    offset-bearing row and the final byte plus mnemonic on a continuation row.
    ``parse_assembly`` correctly joins the bytes, but the resulting
    ``Instruction.source_line`` is the addressless continuation.  Publish the
    derived relative offset only when both neighboring instruction boundaries,
    the one-byte continuation, and the unique computed address agree exactly.
    Malformed or ambiguous rows remain addressless and therefore fail closed in
    the IAT provenance verifier.
    """
    if not (0 < instruction_index < len(instructions) - 1):
        return ""
    instruction = instructions[instruction_index]
    if _source_instruction_address(instruction):
        return ""
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return ""
    continuation_parts = instruction.source_line.strip().split()
    if (
        len(body) != 6
        or body[0] != 0x8B
        or body[1] >> 6 != 0
        or body[1] & 0x07 != 0x05
        or len(continuation_parts) < 2
        or continuation_parts[0] != "00"
        or continuation_parts[1].casefold() != "mov"
        or _instruction_mnemonic(instruction) != "mov"
    ):
        return ""
    address = instruction_addresses[instruction_index]
    previous_address = instruction_addresses[instruction_index - 1]
    previous_length = _exact_instruction_byte_length(
        instructions[instruction_index - 1]
    )
    next_explicit_address = _instruction_runtime_address(
        instructions[instruction_index + 1],
        source="cod",
        caller_start=caller_start,
    )
    if (
        address is None
        or previous_address is None
        or previous_length is None
        or not caller_start <= address < caller_end
        or instruction_address_counts.get(address, 0) != 1
        or previous_address + previous_length != address
        or next_explicit_address != address + len(body)
    ):
        return ""
    return normalize_address(address - caller_start)


def _reviewed_cod_register_definition_address(
    instructions: Sequence[Instruction],
    *,
    instruction_index: int,
    instruction_addresses: Sequence[int | None],
    instruction_address_counts: Mapping[int, int],
    caller_start: int,
    caller_end: int,
    reviewed_absolute_storage_load_bridges: Mapping[
        str, ReviewedAbsoluteStorageLoadBridge
    ],
    reviewed_member_vptr_load_bridges: Mapping[
        str, ReviewedMemberVptrStorageBridge
    ],
    reviewed_iat_register_definition_offsets: frozenset[str],
) -> str:
    """Return a unique explicit, exact split-IAT, or reviewed bridge offset."""
    instruction = instructions[instruction_index]
    address = instruction_addresses[instruction_index]
    relative_address = (
        normalize_address(address - caller_start)
        if address is not None
        else ""
    )
    if (
        address is not None
        and _source_instruction_address(instruction)
        and instruction_address_counts.get(address, 0) == 1
    ):
        return relative_address
    split_address = _exact_cod_split_absolute_mov_instruction_address(
        instructions,
        instruction_index=instruction_index,
        instruction_addresses=instruction_addresses,
        instruction_address_counts=instruction_address_counts,
        caller_start=caller_start,
        caller_end=caller_end,
    )
    if split_address:
        return split_address
    if relative_address in reviewed_iat_register_definition_offsets:
        # A caller-scoped candidate proof may authorize an otherwise exact
        # split MOV whose immediate right neighbor is another wrapped MOV.
        # The proof owns the complete COD/COFF cover, relocation, unique CFG
        # definition, and register lifetime; this path supplies only the
        # already-proved relative definition address to the generic marker.
        return relative_address
    # Reviewed aggregate-load and member-vptr bridges already require exact
    # candidate COFF/COD bytes, operands, relocation-backed storage lineage,
    # unique inferred offsets, and an unclobbered path to the reviewed call.
    # Preserve those independent proof paths for abbreviated COD
    # views; an unreviewed addressless register load remains unresolved.
    if relative_address in {
        *reviewed_absolute_storage_load_bridges,
        *reviewed_member_vptr_load_bridges,
    }:
        return relative_address
    return ""


def _rendered_numeric_branch_target(
    instruction: Instruction,
    *,
    source: str,
    caller_start: int,
) -> int | None:
    if source == "cod" and _exact_cod_local_branch_label(
        _instruction_operand(instruction)
    ):
        # VC5's generated labels commonly contain only decimal digits after
        # ``$L``.  Those digits are a label serial, not a rendered address;
        # the exact branch bytes below remain the sole target authority.
        return None
    matches = _cc_catalog.ADDRESS_RE.findall(_instruction_operand(instruction))
    if not matches:
        return None
    value = address_value(matches[-1])
    return caller_start + value if source == "cod" else value


def _exact_cod_local_branch_label(operand: str) -> bool:
    return _cc_catalog._COD_LOCAL_BRANCH_LABEL_RE.fullmatch(operand.strip()) is not None


def _exact_local_direct_branch(
    instruction: Instruction,
    *,
    instruction_index: int,
    instruction_addresses: Sequence[int | None],
    instruction_index_by_address: Mapping[int, int],
    source: str,
    caller_start: int,
    caller_end: int,
) -> tuple[str, int] | None:
    """Decode one exact unprefixed local x86 branch.

    Only direct short/near Jcc and JMP encodings participate in forward
    provenance.  Text is not trusted to supply a target, but an available
    numeric rendering must agree with the encoded relative displacement.
    A VC5 COD external REL32 operand has a zero placeholder until link time;
    require an exact local-label or numeric rendering before treating that
    placeholder as caller-local control flow.
    """
    address = instruction_addresses[instruction_index]
    if address is None:
        return None
    rendered_target = _rendered_numeric_branch_target(
        instruction,
        source=source,
        caller_start=caller_start,
    )
    if source == "cod" and rendered_target is None:
        if not _exact_cod_local_branch_label(
            _instruction_operand(instruction)
        ):
            return None
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    mnemonic = _instruction_mnemonic(instruction)
    kind: str
    displacement: int
    if (
        len(body) == 2
        and 0x70 <= body[0] <= 0x7F
        and mnemonic in _cc_catalog.SHORT_JCC_MNEMONICS[body[0] - 0x70]
    ):
        kind = "conditional"
        displacement = struct.unpack("<b", body[1:2])[0]
    elif (
        len(body) == 6
        and body[0] == 0x0F
        and 0x80 <= body[1] <= 0x8F
        and mnemonic in _cc_catalog.SHORT_JCC_MNEMONICS[body[1] - 0x80]
    ):
        kind = "conditional"
        displacement = struct.unpack("<i", body[2:6])[0]
    elif len(body) == 2 and body[0] == 0xEB and mnemonic == "jmp":
        kind = "unconditional"
        displacement = struct.unpack("<b", body[1:2])[0]
    elif len(body) == 5 and body[0] == 0xE9 and mnemonic == "jmp":
        kind = "unconditional"
        displacement = struct.unpack("<i", body[1:5])[0]
    else:
        return None
    target = address + len(body) + displacement
    if not caller_start <= target < caller_end:
        return None
    target_index = instruction_index_by_address.get(target)
    if target_index is None:
        return None
    if rendered_target is not None and rendered_target != target:
        return None
    return kind, target_index


def _exact_return_terminates(instruction: Instruction) -> bool:
    mnemonic = _instruction_mnemonic(instruction)
    if mnemonic not in {"ret", "retn"}:
        return False
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return False
    operand = _instruction_operand(instruction).strip().lower()
    if body == b"\xc3":
        return operand in {"", "0", "0x0"}
    if len(body) != 3 or body[0] != 0xC2:
        return False
    try:
        rendered_immediate = _parse_unsigned_assembly_integer(operand)
    except ValueError:
        return False
    return rendered_immediate == struct.unpack("<H", body[1:3])[0]


def _empty_register_state() -> dict[str, str]:
    return {register: "" for register in _cc_catalog.REGISTER_STATE_NAMES}


def _register_state_snapshot(registers: Mapping[str, str]) -> dict[str, str]:
    return {
        register: str(registers.get(register, "") or "")
        for register in _cc_catalog.REGISTER_STATE_NAMES
    }


def _merge_register_states(
    predecessors: Sequence[Mapping[str, str]],
    *,
    reviewed_iat_register_joins: frozenset[tuple[str, str]] = frozenset(),
) -> dict[str, str]:
    merged = _empty_register_state()
    for register in _cc_catalog.REGISTER_STATE_NAMES:
        values = {
            str(predecessor.get(register, "") or "")
            for predecessor in predecessors
        }
        if len(values) == 1:
            value = next(iter(values))
            if value:
                merged[register] = value
            continue
        exact_iat_definitions = [
            re.fullmatch(
                r"exact-iat-load\("
                r"(?P<register>e(?:ax|bx|cx|dx|si|di|bp)),"
                r"0x[0-9a-f]+,"
                r"(?P<identity>iat:[^)]+)\)",
                value,
            )
            for value in values
        ]
        if (
            values
            and "" not in values
            and all(match is not None for match in exact_iat_definitions)
            and {
                match.group("register")
                for match in exact_iat_definitions
                if match is not None
            }
            == {register}
            and len(
                {
                    match.group("identity")
                    for match in exact_iat_definitions
                    if match is not None
                }
            )
            == 1
        ):
            identity = next(
                match.group("identity")
                for match in exact_iat_definitions
                if match is not None
            )
            if (register, identity) not in reviewed_iat_register_joins:
                continue
            # Every incoming path independently loads the same immutable IAT
            # identity into the same register.  Preserve that path-complete
            # fact without pretending the reaching definition has one site.
            merged[register] = f"exact-iat-join({register},{identity})"
            continue
        non_null = values - {"null"}
        if (
            len(values) == 2
            and len(non_null) == 1
            and "" not in values
        ):
            value = next(iter(non_null))
            if value.startswith("call-result(") and value.endswith(")"):
                merged[register] = f"nullable({value})"
    return merged


def _is_terminal_local_frame_teardown(
    instructions: Sequence[Instruction],
    *,
    call_index: int,
    cleanup_index: int,
    cleanup_bytes: int,
) -> bool:
    """Prove one ADD ESP is a terminal local-frame reversal, not call cleanup.

    The proof stays instruction-local and source-independent: the adjustment
    must reverse one unique earlier ``sub esp, N`` frame allocation, immediately
    follow an explicit EAX return-value write, and immediately precede the
    function return.  Missing or competing frame facts leave the ADD classified
    as caller cleanup.
    """

    if cleanup_index <= call_index or cleanup_index + 1 >= len(instructions):
        return False

    if _instruction_mnemonic(instructions[cleanup_index + 1]) not in {
        "ret", "retn"
    }:
        return False

    adjacent = list(instructions[call_index + 1 : cleanup_index])
    if (
        not adjacent
        or not _instruction_may_clobber_register(adjacent[-1], "eax")
    ):
        return False

    matching_allocations: list[int] = []
    for instruction_index, instruction in enumerate(
        instructions[:call_index]
    ):
        match = _cc_catalog.STACK_ALLOCATION_RE.fullmatch(
            instruction.raw_text.strip().lower()
        )
        if (
            match is not None
            and _parse_unsigned_assembly_integer(match.group(1))
            == cleanup_bytes
        ):
            matching_allocations.append(instruction_index)
    if len(matching_allocations) != 1:
        return False

    allocation_index = matching_allocations[0]
    return not any(
        (match := _cc_catalog.STACK_CLEANUP_RE.fullmatch(
            instruction.raw_text.strip().lower()
        ))
        is not None
        and _parse_unsigned_assembly_integer(match.group(1))
        == cleanup_bytes
        for instruction in instructions[allocation_index + 1 : call_index]
    )


def _cleanup_after(instructions: Sequence[Instruction], index: int) -> int | None:
    """Attribute the first straight-line caller cleanup to one call.

    VC5 may schedule independent register work between a cdecl call and its
    ``add esp, N``. Stop before any later invocation, control-flow transfer, or
    competing stack mutation so cleanup cannot be stolen across semantic
    boundaries.
    """
    for instruction_index, instruction in enumerate(
        instructions[index + 1 :], start=index + 1
    ):
        text = instruction.raw_text.strip().lower()
        match = _cc_catalog.STACK_CLEANUP_RE.fullmatch(text)
        if match:
            cleanup_bytes = _parse_unsigned_assembly_integer(match.group(1))
            if _is_terminal_local_frame_teardown(
                instructions,
                call_index=index,
                cleanup_index=instruction_index,
                cleanup_bytes=cleanup_bytes,
            ):
                return None
            return cleanup_bytes
        mnemonic = _instruction_mnemonic(instruction)
        if (
            mnemonic == "call"
            or mnemonic.startswith("j")
            or mnemonic in {"ret", "retn", "iret", "loop", "loope", "loopne", "jecxz"}
        ):
            return None
        if mnemonic in {
            "push",
            "pop",
            "pusha",
            "pushad",
            "popa",
            "popad",
            "enter",
            "leave",
        }:
            return None
        operands = [
            item.strip().lower()
            for item in _instruction_operand(instruction).split(",")
        ]
        if operands and (
            operands[0] == "esp"
            or (mnemonic == "xchg" and "esp" in operands)
        ):
            return None
    return None


def _guarded_switch_entry_count(
    instructions: Sequence[Instruction],
    dispatch_index: int,
) -> int | None:
    operand = _instruction_operand(instructions[dispatch_index]).lower()
    indexed = re.search(
        r"\[\s*[^\]]*\b(?P<index>e?[abcd]x|e?[sd]i|e?[sb]p)\s*\*\s*4\b[^\]]*\]",
        operand,
    )
    if indexed is None:
        return None
    index_register = indexed.group("index")
    for compare_index in range(dispatch_index - 1, max(-1, dispatch_index - 9), -1):
        compare = re.fullmatch(
            rf"cmp\s+{re.escape(index_register)}\s*,\s*(0x[0-9a-f]+|\d+)",
            instructions[compare_index].raw_text.strip().lower(),
        )
        if compare is None:
            continue
        branches = {
            _instruction_mnemonic(item)
            for item in instructions[compare_index + 1 : dispatch_index]
        }
        maximum = _parse_unsigned_assembly_integer(compare.group(1))
        if "ja" in branches:
            return maximum + 1
        if "jae" in branches:
            return maximum
        return None
    return None


def _hexdump_bytes(text: str) -> bytes:
    result = bytearray()
    for line in text.splitlines():
        parts = line.strip().split()
        if (
            not parts
            or re.fullmatch(
                r"[0-9a-fA-F]{5,8}", parts[0].rstrip(":")
            ) is None
        ):
            continue
        for token in parts[1:]:
            if re.fullmatch(r"[0-9a-fA-F]{2}", token) is None:
                break
            result.append(int(token, 16))
    return bytes(result)


def _classifier_switch_entry_count(
    instructions: Sequence[Instruction],
    dispatch_index: int,
    *,
    caller_start: int,
    caller_end: int,
    bridge: BinaryNinjaBridge,
) -> int | None:
    """Prove the bounded VC5 byte-classifier indexed-JMP shape.

    The established shape is ``cmp base,31`` with a 32-byte classifier and
    five-entry table.  zInterp additionally has one exact ``cmp eax,0x2c`` /
    ECX classifier at 0x4c543c with an adjacent 20-entry table at 0x4c53ec.
    zVideo_dd::ReportError has one finite ``cmp eax,0x3c`` / EDX classifier at
    0x4ae274 with an adjacent 34-entry table at 0x4ae1ec.  In all cases the
    unsigned guard supplies the classifier length and every classifier value
    selects a table entry.  The two finite extensions also prove exact bytes,
    complete xrefs, and unique caller-local table/default targets.  This
    recognizes CFG only; it publishes no invocation, target identity, or
    storage fact.
    """
    if dispatch_index < 4:
        return None
    dispatch = instructions[dispatch_index]
    operand = _instruction_operand(dispatch).strip().lower()
    match = re.fullmatch(
        r"(?:dword(?:ptr)?)?\["
        r"(?P<index>e(?:ax|cx|dx|bx|bp|si|di))\*4\+"
        r"(?P<table>0x[0-9a-f]+)\]",
        re.sub(r"\s+", "", operand),
    )
    if match is None:
        return None
    index_register = match.group("index")
    register_code = {
        "eax": 0, "ecx": 1, "edx": 2, "ebx": 3,
        "ebp": 5, "esi": 6, "edi": 7,
    }[index_register]
    try:
        dispatch_body = bytes(int(item, 16) for item in dispatch.bytes)
    except (TypeError, ValueError):
        return None
    expected_sib = (2 << 6) | (register_code << 3) | 5
    table_address = address_value(match.group("table"))
    if (
        _instruction_mnemonic(dispatch) != "jmp"
        or len(dispatch_body) != 7
        or dispatch_body[:2] != b"\xff\x24"
        or dispatch_body[2] != expected_sib
        or struct.unpack_from("<I", dispatch_body, 3)[0] != table_address
    ):
        return None

    load = instructions[dispatch_index - 1]
    zero = instructions[dispatch_index - 2]
    guard_branch = instructions[dispatch_index - 3]
    guard_compare = instructions[dispatch_index - 4]
    low_register = {
        "eax": "al", "ecx": "cl", "edx": "dl", "ebx": "bl",
    }.get(index_register)
    if low_register is None or not _cc_receiver_cursor._is_exact_self_xor_zero(
        zero, destination=index_register
    ):
        return None
    try:
        load_body = bytes(int(item, 16) for item in load.bytes)
    except (TypeError, ValueError):
        return None
    load_operands = _instruction_operand(load).split(",", 1)
    if (
        _instruction_mnemonic(load) != "mov"
        or len(load_body) != 6
        or load_body[0] != 0x8A
        or load_body[1] >> 6 != 2
        or ((load_body[1] >> 3) & 7) != register_code
        or (load_body[1] & 7) == 4
        or len(load_operands) != 2
        or load_operands[0].strip().lower() != low_register
    ):
        return None
    classifier_address = struct.unpack_from("<I", load_body, 2)[0]
    base_code = load_body[1] & 7
    base_register = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )[base_code]
    try:
        compare_body = bytes(int(item, 16) for item in guard_compare.bytes)
    except (TypeError, ValueError):
        return None
    compare_operands = [
        item.strip().lower()
        for item in _instruction_operand(guard_compare).split(",")
    ]
    try:
        guard_maximum = _parse_unsigned_assembly_integer(compare_operands[1])
    except (IndexError, ValueError):
        return None
    classifier_length = guard_maximum + 1
    compare_encoding = compare_body == bytes(
        (0x83, 0xF8 | base_code, guard_maximum)
    )
    runtime_addresses = _instruction_runtime_addresses(
        instructions,
        source="bn",
        caller_start=caller_start,
    )
    address_counts: dict[int, int] = {}
    for runtime_address in runtime_addresses:
        if runtime_address is not None:
            address_counts[runtime_address] = (
                address_counts.get(runtime_address, 0) + 1
            )
    index_by_address = {
        runtime_address: index
        for index, runtime_address in enumerate(runtime_addresses)
        if runtime_address is not None
        and address_counts.get(runtime_address) == 1
    }
    exact_branch = _exact_local_direct_branch(
        guard_branch,
        instruction_index=dispatch_index - 3,
        instruction_addresses=runtime_addresses,
        instruction_index_by_address=index_by_address,
        source="bn",
        caller_start=caller_start,
        caller_end=caller_end,
    )
    rendered_addresses = _cc_catalog.ADDRESS_RE.findall(load_operands[1])
    if (
        _instruction_mnemonic(guard_compare) != "cmp"
        or len(compare_operands) != 2
        or compare_operands[0] != base_register
        or classifier_length not in {32, 45, 61}
        or not compare_encoding
        or _instruction_mnemonic(guard_branch) != "ja"
        or exact_branch is None
        or exact_branch[0] != "conditional"
        or exact_branch[1] <= dispatch_index
        or len(rendered_addresses) != 1
        or address_value(rendered_addresses[0]) != classifier_address
        or not caller_start <= table_address < classifier_address
        or classifier_address + classifier_length > caller_end
    ):
        return None
    exact_zinterp_shape = (
        classifier_length == 45
        and caller_start == address_value("0x4c20a0")
        and caller_end == address_value("0x4c5480")
        and base_register == "eax"
        and index_register == "ecx"
        and table_address == address_value("0x4c53ec")
        and classifier_address == address_value("0x4c543c")
    )
    exact_zvid_reporterror_shape = (
        classifier_length == 61
        and caller_start == address_value("0x4ad6a0")
        and caller_end == address_value("0x4ae380")
        and tuple(
            runtime_addresses[dispatch_index - offset]
            for offset in (4, 3, 2, 1, 0)
        )
        == (0x4ADC9C, 0x4ADC9F, 0x4ADCA5, 0x4ADCA7, 0x4ADCAD)
        and base_register == "eax"
        and index_register == "edx"
        and table_address == address_value("0x4ae1ec")
        and classifier_address == address_value("0x4ae274")
        and exact_branch[1] == index_by_address.get(0x4AE156)
    )
    if (
        classifier_length == 45 and not exact_zinterp_shape
        or classifier_length == 61 and not exact_zvid_reporterror_shape
    ):
        return None
    classifier = _hexdump_bytes(
        bridge.hexdump(
            normalize_address(classifier_address),
            classifier_length,
        )
    )
    if len(classifier) != classifier_length:
        return None
    exact_zvid_classifier = (
        bytes(range(0x12))
        + b"\x21"
        + bytes(range(0x12, 0x20))
        + b"\x21" * 27
        + b"\x20"
    )
    entry_count = max(classifier, default=0) + 1
    if (
        entry_count
        != (
            20
            if exact_zinterp_shape
            else 34 if exact_zvid_reporterror_shape else 5
        )
        or set(classifier) != set(range(entry_count))
        or table_address + entry_count * 4 != classifier_address
        or exact_zvid_reporterror_shape
        and classifier != exact_zvid_classifier
    ):
        return None
    if exact_zinterp_shape or exact_zvid_reporterror_shape:
        raw_table = _hexdump_bytes(
            bridge.hexdump(normalize_address(table_address), entry_count * 4)
        )
        if len(raw_table) != entry_count * 4:
            return None
        table_targets = struct.unpack(f"<{entry_count}I", raw_table)
        table_target_indices = tuple(
            index_by_address.get(target, -1) for target in table_targets
        )
        default_index = exact_branch[1]
        exact_zvid_targets = (
            0x4ADE5D, 0x4ADD36, 0x4ADCF5, 0x4ADD95, 0x4ADE26,
            0x4ADDA4, 0x4ADE35, 0x4ADCDC, 0x4ADD6D, 0x4ADDAE,
            0x4ADD1D, 0x4ADD0E, 0x4ADDCC, 0x4ADDD6, 0x4ADCB4,
            0x4ADD5E, 0x4ADD86, 0x4ADE1C, 0x4ADD45, 0x4ADDE5,
            0x4ADD04, 0x4ADDF4, 0x4ADCCD, 0x4ADDFE, 0x4ADCBE,
            0x4ADD7C, 0x4ADE4E, 0x4ADD2C, 0x4ADE0D, 0x4ADE44,
            0x4ADDBD, 0x4ADD54, 0x4ADCE6, 0x4AE156,
        )
        if (
            len(set(table_targets)) != entry_count
            or any(
                target_index <= dispatch_index
                for target_index in table_target_indices
            )
            or default_index <= dispatch_index
            or exact_zinterp_shape and default_index in table_target_indices
            or exact_zvid_reporterror_shape
            and table_targets != exact_zvid_targets
        ):
            return None
        if exact_zvid_reporterror_shape:
            get_json = getattr(bridge, "get_json", None)
            if not callable(get_json):
                return None
            try:
                classifier_xrefs = get_json(
                    "getXrefsTo", address="0x4ae274", limit=100000
                )
                table_xrefs = get_json(
                    "getXrefsTo", address="0x4ae1ec", limit=100000
                )
            except (BridgeError, OSError, RuntimeError, ValueError):
                return None
            classifier_code, classifier_data, classifier_complete = (
                _cc_identity._bn_inbound_xref_items(classifier_xrefs)
            )
            table_code, table_data, table_complete = _cc_identity._bn_inbound_xref_items(
                table_xrefs
            )
            classifier_sources = Counter(
                _cc_identity._bn_xref_address(
                    row,
                    "source_address",
                    "source_addr",
                    "from_address",
                    "address",
                )
                for row in classifier_code
            )
            table_sources = Counter(
                _cc_identity._bn_xref_address(
                    row,
                    "source_address",
                    "source_addr",
                    "from_address",
                    "address",
                )
                for row in table_code
            )
            if (
                not classifier_complete
                or not table_complete
                or classifier_data
                or table_data
                or classifier_sources
                != {"0x4adca7": 1, "0x4adcad": 1}
                or table_sources != {"0x4adcad": 1}
            ):
                return None
    return entry_count


def _complete_classifier_switch_targets(
    instructions: Sequence[Instruction], index: int, *, addresses: Sequence[int | None],
    by_address: Mapping[int, int], caller_start: int, caller_end: int, bridge: BinaryNinjaBridge,
) -> tuple[int, ...]:
    """Recover a bounded byte-classifier table for a complete CFG consumer."""
    if index < 4:
        return ()
    try:
        compare, branch, clear, load, jump = (
            _cc_instructions.instruction_fact(row) for row in instructions[index - 4:index + 1])
        if (compare.mnemonic != "cmp" or branch.mnemonic != "ja" or clear.mnemonic != "xor"
                or load.mnemonic != "mov" or jump.mnemonic != "jmp"
                or tuple(len(row.operands) for row in (compare, clear, load, jump)) != (2, 2, 2, 1)):
            return ()
        base, bound = compare.operands
        zero, same = clear.operands
        byte, classifier = load.operands
        table = jump.operands[0]
        if (base.kind != "register" or base.size != 4 or bound.kind != "immediate" or not 0 <= bound.immediate < 256
                or zero.kind != "register" or zero.size != 4
                or (same.kind, same.size, same.register) != ("register", 4, zero.register)
                or byte.kind != "register" or _cc_instructions.REGISTER_BITS.get(byte.register) != (zero.register, 0, 8)
                or classifier.kind != "memory" or classifier.size != 1 or classifier.segment
                or classifier.base != base.register or classifier.index
                or table.kind != "memory" or table.size != 4 or table.segment or table.base
                or table.index != zero.register or table.scale != 4):
            return ()
        table_address, classifier_address = table.displacement & 0xffffffff, classifier.displacement & 0xffffffff
        if any(addresses[at] is None or addresses[at] + len(instructions[at].bytes) != addresses[at + 1]
               for at in range(index - 4, index)):
            return ()
        if not caller_start <= table_address < classifier_address or classifier_address + bound.immediate + 1 > caller_end:
            return ()
        guard = _exact_local_direct_branch(instructions[index - 3], instruction_index=index - 3,
            instruction_addresses=addresses, instruction_index_by_address=by_address, source="bn",
            caller_start=caller_start, caller_end=caller_end)
        if guard is None or guard[0] != "conditional" or index - 3 <= guard[1] <= index:
            return ()
        for source_index, row in enumerate(instructions):
            edge = _exact_local_direct_branch(row, instruction_index=source_index,
                instruction_addresses=addresses, instruction_index_by_address=by_address, source="bn",
                caller_start=caller_start, caller_end=caller_end)
            if edge is not None and index - 3 <= edge[1] <= index:
                return ()  # An incoming branch bypasses the bound check.
        data = _hexdump_bytes(bridge.hexdump(normalize_address(classifier_address), bound.immediate + 1))
        if len(data) != bound.immediate + 1:
            return ()
        count = max(data) + 1
        if set(data) != set(range(count)) or table_address + count * 4 != classifier_address:
            return ()
        raw = _hexdump_bytes(bridge.hexdump(normalize_address(table_address), count * 4))
        if len(raw) != count * 4:
            return ()
        targets = tuple(by_address.get(address, -1) for address in struct.unpack(f"<{count}I", raw))
        if any(target < 0 or index - 3 <= target <= index for target in targets):
            return ()
        return targets
    except (ValueError, IndexError):
        return ()


def retail_local_switch_targets(
    instructions: Sequence[Instruction],
    *,
    caller_start: str,
    caller_end_exclusive: str,
    bridge: BinaryNinjaBridge,
    include_backward_targets: bool = False,
) -> dict[int, tuple[int, ...]]:
    """Return exact caller-local BN jump-table edges.

    A table participates only when its guarded cardinality is exact, every
    immutable table entry names one unique instruction start in the caller,
    and each target is forward of the dispatch unless a complete CFG consumer
    explicitly requests backward edges. Single-pass consumers retain the
    forward-only restriction.
    """
    start = address_value(caller_start)
    end = address_value(caller_end_exclusive)
    instruction_addresses = _instruction_runtime_addresses(
        instructions,
        source="bn",
        caller_start=start,
    )
    address_counts: dict[int, int] = {}
    for address in instruction_addresses:
        if address is not None and start <= address < end:
            address_counts[address] = address_counts.get(address, 0) + 1
    instruction_index_by_address = {
        address: index
        for index, address in enumerate(instruction_addresses)
        if (
            address is not None
            and start <= address < end
            and address_counts.get(address) == 1
        )
    }
    local: dict[int, tuple[int, ...]] = {}
    for index, instruction in enumerate(instructions):
        if _instruction_mnemonic(instruction) != "jmp" or "[" not in _instruction_operand(
            instruction
        ):
            continue
        count = _guarded_switch_entry_count(instructions, index)
        if count is None:
            count = _classifier_switch_entry_count(
                instructions,
                index,
                caller_start=start,
                caller_end=end,
                bridge=bridge,
            )
        if count is None or count < 1:
            if include_backward_targets:
                targets = _complete_classifier_switch_targets(instructions, index,
                    addresses=instruction_addresses, by_address=instruction_index_by_address,
                    caller_start=start, caller_end=end, bridge=bridge)
                if targets:
                    local[index] = targets
            continue
        addresses = _cc_catalog.ADDRESS_RE.findall(_instruction_operand(instruction))
        if len(addresses) != 1:
            continue
        table_address = normalize_address(addresses[0])
        raw = _hexdump_bytes(bridge.hexdump(table_address, count * 4))
        if len(raw) != count * 4:
            continue
        targets = struct.unpack(f"<{count}I", raw)
        target_indices = tuple(
            instruction_index_by_address.get(target, -1) for target in targets
        )
        if all(target_index >= 0 if include_backward_targets else target_index > index
               for target_index in target_indices):
            local[index] = target_indices
    return local




def _exact_invocation_cfg(
    instructions: Sequence[Instruction],
    *,
    instruction_addresses: Sequence[int | None],
    instruction_index_by_address: Mapping[int, int],
    source: str,
    caller_start: int,
    caller_end: int,
    local_control_flow_indices: frozenset[int],
    local_control_flow_targets: Mapping[int, tuple[int, ...]],
) -> tuple[dict[int, tuple[int, ...]], frozenset[int]]:
    """Build the exact local CFG used by loop-sensitive provenance proofs."""
    successors: dict[int, tuple[int, ...]] = {}
    unresolved: set[int] = set()
    conditional_mnemonics = {
        "ja", "jae", "jb", "jbe", "jc", "je", "jg", "jge", "jl", "jle",
        "jna", "jnae", "jnb", "jnbe", "jnc", "jne", "jng", "jnge", "jnl",
        "jnle", "jno", "jnp", "jns", "jnz", "jo", "jp", "jpe", "jpo",
        "js", "jz", "jecxz", "loop", "loope", "loopne", "loopnz", "loopz",
    }
    for index, instruction in enumerate(instructions):
        mnemonic = _instruction_mnemonic(instruction)
        fallthrough = (index + 1,) if index + 1 < len(instructions) else ()
        if source == "cod" and mnemonic in {"db", "dw", "dd", "dq", "dt"}:
            # A listing data directive cannot be propagated as a harmless
            # instruction. Keep the conservative fallthrough edge so a path
            # reaching the queried call through data remains unresolved.
            successors[index] = fallthrough
            unresolved.add(index)
            continue
        if _exact_return_terminates(instruction):
            successors[index] = ()
            continue
        if mnemonic in {"ret", "retn"}:
            successors[index] = ()
            unresolved.add(index)
            continue
        if index in local_control_flow_indices:
            targets = local_control_flow_targets.get(index, ())
            if (
                mnemonic != "jmp"
                or not targets
                or any(
                    target < 0 or target >= len(instructions)
                    for target in targets
                )
            ):
                successors[index] = ()
                unresolved.add(index)
            else:
                successors[index] = tuple(dict.fromkeys(targets))
            continue
        branch = _exact_local_direct_branch(
            instruction,
            instruction_index=index,
            instruction_addresses=instruction_addresses,
            instruction_index_by_address=instruction_index_by_address,
            source=source,
            caller_start=caller_start,
            caller_end=caller_end,
        )
        if branch is not None:
            kind, target = branch
            successors[index] = (
                tuple(dict.fromkeys((*fallthrough, target)))
                if kind == "conditional"
                else (target,)
            )
            continue
        if mnemonic in conditional_mnemonics:
            successors[index] = fallthrough
            unresolved.add(index)
            continue
        if mnemonic == "jmp":
            # A direct or indirect non-local JMP is a terminal tail call.  A
            # rendered caller-local target or COD local label that failed the
            # exact decoder is unresolved rather than a trustworthy exit.
            rendered_target = _rendered_numeric_branch_target(
                instruction,
                source=source,
                caller_start=caller_start,
            )
            local_label = re.search(
                r"\$L[0-9A-Za-z_]+",
                _instruction_operand(instruction),
                flags=re.IGNORECASE,
            )
            successors[index] = ()
            if (
                local_label is not None
                or (
                    rendered_target is not None
                    and caller_start <= rendered_target < caller_end
                )
            ):
                unresolved.add(index)
            continue
        successors[index] = fallthrough
    return successors, frozenset(unresolved)


def _exact_register_definition_set_covers_transfer(
    instructions: Sequence[Instruction],
    *,
    instruction_addresses: Sequence[int | None],
    instruction_index_by_address: Mapping[int, int],
    definition_indices: frozenset[int],
    transfer_index: int,
    register: str,
    source: str,
    caller_start: int,
    caller_end: int,
    local_control_flow_indices: frozenset[int],
    local_control_flow_targets: Mapping[int, tuple[int, ...]],
) -> bool:
    """Require every exact CFG path to carry one live reviewed definition."""

    if (
        not definition_indices
        or transfer_index < 0
        or transfer_index >= len(instructions)
        or register not in {
            "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"
        }
    ):
        return False
    successors, unresolved = _exact_invocation_cfg(
        instructions,
        instruction_addresses=instruction_addresses,
        instruction_index_by_address=instruction_index_by_address,
        source=source,
        caller_start=caller_start,
        caller_end=caller_end,
        local_control_flow_indices=local_control_flow_indices,
        local_control_flow_targets=local_control_flow_targets,
    )
    work: list[tuple[int, bool]] = [(0, False)] if instructions else []
    visited: set[tuple[int, bool]] = set()
    reached_states: set[bool] = set()
    while work:
        index, live = work.pop()
        state = (index, live)
        if state in visited:
            continue
        visited.add(state)
        if index == transfer_index:
            # Record every arrival, then continue through the transfer.  A
            # loop may revisit this same CALL/JMP after a later kill; stopping
            # at the first arrival would incorrectly prove only iteration one.
            reached_states.add(live)
        if index in unresolved:
            return False
        instruction = instructions[index]
        next_live = live
        if index in definition_indices:
            next_live = True
        elif (
            _instruction_mnemonic(instruction) == "call"
            and register in {"eax", "ecx", "edx"}
        ) or _instruction_may_clobber_register(instruction, register):
            next_live = False
        for successor in successors.get(index, ()):
            work.append((successor, next_live))
    return reached_states == {True}


def _reachable_cfg_indices(
    successors: Mapping[int, tuple[int, ...]],
    starts: Iterable[int],
    *,
    blocked: frozenset[int] = frozenset(),
) -> frozenset[int]:
    pending = [index for index in starts if index not in blocked]
    reached: set[int] = set()
    while pending:
        index = pending.pop()
        if index in reached or index in blocked:
            continue
        reached.add(index)
        pending.extend(
            target
            for target in successors.get(index, ())
            if target not in reached and target not in blocked
        )
    return frozenset(reached)


def _indices_reaching_cfg_target(
    successors: Mapping[int, tuple[int, ...]],
    target: int,
) -> frozenset[int]:
    predecessors: dict[int, list[int]] = {}
    for source_index, targets in successors.items():
        for target_index in targets:
            predecessors.setdefault(target_index, []).append(source_index)
    return _reachable_cfg_indices(
        {index: tuple(rows) for index, rows in predecessors.items()},
        (target,),
    )


def _loop_invariant_exact_iat_markers(
    instructions: Sequence[Instruction],
    *,
    instruction_addresses: Sequence[int | None],
    instruction_index_by_address: Mapping[int, int],
    source: str,
    caller_start: int,
    caller_end: int,
    local_control_flow_indices: frozenset[int],
    local_control_flow_targets: Mapping[int, tuple[int, ...]],
) -> frozenset[tuple[int, str, str]]:
    """Prove exact preheader IAT definitions invariant across backedges.

    The returned key is ``(backedge instruction, register, exact marker)``.
    Only x86 nonvolatile registers qualify.  The unique marker definition must
    dominate both the loop header and backedge, and every path from the
    definition through the cycle to that backedge must preserve the register.
    Requiring the definition, rather than the header, to dominate the backedge
    admits VC5's peeled-loop shape: the first iteration may enter after the
    later-iteration header while retaining the same exact definition.  Any
    entry that bypasses the definition, unresolved local edge, alternate
    reaching definition, or clobber still fails closed.
    """
    successors, unresolved = _exact_invocation_cfg(
        instructions,
        instruction_addresses=instruction_addresses,
        instruction_index_by_address=instruction_index_by_address,
        source=source,
        caller_start=caller_start,
        caller_end=caller_end,
        local_control_flow_indices=local_control_flow_indices,
        local_control_flow_targets=local_control_flow_targets,
    )
    if not instructions:
        return frozenset()
    entry_reachable = _reachable_cfg_indices(successors, (0,))
    address_keys: dict[str, list[int]] = {}
    for index, address in enumerate(instruction_addresses):
        if address is None:
            continue
        key = normalize_address(address - caller_start if source == "cod" else address)
        address_keys.setdefault(key, []).append(index)

    register_names = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    definition_sites: list[tuple[str, int, str]] = []
    for definition_key, definition_indices in address_keys.items():
        if len(definition_indices) != 1:
            continue
        definition_index = definition_indices[0]
        try:
            body = bytes(
                int(item, 16)
                for item in instructions[definition_index].bytes
            )
        except (TypeError, ValueError):
            continue
        if (
            len(body) != 6
            or body[0] != 0x8B
            or body[1] >> 6 != 0
            or body[1] & 0x07 != 0x05
        ):
            continue
        register = register_names[(body[1] >> 3) & 0x07]
        if register in {"ebx", "ebp", "esi", "edi"}:
            definition_sites.append(
                (register, definition_index, definition_key)
            )

    safe: set[tuple[int, str, str]] = set()
    for source_index, targets in successors.items():
        for header_index in targets:
            if header_index > source_index:
                continue
            if source_index not in entry_reachable or header_index not in entry_reachable:
                continue
            reaching_source = _indices_reaching_cfg_target(successors, source_index)
            cycle_nodes = (
                _reachable_cfg_indices(successors, (header_index,))
                & reaching_source
            )
            # The marker at the linear backedge state is checked by the
            # caller.  Retain only exact absolute MOV definitions that can
            # uniquely dominate this loop.
            for register, definition_index, definition_key in definition_sites:
                if not definition_index < header_index:
                    continue
                if header_index in _reachable_cfg_indices(
                    successors,
                    (0,),
                    blocked=frozenset({definition_index}),
                ):
                    continue
                if source_index in _reachable_cfg_indices(
                    successors,
                    (0,),
                    blocked=frozenset({definition_index}),
                ):
                    continue
                marker_prefix = f"exact-iat-load({register},{definition_key},iat:"
                definition_to_source = (
                    _reachable_cfg_indices(
                        successors,
                        successors.get(definition_index, ()),
                    )
                    & reaching_source
                )
                if source_index not in definition_to_source:
                    continue
                proof_nodes = definition_to_source | cycle_nodes
                if unresolved & proof_nodes:
                    continue
                if any(
                    _instruction_may_clobber_register(
                        instructions[index], register
                    )
                    for index in proof_nodes
                    if index != definition_index
                ):
                    continue
                # The identity suffix is intentionally matched only when the
                # live marker reaches the backedge.  Exact IAT identity
                # remains owned by the marker producer and the indirect-call
                # canonicalizer.
                safe.add((source_index, register, marker_prefix))
    return frozenset(safe)


def _cod_cfg_successors(
    instructions: Sequence[Instruction],
    *,
    label_instruction_indices: Mapping[str, int],
    proven_switch_targets: Mapping[int, tuple[int, ...]],
) -> tuple[dict[int, tuple[int, ...]], frozenset[int]]:
    successors: dict[int, tuple[int, ...]] = {}
    unresolved: set[int] = set()
    conditional_branches = {
        "ja", "jae", "jb", "jbe", "jc", "je", "jg", "jge", "jl", "jle",
        "jna", "jnae", "jnb", "jnbe", "jnc", "jne", "jng", "jnge", "jnl",
        "jnle", "jno", "jnp", "jns", "jnz", "jo", "jp", "jpe", "jpo",
        "js", "jz", "jecxz", "loop", "loope", "loopne", "loopnz", "loopz",
    }
    for index, instruction in enumerate(instructions):
        mnemonic = _instruction_mnemonic(instruction)
        fallthrough = (index + 1,) if index + 1 < len(instructions) else ()
        if _exact_return_terminates(instruction):
            successors[index] = ()
            continue
        if mnemonic == "jmp":
            if index in proven_switch_targets:
                successors[index] = tuple(proven_switch_targets[index])
                continue
            target = _cc_listing._cod_direct_label_target(
                instruction,
                label_instruction_indices=label_instruction_indices,
            )
            if target is not None:
                successors[index] = (target,)
            else:
                successors[index] = ()
                unresolved.add(index)
            continue
        if mnemonic in conditional_branches:
            target = _cc_listing._cod_direct_label_target(
                instruction,
                label_instruction_indices=label_instruction_indices,
            )
            if target is None:
                successors[index] = fallthrough
                unresolved.add(index)
            else:
                successors[index] = tuple(dict.fromkeys((*fallthrough, target)))
            continue
        successors[index] = fallthrough
    return successors, frozenset(unresolved)


def _cod_reachable_indices(
    successors: Mapping[int, tuple[int, ...]],
    starts: Iterable[int],
    *,
    blocked: frozenset[int] = frozenset(),
) -> frozenset[int]:
    pending = [item for item in starts if item not in blocked]
    reached: set[int] = set()
    while pending:
        index = pending.pop()
        if index in reached or index in blocked:
            continue
        reached.add(index)
        pending.extend(
            target
            for target in successors.get(index, ())
            if target not in reached and target not in blocked
        )
    return frozenset(reached)


def _cod_indices_reaching_target(
    successors: Mapping[int, tuple[int, ...]],
    target: int,
) -> frozenset[int]:
    predecessors: dict[int, set[int]] = {}
    for source, targets in successors.items():
        for destination in targets:
            predecessors.setdefault(destination, set()).add(source)
    return _cod_reachable_indices(
        {index: tuple(rows) for index, rows in predecessors.items()},
        (target,),
    )


def _instruction_may_clobber_register(instruction: Instruction, register: str) -> bool:
    """Use the common byte-decoded explicit/implicit/partial register effects."""
    return _cc_instructions.may_clobber_register(instruction, register)


def _cod_guarded_switch_entry_count(
    instructions: Sequence[Instruction],
    dispatch_index: int,
    *,
    table_entry_count: int,
    index_register: str,
    label_instruction_indices: Mapping[str, int],
    proven_switch_targets: Mapping[int, tuple[int, ...]],
) -> int | None:
    """Prove a candidate-local switch guard through exact forward CFG facts."""
    successors, unresolved = _cod_cfg_successors(
        instructions,
        label_instruction_indices=label_instruction_indices,
        proven_switch_targets=proven_switch_targets,
    )
    entry_reachable = _cod_reachable_indices(successors, (0,))
    if dispatch_index not in entry_reachable:
        return None
    candidates: list[int] = []
    for compare_index in range(dispatch_index):
        compare = re.fullmatch(
            rf"cmp\s+{re.escape(index_register)}\s*,\s*(0x[0-9a-f]+|\d+)",
            instructions[compare_index].raw_text.strip().lower(),
        )
        if compare is None or compare_index + 1 >= dispatch_index:
            continue
        branch_index = compare_index + 1
        while (
            branch_index < dispatch_index
            and _instruction_mnemonic(instructions[branch_index])
            not in {"ja", "jae"}
        ):
            intervening = instructions[branch_index]
            if (
                _instruction_mnemonic(intervening)
                not in {"lea", "mov", "nop", "npad", "pop", "push", "xchg"}
                or _instruction_may_clobber_register(
                    intervening,
                    index_register,
                )
            ):
                branch_index = dispatch_index
                break
            branch_index += 1
        if branch_index >= dispatch_index:
            continue
        branch = instructions[branch_index]
        branch_mnemonic = _instruction_mnemonic(branch)
        raw = tuple(item.lower() for item in branch.bytes)
        if branch_mnemonic == "ja":
            branch_encoding_ok = (
                (len(raw) == 2 and raw[0] == "77")
                or (len(raw) == 6 and raw[0:2] == ("0f", "87"))
            )
            count = _parse_unsigned_assembly_integer(compare.group(1)) + 1
        elif branch_mnemonic == "jae":
            branch_encoding_ok = (
                (len(raw) == 2 and raw[0] == "73")
                or (len(raw) == 6 and raw[0:2] == ("0f", "83"))
            )
            count = _parse_unsigned_assembly_integer(compare.group(1))
        else:
            continue
        if not branch_encoding_ok or count != table_entry_count:
            continue
        taken_target = _cc_listing._cod_direct_label_target(
            branch,
            label_instruction_indices=label_instruction_indices,
        )
        fallthrough = branch_index + 1
        if (
            taken_target is None
            or fallthrough >= len(instructions)
            or dispatch_index not in _cod_reachable_indices(successors, (fallthrough,))
            or dispatch_index in _cod_reachable_indices(successors, (taken_target,))
            or dispatch_index
            in _cod_reachable_indices(
                successors,
                (0,),
                blocked=frozenset({compare_index}),
            )
        ):
            continue
        reaching_dispatch = _cod_indices_reaching_target(successors, dispatch_index)
        relevant = (
            _cod_reachable_indices(successors, (fallthrough,))
            & reaching_dispatch
        ) - {dispatch_index}
        if any(index in unresolved for index in relevant):
            continue
        if any(
            _instruction_may_clobber_register(instructions[index], index_register)
            for index in relevant
            if index > branch_index
        ):
            continue
        candidates.append(compare_index)
    return table_entry_count if len(candidates) == 1 else None


def _cod_local_switch_targets(
    lines: Sequence[str],
    instructions: Sequence[Instruction],
    *,
    allow_named_guard_labels: bool = False,
) -> dict[int, tuple[int, ...]]:
    label_lines: dict[str, int] = {}
    label_counts: dict[str, int] = {}
    label_offsets: dict[str, int] = {}
    pending_labels: list[str] = []
    for line_index, line in enumerate(lines):
        label = re.match(
            (
                r"^\s*(\$[A-Za-z_][0-9A-Za-z_$]*):"
                if allow_named_guard_labels
                else r"^\s*(\$L[0-9A-Za-z_]+):"
            ),
            line,
        )
        if label:
            pending_labels.append(label.group(1))
            label_lines[label.group(1)] = line_index
            label_counts[label.group(1)] = label_counts.get(label.group(1), 0) + 1
            continue
        offset = re.match(r"^\s*([0-9a-fA-F]{5,8})\s+", line)
        if offset and pending_labels:
            value = int(offset.group(1), 16)
            for name in pending_labels:
                label_offsets[name] = value
            pending_labels.clear()

    # Use listing records rather than parsed instruction.source_line here:
    # VC5 wraps long byte sequences and places the mnemonic on a continuation
    # line, so the parsed instruction may no longer carry its leading offset.
    # A DD table row is explicitly data; every other offset-bearing row inside
    # this PROC is a machine-instruction record (possibly continued next line).
    code_offsets = {
        int(match.group(1), 16)
        for line in lines
        if (match := re.match(r"^\s*([0-9a-fA-F]{5,8})\s+", line))
        and re.search(r"\bDD\b", line, re.IGNORECASE) is None
    }

    instruction_offsets = _instruction_runtime_addresses(
        instructions,
        source="cod",
        caller_start=0,
    )
    offset_counts: dict[int, int] = {}
    for offset in instruction_offsets:
        if offset is not None:
            offset_counts[offset] = offset_counts.get(offset, 0) + 1
    instruction_index_by_offset = {
        offset: index
        for index, offset in enumerate(instruction_offsets)
        if offset is not None and offset_counts.get(offset) == 1
    }
    label_instruction_indices = {
        label: instruction_index_by_offset[offset]
        for label, offset in label_offsets.items()
        if (
            label_counts.get(label) == 1
            and offset in instruction_index_by_offset
        )
    }

    local: dict[int, tuple[int, ...]] = {}
    for index, instruction in enumerate(instructions):
        switch = _cc_listing._cod_exact_indexed_switch(instruction)
        if switch is None:
            continue
        table_label, index_register = switch
        if label_counts.get(table_label) != 1 or table_label not in label_lines:
            continue
        entries: list[str] = []
        for line in lines[label_lines[table_label] + 1 :]:
            match = re.match(
                r"^\s*[0-9a-fA-F]{5,8}\s+(?:[0-9a-fA-F]{2}\s+)+"
                r"DD\s+(\$L[0-9A-Za-z_]+)\b",
                line,
                re.IGNORECASE,
            )
            if match is None:
                if entries:
                    break
                continue
            entries.append(match.group(1))
        count = _cod_guarded_switch_entry_count(
            instructions,
            index,
            table_entry_count=len(entries),
            index_register=index_register,
            label_instruction_indices=label_instruction_indices,
            proven_switch_targets=local,
        )
        if count is None or count < 1:
            continue
        target_indices = tuple(
            instruction_index_by_offset.get(label_offsets.get(entry, -1), -1)
            for entry in entries
        )
        if (
            len(entries) == count
            and all(label_counts.get(entry) == 1 for entry in entries)
            and all(label_offsets.get(entry) in code_offsets for entry in entries)
            and all(target_index > index for target_index in target_indices)
        ):
            local[index] = target_indices
    return local




def _candidate_exact_coff_switch_targets(
    parsed: CandidateAssembly,
    caller_bytes: CoffFunctionBytes,
    coff_object: CoffObject,
) -> dict[int, tuple[int, ...]]:
    """Decode bounded direct or byte-classifier switches from live COFF.

    This publishes complete CFG edges, including backward cases, only for the
    CMP/JA[/XOR/MOV-byte]/JMP lowering. Exact ESP-relative stores may be
    scheduled between CMP and JA: neither the index nor flags are changed.
    Numeric local label names and body sizes are irrelevant. The guard cannot
    be bypassed by an encoded branch or a local relocation into its interior.
    Preserve physical table order and duplicate destinations for case-map
    consumers; CFG consumers derive their own unique successor view.
    No retail fact is inferred.
    """
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(parsed)
    counts = Counter(offsets)
    data = caller_bytes.data
    by_offset = {value: index for index, value in enumerate(offsets)
                 if value is not None and counts[value] == 1
                 and re.search(r"\b(?:DD|DB)\b", parsed.instructions[index].source_line,
                               re.IGNORECASE) is None}
    mask = caller_bytes.relocation_mask
    relocations = tuple(caller_bytes.relocations)
    symbols = {symbol.index: symbol for symbol in coff_object.symbols}
    if len(mask) != len(data) or len(symbols) != len(coff_object.symbols):
        return {}
    result: dict[int, tuple[int, ...]] = {}

    def local_relocation(at: int, name: str | None = None) -> int | None:
        rows = [row for row in relocations if row.offset == at]
        if (len(rows) != 1 or rows[0].type != IMAGE_REL_I386_DIR32
                or at < 0 or at + 4 > len(data)
                or data[at:at + 4] != b"\0" * 4
                or not all(mask[at:at + 4])):
            return None
        row = rows[0]
        symbol = symbols.get(row.symbol_index)
        if (symbol is None or symbol.name != row.symbol_name
                or name is not None and symbol.name != name
                or symbol.section_number != caller_bytes.section_index):
            return None
        value = symbol.value - caller_bytes.start
        return value if 0 <= value < len(data) else None

    for index, instruction in enumerate(parsed.instructions):
        switch = _cc_listing._cod_exact_indexed_switch(instruction)
        if switch is None or index < 2:
            continue
        table_name, destination = switch
        code = _cc_catalog._COD_SWITCH_INDEX_CODES[destination]
        load_match = re.fullmatch(
            r"mov\s+(al|cl|dl|bl),\s*byte(?: ptr)?\s+(\$L[0-9A-Za-z_]+)\[(e[a-z]{2})\]",
            parsed.instructions[index - 1].raw_text.strip(), re.IGNORECASE,
        )
        classifier_form = load_match is not None
        branch_index = index - (3 if classifier_form else 1)
        compare_index = branch_index - 1
        if compare_index < 0 or classifier_form and code > 3:
            continue
        # Accept only exact flag/index-preserving stack stores, not arbitrary
        # instructions that merely render as MOV or appear harmless in HLIL.
        while compare_index >= 0:
            scheduled = parsed.instructions[compare_index]
            try:
                body = bytes(int(byte, 16) for byte in scheduled.bytes)
            except (TypeError, ValueError):
                break
            stack_store = (
                len(body) == 4 and body[0] == 0x89
                and body[1] & 0xC7 == 0x44 and body[2] == 0x24
                or len(body) == 8 and body[:3] == b"\xc7\x44\x24"
            )
            if not stack_store or _instruction_mnemonic(scheduled) != "mov":
                break
            compare_index -= 1
        if compare_index < 0:
            continue
        rows = parsed.instructions[compare_index:index + 1]
        starts = offsets[compare_index:index + 1]
        try:
            bodies = [bytes(int(byte, 16) for byte in row.bytes) for row in rows]
        except (TypeError, ValueError):
            continue
        if (any(value is None for value in starts)
                or any(starts[n] + len(bodies[n]) != starts[n + 1]
                       for n in range(len(rows) - 1))
                or any(data[start:start + len(body)] != body
                       for start, body in zip(starts, bodies))):
            continue
        compare = bodies[0]
        branch = bodies[branch_index - compare_index]
        jump = bodies[-1]
        source = load_match.group(3).lower() if classifier_form else destination
        source_code = _cc_catalog._COD_SWITCH_INDEX_CODES.get(source, -1)
        if (source_code < 0 or source_code == 4
                or classifier_form and source == destination):
            continue
        if compare[:1] == b"\x3d" and len(compare) == 5 and source_code == 0:
            bound = struct.unpack_from("<I", compare, 1)[0]
        elif compare[:2] == bytes((0x81, 0xF8 | source_code)) and len(compare) == 6:
            bound = struct.unpack_from("<I", compare, 2)[0]
        elif (compare[:2] == bytes((0x83, 0xF8 | source_code))
              and len(compare) == 3 and compare[2] < 0x80):
            bound = compare[2]
        else:
            continue
        if bound > 0xFFFF:
            continue
        if len(branch) == 2 and branch[0] == 0x77:
            default = offsets[branch_index] + 2 + struct.unpack_from("<b", branch, 1)[0]
        elif len(branch) == 6 and branch[:2] == b"\x0f\x87":
            default = offsets[branch_index] + 6 + struct.unpack_from("<i", branch, 2)[0]
        else:
            continue
        if (default not in by_offset
                or jump != bytes((0xFF, 0x24, 0x85 | code << 3, 0, 0, 0, 0))):
            continue
        classifier = None
        values = ()
        allowed = {starts[-1] + 3}
        if classifier_form:
            if (load_match.group(1).lower() != ("al", "cl", "dl", "bl")[code]
                    or bodies[-3] != bytes((0x33, 0xC0 | code << 3 | code))
                    or bodies[-2] != bytes((0x8A, 0x80 | code << 3 | source_code, 0, 0, 0, 0))):
                continue
            classifier = local_relocation(starts[-2] + 2, load_match.group(2))
            if (classifier is None or classifier + bound + 1 > len(data)
                    or any(mask[classifier:classifier + bound + 1])):
                continue
            values = data[classifier:classifier + bound + 1]
            allowed.add(starts[-2] + 2)
        table = local_relocation(starts[-1] + 3, table_name)
        if table is None:
            continue
        count = max(values) + 1 if classifier_form else bound + 1
        targets = [local_relocation(table + 4 * item) for item in range(count)]
        if (any(target not in by_offset for target in targets)
                or classifier_form and set(values) != set(range(count))):
            continue
        # Reject relocation overlap in instructions/classifier, and a classifier
        # or table overlaid with executable instruction records.
        if (any(row.offset not in allowed for row in relocations
                if starts[0] <= row.offset < starts[-1] + len(jump))
                or any(classifier is not None and classifier <= value < classifier + len(values)
                       or table <= value < table + count * 4 for value in by_offset)
                or any(mask[at] for at in range(starts[0], starts[-1] + len(jump))
                       if not any(field <= at < field + 4 for field in allowed))):
            continue
        interior = range(starts[0] + len(compare), starts[-1] + 1)
        bypass = any(target in interior for target in targets)
        for other_index, other in enumerate(parsed.instructions):
            branch_target = _exact_local_direct_branch(
                other, instruction_index=other_index,
                instruction_addresses=offsets, instruction_index_by_address=by_offset,
                source="cod", caller_start=0, caller_end=len(data),
            )
            if branch_target is not None and offsets[branch_target[1]] in interior:
                bypass = True
        for relocation in relocations:
            symbol = symbols.get(relocation.symbol_index)
            if (symbol is not None and symbol.section_number == caller_bytes.section_index
                    and symbol.value - caller_bytes.start in interior):
                bypass = True
        if not bypass:
            result[index] = tuple(by_offset[target] for target in targets)
    return result


def _compose_candidate_coff_switch_maps(
    parsed: CandidateAssembly,
    exact_targets: Mapping[int, tuple[int, ...]],
) -> CandidateAssembly:
    """Keep complete proved case maps separate from one-pass forward flow.

    A mixed/backward table terminates the linear walk as a whole. Its complete
    ordered cases remain available to independent bounded CFG proofs; selecting
    only the forward cases would silently lose predecessors.
    """
    forward = dict(parsed.local_control_flow_targets)
    complete = dict(parsed.classification_only_local_control_flow_targets)
    for index, targets in exact_targets.items():
        if (type(index) is not int or not 0 <= index < len(parsed.instructions)
                or not targets or any(type(target) is not int
                    or not 0 <= target < len(parsed.instructions) for target in targets)):
            raise _cc_errors.CandidateCallContractEvidenceError("exact COFF switch has invalid local targets")
        for prior in (forward.get(index), complete.get(index)):
            if prior is not None and prior != targets:
                raise _cc_errors.CandidateCallContractEvidenceError(
                    "exact COFF switch targets conflict with the parsed local CFG")
        if all(target > index for target in targets):
            forward[index] = targets
        else:
            forward.pop(index, None)
            complete[index] = targets
    return replace(parsed,
        local_control_flow_indices=parsed.local_control_flow_indices | frozenset(exact_targets),
        local_control_flow_targets=forward,
        classification_only_local_control_flow_targets=complete)


def _candidate_exact_local_switch_classification_targets(
    lines: Sequence[str],
    parsed: CandidateAssembly,
    caller_bytes: CoffFunctionBytes,
    coff_object: CoffObject,
) -> dict[int, tuple[int, ...]]:
    """Prove classification-only VC5 local switches, including back targets.

    Complete forward-only target sets remain owned by
    ``_cod_local_switch_targets``.  This narrower view recognizes a guarded
    compiler table whose entries all relocate to unique labels inside the
    same COFF function, but publishes no successor edges when one or more
    cases are backward.  Thus the JMP is control flow, never an invocation,
    while forward data-flow remains conservatively terminated.
    """

    instructions = parsed.instructions
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(parsed)
    offset_counts = Counter(offset for offset in offsets if offset is not None)
    index_by_offset = {
        int(offset): index
        for index, offset in enumerate(offsets)
        if offset is not None and offset_counts[offset] == 1
    }
    label_counts: Counter[str] = Counter()
    label_offsets: dict[str, int] = {}
    label_lines: dict[str, int] = {}
    pending: list[str] = []
    for line_index, line in enumerate(lines):
        label = re.match(r"^\s*(\$L[0-9A-Za-z_]+):", line)
        if label is not None:
            name = label.group(1)
            label_counts[name] += 1
            label_lines[name] = line_index
            pending.append(name)
            continue
        offset = re.match(r"^\s*([0-9A-Fa-f]{5,8})\s+", line)
        if offset is not None and pending:
            value = int(offset.group(1), 16)
            for name in pending:
                label_offsets[name] = value
            pending.clear()

    result = {
        index: targets for index, targets in
        _candidate_exact_coff_switch_targets(parsed, caller_bytes, coff_object).items()
        if index not in parsed.local_control_flow_indices
    }
    body_size = len(caller_bytes.data)
    body_start = int(caller_bytes.start)
    body_end = int(caller_bytes.end)
    relocations = tuple(caller_bytes.relocations)

    def exact_symbol(name: str, value: int) -> Any | None:
        matches = [
            symbol
            for symbol in coff_object.symbols
            if symbol.name == name
            and symbol.section_number == caller_bytes.section_index
            and symbol.value == body_start + value
        ]
        return matches[0] if len(matches) == 1 else None

    for index, dispatch in enumerate(instructions):
        switch = _cc_listing._cod_exact_indexed_switch(dispatch)
        if (
            switch is None
            or index in parsed.local_control_flow_indices
            or index < 2
        ):
            continue
        table_label, index_register = switch
        table_offset = label_offsets.get(table_label)
        if (
            label_counts[table_label] != 1
            or table_offset is None
            or table_offset < 0
            or table_offset >= body_size
        ):
            continue
        entries: list[str] = []
        for line in lines[label_lines[table_label] + 1 :]:
            row = re.match(
                r"^\s*[0-9A-Fa-f]{5,8}\s+"
                r"(?:[0-9A-Fa-f]{2}\s+)+DD\s+(\$L[0-9A-Za-z_]+)\b",
                line,
                re.IGNORECASE,
            )
            if row is None:
                if entries:
                    break
                continue
            entries.append(row.group(1))
        compare = instructions[index - 2]
        branch = instructions[index - 1]
        compare_match = re.fullmatch(
            rf"cmp\s+{re.escape(index_register)}\s*,\s*(0x[0-9a-f]+|\d+)",
            compare.raw_text.strip().lower(),
        )
        try:
            compare_body = bytes(int(item, 16) for item in compare.bytes)
            branch_body = bytes(int(item, 16) for item in branch.bytes)
            dispatch_body = bytes(int(item, 16) for item in dispatch.bytes)
        except (TypeError, ValueError):
            continue
        register_code = _cc_catalog._COD_SWITCH_INDEX_CODES[index_register]
        count = (
            _parse_unsigned_assembly_integer(compare_match.group(1)) + 1
            if compare_match is not None else 0
        )
        branch_target_labels = re.findall(
            r"\$L[0-9A-Za-z_]+", _instruction_operand(branch)
        )
        branch_target_offset = (
            label_offsets.get(branch_target_labels[0])
            if len(branch_target_labels) == 1 else None
        )
        compare_encoding = (
            count >= 1
            and count <= 0x80
            and compare_body
            == bytes((0x83, 0xF8 | register_code, (count - 1) & 0xFF))
        )
        branch_encoding = (
            _instruction_mnemonic(branch) == "ja"
            and (
                len(branch_body) == 2 and branch_body[0] == 0x77
                or len(branch_body) == 6 and branch_body[:2] == b"\x0f\x87"
            )
        )
        branch_offset = offsets[index - 1]
        encoded_branch_target = (
            int(branch_offset) + 2 + struct.unpack("<b", branch_body[1:2])[0]
            if branch_offset is not None and len(branch_body) == 2
            else (
                int(branch_offset)
                + 6
                + struct.unpack("<i", branch_body[2:6])[0]
                if branch_offset is not None and len(branch_body) == 6
                else None
            )
        )
        target_offsets = [label_offsets.get(entry) for entry in entries]
        if (
            not compare_encoding
            or not branch_encoding
            or encoded_branch_target != branch_target_offset
            or branch_target_offset not in index_by_offset
            or len(entries) != count
            or table_offset + count * 4 > body_size
            or any(label_counts[entry] != 1 for entry in entries)
            or any(offset not in index_by_offset for offset in target_offsets)
            or any(
                offset is None or not 0 <= offset < body_size
                for offset in target_offsets
            )
            or not body_start <= body_start + table_offset < body_end
        ):
            continue
        dispatch_offset = offsets[index]
        if dispatch_offset is None:
            continue
        dispatch_relocations = [
            relocation
            for relocation in relocations
            if relocation.offset == int(dispatch_offset) + 3
        ]
        table_relocations = [
            [
                relocation
                for relocation in relocations
                if relocation.offset == table_offset + entry_index * 4
            ]
            for entry_index in range(count)
        ]
        table_symbol = exact_symbol(table_label, table_offset)
        target_symbols = [
            exact_symbol(entry, int(target_offset))
            if target_offset is not None else None
            for entry, target_offset in zip(entries, target_offsets)
        ]
        expected_sib = (2 << 6) | (register_code << 3) | 5
        masked_ranges = (
            range(int(dispatch_offset) + 3, int(dispatch_offset) + 7),
            *(range(table_offset + entry * 4, table_offset + entry * 4 + 4)
              for entry in range(count)),
        )
        if (
            dispatch_body != bytes((0xFF, 0x24, expected_sib, 0, 0, 0, 0))
            or caller_bytes.data[int(dispatch_offset):int(dispatch_offset) + 7]
            != dispatch_body
            or caller_bytes.data[table_offset:table_offset + count * 4]
            != b"\0" * (count * 4)
            or table_symbol is None
            or any(symbol is None for symbol in target_symbols)
            or len(dispatch_relocations) != 1
            or dispatch_relocations[0].type != IMAGE_REL_I386_DIR32
            or dispatch_relocations[0].symbol_name != table_label
            or dispatch_relocations[0].symbol_index != table_symbol.index
            or any(len(rows) != 1 for rows in table_relocations)
            or any(
                rows[0].type != IMAGE_REL_I386_DIR32
                or rows[0].symbol_name != entry
                or rows[0].symbol_index != symbol.index
                for rows, entry, symbol in zip(
                    table_relocations, entries, target_symbols
                )
                if symbol is not None
            )
            or any(
                byte_index >= len(caller_bytes.relocation_mask)
                or not caller_bytes.relocation_mask[byte_index]
                for byte_range in masked_ranges
                for byte_index in byte_range
            )
        ):
            continue
        target_indices = tuple(
            index_by_offset[int(offset)]
            for offset in target_offsets
            if offset is not None
        )
        result[index] = (
            *target_indices,
            index_by_offset[int(branch_target_offset)],
        )
    return result
