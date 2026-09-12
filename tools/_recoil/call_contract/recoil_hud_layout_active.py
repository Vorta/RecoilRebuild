"""Recoil call-contract recoil hud layout active evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import extraction as _cc_extraction
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract import receiver_instructions as _cc_receiver_instructions
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        CandidateCallerDefinition,
        IdentityIndexes,
        ReviewedLoopVptrStorageBridge,
        ReviewedMemberVptrStorageBridge,
        ReviewedStaticStorageReferenceBridge,
    )

import struct
from pathlib import Path
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_DIR32,
    IMAGE_REL_I386_REL32,
    Instruction,
)
from _recoil.lib.authored_icf import exact_selected_target_membership
from _recoil.lib.progress import ProgressDocument, address_value, normalize_address


def _hud_layout_hw_set_active_candidate_variant(
    caller: CandidateCallerDefinition,
) -> str:
    """Select only one of the two reviewed complete SetActive candidates."""
    size = len(caller.data)
    if size == _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_CANDIDATE_SIZE:
        discriminator_relocations = tuple(
            (
                relocation.offset,
                relocation.type,
                relocation.symbol_name,
                struct.unpack_from("<I", caller.data, relocation.offset)[0],
            )
            for relocation in caller.relocations
            if relocation.offset
            in {0x49, 0xA2, 0xA7, 0x168, 0x16D}
            and relocation.offset + 4 <= size
        )
        expected_relocations = (
            (
                0x49,
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0xBC8,
            ),
            (
                0xA2,
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0x420,
            ),
            (
                0xA7,
                IMAGE_REL_I386_REL32,
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_NANITE_DIRECT_SYMBOL,
                0,
            ),
            (
                0x168,
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0x420,
            ),
            (
                0x16D,
                IMAGE_REL_I386_REL32,
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_NANITE_DIRECT_SYMBOL,
                0,
            ),
        )
        discriminator_mask = {
            position
            for position, masked in enumerate(caller.relocation_mask)
            if masked
            and (
                position in range(0x49, 0x4D)
                or position in range(0xA2, 0xA6)
                or position in range(0xA7, 0xAB)
                or position in range(0x168, 0x16C)
                or position in range(0x16D, 0x171)
            )
        }
        required_mask = {
            position
            for offset in (0x49, 0xA2, 0xA7, 0x168, 0x16D)
            for position in range(offset, offset + 4)
        }
        if (
            caller.data[0x40:0x47]
            == bytes.fromhex("8b 16 8b ce ff 52 18")
            and caller.data[0x47:0x61]
            == bytes.fromhex(
                "8b 0d c8 0b 00 00 "
                "8b 9e 28 01 00 00 "
                "8b be f0 01 00 00 "
                "6a 00 8b 01 53 ff 50 18"
            )
            and caller.data[0x9E:0xAB]
            == bytes.fromhex(
                "6a 00 57 b9 20 04 00 00 e8 00 00 00 00"
            )
            and caller.data[0x163:0x171]
            == bytes.fromhex(
                "6a 00 6a 00 b9 20 04 00 00 e8 00 00 00 00"
            )
            and discriminator_relocations == expected_relocations
            and discriminator_mask == required_mask
        ):
            return "active-first-direct-1d0"
        return "legacy-inactive-first-1d0"
    if size != _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_ACTIVE_FIRST_CANDIDATE_SIZE:
        raise ValueError(
            "HUD layout SetActive bridge requires one exact reviewed "
            "candidate extent"
        )
    padding_start = _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_ACTIVE_FIRST_BODY_END
    if (
        caller.data[0x1C3:padding_start]
        != bytes.fromhex(
            "5f 5e b8 01 00 00 00 5b 83 c4 14 c2 04 00"
        )
        or
        caller.data[padding_start:] != b"\x90" * (size - padding_start)
        or any(
            relocation.offset >= padding_start
            for relocation in caller.relocations
        )
        or any(caller.relocation_mask[padding_start:])
    ):
        raise ValueError(
            "HUD layout SetActive active-first bridge requires exact "
            "+0x1d1 semantic end and +0x1d1..+0x1e0 COFF NOP padding"
        )
    return "active-first-indirect-1e0"


def _hud_layout_hw_set_active_is_active_first_candidate(
    caller: CandidateCallerDefinition,
) -> bool:
    return (
        _hud_layout_hw_set_active_candidate_variant(caller)
        != "legacy-inactive-first-1d0"
    )


def _hud_layout_hw_set_active_require_active_first_lead(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> None:
    """Prove the corrected candidate's leading OnActivated virtual call."""
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge
    caller = candidate.caller_definition
    assert caller is not None
    start = _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_ACTIVE_FIRST_LEAD_START
    end = _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_ACTIVE_FIRST_LEAD_END
    call_offset = _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_ACTIVE_FIRST_LEAD_CALL_OFFSET
    if (
        caller.data[start:end] != bytes.fromhex("8b 16 8b ce ff 52 18")
        or any(start <= relocation.offset < end for relocation in caller.relocations)
        or any(caller.relocation_mask[start:end])
    ):
        raise ValueError(
            "HUD layout SetActive active-first bridge requires the exact "
            "OnActivated body, empty COFF set, and empty relocation mask"
        )

    candidate_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    by_offset: dict[int, Instruction] = {}
    index_by_offset: dict[int, int] = {}
    counts: dict[int, int] = {}
    for index, offset in enumerate(candidate_offsets):
        if offset is None:
            continue
        by_offset[offset] = candidate.instructions[index]
        index_by_offset[offset] = index
        counts[offset] = counts.get(offset, 0) + 1
    required_offsets = (0x40, 0x42, 0x44)
    if (
        {offset for offset in by_offset if start <= offset < end}
        != set(required_offsets)
        or any(counts.get(offset) != 1 for offset in required_offsets)
    ):
        raise ValueError(
            "HUD layout SetActive active-first bridge requires the exact "
            "OnActivated instruction topology"
        )
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    ordinal_by_index = {
        instruction_index: ordinal
        for ordinal, instruction_index in enumerate(invocation_indices)
    }
    call_index = index_by_offset[call_offset]
    expected_call = {
        "ordinal": 4,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": "load(this)",
        "slot_displacement": 0x18,
        "cleanup_bytes": None,
    }
    if (
        len(expected) <= 4
        or dict(expected[4]) != expected_call
        or ordinal_by_index.get(call_index) != 4
        or call_index in candidate.local_control_flow_indices
        or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
    ):
        raise ValueError(
            "HUD layout SetActive active-first bridge requires exact "
            "immutable/candidate OnActivated ordinal-4 semantics"
        )
    bridge = ReviewedLoopVptrStorageBridge(
        register="edx",
        storage_identity="load(this)",
        slot_displacement=0x18,
        assembly_source="cod",
    )
    structural = _cc_extraction.extract_invocation_contract(
        tuple(by_offset[offset] for offset in required_offsets),
        source="cod",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        reviewed_loop_vptr_storage_bridges={
            normalize_address(hex(call_offset)): bridge
        },
    )
    if structural != [{**expected_call, "ordinal": 0}]:
        raise ValueError(
            "HUD layout SetActive active-first bridge cannot independently "
            "derive immutable OnActivated equality"
        )


def _hud_layout_hw_set_active_objective_counter_candidate_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedStaticStorageReferenceBridge],
    dict[str, ReviewedLoopVptrStorageBridge],
]:
    """Prove SetActive's first objective-counter zero-source virtual call."""
    from _recoil.call_contract.records import (
        ReviewedLoopVptrStorageBridge,
        ReviewedStaticStorageReferenceBridge,
    )
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_CALLER_START:
        return {}, {}

    caller_symbol_id = (
        _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_CALLER_IDENTITY.removeprefix("symbol:")
    )
    caller_row = document.collection("symbols").get(caller_symbol_id)
    trace = (
        caller_row.get("source_traceability")
        if isinstance(caller_row, Mapping)
        else None
    )
    source_edges = (
        trace.get("source_edges") if isinstance(trace, Mapping) else None
    )
    caller = candidate.caller_definition
    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(
            target,
            "translation_unit_function_order",
            (),
        )
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_CALLER_START
    ]
    if (
        caller_identity != _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(normalized_start) != caller_identity
        or caller_identity in indexes.provider_ids
        or not isinstance(caller_row, Mapping)
        or caller_row.get("binary") != "recoil"
        or caller_row.get("kind") != "function"
        or caller_row.get("pipeline_class") != "authored"
        or caller_row.get("ownership_state") != "primary-owned"
        or caller_row.get("address") != normalized_start
        or caller_row.get("end_exclusive")
        != _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_CALLER_END_EXCLUSIVE
        or caller_row.get("extent_state") != "known"
        or caller_row.get("size") != 0x1E0
        or caller_row.get("navigation_name") != "HudLayoutHW::SetActive"
        or caller_row.get("output_section_id") != "recoil:section:.text"
        or caller_row.get("physical_block_id") != "recoil:block:0x404ca0"
        or not exact_selected_target_membership(
            caller_row.get("verification_target_ids", ()),
            "recoil:vc5-target:hud_404ca0_415ab0_authored_order",
        )
        or not isinstance(trace, Mapping)
        or trace.get("state") != "resolved"
        or trace.get("reason_code") not in {None, ""}
        or not isinstance(source_edges, list)
        or len(source_edges) != 1
        or not isinstance(source_edges[0], Mapping)
        or source_edges[0].get("relation") != "defines"
        or source_edges[0].get("anchor_id")
        != _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
        or caller is None
        or caller.symbol != _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_CALLER_SYMBOL
        or len(caller.data)
        not in {
            _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_CANDIDATE_SIZE,
            _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_ACTIVE_FIRST_CANDIDATE_SIZE,
        }
        or len(caller.relocation_mask) != len(caller.data)
        or target is None
        or getattr(target, "name", "") != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_TARGET_NAME
        or getattr(target, "target_binary", "") != "recoil"
        or Path(str(getattr(target, "manifest_path", ""))).resolve()
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_TARGET_MANIFEST.resolve()
        or not bool(
            getattr(target, "check_translation_unit_function_order", False)
        )
        or tuple(getattr(target, "source_files", ())).count(
            _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        )
        != 1
        or len(contribution_rows) != 1
    ):
        raise ValueError(
            "HUD layout SetActive objective-counter bridge requires the "
            "exact authored caller, source edge, extent, symbol, and target "
            "contribution authority"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != r"\?SetActive@HudLayoutHW@@.*"
        or getattr(contribution_row, "name", "")
        != "HudLayoutHW::SetActive"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD layout SetActive objective-counter bridge requires the "
            "exact authored hud.cpp contribution row"
        )
    candidate_variant = _hud_layout_hw_set_active_candidate_variant(caller)
    active_first = candidate_variant != "legacy-inactive-first-1d0"
    if active_first:
        _hud_layout_hw_set_active_require_active_first_lead(
            expected,
            candidate,
            caller_identity=caller_identity,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            indexes=indexes,
        )

    symbols = document.collection("symbols")
    storages = document.collection("storage_contributions")
    owners = document.collection("owners")
    targets = document.collection("verification_targets")
    aggregate_symbol = symbols.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID)
    aggregate_storage = storages.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_STORAGE_ID)
    aggregate_target = targets.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_ID)
    aggregate_registration = (
        aggregate_target.get("registration")
        if isinstance(aggregate_target, Mapping)
        else None
    )
    owner = owners.get(_cc_catalog.HUD_UI_MGR_OWNER_ID)
    gates = owner.get("gates") if isinstance(owner, Mapping) else None
    owner_relationships = [
        relationship
        for relationship in (
            owner.get("relationships", ())
            if isinstance(owner, Mapping)
            else ()
        )
        if isinstance(relationship, Mapping)
        and relationship.get("kind") == "primary-data"
        and relationship.get("symbol_id") == _cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID
    ]
    reference = (
        aggregate_storage.get("reference")
        if isinstance(aggregate_storage, Mapping)
        else None
    )
    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    aggregate_start = address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
    field_address = normalize_address(
        aggregate_start + _cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_DISPLACEMENT
    )
    exact_containers = [
        row
        for row in indexes.storage_containers
        if (
            row.identity == aggregate_identity
            and row.start == aggregate_start
            and row.end_exclusive
            == aggregate_start + _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
        )
    ]
    aggregate_addresses = [
        address
        for address, identity in indexes.storage_by_address.items()
        if identity == aggregate_identity
    ]
    if (
        not isinstance(aggregate_symbol, Mapping)
        or aggregate_symbol.get("binary") != "recoil"
        or aggregate_symbol.get("kind") != "data"
        or aggregate_symbol.get("address") != _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
        or aggregate_symbol.get("disposition") != "authored"
        or aggregate_symbol.get("navigation_name")
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_NAME
        or aggregate_symbol.get("output_section_id") != "recoil:section:.data"
        or aggregate_symbol.get("storage_contribution_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_STORAGE_ID]
        or aggregate_symbol.get("verification_target_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_ID]
        or aggregate_symbol.get("extent_state") != "unknown"
        or not isinstance(aggregate_storage, Mapping)
        or aggregate_storage.get("binary") != "recoil"
        or aggregate_storage.get("kind") != "data-symbol"
        or aggregate_storage.get("output_section_id")
        != "recoil:section:.data"
        or aggregate_storage.get("overlap") != "none"
        or aggregate_storage.get("owner_ids") != [_cc_catalog.HUD_UI_MGR_OWNER_ID]
        or aggregate_storage.get("parent_contribution_id") is not None
        or aggregate_storage.get("symbol_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID]
        or not isinstance(reference, Mapping)
        or reference.get("address") != _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
        or reference.get("extent_state") != "unknown"
        or not isinstance(aggregate_target, Mapping)
        or aggregate_target.get("binary") != "recoil"
        or aggregate_target.get("kind") != "vc5"
        or aggregate_target.get("name") != "hud_ui_mgr_data"
        or _cc_targets._registered_target_artifact_ids(
            aggregate_target, document.collection("symbols")
        )
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID]
        or not isinstance(aggregate_registration, Mapping)
        or aggregate_registration.get("manifest_path")
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_MANIFEST
        or aggregate_registration.get("data_addresses")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS]
        or not isinstance(owner, Mapping)
        or owner.get("binary") != "recoil"
        or owner.get("kind") != "data-owner"
        or owner.get("provider_state") == "accepted"
        or not isinstance(gates, Mapping)
        or any(
            gates.get(gate) != "accepted"
            for gate in ("boundary", "source", "data", "owner_linkage")
        )
        or owner_relationships
        != [
            {
                "kind": "primary-data",
                "symbol_id": _cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID,
                "address": _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS,
                "name": _cc_catalog.HUD_UI_MGR_AGGREGATE_NAME,
            }
        ]
        or indexes.storage_by_address.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
        != aggregate_identity
        or aggregate_addresses != [_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS]
        or len(exact_containers) != 1
        or aggregate_identity in indexes.provider_ids
        or field_address != _cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_ADDRESS
        or indexes.storage_by_address.get(field_address, "") != ""
        or (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_DISPLACEMENT
            + _cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_ACCESS_WIDTH
            > _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
        )
    ):
        raise ValueError(
            "HUD layout SetActive objective-counter bridge requires the "
            "exact reviewed g_HudUiMgr aggregate and bounded uncatalogued "
            "+0xbc8 field authority"
        )

    field_provenance = _cc_receiver_instructions._abstract_with_displacement(
        aggregate_identity,
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_DISPLACEMENT,
    )
    expected_storage = f"load(load({field_provenance}))"
    expected_call = {
        "ordinal": _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_RETAIL_CALL_ORDINAL,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": expected_storage,
        "slot_displacement": _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_SLOT,
        "cleanup_bytes": None,
    }
    if (
        len(expected) <= _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_RETAIL_CALL_ORDINAL
        or dict(
            expected[_cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_RETAIL_CALL_ORDINAL]
        )
        != expected_call
    ):
        raise ValueError(
            "HUD layout SetActive objective-counter bridge requires the "
            "exact immutable retail ordinal-5 objective-counter "
            "slot-0x18/no-cleanup contract; observed="
            f"{(
                dict(
                    expected[
                        _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_RETAIL_CALL_ORDINAL
                    ]
                )
                if len(expected)
                > _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_RETAIL_CALL_ORDINAL
                else None
            )!r}"
        )

    if active_first:
        unit_start = 0x47
        unit_end = 0x61
        relocation_offset = 0x49
        call_offset = 0x5E
        offsets = (0x47, 0x4D, 0x53, 0x59, 0x5B, 0x5D, 0x5E)
        unit_relocations = [
            relocation
            for relocation in caller.relocations
            if unit_start <= relocation.offset < unit_end
        ]
        expected_mask = {
            position
            for relocation in caller.relocations
            for position in range(relocation.offset, relocation.offset + 4)
            if relocation.offset + 4 <= len(caller.data)
        }
        observed_mask = {
            index
            for index, masked in enumerate(caller.relocation_mask)
            if masked
        }
        if (
            len(unit_relocations) != 1
            or unit_relocations[0].offset != relocation_offset
            or unit_relocations[0].type != IMAGE_REL_I386_DIR32
            or unit_relocations[0].symbol_name
            != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
            or struct.unpack_from("<I", caller.data, relocation_offset)[0]
            != _cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_DISPLACEMENT
            or caller.undefined_external_data.count(
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
            )
            != 1
            or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
            in (
                caller.defined_external_data
                + caller.undefined_external_functions
                + caller.defined_external_functions
            )
            or any(
                relocation.offset < 0
                or relocation.offset + 4 > len(caller.data)
                for relocation in caller.relocations
            )
            or observed_mask != expected_mask
            or {
                position
                for position in observed_mask
                if unit_start <= position < unit_end
            }
            != set(range(relocation_offset, relocation_offset + 4))
        ):
            raise ValueError(
                "HUD layout SetActive active-first objective bridge requires "
                "the exact +0x49 COFF relocation and complete caller mask"
            )
        if caller.data[unit_start:unit_end] != bytes.fromhex(
            "8b 0d c8 0b 00 00 "
            "8b 9e 28 01 00 00 "
            "8b be f0 01 00 00 "
            "6a 00 8b 01 53 ff 50 18"
        ):
            raise ValueError(
                "HUD layout SetActive active-first objective bridge requires "
                "the exact active objective body"
            )
        candidate_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
        by_offset: dict[int, Instruction] = {}
        index_by_offset: dict[int, int] = {}
        counts: dict[int, int] = {}
        for index, offset in enumerate(candidate_offsets):
            if offset is None:
                continue
            by_offset[offset] = candidate.instructions[index]
            index_by_offset[offset] = index
            counts[offset] = counts.get(offset, 0) + 1
        this_origin = by_offset.get(0x06)
        if (
            {offset for offset in by_offset if unit_start <= offset < unit_end}
            != set(offsets)
            or any(counts.get(offset) != 1 for offset in offsets)
            or this_origin is None
            or counts.get(0x06) != 1
            or bytes(int(value, 16) for value in this_origin.bytes)
            != b"\x8b\xf1"
            or _cc_cfg._instruction_operand(this_origin).strip().lower()
            != "esi, ecx"
        ):
            raise ValueError(
                "HUD layout SetActive active-first objective bridge requires "
                "the exact instruction topology and this origin"
            )
        invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
            candidate,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
        )
        ordinal_by_index = {
            instruction_index: ordinal
            for ordinal, instruction_index in enumerate(invocation_indices)
        }
        call_index = index_by_offset[call_offset]
        if (
            ordinal_by_index.get(call_index) != 5
            or call_index in candidate.local_control_flow_indices
            or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
        ):
            raise ValueError(
                "HUD layout SetActive active-first objective bridge rejects "
                "candidate ordinal, cleanup, or CFG drift"
            )
        candidate_expression = (
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
            f"+{_cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_DISPLACEMENT}"
        )
        static_bridge = ReviewedStaticStorageReferenceBridge(
            aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            displacement=_cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_DISPLACEMENT,
            access_width=_cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_ACCESS_WIDTH,
            storage_identity=field_provenance,
        )
        callsite_bridge = ReviewedLoopVptrStorageBridge(
            register="eax",
            storage_identity=expected_storage,
            slot_displacement=_cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_SLOT,
            assembly_source="cod",
        )
        structural = _cc_extraction.extract_invocation_contract(
            tuple(by_offset[offset] for offset in (0x47, 0x59, 0x5B, 0x5D, 0x5E)),
            source="cod",
            caller_identity=caller_identity,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            indexes=indexes,
            reviewed_static_storage_reference_bridges={
                candidate_expression: static_bridge
            },
            reviewed_loop_vptr_storage_bridges={
                normalize_address(hex(call_offset)): callsite_bridge
            },
        )
        if structural != [{**expected_call, "ordinal": 0}]:
            raise ValueError(
                "HUD layout SetActive active-first objective bridge cannot "
                "independently derive immutable retail equality"
            )
        return (
            {candidate_expression: static_bridge},
            {normalize_address(hex(call_offset)): callsite_bridge},
        )

    unit_start = _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_LOAD_OFFSET
    unit_end = _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_CALL_OFFSET + 3
    unit_relocations = [
        relocation
        for relocation in caller.relocations
        if unit_start <= relocation.offset < unit_end
    ]
    if (
        len(unit_relocations) != 1
        or unit_relocations[0].offset
        != _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_RELOCATION_OFFSET
        or unit_relocations[0].type != IMAGE_REL_I386_DIR32
        or unit_relocations[0].symbol_name
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        or unit_relocations[0].offset + 4 > len(caller.data)
        or struct.unpack_from(
            "<I",
            caller.data,
            unit_relocations[0].offset,
        )[0]
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_DISPLACEMENT
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in (
            caller.defined_external_data
            + caller.undefined_external_functions
            + caller.defined_external_functions
        )
    ):
        raise ValueError(
            "HUD layout SetActive objective-counter bridge requires the "
            "unique +0x42 DIR32 g_HudUiMgr+0xbc8 relocation and exact "
            "undefined-data symbol class"
        )
    expected_mask = {
        position
        for relocation in caller.relocations
        for position in range(relocation.offset, relocation.offset + 4)
        if relocation.offset + 4 <= len(caller.data)
    }
    observed_mask = {
        index
        for index, masked in enumerate(caller.relocation_mask)
        if masked
    }
    if (
        any(
            relocation.offset < 0
            or relocation.offset + 4 > len(caller.data)
            for relocation in caller.relocations
        )
        or observed_mask != expected_mask
        or {
            position
            for position in observed_mask
            if unit_start <= position < unit_end
        }
        != set(
            range(
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_RELOCATION_OFFSET,
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_RELOCATION_OFFSET + 4,
            )
        )
    ):
        raise ValueError(
            "HUD layout SetActive objective-counter bridge requires the "
            "exact candidate COFF relocation mask"
        )

    exact_unit = bytes.fromhex(
        "8b 0d c8 0b 00 00 "
        "6a 00 "
        "6a 00 "
        "8b 11 "
        "ff 52 18"
    )
    if caller.data[unit_start:unit_end] != exact_unit:
        raise ValueError(
            "HUD layout SetActive objective-counter bridge requires the "
            "exact ECX-load/two-zero/EDX-vptr/slot-0x18 candidate bytes"
        )

    candidate_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    offset_counts: dict[int, int] = {}
    candidate_by_offset: dict[int, Instruction] = {}
    candidate_index_by_offset: dict[int, int] = {}
    for index, offset in enumerate(candidate_offsets):
        if offset is None:
            continue
        offset_counts[offset] = offset_counts.get(offset, 0) + 1
        candidate_by_offset[offset] = candidate.instructions[index]
        candidate_index_by_offset[offset] = index
    required_instruction_offsets = {
        _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_LOAD_OFFSET,
        _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_FIRST_ZERO_OFFSET,
        _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_SECOND_ZERO_OFFSET,
        _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_VPTR_OFFSET,
        _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_CALL_OFFSET,
    }
    observed_unit_instruction_offsets = {
        offset
        for offset in candidate_by_offset
        if unit_start <= offset < unit_end
    }
    if (
        observed_unit_instruction_offsets != required_instruction_offsets
        or any(offset_counts.get(offset) != 1 for offset in required_instruction_offsets)
    ):
        raise ValueError(
            "HUD layout SetActive objective-counter bridge requires the "
            "exact candidate +0x40..+0x4c instruction topology"
        )

    def encoded_at(offset: int) -> bytes:
        return bytes(
            int(value, 16)
            for value in candidate_by_offset[offset].bytes
        )

    if (
        _cc_cfg._instruction_mnemonic(
            candidate_by_offset[
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_FIRST_ZERO_OFFSET
            ]
        )
        != "push"
        or _cc_cfg._instruction_operand(
            candidate_by_offset[
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_FIRST_ZERO_OFFSET
            ]
        ).strip()
        not in {"0", "0x0"}
        or encoded_at(
            _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_FIRST_ZERO_OFFSET
        )
        != b"\x6a\x00"
        or _cc_cfg._instruction_mnemonic(
            candidate_by_offset[
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_SECOND_ZERO_OFFSET
            ]
        )
        != "push"
        or _cc_cfg._instruction_operand(
            candidate_by_offset[
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_SECOND_ZERO_OFFSET
            ]
        ).strip()
        not in {"0", "0x0"}
        or encoded_at(
            _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_SECOND_ZERO_OFFSET
        )
        != b"\x6a\x00"
        or _cc_cfg._instruction_mnemonic(
            candidate_by_offset[
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_VPTR_OFFSET
            ]
        )
        != "mov"
        or _cc_cfg._instruction_operand(
            candidate_by_offset[
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_VPTR_OFFSET
            ]
        ).strip().lower()
        not in {"edx, dword [ecx]", "edx, dword ptr [ecx]"}
        or encoded_at(_cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_VPTR_OFFSET)
        != b"\x8b\x11"
        or _cc_cfg._instruction_mnemonic(
            candidate_by_offset[
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_CALL_OFFSET
            ]
        )
        != "call"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(
                candidate_by_offset[
                    _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_CALL_OFFSET
                ]
            )
        )
        not in {"edx+24", "edx+0x18"}
        or encoded_at(_cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_CALL_OFFSET)
        != b"\xff\x52\x18"
    ):
        raise ValueError(
            "HUD layout SetActive objective-counter bridge rejects "
            "receiver, vptr reaching-definition, zero-argument, or virtual "
            "slot drift"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    call_index = candidate_index_by_offset[
        _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_CALL_OFFSET
    ]
    ordinal_by_index = {
        instruction_index: ordinal
        for ordinal, instruction_index in enumerate(invocation_indices)
    }
    if (
        ordinal_by_index.get(call_index)
        != _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_CANDIDATE_CALL_ORDINAL
        or call_index in candidate.local_control_flow_indices
        or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
    ):
        raise ValueError(
            "HUD layout SetActive objective-counter bridge rejects candidate "
            "call order, local-control-flow classification, or cleanup drift"
        )

    candidate_expression = (
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
        f"+{_cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_DISPLACEMENT}"
    )
    static_bridge = ReviewedStaticStorageReferenceBridge(
        aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
        displacement=_cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_DISPLACEMENT,
        access_width=_cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_ACCESS_WIDTH,
        storage_identity=field_provenance,
    )
    callsite_bridge = ReviewedLoopVptrStorageBridge(
        register="edx",
        storage_identity=expected_storage,
        slot_displacement=_cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_SLOT,
        assembly_source="cod",
    )
    structural_contract = _cc_extraction.extract_invocation_contract(
        tuple(
            candidate_by_offset[offset]
            for offset in (
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_FIRST_ZERO_OFFSET,
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_SECOND_ZERO_OFFSET,
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_VPTR_OFFSET,
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_CALL_OFFSET,
            )
        ),
        source="cod",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        reviewed_loop_vptr_storage_bridges={
            normalize_address(
                hex(_cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_CALL_OFFSET)
            ): callsite_bridge
        },
    )
    normalized_expected_call = {
        **expected_call,
        "ordinal": 0,
    }
    if structural_contract != [normalized_expected_call]:
        raise ValueError(
            "HUD layout SetActive objective-counter bridge cannot "
            "independently derive equality with the immutable retail "
            "virtual-call contract"
        )
    return (
        {candidate_expression: static_bridge},
        {
            normalize_address(
                hex(_cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_CALL_OFFSET)
            ): callsite_bridge
        },
    )


def _hud_layout_hw_set_active_timer_panel_candidate_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedStaticStorageReferenceBridge],
    dict[str, ReviewedMemberVptrStorageBridge],
]:
    """Prove only SetActive's first TimerPanel slot-0x18 invocation."""
    from _recoil.call_contract.records import (
        ReviewedMemberVptrStorageBridge,
        ReviewedStaticStorageReferenceBridge,
    )
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_CALLER_START:
        return {}, {}

    # The preceding bridge is the exact shared caller/source/aggregate
    # prerequisite.  Its returned bridges are deliberately not reused here:
    # this unit independently supplies its TimerPanel field and vptr bridges.
    (
        objective_static_bridges,
        objective_vptr_bridges,
    ) = _hud_layout_hw_set_active_objective_counter_candidate_bridges(
        expected,
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    caller = candidate.caller_definition
    assert caller is not None  # Proved by the objective prerequisite.
    active_first = _hud_layout_hw_set_active_is_active_first_candidate(caller)
    objective_call_offset = (
        0x5E
        if active_first
        else _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_CALL_OFFSET
    )
    if (
        set(objective_static_bridges)
        != {
            (
                f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
                f"+{_cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_DISPLACEMENT}"
            )
        }
        or set(objective_vptr_bridges)
        != {
            normalize_address(hex(objective_call_offset))
        }
    ):
        raise ValueError(
            "HUD layout SetActive TimerPanel bridge requires WSI-013's "
            "exact objective-counter caller and aggregate prerequisite"
        )

    symbols = document.collection("symbols")
    storage_rows = document.collection("storage_contributions")
    targets = document.collection("verification_targets")
    timer_symbol = symbols.get(_cc_catalog.HUD_UI_MGR_TIMER_PANEL_SYMBOL_ID)
    timer_storage = storage_rows.get(_cc_catalog.HUD_UI_MGR_TIMER_PANEL_STORAGE_ID)
    timer_target = targets.get(_cc_catalog.HUD_UI_MGR_TIMER_PANEL_TARGET_ID)
    timer_registration = (
        timer_target.get("registration")
        if isinstance(timer_target, Mapping)
        else None
    )
    if (
        not isinstance(timer_symbol, Mapping)
        or _cc_identity._symbol_identity(
            _cc_catalog.HUD_UI_MGR_TIMER_PANEL_SYMBOL_ID,
            timer_symbol,
        )
        != f"symbol:{_cc_catalog.HUD_UI_MGR_TIMER_PANEL_SYMBOL_ID}"
        or timer_symbol.get("binary") != "recoil"
        or timer_symbol.get("kind") != "data"
        or timer_symbol.get("disposition") != "authored"
        or timer_symbol.get("navigation_name") != "g_HudUiMgrTimerPanel"
        or timer_symbol.get("extent_state") != "unknown"
        or normalize_address(str(timer_symbol.get("address", "")))
        != _cc_catalog.HUD_UI_MGR_TIMER_PANEL_ADDRESS
        or timer_symbol.get("output_section_id") != "recoil:section:.data"
        or timer_symbol.get("storage_contribution_ids")
        != [_cc_catalog.HUD_UI_MGR_TIMER_PANEL_STORAGE_ID]
        or timer_symbol.get("verification_target_ids")
        != [_cc_catalog.HUD_UI_MGR_TIMER_PANEL_TARGET_ID]
        or not isinstance(timer_storage, Mapping)
        or timer_storage.get("binary") != "recoil"
        or timer_storage.get("kind") != "data-symbol"
        or timer_storage.get("output_section_id") != "recoil:section:.data"
        or timer_storage.get("overlap") != "none"
        or timer_storage.get("parent_contribution_id") is not None
        or timer_storage.get("owner_ids")
        != ["recoil:owner:hud_ui.hud_ui_timer_panel_class"]
        or timer_storage.get("symbol_ids")
        != [_cc_catalog.HUD_UI_MGR_TIMER_PANEL_SYMBOL_ID]
        or timer_storage.get("reference")
        != {
            "address": _cc_catalog.HUD_UI_MGR_TIMER_PANEL_ADDRESS,
            "evidence_ids": [],
            "extent_state": "unknown",
        }
        or not isinstance(timer_target, Mapping)
        or timer_target.get("binary") != "recoil"
        or timer_target.get("kind") != "vc5"
        or timer_target.get("name")
        != "hud_ui_timer_panel_global_accessors_data"
        or _cc_targets._registered_target_artifact_ids(
            timer_target, document.collection("symbols")
        )
        != [_cc_catalog.HUD_UI_MGR_TIMER_PANEL_SYMBOL_ID, "recoil:data:0x4ed4e0"]
        or not isinstance(timer_registration, Mapping)
        or timer_registration.get("manifest_path")
        != (
            "tools/vc5_verify_targets/"
            "hud_ui_timer_panel_global_accessors_data.json"
        )
        or timer_registration.get("source_from")
        != "src/GameZRecoil/zUI/zui_widgets.cpp"
        or timer_registration.get("data_addresses")
        != [_cc_catalog.HUD_UI_MGR_TIMER_PANEL_ADDRESS, "0x4ed4e0"]
        or indexes.storage_by_address.get(_cc_catalog.HUD_UI_MGR_TIMER_PANEL_ADDRESS)
        != _cc_catalog.HUD_UI_MGR_TIMER_PANEL_STORAGE_IDENTITY
        or _cc_catalog.HUD_UI_MGR_TIMER_PANEL_STORAGE_IDENTITY in indexes.provider_ids
    ):
        raise ValueError(
            "HUD layout SetActive TimerPanel bridge requires the exact "
            "reviewed TimerPanel data/storage/verification-target authority"
        )

    aggregate_start = address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
    if (
        normalize_address(
            aggregate_start + _cc_catalog.HUD_UI_MGR_TIMER_PANEL_DISPLACEMENT
        )
        != _cc_catalog.HUD_UI_MGR_TIMER_PANEL_ADDRESS
    ):
        raise ValueError(
            "HUD layout SetActive TimerPanel bridge requires the exact "
            "g_HudUiMgr+0x4784 TimerPanel address relationship"
        )

    expected_storage = f"load({_cc_catalog.HUD_UI_MGR_TIMER_PANEL_STORAGE_IDENTITY})"
    expected_call = {
        "ordinal": _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_RETAIL_CALL_ORDINAL,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": expected_storage,
        "slot_displacement": _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_SLOT,
        "cleanup_bytes": None,
    }
    if (
        len(expected) <= _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_RETAIL_CALL_ORDINAL
        or dict(
            expected[_cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_RETAIL_CALL_ORDINAL]
        )
        != expected_call
    ):
        raise ValueError(
            "HUD layout SetActive TimerPanel bridge requires the exact "
            "immutable retail ordinal-6 TimerPanel slot-0x18/no-cleanup "
            "contract"
        )

    if active_first:
        unit_start = 0x61
        unit_end = 0x6F
        relocation_offset = 0x63
        call_offset = 0x6C
        offsets = (0x61, 0x67, 0x69, 0x6A, 0x6C)
        unit_relocations = [
            relocation
            for relocation in caller.relocations
            if unit_start <= relocation.offset < unit_end
        ]
        expected_mask = {
            position
            for relocation in caller.relocations
            for position in range(relocation.offset, relocation.offset + 4)
            if relocation.offset + 4 <= len(caller.data)
        }
        observed_mask = {
            index
            for index, masked in enumerate(caller.relocation_mask)
            if masked
        }
        if (
            len(unit_relocations) != 1
            or unit_relocations[0].offset != relocation_offset
            or unit_relocations[0].type != IMAGE_REL_I386_DIR32
            or unit_relocations[0].symbol_name
            != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
            or struct.unpack_from("<I", caller.data, relocation_offset)[0]
            != _cc_catalog.HUD_UI_MGR_TIMER_PANEL_DISPLACEMENT
            or any(
                relocation.offset < 0
                or relocation.offset + 4 > len(caller.data)
                for relocation in caller.relocations
            )
            or observed_mask != expected_mask
            or {
                position
                for position in observed_mask
                if unit_start <= position < unit_end
            }
            != set(range(relocation_offset, relocation_offset + 4))
        ):
            raise ValueError(
                "HUD layout SetActive active-first TimerPanel bridge requires "
                "the exact +0x63 COFF relocation and complete caller mask"
            )
        if caller.data[unit_start:unit_end] != bytes.fromhex(
            "8b 0d 84 47 00 00 6a 00 53 8b 11 ff 52 18"
        ):
            raise ValueError(
                "HUD layout SetActive active-first TimerPanel bridge requires "
                "the exact active TimerPanel body"
            )
        candidate_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
        by_offset: dict[int, Instruction] = {}
        index_by_offset: dict[int, int] = {}
        counts: dict[int, int] = {}
        for index, offset in enumerate(candidate_offsets):
            if offset is None:
                continue
            by_offset[offset] = candidate.instructions[index]
            index_by_offset[offset] = index
            counts[offset] = counts.get(offset, 0) + 1
        if (
            {offset for offset in by_offset if unit_start <= offset < unit_end}
            != set(offsets)
            or any(counts.get(offset) != 1 for offset in offsets)
        ):
            raise ValueError(
                "HUD layout SetActive active-first TimerPanel bridge requires "
                "the exact instruction topology"
            )
        invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
            candidate,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
        )
        ordinal_by_index = {
            instruction_index: ordinal
            for ordinal, instruction_index in enumerate(invocation_indices)
        }
        call_index = index_by_offset[call_offset]
        if (
            ordinal_by_index.get(call_index) != 6
            or call_index in candidate.local_control_flow_indices
            or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
        ):
            raise ValueError(
                "HUD layout SetActive active-first TimerPanel bridge rejects "
                "candidate ordinal, cleanup, or CFG drift"
            )
        candidate_expression = (
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
            f"+{_cc_catalog.HUD_UI_MGR_TIMER_PANEL_DISPLACEMENT}"
        )
        static_bridge = ReviewedStaticStorageReferenceBridge(
            aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            displacement=_cc_catalog.HUD_UI_MGR_TIMER_PANEL_DISPLACEMENT,
            access_width=_cc_catalog.HUD_UI_MGR_TIMER_PANEL_ACCESS_WIDTH,
            storage_identity=_cc_catalog.HUD_UI_MGR_TIMER_PANEL_STORAGE_IDENTITY,
        )
        member_bridge = ReviewedMemberVptrStorageBridge(
            register="edx",
            source_register="ecx",
            source_provenance=_cc_catalog.HUD_UI_MGR_TIMER_PANEL_STORAGE_IDENTITY,
            receiver_register="ecx",
            receiver_provenance=_cc_catalog.HUD_UI_MGR_TIMER_PANEL_STORAGE_IDENTITY,
            storage_identity=expected_storage,
            slot_displacement=_cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_SLOT,
            call_address=normalize_address(hex(call_offset)),
        )
        structural = _cc_extraction.extract_invocation_contract(
            tuple(by_offset[offset] for offset in offsets),
            source="cod",
            caller_identity=caller_identity,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            indexes=indexes,
            reviewed_static_storage_reference_bridges={
                candidate_expression: static_bridge
            },
            reviewed_member_vptr_storage_bridges={
                normalize_address("0x6a"): member_bridge
            },
        )
        if structural != [{**expected_call, "ordinal": 0}]:
            raise ValueError(
                "HUD layout SetActive active-first TimerPanel bridge cannot "
                "independently derive immutable retail equality"
            )
        return (
            {candidate_expression: static_bridge},
            {normalize_address("0x6a"): member_bridge},
        )

    unit_start = _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_LOAD_OFFSET
    unit_end = _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_CALL_OFFSET + 3
    unit_relocations = [
        relocation
        for relocation in caller.relocations
        if unit_start <= relocation.offset < unit_end
    ]
    if (
        len(unit_relocations) != 1
        or unit_relocations[0].offset
        != _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_RELOCATION_OFFSET
        or unit_relocations[0].type != IMAGE_REL_I386_DIR32
        or unit_relocations[0].symbol_name
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        or unit_relocations[0].offset + 4 > len(caller.data)
        or struct.unpack_from(
            "<I",
            caller.data,
            unit_relocations[0].offset,
        )[0]
        != _cc_catalog.HUD_UI_MGR_TIMER_PANEL_DISPLACEMENT
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in (
            caller.defined_external_data
            + caller.undefined_external_functions
            + caller.defined_external_functions
        )
    ):
        raise ValueError(
            "HUD layout SetActive TimerPanel bridge requires the unique "
            "+0x51 DIR32 g_HudUiMgr+0x4784 relocation and exact "
            "undefined-data symbol class"
        )
    expected_mask = {
        position
        for relocation in caller.relocations
        for position in range(relocation.offset, relocation.offset + 4)
        if relocation.offset + 4 <= len(caller.data)
    }
    observed_mask = {
        index
        for index, masked in enumerate(caller.relocation_mask)
        if masked
    }
    if (
        any(
            relocation.offset < 0
            or relocation.offset + 4 > len(caller.data)
            for relocation in caller.relocations
        )
        or observed_mask != expected_mask
        or {
            position
            for position in observed_mask
            if unit_start <= position < unit_end
        }
        != set(
            range(
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_RELOCATION_OFFSET,
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_RELOCATION_OFFSET + 4,
            )
        )
    ):
        raise ValueError(
            "HUD layout SetActive TimerPanel bridge requires the exact "
            "candidate COFF relocation mask"
        )

    exact_unit = bytes.fromhex(
        "8b 0d 84 47 00 00 "
        "6a 00 "
        "6a 00 "
        "8b 01 "
        "ff 50 18"
    )
    if caller.data[unit_start:unit_end] != exact_unit:
        raise ValueError(
            "HUD layout SetActive TimerPanel bridge requires the exact "
            "ECX-load/two-zero/EAX-vptr/slot-0x18 candidate bytes"
        )

    candidate_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    offset_counts: dict[int, int] = {}
    candidate_by_offset: dict[int, Instruction] = {}
    candidate_index_by_offset: dict[int, int] = {}
    for index, offset in enumerate(candidate_offsets):
        if offset is None:
            continue
        offset_counts[offset] = offset_counts.get(offset, 0) + 1
        candidate_by_offset[offset] = candidate.instructions[index]
        candidate_index_by_offset[offset] = index
    required_instruction_offsets = {
        _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_LOAD_OFFSET,
        _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_FIRST_ZERO_OFFSET,
        _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_SECOND_ZERO_OFFSET,
        _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_VPTR_OFFSET,
        _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_CALL_OFFSET,
    }
    observed_unit_instruction_offsets = {
        offset
        for offset in candidate_by_offset
        if unit_start <= offset < unit_end
    }
    if (
        observed_unit_instruction_offsets != required_instruction_offsets
        or any(
            offset_counts.get(offset) != 1
            for offset in required_instruction_offsets
        )
    ):
        raise ValueError(
            "HUD layout SetActive TimerPanel bridge requires the exact "
            "candidate +0x4f..+0x5b instruction topology"
        )

    def encoded_at(offset: int) -> bytes:
        return bytes(
            int(value, 16)
            for value in candidate_by_offset[offset].bytes
        )

    if (
        _cc_cfg._instruction_mnemonic(
            candidate_by_offset[
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_FIRST_ZERO_OFFSET
            ]
        )
        != "push"
        or _cc_cfg._instruction_operand(
            candidate_by_offset[
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_FIRST_ZERO_OFFSET
            ]
        ).strip()
        not in {"0", "0x0"}
        or encoded_at(_cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_FIRST_ZERO_OFFSET)
        != b"\x6a\x00"
        or _cc_cfg._instruction_mnemonic(
            candidate_by_offset[
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_SECOND_ZERO_OFFSET
            ]
        )
        != "push"
        or _cc_cfg._instruction_operand(
            candidate_by_offset[
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_SECOND_ZERO_OFFSET
            ]
        ).strip()
        not in {"0", "0x0"}
        or encoded_at(_cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_SECOND_ZERO_OFFSET)
        != b"\x6a\x00"
        or _cc_cfg._instruction_mnemonic(
            candidate_by_offset[
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_VPTR_OFFSET
            ]
        )
        != "mov"
        or _cc_cfg._instruction_operand(
            candidate_by_offset[
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_VPTR_OFFSET
            ]
        ).strip().lower()
        not in {"eax, dword [ecx]", "eax, dword ptr [ecx]"}
        or encoded_at(_cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_VPTR_OFFSET)
        != b"\x8b\x01"
        or _cc_cfg._instruction_mnemonic(
            candidate_by_offset[
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_CALL_OFFSET
            ]
        )
        != "call"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(
                candidate_by_offset[
                    _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_CALL_OFFSET
                ]
            )
        )
        not in {"eax+24", "eax+0x18"}
        or encoded_at(_cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_CALL_OFFSET)
        != b"\xff\x50\x18"
    ):
        raise ValueError(
            "HUD layout SetActive TimerPanel bridge rejects receiver, "
            "vptr reaching-definition, zero-argument, or virtual-slot drift"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    call_index = candidate_index_by_offset[
        _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_CALL_OFFSET
    ]
    ordinal_by_index = {
        instruction_index: ordinal
        for ordinal, instruction_index in enumerate(invocation_indices)
    }
    if (
        ordinal_by_index.get(call_index)
        != _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_CANDIDATE_CALL_ORDINAL
        or call_index in candidate.local_control_flow_indices
        or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
    ):
        raise ValueError(
            "HUD layout SetActive TimerPanel bridge rejects candidate call "
            "order, local-control-flow classification, or cleanup drift"
        )

    candidate_expression = (
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
        f"+{_cc_catalog.HUD_UI_MGR_TIMER_PANEL_DISPLACEMENT}"
    )
    static_bridge = ReviewedStaticStorageReferenceBridge(
        aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
        displacement=_cc_catalog.HUD_UI_MGR_TIMER_PANEL_DISPLACEMENT,
        access_width=_cc_catalog.HUD_UI_MGR_TIMER_PANEL_ACCESS_WIDTH,
        storage_identity=_cc_catalog.HUD_UI_MGR_TIMER_PANEL_STORAGE_IDENTITY,
    )
    member_bridge = ReviewedMemberVptrStorageBridge(
        register="eax",
        source_register="ecx",
        source_provenance=_cc_catalog.HUD_UI_MGR_TIMER_PANEL_STORAGE_IDENTITY,
        receiver_register="ecx",
        receiver_provenance=_cc_catalog.HUD_UI_MGR_TIMER_PANEL_STORAGE_IDENTITY,
        storage_identity=expected_storage,
        slot_displacement=_cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_SLOT,
        call_address=normalize_address(
            hex(_cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_CALL_OFFSET)
        ),
    )
    structural_contract = _cc_extraction.extract_invocation_contract(
        tuple(
            candidate_by_offset[offset]
            for offset in (
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_LOAD_OFFSET,
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_FIRST_ZERO_OFFSET,
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_SECOND_ZERO_OFFSET,
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_VPTR_OFFSET,
                _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_CALL_OFFSET,
            )
        ),
        source="cod",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        reviewed_static_storage_reference_bridges={
            candidate_expression: static_bridge
        },
        reviewed_member_vptr_storage_bridges={
            normalize_address(
                hex(_cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_VPTR_OFFSET)
            ): member_bridge
        },
    )
    if structural_contract != [{**expected_call, "ordinal": 0}]:
        raise ValueError(
            "HUD layout SetActive TimerPanel bridge cannot independently "
            "derive equality with the immutable retail virtual-call contract"
        )
    return (
        {candidate_expression: static_bridge},
        {
            normalize_address(
                hex(_cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_VPTR_OFFSET)
            ): member_bridge
        },
    )


def _hud_layout_hw_set_active_static_panel_census_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedStaticStorageReferenceBridge],
    dict[str, ReviewedLoopVptrStorageBridge],
]:
    """Prove SetActive's four remaining static-panel candidate units."""
    from _recoil.call_contract.records import (
        ReviewedLoopVptrStorageBridge,
        ReviewedStaticStorageReferenceBridge,
    )
    if normalize_address(caller_start) != _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_CALLER_START:
        return {}, {}

    timer_static, timer_member = (
        _hud_layout_hw_set_active_timer_panel_candidate_bridges(
            expected,
            candidate,
            document=document,
            caller_identity=caller_identity,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            indexes=indexes,
        )
    )
    caller = candidate.caller_definition
    assert caller is not None
    candidate_variant = _hud_layout_hw_set_active_candidate_variant(caller)
    active_first = candidate_variant != "legacy-inactive-first-1d0"
    timer_vptr_offset = (
        0x6A if active_first else _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_TIMER_VPTR_OFFSET
    )
    if (
        set(timer_static)
        != {
            (
                f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
                f"+{_cc_catalog.HUD_UI_MGR_TIMER_PANEL_DISPLACEMENT}"
            )
        }
        or set(timer_member)
        != {
            normalize_address(hex(timer_vptr_offset))
        }
    ):
        raise ValueError(
            "HUD layout SetActive static-panel census requires WSI-014's "
            "exact caller, aggregate, and typed TimerPanel prerequisite"
        )

    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    aggregate_start = address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
    nanite_identity = _cc_receiver_instructions._abstract_with_displacement(
        aggregate_identity,
        _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_NANITE_PANEL_DISPLACEMENT,
    )
    nanite_address = normalize_address(
        aggregate_start + _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_NANITE_PANEL_DISPLACEMENT
    )
    exact_containers = [
        row
        for row in indexes.storage_containers
        if (
            row.identity == aggregate_identity
            and row.start == aggregate_start
            and row.end_exclusive
            == aggregate_start + _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
        )
    ]
    if (
        len(exact_containers) != 1
        or indexes.storage_by_address.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
        != aggregate_identity
        or aggregate_identity in indexes.provider_ids
        or nanite_identity in indexes.provider_ids
        or address_value(nanite_address) + 4
        > aggregate_start + _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
    ):
        raise ValueError(
            "HUD layout SetActive static-panel census requires the exact "
            "reviewed g_HudUiMgr aggregate and bounded Nanite +0x420 storage"
        )

    objective_storage = (
        "load(load("
        f"{_cc_receiver_instructions._abstract_with_displacement(
            aggregate_identity,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_DISPLACEMENT,
        )}"
        "))"
    )
    timer_storage = f"load({_cc_catalog.HUD_UI_MGR_TIMER_PANEL_STORAGE_IDENTITY})"
    candidate_nanite_storage = f"load({nanite_identity})"
    legacy_specs = (
        {
            "label": "first NanitePanel",
            "start": 0x5E,
            "end": 0x70,
            "call": 0x6D,
            "candidate_ordinal": 6,
            "retail_ordinal": 9,
            "retail_direct_target": "symbol:recoil:function:0x4b4190",
            "retail_storage": "",
            "candidate_storage": candidate_nanite_storage,
            "body": bytes.fromhex(
                "8b 15 20 04 00 00 6a 00 6a 00 "
                "b9 20 04 00 00 ff 52 18"
            ),
            "relocations": ((0x60, 0x420), (0x69, 0x420)),
            "offsets": (0x5E, 0x64, 0x66, 0x68, 0x6D),
            "register": "edx",
            "expression_displacement": 0x420,
            "bridge_storage": nanite_identity,
        },
        {
            "label": "second ObjectiveCounter",
            "start": 0xD0,
            "end": 0xEA,
            "call": 0xE7,
            "candidate_ordinal": 13,
            "retail_ordinal": 15,
            "retail_direct_target": "",
            "retail_storage": objective_storage,
            "candidate_storage": objective_storage,
            "body": bytes.fromhex(
                "8b 0d c8 0b 00 00 8b 9e 28 01 00 00 "
                "8b be f0 01 00 00 6a 00 8b 11 53 ff 52 18"
            ),
            "relocations": ((0xD2, 0xBC8),),
            "offsets": (0xD0, 0xD6, 0xDC, 0xE2, 0xE4, 0xE6, 0xE7),
            "register": "edx",
            "expression_displacement": 0xBC8,
            "bridge_storage": _cc_receiver_instructions._abstract_with_displacement(
                aggregate_identity,
                0xBC8,
            ),
        },
        {
            "label": "second TimerPanel",
            "start": 0xEA,
            "end": 0xF8,
            "call": 0xF5,
            "candidate_ordinal": 14,
            "retail_ordinal": 16,
            "retail_direct_target": "",
            "retail_storage": timer_storage,
            "candidate_storage": timer_storage,
            "body": bytes.fromhex(
                "8b 0d 84 47 00 00 6a 00 53 8b 01 ff 50 18"
            ),
            "relocations": ((0xEC, 0x4784),),
            "offsets": (0xEA, 0xF0, 0xF2, 0xF3, 0xF5),
            "register": "eax",
            "expression_displacement": 0x4784,
            "bridge_storage": _cc_catalog.HUD_UI_MGR_TIMER_PANEL_STORAGE_IDENTITY,
        },
        {
            "label": "second NanitePanel",
            "start": 0x127,
            "end": 0x138,
            "call": 0x135,
            "candidate_ordinal": 17,
            "retail_ordinal": 17,
            "retail_direct_target": "symbol:recoil:function:0x4b4190",
            "retail_storage": "",
            "candidate_storage": candidate_nanite_storage,
            "body": bytes.fromhex(
                "8b 15 20 04 00 00 6a 00 57 "
                "b9 20 04 00 00 ff 52 18"
            ),
            "relocations": ((0x129, 0x420), (0x131, 0x420)),
            "offsets": (0x127, 0x12D, 0x12F, 0x130, 0x135),
            "register": "edx",
            "expression_displacement": 0x420,
            "bridge_storage": nanite_identity,
        },
    )

    active_first_indirect_specs = (
        {
            "label": "active NanitePanel",
            "start": 0x9E,
            "end": 0xAE,
            "call": 0xAB,
            "candidate_ordinal": 9,
            "retail_ordinal": 9,
            "retail_direct_target": "symbol:recoil:function:0x4b4190",
            "retail_storage": "",
            "candidate_storage": candidate_nanite_storage,
            "body": bytes.fromhex(
                "a1 20 04 00 00 6a 00 57 "
                "b9 20 04 00 00 ff 50 18"
            ),
            "relocations": ((0x9F, 0x420), (0xA7, 0x420)),
            "offsets": (0x9E, 0xA3, 0xA5, 0xA6, 0xAB),
            "register": "eax",
            "expression_displacement": 0x420,
            "bridge_storage": nanite_identity,
        },
        {
            "label": "inactive ObjectiveCounter",
            "start": 0x149,
            "end": 0x158,
            "call": 0x155,
            "candidate_ordinal": 15,
            "retail_ordinal": 15,
            "retail_direct_target": "",
            "retail_storage": objective_storage,
            "candidate_storage": objective_storage,
            "body": bytes.fromhex(
                "8b 0d c8 0b 00 00 6a 00 6a 00 8b 01 ff 50 18"
            ),
            "relocations": ((0x14B, 0xBC8),),
            "offsets": (0x149, 0x14F, 0x151, 0x153, 0x155),
            "register": "eax",
            "expression_displacement": 0xBC8,
            "bridge_storage": _cc_receiver_instructions._abstract_with_displacement(
                aggregate_identity,
                0xBC8,
            ),
        },
        {
            "label": "inactive TimerPanel",
            "start": 0x158,
            "end": 0x167,
            "call": 0x164,
            "candidate_ordinal": 16,
            "retail_ordinal": 16,
            "retail_direct_target": "",
            "retail_storage": timer_storage,
            "candidate_storage": timer_storage,
            "body": bytes.fromhex(
                "8b 0d 84 47 00 00 6a 00 6a 00 8b 11 ff 52 18"
            ),
            "relocations": ((0x15A, 0x4784),),
            "offsets": (0x158, 0x15E, 0x160, 0x162, 0x164),
            "register": "edx",
            "expression_displacement": 0x4784,
            "bridge_storage": _cc_catalog.HUD_UI_MGR_TIMER_PANEL_STORAGE_IDENTITY,
        },
        {
            "label": "inactive NanitePanel",
            "start": 0x167,
            "end": 0x178,
            "call": 0x175,
            "candidate_ordinal": 17,
            "retail_ordinal": 17,
            "retail_direct_target": "symbol:recoil:function:0x4b4190",
            "retail_storage": "",
            "candidate_storage": candidate_nanite_storage,
            "body": bytes.fromhex(
                "a1 20 04 00 00 6a 00 6a 00 "
                "b9 20 04 00 00 ff 50 18"
            ),
            "relocations": ((0x168, 0x420), (0x171, 0x420)),
            "offsets": (0x167, 0x16C, 0x16E, 0x170, 0x175),
            "register": "eax",
            "expression_displacement": 0x420,
            "bridge_storage": nanite_identity,
        },
    )
    active_first_direct_specs = (
        {
            "label": "active direct NanitePanel",
            "start": 0x9E,
            "end": 0xAB,
            "call": 0xA6,
            "candidate_ordinal": 9,
            "retail_ordinal": 9,
            "retail_direct_target": "symbol:recoil:function:0x4b4190",
            "retail_storage": "",
            "candidate_storage": "",
            "candidate_direct_target": "symbol:recoil:function:0x4b4190",
            "body": bytes.fromhex(
                "6a 00 57 b9 20 04 00 00 e8 00 00 00 00"
            ),
            "relocations": (
                (
                    0xA2,
                    IMAGE_REL_I386_DIR32,
                    _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                    0x420,
                ),
                (
                    0xA7,
                    IMAGE_REL_I386_REL32,
                    _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_NANITE_DIRECT_SYMBOL,
                    0,
                ),
            ),
            "offsets": (0x9E, 0xA0, 0xA1, 0xA6),
            "expression_displacement": 0x420,
            "bridge_storage": nanite_identity,
        },
        {
            "label": "inactive ObjectiveCounter",
            "start": 0x145,
            "end": 0x154,
            "call": 0x151,
            "candidate_ordinal": 15,
            "retail_ordinal": 15,
            "retail_direct_target": "",
            "retail_storage": objective_storage,
            "candidate_storage": objective_storage,
            "candidate_direct_target": "",
            "body": bytes.fromhex(
                "8b 0d c8 0b 00 00 6a 00 6a 00 8b 11 ff 52 18"
            ),
            "relocations": (
                (
                    0x147,
                    IMAGE_REL_I386_DIR32,
                    _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                    0xBC8,
                ),
            ),
            "offsets": (0x145, 0x14B, 0x14D, 0x14F, 0x151),
            "register": "edx",
            "expression_displacement": 0xBC8,
            "bridge_storage": _cc_receiver_instructions._abstract_with_displacement(
                aggregate_identity,
                0xBC8,
            ),
        },
        {
            "label": "inactive TimerPanel",
            "start": 0x154,
            "end": 0x163,
            "call": 0x160,
            "candidate_ordinal": 16,
            "retail_ordinal": 16,
            "retail_direct_target": "",
            "retail_storage": timer_storage,
            "candidate_storage": timer_storage,
            "candidate_direct_target": "",
            "body": bytes.fromhex(
                "8b 0d 84 47 00 00 6a 00 6a 00 8b 01 ff 50 18"
            ),
            "relocations": (
                (
                    0x156,
                    IMAGE_REL_I386_DIR32,
                    _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                    0x4784,
                ),
            ),
            "offsets": (0x154, 0x15A, 0x15C, 0x15E, 0x160),
            "register": "eax",
            "expression_displacement": 0x4784,
            "bridge_storage": _cc_catalog.HUD_UI_MGR_TIMER_PANEL_STORAGE_IDENTITY,
        },
        {
            "label": "inactive direct NanitePanel",
            "start": 0x163,
            "end": 0x171,
            "call": 0x16C,
            "candidate_ordinal": 17,
            "retail_ordinal": 17,
            "retail_direct_target": "symbol:recoil:function:0x4b4190",
            "retail_storage": "",
            "candidate_storage": "",
            "candidate_direct_target": "symbol:recoil:function:0x4b4190",
            "body": bytes.fromhex(
                "6a 00 6a 00 b9 20 04 00 00 e8 00 00 00 00"
            ),
            "relocations": (
                (
                    0x168,
                    IMAGE_REL_I386_DIR32,
                    _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                    0x420,
                ),
                (
                    0x16D,
                    IMAGE_REL_I386_REL32,
                    _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_NANITE_DIRECT_SYMBOL,
                    0,
                ),
            ),
            "offsets": (0x163, 0x165, 0x167, 0x16C),
            "expression_displacement": 0x420,
            "bridge_storage": nanite_identity,
        },
    )
    specs = (
        active_first_direct_specs
        if candidate_variant == "active-first-direct-1d0"
        else (
            active_first_indirect_specs
            if active_first
            else legacy_specs
        )
    )

    expected_mask = {
        position
        for relocation in caller.relocations
        for position in range(relocation.offset, relocation.offset + 4)
        if relocation.offset + 4 <= len(caller.data)
    }
    observed_mask = {
        index
        for index, masked in enumerate(caller.relocation_mask)
        if masked
    }
    if (
        any(
            relocation.offset < 0
            or relocation.offset + 4 > len(caller.data)
            for relocation in caller.relocations
        )
        or observed_mask != expected_mask
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in (
            caller.defined_external_data
            + caller.undefined_external_functions
            + caller.defined_external_functions
        )
    ):
        raise ValueError(
            "HUD layout SetActive static-panel census requires the complete "
            "exact caller relocation mask and aggregate symbol class"
        )

    candidate_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    by_offset: dict[int, Instruction] = {}
    index_by_offset: dict[int, int] = {}
    counts: dict[int, int] = {}
    for index, offset in enumerate(candidate_offsets):
        if offset is None:
            continue
        by_offset[offset] = candidate.instructions[index]
        index_by_offset[offset] = index
        counts[offset] = counts.get(offset, 0) + 1
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    ordinal_by_index = {
        instruction_index: ordinal
        for ordinal, instruction_index in enumerate(invocation_indices)
    }

    # Exact argument origins shared by the later three units.
    legacy_origin_rows = {
        0x06: ("mov", {"esi, ecx"}, b"\x8b\xf1"),
        0xD6: (
            "mov",
            {"ebx, dword [esi+296]", "ebx, dword ptr [esi+296]"},
            b"\x8b\x9e\x28\x01\x00\x00",
        ),
        0xDC: (
            "mov",
            {"edi, dword [esi+496]", "edi, dword ptr [esi+496]"},
            b"\x8b\xbe\xf0\x01\x00\x00",
        ),
    }
    active_first_origin_rows = {
        0x06: ("mov", {"esi, ecx"}, b"\x8b\xf1"),
        0x4D: (
            "mov",
            {"ebx, dword [esi+296]", "ebx, dword ptr [esi+296]"},
            b"\x8b\x9e\x28\x01\x00\x00",
        ),
        0x53: (
            "mov",
            {"edi, dword [esi+496]", "edi, dword ptr [esi+496]"},
            b"\x8b\xbe\xf0\x01\x00\x00",
        ),
    }
    origin_rows = (
        active_first_origin_rows if active_first else legacy_origin_rows
    )
    for offset, (mnemonic, operands, encoded) in origin_rows.items():
        instruction = by_offset.get(offset)
        if (
            instruction is None
            or counts.get(offset) != 1
            or _cc_cfg._instruction_mnemonic(instruction) != mnemonic
            or _cc_cfg._instruction_operand(instruction).strip().lower()
            not in operands
            or bytes(int(value, 16) for value in instruction.bytes)
            != encoded
        ):
            raise ValueError(
                "HUD layout SetActive static-panel census rejects this/EBX/"
                "EDI argument-origin drift"
            )

    static_bridges: dict[str, ReviewedStaticStorageReferenceBridge] = {}
    loop_bridges: dict[str, ReviewedLoopVptrStorageBridge] = {}
    for spec in specs:
        start = int(spec["start"])
        end = int(spec["end"])
        call_offset = int(spec["call"])
        relocation_specs = tuple(spec["relocations"])
        expected_relocations = tuple(
            (
                (
                    int(row[0]),
                    IMAGE_REL_I386_DIR32,
                    _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                    int(row[1]),
                )
                if len(row) == 2
                else (
                    int(row[0]),
                    int(row[1]),
                    str(row[2]),
                    int(row[3]),
                )
            )
            for row in relocation_specs
        )
        unit_relocations = [
            relocation
            for relocation in caller.relocations
            if start <= relocation.offset < end
        ]
        observed_relocations = tuple(
            (
                relocation.offset,
                relocation.type,
                relocation.symbol_name,
                (
                    struct.unpack_from("<I", caller.data, relocation.offset)[0]
                    if relocation.offset + 4 <= len(caller.data)
                    else None
                ),
            )
            for relocation in unit_relocations
        )
        required_relocations = expected_relocations
        unit_mask = {
            position
            for position in observed_mask
            if start <= position < end
        }
        required_mask = {
            position
            for offset, *_rest in expected_relocations
            for position in range(offset, offset + 4)
        }
        offsets = tuple(spec["offsets"])
        if (
            caller.data[start:end] != spec["body"]
            or observed_relocations != required_relocations
            or unit_mask != required_mask
            or {
                offset
                for offset in by_offset
                if start <= offset < end
            }
            != set(offsets)
            or any(counts.get(offset) != 1 for offset in offsets)
        ):
            raise ValueError(
                "HUD layout SetActive static-panel census requires the exact "
                f"{spec['label']} body/COFF/mask/instruction topology"
            )

        call_index = index_by_offset[call_offset]
        if (
            ordinal_by_index.get(call_index)
            != spec["candidate_ordinal"]
            or call_index in candidate.local_control_flow_indices
            or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
        ):
            raise ValueError(
                "HUD layout SetActive static-panel census rejects "
                f"{spec['label']} candidate ordinal/cleanup/CFG drift"
            )
        retail_ordinal = int(spec["retail_ordinal"])
        retail_direct_target = str(spec["retail_direct_target"])
        retail_call = {
            "ordinal": retail_ordinal,
            "form": "call",
            "dispatch": (
                "direct" if retail_direct_target else "indirect"
            ),
            "identity_kind": (
                "direct" if retail_direct_target else "virtual-slot"
            ),
            "target_identity": retail_direct_target,
            "storage_identity": spec["retail_storage"],
            "slot_displacement": (
                None if retail_direct_target else 0x18
            ),
            "cleanup_bytes": None,
        }
        if (
            len(expected) <= retail_ordinal
            or dict(expected[retail_ordinal]) != retail_call
        ):
            raise ValueError(
                "HUD layout SetActive static-panel census requires the exact "
                f"immutable retail {spec['label']} invocation"
            )

        expression = (
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
            f"+{int(spec['expression_displacement'])}"
        )
        static_bridge = ReviewedStaticStorageReferenceBridge(
            aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            displacement=int(spec["expression_displacement"]),
            access_width=4,
            storage_identity=str(spec["bridge_storage"]),
        )
        if (
            expression in static_bridges
            and static_bridges[expression] != static_bridge
        ):
            raise ValueError(
                "HUD layout SetActive static-panel census has conflicting "
                "candidate static-storage authority"
            )
        static_bridges[expression] = static_bridge
        candidate_direct_target = str(
            spec.get("candidate_direct_target", "")
        )
        call_address = normalize_address(hex(call_offset))
        local_loop_bridges: dict[
            str, ReviewedLoopVptrStorageBridge
        ] = {}
        if not candidate_direct_target:
            loop_bridge = ReviewedLoopVptrStorageBridge(
                register=str(spec["register"]),
                storage_identity=str(spec["candidate_storage"]),
                slot_displacement=0x18,
                assembly_source="cod",
            )
            loop_bridges[call_address] = loop_bridge
            local_loop_bridges[call_address] = loop_bridge
        structural = _cc_extraction.extract_invocation_contract(
            tuple(by_offset[offset] for offset in offsets),
            source="cod",
            caller_identity=caller_identity,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            indexes=indexes,
            reviewed_static_storage_reference_bridges={
                expression: static_bridge
            },
            reviewed_loop_vptr_storage_bridges=local_loop_bridges,
        )
        candidate_call = (
            {
                "ordinal": 0,
                "form": "call",
                "dispatch": "direct",
                "identity_kind": "direct",
                "target_identity": candidate_direct_target,
                "storage_identity": "",
                "slot_displacement": None,
                "cleanup_bytes": None,
            }
            if candidate_direct_target
            else {
                "ordinal": 0,
                "form": "call",
                "dispatch": "indirect",
                "identity_kind": "virtual-slot",
                "target_identity": "",
                "storage_identity": spec["candidate_storage"],
                "slot_displacement": 0x18,
                "cleanup_bytes": None,
            }
        )
        if structural != [candidate_call]:
            raise ValueError(
                "HUD layout SetActive static-panel census cannot "
                f"independently derive {spec['label']} candidate provenance"
            )
    return static_bridges, loop_bridges


def _hud_layout_hw_set_active_message_loop_candidate_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedLoopVptrStorageBridge]:
    """Prove exactly SetActive's inactive and active message/panel loops."""
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge
    if normalize_address(caller_start) != _cc_catalog.HUD_LAYOUT_HW_SET_ACTIVE_CALLER_START:
        return {}

    census_static, census_loop = (
        _hud_layout_hw_set_active_static_panel_census_bridges(
            expected,
            candidate,
            document=document,
            caller_identity=caller_identity,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            indexes=indexes,
        )
    )
    caller = candidate.caller_definition
    assert caller is not None
    candidate_variant = _hud_layout_hw_set_active_candidate_variant(caller)
    active_first = candidate_variant != "legacy-inactive-first-1d0"
    expected_census_loop = (
        {"0x151", "0x160"}
        if candidate_variant == "active-first-direct-1d0"
        else (
            {"0xab", "0x155", "0x164", "0x175"}
            if active_first
            else {"0x6d", "0xe7", "0xf5", "0x135"}
        )
    )
    if (
        set(census_static)
        != {
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}+1056",
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}+3016",
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}+18308",
        }
        or set(census_loop) != expected_census_loop
    ):
        raise ValueError(
            "HUD layout SetActive message-loop bridge requires WSI-015's "
            "exact four-unit candidate census"
        )

    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    aggregate_start = address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
    exact_containers = [
        row
        for row in indexes.storage_containers
        if (
            row.identity == aggregate_identity
            and row.start == aggregate_start
            and row.end_exclusive
            == aggregate_start + _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
        )
    ]
    if (
        len(exact_containers) != 1
        or indexes.storage_by_address.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
        != aggregate_identity
        or aggregate_identity in indexes.provider_ids
    ):
        raise ValueError(
            "HUD layout SetActive message-loop bridge requires the exact "
            "reviewed g_HudUiMgr aggregate container"
        )

    legacy_loop_specs = (
        {
            "label": "inactive",
            "start": 0x70,
            "end": 0xA1,
            "message_base": 0x4B18,
            "panel_base": 0x4BF8,
            "limit": 0x7610,
            "stride": 0x44C,
            "population": 10,
            "calls": (
                (
                    0x7D,
                    7,
                    18,
                    "eax",
                    (
                        "load(load("
                        "storage:recoil:data:0x4e5ed0+0x4bf8)-0xe0)"
                    ),
                ),
                (
                    0x90,
                    8,
                    19,
                    "edx",
                    (
                        "load(load("
                        "storage:recoil:data:0x4e5ed0+0x4bf8))"
                    ),
                ),
            ),
            "relocations": ((0x71, 0x4B18), (0x9B, 0x7610)),
            "body": bytes.fromhex(
                "be 18 4b 00 00 8b 06 6a 00 6a 00 8b ce ff 50 18 "
                "8b 96 e0 00 00 00 8d 8e e0 00 00 00 6a 00 6a 00 "
                "ff 52 18 81 c6 4c 04 00 00 81 fe 10 76 00 00 7c d4"
            ),
            "offsets": (
                0x70, 0x75, 0x77, 0x79, 0x7B, 0x7D,
                0x80, 0x86, 0x8C, 0x8E, 0x90, 0x93, 0x99, 0x9F,
            ),
        },
        {
            "label": "active",
            "start": 0xF8,
            "end": 0x127,
            "message_base": 0x4F64,
            "panel_base": 0x5044,
            "limit": 0x7610,
            "stride": 0x44C,
            "population": 9,
            "calls": (
                (
                    0x104,
                    15,
                    7,
                    "edx",
                    (
                        "load(load("
                        "storage:recoil:data:0x4e5ed0+0x5044)-0xe0)"
                    ),
                ),
                (
                    0x116,
                    16,
                    8,
                    "eax",
                    (
                        "load(load("
                        "storage:recoil:data:0x4e5ed0+0x5044))"
                    ),
                ),
            ),
            "relocations": ((0xF9, 0x4F64), (0x121, 0x7610)),
            "body": bytes.fromhex(
                "be 64 4f 00 00 8b 16 6a 00 57 8b ce ff 52 18 "
                "8b 86 e0 00 00 00 8d 8e e0 00 00 00 6a 00 57 "
                "ff 50 18 81 c6 4c 04 00 00 81 fe 10 76 00 00 7c d6"
            ),
            "offsets": (
                0xF8, 0xFD, 0xFF, 0x101, 0x102, 0x104,
                0x107, 0x10D, 0x113, 0x115, 0x116,
                0x119, 0x11F, 0x125,
            ),
        },
    )

    active_first_indirect_loop_specs = (
        {
            "label": "active",
            "start": 0x6F,
            "end": 0x9E,
            "message_base": 0x4F64,
            "panel_base": 0x5044,
            "limit": 0x7610,
            "stride": 0x44C,
            "population": 9,
            "calls": (
                (
                    0x7B,
                    7,
                    7,
                    "eax",
                    (
                        "load(load("
                        "storage:recoil:data:0x4e5ed0+0x5044)-0xe0)"
                    ),
                ),
                (
                    0x8D,
                    8,
                    8,
                    "edx",
                    (
                        "load(load("
                        "storage:recoil:data:0x4e5ed0+0x5044))"
                    ),
                ),
            ),
            "relocations": ((0x70, 0x4F64), (0x98, 0x7610)),
            "body": bytes.fromhex(
                "be 64 4f 00 00 8b 06 6a 00 57 8b ce ff 50 18 "
                "8b 96 e0 00 00 00 8d 8e e0 00 00 00 6a 00 57 "
                "ff 52 18 81 c6 4c 04 00 00 81 fe 10 76 00 00 7c d6"
            ),
            "offsets": (
                0x6F, 0x74, 0x76, 0x78, 0x79, 0x7B,
                0x7E, 0x84, 0x8A, 0x8C, 0x8D,
                0x90, 0x96, 0x9C,
            ),
        },
        {
            "label": "inactive",
            "start": 0x178,
            "end": 0x1A9,
            "message_base": 0x4B18,
            "panel_base": 0x4BF8,
            "limit": 0x7610,
            "stride": 0x44C,
            "population": 10,
            "calls": (
                (
                    0x185,
                    18,
                    18,
                    "edx",
                    (
                        "load(load("
                        "storage:recoil:data:0x4e5ed0+0x4bf8)-0xe0)"
                    ),
                ),
                (
                    0x198,
                    19,
                    19,
                    "eax",
                    (
                        "load(load("
                        "storage:recoil:data:0x4e5ed0+0x4bf8))"
                    ),
                ),
            ),
            "relocations": ((0x179, 0x4B18), (0x1A3, 0x7610)),
            "body": bytes.fromhex(
                "be 18 4b 00 00 8b 16 6a 00 6a 00 8b ce ff 52 18 "
                "8b 86 e0 00 00 00 8d 8e e0 00 00 00 6a 00 6a 00 "
                "ff 50 18 81 c6 4c 04 00 00 81 fe 10 76 00 00 7c d4"
            ),
            "offsets": (
                0x178, 0x17D, 0x17F, 0x181, 0x183, 0x185,
                0x188, 0x18E, 0x194, 0x196, 0x198,
                0x19B, 0x1A1, 0x1A7,
            ),
        },
    )
    active_first_direct_loop_specs = (
        active_first_indirect_loop_specs[0],
        {
            "label": "inactive",
            "start": 0x171,
            "end": 0x1A2,
            "message_base": 0x4B18,
            "panel_base": 0x4BF8,
            "limit": 0x7610,
            "stride": 0x44C,
            "population": 10,
            "calls": (
                (
                    0x17E,
                    18,
                    18,
                    "edx",
                    (
                        "load(load("
                        "storage:recoil:data:0x4e5ed0+0x4bf8)-0xe0)"
                    ),
                ),
                (
                    0x191,
                    19,
                    19,
                    "eax",
                    (
                        "load(load("
                        "storage:recoil:data:0x4e5ed0+0x4bf8))"
                    ),
                ),
            ),
            "relocations": ((0x172, 0x4B18), (0x19C, 0x7610)),
            "body": bytes.fromhex(
                "be 18 4b 00 00 8b 16 6a 00 6a 00 8b ce ff 52 18 "
                "8b 86 e0 00 00 00 8d 8e e0 00 00 00 6a 00 6a 00 "
                "ff 50 18 81 c6 4c 04 00 00 81 fe 10 76 00 00 7c d4"
            ),
            "offsets": (
                0x171, 0x176, 0x178, 0x17A, 0x17C, 0x17E,
                0x181, 0x187, 0x18D, 0x18F, 0x191,
                0x194, 0x19A, 0x1A0,
            ),
        },
    )
    loop_specs = (
        active_first_direct_loop_specs
        if candidate_variant == "active-first-direct-1d0"
        else (
            active_first_indirect_loop_specs
            if active_first
            else legacy_loop_specs
        )
    )

    expected_mask = {
        position
        for relocation in caller.relocations
        for position in range(relocation.offset, relocation.offset + 4)
        if relocation.offset + 4 <= len(caller.data)
    }
    observed_mask = {
        index
        for index, masked in enumerate(caller.relocation_mask)
        if masked
    }
    if (
        observed_mask != expected_mask
        or any(
            relocation.offset < 0
            or relocation.offset + 4 > len(caller.data)
            for relocation in caller.relocations
        )
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in (
            caller.defined_external_data
            + caller.undefined_external_functions
            + caller.defined_external_functions
        )
    ):
        raise ValueError(
            "HUD layout SetActive message-loop bridge requires the complete "
            "exact caller relocation mask and aggregate symbol class"
        )

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    by_offset: dict[int, Instruction] = {}
    index_by_offset: dict[int, int] = {}
    counts: dict[int, int] = {}
    for index, offset in enumerate(offsets):
        if offset is None:
            continue
        by_offset[offset] = candidate.instructions[index]
        index_by_offset[offset] = index
        counts[offset] = counts.get(offset, 0) + 1
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    ordinal_by_index = {
        instruction_index: ordinal
        for ordinal, instruction_index in enumerate(invocation_indices)
    }

    # The active loop's nonzero arguments retain WSI-015's exact EDI origin.
    edi_origin_offset = 0x53 if active_first else 0xDC
    edi_origin = by_offset.get(edi_origin_offset)
    if (
        edi_origin is None
        or counts.get(edi_origin_offset) != 1
        or _cc_cfg._instruction_mnemonic(edi_origin) != "mov"
        or _cc_cfg._instruction_operand(edi_origin).strip().lower()
        not in {
            "edi, dword [esi+496]",
            "edi, dword ptr [esi+496]",
        }
        or bytes(int(value, 16) for value in edi_origin.bytes)
        != b"\x8b\xbe\xf0\x01\x00\x00"
    ):
        raise ValueError(
            "HUD layout SetActive message-loop bridge rejects active-loop "
            "EDI argument-origin drift"
        )

    bridges: dict[str, ReviewedLoopVptrStorageBridge] = {}
    for spec in loop_specs:
        start = int(spec["start"])
        end = int(spec["end"])
        message_base = int(spec["message_base"])
        panel_base = int(spec["panel_base"])
        limit = int(spec["limit"])
        stride = int(spec["stride"])
        population = int(spec["population"])
        if (
            message_base + 0xE0 != panel_base
            or message_base + population * stride != limit
            or panel_base - 0xE0 != message_base
            or aggregate_start + limit
            > aggregate_start + _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
        ):
            raise ValueError(
                "HUD layout SetActive message-loop bridge rejects "
                f"{spec['label']} base/embedded-panel/stride/population drift"
            )
        expected_relocations = tuple(
            (
                offset,
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                addend,
            )
            for offset, addend in spec["relocations"]
        )
        unit_relocations = [
            relocation
            for relocation in caller.relocations
            if start <= relocation.offset < end
        ]
        observed_relocations = tuple(
            (
                relocation.offset,
                relocation.type,
                relocation.symbol_name,
                (
                    struct.unpack_from("<I", caller.data, relocation.offset)[0]
                    if relocation.offset + 4 <= len(caller.data)
                    else None
                ),
            )
            for relocation in unit_relocations
        )
        required_mask = {
            position
            for offset, _ in spec["relocations"]
            for position in range(offset, offset + 4)
        }
        unit_mask = {
            position
            for position in observed_mask
            if start <= position < end
        }
        required_offsets = tuple(spec["offsets"])
        if (
            caller.data[start:end] != spec["body"]
            or observed_relocations != expected_relocations
            or unit_mask != required_mask
            or {
                offset
                for offset in by_offset
                if start <= offset < end
            }
            != set(required_offsets)
            or any(counts.get(offset) != 1 for offset in required_offsets)
        ):
            raise ValueError(
                "HUD layout SetActive message-loop bridge requires the exact "
                f"{spec['label']} body/COFF/mask/instruction topology"
            )

        local_bridges: dict[str, ReviewedLoopVptrStorageBridge] = {}
        normalized_retail: list[dict[str, Any]] = []
        for local_ordinal, (
            call_offset,
            candidate_ordinal,
            retail_ordinal,
            register,
            storage_identity,
        ) in enumerate(spec["calls"]):
            call_index = index_by_offset[int(call_offset)]
            if (
                ordinal_by_index.get(call_index) != candidate_ordinal
                or call_index in candidate.local_control_flow_indices
                or _cc_cfg._cleanup_after(candidate.instructions, call_index)
                is not None
            ):
                raise ValueError(
                    "HUD layout SetActive message-loop bridge rejects "
                    f"{spec['label']} call ordinal/cleanup/CFG drift"
                )
            retail_call = {
                "ordinal": retail_ordinal,
                "form": "call",
                "dispatch": "indirect",
                "identity_kind": "virtual-slot",
                "target_identity": "",
                "storage_identity": storage_identity,
                "slot_displacement": 0x18,
                "cleanup_bytes": None,
            }
            if (
                len(expected) <= retail_ordinal
                or dict(expected[retail_ordinal]) != retail_call
            ):
                raise ValueError(
                    "HUD layout SetActive message-loop bridge requires the "
                    f"exact immutable retail {spec['label']} loop calls"
                )
            call_address = normalize_address(hex(int(call_offset)))
            bridge = ReviewedLoopVptrStorageBridge(
                register=str(register),
                storage_identity=str(storage_identity),
                slot_displacement=0x18,
                assembly_source="cod",
            )
            local_bridges[call_address] = bridge
            bridges[call_address] = bridge
            normalized_retail.append(
                {**retail_call, "ordinal": local_ordinal}
            )
        structural = _cc_extraction.extract_invocation_contract(
            tuple(by_offset[offset] for offset in required_offsets),
            source="cod",
            caller_identity=caller_identity,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            indexes=indexes,
            reviewed_loop_vptr_storage_bridges=local_bridges,
        )
        if structural != normalized_retail:
            raise ValueError(
                "HUD layout SetActive message-loop bridge cannot "
                f"independently derive immutable {spec['label']} loop equality"
            )
    return bridges
