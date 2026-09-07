"""Recoil call-contract recoil hud panels evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import extraction as _cc_extraction
from _recoil.call_contract import receiver_storage as _cc_receiver_storage
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        IdentityIndexes,
        ReviewedCallResultBridge,
        ReviewedVptrStorageBridge,
    )

import re
import struct
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_DIR32,
    IMAGE_REL_I386_REL32,
    Instruction,
)
from _recoil.lib.binja import BinaryNinjaBridge
from _recoil.lib.progress import (
    ProgressDocument,
    ProgressError,
    address_value,
    normalize_address,
)


def _hud_triplet_sort_key_tu_local_candidate_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
) -> dict[str, str]:
    """Resolve one exact VC5 TU-local helper without hiding call drift.

    Retail ``HudUiListMenuEntry::CompareSortKey`` contains the key calculation
    inline and therefore has an empty invocation contract.  The current VC5
    diagnostic emits that source helper in the same COFF object and calls it
    twice.  This bridge uses the reviewed caller identity only as a
    candidate-side comparison sentinel for the retail body where the helper
    semantics were inlined.  It does not invent a tracker-backed identity for
    a non-retail helper.  The rows remain direct (not ``self``) calls and the
    untouched empty retail contract makes comparison report them as extra; the
    bridge never removes them or treats candidate output as retail truth.
    """
    matching_instructions: list[Instruction] = []
    matching_symbols: set[str] = set()
    for instruction in candidate.instructions:
        if _cc_cfg._instruction_mnemonic(instruction) not in {"call", "jmp"}:
            continue
        operand = _cc_cfg._instruction_operand(instruction).strip()
        if _cc_catalog.HUD_TRIPLET_SORT_KEY_TU_LOCAL_COD_RE.fullmatch(operand):
            matching_instructions.append(instruction)
            matching_symbols.add(operand)
            continue
        if "HudUiTripletEntrySortKey" in operand:
            raise ValueError(
                "HUD triplet sort-key TU-local bridge accepts only the exact "
                "helper name, 00_hud.cpp TU marker, decimal discriminator, "
                "and function signature"
            )
    if not matching_instructions:
        return {}
    if len(matching_symbols) != 1:
        raise ValueError(
            "HUD triplet sort-key TU-local bridge requires one exact candidate "
            "helper decoration"
        )
    cod_symbol = next(iter(matching_symbols))
    coff_symbol = cod_symbol.replace(
        "?_tu_order\\",
        "?%_tu_order\\",
        1,
    )

    definition = candidate.caller_definition
    normalized_start = normalize_address(caller_start)
    normalized_end = normalize_address(caller_end_exclusive)
    if (
        caller_identity != _cc_catalog.HUD_TRIPLET_SORT_KEY_CALLER_IDENTITY
        or normalized_start != _cc_catalog.HUD_TRIPLET_SORT_KEY_CALLER_START
        or normalized_end != _cc_catalog.HUD_TRIPLET_SORT_KEY_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(normalized_start) != caller_identity
        or caller_identity in indexes.provider_ids
        or definition is None
        or definition.symbol != _cc_catalog.HUD_TRIPLET_SORT_KEY_CALLER_SYMBOL
        or expected
    ):
        raise ValueError(
            "HUD triplet sort-key TU-local bridge requires the exact reviewed "
            "0x40d220 caller identity, extent, symbol, and empty retail "
            "invocation contract"
        )
    if (
        cod_symbol in indexes.by_candidate_name
        or coff_symbol in indexes.by_candidate_name
        or cod_symbol in bridge_names
        or coff_symbol in bridge_names
        or cod_symbol in indexes.storage_by_name
        or coff_symbol in indexes.storage_by_name
    ):
        raise ValueError(
            "HUD triplet sort-key TU-local bridge has a conflicting ordinary "
            "candidate, retail, or storage identity"
        )

    defined_case_matches = [
        name
        for name in definition.defined_external_functions
        if name.casefold() == coff_symbol.casefold()
    ]
    undefined_case_matches = [
        name
        for name in definition.undefined_external_functions
        if name.casefold()
        in {
            cod_symbol.casefold(),
            coff_symbol.casefold(),
        }
    ]
    if (
        defined_case_matches != [coff_symbol]
        or undefined_case_matches
    ):
        raise ValueError(
            "HUD triplet sort-key TU-local bridge requires exactly one "
            "same-object external function definition with the exact VC5 "
            "TU-local signature and no undefined or case-folded alias"
        )

    instruction_offsets: list[int] = []
    for instruction in matching_instructions:
        raw_offset = _cc_cfg._source_instruction_address(instruction)
        if (
            _cc_cfg._instruction_mnemonic(instruction) != "call"
            or tuple(instruction.bytes[:1]) != ("e8",)
            or not raw_offset
        ):
            raise ValueError(
                "HUD triplet sort-key TU-local bridge requires exact "
                "offset-bearing direct E8 CALL instructions"
            )
        instruction_offsets.append(address_value(raw_offset))
    if tuple(instruction_offsets) != _cc_catalog.HUD_TRIPLET_SORT_KEY_CALL_OFFSETS:
        raise ValueError(
            "HUD triplet sort-key TU-local bridge requires the exact reviewed "
            "two-call candidate topology"
        )

    references = tuple(
        sorted(
            (
                relocation
                for relocation in definition.relocations
                if relocation.symbol_name == coff_symbol
            ),
            key=lambda relocation: relocation.offset,
        )
    )
    expected_relocation_offsets = tuple(
        offset + 1 for offset in _cc_catalog.HUD_TRIPLET_SORT_KEY_CALL_OFFSETS
    )
    if tuple(reference.offset for reference in references) != (
        expected_relocation_offsets
    ):
        raise ValueError(
            "HUD triplet sort-key TU-local bridge requires one exact COFF "
            "relocation for each reviewed candidate call"
        )
    for reference in references:
        field_end = reference.offset + 4
        if (
            reference.type != IMAGE_REL_I386_REL32
            or reference.offset < 1
            or field_end > len(definition.data)
            or field_end > len(definition.relocation_mask)
            or definition.data[reference.offset - 1 : reference.offset]
            != b"\xe8"
            or struct.unpack_from("<I", definition.data, reference.offset)[0]
            != 0
            or not all(
                definition.relocation_mask[index]
                for index in range(reference.offset, field_end)
            )
        ):
            raise ValueError(
                "HUD triplet sort-key TU-local bridge requires exact "
                "zero-addend fully masked E8 REL32 relocations"
            )

    provisional = _cc_extraction.extract_invocation_contract(
        candidate.instructions,
        source="cod",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge_names=bridge_names,
        compiler_generated_bridges={
            cod_symbol: caller_identity
        },
        local_control_flow_indices=candidate.local_control_flow_indices,
        local_control_flow_targets=candidate.local_control_flow_targets,
    )
    bridged_rows = [
        row
        for row in provisional
        if row.get("target_identity") == caller_identity
    ]
    if (
        len(provisional) != len(matching_instructions)
        or len(bridged_rows) != len(matching_instructions)
        or any(
            row.get("ordinal") != ordinal
            or row.get("form") != "call"
            or row.get("dispatch") != "direct"
            or row.get("identity_kind") != "direct"
            or row.get("storage_identity") != ""
            or row.get("slot_displacement") is not None
            or row.get("cleanup_bytes") is not None
            for ordinal, row in enumerate(bridged_rows)
        )
    ):
        raise ValueError(
            "HUD triplet sort-key TU-local bridge does not produce the exact "
            "two direct candidate calls to the reviewed retail identity"
        )
    return {
        cod_symbol: caller_identity,
    }


def _hud_triplet_panel_constructor_candidate_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    compiler_generated_bridges: Mapping[str, str],
) -> dict[str, str]:
    """Map one exact natural triplet-panel constructor to reviewed retail.

    The source-faithful ``HudUiMgrData`` constructor directly constructs its
    embedded ``HudUiTripletPanel``.  Prefer the ordinary exact candidate-name
    index once it maps that natural decoration to the reviewed identity; the
    caller-scoped bridge remains only for older reviewed index populations
    which do not yet contain that exact mapping.  Candidate offsets and
    ordinals remain untouched: the relocation follows the exact E8
    instruction, while ordinary contract comparison still reports call
    ordering drift.
    """
    constructor_symbol_casefold = (
        _cc_catalog.HUD_TRIPLET_PANEL_CONSTRUCTOR_SYMBOL.casefold()
    )
    ordinary_candidate_matches = [
        (name, identity)
        for name, identity in indexes.by_candidate_name.items()
        if name.casefold() == constructor_symbol_casefold
    ]
    ordinary_candidate_is_exact = ordinary_candidate_matches == [
        (
            _cc_catalog.HUD_TRIPLET_PANEL_CONSTRUCTOR_SYMBOL,
            _cc_catalog.HUD_TRIPLET_PANEL_CONSTRUCTOR_TARGET_IDENTITY,
        )
    ]
    if (
        (
            ordinary_candidate_matches
            and not ordinary_candidate_is_exact
        )
        or any(
            name.casefold() == constructor_symbol_casefold
            for name in indexes.storage_by_name
        )
        or any(
            str(name).casefold() == constructor_symbol_casefold
            for name in bridge_names
        )
        or any(
            name.casefold() == constructor_symbol_casefold
            for name in compiler_generated_bridges
        )
    ):
        raise ValueError(
            "HUD triplet-panel constructor bridge has a conflicting ordinary "
            "candidate, retail, storage, or compiler bridge identity"
        )
    if ordinary_candidate_is_exact:
        return {}

    matching_instructions: list[Instruction] = []
    for instruction in candidate.instructions:
        if _cc_cfg._instruction_mnemonic(instruction) not in {"call", "jmp"}:
            continue
        operand = _cc_cfg._instruction_operand(instruction).strip()
        if operand == _cc_catalog.HUD_TRIPLET_PANEL_CONSTRUCTOR_SYMBOL:
            matching_instructions.append(instruction)
            continue
        if "HudUiTripletPanel" in operand:
            raise ValueError(
                "HUD triplet-panel constructor bridge accepts only the exact "
                "HudUiTripletPanel zero-argument C++ constructor decoration"
            )
    if not matching_instructions:
        return {}
    if len(matching_instructions) != 1:
        raise ValueError(
            "HUD triplet-panel constructor bridge requires exactly one "
            "candidate invocation"
        )

    definition = candidate.caller_definition
    normalized_start = normalize_address(caller_start)
    normalized_end = normalize_address(caller_end_exclusive)
    caller_identity_addresses = [
        address
        for address, identity in indexes.by_address.items()
        if identity == caller_identity
    ]
    if (
        caller_identity != _cc_catalog.HUD_TRIPLET_PANEL_CONSTRUCTOR_CALLER_IDENTITY
        or normalized_start != _cc_catalog.HUD_TRIPLET_PANEL_CONSTRUCTOR_CALLER_START
        or normalized_end
        != _cc_catalog.HUD_TRIPLET_PANEL_CONSTRUCTOR_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(normalized_start) != caller_identity
        or caller_identity_addresses != [normalized_start]
        or caller_identity in indexes.provider_ids
        or definition is None
        or definition.symbol != _cc_catalog.HUD_TRIPLET_PANEL_CONSTRUCTOR_CALLER_SYMBOL
    ):
        raise ValueError(
            "HUD triplet-panel constructor bridge requires the exact reviewed "
            "0x40d7e0 caller identity, extent, and VC5 constructor symbol"
        )
    target_identity_addresses = [
        address
        for address, identity in indexes.by_address.items()
        if identity == _cc_catalog.HUD_TRIPLET_PANEL_CONSTRUCTOR_TARGET_IDENTITY
    ]
    if (
        indexes.by_address.get(_cc_catalog.HUD_TRIPLET_PANEL_CONSTRUCTOR_TARGET_ADDRESS)
        != _cc_catalog.HUD_TRIPLET_PANEL_CONSTRUCTOR_TARGET_IDENTITY
        or target_identity_addresses
        != [_cc_catalog.HUD_TRIPLET_PANEL_CONSTRUCTOR_TARGET_ADDRESS]
        or _cc_catalog.HUD_TRIPLET_PANEL_CONSTRUCTOR_TARGET_IDENTITY in indexes.provider_ids
    ):
        raise ValueError(
            "HUD triplet-panel constructor bridge requires the exact reviewed "
            "0x40f200 authored constructor identity"
        )
    undefined_case_matches = [
        name
        for name in definition.undefined_external_functions
        if name.casefold() == constructor_symbol_casefold
    ]
    defined_case_matches = [
        name
        for name in definition.defined_external_functions
        if name.casefold() == constructor_symbol_casefold
    ]
    if (
        defined_case_matches != [_cc_catalog.HUD_TRIPLET_PANEL_CONSTRUCTOR_SYMBOL]
        or undefined_case_matches
    ):
        raise ValueError(
            "HUD triplet-panel constructor bridge requires exactly one "
            "same-object defined external function with the exact constructor "
            "decoration and no undefined or case-folded alias"
        )

    instruction = matching_instructions[0]
    instruction_index = next(
        index
        for index, item in enumerate(candidate.instructions)
        if item is instruction
    )
    raw_offset = _cc_cfg._source_instruction_address(instruction)
    if (
        _cc_cfg._instruction_mnemonic(instruction) != "call"
        or tuple(instruction.bytes[:1]) != ("e8",)
        or not raw_offset
        or _cc_cfg._cleanup_after(candidate.instructions, instruction_index)
        is not None
    ):
        raise ValueError(
            "HUD triplet-panel constructor bridge requires one exact "
            "offset-bearing direct E8 candidate call with no caller cleanup"
        )
    call_offset = address_value(raw_offset)

    references = tuple(
        relocation
        for relocation in definition.relocations
        if relocation.symbol_name.casefold() == constructor_symbol_casefold
    )
    relocation_offset = call_offset + 1
    relocations_at_call = tuple(
        relocation
        for relocation in definition.relocations
        if relocation.offset == relocation_offset
    )
    if (
        len(references) != 1
        or relocations_at_call != references
        or references[0].symbol_name
        != _cc_catalog.HUD_TRIPLET_PANEL_CONSTRUCTOR_SYMBOL
        or references[0].offset != relocation_offset
        or references[0].type != IMAGE_REL_I386_REL32
        or relocation_offset + 4 > len(definition.data)
        or relocation_offset + 4 > len(definition.relocation_mask)
        or definition.data[call_offset:relocation_offset] != b"\xe8"
        or struct.unpack_from(
            "<I",
            definition.data,
            relocation_offset,
        )[0]
        != 0
        or not all(
            definition.relocation_mask[index]
            for index in range(relocation_offset, relocation_offset + 4)
        )
    ):
        raise ValueError(
            "HUD triplet-panel constructor bridge requires one unique exact "
            "zero-addend fully masked E8 REL32 relocation at the candidate "
            "call"
        )

    expected_rows = [
        row
        for row in expected
        if row.get("target_identity")
        == _cc_catalog.HUD_TRIPLET_PANEL_CONSTRUCTOR_TARGET_IDENTITY
    ]
    if (
        len(expected_rows) != 1
        or expected_rows[0].get("ordinal")
        != _cc_catalog.HUD_TRIPLET_PANEL_CONSTRUCTOR_RETAIL_ORDINAL
        or expected_rows[0].get("form") != "call"
        or expected_rows[0].get("dispatch") != "direct"
        or expected_rows[0].get("identity_kind") != "direct"
        or expected_rows[0].get("storage_identity") != ""
        or expected_rows[0].get("slot_displacement") is not None
        or expected_rows[0].get("cleanup_bytes") is not None
    ):
        raise ValueError(
            "HUD triplet-panel constructor bridge requires the exact reviewed "
            "ordinal-3 direct retail call contract for 0x40f200"
        )

    return {
        _cc_catalog.HUD_TRIPLET_PANEL_CONSTRUCTOR_SYMBOL: (
            _cc_catalog.HUD_TRIPLET_PANEL_CONSTRUCTOR_TARGET_IDENTITY
        )
    }


def _hud_counter_text_panel_constructor_vptr_storage_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    bridge: BinaryNinjaBridge,
    compiler_generated_bridges: Mapping[str, str],
    candidate_storage_bridges: Mapping[str, str],
    reviewed_register_storage_bridges: Mapping[str, str],
    reviewed_call_result_bridges: Mapping[str, ReviewedCallResultBridge],
) -> dict[str, ReviewedVptrStorageBridge]:
    """Prove one exact register-loaded constructor vptr callsite.

    This bridge is deliberately keyed by the reviewed candidate call offset.
    It canonicalizes only the proven ``load(this)`` value in EAX at that one
    invocation; it grants no name alias and does not alter generic register
    provenance.
    """
    from _recoil.call_contract.records import ReviewedVptrStorageBridge
    if (
        caller_identity
        != _cc_catalog.HUD_COUNTER_TEXT_PANEL_CONSTRUCTOR_CALLER_IDENTITY
        and normalize_address(caller_start)
        != _cc_catalog.HUD_COUNTER_TEXT_PANEL_CONSTRUCTOR_CALLER_START
    ):
        return {}
    caller = candidate.caller_definition
    caller_addresses = [
        address
        for address, identity in indexes.by_address.items()
        if identity == caller_identity
    ]
    caller_name_matches = [
        (name, identity)
        for name, identity in indexes.by_candidate_name.items()
        if name.casefold()
        == _cc_catalog.HUD_COUNTER_TEXT_PANEL_CONSTRUCTOR_CALLER_SYMBOL.casefold()
    ]
    if (
        caller_identity
        != _cc_catalog.HUD_COUNTER_TEXT_PANEL_CONSTRUCTOR_CALLER_IDENTITY
        or normalize_address(caller_start)
        != _cc_catalog.HUD_COUNTER_TEXT_PANEL_CONSTRUCTOR_CALLER_START
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_COUNTER_TEXT_PANEL_CONSTRUCTOR_CALLER_END_EXCLUSIVE
        or caller_addresses
        != [_cc_catalog.HUD_COUNTER_TEXT_PANEL_CONSTRUCTOR_CALLER_START]
        or caller_name_matches
        != [
            (
                _cc_catalog.HUD_COUNTER_TEXT_PANEL_CONSTRUCTOR_CALLER_SYMBOL,
                _cc_catalog.HUD_COUNTER_TEXT_PANEL_CONSTRUCTOR_CALLER_IDENTITY,
            )
        ]
        or caller_identity in indexes.provider_ids
        or caller is None
        or caller.symbol
        != _cc_catalog.HUD_COUNTER_TEXT_PANEL_CONSTRUCTOR_CALLER_SYMBOL
        or len(caller.data) != 0xD0
        or len(caller.relocation_mask) != len(caller.data)
    ):
        raise ValueError(
            "HUD counter-text constructor vptr bridge requires the exact "
            "reviewed 0x40dbf0 caller identity, unique case-exact natural "
            "constructor mapping, extent, symbol, and 0xd0-byte candidate body"
        )

    storage_identity = _cc_catalog.HUD_COUNTER_TEXT_PANEL_STORAGE_IDENTITY
    storage_address = _cc_catalog.HUD_COUNTER_TEXT_PANEL_STORAGE_ADDRESS
    storage_start = address_value(storage_address)
    storage_end = address_value(
        _cc_catalog.HUD_COUNTER_TEXT_PANEL_STORAGE_END_EXCLUSIVE
    )
    storage_addresses = [
        address
        for address, identity in indexes.storage_by_address.items()
        if identity == storage_identity
    ]
    containers = [
        row
        for row in indexes.storage_containers
        if row.identity == storage_identity
    ]
    data_symbol_id = storage_identity.removeprefix("storage:")
    data_symbol = document.collection("symbols").get(data_symbol_id)
    if (
        storage_addresses != [storage_address]
        or len(containers) != 1
        or containers[0].start != storage_start
        or containers[0].end_exclusive != storage_end
        or storage_end - storage_start
        != _cc_catalog.HUD_COUNTER_TEXT_PANEL_VFTABLE_SIZE
        or not isinstance(data_symbol, Mapping)
        or data_symbol.get("binary") != "recoil"
        or data_symbol.get("kind") != "data"
        or data_symbol.get("disposition") != "authored"
        or normalize_address(str(data_symbol.get("address", "")))
        != storage_address
        or normalize_address(
            str(data_symbol.get("end_exclusive", ""))
        )
        != _cc_catalog.HUD_COUNTER_TEXT_PANEL_STORAGE_END_EXCLUSIVE
        or data_symbol.get("output_section_id")
        != "recoil:section:.rdata"
    ):
        raise ValueError(
            "HUD counter-text constructor vptr bridge requires the exact "
            "reviewed authored 0x4ce578..0x4ce60c .rdata identity and extent"
        )

    vftable_symbol = _cc_catalog.HUD_COUNTER_TEXT_PANEL_VFTABLE_SYMBOL
    vftable_casefold = vftable_symbol.casefold()
    storage_name_matches = [
        (name, identity)
        for name, identity in indexes.storage_by_name.items()
        if name.casefold() == vftable_casefold
    ]
    conflicting_name_bridges = (
        [
            name
            for name in indexes.by_candidate_name
            if name.casefold() == vftable_casefold
        ]
        + [
            name
            for name in candidate_storage_bridges
            if name.casefold() == vftable_casefold
        ]
        + [
            name
            for name in compiler_generated_bridges
            if name.casefold() == vftable_casefold
        ]
    )
    retail_name_matches = [
        (name, rows)
        for name, rows in bridge_names.items()
        if str(name).casefold() == vftable_casefold
    ]
    retail_name_addresses: set[str] = set()
    for name, rows in retail_name_matches:
        if name != vftable_symbol:
            retail_name_addresses.add("")
            continue
        values = (
            list(rows)
            if isinstance(rows, (list, tuple, set, frozenset))
            else [rows]
        )
        try:
            retail_name_addresses.update(
                normalize_address(str(getattr(row, "address", "")))
                for row in values
            )
        except ProgressError:
            retail_name_addresses.add("")
    if (
        tuple(storage_name_matches) not in {
            (),
            ((vftable_symbol, storage_identity),),
        }
        or conflicting_name_bridges
        or (
            retail_name_addresses
            and retail_name_addresses != {storage_address}
        )
    ):
        raise ValueError(
            "HUD counter-text constructor vptr bridge has a conflicting "
            "vftable storage identity"
        )

    definition = candidate.vftable_definitions.get(vftable_symbol)
    if (
        definition is None
        or definition.symbol != vftable_symbol
        or definition.section_name != ".rdata"
        or not definition.section_is_comdat
        or definition.comdat_selection != 2
        or definition.section_external_symbols != (vftable_symbol,)
        or len(definition.data) != definition.section_size
        or definition.section_size
        != _cc_catalog.HUD_COUNTER_TEXT_PANEL_VFTABLE_SIZE
        or any(definition.data)
        or len(definition.relocations) != 37
        or any(
            relocation.type != IMAGE_REL_I386_DIR32
            for relocation in definition.relocations
        )
        or tuple(
            relocation.offset for relocation in definition.relocations
        )
        != tuple(range(0, _cc_catalog.HUD_COUNTER_TEXT_PANEL_VFTABLE_SIZE, 4))
        or not all(definition.relocation_mask)
    ):
        raise ValueError(
            "HUD counter-text constructor vptr bridge requires one exact "
            "148-byte full-section VC5 .rdata COMDAT SELECT_ANY vftable"
        )
    slot_rows = [
        relocation
        for relocation in definition.relocations
        if relocation.offset
        == _cc_catalog.HUD_COUNTER_TEXT_PANEL_SLOT_DISPLACEMENT
    ]
    if (
        len(slot_rows) != 1
        or slot_rows[0].symbol_name
        != _cc_catalog.HUD_COUNTER_TEXT_PANEL_SLOT_SYMBOL
    ):
        raise ValueError(
            "HUD counter-text constructor vptr bridge requires the exact "
            "slot-0x74 SetTextFmt relocation"
        )

    slot_symbol_matches = [
        (name, identity)
        for name, identity in indexes.by_candidate_name.items()
        if name.casefold()
        == _cc_catalog.HUD_COUNTER_TEXT_PANEL_SLOT_SYMBOL.casefold()
    ]
    slot_identity_addresses = (
        [
            address
            for address, identity in indexes.by_address.items()
            if identity == slot_symbol_matches[0][1]
        ]
        if len(slot_symbol_matches) == 1
        else []
    )
    if (
        len(slot_symbol_matches) != 1
        or slot_symbol_matches[0][0]
        != _cc_catalog.HUD_COUNTER_TEXT_PANEL_SLOT_SYMBOL
        or not slot_symbol_matches[0][1]
        or slot_symbol_matches[0][1] in indexes.provider_ids
        or len(slot_identity_addresses) != 1
    ):
        raise ValueError(
            "HUD counter-text constructor vptr bridge requires one exact "
            "authored SetTextFmt candidate identity without name collisions"
        )

    references = [
        relocation
        for relocation in caller.relocations
        if relocation.symbol_name.casefold() == vftable_casefold
    ]
    field_references = [
        relocation
        for relocation in caller.relocations
        if relocation.offset
        == _cc_catalog.HUD_COUNTER_TEXT_PANEL_VPTR_RELOCATION_OFFSET
    ]
    if (
        len(references) != 1
        or field_references != references
        or references[0].symbol_name != vftable_symbol
        or references[0].type != IMAGE_REL_I386_DIR32
        or caller.data[
            _cc_catalog.HUD_COUNTER_TEXT_PANEL_VPTR_STORE_OFFSET:
            _cc_catalog.HUD_COUNTER_TEXT_PANEL_VPTR_RELOCATION_OFFSET
        ]
        != b"\xc7\x06"
        or struct.unpack_from(
            "<I",
            caller.data,
            _cc_catalog.HUD_COUNTER_TEXT_PANEL_VPTR_RELOCATION_OFFSET,
        )[0]
        != 0
        or not all(
            caller.relocation_mask[index]
            for index in range(
                _cc_catalog.HUD_COUNTER_TEXT_PANEL_VPTR_RELOCATION_OFFSET,
                _cc_catalog.HUD_COUNTER_TEXT_PANEL_VPTR_RELOCATION_OFFSET + 4,
            )
        )
    ):
        raise ValueError(
            "HUD counter-text constructor vptr bridge requires one unique "
            "zero-addend fully masked C706/DIR32 same-object vptr store at "
            "+0x4b"
        )

    instruction_offsets = _cc_cfg._instruction_runtime_addresses(
        candidate.instructions,
        source="cod",
        caller_start=0,
    )
    offset_rows: dict[int, list[int]] = {}
    for index, offset in enumerate(instruction_offsets):
        if offset is not None:
            offset_rows.setdefault(offset, []).append(index)
    required_offsets = {
        _cc_catalog.HUD_COUNTER_TEXT_PANEL_VPTR_STORE_OFFSET,
        _cc_catalog.HUD_COUNTER_TEXT_PANEL_VPTR_LOAD_OFFSET,
        _cc_catalog.HUD_COUNTER_TEXT_PANEL_CALL_OFFSET,
    }
    if any(len(offset_rows.get(offset, ())) != 1 for offset in required_offsets):
        raise ValueError(
            "HUD counter-text constructor vptr bridge requires unique exact "
            "store, load, and call instruction offsets"
        )
    store_index = offset_rows[_cc_catalog.HUD_COUNTER_TEXT_PANEL_VPTR_STORE_OFFSET][0]
    load_index = offset_rows[_cc_catalog.HUD_COUNTER_TEXT_PANEL_VPTR_LOAD_OFFSET][0]
    call_index = offset_rows[_cc_catalog.HUD_COUNTER_TEXT_PANEL_CALL_OFFSET][0]
    store_instruction = candidate.instructions[store_index]
    load_instruction = candidate.instructions[load_index]
    call_instruction = candidate.instructions[call_index]
    if (
        tuple(item.lower() for item in store_instruction.bytes)
        != ("c7", "06", "00", "00", "00", "00")
        or tuple(item.lower() for item in load_instruction.bytes)
        != ("8b", "06")
        or tuple(item.lower() for item in call_instruction.bytes)
        != ("ff", "50", "74")
        or _cc_cfg._instruction_mnemonic(call_instruction) != "call"
        or _cc_targets._memory_slot(_cc_cfg._instruction_operand(call_instruction))[1]
        != _cc_catalog.HUD_COUNTER_TEXT_PANEL_SLOT_DISPLACEMENT
        or _cc_cfg._cleanup_after(candidate.instructions, call_index)
        != _cc_catalog.HUD_COUNTER_TEXT_PANEL_CLEANUP_BYTES
        or not store_index < load_index < call_index
    ):
        raise ValueError(
            "HUD counter-text constructor vptr bridge requires exact +0x4b "
            "C706 store, +0x7d 8B06 load, +0x8f FF5074 call, and 12-byte "
            "caller cleanup"
        )
    matching_slot_calls = [
        index
        for index, instruction in enumerate(candidate.instructions)
        if _cc_cfg._instruction_mnemonic(instruction) in {"call", "jmp"}
        and re.fullmatch(
            r"eax(?:\+0x74|\+116)",
            _cc_targets._exact_memory_expression(
                _cc_cfg._instruction_operand(instruction)
            ),
            flags=re.IGNORECASE,
        )
    ]
    if matching_slot_calls != [call_index]:
        raise ValueError(
            "HUD counter-text constructor vptr bridge requires one unique "
            "exact EAX slot-0x74 invocation"
        )

    if (
        any(
            _cc_cfg._instruction_mnemonic(instruction).startswith("j")
            or _cc_cfg._instruction_mnemonic(instruction)
            in {"loop", "loope", "loopne", "loopnz", "loopz"}
            for instruction in candidate.instructions[:call_index]
        )
        or any(
            target <= call_index
            for targets in candidate.local_control_flow_targets.values()
            for target in targets
        )
        or any(
            index <= call_index
            for index in candidate.local_control_flow_indices
        )
    ):
        raise ValueError(
            "HUD counter-text constructor vptr bridge rejects branches or "
            "alternate entries before the reviewed call"
        )
    vptr_write_mnemonics = {
        "adc", "add", "and", "btc", "btr", "bts", "cmpxchg", "dec",
        "inc", "mov", "neg", "not", "or", "sbb", "sub", "xchg", "xor",
    }
    provenance = _cc_cfg._empty_register_state()
    provenance.update(
        {
            "ecx": "this",
            "esp": "stack",
            "ebp": "frame",
        }
    )
    for index, instruction in enumerate(
        candidate.instructions[:call_index + 1]
    ):
        if (
            index in {store_index, load_index}
            and provenance.get("esi") != "this"
        ):
            raise ValueError(
                "HUD counter-text constructor vptr bridge rejects clobbered "
                "or non-this vptr base provenance"
            )
        if (
            index == call_index
            and provenance.get("eax") != "load(this)"
        ):
            raise ValueError(
                "HUD counter-text constructor vptr bridge rejects clobbered "
                "loaded dispatch provenance"
            )
        operands = [
            item.strip()
            for item in _cc_cfg._instruction_operand(instruction).split(",")
        ]
        if (
            store_index < index < call_index
            and
            _cc_cfg._instruction_mnemonic(instruction) in vptr_write_mnemonics
            and operands
            and "[" in operands[0]
        ):
            expression, displacement = _cc_targets._memory_slot(operands[0])
            base_match = re.fullmatch(
                r"(e?[abcd]x|e?[sd]i|e?[sb]p)",
                expression,
                flags=re.IGNORECASE,
            )
            base_provenance = (
                provenance.get(base_match.group(1).lower(), "")
                if base_match is not None
                else ""
            )
            if displacement in {None, 0} and base_provenance in {
                "this",
                "address(this)",
            }:
                raise ValueError(
                    "HUD counter-text constructor vptr bridge rejects an "
                    "overwritten vptr through a same-object alias on the "
                    "reaching path"
                )
        if index == call_index:
            continue
        if _cc_cfg._instruction_mnemonic(instruction) == "call":
            for volatile in ("eax", "ecx", "edx"):
                provenance[volatile] = ""
            continue
        _cc_receiver_storage._update_register_state(
            instruction,
            provenance,
            assembly_source="cod",
            indexes=indexes,
            reviewed_register_storage_bridges=(
                reviewed_register_storage_bridges
            ),
        )

    retail_bytes = _cc_cfg._hexdump_bytes(
        bridge.hexdump(
            storage_address,
            _cc_catalog.HUD_COUNTER_TEXT_PANEL_VFTABLE_SIZE,
        )
    )
    if (
        len(retail_bytes) != _cc_catalog.HUD_COUNTER_TEXT_PANEL_VFTABLE_SIZE
        or normalize_address(
            hex(
                storage_start
                + _cc_catalog.HUD_COUNTER_TEXT_PANEL_SLOT_DISPLACEMENT
            )
        )
        != _cc_catalog.HUD_COUNTER_TEXT_PANEL_RETAIL_SLOT_ADDRESS
    ):
        raise ValueError(
            "HUD counter-text constructor vptr bridge requires the exact "
            "complete retail vftable and absolute slot address"
        )
    retail_slot_target = normalize_address(
        hex(
            struct.unpack_from(
                "<I",
                retail_bytes,
                _cc_catalog.HUD_COUNTER_TEXT_PANEL_SLOT_DISPLACEMENT,
            )[0]
        )
    )
    slot_identity = slot_symbol_matches[0][1]
    retail_slot_addresses = [
        address
        for address, identity in indexes.by_address.items()
        if identity == slot_identity
    ]
    if (
        indexes.by_address.get(retail_slot_target) != slot_identity
        or retail_slot_addresses != [retail_slot_target]
    ):
        raise ValueError(
            "HUD counter-text constructor vptr bridge candidate SetTextFmt "
            "slot target does not exactly match live retail"
        )

    expected_rows = [
        row
        for row in expected
        if row.get("storage_identity") == storage_identity
    ]
    if (
        len(expected_rows) != 1
        or expected_rows[0]
        != {
            "ordinal": _cc_catalog.HUD_COUNTER_TEXT_PANEL_RETAIL_ORDINAL,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "callback",
            "target_identity": "",
            "storage_identity": storage_identity,
            "slot_displacement": (
                _cc_catalog.HUD_COUNTER_TEXT_PANEL_SLOT_DISPLACEMENT
            ),
            "cleanup_bytes": _cc_catalog.HUD_COUNTER_TEXT_PANEL_CLEANUP_BYTES,
        }
    ):
        raise ValueError(
            "HUD counter-text constructor vptr bridge requires the exact "
            "ordinal-2 indirect callback retail contract"
        )

    reviewed = ReviewedVptrStorageBridge(
        register="eax",
        provenance="load(this)",
        storage_identity=storage_identity,
        slot_displacement=_cc_catalog.HUD_COUNTER_TEXT_PANEL_SLOT_DISPLACEMENT,
    )
    result = {
        normalize_address(hex(_cc_catalog.HUD_COUNTER_TEXT_PANEL_CALL_OFFSET)): reviewed
    }
    provisional = _cc_extraction.extract_invocation_contract(
        candidate.instructions,
        source="cod",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge_names=bridge_names,
        compiler_generated_bridges=compiler_generated_bridges,
        candidate_storage_bridges=candidate_storage_bridges,
        reviewed_register_storage_bridges=(
            reviewed_register_storage_bridges
        ),
        reviewed_call_result_bridges=reviewed_call_result_bridges,
        reviewed_vptr_storage_bridges=result,
        local_control_flow_indices=candidate.local_control_flow_indices,
        local_control_flow_targets=candidate.local_control_flow_targets,
    )
    if (
        len(provisional) <= _cc_catalog.HUD_COUNTER_TEXT_PANEL_RETAIL_ORDINAL
        or provisional[_cc_catalog.HUD_COUNTER_TEXT_PANEL_RETAIL_ORDINAL]
        != expected_rows[0]
        or sum(
            row.get("storage_identity") == storage_identity
            for row in provisional
        )
        != 1
    ):
        raise ValueError(
            "HUD counter-text constructor vptr bridge does not produce one "
            "exact ordinal-2 candidate invocation"
        )
    return result
