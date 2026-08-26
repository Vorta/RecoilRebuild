from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
from pathlib import Path
import re
import struct
import sys
from typing import Any, Iterable, Mapping, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_DIR32,
    IMAGE_REL_I386_REL32,
    relocation_type_name,
)
from _recoil.lib.pe import PeHeaders, parse_pe_headers, rva_to_offset
from _recoil.lib.progress import (
    DEFAULT_PROGRESS_PATH,
    ProgressDocument,
    address_value,
    normalize_address,
)
from _recoil.lib.tooling import REPO_ROOT, configure_stdio, display_path


DEFAULT_TRACKER = DEFAULT_PROGRESS_PATH
DEFAULT_REFERENCE = REPO_ROOT / "support" / "Recoil.exe"


class RelocationExpectationError(RuntimeError):
    pass


@dataclass(frozen=True)
class OperandSite:
    offset: int
    relocation_type: int
    kind: str
    instruction_offset: int
    opcode: str


@dataclass(frozen=True)
class DecodedInstruction:
    offset: int
    size: int
    opcode: str
    operand_sites: tuple[OperandSite, ...]
    relative8_offset: int | None = None


@dataclass(frozen=True)
class TargetIdentity:
    symbol_id: str
    address: int
    end_exclusive: int | None
    object_symbols: tuple[str, ...]
    source: str


@dataclass(frozen=True)
class InlineSwitchTable:
    dispatch_offset: int
    table_offset: int
    table_end: int
    entry_targets: tuple[int, ...]
    bound_instruction_offset: int
    branch_instruction_offset: int


_EXCEPTION_BASE_FIELDS = {
    "reviewed",
    "status",
    "exception_mode",
    "object_symbol",
    "offset",
    "offsets",
    "type",
    "target_symbol",
    "target_symbol_id",
    "coff_addend",
    "resolved_target_addend",
    "retail_target",
    "reason",
    "evidence_ids",
    "create_missing_data",
}
_EXCEPTION_CONTEXT_FIELDS = {
    "source_binding",
    "target_binding",
    "physical_target_binding",
    "physical_target_owner_binding",
    "physical_target_relationship",
    "witness_contract",
    "legacy_reviewed_catalog",
    "_legacy_reviewed_catalog",
}

PHYSICAL_TARGET_UNRESOLVED_VC5_TEMPORARY = (
    "physical-target-unresolved-vc5-temporary"
)
_PHYSICAL_TARGET_TOKEN_PREFIX = "@physical-target:"
_PHYSICAL_TARGET_WITNESS_CONTRACT = {
    "kind": "vc5-temporary-static-data",
    "symbol_family": "$T<digits>",
    "storage_class": 3,
    "symbol_type": 0,
    "section_name": ".rdata",
    "requires_initialized_data": True,
    "forbids_uninitialized_data": True,
    "forbids_writable_data": True,
    "one_symbol_for_all_sites": True,
}
_NAVIGATION_LABEL = re.compile(
    r"^[A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z_][A-Za-z0-9_]*)*$"
)


_PREFIXES = {0x26, 0x2E, 0x36, 0x3E, 0x64, 0x65, 0x66, 0x67, 0xF0, 0xF2, 0xF3}
_SIMPLE_ONE_BYTE = set(range(0x40, 0x60)) | {
    0x06,
    0x07,
    0x0E,
    0x16,
    0x17,
    0x1E,
    0x1F,
    0x27,
    0x2F,
    0x37,
    0x3F,
    0x60,
    0x61,
    0x90,
    0x91,
    0x92,
    0x93,
    0x94,
    0x95,
    0x96,
    0x97,
    0x98,
    0x99,
    0x9B,
    0x9C,
    0x9D,
    0x9E,
    0x9F,
    0xC3,
    0xC9,
    0xCB,
    0xCC,
    0xCE,
    0xCF,
    0xD6,
    0xD7,
    0xEC,
    0xED,
    0xEE,
    0xEF,
    0xF1,
    0xF4,
    0xF5,
    0xF8,
    0xF9,
    0xFA,
    0xFB,
    0xFC,
    0xFD,
}
_SIMPLE_ONE_BYTE.update(range(0xA4, 0xA8))
_SIMPLE_ONE_BYTE.update(range(0xAA, 0xB0))
_SIMPLE_ONE_BYTE.update(range(0x6C, 0x70))

_MODRM_ONE_BYTE = {
    0x00,
    0x01,
    0x02,
    0x03,
    0x08,
    0x09,
    0x0A,
    0x0B,
    0x10,
    0x11,
    0x12,
    0x13,
    0x18,
    0x19,
    0x1A,
    0x1B,
    0x20,
    0x21,
    0x22,
    0x23,
    0x28,
    0x29,
    0x2A,
    0x2B,
    0x30,
    0x31,
    0x32,
    0x33,
    0x38,
    0x39,
    0x3A,
    0x3B,
    0x62,
    0x63,
    0x69,
    0x6B,
    0x80,
    0x81,
    0x82,
    0x83,
    0x84,
    0x85,
    0x86,
    0x87,
    0x88,
    0x89,
    0x8A,
    0x8B,
    0x8C,
    0x8D,
    0x8E,
    0x8F,
    0xC0,
    0xC1,
    0xC4,
    0xC5,
    0xC6,
    0xC7,
    0xD0,
    0xD1,
    0xD2,
    0xD3,
    0xF6,
    0xF7,
    0xFE,
    0xFF,
}
_MODRM_ONE_BYTE.update(range(0xD8, 0xE0))

_TWO_BYTE_NO_MODRM = {
    0x05,
    0x06,
    0x07,
    0x08,
    0x09,
    0x0B,
    0x30,
    0x31,
    0x32,
    0x33,
    0x34,
    0x35,
    0x37,
    0x77,
    0xA0,
    0xA1,
    0xA2,
    0xA8,
    0xA9,
}
_TWO_BYTE_NO_MODRM.update(range(0xC8, 0xD0))


def _require(data: bytes, offset: int, size: int, *, instruction_offset: int) -> None:
    if offset + size > len(data):
        raise RelocationExpectationError(
            f"truncated x86 instruction at function offset 0x{instruction_offset:x}"
        )


def _modrm_extent(
    data: bytes,
    offset: int,
    *,
    instruction_offset: int,
) -> tuple[int, int, int | None]:
    _require(data, offset, 1, instruction_offset=instruction_offset)
    modrm = data[offset]
    mod = modrm >> 6
    reg = (modrm >> 3) & 7
    rm = modrm & 7
    cursor = offset + 1
    absolute_disp: int | None = None
    if mod != 3 and rm == 4:
        _require(data, cursor, 1, instruction_offset=instruction_offset)
        sib = data[cursor]
        cursor += 1
        base = sib & 7
        if mod == 0 and base == 5:
            absolute_disp = cursor
            cursor += 4
        elif mod == 1:
            cursor += 1
        elif mod == 2:
            cursor += 4
    elif mod == 0 and rm == 5:
        absolute_disp = cursor
        cursor += 4
    elif mod == 1:
        cursor += 1
    elif mod == 2:
        cursor += 4
    _require(data, offset, cursor - offset, instruction_offset=instruction_offset)
    return cursor, reg, absolute_disp


def _decode_one(data: bytes, offset: int) -> DecodedInstruction:
    start = offset
    operand_size_16 = False
    address_size_16 = False
    while offset < len(data) and data[offset] in _PREFIXES:
        operand_size_16 |= data[offset] == 0x66
        address_size_16 |= data[offset] == 0x67
        offset += 1
    _require(data, offset, 1, instruction_offset=start)
    opcode = data[offset]
    offset += 1
    sites: list[OperandSite] = []
    relative8_offset: int | None = None
    opcode_text = f"{opcode:02x}"

    if address_size_16:
        raise RelocationExpectationError(
            f"unsupported x86 address-size override at function offset 0x{start:x}"
        )

    if opcode in {0xE8, 0xE9}:
        if operand_size_16:
            raise RelocationExpectationError(
                f"unsupported 16-bit relative operand at function offset 0x{start:x}"
            )
        _require(data, offset, 4, instruction_offset=start)
        sites.append(OperandSite(offset, IMAGE_REL_I386_REL32, "rel32", start, opcode_text))
        offset += 4
    elif opcode == 0x0F:
        _require(data, offset, 1, instruction_offset=start)
        second = data[offset]
        offset += 1
        opcode_text = f"0f {second:02x}"
        if 0x80 <= second <= 0x8F:
            if operand_size_16:
                raise RelocationExpectationError(
                    f"unsupported 16-bit conditional branch at function offset 0x{start:x}"
                )
            _require(data, offset, 4, instruction_offset=start)
            sites.append(OperandSite(offset, IMAGE_REL_I386_REL32, "rel32", start, opcode_text))
            offset += 4
        elif second in _TWO_BYTE_NO_MODRM:
            pass
        else:
            cursor, _reg, absolute_disp = _modrm_extent(
                data, offset, instruction_offset=start
            )
            if absolute_disp is not None:
                sites.append(
                    OperandSite(
                        absolute_disp,
                        IMAGE_REL_I386_DIR32,
                        "absolute32",
                        start,
                        opcode_text,
                    )
                )
            offset = cursor
            if second in {0x70, 0x71, 0x72, 0x73, 0xA4, 0xAC, 0xBA, 0xC2, 0xC4, 0xC5, 0xC6}:
                _require(data, offset, 1, instruction_offset=start)
                offset += 1
    elif opcode in _SIMPLE_ONE_BYTE:
        pass
    elif opcode in {0xC2, 0xCA}:
        _require(data, offset, 2, instruction_offset=start)
        offset += 2
    elif opcode in {
        0x04,
        0x0C,
        0x14,
        0x1C,
        0x24,
        0x2C,
        0x34,
        0x3C,
        0xCD,
        0xD4,
        0xD5,
        0xE4,
        0xE5,
        0xE6,
        0xE7,
    }:
        _require(data, offset, 1, instruction_offset=start)
        offset += 1
    elif opcode == 0xC8:
        _require(data, offset, 3, instruction_offset=start)
        offset += 3
    elif opcode in {0xEA, 0x9A}:
        raise RelocationExpectationError(
            f"unsupported far address operand at function offset 0x{start:x}"
        )
    elif opcode in {0xEB} or 0x70 <= opcode <= 0x7F or 0xE0 <= opcode <= 0xE3:
        _require(data, offset, 1, instruction_offset=start)
        relative8_offset = offset
        offset += 1
    elif opcode == 0x6A:
        _require(data, offset, 1, instruction_offset=start)
        offset += 1
    elif opcode == 0x68:
        size = 2 if operand_size_16 else 4
        _require(data, offset, size, instruction_offset=start)
        if size == 4:
            sites.append(
                OperandSite(offset, IMAGE_REL_I386_DIR32, "potential-absolute32", start, opcode_text)
            )
        offset += size
    elif 0xB0 <= opcode <= 0xB7:
        _require(data, offset, 1, instruction_offset=start)
        offset += 1
    elif 0xB8 <= opcode <= 0xBF:
        size = 2 if operand_size_16 else 4
        _require(data, offset, size, instruction_offset=start)
        if size == 4:
            sites.append(
                OperandSite(offset, IMAGE_REL_I386_DIR32, "potential-absolute32", start, opcode_text)
            )
        offset += size
    elif opcode in {0xA0, 0xA1, 0xA2, 0xA3}:
        _require(data, offset, 4, instruction_offset=start)
        sites.append(OperandSite(offset, IMAGE_REL_I386_DIR32, "absolute32", start, opcode_text))
        offset += 4
    elif opcode in {0xA8}:
        _require(data, offset, 1, instruction_offset=start)
        offset += 1
    elif opcode in {0xA9, 0x05, 0x0D, 0x15, 0x1D, 0x25, 0x2D, 0x35, 0x3D}:
        size = 2 if operand_size_16 else 4
        _require(data, offset, size, instruction_offset=start)
        if size == 4:
            sites.append(
                OperandSite(offset, IMAGE_REL_I386_DIR32, "potential-absolute32", start, opcode_text)
            )
        offset += size
    elif opcode in _MODRM_ONE_BYTE:
        cursor, reg, absolute_disp = _modrm_extent(data, offset, instruction_offset=start)
        if absolute_disp is not None:
            sites.append(
                OperandSite(
                    absolute_disp,
                    IMAGE_REL_I386_DIR32,
                    "absolute32",
                    start,
                    opcode_text,
                )
            )
        offset = cursor
        immediate_size = 0
        potential_pointer = False
        if opcode in {0x69, 0x81, 0xC7}:
            immediate_size = 2 if operand_size_16 else 4
            potential_pointer = immediate_size == 4
        elif opcode in {0x6B, 0x80, 0x82, 0x83, 0xC0, 0xC1, 0xC6}:
            immediate_size = 1
        elif opcode == 0xF6 and reg in {0, 1}:
            immediate_size = 1
        elif opcode == 0xF7 and reg in {0, 1}:
            immediate_size = 2 if operand_size_16 else 4
            potential_pointer = immediate_size == 4
        if immediate_size:
            _require(data, offset, immediate_size, instruction_offset=start)
            if potential_pointer:
                sites.append(
                    OperandSite(
                        offset,
                        IMAGE_REL_I386_DIR32,
                        "potential-absolute32",
                        start,
                        opcode_text,
                    )
                )
            offset += immediate_size
    elif opcode in {0x9A, 0xEA}:
        raise RelocationExpectationError(
            f"unsupported far pointer at function offset 0x{start:x}"
        )
    else:
        raise RelocationExpectationError(
            f"unsupported x86 opcode 0x{opcode:02x} at function offset 0x{start:x}"
        )

    if offset <= start or offset - start > 15:
        raise RelocationExpectationError(
            f"invalid x86 instruction extent at function offset 0x{start:x}"
        )
    return DecodedInstruction(
        offset=start,
        size=offset - start,
        opcode=opcode_text,
        operand_sites=tuple(sites),
        relative8_offset=relative8_offset,
    )


def _indexed_switch_dispatch(
    data: bytes,
    instruction: DecodedInstruction,
) -> tuple[int, int] | None:
    """Return (index register, absolute table address) for `jmp [reg*4+disp32]`."""
    start = instruction.offset
    if instruction.size != 7 or data[start : start + 2] != b"\xff\x24":
        return None
    sib = data[start + 2]
    scale = (sib >> 6) & 3
    index_register = (sib >> 3) & 7
    base = sib & 7
    if scale != 2 or index_register == 4 or base != 5:
        return None
    if len(instruction.operand_sites) != 1:
        return None
    site = instruction.operand_sites[0]
    if (
        site.offset != start + 3
        or site.relocation_type != IMAGE_REL_I386_DIR32
        or site.kind != "absolute32"
    ):
        return None
    return index_register, struct.unpack_from("<I", data, start + 3)[0]


def _register_bound_compare(
    data: bytes,
    instruction: DecodedInstruction,
) -> tuple[int, int] | None:
    """Return (register, unsigned upper bound) for a narrow `cmp reg, imm` form."""
    start = instruction.offset
    if instruction.size == 3 and data[start] == 0x83:
        modrm = data[start + 1]
        if modrm >> 6 == 3 and (modrm >> 3) & 7 == 7:
            upper_bound = data[start + 2]
            if upper_bound <= 0x7F:
                return modrm & 7, upper_bound
    if instruction.size == 6 and data[start] == 0x81:
        modrm = data[start + 1]
        if modrm >> 6 == 3 and (modrm >> 3) & 7 == 7:
            return modrm & 7, struct.unpack_from("<I", data, start + 2)[0]
    return None


def _unsigned_above_target(data: bytes, instruction: DecodedInstruction) -> int | None:
    start = instruction.offset
    if instruction.size == 2 and data[start] == 0x77:
        return start + instruction.size + struct.unpack_from("<b", data, start + 1)[0]
    if instruction.size == 6 and data[start : start + 2] == b"\x0f\x87":
        return start + instruction.size + struct.unpack_from("<i", data, start + 2)[0]
    return None


def _flags_preserved_between(instructions: Sequence[DecodedInstruction]) -> bool:
    # This deliberately small whitelist covers register/memory moves, LEA, and
    # NOP without treating an unknown instruction as proof that CMP flags reach JA.
    return all(instruction.opcode in {"89", "8b", "8d", "90"} for instruction in instructions)


def _is_vc5_same_register_lea_nop(data: bytes, instruction: DecodedInstruction) -> bool:
    start = instruction.offset
    if instruction.opcode != "8d" or instruction.size != 3:
        return False
    if data[start] != 0x8D or data[start + 2] != 0:
        return False
    modrm = data[start + 1]
    return modrm >> 6 == 1 and (modrm >> 3) & 7 == modrm & 7 and modrm & 7 != 4


def _is_proven_switch_table_padding(
    data: bytes,
    instructions: Sequence[DecodedInstruction],
    *,
    start: int,
    end: int,
) -> bool:
    if start >= end:
        return False
    instructions_by_offset = {instruction.offset: instruction for instruction in instructions}
    offset = start
    while offset < end:
        instruction = instructions_by_offset.get(offset)
        if instruction is None or offset + instruction.size > end:
            return False
        if data[offset : offset + instruction.size] != b"\x90" and not _is_vc5_same_register_lea_nop(
            data, instruction
        ):
            return False
        offset += instruction.size
    return offset == end


def _proven_trailing_inline_switch_table(
    data: bytes,
    *,
    function_address: int,
    dispatch: DecodedInstruction,
    prior_instructions: Sequence[DecodedInstruction],
) -> tuple[InlineSwitchTable | None, dict[str, Any] | None]:
    indexed_dispatch = _indexed_switch_dispatch(data, dispatch)
    if indexed_dispatch is None:
        return None, None
    index_register, table_address = indexed_dispatch
    table_offset = table_address - function_address
    if table_offset < 0 or table_offset >= len(data):
        return None, None

    candidates: list[InlineSwitchTable] = []
    for branch_index, branch in enumerate(prior_instructions):
        branch_target = _unsigned_above_target(data, branch)
        if branch_target is None or branch.offset + branch.size != dispatch.offset:
            continue
        for compare_index, compare in enumerate(prior_instructions[:branch_index]):
            register_bound = _register_bound_compare(data, compare)
            if register_bound is None or register_bound[0] != index_register:
                continue
            if dispatch.offset - compare.offset > 0x20:
                continue
            if not _flags_preserved_between(
                prior_instructions[compare_index + 1 : branch_index]
            ):
                continue
            entry_count = register_bound[1] + 1
            table_end = table_offset + entry_count * 4
            if not (1 <= entry_count <= 0x100):
                continue
            if table_address & 3 or table_offset <= dispatch.offset + dispatch.size:
                continue
            # The only source shape proven here is a trailing inline table.  A
            # non-trailing extent needs independent boundary evidence.
            if table_end != len(data):
                continue
            if not (dispatch.offset + dispatch.size <= branch_target < table_offset):
                continue
            entry_targets = tuple(
                struct.unpack_from("<I", data, table_offset + index * 4)[0]
                for index in range(entry_count)
            )
            target_offsets = tuple(target - function_address for target in entry_targets)
            if not all(
                dispatch.offset + dispatch.size <= target_offset <= branch_target
                for target_offset in target_offsets
            ):
                continue
            candidates.append(
                InlineSwitchTable(
                    dispatch_offset=dispatch.offset,
                    table_offset=table_offset,
                    table_end=table_end,
                    entry_targets=entry_targets,
                    bound_instruction_offset=compare.offset,
                    branch_instruction_offset=branch.offset,
                )
            )

    unique_candidates = list(dict.fromkeys(candidates))
    if len(unique_candidates) == 1:
        return unique_candidates[0], None
    site_offset = dispatch.operand_sites[0].offset
    return None, {
        "kind": "ambiguous-inline-switch-table",
        "offset": site_offset,
        "instruction_offset": dispatch.offset,
        "candidate_count": len(unique_candidates),
        "table_address": f"0x{table_address:x}",
        "message": (
            "an indexed jump addresses the current function extent, but one exact "
            "same-register CMP/JA bound and trailing table extent were not proven"
        ),
    }


def decode_x86_operand_sites(
    data: bytes,
    *,
    function_address: int | None = None,
) -> tuple[tuple[OperandSite, ...], tuple[dict[str, Any], ...]]:
    sites: list[OperandSite] = []
    unresolved: list[dict[str, Any]] = []
    instructions: list[DecodedInstruction] = []
    switch_tables: dict[int, InlineSwitchTable] = {}
    skipped_switch_tables: set[int] = set()
    offset = 0
    while offset < len(data):
        switch_table = switch_tables.get(offset)
        if switch_table is not None:
            skipped_switch_tables.add(offset)
            offset = switch_table.table_end
            continue
        overlapping = next(
            (
                table
                for table in switch_tables.values()
                if table.table_offset < offset < table.table_end
            ),
            None,
        )
        if overlapping is not None:
            unresolved.append(
                {
                    "kind": "ambiguous-inline-switch-table-boundary",
                    "offset": offset,
                    "table_offset": overlapping.table_offset,
                    "message": "instruction decoding crossed a proposed inline switch-table boundary",
                }
            )
            break
        try:
            instruction = _decode_one(data, offset)
        except RelocationExpectationError as exc:
            unresolved.append(
                {
                    "kind": "unsupported-instruction",
                    "offset": offset,
                    "message": str(exc),
                }
            )
            break
        instructions.append(instruction)
        sites.extend(instruction.operand_sites)
        if instruction.relative8_offset is not None:
            displacement = struct.unpack_from("<b", data, instruction.relative8_offset)[0]
            target_offset = instruction.relative8_offset + 1 + displacement
            if target_offset < 0 or target_offset >= len(data):
                unresolved.append(
                    {
                        "kind": "unsupported-external-rel8",
                        "offset": instruction.relative8_offset,
                        "opcode": instruction.opcode,
                        "target_offset": target_offset,
                        "message": "an 8-bit branch leaves the current retail function extent",
                    }
                )
        if function_address is not None:
            table, table_unresolved = _proven_trailing_inline_switch_table(
                data,
                function_address=function_address,
                dispatch=instruction,
                prior_instructions=instructions[:-1],
            )
            if table_unresolved is not None:
                unresolved.append(table_unresolved)
                break
            if table is not None:
                if table.table_offset in switch_tables:
                    unresolved.append(
                        {
                            "kind": "ambiguous-inline-switch-table-boundary",
                            "offset": instruction.offset,
                            "table_offset": table.table_offset,
                            "message": "multiple dispatches claim one inline switch-table boundary",
                        }
                    )
                    break
                switch_tables[table.table_offset] = table
        offset += instruction.size

    instruction_offsets = {instruction.offset for instruction in instructions}
    for table in switch_tables.values():
        if table.table_offset not in skipped_switch_tables:
            unresolved.append(
                {
                    "kind": "ambiguous-inline-switch-table-boundary",
                    "offset": table.table_offset,
                    "message": "instruction decoding did not reach the proposed table boundary exactly",
                }
            )
            continue
        target_offsets = tuple(target - int(function_address) for target in table.entry_targets)
        missing_boundaries = sorted(
            target_offset
            for target_offset in target_offsets
            if target_offset not in instruction_offsets
        )
        return_boundaries = [
            instruction.offset + instruction.size
            for instruction in instructions
            if instruction.opcode in {"c2", "c3", "ca", "cb"}
            and instruction.offset + instruction.size <= table.table_offset
            and _is_proven_switch_table_padding(
                data,
                instructions,
                start=instruction.offset + instruction.size,
                end=table.table_offset,
            )
        ]
        if missing_boundaries or len(return_boundaries) != 1:
            unresolved.append(
                {
                    "kind": "ambiguous-inline-switch-table-boundary",
                    "offset": table.table_offset,
                    "dispatch_offset": table.dispatch_offset,
                    "missing_target_instruction_offsets": missing_boundaries,
                    "return_padding_boundary_count": len(return_boundaries),
                    "message": (
                        "inline switch-table entries must target decoded instruction boundaries, "
                        "and the trailing table must follow one RET plus NOP-only padding"
                    ),
                }
            )
            continue
        sites.extend(
            OperandSite(
                table.table_offset + index * 4,
                IMAGE_REL_I386_DIR32,
                "switch-table-entry",
                table.dispatch_offset,
                "switch-table",
            )
            for index in range(len(table.entry_targets))
        )
    sites.sort(key=lambda site: (site.offset, site.relocation_type, site.kind))
    unresolved.sort(key=lambda item: (int(item.get("offset", -1)), str(item.get("kind", ""))))
    return tuple(sites), tuple(unresolved)


def _pe_bytes(data: bytes, headers: PeHeaders, address: int, length: int) -> bytes:
    offset = rva_to_offset(address - headers.image_base, headers.sections)
    if offset is None or offset + length > len(data):
        raise RelocationExpectationError(
            f"retail address range [0x{address:x},0x{address + length:x}) is not file-backed"
        )
    return data[offset : offset + length]


def _integer(value: Any, *, field: str) -> int:
    if isinstance(value, int) and not isinstance(value, bool):
        return value
    if isinstance(value, str):
        try:
            return int(value, 0)
        except ValueError as exc:
            raise RelocationExpectationError(f"invalid {field} value {value!r}") from exc
    raise RelocationExpectationError(f"missing integer {field}")


def build_target_identities(
    document: ProgressDocument,
    bindings: Mapping[str, Sequence[Any]],
) -> tuple[TargetIdentity, ...]:
    identities, _blockers = build_target_identity_state(document, bindings)
    return identities


def _reviewed_exceptions(
    row: Mapping[str, Any], object_symbol: str
) -> tuple[Mapping[str, Any], ...]:
    selected: list[Mapping[str, Any]] = []
    for physical_row in row.get("physical_rows", ()):
        if not isinstance(physical_row, Mapping):
            continue
        values = physical_row.get("relocation_expectation_exceptions")
        if isinstance(values, list):
            for item in values:
                if (
                    isinstance(item, Mapping)
                    and item.get("object_symbol") == object_symbol
                    and (item.get("reviewed") is True or item.get("status") == "accepted")
                ):
                    selected.append(item)
        # Compatibility only: old rows explicitly called this a reviewed catalog.
        legacy = physical_row.get("retail_relocations")
        if isinstance(legacy, list):
            for item in legacy:
                if isinstance(item, Mapping) and item.get("object_symbol") == object_symbol:
                    selected.append({**item, "_legacy_reviewed_catalog": True})
    return tuple(selected)


def _string_list(value: Any, *, field: str, required: bool) -> list[str]:
    if not isinstance(value, list) or any(
        not isinstance(item, str) or not item for item in value
    ):
        raise RelocationExpectationError(f"{field} must be a string list")
    if required and not value:
        raise RelocationExpectationError(f"{field} must be non-empty")
    return sorted(set(value))


def _normalize_binding_snapshot(value: Any, *, field: str) -> dict[str, Any]:
    if not isinstance(value, Mapping):
        raise RelocationExpectationError(f"{field} must be an object")
    allowed = {
        "symbol_id",
        "address",
        "end_exclusive",
        "object_symbol",
        "physical_pipeline_class",
        "object_pipeline_class",
        "registration_ids",
        "evidence_ids",
        "alias_identity",
        "provider_identity",
    }
    unknown = sorted(str(key) for key in value if key not in allowed)
    if unknown:
        raise RelocationExpectationError(f"{field} contains unsupported fields: {unknown}")
    strings: dict[str, str] = {}
    for name in (
        "symbol_id",
        "object_symbol",
        "physical_pipeline_class",
        "object_pipeline_class",
    ):
        item = value.get(name)
        if not isinstance(item, str) or not item:
            raise RelocationExpectationError(f"{field}.{name} must be non-empty")
        strings[name] = item
    try:
        address = normalize_address(value.get("address"))
        end_exclusive = normalize_address(value.get("end_exclusive"))
    except (TypeError, ValueError) as exc:
        raise RelocationExpectationError(f"{field} has an invalid address extent") from exc
    if address_value(end_exclusive) <= address_value(address):
        raise RelocationExpectationError(f"{field} extent must be non-empty")
    result: dict[str, Any] = {
        "symbol_id": strings["symbol_id"],
        "address": address,
        "end_exclusive": end_exclusive,
        "object_symbol": strings["object_symbol"],
        "physical_pipeline_class": strings["physical_pipeline_class"],
        "object_pipeline_class": strings["object_pipeline_class"],
        "registration_ids": _string_list(
            value.get("registration_ids"), field=f"{field}.registration_ids", required=True
        ),
        "evidence_ids": _string_list(
            value.get("evidence_ids"), field=f"{field}.evidence_ids", required=False
        ),
    }
    for name in ("alias_identity", "provider_identity"):
        item = value.get(name)
        if item is None:
            continue
        if not isinstance(item, Mapping):
            raise RelocationExpectationError(f"{field}.{name} must be an object")
        # These are transparent tracker identity snapshots. JSON scalar/list values
        # only keep the exception reviewable and reject opaque/candidate structures.
        normalized_item: dict[str, Any] = {}
        for key, nested in item.items():
            if not isinstance(key, str) or not key:
                raise RelocationExpectationError(f"{field}.{name} has an invalid field name")
            if "candidate" in key.casefold():
                raise RelocationExpectationError(
                    f"candidate-derived exception field is forbidden: {field}.{name}.{key}"
                )
            if isinstance(nested, str) or nested is None or isinstance(nested, bool):
                normalized_item[key] = nested
            elif isinstance(nested, list) and all(
                isinstance(entry, str) and entry for entry in nested
            ):
                normalized_item[key] = sorted(set(nested))
            else:
                raise RelocationExpectationError(
                    f"{field}.{name}.{key} must be a string, boolean, null, or string list"
                )
        result[name] = normalized_item
    return result


def _normalize_physical_target_snapshot(value: Any) -> dict[str, Any]:
    field = "physical_target_binding"
    if not isinstance(value, Mapping):
        raise RelocationExpectationError(f"{field} must be an object")
    allowed = {
        "symbol_id",
        "binary",
        "kind",
        "address",
        "end_exclusive",
        "size",
        "extent_state",
        "output_section_id",
        "ownership_state",
        "retail_content_hex",
    }
    unknown = sorted(str(key) for key in value if key not in allowed)
    if unknown:
        raise RelocationExpectationError(f"{field} contains unsupported fields: {unknown}")
    expected_strings = {
        "binary": "recoil",
        "kind": "data",
        "extent_state": "known",
        "output_section_id": "recoil:section:.rdata",
    }
    result: dict[str, Any] = {}
    symbol_id = value.get("symbol_id")
    if not isinstance(symbol_id, str) or not symbol_id:
        raise RelocationExpectationError(f"{field}.symbol_id must be non-empty")
    result["symbol_id"] = symbol_id
    for name, expected in expected_strings.items():
        actual = value.get(name)
        if actual != expected:
            raise RelocationExpectationError(
                f"{field}.{name} must be {expected!r}"
            )
        result[name] = expected
    ownership_state = value.get("ownership_state")
    if not isinstance(ownership_state, str) or not ownership_state:
        raise RelocationExpectationError(
            f"{field}.ownership_state must be non-empty"
        )
    result["ownership_state"] = ownership_state
    try:
        address = normalize_address(value.get("address"))
        end_exclusive = normalize_address(value.get("end_exclusive"))
    except (TypeError, ValueError) as exc:
        raise RelocationExpectationError(
            f"{field} has an invalid address extent"
        ) from exc
    extent = address_value(end_exclusive) - address_value(address)
    if extent <= 0:
        raise RelocationExpectationError(f"{field} extent must be non-empty")
    size = _integer(value.get("size"), field=f"{field}.size")
    if size != extent:
        raise RelocationExpectationError(
            f"{field}.size must equal its exact address extent"
        )
    content_hex = value.get("retail_content_hex")
    if (
        not isinstance(content_hex, str)
        or len(content_hex) != extent * 2
        or any(character not in "0123456789abcdef" for character in content_hex)
    ):
        raise RelocationExpectationError(
            f"{field}.retail_content_hex must be exact lowercase hex for its extent"
        )
    result.update(
        address=address,
        end_exclusive=end_exclusive,
        size=size,
        retail_content_hex=content_hex,
    )
    return result


def _normalize_physical_witness_contract(value: Any) -> dict[str, Any]:
    if not isinstance(value, Mapping):
        raise RelocationExpectationError("witness_contract must be an object")
    normalized = dict(value)
    if normalized != _PHYSICAL_TARGET_WITNESS_CONTRACT:
        raise RelocationExpectationError(
            "witness_contract must equal the fixed VC5 temporary static-data contract"
        )
    return dict(_PHYSICAL_TARGET_WITNESS_CONTRACT)


def _normalize_create_missing_data(value: Any) -> dict[str, Any]:
    field = "create_missing_data"
    if not isinstance(value, Mapping):
        raise RelocationExpectationError(f"{field} must be an object")
    allowed = {"target_owner_id", "target_end_exclusive", "target_name"}
    keys = {str(key) for key in value}
    candidate_fields = sorted(key for key in keys if "candidate" in key.casefold())
    if candidate_fields:
        raise RelocationExpectationError(
            f"candidate-derived {field} fields are forbidden: {candidate_fields}"
        )
    unknown = sorted(keys - allowed)
    if unknown:
        raise RelocationExpectationError(
            f"{field} contains unsupported fields: {unknown}"
        )
    result: dict[str, Any] = {}
    for name in ("target_owner_id", "target_name"):
        item = value.get(name)
        if not isinstance(item, str) or not item.strip():
            raise RelocationExpectationError(f"{field}.{name} must be non-empty")
        result[name] = item.strip()
    if _NAVIGATION_LABEL.fullmatch(result["target_name"]) is None:
        raise RelocationExpectationError(
            f"{field}.target_name must use identifier/namespace components only; "
            "raw object symbols, ordinals, patterns, and regex syntax are forbidden"
        )
    try:
        result["target_end_exclusive"] = normalize_address(
            value.get("target_end_exclusive")
        )
    except (TypeError, ValueError) as exc:
        raise RelocationExpectationError(
            f"{field}.target_end_exclusive must be an address"
        ) from exc
    return result


def _normalize_physical_target_owner_binding(value: Any) -> dict[str, Any]:
    field = "physical_target_owner_binding"
    if not isinstance(value, Mapping):
        raise RelocationExpectationError(f"{field} must be an object")
    allowed = {
        "owner_id",
        "kind",
        "provider_state",
        "lifecycle_state",
        "binding_evidence_ids",
    }
    unknown = sorted(str(key) for key in value if key not in allowed)
    if unknown:
        raise RelocationExpectationError(f"{field} contains unsupported fields: {unknown}")
    result: dict[str, Any] = {}
    for name in ("owner_id", "kind", "provider_state", "lifecycle_state"):
        item = value.get(name)
        if not isinstance(item, str) or not item:
            raise RelocationExpectationError(f"{field}.{name} must be non-empty")
        result[name] = item
    if result["kind"] == "provider-boundary":
        raise RelocationExpectationError(
            f"{field} must identify an existing non-provider owner"
        )
    result["binding_evidence_ids"] = _string_list(
        value.get("binding_evidence_ids"),
        field=f"{field}.binding_evidence_ids",
        required=True,
    )
    return result


def _normalize_physical_target_relationship(value: Any) -> dict[str, Any]:
    field = "physical_target_relationship"
    if not isinstance(value, Mapping):
        raise RelocationExpectationError(f"{field} must be an object")
    allowed = {"kind", "address", "symbol_id", "name"}
    unknown = sorted(str(key) for key in value if key not in allowed)
    if unknown:
        raise RelocationExpectationError(f"{field} contains unsupported fields: {unknown}")
    if value.get("kind") != "primary-data":
        raise RelocationExpectationError(f"{field}.kind must be 'primary-data'")
    result = {"kind": "primary-data"}
    for name in ("symbol_id", "name"):
        item = value.get(name)
        if not isinstance(item, str) or not item:
            raise RelocationExpectationError(f"{field}.{name} must be non-empty")
        result[name] = item
    try:
        result["address"] = normalize_address(value.get("address"))
    except (TypeError, ValueError) as exc:
        raise RelocationExpectationError(f"{field}.address must be valid") from exc
    return result


def _exception_sites(value: Mapping[str, Any]) -> tuple[tuple[int, int], ...]:
    relocation_type = int(value["type"])
    if value.get("exception_mode") == PHYSICAL_TARGET_UNRESOLVED_VC5_TEMPORARY:
        return tuple((int(offset), relocation_type) for offset in value["offsets"])
    return ((int(value["offset"]), relocation_type),)


def normalize_reviewed_exception(value: Mapping[str, Any]) -> dict[str, Any]:
    """Validate one candidate-independent parent-owned exception payload or row."""
    legacy = (
        value.get("_legacy_reviewed_catalog") is True
        or value.get("legacy_reviewed_catalog") is True
    )
    keys = {str(key) for key in value}
    candidate_fields = sorted(key for key in keys if "candidate" in key.casefold())
    if candidate_fields:
        raise RelocationExpectationError(
            f"candidate-derived exception fields are forbidden: {candidate_fields}"
        )
    unknown = sorted(keys - _EXCEPTION_BASE_FIELDS - _EXCEPTION_CONTEXT_FIELDS)
    if unknown:
        raise RelocationExpectationError(
            f"reviewed relocation exception contains unsupported fields: {unknown}"
        )
    if not legacy and value.get("reviewed") is not True and value.get("status") != "accepted":
        raise RelocationExpectationError(
            "reviewed relocation exception must set reviewed=true or status=accepted"
        )
    mode = value.get("exception_mode")
    if mode is not None and mode != PHYSICAL_TARGET_UNRESOLVED_VC5_TEMPORARY:
        raise RelocationExpectationError(
            f"reviewed exception mode is unsupported: {mode!r}"
        )
    physical_mode = mode == PHYSICAL_TARGET_UNRESOLVED_VC5_TEMPORARY
    object_symbol = value.get("object_symbol")
    target_symbol = value.get("target_symbol")
    if not isinstance(object_symbol, str) or not object_symbol:
        raise RelocationExpectationError("reviewed exception object_symbol must be non-empty")
    if physical_mode and target_symbol is not None:
        raise RelocationExpectationError(
            "physical-target unresolved-provenance exceptions must not set target_symbol"
        )
    if not physical_mode and (not isinstance(target_symbol, str) or not target_symbol):
        raise RelocationExpectationError("reviewed exception target_symbol must be non-empty")
    target_symbol_id = value.get("target_symbol_id")
    if not legacy and (not isinstance(target_symbol_id, str) or not target_symbol_id):
        raise RelocationExpectationError(
            "reviewed exception target_symbol_id must be non-empty"
        )
    relocation_type = _integer(value.get("type"), field="exception relocation type")
    coff_addend = _integer(value.get("coff_addend"), field="reviewed COFF addend")
    target_addend = _integer(
        value.get("resolved_target_addend", 0), field="reviewed resolved target addend"
    )
    retail_target = _integer(value.get("retail_target"), field="reviewed retail target")
    if physical_mode:
        if "offset" in value:
            raise RelocationExpectationError(
                "physical-target unresolved-provenance exceptions use offsets, not offset"
            )
        raw_offsets = value.get("offsets")
        if (
            not isinstance(raw_offsets, list)
            or any(not isinstance(item, int) or isinstance(item, bool) for item in raw_offsets)
        ):
            raise RelocationExpectationError(
                "reviewed exception offsets must be an integer list"
            )
        offsets = sorted(set(raw_offsets))
        if len(offsets) != len(raw_offsets) or not offsets or offsets[0] < 0:
            raise RelocationExpectationError(
                "reviewed exception offsets must contain one or more unique non-negative sites"
            )
        if relocation_type != IMAGE_REL_I386_DIR32:
            raise RelocationExpectationError(
                "physical-target unresolved-provenance exceptions require IMAGE_REL_I386_DIR32"
            )
        if coff_addend != 0 or target_addend != 0:
            raise RelocationExpectationError(
                "physical-target unresolved-provenance exceptions require zero "
                "coff_addend and resolved_target_addend"
            )
    else:
        offset = _integer(value.get("offset"), field="exception offset")
        if offset < 0:
            raise RelocationExpectationError("reviewed exception offset must be non-negative")
    if relocation_type not in {IMAGE_REL_I386_REL32, IMAGE_REL_I386_DIR32}:
        raise RelocationExpectationError(
            f"reviewed exception relocation type is unsupported: 0x{relocation_type:04x}"
        )
    if not 0 <= coff_addend <= 0xFFFFFFFF:
        raise RelocationExpectationError("reviewed exception COFF addend must fit uint32")
    if not legacy:
        reason = value.get("reason")
        if not isinstance(reason, str) or not reason.strip():
            raise RelocationExpectationError("reviewed exception reason must be non-empty")
        evidence_ids = _string_list(
            value.get("evidence_ids"),
            field="reviewed exception evidence_ids",
            required=True,
        )
    else:
        evidence_ids = _string_list(
            value.get("evidence_ids", []),
            field="legacy reviewed exception evidence_ids",
            required=False,
        )
    normalized: dict[str, Any] = {
        "reviewed": True,
        "object_symbol": object_symbol,
        "type": relocation_type,
        "coff_addend": coff_addend,
        "resolved_target_addend": target_addend,
        "retail_target": retail_target,
        "reason": str(value.get("reason", "legacy reviewed retail relocation catalog")),
        "evidence_ids": evidence_ids,
    }
    if physical_mode:
        normalized["exception_mode"] = PHYSICAL_TARGET_UNRESOLVED_VC5_TEMPORARY
        normalized["offsets"] = offsets
    else:
        normalized["offset"] = offset
        normalized["target_symbol"] = target_symbol
    if isinstance(target_symbol_id, str) and target_symbol_id:
        normalized["target_symbol_id"] = target_symbol_id
    if legacy:
        normalized["legacy_reviewed_catalog"] = True
    for field in ("source_binding", "target_binding"):
        if field in value:
            normalized[field] = _normalize_binding_snapshot(value[field], field=field)
    if "physical_target_binding" in value:
        normalized["physical_target_binding"] = _normalize_physical_target_snapshot(
            value["physical_target_binding"]
        )
    if "witness_contract" in value:
        normalized["witness_contract"] = _normalize_physical_witness_contract(
            value["witness_contract"]
        )
    if "physical_target_owner_binding" in value:
        normalized["physical_target_owner_binding"] = (
            _normalize_physical_target_owner_binding(
                value["physical_target_owner_binding"]
            )
        )
    if "physical_target_relationship" in value:
        normalized["physical_target_relationship"] = (
            _normalize_physical_target_relationship(
                value["physical_target_relationship"]
            )
        )
    if "create_missing_data" in value:
        if not physical_mode:
            raise RelocationExpectationError(
                "create_missing_data is valid only for the physical-target "
                "unresolved-provenance mode"
            )
        normalized["create_missing_data"] = _normalize_create_missing_data(
            value["create_missing_data"]
        )
    if physical_mode:
        if "target_binding" in normalized:
            raise RelocationExpectationError(
                "physical-target unresolved-provenance exceptions must not store target_binding"
            )
        owner_context_present = "physical_target_owner_binding" in normalized
        relationship_present = "physical_target_relationship" in normalized
        if owner_context_present != relationship_present:
            raise RelocationExpectationError(
                "physical target owner binding and relationship must be stored together"
            )
    elif any(
        field in normalized
        for field in (
            "physical_target_binding",
            "physical_target_owner_binding",
            "physical_target_relationship",
            "witness_contract",
        )
    ):
        raise RelocationExpectationError(
            "physical target context is only valid for the unresolved-provenance mode"
        )
    return normalized


def _accepted_relocation_target_bindings(row: Mapping[str, Any]) -> tuple[Mapping[str, Any], ...]:
    value = row.get("relocation_target_binding")
    if isinstance(value, Mapping):
        rows: Iterable[Any] = (value,)
    elif isinstance(value, list):
        rows = value
    else:
        rows = ()
    return tuple(
        item
        for item in rows
        if isinstance(item, Mapping)
        and (item.get("reviewed") is True or item.get("status") == "accepted")
    )


def build_object_binding_snapshot(
    document: ProgressDocument,
    bindings: Mapping[str, Sequence[Any]],
    *,
    symbol_id: str,
    object_symbol: str,
) -> dict[str, Any]:
    """Return transparent current tracker/manifest identity facts for one object symbol."""
    row = document.collection("symbols").get(symbol_id)
    if not isinstance(row, Mapping) or row.get("binary") != "recoil":
        raise RelocationExpectationError(f"unknown Recoil physical symbol {symbol_id!r}")
    if not isinstance(row.get("address"), str) or not isinstance(
        row.get("end_exclusive"), str
    ):
        raise RelocationExpectationError(
            f"physical symbol {symbol_id!r} lacks a known address extent"
        )
    address = normalize_address(row["address"])
    end_exclusive = normalize_address(row["end_exclusive"])
    if address_value(end_exclusive) <= address_value(address):
        raise RelocationExpectationError(
            f"physical symbol {symbol_id!r} has an empty address extent"
        )

    registration_ids: set[str] = set()
    evidence_ids: set[str] = set()
    object_pipeline_class = str(row.get("pipeline_class", "unresolved"))
    alias_identity: dict[str, Any] | None = None
    provider_identity: dict[str, Any] | None = None

    for binding in bindings.get(symbol_id, ()):
        function = getattr(binding, "function", None)
        if getattr(function, "symbol", None) != object_symbol:
            continue
        target = getattr(binding, "target", None)
        target_name = str(getattr(target, "name", ""))
        logical_key = str(getattr(function, "logical_identity_key", ""))
        source_from = str(getattr(binding, "source_from", ""))
        identity = f"vc5:{target_name}"
        if logical_key:
            identity += f":logical:{logical_key}"
        if source_from:
            identity += f":source:{source_from.replace('\\', '/')}"
        registration_ids.add(identity)

    if row.get("object_symbol") == object_symbol:
        registration_ids.add("physical-row-object-symbol")

    aliases = row.get("logical_aliases")
    alias_matches: list[tuple[str, Mapping[str, Any]]] = []
    if isinstance(aliases, Mapping):
        alias_matches = [
            (str(identity_key), alias)
            for identity_key, alias in aliases.items()
            if isinstance(alias, Mapping) and alias.get("object_symbol") == object_symbol
        ]
    if len(alias_matches) > 1:
        raise RelocationExpectationError(
            f"physical symbol {symbol_id!r} has multiple logical aliases for {object_symbol!r}"
        )
    if alias_matches:
        identity_key, alias = alias_matches[0]
        alias_evidence = _string_list(
            list(alias.get("evidence_ids", [])),
            field="logical alias evidence_ids",
            required=False,
        )
        evidence_ids.update(alias_evidence)
        object_pipeline_class = str(alias.get("pipeline_class", "unresolved"))
        alias_identity = {
            "identity_key": identity_key,
            "pipeline_class": object_pipeline_class,
            "fold_status": (
                str(alias["fold_status"]) if isinstance(alias.get("fold_status"), str) else None
            ),
            "owner_id": str(alias["owner_id"]) if isinstance(alias.get("owner_id"), str) else None,
            "evidence_ids": alias_evidence,
        }
        registration_ids.add(f"logical-alias:{identity_key}")

    provider = row.get("linked_provider_binding")
    if isinstance(provider, Mapping) and provider.get("map_symbol") == object_symbol:
        required_provider_fields = (
            "symbol_id",
            "map_symbol",
            "object",
            "provider",
            "archive_member",
        )
        if any(not isinstance(provider.get(name), str) for name in required_provider_fields):
            raise RelocationExpectationError(
                f"physical symbol {symbol_id!r} has an incomplete linked provider binding"
            )
        provider_evidence = _string_list(
            list(provider.get("evidence_ids", [])),
            field="linked provider evidence_ids",
            required=False,
        )
        evidence_ids.update(provider_evidence)
        provider_identity = {
            **{name: str(provider[name]) for name in required_provider_fields},
            "evidence_ids": provider_evidence,
        }
        registration_ids.add(
            "provider:"
            + ":".join(str(provider[name]) for name in required_provider_fields)
        )

    for index, target_binding in enumerate(_accepted_relocation_target_bindings(row)):
        if target_binding.get("object_symbol") != object_symbol:
            continue
        binding_evidence = _string_list(
            list(target_binding.get("evidence_ids", [])),
            field="relocation target binding evidence_ids",
            required=True,
        )
        evidence_ids.update(binding_evidence)
        registration_ids.add(
            "reviewed-target-binding:"
            + str(index)
            + ":"
            + ",".join(binding_evidence)
        )

    if not registration_ids:
        raise RelocationExpectationError(
            f"object symbol {object_symbol!r} is not registered on physical symbol {symbol_id!r}"
        )
    snapshot: dict[str, Any] = {
        "symbol_id": symbol_id,
        "address": address,
        "end_exclusive": end_exclusive,
        "object_symbol": object_symbol,
        "physical_pipeline_class": str(row.get("pipeline_class", "unresolved")),
        "object_pipeline_class": object_pipeline_class,
        "registration_ids": sorted(registration_ids),
        "evidence_ids": sorted(evidence_ids),
    }
    if alias_identity is not None:
        snapshot["alias_identity"] = alias_identity
    if provider_identity is not None:
        snapshot["provider_identity"] = provider_identity
    return _normalize_binding_snapshot(snapshot, field="binding snapshot")


def build_physical_target_snapshot(
    document: ProgressDocument,
    *,
    symbol_id: str,
    reference: Path = DEFAULT_REFERENCE,
) -> dict[str, Any]:
    row = document.collection("symbols").get(symbol_id)
    if not isinstance(row, Mapping):
        raise RelocationExpectationError(
            f"unknown physical relocation target {symbol_id!r}"
        )
    try:
        address = address_value(str(row["address"]))
        end_exclusive = address_value(str(row["end_exclusive"]))
    except (KeyError, TypeError, ValueError) as exc:
        raise RelocationExpectationError(
            f"physical relocation target {symbol_id!r} lacks an exact extent"
        ) from exc
    if end_exclusive <= address:
        raise RelocationExpectationError(
            f"physical relocation target {symbol_id!r} has an empty extent"
        )
    image = reference.read_bytes()
    headers = parse_pe_headers(image, source=str(reference))
    content = _pe_bytes(image, headers, address, end_exclusive - address)
    return _normalize_physical_target_snapshot(
        {
            "symbol_id": symbol_id,
            "binary": row.get("binary"),
            "kind": row.get("kind"),
            "address": row.get("address"),
            "end_exclusive": row.get("end_exclusive"),
            "size": row.get("size"),
            "extent_state": row.get("extent_state"),
            "output_section_id": row.get("output_section_id"),
            "ownership_state": row.get("ownership_state"),
            "retail_content_hex": content.hex(),
        }
    )


def bind_reviewed_exception_context(
    value: Mapping[str, Any],
    *,
    document: ProgressDocument,
    bindings: Mapping[str, Sequence[Any]],
    source_symbol_id: str,
    reference: Path = DEFAULT_REFERENCE,
) -> dict[str, Any]:
    """Normalize and attach current source/target context before tracker mutation."""
    if any(
        field in value
        for field in (
            "source_binding",
            "target_binding",
            "physical_target_binding",
            "physical_target_owner_binding",
            "physical_target_relationship",
            "witness_contract",
        )
    ):
        raise RelocationExpectationError(
            "binding snapshots and witness_contract are derived by the mutation tool, "
            "not accepted as input"
        )
    if "create_missing_data" in value:
        raise RelocationExpectationError(
            "create_missing_data must be staged atomically by the mutation command"
        )
    normalized = normalize_reviewed_exception(value)
    if normalized.get("legacy_reviewed_catalog") is True:
        raise RelocationExpectationError("legacy relocation catalogs cannot be newly reviewed")
    evidence = document.collection("evidence")
    missing_evidence = sorted(
        evidence_id
        for evidence_id in normalized["evidence_ids"]
        if evidence_id not in evidence
    )
    if missing_evidence:
        raise RelocationExpectationError(
            f"reviewed exception references unknown tracker evidence ids: {missing_evidence}"
        )
    target_symbol_id = str(normalized["target_symbol_id"])
    normalized["source_binding"] = build_object_binding_snapshot(
        document,
        bindings,
        symbol_id=source_symbol_id,
        object_symbol=str(normalized["object_symbol"]),
    )
    if (
        normalized.get("exception_mode")
        == PHYSICAL_TARGET_UNRESOLVED_VC5_TEMPORARY
    ):
        normalized["physical_target_binding"] = build_physical_target_snapshot(
            document,
            symbol_id=target_symbol_id,
            reference=reference,
        )
        normalized["witness_contract"] = dict(
            _PHYSICAL_TARGET_WITNESS_CONTRACT
        )
    else:
        normalized["target_binding"] = build_object_binding_snapshot(
            document,
            bindings,
            symbol_id=target_symbol_id,
            object_symbol=str(normalized["target_symbol"]),
        )
    return normalize_reviewed_exception(normalized)


def reviewed_exception_staleness(
    value: Mapping[str, Any],
    *,
    document: ProgressDocument,
    bindings: Mapping[str, Sequence[Any]],
    reference: Path = DEFAULT_REFERENCE,
) -> tuple[dict[str, Any], list[dict[str, Any]]]:
    """Return normalized exception plus transparent current-context differences."""
    normalized = normalize_reviewed_exception(value)
    if normalized.get("legacy_reviewed_catalog") is True:
        return normalized, []
    differences: list[dict[str, Any]] = []
    bound_evidence_ids: set[str] = set(normalized.get("evidence_ids", ()))
    for binding_field in (
        "source_binding",
        "target_binding",
        "physical_target_binding",
        "physical_target_owner_binding",
    ):
        binding_value = normalized.get(binding_field)
        if not isinstance(binding_value, Mapping):
            continue
        bound_evidence_ids.update(binding_value.get("evidence_ids", ()))
        bound_evidence_ids.update(binding_value.get("binding_evidence_ids", ()))
        for identity_field in ("alias_identity", "provider_identity"):
            identity = binding_value.get(identity_field)
            if isinstance(identity, Mapping):
                bound_evidence_ids.update(identity.get("evidence_ids", ()))
    current_evidence = document.collection("evidence")
    missing_evidence_ids = sorted(
        evidence_id
        for evidence_id in bound_evidence_ids
        if evidence_id not in current_evidence
    )
    if missing_evidence_ids:
        differences.append(
            {
                "field": "evidence_ids",
                "reason": "missing-current-evidence",
                "stored": sorted(bound_evidence_ids),
                "current": sorted(set(bound_evidence_ids) - set(missing_evidence_ids)),
                "missing": missing_evidence_ids,
            }
        )
    binding_specs = [("source_binding", "source_binding", "object_symbol")]
    physical_mode = (
        normalized.get("exception_mode")
        == PHYSICAL_TARGET_UNRESOLVED_VC5_TEMPORARY
    )
    if not physical_mode:
        binding_specs.append(("target_binding", "target_symbol_id", "target_symbol"))
    for field, symbol_field, object_field in binding_specs:
        stored = normalized.get(field)
        if not isinstance(stored, Mapping):
            differences.append(
                {
                    "field": field,
                    "reason": "missing-binding-snapshot",
                    "stored": stored,
                    "current": None,
                }
            )
            continue
        symbol_id = (
            str(stored.get("symbol_id", ""))
            if symbol_field == "source_binding"
            else str(normalized.get(symbol_field, ""))
        )
        object_symbol = str(normalized.get(object_field, ""))
        try:
            current = build_object_binding_snapshot(
                document,
                bindings,
                symbol_id=symbol_id,
                object_symbol=object_symbol,
            )
        except RelocationExpectationError as exc:
            differences.append(
                {
                    "field": field,
                    "reason": "current-binding-invalid",
                    "stored": dict(stored),
                    "current": None,
                    "message": str(exc),
                }
            )
            continue
        if dict(stored) != current:
            differences.append(
                {
                    "field": field,
                    "reason": "binding-drift",
                    "stored": dict(stored),
                    "current": current,
                }
            )
    if physical_mode:
        stored = normalized.get("physical_target_binding")
        if not isinstance(stored, Mapping):
            differences.append(
                {
                    "field": "physical_target_binding",
                    "reason": "missing-binding-snapshot",
                    "stored": stored,
                    "current": None,
                }
            )
        else:
            try:
                current = build_physical_target_snapshot(
                    document,
                    symbol_id=str(normalized.get("target_symbol_id", "")),
                    reference=reference,
                )
            except (RelocationExpectationError, OSError, ValueError) as exc:
                differences.append(
                    {
                        "field": "physical_target_binding",
                        "reason": "current-binding-invalid",
                        "stored": dict(stored),
                        "current": None,
                        "message": str(exc),
                    }
                )
            else:
                if dict(stored) != current:
                    differences.append(
                        {
                            "field": "physical_target_binding",
                            "reason": "binding-drift",
                            "stored": dict(stored),
                            "current": current,
                        }
                    )
        stored_owner = normalized.get("physical_target_owner_binding")
        stored_relationship = normalized.get("physical_target_relationship")
        if isinstance(stored_owner, Mapping) and isinstance(
            stored_relationship, Mapping
        ):
            owner_id = str(stored_owner.get("owner_id", ""))
            owner = document.collection("owners").get(owner_id)
            if not isinstance(owner, Mapping) or owner.get("binary") != "recoil":
                differences.append(
                    {
                        "field": "physical_target_owner_binding",
                        "reason": "current-owner-invalid",
                        "stored": dict(stored_owner),
                        "current": None,
                    }
                )
            else:
                owner_evidence = {
                    str(item) for item in owner.get("evidence_ids", ())
                }
                invalid_owner_evidence: list[str] = []
                for evidence_id in stored_owner["binding_evidence_ids"]:
                    evidence_row = document.collection("evidence").get(evidence_id)
                    scopes = (
                        {
                            str(item)
                            for item in evidence_row.get("scope_ids", ())
                        }
                        if isinstance(evidence_row, Mapping)
                        else set()
                    )
                    if (
                        evidence_id not in owner_evidence
                        or owner_id not in scopes
                    ):
                        invalid_owner_evidence.append(evidence_id)
                if invalid_owner_evidence:
                    differences.append(
                        {
                            "field": "physical_target_owner_binding.binding_evidence_ids",
                            "reason": "owner-evidence-drift",
                            "stored": list(
                                stored_owner["binding_evidence_ids"]
                            ),
                            "current": sorted(
                                set(stored_owner["binding_evidence_ids"])
                                - set(invalid_owner_evidence)
                            ),
                            "invalid": sorted(invalid_owner_evidence),
                        }
                    )
                try:
                    current_owner = relocation_target_owner_context(
                        owner_id=owner_id,
                        owner=owner,
                        evidence_ids=stored_owner["binding_evidence_ids"],
                    )
                except (RelocationExpectationError, KeyError, TypeError) as exc:
                    differences.append(
                        {
                            "field": "physical_target_owner_binding",
                            "reason": "current-owner-invalid",
                            "stored": dict(stored_owner),
                            "current": None,
                            "message": str(exc),
                        }
                    )
                else:
                    if dict(stored_owner) != current_owner:
                        differences.append(
                            {
                                "field": "physical_target_owner_binding",
                                "reason": "owner-binding-drift",
                                "stored": dict(stored_owner),
                                "current": current_owner,
                            }
                        )
                matches: list[dict[str, Any]] = []
                for item in owner.get("relationships", ()):
                    if not isinstance(item, Mapping):
                        continue
                    try:
                        normalized_relationship = (
                            _normalize_physical_target_relationship(item)
                        )
                    except RelocationExpectationError:
                        continue
                    if (
                        normalized_relationship["symbol_id"]
                        == stored_relationship["symbol_id"]
                        and normalized_relationship["address"]
                        == stored_relationship["address"]
                    ):
                        matches.append(normalized_relationship)
                if matches != [dict(stored_relationship)]:
                    differences.append(
                        {
                            "field": "physical_target_relationship",
                            "reason": "owner-relationship-drift",
                            "stored": dict(stored_relationship),
                            "current": matches,
                        }
                    )
    return normalized, differences


def decode_retail_relocation_site(
    *,
    row: Mapping[str, Any],
    offset: int,
    relocation_type: int,
    reference: Path = DEFAULT_REFERENCE,
) -> dict[str, Any]:
    """Decode one exact immutable-retail relocation operand inside a physical symbol."""
    address = address_value(str(row.get("address", "")))
    end_exclusive = address_value(str(row.get("end_exclusive", "")))
    if end_exclusive <= address:
        raise RelocationExpectationError("retail source extent must be non-empty")
    image = reference.read_bytes()
    headers = parse_pe_headers(image, source=str(reference))
    retail_bytes = _pe_bytes(image, headers, address, end_exclusive - address)
    sites, decoder_unresolved = decode_x86_operand_sites(
        retail_bytes,
        function_address=address,
    )
    if decoder_unresolved:
        first = decoder_unresolved[0]
        raise RelocationExpectationError(
            "retail source instruction decoding is incomplete: " + str(first.get("message"))
        )
    matches = [
        site
        for site in sites
        if site.offset == offset and site.relocation_type == relocation_type
    ]
    if len(matches) != 1:
        raise RelocationExpectationError(
            f"retail relocation site ({offset},0x{relocation_type:04x}) resolved {len(matches)} times"
        )
    site = matches[0]
    operand = retail_bytes[offset : offset + 4]
    if len(operand) != 4:
        raise RelocationExpectationError("retail relocation operand is truncated")
    if relocation_type == IMAGE_REL_I386_REL32:
        retail_target = address + offset + 4 + struct.unpack("<i", operand)[0]
        if address <= retail_target < end_exclusive:
            raise RelocationExpectationError(
                "internal retail relative branch does not require a COFF relocation exception"
            )
    elif relocation_type == IMAGE_REL_I386_DIR32:
        retail_target = struct.unpack("<I", operand)[0]
        image_end = headers.image_base + headers.size_of_image
        if site.kind == "potential-absolute32" and not (
            headers.image_base <= retail_target < image_end
        ):
            raise RelocationExpectationError(
                "retail immediate is not an image address and cannot be reviewed as a relocation"
            )
    else:
        raise RelocationExpectationError(
            f"unsupported relocation type 0x{relocation_type:04x}"
        )
    return {
        "source_address": address,
        "source_end_exclusive": end_exclusive,
        "offset": offset,
        "type": relocation_type,
        "type_name": relocation_type_name(relocation_type),
        "retail_target": retail_target,
        "operand_kind": site.kind,
        "instruction_offset": site.instruction_offset,
        "opcode": site.opcode,
    }


def decode_retail_target_sites(
    *,
    row: Mapping[str, Any],
    retail_target: int,
    reference: Path = DEFAULT_REFERENCE,
) -> tuple[dict[str, Any], ...]:
    """Return every decoded immutable-retail address operand for one exact target."""
    address = address_value(str(row.get("address", "")))
    end_exclusive = address_value(str(row.get("end_exclusive", "")))
    if end_exclusive <= address:
        raise RelocationExpectationError("retail source extent must be non-empty")
    image = reference.read_bytes()
    headers = parse_pe_headers(image, source=str(reference))
    retail_bytes = _pe_bytes(image, headers, address, end_exclusive - address)
    sites, decoder_unresolved = decode_x86_operand_sites(
        retail_bytes,
        function_address=address,
    )
    if decoder_unresolved:
        first = decoder_unresolved[0]
        raise RelocationExpectationError(
            "retail source instruction decoding is incomplete: "
            + str(first.get("message"))
        )
    matched: list[dict[str, Any]] = []
    image_end = headers.image_base + headers.size_of_image
    for site in sites:
        operand = retail_bytes[site.offset : site.offset + 4]
        if len(operand) != 4:
            raise RelocationExpectationError(
                f"retail relocation operand at offset {site.offset} is truncated"
            )
        if site.relocation_type == IMAGE_REL_I386_REL32:
            decoded_target = (
                address + site.offset + 4 + struct.unpack("<i", operand)[0]
            )
            if address <= decoded_target < end_exclusive:
                continue
        elif site.relocation_type == IMAGE_REL_I386_DIR32:
            decoded_target = struct.unpack("<I", operand)[0]
            if site.kind == "potential-absolute32" and not (
                headers.image_base <= decoded_target < image_end
            ):
                continue
        else:
            continue
        if decoded_target == retail_target:
            matched.append(
                {
                    "offset": site.offset,
                    "type": site.relocation_type,
                    "type_name": relocation_type_name(site.relocation_type),
                    "retail_target": decoded_target,
                    "instruction_offset": site.instruction_offset,
                    "opcode": site.opcode,
                }
            )
    return tuple(
        sorted(matched, key=lambda item: (int(item["offset"]), int(item["type"])))
    )


def decode_retail_relocation_at_offset(
    *,
    row: Mapping[str, Any],
    offset: int,
    reference: Path = DEFAULT_REFERENCE,
) -> dict[str, Any]:
    """Derive relocation type and target from one exact immutable-retail operand offset."""
    address = address_value(str(row.get("address", "")))
    end_exclusive = address_value(str(row.get("end_exclusive", "")))
    if end_exclusive <= address:
        raise RelocationExpectationError("retail source extent must be non-empty")
    image = reference.read_bytes()
    headers = parse_pe_headers(image, source=str(reference))
    retail_bytes = _pe_bytes(image, headers, address, end_exclusive - address)
    sites, decoder_unresolved = decode_x86_operand_sites(
        retail_bytes,
        function_address=address,
    )
    if decoder_unresolved:
        first = decoder_unresolved[0]
        raise RelocationExpectationError(
            "retail source instruction decoding is incomplete: " + str(first.get("message"))
        )
    matches = [site for site in sites if site.offset == offset]
    if len(matches) != 1:
        raise RelocationExpectationError(
            f"retail relocation offset {offset} resolved {len(matches)} times"
        )
    return decode_retail_relocation_site(
        row=row,
        offset=offset,
        relocation_type=matches[0].relocation_type,
        reference=reference,
    )


def _normalize_target_row_context(value: Any) -> dict[str, Any]:
    if not isinstance(value, Mapping):
        raise RelocationExpectationError("relocation target row context must be an object")
    allowed = {
        "symbol_id",
        "address",
        "end_exclusive",
        "kind",
        "navigation_name",
        "object_symbol",
        "output_section_id",
        "pipeline_class",
        "ownership_state",
    }
    unknown = sorted(str(key) for key in value if key not in allowed)
    if unknown:
        raise RelocationExpectationError(
            f"relocation target row context has unsupported fields: {unknown}"
        )
    result: dict[str, Any] = {}
    for field in ("symbol_id", "kind", "navigation_name", "object_symbol", "output_section_id"):
        item = value.get(field)
        if not isinstance(item, str) or not item:
            raise RelocationExpectationError(
                f"relocation target row context {field} must be non-empty"
            )
        result[field] = item
    result["address"] = normalize_address(value.get("address"))
    result["end_exclusive"] = normalize_address(value.get("end_exclusive"))
    if address_value(result["end_exclusive"]) <= address_value(result["address"]):
        raise RelocationExpectationError("relocation target row extent must be non-empty")
    for field in ("pipeline_class", "ownership_state"):
        item = value.get(field)
        if item is not None and not isinstance(item, str):
            raise RelocationExpectationError(
                f"relocation target row context {field} must be a string or null"
            )
        result[field] = item
    return result


def _normalize_target_owner_context(value: Any) -> dict[str, Any]:
    if not isinstance(value, Mapping):
        raise RelocationExpectationError("relocation target owner context must be an object")
    allowed = {
        "owner_id",
        "kind",
        "provider_state",
        "lifecycle_state",
        "binding_evidence_ids",
    }
    unknown = sorted(str(key) for key in value if key not in allowed)
    if unknown:
        raise RelocationExpectationError(
            f"relocation target owner context has unsupported fields: {unknown}"
        )
    result: dict[str, Any] = {}
    for field in ("owner_id", "kind", "provider_state", "lifecycle_state"):
        item = value.get(field)
        if not isinstance(item, str) or not item:
            raise RelocationExpectationError(
                f"relocation target owner context {field} must be non-empty"
            )
        result[field] = item
    result["binding_evidence_ids"] = _string_list(
        value.get("binding_evidence_ids"),
        field="relocation target owner binding_evidence_ids",
        required=True,
    )
    return result


def _target_primary_relationship_kind(target: Mapping[str, Any]) -> str:
    target_kind = target.get("kind")
    if target_kind in {"function", "provider-function", "compiler-function"}:
        return "primary-function"
    if target_kind in {"data", "data-symbol", "provider-data", "compiler-data"}:
        return "primary-data"
    raise RelocationExpectationError(
        f"relocation target row kind {target_kind!r} has no primary relationship contract"
    )


def _normalize_target_relationship(
    value: Any, *, target: Mapping[str, Any]
) -> dict[str, Any] | None:
    if value is None:
        return None
    if not isinstance(value, Mapping):
        raise RelocationExpectationError("relocation target relationship must be null or an object")
    candidate_fields = sorted(
        str(key) for key in value if "candidate" in str(key).casefold()
    )
    if candidate_fields:
        raise RelocationExpectationError(
            "candidate-derived relocation target relationship fields are forbidden: "
            f"{candidate_fields}"
        )
    expected_kind = _target_primary_relationship_kind(target)
    allowed = (
        {"kind", "address", "symbol_id", "name"}
        if expected_kind == "primary-data"
        else {"kind", "address", "symbol_id"}
    )
    unknown = sorted(str(key) for key in value if key not in allowed)
    if unknown:
        raise RelocationExpectationError(
            f"relocation target relationship has unsupported fields: {unknown}"
        )
    if value.get("kind") != expected_kind:
        raise RelocationExpectationError(
            "relocation target relationship kind does not match target row kind: "
            f"expected {expected_kind}"
        )
    symbol_id = value.get("symbol_id")
    if not isinstance(symbol_id, str) or not symbol_id:
        raise RelocationExpectationError("relocation target relationship symbol_id is required")
    address = normalize_address(value.get("address"))
    if symbol_id != target.get("symbol_id"):
        raise RelocationExpectationError(
            "relocation target relationship symbol_id does not match target row"
        )
    if address != target.get("address"):
        raise RelocationExpectationError(
            "relocation target relationship address does not match target row"
        )
    result = {
        "kind": expected_kind,
        "address": address,
        "symbol_id": symbol_id,
    }
    if expected_kind == "primary-data":
        name = value.get("name")
        if not isinstance(name, str) or not name:
            raise RelocationExpectationError("relocation target relationship name is required")
        result["name"] = name
    return result


def normalize_relocation_target_binding(value: Mapping[str, Any]) -> dict[str, Any]:
    """Normalize one governed, candidate-independent relocation target binding row."""
    allowed = {
        "reviewed",
        "object_symbol",
        "reason",
        "evidence_ids",
        "binding_context",
    }
    keys = {str(key) for key in value}
    candidate_fields = sorted(key for key in keys if "candidate" in key.casefold())
    if candidate_fields:
        raise RelocationExpectationError(
            f"candidate-derived relocation target fields are forbidden: {candidate_fields}"
        )
    unknown = sorted(keys - allowed)
    if unknown:
        raise RelocationExpectationError(
            f"relocation target binding contains unsupported fields: {unknown}"
        )
    if value.get("reviewed") is not True:
        raise RelocationExpectationError("relocation target binding must set reviewed=true")
    object_symbol = value.get("object_symbol")
    reason = value.get("reason")
    if not isinstance(object_symbol, str) or not object_symbol:
        raise RelocationExpectationError("relocation target object_symbol must be non-empty")
    if not isinstance(reason, str) or not reason.strip():
        raise RelocationExpectationError("relocation target reason must be non-empty")
    evidence_ids = _string_list(
        value.get("evidence_ids"),
        field="relocation target evidence_ids",
        required=True,
    )
    context = value.get("binding_context")
    if not isinstance(context, Mapping):
        raise RelocationExpectationError("relocation target binding_context must be an object")
    context_allowed = {
        "source_binding",
        "relocation",
        "target",
        "owner",
        "relationship",
        "creation_mode",
    }
    context_unknown = sorted(str(key) for key in context if key not in context_allowed)
    if context_unknown:
        raise RelocationExpectationError(
            f"relocation target binding_context has unsupported fields: {context_unknown}"
        )
    creation_mode = context.get("creation_mode")
    if creation_mode not in {"existing-symbol", "created-data-symbol"}:
        raise RelocationExpectationError("invalid relocation target creation_mode")
    relocation = context.get("relocation")
    if not isinstance(relocation, Mapping):
        raise RelocationExpectationError("relocation target relocation context must be an object")
    relocation_allowed = {
        "offset",
        "type",
        "type_name",
        "retail_target",
        "instruction_offset",
        "opcode",
    }
    relocation_unknown = sorted(str(key) for key in relocation if key not in relocation_allowed)
    if relocation_unknown:
        raise RelocationExpectationError(
            f"relocation context has unsupported fields: {relocation_unknown}"
        )
    relocation_type = _integer(relocation.get("type"), field="retail relocation type")
    if relocation_type not in {IMAGE_REL_I386_REL32, IMAGE_REL_I386_DIR32}:
        raise RelocationExpectationError(
            f"unsupported relocation target type 0x{relocation_type:04x}"
        )
    normalized_relocation = {
        "offset": _integer(relocation.get("offset"), field="retail relocation offset"),
        "type": relocation_type,
        "type_name": relocation_type_name(relocation_type),
        "retail_target": _integer(
            relocation.get("retail_target"), field="retail relocation target"
        ),
        "instruction_offset": _integer(
            relocation.get("instruction_offset"), field="retail instruction offset"
        ),
        "opcode": str(relocation.get("opcode", "")),
    }
    if not normalized_relocation["opcode"]:
        raise RelocationExpectationError("retail relocation opcode must be non-empty")
    normalized_target = _normalize_target_row_context(context.get("target"))
    normalized_owner = _normalize_target_owner_context(context.get("owner"))
    normalized_relationship = _normalize_target_relationship(
        context.get("relationship"), target=normalized_target
    )
    if normalized_owner["kind"] == "provider-boundary":
        if normalized_relationship is not None:
            raise RelocationExpectationError(
                "provider relocation target relationship must be null"
            )
    elif normalized_relationship is None:
        raise RelocationExpectationError(
            "non-provider relocation target relationship must be present"
        )
    normalized_context = {
        "source_binding": _normalize_binding_snapshot(
            context.get("source_binding"), field="relocation target source_binding"
        ),
        "relocation": normalized_relocation,
        "target": normalized_target,
        "owner": normalized_owner,
        "relationship": normalized_relationship,
        "creation_mode": creation_mode,
    }
    if normalized_context["target"]["object_symbol"] != object_symbol:
        raise RelocationExpectationError(
            "relocation target object_symbol differs from its bound target context"
        )
    if normalized_context["owner"]["binding_evidence_ids"] != evidence_ids:
        raise RelocationExpectationError(
            "relocation target evidence_ids differ from owner binding evidence context"
        )
    return {
        "reviewed": True,
        "object_symbol": object_symbol,
        "reason": reason.strip(),
        "evidence_ids": evidence_ids,
        "binding_context": normalized_context,
    }


def relocation_target_row_context(
    *, symbol_id: str, row: Mapping[str, Any], object_symbol: str
) -> dict[str, Any]:
    return _normalize_target_row_context(
        {
            "symbol_id": symbol_id,
            "address": row.get("address"),
            "end_exclusive": row.get("end_exclusive"),
            "kind": row.get("kind"),
            "navigation_name": row.get("navigation_name"),
            "object_symbol": object_symbol,
            "output_section_id": row.get("output_section_id"),
            "pipeline_class": row.get("pipeline_class"),
            "ownership_state": row.get("ownership_state"),
        }
    )


def relocation_target_owner_context(
    *, owner_id: str, owner: Mapping[str, Any], evidence_ids: Sequence[str]
) -> dict[str, Any]:
    return _normalize_target_owner_context(
        {
            "owner_id": owner_id,
            "kind": owner.get("kind"),
            "provider_state": owner.get("provider_state"),
            "lifecycle_state": owner.get("lifecycle_state"),
            "binding_evidence_ids": list(evidence_ids),
        }
    )


def _provider_null_relationship_state(
    *,
    document: ProgressDocument,
    owner: Mapping[str, Any],
    target: Mapping[str, Any],
) -> tuple[bool, list[dict[str, Any]], list[dict[str, Any]]]:
    """Validate exact owner relationships for a provider target stored as null.

    Provider relocation bindings deliberately store relationship=null.  A
    registered imported-function target nevertheless has two typed owner
    views at one IAT address: the callable provider-function and its primary
    data storage.  A non-IAT provider function has exactly one primary-function
    relationship instead.  This helper recognizes only those two exact shapes;
    unrelated provider-owner relationships do not participate.
    """

    if owner.get("kind") != "provider-boundary" or target.get("kind") not in {
        "function",
        "provider-function",
        "compiler-function",
    }:
        return False, [], []
    target_symbol_id = str(target.get("symbol_id", ""))
    target_address = normalize_address(target.get("address"))
    symbols = document.collection("symbols")
    coaddressed_data: dict[str, Mapping[str, Any]] = {}
    for symbol_id, row in symbols.items():
        if not isinstance(row, Mapping) or row.get("binary") != "recoil":
            continue
        if row.get("kind") not in {"data", "data-symbol", "provider-data", "compiler-data"}:
            continue
        try:
            row_address = normalize_address(row.get("address"))
        except (TypeError, ValueError):
            continue
        if row_address == target_address:
            coaddressed_data[str(symbol_id)] = row

    raw_relationships = owner.get("relationships", ())
    if not isinstance(raw_relationships, (list, tuple)):
        return True, [], [
            {
                "relationship": raw_relationships,
                "message": "provider owner relationships must be a list",
            }
        ]

    function_rows: list[Mapping[str, Any]] = []
    data_rows: list[Mapping[str, Any]] = []
    invalid: list[dict[str, Any]] = []
    if owner.get("provider_state") != "accepted":
        invalid.append(
            {
                "relationship": None,
                "message": "provider target owner must have provider_state=accepted",
            }
        )
    # provider-function is the tracker type for the callable view of an IAT
    # slot.  Keep that existing two-view contract fail-closed even when its
    # co-addressed data row is missing; ordinary compiler/provider bodies use
    # function or compiler-function and take the one-relationship branch.
    iat_pair = target.get("kind") == "provider-function"
    for item in raw_relationships:
        if not isinstance(item, Mapping):
            continue
        kind = item.get("kind")
        symbol_id = str(item.get("symbol_id", ""))
        try:
            address = normalize_address(item.get("address"))
        except (TypeError, ValueError):
            address = None
        function_relevant = (
            symbol_id == target_symbol_id
            or (kind == "primary-function" and address == target_address)
        )
        data_relevant = (
            iat_pair
            and (
                symbol_id in coaddressed_data
                or (kind == "primary-data" and address == target_address)
            )
        )
        if not function_relevant and not data_relevant:
            continue
        candidate_fields = sorted(
            str(key) for key in item if "candidate" in str(key).casefold()
        )
        if candidate_fields:
            invalid.append(
                {
                    "relationship": dict(item),
                    "message": (
                        "candidate-derived provider relationship fields are forbidden: "
                        f"{candidate_fields}"
                    ),
                }
            )
            continue
        if function_relevant:
            function_rows.append(item)
        elif data_relevant:
            data_rows.append(item)

    current: list[dict[str, Any]] = []
    valid_function: list[dict[str, Any]] = []
    for item in function_rows:
        try:
            normalized = _normalize_target_relationship(item, target=target)
        except RelocationExpectationError as exc:
            invalid.append({"relationship": dict(item), "message": str(exc)})
        else:
            if normalized is not None:
                valid_function.append(normalized)
    if len(valid_function) != 1:
        invalid.append(
            {
                "relationship": [dict(item) for item in function_rows],
                "message": (
                    "provider target requires exactly one valid primary-function "
                    f"relationship; found {len(valid_function)}"
                ),
            }
        )
    else:
        current.extend(valid_function)

    if not iat_pair:
        return True, current, invalid

    valid_data: list[dict[str, Any]] = []
    for item in data_rows:
        symbol_id = str(item.get("symbol_id", ""))
        data_row = coaddressed_data.get(symbol_id)
        if data_row is None:
            invalid.append(
                {
                    "relationship": dict(item),
                    "message": (
                        "provider IAT primary-data relationship does not resolve to one "
                        "co-addressed Recoil data symbol"
                    ),
                }
            )
            continue
        data_target = {
            "symbol_id": symbol_id,
            "address": data_row.get("address"),
            "kind": data_row.get("kind"),
        }
        try:
            normalized = _normalize_target_relationship(item, target=data_target)
        except RelocationExpectationError as exc:
            invalid.append({"relationship": dict(item), "message": str(exc)})
        else:
            if normalized is not None:
                valid_data.append(normalized)
    if len(valid_data) != 1:
        invalid.append(
            {
                "relationship": [dict(item) for item in data_rows],
                "message": (
                    "provider IAT target requires exactly one valid same-address primary-data "
                    f"relationship; found {len(valid_data)}"
                ),
            }
        )
    else:
        current.extend(valid_data)
    return True, current, invalid


def relocation_target_binding_staleness(
    value: Mapping[str, Any],
    *,
    document: ProgressDocument,
    bindings: Mapping[str, Sequence[Any]],
    target_symbol_id: str,
    reference: Path = DEFAULT_REFERENCE,
) -> tuple[dict[str, Any], list[dict[str, Any]]]:
    """Validate a target binding against current retail, tracker, owner, and manifest facts."""
    normalized = normalize_relocation_target_binding(value)
    context = normalized["binding_context"]
    differences: list[dict[str, Any]] = []
    source_stored = context["source_binding"]
    try:
        source_current = build_object_binding_snapshot(
            document,
            bindings,
            symbol_id=str(source_stored["symbol_id"]),
            object_symbol=str(source_stored["object_symbol"]),
        )
    except RelocationExpectationError as exc:
        source_current = None
        differences.append(
            {
                "field": "source_binding",
                "reason": "current-source-binding-invalid",
                "stored": source_stored,
                "current": None,
                "message": str(exc),
            }
        )
    if source_current is not None and source_current != source_stored:
        differences.append(
            {
                "field": "source_binding",
                "reason": "source-binding-drift",
                "stored": source_stored,
                "current": source_current,
            }
        )
    source_row = document.collection("symbols").get(str(source_stored["symbol_id"]))
    if isinstance(source_row, Mapping):
        try:
            relocation_current = decode_retail_relocation_at_offset(
                row=source_row,
                offset=int(context["relocation"]["offset"]),
                reference=reference,
            )
            relocation_current = {
                field: relocation_current[field]
                for field in (
                    "offset",
                    "type",
                    "type_name",
                    "retail_target",
                    "instruction_offset",
                    "opcode",
                )
            }
        except (RelocationExpectationError, OSError, ValueError) as exc:
            relocation_current = None
            differences.append(
                {
                    "field": "relocation",
                    "reason": "immutable-retail-decode-failed",
                    "stored": context["relocation"],
                    "current": None,
                    "message": str(exc),
                }
            )
        if relocation_current is not None and relocation_current != context["relocation"]:
            differences.append(
                {
                    "field": "relocation",
                    "reason": "immutable-retail-relocation-drift",
                    "stored": context["relocation"],
                    "current": relocation_current,
                }
            )

    target_row = document.collection("symbols").get(target_symbol_id)
    if not isinstance(target_row, Mapping):
        target_current = None
    else:
        try:
            target_current = relocation_target_row_context(
                symbol_id=target_symbol_id,
                row=target_row,
                object_symbol=str(normalized["object_symbol"]),
            )
        except RelocationExpectationError:
            target_current = None
    if target_current != context["target"]:
        differences.append(
            {
                "field": "target",
                "reason": "target-row-drift",
                "stored": context["target"],
                "current": target_current,
            }
        )

    owner_id = str(context["owner"]["owner_id"])
    owner = document.collection("owners").get(owner_id)
    if not isinstance(owner, Mapping):
        owner_current = None
    else:
        try:
            owner_current = relocation_target_owner_context(
                owner_id=owner_id,
                owner=owner,
                evidence_ids=context["owner"]["binding_evidence_ids"],
            )
        except RelocationExpectationError:
            owner_current = None
    if owner_current != context["owner"]:
        differences.append(
            {
                "field": "owner",
                "reason": "target-owner-drift",
                "stored": context["owner"],
                "current": owner_current,
            }
        )
    evidence = document.collection("evidence")
    missing_or_unscoped: list[str] = []
    if isinstance(owner, Mapping):
        owner_evidence = set(str(item) for item in owner.get("evidence_ids", ()))
    else:
        owner_evidence = set()
    for evidence_id in normalized["evidence_ids"]:
        evidence_row = evidence.get(evidence_id)
        scope_ids = (
            set(str(item) for item in evidence_row.get("scope_ids", ()))
            if isinstance(evidence_row, Mapping)
            else set()
        )
        if evidence_id not in owner_evidence or owner_id not in scope_ids:
            missing_or_unscoped.append(evidence_id)
    if missing_or_unscoped:
        differences.append(
            {
                "field": "evidence_ids",
                "reason": "missing-or-unscoped-target-owner-evidence",
                "stored": normalized["evidence_ids"],
                "current": sorted(set(normalized["evidence_ids"]) - set(missing_or_unscoped)),
                "missing_or_unscoped": sorted(missing_or_unscoped),
            }
        )
    relationship = context["relationship"]
    relationships = owner.get("relationships", ()) if isinstance(owner, Mapping) else ()
    target_context = context["target"]
    current_relationships: list[dict[str, Any]] = []
    invalid_relationships: list[dict[str, Any]] = []
    for item in relationships:
        if not isinstance(item, Mapping):
            continue
        try:
            item_address = normalize_address(item.get("address"))
        except (TypeError, ValueError):
            item_address = None
        if (
            item.get("symbol_id") != target_context["symbol_id"]
            or item_address != target_context["address"]
        ):
            continue
        try:
            normalized_item = _normalize_target_relationship(item, target=target_context)
        except RelocationExpectationError as exc:
            invalid_relationships.append(
                {"relationship": dict(item), "message": str(exc)}
            )
        else:
            if normalized_item is not None:
                current_relationships.append(normalized_item)
    relationship_drift = False
    relationship_reason = "target-owner-relationship-drift"
    provider_pair_applicable = False
    if relationship is None and isinstance(owner, Mapping):
        provider_pair_applicable, provider_current, provider_invalid = (
            _provider_null_relationship_state(
                document=document,
                owner=owner,
                target=target_context,
            )
        )
        if provider_pair_applicable:
            current_relationships = provider_current
            invalid_relationships = provider_invalid
            relationship_drift = bool(provider_invalid)
            relationship_reason = "provider-target-owner-relationship-invalid"
    if relationship is None:
        if not provider_pair_applicable:
            relationship_drift = bool(current_relationships or invalid_relationships)
    elif invalid_relationships:
        relationship_drift = True
        relationship_reason = "target-owner-relationship-invalid"
    elif len(current_relationships) != 1:
        relationship_drift = True
        relationship_reason = (
            "target-owner-relationship-duplicate"
            if len(current_relationships) > 1
            else "target-owner-relationship-drift"
        )
    elif current_relationships[0] != relationship:
        relationship_drift = True
    if relationship_drift:
        differences.append(
            {
                "field": "relationship",
                "reason": relationship_reason,
                "stored": relationship,
                "current": current_relationships,
                "invalid": invalid_relationships,
            }
        )
    return normalized, differences


def build_target_identity_state(
    document: ProgressDocument,
    bindings: Mapping[str, Sequence[Any]],
    *,
    reference: Path = DEFAULT_REFERENCE,
) -> tuple[tuple[TargetIdentity, ...], list[dict[str, Any]]]:
    """Build usable target identities and typed blockers for stale governed bindings."""
    identities: list[TargetIdentity] = []
    blockers: list[dict[str, Any]] = []
    for symbol_id, row in document.collection("symbols").items():
        if not isinstance(row, Mapping) or row.get("binary") != "recoil":
            continue
        address_text = row.get("address")
        if not isinstance(address_text, str):
            continue
        try:
            address = address_value(address_text)
        except ValueError:
            continue
        end_exclusive: int | None = None
        if isinstance(row.get("end_exclusive"), str):
            try:
                end_exclusive = address_value(str(row["end_exclusive"]))
            except ValueError:
                end_exclusive = None
        object_symbols = {
            str(item.function.symbol)
            for item in bindings.get(str(symbol_id), ())
            if isinstance(getattr(item.function, "symbol", None), str)
            and str(item.function.symbol)
        }
        source_parts: list[str] = []
        if object_symbols:
            source_parts.append("registered-vc5-target")
        provider = row.get("linked_provider_binding")
        provider_complete = (
            isinstance(provider, Mapping)
            and provider.get("symbol_id") == str(symbol_id)
            and all(
                isinstance(provider.get(field), str)
                for field in ("map_symbol", "object", "provider", "archive_member")
            )
            and isinstance(provider.get("operands"), list)
        )
        if provider_complete:
            assert isinstance(provider, Mapping)
            object_symbols.add(str(provider["map_symbol"]))
            source_parts.append("linked-provider-binding")
        for index, item in enumerate(_accepted_relocation_target_bindings(row)):
            # Compatibility for pre-governance reviewed target rows. New mutations
            # always write binding_context and therefore receive live staleness checks.
            if not isinstance(item.get("binding_context"), Mapping):
                legacy_object = item.get("object_symbol")
                legacy_reason = item.get("reason")
                legacy_evidence = item.get("evidence_ids")
                if (
                    isinstance(legacy_object, str)
                    and legacy_object
                    and isinstance(legacy_reason, str)
                    and legacy_reason.strip()
                    and isinstance(legacy_evidence, list)
                    and legacy_evidence
                    and all(
                        isinstance(evidence_id, str) and evidence_id
                        for evidence_id in legacy_evidence
                    )
                ):
                    object_symbols.add(legacy_object)
                    source_parts.append("legacy-reviewed-relocation-target-binding")
                    continue
            try:
                normalized, stale = relocation_target_binding_staleness(
                    item,
                    document=document,
                    bindings=bindings,
                    target_symbol_id=str(symbol_id),
                    reference=reference,
                )
            except (RelocationExpectationError, OSError, ValueError) as exc:
                raw_context = item.get("binding_context")
                raw_source = (
                    raw_context.get("source_binding")
                    if isinstance(raw_context, Mapping)
                    else None
                )
                blockers.append(
                    {
                        "kind": "invalid-relocation-target-binding",
                        "target_symbol_id": str(symbol_id),
                        "source_symbol_id": (
                            str(raw_source.get("symbol_id", ""))
                            if isinstance(raw_source, Mapping)
                            else ""
                        ),
                        "binding_index": index,
                        "message": str(exc),
                    }
                )
                continue
            if stale:
                blockers.append(
                    {
                        "kind": "stale-relocation-target-binding",
                        "target_symbol_id": str(symbol_id),
                        "source_symbol_id": str(
                            normalized["binding_context"]["source_binding"]["symbol_id"]
                        ),
                        "binding_index": index,
                        "object_symbol": normalized["object_symbol"],
                        "message": "reviewed relocation target binding no longer matches current facts",
                        "stale_fields": stale,
                    }
                )
                continue
            object_symbols.add(str(normalized["object_symbol"]))
            source_parts.append("reviewed-relocation-target-binding")
        if object_symbols:
            identities.append(
                TargetIdentity(
                    symbol_id=str(symbol_id),
                    address=address,
                    end_exclusive=end_exclusive,
                    object_symbols=tuple(sorted(object_symbols)),
                    source="+".join(sorted(set(source_parts))),
                )
            )
    return tuple(identities), blockers


def _resolve_identity(
    target: int,
    identities: Sequence[TargetIdentity],
) -> tuple[TargetIdentity | None, int, list[dict[str, Any]]]:
    candidates: list[tuple[TargetIdentity, int]] = []
    for identity in identities:
        if target == identity.address:
            candidates.append((identity, 0))
        elif identity.end_exclusive is not None and identity.address < target < identity.end_exclusive:
            candidates.append((identity, target - identity.address))
    projections = [
        {
            "symbol_id": identity.symbol_id,
            "address": f"0x{identity.address:x}",
            "end_exclusive": (
                f"0x{identity.end_exclusive:x}" if identity.end_exclusive is not None else None
            ),
            "object_symbols": list(identity.object_symbols),
            "target_addend": addend,
            "source": identity.source,
        }
        for identity, addend in candidates
    ]
    if not candidates:
        return None, 0, projections
    unique = {
        (identity.object_symbols, addend)
        for identity, addend in candidates
    }
    if len(unique) != 1 or len(candidates[0][0].object_symbols) != 1:
        return None, 0, projections
    return candidates[0][0], candidates[0][1], projections


def _exception_by_site(
    exceptions: Sequence[Mapping[str, Any]],
    *,
    document: ProgressDocument,
    row: Mapping[str, Any],
    bindings: Mapping[str, Sequence[Any]],
    reference: Path = DEFAULT_REFERENCE,
) -> tuple[dict[tuple[int, int], Mapping[str, Any]], list[dict[str, Any]]]:
    result: dict[tuple[int, int], Mapping[str, Any]] = {}
    unresolved: list[dict[str, Any]] = []
    normalized_rows: list[tuple[int, dict[str, Any], tuple[int, int]]] = []
    for index, item in enumerate(exceptions):
        try:
            normalized = normalize_reviewed_exception(item)
            keys = _exception_sites(normalized)
        except RelocationExpectationError as exc:
            unresolved.append(
                {
                    "kind": "invalid-reviewed-exception",
                    "exception_index": index,
                    "message": str(exc),
                }
            )
            continue
        normalized_rows.extend((index, normalized, key) for key in keys)

    by_site: dict[tuple[int, int], dict[str, Any]] = {}
    for index, normalized, key in normalized_rows:
        if key in by_site and by_site[key] != normalized:
            unresolved.append(
                {
                    "kind": "conflicting-reviewed-exception",
                    "offset": key[0],
                    "type": key[1],
                    "message": "multiple reviewed relocation exceptions disagree for one operand",
                }
            )
            continue
        by_site[key] = normalized

    scope_ids = {str(value) for value in row.get("scope_ids", ())}
    if not scope_ids and isinstance(row.get("symbol_id"), str):
        scope_ids.add(str(row["symbol_id"]))
    for index, normalized, key in normalized_rows:
        if by_site.get(key) != normalized or key in result:
            continue
        if normalized.get("legacy_reviewed_catalog") is not True:
            source_binding = normalized.get("source_binding")
            source_symbol_id = (
                str(source_binding.get("symbol_id", ""))
                if isinstance(source_binding, Mapping)
                else ""
            )
            if source_symbol_id not in scope_ids:
                unresolved.append(
                    {
                        "kind": "stale-reviewed-exception",
                        "exception_index": index,
                        "offset": key[0],
                        "type": key[1],
                        "message": "reviewed exception source symbol is not in the current physical group",
                        "stale_fields": [
                            {
                                "field": "source_binding.symbol_id",
                                "stored": source_symbol_id,
                                "current": sorted(scope_ids),
                            }
                        ],
                    }
                )
                continue
        try:
            fresh, differences = reviewed_exception_staleness(
                normalized,
                document=document,
                bindings=bindings,
                reference=reference,
            )
        except RelocationExpectationError as exc:
            unresolved.append(
                {
                    "kind": "invalid-reviewed-exception",
                    "exception_index": index,
                    "offset": key[0],
                    "type": key[1],
                    "message": str(exc),
                }
            )
            continue
        if differences:
            unresolved.append(
                {
                    "kind": "stale-reviewed-exception",
                    "exception_index": index,
                    "offset": key[0],
                    "type": key[1],
                    "message": "reviewed exception no longer matches current tracker identity facts",
                    "stale_fields": differences,
                }
            )
            continue
        result[key] = fresh
    return result, unresolved


def derive_relocation_expectations(
    *,
    document: ProgressDocument,
    row: Mapping[str, Any],
    object_symbol: str,
    bindings: Mapping[str, Sequence[Any]],
    reference: Path = DEFAULT_REFERENCE,
) -> dict[str, Any]:
    address = address_value(str(row["address"]))
    end_exclusive = address_value(str(row["end_exclusive"]))
    if end_exclusive <= address:
        raise RelocationExpectationError("retail function extent must be non-empty")
    image = reference.read_bytes()
    headers = parse_pe_headers(image, source=str(reference))
    retail_bytes = _pe_bytes(image, headers, address, end_exclusive - address)
    sites, decoder_unresolved = decode_x86_operand_sites(
        retail_bytes,
        function_address=address,
    )
    current_scope_ids = {str(value) for value in row.get("scope_ids", ())}
    if not current_scope_ids and isinstance(row.get("symbol_id"), str):
        current_scope_ids.add(str(row["symbol_id"]))
    current_symbol_id = "|".join(sorted(current_scope_ids)) or "current-retail-function"
    target_identities, target_binding_blockers = build_target_identity_state(
        document,
        bindings,
        reference=reference,
    )
    target_binding_blockers = [
        blocker
        for blocker in target_binding_blockers
        if blocker.get("source_symbol_id") in current_scope_ids
    ]
    identities = list(target_identities)
    identities.append(
        TargetIdentity(
            symbol_id=current_symbol_id,
            address=address,
            end_exclusive=end_exclusive,
            object_symbols=(object_symbol,),
            source="current-registered-object-symbol",
        )
    )
    exceptions = _reviewed_exceptions(row, object_symbol)
    exception_map, exception_unresolved = _exception_by_site(
        exceptions,
        document=document,
        row=row,
        bindings=bindings,
        reference=reference,
    )
    unresolved: list[dict[str, Any]] = (
        list(decoder_unresolved) + target_binding_blockers + exception_unresolved
    )
    expected: list[dict[str, Any]] = []
    used_exceptions: set[tuple[int, int]] = set()
    image_end = headers.image_base + headers.size_of_image
    for site in sites:
        operand = retail_bytes[site.offset : site.offset + 4]
        if len(operand) != 4:
            unresolved.append(
                {
                    "kind": "truncated-address-operand",
                    "offset": site.offset,
                    "type": site.relocation_type,
                }
            )
            continue
        if site.relocation_type == IMAGE_REL_I386_REL32:
            retail_target = address + site.offset + 4 + struct.unpack("<i", operand)[0]
            if address <= retail_target < end_exclusive:
                if site.opcode == "e8":
                    if retail_target != address:
                        unresolved.append(
                            {
                                "kind": "unsupported-internal-call-target",
                                "offset": site.offset,
                                "type": site.relocation_type,
                                "type_name": relocation_type_name(site.relocation_type),
                                "operand_kind": site.kind,
                                "instruction_offset": site.instruction_offset,
                                "opcode": site.opcode,
                                "retail_target": f"0x{retail_target:x}",
                                "message": (
                                    "retail call targets an interior address of the current "
                                    "function; a recursive VC5 COFF relocation is deterministic "
                                    "only when the call targets the registered function entry"
                                ),
                            }
                        )
                        continue
                    expected.append(
                        {
                            "object_symbol": object_symbol,
                            "offset": site.offset,
                            "type": site.relocation_type,
                            "type_name": relocation_type_name(site.relocation_type),
                            "target_symbol": object_symbol,
                            "target_symbol_id": current_symbol_id,
                            "coff_addend": 0,
                            "resolved_target_addend": 0,
                            "retail_target": retail_target,
                            "derivation": "current-registered-object-symbol",
                            "instruction_offset": site.instruction_offset,
                            "opcode": site.opcode,
                        }
                    )
                    continue
                continue
        else:
            retail_target = struct.unpack("<I", operand)[0]
            if site.kind == "potential-absolute32" and not (
                headers.image_base <= retail_target < image_end
            ):
                continue
        key = (site.offset, site.relocation_type)
        reviewed = exception_map.get(key)
        if reviewed is not None:
            used_exceptions.add(key)
            try:
                reviewed_target = _integer(
                    reviewed.get("retail_target"), field="reviewed retail target"
                )
                physical_mode = (
                    reviewed.get("exception_mode")
                    == PHYSICAL_TARGET_UNRESOLVED_VC5_TEMPORARY
                )
                if physical_mode:
                    target_symbol_id = str(reviewed.get("target_symbol_id", ""))
                    target_symbol = _PHYSICAL_TARGET_TOKEN_PREFIX + target_symbol_id
                    if not target_symbol_id:
                        raise RelocationExpectationError(
                            "reviewed physical target_symbol_id must be non-empty"
                        )
                else:
                    target_symbol = str(reviewed.get("target_symbol", ""))
                    if not target_symbol:
                        raise RelocationExpectationError(
                            "reviewed target_symbol must be non-empty"
                        )
                target_addend = _integer(
                    reviewed.get("resolved_target_addend", 0),
                    field="reviewed resolved target addend",
                )
                coff_addend = _integer(
                    reviewed.get("coff_addend"), field="reviewed COFF addend"
                )
                if reviewed_target != retail_target:
                    raise RelocationExpectationError(
                        f"reviewed retail target 0x{reviewed_target:x} != decoded 0x{retail_target:x}"
                    )
            except RelocationExpectationError as exc:
                unresolved.append(
                    {
                        "kind": "invalid-reviewed-exception",
                        "offset": site.offset,
                        "type": site.relocation_type,
                        "message": str(exc),
                    }
                )
                continue
            expected_row = {
                    "object_symbol": object_symbol,
                    "offset": site.offset,
                    "type": site.relocation_type,
                    "type_name": relocation_type_name(site.relocation_type),
                    "target_symbol": target_symbol,
                    "coff_addend": coff_addend,
                    "resolved_target_addend": target_addend,
                    "retail_target": retail_target,
                    "derivation": (
                        "legacy-reviewed-catalog"
                        if reviewed.get("legacy_reviewed_catalog") is True
                        else "reviewed-exception"
                    ),
                    "instruction_offset": site.instruction_offset,
                    "opcode": site.opcode,
                }
            if physical_mode:
                expected_row.update(
                    target_symbol_id=target_symbol_id,
                    provenance_mode=PHYSICAL_TARGET_UNRESOLVED_VC5_TEMPORARY,
                    witness_site_offsets=list(reviewed["offsets"]),
                    physical_target_binding=dict(
                        reviewed["physical_target_binding"]
                    ),
                    witness_contract=dict(reviewed["witness_contract"]),
                )
            expected.append(expected_row)
            continue
        identity, target_addend, candidates = _resolve_identity(retail_target, identities)
        if identity is None:
            unresolved.append(
                {
                    "kind": (
                        "ambiguous-target-identity" if candidates else "missing-target-identity"
                    ),
                    "offset": site.offset,
                    "type": site.relocation_type,
                    "type_name": relocation_type_name(site.relocation_type),
                    "operand_kind": site.kind,
                    "instruction_offset": site.instruction_offset,
                    "opcode": site.opcode,
                    "retail_target": f"0x{retail_target:x}",
                    "candidate_identities": candidates,
                    "message": (
                        "retail operand does not resolve to exactly one accepted typed target and "
                        "one exact object symbol; add or correct a governed target/provider/alias "
                        "binding, or a reviewed relocation exception"
                    ),
                }
            )
            continue
        target_symbol = identity.object_symbols[0]
        if site.relocation_type == IMAGE_REL_I386_REL32:
            # IMAGE_REL_I386_REL32 applies S + A - (P + 4) when linked.  The
            # instruction decoder already used P + 4 to recover retail_target,
            # so the raw COFF field carries only the symbol-relative addend A.
            coff_addend = target_addend & 0xFFFFFFFF
        elif site.relocation_type == IMAGE_REL_I386_DIR32:
            coff_addend = target_addend & 0xFFFFFFFF
        else:
            unresolved.append(
                {
                    "kind": "unsupported-relocation-type",
                    "offset": site.offset,
                    "type": site.relocation_type,
                }
            )
            continue
        expected.append(
            {
                "object_symbol": object_symbol,
                "offset": site.offset,
                "type": site.relocation_type,
                "type_name": relocation_type_name(site.relocation_type),
                "target_symbol": target_symbol,
                "target_symbol_id": identity.symbol_id,
                "coff_addend": coff_addend,
                "resolved_target_addend": target_addend,
                "retail_target": retail_target,
                "derivation": identity.source,
                "instruction_offset": site.instruction_offset,
                "opcode": site.opcode,
            }
        )
    for key, item in exception_map.items():
        if key not in used_exceptions:
            unresolved.append(
                {
                    "kind": "reviewed-exception-site-not-decoded",
                    "offset": key[0],
                    "type": key[1],
                    "target_symbol": item.get("target_symbol"),
                    "message": "the reviewed exception does not match a decoded retail address operand",
                }
            )
    expected.sort(key=lambda item: (int(item["offset"]), int(item["type"]), str(item["target_symbol"])))
    unresolved.sort(key=lambda item: (int(item.get("offset", -1)), str(item.get("kind", ""))))
    return {
        "report_version": 1,
        "kind": "retail-relocation-expectations",
        "validation_mode": "live-retail-derived",
        "candidate_independent": True,
        "reference": display_path(reference),
        "address": normalize_address(row["address"]),
        "end_exclusive": normalize_address(row["end_exclusive"]),
        "scope_ids": sorted(current_scope_ids),
        "object_symbol": object_symbol,
        "status": "complete" if not unresolved else "unresolved",
        "passed": not unresolved,
        "explicit_empty": not expected and not unresolved,
        "expectations": expected,
        "unresolved": unresolved,
        "reviewed_exception_count": len(exceptions),
        "legacy_reviewed_catalog_count": sum(
            1
            for item in exception_map.values()
            if item.get("legacy_reviewed_catalog") is True
        ),
    }


def audit_at(
    *,
    document: ProgressDocument,
    at: str,
    reference: Path,
    manifest_dir: Path,
) -> dict[str, Any]:
    # Imported lazily to keep the pure derivation module independent of candidate build code.
    from _recoil.commands.live_byte_verify import _bindings, _rows, _select_bindings

    try:
        rows = _rows(document, "authored", at)
        bindings = _bindings(document, manifest_dir)
    except RuntimeError as exc:
        raise RelocationExpectationError(str(exc)) from exc
    reports: list[dict[str, Any]] = []
    seen: set[tuple[str, str]] = set()
    for row in rows:
        try:
            selected_bindings = _select_bindings(bindings, row)
        except RuntimeError as exc:
            raise RelocationExpectationError(str(exc)) from exc
        for binding in selected_bindings:
            key = (normalize_address(row["address"]), str(binding.function.symbol))
            if key in seen:
                continue
            seen.add(key)
            reports.append(
                derive_relocation_expectations(
                    document=document,
                    row=row,
                    object_symbol=str(binding.function.symbol),
                    bindings=bindings,
                    reference=reference,
                )
            )
    return {
        "report_version": 1,
        "kind": "relocation-expectations-audit",
        "validation_mode": "live-retail-derived",
        "candidate_independent": True,
        "tracker_revision": document.revision,
        "address": normalize_address(at),
        "passed": bool(reports) and all(report["passed"] for report in reports),
        "reports": reports,
    }


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Derive candidate-independent x86 COFF relocation expectations from retail bytes "
            "and accepted typed tracker bindings."
        )
    )
    parser.add_argument("--at", required=True)
    parser.add_argument("--progress", type=Path, default=DEFAULT_TRACKER)
    parser.add_argument("--reference", type=Path, default=DEFAULT_REFERENCE)
    parser.add_argument(
        "--manifest-dir", type=Path, default=REPO_ROOT / "tools" / "vc5_verify_targets"
    )
    parser.add_argument("--json", action="store_true")
    return parser


def run(args: argparse.Namespace) -> dict[str, Any]:
    document = ProgressDocument.load(args.progress)
    return audit_at(
        document=document,
        at=args.at,
        reference=args.reference,
        manifest_dir=args.manifest_dir,
    )


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    try:
        report = run(args)
    except (OSError, ValueError, RelocationExpectationError) as exc:
        print(f"relocation expectation audit error: {exc}", file=sys.stderr)
        return 2
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print(f"Relocation expectations at {report['address']}: {'PASS' if report['passed'] else 'BLOCKED'}")
        for item in report["reports"]:
            print(
                f"- {item['object_symbol']}: {item['status']}; "
                f"expectations={len(item['expectations'])}, unresolved={len(item['unresolved'])}"
            )
            for unresolved in item["unresolved"]:
                print("  - " + json.dumps(unresolved, sort_keys=True))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
