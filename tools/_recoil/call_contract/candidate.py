"""Recoil call-contract candidate evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import errors as _cc_errors
from _recoil.call_contract import listing as _cc_listing
from _recoil.call_contract import source as _cc_source

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        CandidateAssociatedSection,
        CandidateCoffSymbolDefinition,
        CandidateCompilerLocalFunctionDefinition,
        CandidateConstructorDefinition,
        CandidateDestructorDefinition,
        CandidateTuLocalFunctionDefinition,
        CandidateVftableDefinition,
    )

import os
import re
import struct
from collections import Counter
from copy import deepcopy
from dataclasses import replace
from pathlib import Path
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_DIR32,
    IMAGE_SCN_CNT_CODE,
    IMAGE_SYM_CLASS_EXTERNAL,
    IMAGE_SYM_CLASS_STATIC,
    CoffFunctionBytes,
    CoffObject,
    CoffRelocation,
    CoffSymbol,
    Instruction,
    relocation_size,
)
from _recoil.commands.vc5_verify import (
    build_compiler_receipt,
    compile_target,
    compile_translation_unit_order,
    compiler_env_path,
    compiler_receipt_stability,
    detect_compiler_version,
    load_manifest,
    prepare_clean_build_dir,
    require_clean_target_source_fragments,
    resolve_function_symbol_for_coff,
)
from _recoil.lib.progress import ProgressDocument, normalize_address
from _recoil.lib.tooling import REPO_ROOT


def _candidate_local_read_only_scalar_matches(definition: Any, data: bytes) -> bool:
    """Check storage/value facts without freezing movable COFF coordinates.

    The caller proves the relocation-selected name and its semantic role,
    uniqueness, relocation type/addend, operand use, and external exclusions.
    This predicate grants no retail identity or original source storage model.
    """
    return (
        definition.section_name == ".rdata"
        and definition.section_number > 0
        and definition.value >= 0
        and definition.symbol_type == 0
        and definition.storage_class == IMAGE_SYM_CLASS_STATIC
        and definition.aux_count == 0
        and bool(data)
        and definition.data == data
    )


def _candidate_compiler_local_scalar_matches(definition: Any, data: bytes) -> bool:
    """Additionally require a VC5-generated temporary scalar spelling."""
    return (
        re.fullmatch(r"\$T[0-9]+", definition.name) is not None
        and _candidate_local_read_only_scalar_matches(definition, data)
    )


def _exact_cod_inline_emitted_bsf_instructions(
    lines: Sequence[str],
    instructions: Sequence[Instruction],
) -> tuple[Instruction, ...]:
    """Recover exact register BSF instructions emitted as three VC5 DB rows.

    VC5's COD listing renders a source ``_emit`` sequence as one ``DB`` row
    per byte.  The general listing parser can consume the hexadecimal ``DB``
    token as another byte and lose the first address, which makes an exact
    short backedge to the emitted instruction appear unresolved.  Recover
    only a contiguous ``0f bc /r`` register form that is the encoded target of
    an exact named caller-local short branch.  Every other DB sequence remains
    untouched and therefore fails closed in the ordinary CFG proof.
    """

    emitted_rows: dict[int, int] = {}
    for line in lines:
        match = re.match(
            r"^\s*(?P<offset>[0-9A-Fa-f]{5,8})\s+"
            r"(?P<byte>[0-9A-Fa-f]{2})\s+DB\s+"
            r"(?P<value>-?(?:0x[0-9A-Fa-f]+|[0-9]+))\b",
            line,
            flags=re.IGNORECASE,
        )
        if match is None:
            continue
        offset = int(match.group("offset"), 16)
        byte = int(match.group("byte"), 16)
        try:
            rendered = int(match.group("value"), 0) & 0xFF
        except ValueError:
            continue
        if rendered != byte or offset in emitted_rows:
            continue
        emitted_rows[offset] = byte

    instruction_offsets = _cc_cfg._instruction_runtime_addresses(
        instructions,
        source="cod",
        caller_start=0,
    )
    exact_short_targets: set[int] = set()
    for offset, instruction in zip(instruction_offsets, instructions):
        if offset is None or not _cc_cfg._exact_cod_local_branch_label(
            _cc_cfg._instruction_operand(instruction)
        ):
            continue
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            continue
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        if (
            len(body) == 2
            and (
                (
                    0x70 <= body[0] <= 0x7F
                    and mnemonic in _cc_catalog.SHORT_JCC_MNEMONICS[body[0] - 0x70]
                )
                or (body[0] == 0xEB and mnemonic == "jmp")
            )
        ):
            exact_short_targets.add(
                int(offset) + 2 + struct.unpack("<b", body[1:2])[0]
            )

    register_names = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    recovered: dict[int, Instruction] = {}
    for start in sorted(exact_short_targets):
        body = tuple(emitted_rows.get(start + index) for index in range(3))
        if (
            body[0:2] != (0x0F, 0xBC)
            or body[2] is None
            or body[2] >> 6 != 3
        ):
            continue
        modrm = int(body[2])
        destination = register_names[(modrm >> 3) & 0x07]
        source_register = register_names[modrm & 0x07]
        encoded = ("0f", "bc", f"{modrm:02x}")
        rendered = f"bsf {destination}, {source_register}"
        recovered[start] = Instruction(
            text=rendered,
            raw_text=rendered,
            bytes=encoded,
            source_line=(
                f"{start:05x} {' '.join(encoded)} {rendered}"
            ),
        )
    if not recovered:
        return tuple(instructions)

    recovered_bytes = {
        offset
        for start in recovered
        for offset in range(start, start + 3)
    }
    retained = [
        instruction
        for offset, instruction in zip(instruction_offsets, instructions)
        if offset is None or int(offset) not in recovered_bytes
    ]
    combined = [*retained, *recovered.values()]
    combined_offsets = _cc_cfg._instruction_runtime_addresses(
        combined,
        source="cod",
        caller_start=0,
    )
    if (
        any(offset is None for offset in combined_offsets)
        or len(set(combined_offsets)) != len(combined_offsets)
    ):
        return tuple(instructions)
    return tuple(
        instruction
        for _offset, instruction in sorted(
            zip(combined_offsets, combined),
            key=lambda row: int(row[0]),
        )
    )


def _extract_cod_proc_with_local_switches(
    cod_path: Path,
    symbol_name: str,
    *,
    allow_named_switch_guard_labels: bool = False,
) -> CandidateAssembly:
    from _recoil.call_contract.records import CandidateAssembly
    lines = _cc_listing._extract_cod_proc_lines(
        cod_path,
        symbol_name,
        include_terminator=True,
    )
    instructions = _exact_cod_inline_emitted_bsf_instructions(
        lines,
        tuple(_cc_listing.parse_assembly("\n".join(lines), source="cod")),
    )
    local_control_flow_targets = _cc_cfg._cod_local_switch_targets(
        lines,
        instructions,
        allow_named_guard_labels=allow_named_switch_guard_labels,
    )
    return CandidateAssembly(
        instructions=instructions,
        local_control_flow_indices=frozenset(local_control_flow_targets),
        local_control_flow_targets=local_control_flow_targets,
    )


def _zsnd_reporter_manifest_attached_switch_authority(
    document: ProgressDocument,
    target: Any,
    address: str,
    compiled_entry: Any,
) -> bool:
    """Gate the one reviewed reporter switch that uses a named guard label."""

    if normalize_address(address) != _cc_catalog.ZSND_REPORT_A3D_ADDRESS:
        return False
    registered = document.collection("verification_targets").get(
        _cc_catalog.ZSND_REPORTER_TARGET_ID
    )
    registration = (
        registered.get("registration")
        if isinstance(registered, Mapping)
        else None
    )
    row = document.collection("symbols").get("recoil:function:0x4a3ef0")
    try:
        selected = _target_function(target, _cc_catalog.ZSND_REPORT_A3D_ADDRESS)
    except (RuntimeError, ValueError) as exc:
        raise ValueError(
            "zSnd reporter switch lacks its exact manifest function row"
        ) from exc
    manifest_description = str(getattr(target, "description", ""))
    compiled_functions = tuple(getattr(compiled_entry, "functions", ()))
    compiled_reporters = tuple(
        item
        for item in compiled_functions
        if normalize_address(str(getattr(item, "address", "")))
        == _cc_catalog.ZSND_REPORT_A3D_ADDRESS
    )
    if (
        getattr(target, "name", "") != _cc_catalog.ZSND_REPORTER_TARGET_NAME
        or getattr(target, "target_binary", "") != "recoil"
        or Path(str(getattr(target, "manifest_path", ""))).resolve()
        != _cc_catalog.ZSND_REPORTER_TARGET_MANIFEST.resolve()
        or getattr(target, "source_from", "") != _cc_catalog.ZSND_REPORTER_SOURCE_PATH
        or not bool(getattr(target, "check_translation_unit_function_order", False))
        or "57-entry .text switch table is attached compiler output"
        not in manifest_description
        or getattr(selected, "symbol", "") != _cc_catalog.ZSND_REPORT_A3D_SYMBOL
        or getattr(selected, "pipeline_class", "") != "authored"
        or getattr(selected, "authored_order_role", "") != "authored-body"
        or not bool(getattr(selected, "required_presence", False))
        or not bool(getattr(selected, "full_order_gate", False))
        or getattr(compiled_entry, "source_from", "")
        != _cc_catalog.ZSND_REPORTER_SOURCE_PATH
        or len(compiled_reporters) != 1
        or getattr(compiled_reporters[0], "symbol", "")
        != _cc_catalog.ZSND_REPORT_A3D_SYMBOL
        or not isinstance(registered, Mapping)
        or registered.get("binary") != "recoil"
        or registered.get("kind") != "vc5"
        or registered.get("name") != _cc_catalog.ZSND_REPORTER_TARGET_NAME
        or not isinstance(registration, Mapping)
        or registration.get("manifest_path")
        != _cc_catalog.ZSND_REPORTER_TARGET_MANIFEST.relative_to(REPO_ROOT).as_posix()
        or registration.get("source_from") != _cc_catalog.ZSND_REPORTER_SOURCE_PATH
        or registration.get("check_translation_unit_function_order") is not True
        or registration.get("function_order_scope") != "authored"
        or registration.get("function_addresses", []).count(
            _cc_catalog.ZSND_REPORT_A3D_ADDRESS
        )
        != 1
        or not isinstance(row, Mapping)
        or row.get("binary") != "recoil"
        or row.get("kind") != "function"
        or row.get("pipeline_class") != "authored"
        or row.get("address") != _cc_catalog.ZSND_REPORT_A3D_ADDRESS
        or row.get("end_exclusive") != "0x4a4330"
        or row.get("extent_state") != "known"
        or row.get("size") != 0x440
        or row.get("physical_block_id") != "recoil:block:0x4a3ea0"
    ):
        raise ValueError(
            "zSnd reporter switch requires the exact synchronized manifest, "
            "tracker caller, and attached-output declaration"
        )
    return True


def _require_zsnd_reporter_candidate_switch_coff(
    parsed: CandidateAssembly,
    caller_bytes: CoffFunctionBytes,
    coff_object: CoffObject,
) -> None:
    """Require the exact 57-entry candidate-local table before classifying it."""

    local_target_rows = tuple(parsed.local_control_flow_targets.items())
    if len(local_target_rows) != 1:
        raise ValueError(
            "zSnd reporter manifest-attached switch requires one exact "
            "57-entry structured target set"
        )
    dispatch_index, target_indices = local_target_rows[0]
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(parsed)
    dispatch_rows = [
        (index, instruction)
        for index, (instruction, offset) in enumerate(
            zip(parsed.instructions, offsets)
        )
        if offset == _cc_catalog.ZSND_REPORT_A3D_SWITCH_DISPATCH_OFFSET
    ]
    target_offsets = tuple(
        offsets[index] if 0 <= index < len(offsets) else None
        for index in target_indices
    )
    table_offsets = tuple(
        _cc_catalog.ZSND_REPORT_A3D_SWITCH_TABLE_OFFSET + index * 4
        for index in range(_cc_catalog.ZSND_REPORT_A3D_SWITCH_ENTRY_COUNT)
    )
    try:
        caller_section = coff_object.section(caller_bytes.section_index)
    except ValueError as exc:
        raise ValueError(
            "zSnd reporter switch rejects candidate COD/COFF dispatch, table, "
            "relocation, addend, or mask drift"
        ) from exc
    caller_start = caller_bytes.start
    caller_end = caller_bytes.end
    dispatch_relocation_offset = (
        caller_start
        + _cc_catalog.ZSND_REPORT_A3D_SWITCH_DISPATCH_OFFSET
        + 3
    )
    absolute_table_offsets = tuple(
        caller_start + offset for offset in table_offsets
    )
    dispatch_relocations = [
        relocation
        for relocation in caller_bytes.relocations
        if relocation.offset == dispatch_relocation_offset
    ]
    table_relocations = [
        relocation
        for relocation in caller_bytes.relocations
        if relocation.offset in absolute_table_offsets
    ]
    required_mask_offsets = {
        *range(
            _cc_catalog.ZSND_REPORT_A3D_SWITCH_DISPATCH_OFFSET + 3,
            _cc_catalog.ZSND_REPORT_A3D_SWITCH_DISPATCH_OFFSET + 7,
        ),
        *(
            byte_offset
            for offset in table_offsets
            for byte_offset in range(offset, offset + 4)
        ),
    }
    symbols_by_index: dict[int, list[CoffSymbol]] = {}
    symbols_by_name: dict[str, list[CoffSymbol]] = {}
    for symbol in coff_object.symbols:
        symbols_by_index.setdefault(symbol.index, []).append(symbol)
        symbols_by_name.setdefault(symbol.name, []).append(symbol)

    def exact_local_label(
        relocation: CoffRelocation,
    ) -> CoffSymbol | None:
        indexed = symbols_by_index.get(relocation.symbol_index, [])
        named = symbols_by_name.get(relocation.symbol_name, [])
        if (
            len(indexed) != 1
            or len(named) != 1
            or indexed[0] is not named[0]
        ):
            return None
        symbol = indexed[0]
        if (
            symbol.name != relocation.symbol_name
            or re.fullmatch(r"\$L[0-9A-Za-z_]+", symbol.name) is None
            or symbol.section_number != caller_bytes.section_index
            or symbol.type != 0
            or symbol.storage_class != 6
            or symbol.aux_count != 0
            or symbol.weak_external_tag_index is not None
            or symbol.weak_external_characteristics is not None
            or symbol.value < caller_start
            or symbol.value >= caller_end
        ):
            return None
        return symbol

    dispatch_target = (
        exact_local_label(dispatch_relocations[0])
        if len(dispatch_relocations) == 1
        else None
    )
    dispatch_operand_target = (
        re.search(
            r"(\$L[0-9A-Za-z_]+)\s*\[\s*eax\s*\*\s*4\s*\]\s*$",
            _cc_cfg._instruction_operand(dispatch_rows[0][1]).strip(),
            flags=re.IGNORECASE,
        )
        if len(dispatch_rows) == 1
        else None
    )
    table_target_value = (
        caller_start + _cc_catalog.ZSND_REPORT_A3D_SWITCH_TABLE_OFFSET
    )
    symbols_at_table = [
        symbol
        for symbol in coff_object.symbols
        if symbol.section_number == caller_bytes.section_index
        and symbol.value == table_target_value
    ]
    caller_symbols = [
        symbol
        for symbol in coff_object.symbols
        if symbol.name == caller_bytes.symbol
    ]
    table_targets = [
        exact_local_label(relocation) for relocation in table_relocations
    ]
    table_target_offsets = tuple(
        target.value - caller_start if target is not None else None
        for target in table_targets
    )
    if (
        caller_bytes.symbol != _cc_catalog.ZSND_REPORT_A3D_SYMBOL
        or len(caller_bytes.data) != _cc_catalog.ZSND_REPORT_A3D_CANDIDATE_BODY_SIZE
        or caller_bytes.section_index <= 0
        or caller_start < 0
        or caller_end - caller_start != _cc_catalog.ZSND_REPORT_A3D_CANDIDATE_BODY_SIZE
        or caller_bytes.natural_end != caller_end
        or caller_section.index != caller_bytes.section_index
        or caller_section.name != ".text"
        or len(caller_section.raw_data) < caller_end
        or not (caller_section.characteristics & IMAGE_SCN_CNT_CODE)
        or len(caller_symbols) != 1
        or caller_symbols[0].section_number != caller_bytes.section_index
        or caller_symbols[0].value != caller_start
        or caller_symbols[0].type != 0x20
        or caller_symbols[0].storage_class != IMAGE_SYM_CLASS_EXTERNAL
        or caller_symbols[0].aux_count != 0
        or caller_symbols[0].weak_external_tag_index is not None
        or caller_symbols[0].weak_external_characteristics is not None
        or len(dispatch_rows) != 1
        or dispatch_rows[0][0] != dispatch_index
        or parsed.local_control_flow_indices != frozenset({dispatch_index})
        or len(target_indices) != _cc_catalog.ZSND_REPORT_A3D_SWITCH_ENTRY_COUNT
        or target_offsets != _cc_catalog.ZSND_REPORT_A3D_SWITCH_TARGET_OFFSETS
        or bytes(int(value, 16) for value in dispatch_rows[0][1].bytes)
        != b"\xff\x24\x85\0\0\0\0"
        or len(dispatch_relocations) != 1
        or dispatch_relocations[0].type != IMAGE_REL_I386_DIR32
        or dispatch_target is None
        or dispatch_operand_target is None
        or dispatch_operand_target.group(1) != dispatch_target.name
        or dispatch_target.value != table_target_value
        or len(symbols_at_table) != 1
        or symbols_at_table[0] is not dispatch_target
        or len(table_relocations) != _cc_catalog.ZSND_REPORT_A3D_SWITCH_ENTRY_COUNT
        or tuple(relocation.offset for relocation in table_relocations)
        != absolute_table_offsets
        or any(
            relocation.type != IMAGE_REL_I386_DIR32
            or target is None
            or target.value >= table_target_value
            for relocation, target in zip(table_relocations, table_targets)
        )
        or table_target_offsets != _cc_catalog.ZSND_REPORT_A3D_SWITCH_TARGET_OFFSETS
        or len(caller_bytes.relocation_mask) != len(caller_bytes.data)
        or any(caller_bytes.data[offset] != 0 for offset in required_mask_offsets)
        or any(
            not caller_bytes.relocation_mask[offset]
            for offset in required_mask_offsets
        )
    ):
        raise ValueError(
            "zSnd reporter switch rejects candidate COD/COFF dispatch, table, "
            "relocation, addend, or mask drift"
        )


def _candidate_associated_comdat_sections_index(
    coff_object: CoffObject,
) -> dict[int, tuple[CandidateAssociatedSection, ...]]:
    """Snapshot every associative COMDAT in one bounded object-wide pass."""
    from _recoil.call_contract.records import (
        CandidateAssociatedSection,
        CandidateAssociatedSectionSymbol,
    )

    symbols_by_section: dict[int, list[Any]] = {}
    for item in coff_object.symbols:
        symbols_by_section.setdefault(item.section_number, []).append(item)
    by_parent: dict[int, list[CandidateAssociatedSection]] = {}
    for associated in coff_object.sections:
        section_symbols = symbols_by_section.get(associated.index, ())
        associated_definitions = [
            item
            for item in section_symbols
            if item.section_definition_selection is not None
        ]
        associated_selections = {
            item.section_definition_selection
            for item in associated_definitions
        }
        associated_associations = {
            item.section_definition_association
            for item in associated_definitions
        }
        if associated_selections != {5} or len(associated_associations) != 1:
            continue
        association_section_index = next(iter(associated_associations))
        if (
            type(association_section_index) is not int
            or association_section_index <= 0
        ):
            raise ValueError(
                "Associated COMDAT has a malformed parent section index"
            )
        associated_relocations = tuple(
            coff_object.relocations_by_section.get(associated.index, ())
        )
        associated_relocation_mask = [False] * len(associated.raw_data)
        for relocation in associated_relocations:
            width = relocation_size(relocation.type)
            if (
                relocation.offset < 0
                or relocation.offset + width > len(associated.raw_data)
            ):
                raise ValueError(
                    "Associated COMDAT relocation is outside section "
                    f"{associated.name}: 0x{relocation.offset:x}"
                )
            for index in range(relocation.offset, relocation.offset + width):
                associated_relocation_mask[index] = True
        associated_symbols = tuple(
            CandidateAssociatedSectionSymbol(
                name=item.name,
                value=item.value,
                section_number=item.section_number,
                type=item.type,
                storage_class=item.storage_class,
            )
            for item in section_symbols
            if item.section_definition_selection is None
        )
        associated_external_symbols = tuple(
            sorted(
                item.name
                for item in section_symbols
                if item.storage_class == IMAGE_SYM_CLASS_EXTERNAL
            )
        )
        by_parent.setdefault(association_section_index, []).append(
            CandidateAssociatedSection(
                section_index=associated.index,
                association_section_index=association_section_index,
                name=associated.name,
                data=associated.raw_data,
                relocations=associated_relocations,
                relocation_mask=tuple(associated_relocation_mask),
                section_size=len(associated.raw_data),
                section_is_comdat=bool(
                    associated.characteristics & _cc_catalog.IMAGE_SCN_LNK_COMDAT
                ),
                comdat_selection=5,
                section_external_symbols=associated_external_symbols,
                symbols=associated_symbols,
            )
        )
    return {parent: tuple(rows) for parent, rows in by_parent.items()}


def _candidate_external_symbol_inventory(
    coff_object: CoffObject,
) -> tuple[tuple[str, ...], tuple[str, ...], tuple[str, ...], tuple[str, ...]]:
    """Classify immutable TU-wide external symbols in one object pass."""

    undefined_functions: list[str] = []
    defined_functions: list[str] = []
    undefined_data: list[str] = []
    defined_data: list[str] = []
    for item in coff_object.symbols:
        if item.storage_class != IMAGE_SYM_CLASS_EXTERNAL:
            continue
        if item.type == 0x20:
            population = (
                defined_functions
                if item.section_number > 0
                else undefined_functions
            )
        elif item.type == 0:
            population = (
                defined_data if item.section_number > 0 else undefined_data
            )
        else:
            continue
        population.append(item.name)
    return (
        tuple(sorted(undefined_functions)),
        tuple(sorted(defined_functions)),
        tuple(sorted(undefined_data)),
        tuple(sorted(defined_data)),
    )


def _candidate_associated_comdat_sections(
    coff_object: CoffObject,
    association_section_index: int,
    *,
    sections_by_parent: Mapping[
        int, tuple[CandidateAssociatedSection, ...]
    ] | None = None,
) -> tuple[CandidateAssociatedSection, ...]:
    """Return one parent's immutable snapshot from the object-wide index."""

    index = (
        sections_by_parent
        if sections_by_parent is not None
        else _candidate_associated_comdat_sections_index(coff_object)
    )
    return tuple(index.get(association_section_index, ()))


def _candidate_complete_destructor_definitions(
    cod_path: Path,
    coff_object: CoffObject,
    *,
    associated_sections_by_parent: Mapping[
        int, tuple[CandidateAssociatedSection, ...]
    ] | None = None,
) -> dict[str, CandidateDestructorDefinition]:
    from _recoil.call_contract.records import CandidateDestructorDefinition
    associated_index = (
        associated_sections_by_parent
        if associated_sections_by_parent is not None
        else _candidate_associated_comdat_sections_index(coff_object)
    )
    definitions: dict[str, CandidateDestructorDefinition] = {}
    for symbol in coff_object.symbols:
        if (
            _cc_catalog.MSVC_COMPLETE_DESTRUCTOR_RE.fullmatch(symbol.name) is None
            or symbol.section_number <= 0
            or symbol.storage_class != IMAGE_SYM_CLASS_EXTERNAL
            or symbol.type != 0x20
        ):
            continue
        section = coff_object.section(symbol.section_number)
        section_definitions = [
            item
            for item in coff_object.symbols
            if item.section_number == symbol.section_number
            and item.section_definition_selection is not None
        ]
        selections = {
            item.section_definition_selection for item in section_definitions
        }
        comdat_selection = next(iter(selections)) if len(selections) == 1 else None
        external_functions = tuple(
            sorted(
                {
                    item.name
                    for item in coff_object.symbols
                    if item.section_number == symbol.section_number
                    and item.storage_class == IMAGE_SYM_CLASS_EXTERNAL
                    and item.type == 0x20
                }
            )
        )
        function = coff_object.function_bytes(symbol.name)
        parsed = _extract_cod_proc_with_local_switches(cod_path, symbol.name)
        associated_sections = associated_index.get(symbol.section_number, ())
        definitions[symbol.name] = CandidateDestructorDefinition(
            symbol=symbol.name,
            instructions=parsed.instructions,
            local_control_flow_indices=parsed.local_control_flow_indices,
            data=function.data,
            relocations=function.relocations,
            relocation_mask=function.relocation_mask,
            section_size=len(section.raw_data),
            section_is_comdat=bool(section.characteristics & _cc_catalog.IMAGE_SCN_LNK_COMDAT),
            comdat_selection=comdat_selection,
            section_external_functions=external_functions,
            local_control_flow_targets=parsed.local_control_flow_targets,
            associated_sections=associated_sections,
        )
    return definitions


def _candidate_vftable_definitions(
    coff_object: CoffObject,
) -> dict[str, CandidateVftableDefinition]:
    from _recoil.call_contract.records import CandidateVftableDefinition
    definitions: dict[str, CandidateVftableDefinition] = {}
    for symbol in coff_object.symbols:
        if (
            _cc_catalog.MSVC_VFTABLE_RE.fullmatch(symbol.name) is None
            or symbol.section_number <= 0
            or symbol.storage_class != IMAGE_SYM_CLASS_EXTERNAL
            or symbol.type != 0
        ):
            continue
        section = coff_object.section(symbol.section_number)
        section_definitions = [
            item
            for item in coff_object.symbols
            if item.section_number == symbol.section_number
            and item.section_definition_selection is not None
        ]
        selections = {
            item.section_definition_selection for item in section_definitions
        }
        comdat_selection = next(iter(selections)) if len(selections) == 1 else None
        external_symbols = tuple(
            sorted(
                item.name
                for item in coff_object.symbols
                if item.section_number == symbol.section_number
                and item.storage_class == IMAGE_SYM_CLASS_EXTERNAL
            )
        )
        data = coff_object.data_symbol_bytes(symbol.name)
        definitions[symbol.name] = CandidateVftableDefinition(
            symbol=symbol.name,
            data=data.data,
            relocations=data.relocations,
            relocation_mask=data.relocation_mask,
            section_name=section.name,
            section_size=len(section.raw_data),
            section_is_comdat=bool(section.characteristics & _cc_catalog.IMAGE_SCN_LNK_COMDAT),
            comdat_selection=comdat_selection,
            section_external_symbols=external_symbols,
        )
    return definitions


def _candidate_selected_constructor_definitions(
    cod_path: Path,
    coff_object: CoffObject,
    selected_symbols: set[str],
) -> dict[str, CandidateConstructorDefinition]:
    """Snapshot only an explicitly governed constructor symbol set.

    These candidate COMDATs are natural source-emission topology, not retail
    COMDAT provenance.  Keeping the population exact and caller-specific lets
    the exact bridges below inspect their base-constructor and vtable-stamp
    semantics without turning arbitrary candidate definitions into expected
    truth.
    """
    from _recoil.call_contract.records import CandidateConstructorDefinition
    definitions: dict[str, CandidateConstructorDefinition] = {}
    for symbol in coff_object.symbols:
        if (
            symbol.name not in selected_symbols
            or symbol.section_number <= 0
            or symbol.storage_class != IMAGE_SYM_CLASS_EXTERNAL
            or symbol.type != 0x20
        ):
            continue
        section = coff_object.section(symbol.section_number)
        section_definitions = [
            item
            for item in coff_object.symbols
            if item.section_number == symbol.section_number
            and item.section_definition_selection is not None
        ]
        selections = {
            item.section_definition_selection for item in section_definitions
        }
        comdat_selection = next(iter(selections)) if len(selections) == 1 else None
        external_functions = tuple(
            sorted(
                item.name
                for item in coff_object.symbols
                if item.section_number == symbol.section_number
                and item.storage_class == IMAGE_SYM_CLASS_EXTERNAL
                and item.type == 0x20
            )
        )
        function = coff_object.function_bytes(symbol.name)
        parsed = _extract_cod_proc_with_local_switches(cod_path, symbol.name)
        definitions[symbol.name] = CandidateConstructorDefinition(
            symbol=symbol.name,
            instructions=parsed.instructions,
            local_control_flow_indices=parsed.local_control_flow_indices,
            data=function.data,
            relocations=function.relocations,
            relocation_mask=function.relocation_mask,
            section_size=len(section.raw_data),
            section_is_comdat=bool(
                section.characteristics & _cc_catalog.IMAGE_SCN_LNK_COMDAT
            ),
            comdat_selection=comdat_selection,
            section_external_functions=external_functions,
            coff_symbols=_candidate_coff_symbol_definitions(coff_object),
            section_index=function.section_index,
            associated_sections=_candidate_associated_comdat_sections(
                coff_object, function.section_index
            ),
        )
    return definitions


def _candidate_reviewed_constructor_definitions(
    cod_path: Path,
    coff_object: CoffObject,
) -> dict[str, CandidateConstructorDefinition]:
    """Snapshot the governed caller-local child constructors."""
    selected_symbols = {
        _cc_catalog.HUD_NET_GAME_SETUP_LAUNCH_BUTTON_CONSTRUCTOR_SYMBOL,
        _cc_catalog.HUD_NET_GAME_SETUP_CANCEL_BUTTON_CONSTRUCTOR_SYMBOL,
        _cc_catalog.HUD_BRIEFING_RUNTIME_CONSTRUCTOR_SYMBOL,
        *(spec[1] for spec in _cc_catalog._CONSTRUCTOR_DISPATCH_OBLIGATIONS.values()),
    }
    return _candidate_selected_constructor_definitions(
        cod_path,
        coff_object,
        selected_symbols,
    )


def _candidate_unit_source_observation(
    receipts: Sequence[Mapping[str, Any]],
    *,
    target_name: str,
    source_from: str,
    build_dir: Path,
) -> Mapping[str, Any]:
    """Bind one invocation-local verifier/parent source compilation observation."""
    matches = []
    expected_build_dir = build_dir.resolve()
    for row in receipts:
        if row.get("target_id") != target_name or row.get("source_from") != source_from:
            continue
        parent = row.get("parent_receipt")
        observation = parent.get("post_observation") if isinstance(parent, Mapping) else None
        compiler = observation.get("compiler") if isinstance(observation, Mapping) else None
        observed_cwd = compiler.get("observed_cwd") if isinstance(compiler, Mapping) else None
        if not isinstance(observed_cwd, str) or not Path(observed_cwd).is_absolute():
            raise ValueError("COMDAT header provenance lacks an absolute TU compilation context")
        # Auxiliary definition acquisition can compile this same target/source
        # in another fresh directory. Only the context producing this COD may
        # supply its header population; duplicate observations there still fail.
        if Path(observed_cwd).resolve() == expected_build_dir:
            matches.append(row)
    if len(matches) != 1:
        raise ValueError("COMDAT header provenance requires one fresh TU observation")
    row = matches[0]
    verifier = row.get("verifier_receipt", {})
    parent = row.get("parent_receipt", {})
    observation = parent.get("post_observation", {})
    if (
        verifier.get("verification_eligible") is not True
        or parent.get("verification_eligible") is not True
        or verifier.get("post_observation") != parent.get("pre_observation")
        or parent.get("pre_observation") != observation
        or observation.get("verification_eligible") is not True
        or observation.get("source_from") != source_from
        or Path(observation.get("compiler", {}).get("observed_cwd", "")).resolve()
        != build_dir.resolve()
    ):
        raise ValueError("COMDAT header provenance rejects stale or conflicting TU observations")
    return observation


def _candidate_unit_header_source_files(receipts, *, target_name, source_from, build_dir):
    """Read only headers observed for this fresh TU; no provider authority."""
    observation = _candidate_unit_source_observation(receipts, target_name=target_name,
        source_from=source_from, build_dir=build_dir)
    headers = observation.get("toolchain", {}).get("header_inputs")
    if not isinstance(headers, list):
        raise ValueError("COMDAT header provenance lacks the observed header population")
    result: dict[str, tuple[str, ...]] = {}
    seen: set[str] = set()
    for header in headers:
        path_text = header.get("path") if isinstance(header, Mapping) else None
        if (
            not isinstance(path_text, str) or not Path(path_text).is_absolute()
            or header.get("role") not in {"project-header", "toolchain-header"}
            or not isinstance(header.get("physical_identity"), Mapping)
        ):
            raise ValueError("COMDAT header provenance has a malformed observed input")
        key = os.path.normcase(str(Path(path_text).resolve()))
        if key in seen:
            raise ValueError("COMDAT header provenance has duplicate observed inputs")
        seen.add(key)
        result[path_text] = tuple(Path(path_text).read_text(
            encoding="utf-8", errors="replace",
        ).splitlines())
    return result


def _candidate_current_source_path(provenance, observation, *, build_dir):
    """Map only the exact observed compiled input back to its current source."""
    if observation is None:
        return ""
    source = observation.get("source", {})
    source_from = observation.get("source_from")
    inputs = [row for row in observation.get("dependencies", []) if row.get("role") == "compiled-input"]
    if (not isinstance(source_from, str) or not source_from.startswith("src/")
            or len(inputs) != 1 or not isinstance(source.get("physical_identity"), Mapping)
            or not isinstance(inputs[0].get("physical_identity"), Mapping)
            or not Path(source.get("path", "")).is_absolute()
            or not Path(inputs[0].get("path", "")).is_absolute()
            or Path(source["path"]).resolve() != (REPO_ROOT / source_from).resolve()):
        raise ValueError("current source mapping requires exact authored/compiled input observations")
    emitted = Path(provenance)
    if not emitted.is_absolute():
        emitted = build_dir / emitted
    return source_from if emitted.resolve() == Path(inputs[0]["path"]).resolve() else ""


def _candidate_comdat_source_provenance(
    cod_path: Path | None,
    symbol_name: str,
    *,
    header_source_files: Mapping[str, Sequence[str]] | None = None,
) -> str:
    """Return the exact source file governing one COD COMDAT definition.

    VC5 usually leaves the originating ``; File`` row active through a COMDAT
    body.  It can, however, defer an instantiated header COMDAT until after it
    has switched the listing back to the primary translation unit.  In that
    shape, use the procedure's exact numbered source rows to rebind only when
    one previously listed source file or freshly observed TU header contains
    every row verbatim. Ambiguous
    exact matches fail closed; an unmatched body retains the active COD row so
    downstream canonical-header gates reject non-header provenance.
    """

    if cod_path is None:
        return ""
    lines = _cc_listing._read_cod_text(cod_path, errors="replace").splitlines()
    normalized_symbol = _cc_listing._vc5_compiler_normalized_cod_proc_symbol(symbol_name)
    expected_proc_symbols = {symbol_name}
    if normalized_symbol is not None:
        expected_proc_symbols.add(normalized_symbol)
    path_proc_re = re.compile(
        r"^\s*(\S(?:.*?\S)?)\s+PROC\b", re.IGNORECASE
    )
    proc_indexes = [
        index
        for index, line in enumerate(lines)
        if (
            (proc := path_proc_re.match(line)) is not None
            and proc.group(1) in expected_proc_symbols
        )
    ]
    if len(proc_indexes) != 1:
        raise ValueError(
            f"candidate COMDAT {symbol_name!r} lacks one exact COD procedure"
        )
    # Re-run the complete PROC/ENDP/body extraction at this producer/consumer
    # seam.  The caller has already parsed it for the candidate definition,
    # but provenance must never rebind from a looser label-only search if the
    # COD population changes or collides.
    proc_lines = _cc_listing._extract_cod_proc_lines(cod_path, symbol_name)
    file_rows = [
        line.split("File", 1)[1].strip()
        for line in lines[: proc_indexes[0]]
        if re.match(r"^\s*;\s*File\s+\S", line)
    ]
    title_rows = [
        match.group(1).strip()
        for line in lines[: proc_indexes[0]]
        if (match := re.match(r"^\s*TITLE\s+(.+)$", line, re.IGNORECASE))
    ]
    if len(title_rows) > 1:
        raise ValueError(f"candidate COMDAT {symbol_name!r} has ambiguous COD TITLE provenance")
    # TITLE establishes the primary source before VC5 emits its first File
    # marker. Early out-of-line empty bodies may contain only a closing brace,
    # which cannot uniquely identify a header by source-row content alone.
    if not file_rows and title_rows:
        file_rows = title_rows
    active_file = file_rows[-1] if file_rows else ""
    source_rows: list[tuple[int, str]] = []
    for line in proc_lines:
        source_row = re.match(r"^\s*;\s*(\d+)\s*:(.*)$", line)
        if source_row is None:
            continue
        source_text = source_row.group(2)
        # VC5 inserts one separator space after the colon before preserving
        # the original source text, including its indentation.
        if source_text.startswith(" "):
            source_text = source_text[1:]
        source_rows.append((int(source_row.group(1)), source_text))
    if not source_rows or not (file_rows or header_source_files):
        return active_file

    def matches_source_rows(source_path: str) -> bool:
        try:
            source_lines = (header_source_files or {}).get(source_path)
            if source_lines is None:
                source_lines = Path(source_path).read_text(
                    encoding="utf-8", errors="replace"
                ).splitlines()
        except (OSError, ValueError):
            return False
        return all(
            0 < line_number <= len(source_lines)
            and source_lines[line_number - 1] == source_text
            for line_number, source_text in source_rows
        )

    if active_file and matches_source_rows(active_file):
        return active_file

    exact_matches: dict[str, str] = {}
    for source_path in (*file_rows, *(header_source_files or {})):
        try:
            source_key = os.path.normcase(str(Path(source_path).resolve()))
        except (OSError, ValueError):
            source_key = os.path.normcase(source_path)
        if source_key in exact_matches or not matches_source_rows(source_path):
            continue
        exact_matches[source_key] = source_path
    if len(exact_matches) == 1:
        return next(iter(exact_matches.values()))
    if len(exact_matches) > 1:
        raise ValueError(
            "candidate COMDAT "
            f"{symbol_name!r} has ambiguous exact COD source-row provenance"
        )
    return active_file


def _candidate_tu_local_function_definitions(
    coff_object: CoffObject,
    cod_path: Path | None = None,
    *,
    header_source_files: Mapping[str, Sequence[str]] | None = None,
    source_observation: Mapping[str, Any] | None = None,
) -> dict[str, CandidateTuLocalFunctionDefinition]:
    """Snapshot complete external VC5 function COMDAT definitions.

    This is candidate evidence only.  Merely appearing here grants no retail
    identity; comparison-scoped arbitration below separately requires one
    reviewed physical non-authored provider row and an exact retail body.
    """
    from _recoil.call_contract.records import (
        CandidateAssembly,
        CandidateTuLocalFunctionDefinition,
    )
    definitions: dict[str, CandidateTuLocalFunctionDefinition] = {}
    external_function_names = {
        item.name
        for item in coff_object.symbols
        if item.storage_class == IMAGE_SYM_CLASS_EXTERNAL and item.type == 0x20
    }
    defined_names = [item.name for item in coff_object.symbols
                     if item.storage_class == IMAGE_SYM_CLASS_EXTERNAL and item.type == 0x20
                     and item.section_number > 0]
    if len(defined_names) != len(set(defined_names)):
        raise ValueError("candidate object has duplicate external function definitions")
    for symbol in coff_object.symbols:
        if (
            symbol.section_number <= 0
            or symbol.storage_class != IMAGE_SYM_CLASS_EXTERNAL
            or symbol.type != 0x20
        ):
            continue
        section = coff_object.section(symbol.section_number)
        function = coff_object.function_bytes(symbol.name)
        section_definitions = [
            item
            for item in coff_object.symbols
            if item.section_number == symbol.section_number
            and item.storage_class == IMAGE_SYM_CLASS_STATIC
            and item.name == section.name
            and item.section_definition_selection is not None
        ]
        selections = {
            item.section_definition_selection for item in section_definitions
        }
        comdat_selection = next(iter(selections)) if len(selections) == 1 else None
        if not (section.characteristics & _cc_catalog.IMAGE_SCN_LNK_COMDAT):
            continue
        symbol_end = getattr(coff_object, "symbol_end", None)
        natural_end = (
            symbol_end(symbol, section)
            if callable(symbol_end)
            else len(section.raw_data)
        )
        if (
            not section.raw_data
            or getattr(symbol, "value", 0) != 0
            or natural_end != len(section.raw_data)
        ):
            continue
        external_functions = tuple(
            sorted(
                item.name
                for item in coff_object.symbols
                if item.section_number == symbol.section_number
                and item.storage_class == IMAGE_SYM_CLASS_EXTERNAL
                and item.type == 0x20
            )
        )
        if external_functions != (symbol.name,):
            continue
        normalized_cod_symbol = _cc_listing._vc5_compiler_normalized_cod_proc_symbol(
            symbol.name
        )
        if (
            normalized_cod_symbol is not None
            and normalized_cod_symbol in external_function_names
        ):
            raise ValueError(
                "candidate anonymous COMDAT compiler-normalized COD procedure "
                f"{normalized_cod_symbol!r} collides with an exact COFF symbol"
            )
        parsed = (
            _extract_cod_proc_with_local_switches(cod_path, symbol.name)
            if cod_path is not None
            else CandidateAssembly((), frozenset())
        )
        provenance = _candidate_comdat_source_provenance(
            cod_path, symbol.name, header_source_files=header_source_files,
        )
        definitions[symbol.name] = CandidateTuLocalFunctionDefinition(
            symbol=symbol.name,
            data=function.data,
            relocations=function.relocations,
            relocation_mask=function.relocation_mask,
            section_size=len(section.raw_data),
            section_external_functions=external_functions,
            section_is_comdat=bool(
                section.characteristics & _cc_catalog.IMAGE_SCN_LNK_COMDAT
            ),
            comdat_selection=comdat_selection,
            instructions=parsed.instructions,
            local_control_flow_indices=parsed.local_control_flow_indices,
            local_control_flow_targets=parsed.local_control_flow_targets,
            source_provenance=provenance,
            current_source_path=_candidate_current_source_path(provenance, source_observation,
                build_dir=cod_path.parent) if cod_path is not None else "",
            object_path=str(coff_object.path) if getattr(coff_object, "path", None) is not None else "",
            symbol_index=symbol.index, section_number=symbol.section_number,
            symbol_value=symbol.value, storage_class=symbol.storage_class, symbol_type=symbol.type,
        )
    return definitions


def _candidate_compiler_local_function_definitions(
    coff_object: CoffObject,
    cod_path: Path,
) -> dict[str, CandidateCompilerLocalFunctionDefinition]:
    """Snapshot exact static compiler-local function COMDAT definitions."""
    from _recoil.call_contract.records import CandidateCompilerLocalFunctionDefinition

    definitions: dict[str, CandidateCompilerLocalFunctionDefinition] = {}
    name_counts = Counter(item.name for item in coff_object.symbols)
    for symbol in coff_object.symbols:
        if (
            symbol.section_number <= 0
            or symbol.storage_class != IMAGE_SYM_CLASS_STATIC
            or symbol.type != 0x20
            or symbol.aux_count != 0
            or name_counts[symbol.name] != 1
        ):
            continue
        section = coff_object.section(symbol.section_number)
        section_definitions = [
            item
            for item in coff_object.symbols
            if item.section_number == symbol.section_number
            and item.storage_class == IMAGE_SYM_CLASS_STATIC
            and item.name == section.name
            and item.section_definition_selection is not None
        ]
        selections = {
            item.section_definition_selection for item in section_definitions
        }
        function_symbols = tuple(
            sorted(
                item.name
                for item in coff_object.symbols
                if item.section_number == symbol.section_number
                and item.type == 0x20
            )
        )
        function = coff_object.function_bytes(symbol.name)
        if (
            not section.raw_data
            or symbol.value != 0
            or function.natural_end != len(section.raw_data)
            or function_symbols != (symbol.name,)
            or not (section.characteristics & _cc_catalog.IMAGE_SCN_LNK_COMDAT)
            or len(selections) != 1
        ):
            continue
        parsed = _extract_cod_proc_with_local_switches(cod_path, symbol.name)
        definitions[symbol.name] = CandidateCompilerLocalFunctionDefinition(
            symbol=symbol.name,
            data=function.data,
            relocations=function.relocations,
            relocation_mask=function.relocation_mask,
            section_size=len(section.raw_data),
            section_function_symbols=function_symbols,
            section_is_comdat=True,
            comdat_selection=next(iter(selections)),
            instructions=parsed.instructions,
            local_control_flow_indices=parsed.local_control_flow_indices,
            local_control_flow_targets=parsed.local_control_flow_targets,
            source_provenance=_candidate_comdat_source_provenance(
                cod_path, symbol.name
            ),
        )
    return definitions


def _candidate_coff_symbol_definitions(
    coff_object: CoffObject,
) -> tuple[CandidateCoffSymbolDefinition, ...]:
    """Snapshot exact primary COFF symbol-table facts for candidate calls."""
    from _recoil.call_contract.records import CandidateCoffSymbolDefinition

    definitions: list[CandidateCoffSymbolDefinition] = []
    for symbol in coff_object.symbols:
        section_name = ""
        section_size = 0
        section_characteristics = 0
        natural_end = 0
        section_data = b""
        section_relocations: tuple[CoffRelocation, ...] = ()
        section_snapshot_number = 0
        if symbol.section_number > 0:
            section = coff_object.section(symbol.section_number)
            section_name = section.name
            section_size = len(section.raw_data)
            section_characteristics = section.characteristics
            natural_end = coff_object.symbol_end(symbol, section)
            section_data = section.raw_data
            section_relocations = tuple(
                coff_object.relocations_by_section.get(section.index, ())
            )
            section_snapshot_number = section.index
        definitions.append(
            CandidateCoffSymbolDefinition(
                index=symbol.index,
                name=symbol.name,
                value=symbol.value,
                section_number=symbol.section_number,
                symbol_type=symbol.type,
                storage_class=symbol.storage_class,
                aux_count=symbol.aux_count,
                weak_external_tag_index=symbol.weak_external_tag_index,
                weak_external_characteristics=(
                    symbol.weak_external_characteristics
                ),
                section_name=section_name,
                section_size=section_size,
                section_characteristics=section_characteristics,
                natural_end=natural_end,
                section_data=section_data,
                section_relocations=section_relocations,
                section_snapshot_number=section_snapshot_number,
            )
        )
    return tuple(definitions)




def _target_function(target: Any, address: str) -> Any:
    matches: list[Any] = []
    for function in getattr(target, "functions", ()):
        if normalize_address(function.address) == address:
            matches.append(function)
    for entry in getattr(target, "translation_unit_function_order", ()):
        for function in entry.functions:
            if normalize_address(function.address) == address:
                matches.append(function)
    unique: dict[tuple[str, str, str | None], Any] = {
        (item.address, item.symbol, item.symbol_regex): item for item in matches
    }
    if len(unique) != 1:
        raise ValueError(
            f"{target.name}: address {address} must resolve to one candidate function; "
            f"found {len(unique)}"
        )
    return next(iter(unique.values()))


def _compile_hud_numeric_constructor_authority(
    *,
    build_root: Path,
    vc5_env: Path,
    precompiled: tuple[Any, tuple[tuple[Any, Path, CoffObject], ...]] | None = None,
    toolchain_receipts: list[dict[str, Any]] | None = None,
) -> tuple[
    Any,
    dict[str, CandidateConstructorDefinition],
    dict[str, CandidateConstructorDefinition],
    dict[str, CandidateConstructorDefinition],
]:
    """Acquire the current numeric, cycle-selector, and ZRD base definitions.

    The parameterized network text-input constructor is now a normal authored
    mission.cpp body. Its retired zui.cpp placement-new wrapper is not an
    acquisition prerequisite. These auxiliary snapshots remain candidate-only
    evidence; every consuming identity and constructor proof still runs.
    """
    if precompiled is None:
        target = load_manifest(
            REPO_ROOT / _cc_catalog.HUD_ZRD_WIDGET_CONSTRUCTOR_ORDER_TARGET_MANIFEST
        )
        require_clean_target_source_fragments(target)
        target_build_dir = prepare_clean_build_dir(
            build_root,
            "call-contract-hud-numeric-constructor-authority",
        )
        environment = compiler_env_path(target, vc5_env)
        compiled, rc = compile_translation_unit_order(
            target=target,
            build_dir=target_build_dir,
            compiler_env=environment,
            capture_verification_receipt=True,
        )
        if rc != 0:
            raise ValueError(
                "HUD numeric constructor authority compilation failed with "
                f"exit code {rc}"
            )
        units = tuple(
            (
                target.translation_unit_function_order[unit.manifest_index],
                unit.cod_path,
                CoffObject.from_path(unit.obj_path),
            )
            for unit in compiled
        )
        for unit in compiled:
            parent_receipt = _require_parent_toolchain_observation(
                target=target,
                compiled=unit,
                source_from=unit.source_from,
                manifest_index=unit.manifest_index,
                compiler_env=environment,
                build_dir=target_build_dir,
            )
            if toolchain_receipts is not None:
                toolchain_receipts.append({
                    "target_id": target.name,
                    "source_from": unit.source_from,
                    "manifest_index": unit.manifest_index,
                    "verifier_receipt": deepcopy(dict(unit.compiler_receipt)),
                    "parent_receipt": deepcopy(dict(parent_receipt)),
                })
    else:
        target, units = precompiled
    requested_by_source = {
        _cc_catalog.HUD_NET_GAME_SETUP_NUMERIC_CONSTRUCTOR_SOURCE_PATH: {
            _cc_catalog.HUD_NET_GAME_SETUP_NUMERIC_CONSTRUCTOR_SYMBOL,
            _cc_catalog.HUD_NET_GAME_SETUP_CYCLE_SELECTOR_CONSTRUCTOR_SYMBOL,
            _cc_catalog.HUD_CONFIRM_QUIT_BASE_CONSTRUCTOR_SYMBOL,
        },
    }
    definitions: dict[str, CandidateConstructorDefinition] = {}
    observed_sources: list[str] = []
    for entry, cod_path, coff_object in units:
        if entry is None:
            continue
        source_from = str(getattr(entry, "source_from", ""))
        selected_symbols = requested_by_source.get(source_from)
        if selected_symbols is None:
            continue
        observed_sources.append(source_from)
        extracted = _candidate_selected_constructor_definitions(
            cod_path,
            coff_object,
            selected_symbols,
        )
        if definitions.keys() & extracted.keys():
            raise ValueError(
                "HUD numeric constructor authority compilation produced "
                "duplicate reviewed definitions"
            )
        definitions.update(extracted)
    numeric_symbols = {
        _cc_catalog.HUD_NET_GAME_SETUP_NUMERIC_CONSTRUCTOR_SYMBOL,
    }
    cycle_symbols = {_cc_catalog.HUD_NET_GAME_SETUP_CYCLE_SELECTOR_CONSTRUCTOR_SYMBOL}
    zrd_symbols = {_cc_catalog.HUD_CONFIRM_QUIT_BASE_CONSTRUCTOR_SYMBOL}
    if (
        sorted(observed_sources) != sorted(requested_by_source)
        or set(definitions) != numeric_symbols | cycle_symbols | zrd_symbols
    ):
        raise ValueError(
            "HUD constructor authority compilation requires the exact current "
            "numeric/cycle/ZrdWidget base definitions in zui_widgets.cpp"
        )
    return (
        target,
        {name: definitions[name] for name in numeric_symbols},
        {name: definitions[name] for name in cycle_symbols},
        {name: definitions[name] for name in zrd_symbols},
    )


def _require_parent_toolchain_observation(
    *,
    target: Any,
    compiled: Any,
    source_from: str,
    manifest_index: int,
    compiler_env: Path,
    build_dir: Path,
) -> Mapping[str, Any]:
    """Independently reobserve and bind the verifier's stable compile receipt."""

    receipt = getattr(compiled, "compiler_receipt", None)
    if not isinstance(receipt, Mapping) or receipt.get("verification_eligible") is not True:
        raise ValueError("call-contract compilation lacks an eligible verification receipt")
    verifier_observation = receipt.get("post_observation")
    if not isinstance(verifier_observation, Mapping):
        raise ValueError("call-contract verification receipt lacks its post observation")
    parent_version = detect_compiler_version(compiler_env, cwd=build_dir)
    parent_observation = build_compiler_receipt(
        target=target,
        source_path=Path(compiled.source_path),
        source_from=source_from,
        manifest_index=manifest_index,
        compiler_env=compiler_env,
        build_dir=build_dir,
        compiler_version=parent_version,
    )
    parent_receipt = compiler_receipt_stability(
        verifier_observation,
        parent_observation,
    )
    if parent_receipt.get("verification_eligible") is not True:
        raise ValueError(
            "call-contract parent toolchain observation differs from verifier "
            "observation: "
            + "; ".join(str(row) for row in parent_receipt.get(
                "ineligibility_reasons", ()
            ))
        )
    return parent_receipt


def _compile_call_contract_target_units(
    target: Any,
    *,
    build_root: Path,
    vc5_env: Path,
    toolchain_receipts: list[dict[str, Any]] | None = None,
) -> tuple[tuple[Any, Path, CoffObject], ...]:
    """Compile one exact target once and retain every candidate TU artifact."""

    require_clean_target_source_fragments(target)
    target_build_dir = prepare_clean_build_dir(
        build_root, f"call-contract-{target.name}"
    )
    units: list[tuple[Any, Path, CoffObject]] = []
    if target.check_translation_unit_function_order:
        environment = compiler_env_path(target, vc5_env)
        compiled, rc = compile_translation_unit_order(
            target=target,
            build_dir=target_build_dir,
            compiler_env=environment,
            capture_verification_receipt=True,
        )
        if rc != 0:
            raise ValueError(
                f"{target.name}: VC5 call-contract compilation failed with exit code {rc}"
            )
        for unit in compiled:
            parent_receipt = _require_parent_toolchain_observation(
                target=target,
                compiled=unit,
                source_from=unit.source_from,
                manifest_index=unit.manifest_index,
                compiler_env=environment,
                build_dir=target_build_dir,
            )
            if toolchain_receipts is not None:
                toolchain_receipts.append({
                    "target_id": target.name,
                    "source_from": unit.source_from,
                    "manifest_index": unit.manifest_index,
                    "verifier_receipt": deepcopy(dict(unit.compiler_receipt)),
                    "parent_receipt": deepcopy(dict(parent_receipt)),
                })
            units.append(
                (
                    target.translation_unit_function_order[unit.manifest_index],
                    unit.cod_path,
                    CoffObject.from_path(unit.obj_path),
                )
            )
    else:
        compiled, rc = compile_target(
            target=target,
            build_dir=target_build_dir,
            vc5_env=vc5_env,
            capture_verification_receipt=True,
        )
        if rc != 0 or compiled is None:
            raise ValueError(
                f"{target.name}: VC5 call-contract compilation failed with exit code {rc or 3}"
            )
        parent_receipt = _require_parent_toolchain_observation(
            target=target,
            compiled=compiled,
            source_from=target.source_from,
            manifest_index=0,
            compiler_env=compiled.compiler_env,
            build_dir=target_build_dir,
        )
        if toolchain_receipts is not None:
            toolchain_receipts.append({
                "target_id": target.name,
                "source_from": target.source_from,
                "manifest_index": 0,
                "verifier_receipt": deepcopy(dict(compiled.compiler_receipt)),
                "parent_receipt": deepcopy(dict(parent_receipt)),
            })
        units.append(
            (None, compiled.cod_path, CoffObject.from_path(compiled.obj_path))
        )
    return tuple(units)


def _compile_slice_candidates(
    document: ProgressDocument,
    slice_row: Mapping[str, Any],
    *,
    build_root: Path,
    vc5_env: Path,
    preloaded_targets: Mapping[str, Any] | None = None,
    precompiled_target_units: Mapping[
        str, tuple[Any, tuple[tuple[Any, Path, CoffObject], ...]]
    ] | None = None,
    numeric_constructor_authority: tuple[
        Any,
        dict[str, CandidateConstructorDefinition],
        dict[str, CandidateConstructorDefinition],
        dict[str, CandidateConstructorDefinition],
    ] | None = None,
    toolchain_receipts: list[dict[str, Any]] | None = None,
) -> dict[str, CandidateAssembly]:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        CandidateCallerDefinition,
        CandidateLocalStaticDefinition,
    )
    candidates: dict[str, CandidateAssembly] = {}
    numeric_constructor_target: Any | None = None
    numeric_constructor_definitions: dict[
        str, CandidateConstructorDefinition
    ] = {}
    cycle_selector_constructor_definitions: dict[
        str, CandidateConstructorDefinition
    ] = {}
    zrd_widget_constructor_definitions: dict[
        str, CandidateConstructorDefinition
    ] = {}
    if _cc_catalog.HUD_NET_GAME_SETUP_CONSTRUCTOR_CALLER_START in set(
        slice_row["addresses"]
    ):
        authority = numeric_constructor_authority
        if authority is None:
            authority = _compile_hud_numeric_constructor_authority(
                build_root=build_root,
                vc5_env=vc5_env,
                toolchain_receipts=toolchain_receipts,
            )
        (
            numeric_constructor_target,
            numeric_constructor_definitions,
            cycle_selector_constructor_definitions,
            zrd_widget_constructor_definitions,
        ) = authority
    destructor_definitions_by_cod: dict[
        Path, dict[str, CandidateDestructorDefinition]
    ] = {}
    vftable_definitions_by_cod: dict[
        Path, dict[str, CandidateVftableDefinition]
    ] = {}
    tu_local_function_definitions_by_cod: dict[
        Path, dict[str, CandidateTuLocalFunctionDefinition]
    ] = {}
    compiler_local_function_definitions_by_cod: dict[
        Path, dict[str, CandidateCompilerLocalFunctionDefinition]
    ] = {}
    constructor_definitions_by_cod: dict[
        Path, dict[str, CandidateConstructorDefinition]
    ] = {}
    coff_symbol_definitions_by_cod: dict[
        Path, tuple[CandidateCoffSymbolDefinition, ...]
    ] = {}
    associated_sections_by_cod: dict[
        Path, dict[int, tuple[CandidateAssociatedSection, ...]]
    ] = {}
    external_symbol_inventory_by_cod: dict[
        Path, tuple[tuple[str, ...], tuple[str, ...], tuple[str, ...], tuple[str, ...]]
    ] = {}
    target_for_address: dict[str, str] = {}
    for symbol_id, address in zip(slice_row["symbol_ids"], slice_row["addresses"]):
        symbol = document.collection("symbols").get(symbol_id)
        if not isinstance(symbol, Mapping):
            raise ValueError(f"unknown call-contract symbol {symbol_id}")
        block = document.collection("physical_blocks").get(str(symbol.get("physical_block_id", "")))
        facts = block.get("accepted_order_facts") if isinstance(block, Mapping) else None
        if not isinstance(facts, Mapping):
            raise ValueError(f"call-contract symbol {symbol_id} has no accepted order facts")
        target_for_address[str(address)] = str(facts.get("target_id", ""))

    for target_id in slice_row["target_ids"]:
        target_row = document.collection("verification_targets").get(str(target_id))
        registration = target_row.get("registration") if isinstance(target_row, Mapping) else None
        manifest_path = (
            registration.get("manifest_path") if isinstance(registration, Mapping) else None
        )
        if not isinstance(manifest_path, str) or not manifest_path:
            raise ValueError(f"call-contract target {target_id} has no manifest path")
        target = (
            preloaded_targets.get(str(target_id))
            if preloaded_targets is not None
            else None
        )
        if target is None:
            target = load_manifest(REPO_ROOT / manifest_path)
        precompiled = (
            precompiled_target_units.get(str(target_id))
            if precompiled_target_units is not None
            else None
        )
        if precompiled is not None:
            precompiled_target = precompiled[0]
            if (
                precompiled_target is None
                or getattr(precompiled_target, "name", "")
                != getattr(target, "name", "")
                or getattr(precompiled_target, "target_binary", "")
                != getattr(target, "target_binary", "")
                or Path(str(getattr(
                    precompiled_target, "manifest_path", ""
                ))).resolve()
                != Path(str(getattr(target, "manifest_path", ""))).resolve()
                or getattr(precompiled_target, "source_from", "")
                != getattr(target, "source_from", "")
                or tuple(getattr(
                    precompiled_target, "order_edit_paths", ()
                )) != tuple(getattr(target, "order_edit_paths", ()))
                or bool(getattr(
                    precompiled_target,
                    "check_translation_unit_function_order",
                    False,
                ))
                != bool(getattr(
                    target, "check_translation_unit_function_order", False
                ))
            ):
                raise ValueError(
                    "precompiled call-contract target does not preserve the "
                    "registered target identity, binary, manifest, source, "
                    "or writable closure"
                )
            # The retained tuple owns the exact compile-profile variant that
            # produced its OBJ/COD.  Keep that profile attached to every
            # CandidateAssembly instead of silently falling back to the
            # canonical registered target loaded above.
            target = precompiled_target
        units = list(
            precompiled[1]
            if precompiled is not None
            else _compile_call_contract_target_units(
                target,
                build_root=build_root,
                vc5_env=vc5_env,
                toolchain_receipts=toolchain_receipts,
            )
        )

        selected_addresses = [
            address
            for address, selected_target in target_for_address.items()
            if selected_target == target_id
        ]
        symbol_by_address = {
            str(address): str(symbol_id)
            for symbol_id, address in zip(
                slice_row["symbol_ids"], slice_row["addresses"]
            )
        }
        acquisition_routes: list[dict[str, Any]] = []
        acquisition_closure: _cc_source.CallContractSourceClosure | None = None
        inline_absence_proofs: dict[str, dict[str, Any]] = {}
        for address in selected_addresses:
            function = _target_function(target, address)
            matching_units = [
                item
                for item in units
                if item[0] is None
                or any(
                    normalize_address(row.address) == address
                    for row in item[0].functions
                )
            ]
            if len(matching_units) != 1:
                blocked_route = _cc_source._candidate_body_compiled_unit_blocker_route(
                    target_id=str(target_id),
                    symbol_id=symbol_by_address.get(address, ""),
                    address=address,
                    expected_identity=str(function.symbol),
                    unit_count=len(matching_units),
                )
                if blocked_route is not None:
                    acquisition_routes.append(blocked_route)
                continue
            _entry, cod_path, coff_object = matching_units[0]
            if function.symbol_regex is not None:
                continue
            try:
                expected_symbol = resolve_function_symbol_for_coff(
                    coff_object, function
                )
                coff_object.function_bytes(expected_symbol)
                _extract_cod_proc_with_local_switches(
                    cod_path, expected_symbol
                )
            except (OSError, RuntimeError, ValueError):
                if acquisition_closure is None:
                    acquisition_closure = _cc_source.call_contract_source_closure(
                        document, slice_row
                    )
                absence_proof = (
                    _cc_source._prove_intentionally_inlined_empty_authored_candidate(
                        document,
                        acquisition_closure,
                        target_id=str(target_id),
                        selected_target=target,
                        compiled_entry=_entry,
                        symbol_id=symbol_by_address.get(address, ""),
                        address=address,
                        expected_identity=str(function.symbol),
                        cod_path=cod_path,
                        coff_object=coff_object,
                    )
                )
                if absence_proof is not None:
                    inline_absence_proofs[address] = absence_proof
                    continue
                route = _cc_source._candidate_body_declaration_header_route(
                    document,
                    acquisition_closure,
                    target_id=str(target_id),
                    selected_target=target,
                    compiled_entry=_entry,
                    symbol_id=symbol_by_address.get(address, ""),
                    address=address,
                    expected_identity=str(function.symbol),
                    cod_path=cod_path,
                    coff_object=coff_object,
                )
                if route is not None:
                    acquisition_routes.append(route)
        if acquisition_routes:
            raise _cc_errors.CandidateBodyIdentityAcquisitionError(
                target_id=str(target_id),
                routes=acquisition_routes,
            )
        for address in selected_addresses:
            function = _target_function(target, address)
            matching_units = [
                item
                for item in units
                if item[0] is None
                or any(
                    normalize_address(row.address) == address
                    for row in item[0].functions
                )
            ]
            if len(matching_units) != 1:
                raise ValueError(
                    f"{target.name}: {address} must resolve to one compiled translation unit"
                )
            _entry, cod_path, coff_object = matching_units[0]
            absence_proof = inline_absence_proofs.get(address)
            if absence_proof is not None:
                candidates[address] = CandidateAssembly(
                    instructions=(),
                    local_control_flow_indices=frozenset(),
                    target=target,
                    intentionally_inlined_absence_proof=absence_proof,
                )
                continue
            symbol_name = resolve_function_symbol_for_coff(coff_object, function)
            reporter_switch_authority = (
                _zsnd_reporter_manifest_attached_switch_authority(
                    document,
                    target,
                    address,
                    _entry,
                )
            )
            parsed = _extract_cod_proc_with_local_switches(
                cod_path,
                symbol_name,
                allow_named_switch_guard_labels=reporter_switch_authority,
            )
            caller_bytes = coff_object.function_bytes(symbol_name)
            exact_coff_targets = _cc_cfg._candidate_exact_coff_switch_targets(
                parsed, caller_bytes, coff_object,
            )
            parsed = _cc_cfg._compose_candidate_coff_switch_maps(parsed, exact_coff_targets)
            classification_only_targets = {
                **parsed.classification_only_local_control_flow_targets,
                **_cc_cfg._candidate_exact_local_switch_classification_targets(
                    _cc_listing._extract_cod_proc_lines(cod_path, symbol_name),
                    parsed,
                    caller_bytes,
                    coff_object,
                ),
            }
            classification_indices = (
                parsed.local_control_flow_indices
                | frozenset(classification_only_targets)
            )
            if classification_indices != parsed.local_control_flow_indices:
                parsed = replace(
                    parsed,
                    local_control_flow_indices=classification_indices,
                )
            if reporter_switch_authority:
                _require_zsnd_reporter_candidate_switch_coff(
                    parsed,
                    caller_bytes,
                    coff_object,
                )
            external_symbol_inventory = external_symbol_inventory_by_cod.get(
                cod_path
            )
            if external_symbol_inventory is None:
                external_symbol_inventory = (
                    _candidate_external_symbol_inventory(coff_object)
                )
                external_symbol_inventory_by_cod[cod_path] = (
                    external_symbol_inventory
                )
            (
                undefined_external_functions,
                defined_external_functions,
                undefined_external_data,
                defined_external_data,
            ) = external_symbol_inventory
            local_static_definitions = tuple(
                CandidateLocalStaticDefinition(
                    name=item.name,
                    section_name=section.name,
                    section_number=item.section_number,
                    value=item.value,
                    symbol_type=item.type,
                    storage_class=item.storage_class,
                    aux_count=item.aux_count,
                    data=section.raw_data[item.value : item.value + 4],
                )
                for symbol_index in dict.fromkeys(
                    relocation.symbol_index
                    for relocation in caller_bytes.relocations
                )
                for item in (coff_object.symbols_by_index.get(symbol_index),)
                if item is not None
                and item.section_number > 0
                and item.storage_class == IMAGE_SYM_CLASS_STATIC
                for section in (coff_object.section(item.section_number),)
            )
            associated_sections_by_parent = associated_sections_by_cod.get(
                cod_path
            )
            if associated_sections_by_parent is None:
                associated_sections_by_parent = (
                    _candidate_associated_comdat_sections_index(coff_object)
                )
                associated_sections_by_cod[cod_path] = (
                    associated_sections_by_parent
                )
            definitions = destructor_definitions_by_cod.get(cod_path)
            if definitions is None:
                definitions = _candidate_complete_destructor_definitions(
                    cod_path,
                    coff_object,
                    associated_sections_by_parent=(
                        associated_sections_by_parent
                    ),
                )
                destructor_definitions_by_cod[cod_path] = definitions
            vftable_definitions = vftable_definitions_by_cod.get(cod_path)
            if vftable_definitions is None:
                vftable_definitions = _candidate_vftable_definitions(coff_object)
                vftable_definitions_by_cod[cod_path] = vftable_definitions
            tu_local_function_definitions = (
                tu_local_function_definitions_by_cod.get(cod_path)
            )
            if tu_local_function_definitions is None:
                tu_local_function_definitions = (
                    _candidate_tu_local_function_definitions(
                        coff_object, cod_path,
                        header_source_files=_candidate_unit_header_source_files(
                            toolchain_receipts or (), target_name=target.name,
                            source_from=str(getattr(_entry, "source_from", target.source_from)),
                            build_dir=cod_path.parent,
                        ),
                        source_observation=_candidate_unit_source_observation(
                            toolchain_receipts or (), target_name=target.name,
                            source_from=str(getattr(_entry, "source_from", target.source_from)),
                            build_dir=cod_path.parent,
                        ),
                    )
                )
                tu_local_function_definitions_by_cod[cod_path] = (
                    tu_local_function_definitions
                )
            compiler_local_function_definitions = (
                compiler_local_function_definitions_by_cod.get(cod_path)
            )
            if compiler_local_function_definitions is None:
                compiler_local_function_definitions = (
                    _candidate_compiler_local_function_definitions(
                        coff_object, cod_path
                    )
                )
                compiler_local_function_definitions_by_cod[cod_path] = (
                    compiler_local_function_definitions
                )
            constructor_definitions = constructor_definitions_by_cod.get(
                cod_path
            )
            if constructor_definitions is None:
                constructor_definitions = (
                    _candidate_reviewed_constructor_definitions(
                        cod_path,
                        coff_object,
                    )
                )
                constructor_definitions_by_cod[cod_path] = (
                    constructor_definitions
                )
            coff_symbol_definitions = coff_symbol_definitions_by_cod.get(
                cod_path
            )
            if coff_symbol_definitions is None:
                coff_symbol_definitions = _candidate_coff_symbol_definitions(
                    coff_object
                )
                coff_symbol_definitions_by_cod[cod_path] = (
                    coff_symbol_definitions
                )
            candidates[address] = CandidateAssembly(
                instructions=parsed.instructions,
                local_control_flow_indices=parsed.local_control_flow_indices,
                local_control_flow_targets=parsed.local_control_flow_targets,
                classification_only_local_control_flow_targets=(
                    classification_only_targets
                ),
                target=target,
                complete_destructor_definitions=definitions,
                vftable_definitions=vftable_definitions,
                tu_local_function_definitions=(
                    tu_local_function_definitions
                ),
                compiler_local_function_definitions=(
                    compiler_local_function_definitions
                ),
                constructor_definitions=constructor_definitions,
                numeric_constructor_definitions=(
                    numeric_constructor_definitions
                    if address == _cc_catalog.HUD_NET_GAME_SETUP_CONSTRUCTOR_CALLER_START
                    else {}
                ),
                numeric_constructor_target=(
                    numeric_constructor_target
                    if address == _cc_catalog.HUD_NET_GAME_SETUP_CONSTRUCTOR_CALLER_START
                    else None
                ),
                cycle_selector_constructor_definitions=(
                    cycle_selector_constructor_definitions
                    if address == _cc_catalog.HUD_NET_GAME_SETUP_CONSTRUCTOR_CALLER_START
                    else {}
                ),
                cycle_selector_constructor_target=(
                    numeric_constructor_target
                    if address == _cc_catalog.HUD_NET_GAME_SETUP_CONSTRUCTOR_CALLER_START
                    else None
                ),
                zrd_widget_constructor_definitions=(
                    zrd_widget_constructor_definitions
                    if address == _cc_catalog.HUD_NET_GAME_SETUP_CONSTRUCTOR_CALLER_START
                    else {}
                ),
                zrd_widget_constructor_target=(
                    numeric_constructor_target
                    if address == _cc_catalog.HUD_NET_GAME_SETUP_CONSTRUCTOR_CALLER_START
                    else None
                ),
                caller_definition=CandidateCallerDefinition(
                    object_path=str(coff_object.path) if getattr(coff_object, "path", None) is not None else "",
                    symbol=symbol_name,
                    data=caller_bytes.data,
                    relocations=caller_bytes.relocations,
                    relocation_mask=caller_bytes.relocation_mask,
                    undefined_external_functions=undefined_external_functions,
                    defined_external_functions=defined_external_functions,
                    undefined_external_data=undefined_external_data,
                    defined_external_data=defined_external_data,
                    local_static_definitions=local_static_definitions,
                    coff_symbols=coff_symbol_definitions,
                    section_index=caller_bytes.section_index,
                    section_start=caller_bytes.start,
                    section_end=caller_bytes.end,
                    associated_sections=_candidate_associated_comdat_sections(
                        coff_object,
                        caller_bytes.section_index,
                        sections_by_parent=associated_sections_by_parent,
                    ),
                ),
            )
    if set(candidates) != set(slice_row["addresses"]):
        missing = sorted(set(slice_row["addresses"]) - set(candidates))
        raise ValueError(f"candidate compile did not produce exact slice membership: {missing}")
    return candidates
