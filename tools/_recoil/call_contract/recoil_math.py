"""Recoil call-contract recoil math evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import CandidateAssembly, IdentityIndexes

import json
import struct
from pathlib import Path
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_REL32,
    IMAGE_SCN_CNT_CODE,
    IMAGE_SYM_CLASS_EXTERNAL,
    Instruction,
)
from _recoil.lib.progress import ProgressDocument, address_value, normalize_address
from _recoil.lib.tooling import REPO_ROOT


def _zmath_camera_negate_float_sign_bit_candidate_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    compiler_generated_bridges: Mapping[str, str],
) -> dict[str, str]:
    """Expose one exact nine-call candidate-only zMath helper identity.

    Retail 0x473e60 contains no invocation: VC5 inlined the sign-bit XORs.
    Current source emits the TU-local helper as nine calls instead.  This
    bridge exists only so the ordinary retail/candidate contract comparator
    can report that semantic mismatch.  It does not synthesize retail calls,
    map the helper to a retail address, or grant reviewed identity state.
    """

    caller = candidate.caller_definition
    absolute_calls: dict[str, list[tuple[int, Instruction, int]]] = {}
    for instruction_index, instruction in enumerate(candidate.instructions):
        if _cc_cfg._instruction_mnemonic(instruction) not in {"call", "jmp"}:
            continue
        name = _cc_targets._cod_space_bearing_direct_target(
            _cc_cfg._instruction_operand(instruction).strip(),
            instruction.source_line,
        )
        if name is not None and "NegateFloatSignBit" in name:
            raw_offset = _cc_cfg._source_instruction_address(instruction)
            absolute_calls.setdefault(name, []).append(
                (
                    instruction_index,
                    instruction,
                    address_value(raw_offset) if raw_offset else -1,
                )
            )
    if not absolute_calls:
        return {}
    if len(absolute_calls) != 1:
        raise ValueError(
            "zMath camera negate candidate bridge requires one exact "
            "absolute TU-local helper decoration"
        )
    candidate_name, calls = next(iter(absolute_calls.items()))

    match = _cc_catalog._VC5_TU_LOCAL_CALLABLE_RE.fullmatch(candidate_name)
    expected_source = (
        REPO_ROOT / _cc_catalog._ZMATH_CAMERA_STAGE_INVERSE_ROTATION_SOURCE
    ).resolve()
    if (
        match is None
        or match.group("head") != "?NegateFloatSignBit@?%"
        or match.group("suffix") != "YIMM@Z"
        or Path(match.group("source")).resolve() != expected_source
    ):
        raise ValueError(
            "zMath camera negate candidate bridge requires the exact "
            "absolute TU-local NegateFloatSignBit(float) decoration"
        )

    target = candidate.target
    manifest_path = Path(str(getattr(target, "manifest_path", "")))
    target_rows = tuple(getattr(target, "functions", ()))
    matching_target_rows = [
        row
        for row in target_rows
        if str(getattr(row, "address", ""))
        == _cc_catalog._ZMATH_CAMERA_STAGE_INVERSE_ROTATION_CALLER_START
    ]
    if (
        caller_identity
        != _cc_catalog._ZMATH_CAMERA_STAGE_INVERSE_ROTATION_CALLER_IDENTITY
        or normalize_address(caller_start)
        != _cc_catalog._ZMATH_CAMERA_STAGE_INVERSE_ROTATION_CALLER_START
        or normalize_address(caller_end_exclusive)
        != _cc_catalog._ZMATH_CAMERA_STAGE_INVERSE_ROTATION_CALLER_END
        or tuple(expected) != ()
        or caller is None
        or caller.symbol
        != _cc_catalog._ZMATH_CAMERA_STAGE_INVERSE_ROTATION_CALLER_SYMBOL
        or target is None
        or getattr(target, "target_binary", "") != "recoil"
        or getattr(target, "name", "")
        != _cc_catalog._ZMATH_CAMERA_STAGE_INVERSE_ROTATION_TARGET_NAME
        or not bool(
            getattr(target, "check_translation_unit_function_order", False)
        )
        or manifest_path.resolve()
        != (REPO_ROOT / _cc_catalog._ZMATH_CAMERA_STAGE_INVERSE_ROTATION_MANIFEST).resolve()
        or getattr(target, "source_from", "")
        != _cc_catalog._ZMATH_CAMERA_STAGE_INVERSE_ROTATION_SOURCE
        or tuple(getattr(target, "source_files", ())) != ()
        or len(matching_target_rows) != 1
        or getattr(matching_target_rows[0], "symbol", "")
        != _cc_catalog._ZMATH_CAMERA_STAGE_INVERSE_ROTATION_CALLER_SYMBOL
        or getattr(matching_target_rows[0], "symbol_regex", None) is not None
        or getattr(matching_target_rows[0], "pipeline_class", "")
        != "authored"
        or getattr(matching_target_rows[0], "authored_order_role", "")
        != "authored-body"
        or getattr(matching_target_rows[0], "required_presence", False)
        is not True
        or getattr(matching_target_rows[0], "full_order_gate", False)
        is not True
    ):
        raise ValueError(
            "zMath camera negate candidate bridge requires the exact retail "
            "caller, empty retail contract, and selected VC5 target manifest"
        )

    symbol_id = "recoil:function:0x473e60"
    symbol = document.collection("symbols").get(symbol_id)
    source_traceability = (
        symbol.get("source_traceability")
        if isinstance(symbol, Mapping)
        else None
    )
    source_edges = (
        source_traceability.get("source_edges")
        if isinstance(source_traceability, Mapping)
        else None
    )
    exact_source_edges = [
        edge
        for edge in source_edges or []
        if isinstance(edge, Mapping)
        and edge.get("relation") == "defines"
        and isinstance(edge.get("emission_context"), Mapping)
        and edge["emission_context"].get("translation_unit")
        == _cc_catalog._ZMATH_CAMERA_STAGE_INVERSE_ROTATION_SOURCE
    ]
    registered_target = document.collection("verification_targets").get(
        _cc_catalog._ZMATH_CAMERA_STAGE_INVERSE_ROTATION_TARGET_ID
    )
    registration = (
        registered_target.get("registration")
        if isinstance(registered_target, Mapping)
        else None
    )
    registered_caller_rows = [
        row
        for _view, row in _cc_identity._mapping_target_function_rows_with_views(
            registered_target or {}
        )
        if row.get("address")
        == _cc_catalog._ZMATH_CAMERA_STAGE_INVERSE_ROTATION_CALLER_START
        and row.get("symbol")
        == _cc_catalog._ZMATH_CAMERA_STAGE_INVERSE_ROTATION_CALLER_SYMBOL
    ]
    unique_registered_rows = {
        json.dumps(dict(row), sort_keys=True, separators=(",", ":"))
        for row in registered_caller_rows
    }
    if (
        not isinstance(symbol, Mapping)
        or _cc_identity._symbol_identity(symbol_id, symbol) != caller_identity
        or symbol.get("binary") != "recoil"
        or symbol.get("kind") != "function"
        or symbol.get("pipeline_class") != "authored"
        or symbol.get("authored_order_role") != "authored-body"
        or symbol.get("address")
        != _cc_catalog._ZMATH_CAMERA_STAGE_INVERSE_ROTATION_CALLER_START
        or symbol.get("end_exclusive")
        != _cc_catalog._ZMATH_CAMERA_STAGE_INVERSE_ROTATION_CALLER_END
        or symbol.get("size") != 0x160
        or symbol.get("extent_state") != "known"
        or symbol.get("disposition") != "unresolved"
        or symbol.get("ownership_state") != "primary-owned"
        or symbol.get("physical_block_id") != "recoil:block:0x472670"
        or symbol.get("output_section_id") != "recoil:section:.text"
        or _cc_catalog._ZMATH_CAMERA_STAGE_INVERSE_ROTATION_TARGET_ID
        not in symbol.get("verification_target_ids", [])
        or not isinstance(source_traceability, Mapping)
        or source_traceability.get("state") != "resolved"
        or len(source_edges or []) != 1
        or len(exact_source_edges) != 1
        or indexes.by_address.get(
            _cc_catalog._ZMATH_CAMERA_STAGE_INVERSE_ROTATION_CALLER_START
        )
        != caller_identity
        or caller_identity in indexes.provider_ids
        or not isinstance(registered_target, Mapping)
        or registered_target.get("binary") != "recoil"
        or registered_target.get("kind") != "vc5"
        or registered_target.get("name")
        != _cc_catalog._ZMATH_CAMERA_STAGE_INVERSE_ROTATION_TARGET_NAME
        or not isinstance(registration, Mapping)
        or registration.get("manifest_path")
        != _cc_catalog._ZMATH_CAMERA_STAGE_INVERSE_ROTATION_MANIFEST
        or registration.get("source_from")
        != _cc_catalog._ZMATH_CAMERA_STAGE_INVERSE_ROTATION_SOURCE
        or registration.get("check_translation_unit_function_order")
        is not True
        or registered_target.get("registered_addresses", []).count(
            _cc_catalog._ZMATH_CAMERA_STAGE_INVERSE_ROTATION_CALLER_START
        )
        != 1
        or len(unique_registered_rows) != 1
    ):
        raise ValueError(
            "zMath camera negate candidate bridge requires the exact reviewed "
            "caller identity, extent, source edge, and target registration"
        )

    call_offsets = tuple(offset for _index, _instruction, offset in calls)
    if call_offsets != _cc_catalog._ZMATH_CAMERA_NEGATE_CALL_OFFSETS:
        raise ValueError(
            "zMath camera negate candidate bridge requires the exact nine "
            "candidate call offsets"
        )
    for instruction_index, instruction, offset in calls:
        if (
            instruction_index in candidate.local_control_flow_indices
            or _cc_cfg._instruction_mnemonic(instruction) != "call"
            or tuple(value.lower() for value in instruction.bytes)
            != ("e8", "00", "00", "00", "00")
            or _cc_targets._cod_space_bearing_direct_target(
                _cc_cfg._instruction_operand(instruction).strip(),
                instruction.source_line,
            )
            != candidate_name
            or offset < 0
        ):
            raise ValueError(
                "zMath camera negate candidate bridge requires exact nonlocal "
                "zero-addend E8 COD evidence at all nine call sites"
            )

    folded_name = candidate_name.casefold()
    for label, names in (
        ("candidate identity", indexes.by_candidate_name),
        ("Binary Ninja identity", bridge_names),
        ("compiler/provider identity", compiler_generated_bridges),
        ("candidate storage identity", indexes.storage_by_name),
    ):
        collisions = sorted(name for name in names if name.casefold() == folded_name)
        if collisions:
            raise ValueError(
                "zMath camera negate candidate bridge has an existing "
                f"{label} collision for {candidate_name!r}"
            )

    definitions = [
        row
        for row in caller.coff_symbols
        if row.name.casefold() == folded_name
    ]
    defined_names = [
        name
        for name in caller.defined_external_functions
        if name.casefold() == folded_name
    ]
    undefined_names = [
        name
        for name in caller.undefined_external_functions
        if name.casefold() == folded_name
    ]
    if (
        len(definitions) != 1
        or definitions[0].name != candidate_name
        or defined_names != [candidate_name]
        or undefined_names
    ):
        raise ValueError(
            "zMath camera negate candidate bridge requires one exact same-TU "
            "defined COFF external and no duplicate, undefined, or folded alias"
        )
    definition = definitions[0]
    weak_or_coincident = [
        row
        for row in caller.coff_symbols
        if (
            row.storage_class == 105
            and (
                row.name.casefold() == folded_name
                or row.weak_external_tag_index == definition.index
            )
        )
        or (
            row.index != definition.index
            and row.section_number == definition.section_number
            and row.value == definition.value
            and row.symbol_type == 0x20
        )
    ]
    if (
        weak_or_coincident
        or definition.section_number <= 0
        or definition.storage_class != IMAGE_SYM_CLASS_EXTERNAL
        or definition.symbol_type != 0x20
        or definition.section_name != ".text"
        or definition.section_size != 0x10
        or definition.value != 0
        or definition.natural_end != 0x10
        or not (definition.section_characteristics & IMAGE_SCN_CNT_CODE)
        or not (definition.section_characteristics & _cc_catalog.IMAGE_SCN_LNK_COMDAT)
    ):
        raise ValueError(
            "zMath camera negate candidate bridge requires one exact unaliased "
            "16-byte external code COMDAT definition"
        )

    if (
        caller.section_index <= 0
        or caller.section_start != 0
        or caller.section_end - caller.section_start != len(caller.data)
        or len(caller.data) != len(caller.relocation_mask)
    ):
        raise ValueError(
            "zMath camera negate candidate bridge requires one exact caller "
            "COFF section and body extent"
        )
    matching_relocations = [
        relocation
        for relocation in caller.relocations
        if relocation.symbol_name.casefold() == folded_name
    ]
    expected_relocation_offsets = tuple(
        offset + 1 for offset in _cc_catalog._ZMATH_CAMERA_NEGATE_CALL_OFFSETS
    )
    if (
        len(matching_relocations) != len(expected_relocation_offsets)
        or tuple(relocation.offset for relocation in matching_relocations)
        != expected_relocation_offsets
    ):
        raise ValueError(
            "zMath camera negate candidate bridge requires the exact nine "
            "ordered call-site relocations"
        )
    for relocation in matching_relocations:
        field_end = relocation.offset + 4
        if (
            relocation.type != IMAGE_REL_I386_REL32
            or relocation.symbol_name != candidate_name
            or relocation.symbol_index != definition.index
            or relocation.offset < 1
            or field_end > len(caller.data)
            or caller.data[relocation.offset - 1] != 0xE8
            or struct.unpack_from("<I", caller.data, relocation.offset)[0] != 0
            or caller.relocation_mask[relocation.offset - 1]
            or not all(
                caller.relocation_mask[index]
                for index in range(relocation.offset, field_end)
            )
        ):
            raise ValueError(
                "zMath camera negate candidate bridge requires exact REL32 "
                "target, zero addend, opcode mask, and defining-symbol provenance"
            )

    identity = f"candidate-local-coff:{candidate_name}"
    if (
        identity in indexes.provider_ids
        or identity in indexes.by_address.values()
        or identity in indexes.by_candidate_name.values()
        or identity in compiler_generated_bridges.values()
    ):
        raise ValueError(
            "zMath camera negate candidate bridge comparison-only identity "
            "conflicts with reviewed state"
        )
    return {candidate_name: identity}
