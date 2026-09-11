"""Recoil call-contract recoil hud messages evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import candidate as _cc_candidate
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import comparison as _cc_comparison
from _recoil.call_contract import extraction as _cc_extraction
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        IdentityIndexes,
        ReviewedLoopVptrStorageBridge,
    )

import struct
from dataclasses import replace
from pathlib import Path
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_DIR32,
    IMAGE_REL_I386_REL32,
    IMAGE_SYM_CLASS_STATIC,
    Instruction,
    relocation_size,
)
from _recoil.lib.authored_icf import exact_required_target_membership
from _recoil.lib.progress import ProgressDocument, address_value, normalize_address


def _hud_ui_message_set_value_clear_token_vptr_bridges(
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedLoopVptrStorageBridge],
    dict[str, ReviewedLoopVptrStorageBridge],
]:
    """Bridge only SetValueIfOwnerMatches' clear-token panel invocation."""
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CALLER_START:
        return {}, {}

    symbols = document.collection("symbols")
    caller_row = symbols.get(
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CALLER_IDENTITY.removeprefix("symbol:")
    )
    trace = (
        caller_row.get("source_traceability")
        if isinstance(caller_row, Mapping)
        else None
    )
    source_edges = (
        trace.get("source_edges") if isinstance(trace, Mapping) else None
    )
    definition = candidate.caller_definition
    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    aggregate_start = address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
    aggregate_containers = [
        row
        for row in indexes.storage_containers
        if (
            row.identity == aggregate_identity
            and row.start == aggregate_start
            and row.end_exclusive
            == aggregate_start + _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
        )
    ]
    clear_token_row = symbols.get(
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CLEAR_TOKEN_ID
    )
    aggregate_row = symbols.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID)
    clear_token_identity = (
        f"storage:{_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CLEAR_TOKEN_ID}"
    )
    if (
        caller_identity != _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(normalized_start) != caller_identity
        or caller_identity in indexes.provider_ids
        or not isinstance(caller_row, Mapping)
        or caller_row.get("binary") != "recoil"
        or caller_row.get("kind") != "function"
        or caller_row.get("pipeline_class") != "authored"
        or caller_row.get("ownership_state") != "primary-owned"
        or caller_row.get("address") != normalized_start
        or caller_row.get("end_exclusive")
        != _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CALLER_END_EXCLUSIVE
        or caller_row.get("extent_state") != "known"
        or caller_row.get("size") != 0x90
        or caller_row.get("navigation_name")
        != "HudUiMessage::SetValueIfOwnerMatches"
        or caller_row.get("output_section_id") != "recoil:section:.text"
        or caller_row.get("physical_block_id") != "recoil:block:0x404ca0"
        or not isinstance(trace, Mapping)
        or trace.get("state") != "resolved"
        or trace.get("reason_code") not in {None, ""}
        or not isinstance(source_edges, list)
        or len(source_edges) != 1
        or not isinstance(source_edges[0], Mapping)
        or source_edges[0].get("relation") != "defines"
        or source_edges[0].get("anchor_id")
        != _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
        or definition is None
        or definition.symbol != _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CALLER_SYMBOL
        or len(definition.data) != len(definition.relocation_mask)
    ):
        raise ValueError(
            "HUD message SetValue clear-token bridge requires the exact "
            "authored 0x412650 caller, source edge, extent, and object symbol"
        )
    if (
        not isinstance(aggregate_row, Mapping)
        or aggregate_row.get("binary") != "recoil"
        or aggregate_row.get("kind") != "data"
        or aggregate_row.get("address") != _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
        or aggregate_row.get("disposition") != "authored"
        or aggregate_row.get("navigation_name") != _cc_catalog.HUD_UI_MGR_AGGREGATE_NAME
        or aggregate_row.get("output_section_id") != "recoil:section:.data"
        or aggregate_row.get("storage_contribution_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_STORAGE_ID]
        or aggregate_row.get("verification_target_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_ID]
        or indexes.storage_by_address.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
        != aggregate_identity
        or len(aggregate_containers) != 1
        or aggregate_identity in indexes.provider_ids
        or aggregate_start + _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_ARRAY_DISPLACEMENT
        != 0x4EA9E8
        or aggregate_start + _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_OWNER_DISPLACEMENT
        != 0x4EAD6C
        or not isinstance(clear_token_row, Mapping)
        or clear_token_row.get("binary") != "recoil"
        or clear_token_row.get("kind") != "data"
        or clear_token_row.get("address")
        != _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CLEAR_TOKEN_ADDRESS
        or clear_token_row.get("disposition") != "authored"
        or clear_token_row.get("navigation_name")
        != _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CLEAR_TOKEN_NAME
        or clear_token_row.get("output_section_id")
        != "recoil:section:.data"
        or clear_token_row.get("storage_contribution_ids")
        != ["recoil:storage:va:0x4dae08"]
        or clear_token_row.get("verification_target_ids")
        != ["recoil:vc5-target:hud_ui_message_clear_special_token165"]
        or indexes.storage_by_address.get(
            _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CLEAR_TOKEN_ADDRESS
        )
        != clear_token_identity
        or clear_token_identity in indexes.provider_ids
    ):
        raise ValueError(
            "HUD message SetValue clear-token bridge requires the exact "
            "reviewed aggregate/token rows, extents, targets, and storage "
            "identities"
        )

    def body(instruction: Instruction) -> bytes:
        return bytes(int(value, 16) for value in instruction.bytes)

    retail_addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(normalized_start),
    )
    retail_counts: dict[int, int] = {}
    for address in retail_addresses:
        if address is not None:
            retail_counts[address] = retail_counts.get(address, 0) + 1
    retail_by_address = {
        address: instruction
        for address, instruction in zip(retail_addresses, retail_instructions)
        if address is not None and retail_counts.get(address) == 1
    }
    retail_index_by_address = {
        address: index
        for index, address in enumerate(retail_addresses)
        if address is not None and retail_counts.get(address) == 1
    }
    retail_fixed = {
        0x412650: b"\x8d\x04\x89",
        0x412653: b"\x56",
        0x412654: b"\x8d\x04\x41",
        0x412657: b"\x8d\x04\x80",
        0x41265A: b"\x8d\x0c\x80",
        0x41265D: b"\x8b\x04\x8d\x6c\xad\x4e\x00",
        0x412664: b"\x3b\xd0",
        0x412666: b"\x8d\x34\x8d\xe8\xa9\x4e\x00",
        0x41266D: b"\x75\x65",
        0x41266F: b"\xd9\x44\x24\x08",
        0x412673: b"\xd8\x1d\x84\xe7\x4c\x00",
        0x412679: b"\xdf\xe0",
        0x41267B: b"\xf6\xc4\x40",
        0x41267E: b"\x74\x1c",
        0x412680: b"\x8b\x96\xe0\x00\x00\x00",
        0x412686: b"\x8d\x86\xe0\x00\x00\x00",
        0x41268C: b"\x68\x08\xae\x4d\x00",
        0x412691: b"\x50",
        0x412692: b"\xff\x52\x74",
        0x412695: b"\x83\xc4\x08",
        0x412698: b"\x5e",
        0x412699: b"\xc2\x04\x00",
        0x41269C: b"\xd9\x44\x24\x08",
    }
    if any(
        address not in retail_by_address
        or body(retail_by_address[address]) != expected
        for address, expected in retail_fixed.items()
    ):
        raise ValueError(
            "HUD message SetValue clear-token bridge requires exact retail "
            "index, storage, panel-offset, token, receiver, slot, and body bytes"
        )
    retail_indices = [
        retail_index_by_address[address] for address in retail_fixed
    ]
    retail_call_index = retail_index_by_address[
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_RETAIL_CALL
    ]
    retail_calls = [
        address
        for address, instruction in zip(
            retail_addresses,
            retail_instructions,
        )
        if address is not None
        and _cc_cfg._instruction_mnemonic(instruction) == "call"
    ]
    def branch_target(address: int) -> str:
        matches = _cc_catalog.ADDRESS_RE.findall(
            _cc_cfg._instruction_operand(retail_by_address[address])
        )
        return normalize_address(matches[-1]) if matches else ""

    if (
        retail_indices
        != list(range(retail_indices[0], retail_indices[0] + len(retail_fixed)))
        or branch_target(0x41266D) != "0x4126d4"
        or branch_target(0x41267E) != "0x41269c"
        or _cc_cfg._cleanup_after(retail_instructions, retail_call_index) != 8
        or retail_calls
        != [0x412692, 0x4126A6, 0x4126AF, 0x4126C7, 0x4126D1]
    ):
        raise ValueError(
            "HUD message SetValue clear-token bridge rejects retail branch, "
            "cleanup, invocation-population, or invocation-order drift"
        )

    candidate_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    candidate_counts: dict[int, int] = {}
    for offset in candidate_offsets:
        if offset is not None:
            candidate_counts[offset] = candidate_counts.get(offset, 0) + 1
    candidate_by_offset = {
        offset: instruction
        for offset, instruction in zip(
            candidate_offsets,
            candidate.instructions,
        )
        if offset is not None and candidate_counts.get(offset) == 1
    }
    candidate_index_by_offset = {
        offset: index
        for index, offset in enumerate(candidate_offsets)
        if offset is not None and candidate_counts.get(offset) == 1
    }
    candidate_fixed = {
        0x00: b"\x8d\x04\x89",
        0x06: b"\x8d\x04\x41",
        0x09: b"\x8d\x04\x80",
        0x0C: b"\x8d\x0c\x80",
        0x0F: b"\x8b\x04\x8d\x9c\x4e\x00\x00",
        0x16: b"\x3b\xd0",
        0x18: b"\x8d\x34\x8d\x18\x4b\x00\x00",
        0x1F: b"\x75\x67",
        0x21: b"\xd9\x44\x24\x10",
        0x25: b"\xd8\x1d\x00\x00\x00\x00",
        0x2B: b"\xdf\xe0",
        0x2D: b"\xf6\xc4\x40",
        0x30: b"\x74\x1e",
        0x32: b"\x8b\x96\xe0\x00\x00\x00",
        0x38: b"\x8d\x86\xe0\x00\x00\x00",
        0x3E: b"\x68\x00\x00\x00\x00",
        0x43: b"\x50",
        0x44: b"\xff\x52\x74",
        0x47: b"\x83\xc4\x08",
    }
    if any(
        offset not in candidate_by_offset
        or body(candidate_by_offset[offset]) != expected
        for offset, expected in candidate_fixed.items()
    ):
        raise ValueError(
            "HUD message SetValue clear-token bridge requires exact candidate "
            "index, storage, panel-offset, token, receiver, slot, and body bytes"
        )
    candidate_indices = [
        candidate_index_by_offset[offset] for offset in candidate_fixed
    ]
    candidate_call_index = candidate_index_by_offset[
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CANDIDATE_CALL
    ]
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    candidate_calls = [
        candidate_offsets[index] for index in invocation_indices
    ]
    bounded_indices = set(
        range(candidate_indices[0], candidate_indices[-1] + 1)
    )
    if (
        candidate_indices
        != sorted(candidate_indices)
        or len(set(candidate_indices)) != len(candidate_indices)
        or _cc_cfg._cleanup_after(candidate.instructions, candidate_call_index) != 8
        or candidate_calls != [0x44, 0x66, 0x6F, 0x7B, 0x85]
        or candidate.local_control_flow_indices & bounded_indices
        or any(
            target in bounded_indices
            for targets in candidate.local_control_flow_targets.values()
            for target in targets
        )
    ):
        raise ValueError(
            "HUD message SetValue clear-token bridge rejects candidate "
            "topology, cleanup, invocation-population, or invocation-order drift"
        )

    expected_relocations = {
        0x12: (
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_OWNER_DISPLACEMENT,
        ),
        0x1B: (
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_ARRAY_DISPLACEMENT,
        ),
        0x3F: (_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CLEAR_TOKEN_SYMBOL, 0),
    }
    bounded_relocations = [
        relocation
        for relocation in definition.relocations
        if relocation.offset in expected_relocations
    ]
    expected_mask = {
        index
        for offset in expected_relocations
        for index in range(offset, offset + 4)
    }
    if (
        len(bounded_relocations) != len(expected_relocations)
        or {row.offset for row in bounded_relocations}
        != set(expected_relocations)
        or any(
            row.type != IMAGE_REL_I386_DIR32
            or (
                row.symbol_name,
                struct.unpack_from("<I", definition.data, row.offset)[0],
            )
            != expected_relocations[row.offset]
            for row in bounded_relocations
        )
        or {
            index
            for index in range(0, 0x47)
            if definition.relocation_mask[index]
        }
        & expected_mask
        != expected_mask
        or definition.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or definition.undefined_external_data.count(
            _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CLEAR_TOKEN_SYMBOL
        )
        != 1
    ):
        raise ValueError(
            "HUD message SetValue clear-token bridge rejects candidate "
            "aggregate index/owner or clear-token relocation identity drift"
        )

    return (
        {
            normalize_address(
                _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_RETAIL_CALL
            ): ReviewedLoopVptrStorageBridge(
                register="edx",
                storage_identity=(
                    _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_PANEL_STORAGE_IDENTITY
                ),
                slot_displacement=_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_SLOT,
                assembly_source="bn",
            )
        },
        {
            normalize_address(
                hex(_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CANDIDATE_CALL)
            ): ReviewedLoopVptrStorageBridge(
                register="edx",
                storage_identity=(
                    _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_PANEL_STORAGE_IDENTITY
                ),
                slot_displacement=_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_SLOT,
                assembly_source="cod",
            )
        },
    )


def _hud_ui_message_set_value_vptr_bridges(
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedLoopVptrStorageBridge],
    dict[str, ReviewedLoopVptrStorageBridge],
]:
    """Compose the reviewed panel calls and final message invalidation."""
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge
    (
        retail_bridges,
        candidate_bridges,
    ) = _hud_ui_message_set_value_clear_token_vptr_bridges(
        retail_instructions,
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CALLER_START:
        return retail_bridges, candidate_bridges

    expected_storage = _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_PANEL_STORAGE_IDENTITY
    expected_clear_retail = ReviewedLoopVptrStorageBridge(
        register="edx",
        storage_identity=expected_storage,
        slot_displacement=_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_SLOT,
        assembly_source="bn",
    )
    expected_clear_candidate = ReviewedLoopVptrStorageBridge(
        register="edx",
        storage_identity=expected_storage,
        slot_displacement=_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_SLOT,
        assembly_source="cod",
    )
    definition = candidate.caller_definition
    if (
        retail_bridges
        != {
            normalize_address(
                _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_RETAIL_CALL
            ): expected_clear_retail
        }
        or candidate_bridges
        != {
            normalize_address(
                hex(_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CANDIDATE_CALL)
            ): expected_clear_candidate
        }
        or indexes.storage_by_address.get(
            _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_IAT_ADDRESS
        )
        != _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_IAT_IDENTITY
        or indexes.storage_by_name.get(
            _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_IMPORT_NAME
        )
        != _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_IAT_IDENTITY
        or indexes.storage_by_name.get(
            _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_CANDIDATE_IMPORT_SYMBOL
        )
        != _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_IAT_IDENTITY
        or definition is None
        or definition.symbol != _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CALLER_SYMBOL
        or len(definition.data) != 0x90
        or len(definition.relocation_mask) != len(definition.data)
    ):
        raise ValueError(
            "HUD message numeric SetTextFmt bridge requires the preserved "
            "clear-token bridge, governed ceil IAT identity, exact authored "
            "caller authority, and current 0x90-byte candidate definition"
        )

    def body(instruction: Instruction) -> bytes:
        return bytes(int(value, 16) for value in instruction.bytes)

    retail_addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(normalized_start),
    )
    retail_counts: dict[int, int] = {}
    for address in retail_addresses:
        if address is not None:
            retail_counts[address] = retail_counts.get(address, 0) + 1
    retail_by_address = {
        address: instruction
        for address, instruction in zip(
            retail_addresses,
            retail_instructions,
        )
        if address is not None and retail_counts.get(address) == 1
    }
    retail_index_by_address = {
        address: index
        for index, address in enumerate(retail_addresses)
        if address is not None and retail_counts.get(address) == 1
    }
    retail_fixed = {
        0x41269C: b"\xd9\x44\x24\x08",
        0x4126A0: b"\x83\xec\x08",
        0x4126A3: b"\xdd\x1c\x24",
        0x4126A6: b"\xff\x15\x00\xc5\x4c\x00",
        0x4126AC: b"\x83\xc4\x08",
        0x4126AF: b"\xe8\xf2\x39\x0b\x00",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_RETAIL_VPTR:
            b"\x8b\x96\xe0\x00\x00\x00",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_RETAIL_RECEIVER:
            b"\x8d\x8e\xe0\x00\x00\x00",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_RETAIL_RESULT_ARGUMENT:
            b"\x50",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_RETAIL_FORMAT_ARGUMENT:
            b"\x68\xbc\xac\x4d\x00",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_RETAIL_THIS_ARGUMENT:
            b"\x51",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_RETAIL_CALL:
            b"\xff\x52\x74",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_RETAIL_NEXT_VPTR:
            b"\x8b\x06",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_RETAIL_CLEANUP:
            b"\x83\xc4\x0c",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_RETAIL_NEXT_RECEIVER:
            b"\x8b\xce",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_RETAIL_NEXT_CALL:
            b"\xff\x50\x20",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_RETAIL_TAIL_POP:
            b"\x5e",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_RETAIL_TAIL_RETURN:
            b"\xc2\x04\x00",
    }
    if any(
        address not in retail_by_address
        or body(retail_by_address[address]) != expected
        for address, expected in retail_fixed.items()
    ):
        raise ValueError(
            "HUD message numeric SetTextFmt bridge requires the exact retail "
            "ceil/_ftol result, panel vptr/receiver, variadic arguments, "
            "slot-0x74 call, cleanup, successor Invalidate receiver/call, "
            "and function-tail bytes"
        )
    retail_fixed_indices = [
        retail_index_by_address[address] for address in retail_fixed
    ]
    retail_call_index = retail_index_by_address[
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_RETAIL_CALL
    ]
    retail_next_call_index = retail_index_by_address[
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_RETAIL_NEXT_CALL
    ]
    retail_invocation_order = tuple(
        address
        for address, instruction in zip(
            retail_addresses,
            retail_instructions,
        )
        if address is not None
        and _cc_cfg._instruction_mnemonic(instruction) == "call"
    )
    if (
        retail_fixed_indices
        != list(
            range(
                retail_fixed_indices[0],
                retail_fixed_indices[0] + len(retail_fixed),
            )
        )
        or retail_invocation_order
        != _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_RETAIL_INVOCATION_ORDER
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(
                retail_by_address[
                    _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_RETAIL_CALL
                ]
            )
        )
        not in {"edx+116", "edx+0x74"}
        or _cc_cfg._cleanup_after(retail_instructions, retail_call_index)
        != _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CLEANUP_BYTES
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(
                retail_by_address[
                    _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_RETAIL_NEXT_CALL
                ]
            )
        )
        not in {"eax+32", "eax+0x20"}
        or _cc_cfg._cleanup_after(retail_instructions, retail_next_call_index)
        is not None
        or retail_index_by_address[
            _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_RETAIL_TAIL_RETURN
        ]
        != len(retail_instructions) - 1
    ):
        raise ValueError(
            "HUD message numeric SetTextFmt bridge rejects retail ordinal-3 "
            "slot-0x74/cleanup-12, complete invocation order, or immediate "
            "ordinal-4 Invalidate ESI-to-ECX receiver/slot-0x20/no-cleanup/"
            "function-tail topology drift"
        )

    candidate_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    candidate_counts: dict[int, int] = {}
    for offset in candidate_offsets:
        if offset is not None:
            candidate_counts[offset] = candidate_counts.get(offset, 0) + 1
    candidate_by_offset = {
        offset: instruction
        for offset, instruction in zip(
            candidate_offsets,
            candidate.instructions,
        )
        if offset is not None and candidate_counts.get(offset) == 1
    }
    candidate_index_by_offset = {
        offset: index
        for index, offset in enumerate(candidate_offsets)
        if offset is not None and candidate_counts.get(offset) == 1
    }
    candidate_fixed = {
        0x50: b"\xd9\x44\x24\x10",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_VPTR:
            b"\x8b\x9e\xe0\x00\x00\x00",
        0x5A: b"\x83\xec\x08",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_RECEIVER:
            b"\x8d\xbe\xe0\x00\x00\x00",
        0x63: b"\xdd\x1c\x24",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CEIL_CANDIDATE_CALL_OFFSET:
            b"\xff\x15\x00\x00\x00\x00",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CEIL_CANDIDATE_CLEANUP_OFFSET:
            b"\x83\xc4\x08",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_FTOL_CANDIDATE_CALL_OFFSET:
            b"\xe8\x00\x00\x00\x00",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_RESULT_ARGUMENT:
            b"\x50",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_FORMAT_ARGUMENT:
            b"\x68\x00\x00\x00\x00",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_THIS_ARGUMENT:
            b"\x57",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_CALL:
            b"\xff\x53\x74",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_NEXT_VPTR:
            b"\x8b\x06",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_CLEANUP:
            b"\x83\xc4\x0c",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_NEXT_RECEIVER:
            b"\x8b\xce",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_NEXT_CALL:
            b"\xff\x50\x20",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CANDIDATE_TAIL_POP_EDI:
            b"\x5f",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CANDIDATE_TAIL_POP_ESI:
            b"\x5e",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CANDIDATE_TAIL_POP_EBX:
            b"\x5b",
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CANDIDATE_TAIL_RETURN:
            b"\xc2\x04\x00",
    }
    if any(
        offset not in candidate_by_offset
        or body(candidate_by_offset[offset]) != expected
        for offset, expected in candidate_fixed.items()
    ):
        raise ValueError(
            "HUD message numeric SetTextFmt bridge requires the exact "
            "candidate ceil/_ftol result, EBX-vptr/EDI-receiver reaching "
            "definitions, variadic arguments, cleanup, and successor "
            "Invalidate EAX-vptr/ESI-to-ECX receiver/function-tail bytes"
        )
    candidate_fixed_indices = [
        candidate_index_by_offset[offset] for offset in candidate_fixed
    ]
    candidate_call_index = candidate_index_by_offset[
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_CALL
    ]
    candidate_next_call_index = candidate_index_by_offset[
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_NEXT_CALL
    ]
    candidate_invocations = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    candidate_invocation_order = tuple(
        candidate_offsets[index] for index in candidate_invocations
    )
    bounded_indices = set(
        range(
            candidate_fixed_indices[0],
            candidate_next_call_index + 1,
        )
    )
    if (
        candidate_fixed_indices
        != list(
            range(
                candidate_fixed_indices[0],
                candidate_fixed_indices[0] + len(candidate_fixed),
            )
        )
        or candidate_invocation_order
        != _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CANDIDATE_INVOCATION_ORDER
        or candidate_invocations.index(candidate_call_index)
        != _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CALL_ORDINAL
        or candidate_invocations.index(candidate_next_call_index)
        != _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_INVALIDATE_CALL_ORDINAL
        or _cc_cfg._instruction_operand(
            candidate_by_offset[
                _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_FTOL_CANDIDATE_CALL_OFFSET
            ]
        ).strip()
        != _cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL
        or _cc_cfg._instruction_operand(
            candidate_by_offset[
                _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_RESULT_ARGUMENT
            ]
        ).strip().lower()
        != "eax"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(
                candidate_by_offset[
                    _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_CALL
                ]
            )
        )
        not in {"ebx+116", "ebx+0x74"}
        or _cc_cfg._cleanup_after(candidate.instructions, candidate_call_index)
        != _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CLEANUP_BYTES
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(
                candidate_by_offset[
                    _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_NEXT_CALL
                ]
            )
        )
        not in {"eax+32", "eax+0x20"}
        or _cc_cfg._cleanup_after(candidate.instructions, candidate_next_call_index)
        is not None
        or candidate_index_by_offset[
            _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CANDIDATE_TAIL_RETURN
        ]
        != len(candidate.instructions) - 1
        or candidate.local_control_flow_indices & bounded_indices
        or any(
            target in bounded_indices
            for targets in candidate.local_control_flow_targets.values()
            for target in targets
        )
    ):
        raise ValueError(
            "HUD message numeric SetTextFmt bridge rejects candidate "
            "ordinal-3 slot-0x74/cleanup-12, governed EAX integer result, "
            "complete invocation order, or immediate ordinal-4 Invalidate "
            "EAX-vptr/ESI-to-ECX receiver/slot-0x20/no-cleanup/"
            "function-tail topology drift"
        )

    format_relocations = [
        relocation
        for relocation in definition.relocations
        if relocation.offset
        == _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_FORMAT_RELOCATION
    ]
    format_offset = (
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_FORMAT_RELOCATION
    )
    if (
        len(format_relocations) != 1
        or format_relocations[0].type != IMAGE_REL_I386_DIR32
        or format_relocations[0].symbol_name
        != _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_FORMAT_SYMBOL
        or struct.unpack_from("<I", definition.data, format_offset)[0] != 0
        or not all(
            definition.relocation_mask[position]
            for position in range(format_offset, format_offset + 4)
        )
        or definition.defined_external_data.count(
            _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_FORMAT_SYMBOL
        )
        != 1
        or (
            definition.undefined_external_data
            + definition.undefined_external_functions
            + definition.defined_external_functions
        ).count(_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_FORMAT_SYMBOL)
        != 0
    ):
        raise ValueError(
            "HUD message numeric SetTextFmt bridge rejects candidate `%d` "
            "DIR32 target, zero addend, external identity, or relocation-mask "
            "drift; format_relocations="
            f"{tuple((row.offset, row.type, row.symbol_name) for row in format_relocations)!r}, "
            "format_addend="
            f"{struct.unpack_from('<I', definition.data, format_offset)[0]!r}, "
            "format_mask="
            f"{tuple(definition.relocation_mask[format_offset:format_offset + 4])!r}, "
            "external_counts="
            f"{(definition.undefined_external_data.count(_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_FORMAT_SYMBOL), definition.defined_external_data.count(_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_FORMAT_SYMBOL), definition.undefined_external_functions.count(_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_FORMAT_SYMBOL), definition.defined_external_functions.count(_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_FORMAT_SYMBOL))!r}"
        )

    retail_result = dict(retail_bridges)
    candidate_result = dict(candidate_bridges)
    retail_result[
        normalize_address(_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_RETAIL_CALL)
    ] = ReviewedLoopVptrStorageBridge(
        register="edx",
        storage_identity=expected_storage,
        slot_displacement=_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_SLOT,
        assembly_source="bn",
    )
    candidate_result[
        normalize_address(hex(_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_CALL))
    ] = ReviewedLoopVptrStorageBridge(
        register="ebx",
        storage_identity=expected_storage,
        slot_displacement=_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_SLOT,
        assembly_source="cod",
    )
    retail_result[
        normalize_address(_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_RETAIL_NEXT_CALL)
    ] = ReviewedLoopVptrStorageBridge(
        register="eax",
        storage_identity=_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_MESSAGE_VPTR_IDENTITY,
        slot_displacement=_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_INVALIDATE_SLOT,
        assembly_source="bn",
    )
    candidate_result[
        normalize_address(
            hex(_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_NEXT_CALL)
        )
    ] = ReviewedLoopVptrStorageBridge(
        register="eax",
        storage_identity=_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_MESSAGE_VPTR_IDENTITY,
        slot_displacement=_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_INVALIDATE_SLOT,
        assembly_source="cod",
    )
    return retail_result, candidate_result


def _hud_ui_message_clear_display_vptr_bridges(
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
) -> tuple[
    dict[str, ReviewedLoopVptrStorageBridge],
    dict[str, ReviewedLoopVptrStorageBridge],
]:
    """Derive ClearDisplay's retail and candidate virtual calls independently."""
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_CALLER_START:
        return {}, {}

    symbols = document.collection("symbols")
    caller_row = symbols.get(
        _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_CALLER_IDENTITY.removeprefix("symbol:")
    )
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
        == _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_CALLER_START
    ]
    if (
        caller_identity != _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(normalized_start) != caller_identity
        or caller_identity in indexes.provider_ids
        or not isinstance(caller_row, Mapping)
        or caller_row.get("binary") != "recoil"
        or caller_row.get("kind") != "function"
        or caller_row.get("pipeline_class") != "authored"
        or caller_row.get("ownership_state") != "primary-owned"
        or caller_row.get("address") != normalized_start
        or caller_row.get("end_exclusive")
        != _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_CALLER_END_EXCLUSIVE
        or caller_row.get("extent_state") != "known"
        or caller_row.get("size") != 0x50
        or caller_row.get("navigation_name")
        != "HudUiMessage::ClearDisplay"
        or caller_row.get("output_section_id") != "recoil:section:.text"
        or caller_row.get("physical_block_id") != "recoil:block:0x404ca0"
        or not exact_required_target_membership(
            caller_row.get("verification_target_ids", ()),
            (_cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_ORDER_TARGET_ID, _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_BYTE_TARGET_ID),
        )
        or not isinstance(trace, Mapping)
        or trace.get("state") != "resolved"
        or trace.get("reason_code") not in {None, ""}
        or not isinstance(source_edges, list)
        or len(source_edges) != 1
        or not isinstance(source_edges[0], Mapping)
        or source_edges[0].get("relation") != "defines"
        or source_edges[0].get("anchor_id")
        != _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
        or caller is None
        or caller.symbol != _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_CALLER_SYMBOL
        or len(caller.data) != 0x50
        or len(caller.data) != len(caller.relocation_mask)
        or target is None
        or getattr(target, "name", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_TARGET_NAME
        or getattr(target, "target_binary", "") != "recoil"
        or Path(str(getattr(target, "manifest_path", ""))).resolve()
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_TARGET_MANIFEST.resolve()
        or not bool(
            getattr(
                target,
                "check_translation_unit_function_order",
                False,
            )
        )
        or tuple(getattr(target, "source_files", ())).count(
            _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        )
        != 1
        or len(contribution_rows) != 1
    ):
        raise ValueError(
            "HUD message ClearDisplay bridge requires the exact authored "
            "caller, source edge, extent, object symbol, and candidate "
            "contribution authority"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != r"\?ClearDisplay@HudUiMessage@@.*"
        or getattr(contribution_row, "name", "")
        != "HudUiMessage::ClearDisplay"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(
            getattr(contribution_row, "required_presence", False)
        )
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD message ClearDisplay bridge requires the exact authored "
            "hud.cpp target contribution row"
        )

    aggregate_row = symbols.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID)
    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    aggregate_start = address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
    aggregate_containers = [
        row
        for row in indexes.storage_containers
        if (
            row.identity == aggregate_identity
            and row.start == aggregate_start
            and row.end_exclusive
            == aggregate_start + _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
        )
    ]
    literal_row = symbols.get(_cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_EMPTY_LITERAL_ID)
    literal_identity = (
        f"storage:{_cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_EMPTY_LITERAL_ID}"
    )
    literal_aliases = (
        literal_row.get("logical_aliases")
        if isinstance(literal_row, Mapping)
        else None
    )
    literal_pooling_rows = [
        alias
        for alias in (
            literal_aliases.values()
            if isinstance(literal_aliases, Mapping)
            else ()
        )
        if isinstance(alias, Mapping)
        and alias.get("object_symbol")
        == _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_EMPTY_LITERAL_SYMBOL
        and isinstance(alias.get("pooling"), Mapping)
        and alias["pooling"].get("mode") == "compiler-literal-pooling"
        and alias["pooling"].get("physical_artifact_id")
        == _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_EMPTY_LITERAL_ID
    ]
    if (
        not isinstance(aggregate_row, Mapping)
        or aggregate_row.get("binary") != "recoil"
        or aggregate_row.get("kind") != "data"
        or aggregate_row.get("address") != _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
        or aggregate_row.get("disposition") != "authored"
        or aggregate_row.get("navigation_name") != _cc_catalog.HUD_UI_MGR_AGGREGATE_NAME
        or aggregate_row.get("output_section_id") != "recoil:section:.data"
        or aggregate_row.get("storage_contribution_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_STORAGE_ID]
        or aggregate_row.get("verification_target_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_ID]
        or indexes.storage_by_address.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
        != aggregate_identity
        or len(aggregate_containers) != 1
        or aggregate_identity in indexes.provider_ids
        or aggregate_start + _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_ARRAY_DISPLACEMENT
        != 0x4EA9E8
        or not isinstance(literal_row, Mapping)
        or literal_row.get("binary") != "recoil"
        or literal_row.get("kind") != "data"
        or literal_row.get("disposition") != "provider"
        or literal_row.get("address")
        != _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_EMPTY_LITERAL_ADDRESS
        or literal_row.get("end_exclusive") != "0x4e5ce1"
        or literal_row.get("extent_state") != "known"
        or literal_row.get("size") != 1
        or literal_row.get("navigation_name")
        != _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_EMPTY_LITERAL_NAME
        or literal_row.get("output_section_id") != "recoil:section:.data"
        or literal_row.get("source_traceability")
        != {
            "reason_code": "compiler-linker-literal-pooling",
            "source_edges": [],
            "state": "not-applicable",
        }
        or literal_row.get("verification_target_ids")
        != []
        or indexes.storage_by_address.get(
            _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_EMPTY_LITERAL_ADDRESS
        )
        != literal_identity
        or not literal_pooling_rows
    ):
        raise ValueError(
            "HUD message ClearDisplay bridge requires the reviewed aggregate "
            "array extent and provider-pooled empty-literal identity"
        )
    if (
        indexes.by_address.get(
            _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_DIRECT_TARGET_ADDRESS
        )
        != _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_DIRECT_TARGET_IDENTITY
        or indexes.by_candidate_name.get(
            _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_DIRECT_TARGET_SYMBOL
        )
        != _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_DIRECT_TARGET_IDENTITY
        or _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_DIRECT_TARGET_IDENTITY
        in indexes.provider_ids
    ):
        raise ValueError(
            "HUD message ClearDisplay bridge requires the exact authored "
            "SetImageBorrowedAndInvalidate direct target identity"
        )

    def body(instruction: Instruction) -> bytes:
        return bytes(int(value, 16) for value in instruction.bytes)

    retail_addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(normalized_start),
    )
    retail_counts: dict[int, int] = {}
    for address in retail_addresses:
        if address is not None:
            retail_counts[address] = retail_counts.get(address, 0) + 1
    retail_by_address = {
        address: instruction
        for instruction, address in zip(
            retail_instructions,
            retail_addresses,
        )
        if address is not None and retail_counts.get(address) == 1
    }
    retail_index_by_address = {
        address: index
        for index, address in enumerate(retail_addresses)
        if address is not None and retail_counts.get(address) == 1
    }
    retail_fixed = {
        0x4127D0: b"\x8d\x04\x89",
        0x4127D3: b"\x56",
        0x4127D4: b"\x6a\x00",
        0x4127D6: b"\x8d\x04\x41",
        0x4127D9: b"\x8d\x04\x80",
        0x4127DC: b"\x8d\x0c\x80",
        0x4127DF: b"\x8d\x34\x8d\xe8\xa9\x4e\x00",
        0x4127E6: b"\x8b\xce",
        0x4127E8: b"\xe8\x83\x16\x0a\x00",
        0x4127ED: b"\x8d\x8e\x90\x03\x00\x00",
        0x4127F3: b"\x6a\x00",
        0x4127F5: b"\xe8\x76\x16\x0a\x00",
        0x4127FA: b"\x8b\x96\xe0\x00\x00\x00",
        0x412800: b"\x8d\x86\xe0\x00\x00\x00",
        0x412806: b"\x68\xe0\x5c\x4e\x00",
        0x41280B: b"\x50",
        0x41280C: b"\xff\x52\x74",
        0x41280F: b"\x8b\x06",
        0x412811: b"\x83\xc4\x08",
        0x412814: b"\x8b\xce",
        0x412816: b"\xff\x50\x20",
        0x412819: b"\x5e",
        0x41281A: b"\xc3",
    }
    if any(
        address not in retail_by_address
        or body(retail_by_address[address]) != expected
        for address, expected in retail_fixed.items()
    ):
        raise ValueError(
            "HUD message ClearDisplay bridge requires the exact retail "
            "indexed-message/direct-call/panel-SetTextFmt/Invalidate/tail bytes"
        )
    retail_indices = [
        retail_index_by_address[address] for address in retail_fixed
    ]
    retail_panel_call_index = retail_index_by_address[
        _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_RETAIL_PANEL_CALL
    ]
    retail_invalidate_call_index = retail_index_by_address[
        _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_RETAIL_INVALIDATE_CALL
    ]
    retail_calls = tuple(
        address
        for address, instruction in zip(
            retail_addresses,
            retail_instructions,
        )
        if address is not None
        and _cc_cfg._instruction_mnemonic(instruction) == "call"
    )
    if (
        retail_indices
        != list(range(retail_indices[0], retail_indices[0] + len(retail_fixed)))
        or retail_calls != _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_RETAIL_CALL_ORDER
        or _cc_cfg._cleanup_after(retail_instructions, retail_panel_call_index)
        != _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_RETAIL_PANEL_CLEANUP
        or _cc_cfg._cleanup_after(retail_instructions, retail_invalidate_call_index)
        is not None
        or retail_index_by_address[0x41281A]
        != len(retail_instructions) - 1
    ):
        raise ValueError(
            "HUD message ClearDisplay bridge rejects retail four-call order, "
            "two-argument SetTextFmt cleanup, successor Invalidate, or "
            "function-tail topology drift"
        )

    candidate_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    candidate_counts: dict[int, int] = {}
    for offset in candidate_offsets:
        if offset is not None:
            candidate_counts[offset] = candidate_counts.get(offset, 0) + 1
    candidate_by_offset = {
        offset: instruction
        for instruction, offset in zip(
            candidate.instructions,
            candidate_offsets,
        )
        if offset is not None and candidate_counts.get(offset) == 1
    }
    candidate_index_by_offset = {
        offset: index
        for index, offset in enumerate(candidate_offsets)
        if offset is not None and candidate_counts.get(offset) == 1
    }
    candidate_prefix = {
        0x00: b"\x8d\x04\x89",
        0x03: b"\x56",
        0x04: b"\x6a\x00",
        0x06: b"\x8d\x04\x41",
        0x09: b"\x8d\x04\x80",
        0x0C: b"\x8d\x0c\x80",
        0x0F: b"\x8d\x34\x8d\x18\x4b\x00\x00",
        0x16: b"\x8b\xce",
        0x18: b"\xe8\x00\x00\x00\x00",
        0x1D: b"\x8d\x8e\x90\x03\x00\x00",
        0x23: b"\x6a\x00",
        0x25: b"\xe8\x00\x00\x00\x00",
        0x2A: b"\x8b\x96\xe0\x00\x00\x00",
    }
    candidate_variants = {
        "set-text": {
            "fixed": {
                **candidate_prefix,
                0x30: b"\x8d\x8e\xe0\x00\x00\x00",
                0x36: b"\x68\x00\x00\x00\x00",
                0x3B: b"\xff\x92\x8c\x00\x00\x00",
                0x41: b"\x8b\x06",
                0x43: b"\x8b\xce",
                0x45: b"\xff\x50\x20",
                0x48: b"\x5e",
                0x49: b"\xc3",
            },
            "call_order": _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_CANDIDATE_CALL_ORDER,
            "panel_call": _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_CANDIDATE_PANEL_CALL,
            "invalidate_call":
                _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_CANDIDATE_INVALIDATE_CALL,
            "panel_slot": _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_CANDIDATE_PANEL_SLOT,
            "panel_cleanup": None,
            "tail": 0x49,
        },
        "set-text-fmt": {
            "fixed": {
                **candidate_prefix,
                0x30: b"\x8d\x86\xe0\x00\x00\x00",
                0x36: b"\x68\x00\x00\x00\x00",
                0x3B: b"\x50",
                0x3C: b"\xff\x52\x74",
                0x3F: b"\x8b\x06",
                0x41: b"\x83\xc4\x08",
                0x44: b"\x8b\xce",
                0x46: b"\xff\x50\x20",
                0x49: b"\x5e",
                0x4A: b"\xc3",
            },
            "call_order": (
                _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_CORRECTED_CANDIDATE_CALL_ORDER
            ),
            "panel_call": (
                _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_CORRECTED_CANDIDATE_PANEL_CALL
            ),
            "invalidate_call": (
                _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_CORRECTED_CANDIDATE_INVALIDATE_CALL
            ),
            "panel_slot": _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_RETAIL_PANEL_SLOT,
            "panel_cleanup": (
                _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_CORRECTED_CANDIDATE_PANEL_CLEANUP
            ),
            "tail": 0x4A,
        },
    }

    def candidate_fixed_matches(
        fixed: Mapping[int, bytes],
    ) -> bool:
        return all(
            offset in candidate_by_offset
            and body(candidate_by_offset[offset]) == expected
            and caller.data[offset : offset + len(expected)] == expected
            for offset, expected in fixed.items()
        )

    matching_candidate_variants = [
        (name, variant)
        for name, variant in candidate_variants.items()
        if candidate_fixed_matches(variant["fixed"])
    ]
    if len(matching_candidate_variants) != 1:
        raise ValueError(
            "HUD message ClearDisplay bridge requires the exact candidate "
            "indexed-message/direct-call/panel-SetText or corrected "
            "panel-SetTextFmt/Invalidate/tail COD and COFF bytes"
        )
    candidate_variant_name, candidate_variant = (
        matching_candidate_variants[0]
    )
    candidate_fixed = candidate_variant["fixed"]
    candidate_indices = [
        candidate_index_by_offset[offset] for offset in candidate_fixed
    ]
    candidate_panel_call_index = candidate_index_by_offset[
        candidate_variant["panel_call"]
    ]
    candidate_invalidate_call_index = candidate_index_by_offset[
        candidate_variant["invalidate_call"]
    ]
    candidate_invocations = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    candidate_calls = tuple(
        candidate_offsets[index] for index in candidate_invocations
    )
    if (
        candidate_indices
        != list(
            range(
                candidate_indices[0],
                candidate_indices[0] + len(candidate_fixed),
            )
        )
        or candidate_calls
        != candidate_variant["call_order"]
        or _cc_cfg._cleanup_after(candidate.instructions, candidate_panel_call_index)
        != candidate_variant["panel_cleanup"]
        or _cc_cfg._cleanup_after(
            candidate.instructions,
            candidate_invalidate_call_index,
        )
        is not None
        or candidate_index_by_offset[candidate_variant["tail"]]
        != len(candidate.instructions) - 1
        or candidate.local_control_flow_indices
        or candidate.local_control_flow_targets
    ):
        raise ValueError(
            "HUD message ClearDisplay bridge rejects candidate four-call "
            "order, exact SetText/SetTextFmt argument and cleanup topology, "
            "successor Invalidate, or function-tail topology drift"
        )

    expected_relocations = {
        0x12: (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_ARRAY_DISPLACEMENT,
        ),
        0x19: (
            IMAGE_REL_I386_REL32,
            _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_DIRECT_TARGET_SYMBOL,
            0,
        ),
        0x26: (
            IMAGE_REL_I386_REL32,
            _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_DIRECT_TARGET_SYMBOL,
            0,
        ),
        0x37: (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_EMPTY_LITERAL_SYMBOL,
            0,
        ),
    }
    expected_mask = {
        position
        for offset in expected_relocations
        for position in range(offset, offset + 4)
    }
    if (
        len(caller.relocations) != len(expected_relocations)
        or {row.offset for row in caller.relocations}
        != set(expected_relocations)
        or any(
            (
                row.type,
                row.symbol_name,
                struct.unpack_from("<I", caller.data, row.offset)[0],
            )
            != expected_relocations[row.offset]
            for row in caller.relocations
        )
        or {
            index
            for index, masked in enumerate(caller.relocation_mask)
            if masked
        }
        != expected_mask
        or caller.undefined_external_functions.count(
            _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_DIRECT_TARGET_SYMBOL
        )
        != 1
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or caller.defined_external_data.count(
            _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_EMPTY_LITERAL_SYMBOL
        )
        != 1
        or (
            caller.defined_external_functions
            + caller.undefined_external_data
            + caller.defined_external_data
        ).count(_cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_DIRECT_TARGET_SYMBOL)
        != 0
        or (
            caller.undefined_external_functions
            + caller.defined_external_functions
            + caller.defined_external_data
        ).count(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
        != 0
        or (
            caller.undefined_external_functions
            + caller.defined_external_functions
            + caller.undefined_external_data
        ).count(_cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_EMPTY_LITERAL_SYMBOL)
        != 0
    ):
        raise ValueError(
            "HUD message ClearDisplay bridge rejects candidate aggregate, "
            "direct-target, or pooled-empty-literal COFF relocation "
            "identity/type/addend/mask/provenance drift"
        )

    retail_bridges = {
        normalize_address(
            _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_RETAIL_PANEL_CALL
        ): ReviewedLoopVptrStorageBridge(
            register="edx",
            storage_identity=_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_PANEL_STORAGE_IDENTITY,
            slot_displacement=(
                _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_RETAIL_PANEL_SLOT
            ),
            assembly_source="bn",
        ),
        normalize_address(
            _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_RETAIL_INVALIDATE_CALL
        ): ReviewedLoopVptrStorageBridge(
            register="eax",
            storage_identity=_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_MESSAGE_VPTR_IDENTITY,
            slot_displacement=_cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_INVALIDATE_SLOT,
            assembly_source="bn",
        ),
    }
    candidate_bridges = {
        normalize_address(
            hex(candidate_variant["panel_call"])
        ): ReviewedLoopVptrStorageBridge(
            register="edx",
            storage_identity=_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_PANEL_STORAGE_IDENTITY,
            slot_displacement=candidate_variant["panel_slot"],
            assembly_source="cod",
        ),
        normalize_address(
            hex(candidate_variant["invalidate_call"])
        ): ReviewedLoopVptrStorageBridge(
            register="eax",
            storage_identity=_cc_catalog.HUD_UI_MESSAGE_SET_VALUE_MESSAGE_VPTR_IDENTITY,
            slot_displacement=_cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_INVALIDATE_SLOT,
            assembly_source="cod",
        ),
    }
    direct_contracts = [
        {
            "ordinal": ordinal,
            "form": "call",
            "dispatch": "direct",
            "identity_kind": "direct",
            "target_identity":
                _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_DIRECT_TARGET_IDENTITY,
            "storage_identity": "",
            "slot_displacement": None,
            "cleanup_bytes": None,
        }
        for ordinal in (0, 1)
    ]
    expected_retail_contract = [
        *direct_contracts,
        {
            "ordinal": 2,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity":
                _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_PANEL_STORAGE_IDENTITY,
            "slot_displacement":
                _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_RETAIL_PANEL_SLOT,
            "cleanup_bytes":
                _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_RETAIL_PANEL_CLEANUP,
        },
        {
            "ordinal": 3,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity":
                _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_MESSAGE_VPTR_IDENTITY,
            "slot_displacement":
                _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_INVALIDATE_SLOT,
            "cleanup_bytes": None,
        },
    ]
    expected_candidate_contract = [
        *direct_contracts,
        {
            "ordinal": 2,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity":
                _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_PANEL_STORAGE_IDENTITY,
            "slot_displacement": candidate_variant["panel_slot"],
            "cleanup_bytes": candidate_variant["panel_cleanup"],
        },
        {
            "ordinal": 3,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity":
                _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_MESSAGE_VPTR_IDENTITY,
            "slot_displacement":
                _cc_catalog.HUD_UI_MESSAGE_CLEAR_DISPLAY_INVALIDATE_SLOT,
            "cleanup_bytes": None,
        },
    ]
    retail_contract = _cc_extraction.extract_invocation_contract(
        retail_instructions,
        source="bn",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge_names=bridge_names,
        reviewed_loop_vptr_storage_bridges=retail_bridges,
    )
    candidate_contract = _cc_extraction.extract_invocation_contract(
        candidate.instructions,
        source="cod",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        reviewed_loop_vptr_storage_bridges=candidate_bridges,
    )
    if (
        retail_contract != expected_retail_contract
        or candidate_contract != expected_candidate_contract
    ):
        raise ValueError(
            "HUD message ClearDisplay bridge cannot independently derive "
            "the exact retail SetTextFmt and candidate "
            f"{candidate_variant_name} contract"
        )
    return retail_bridges, candidate_bridges


def _hud_ui_message_update_selected_weapon_vptr_bridges(
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
) -> tuple[
    IdentityIndexes,
    dict[str, ReviewedLoopVptrStorageBridge],
    dict[str, ReviewedLoopVptrStorageBridge],
]:
    """Prove both complete static contracts without equating their populations.

    Retail inlines the selected-message and fixed-slot-1 variant-display paths.
    The legacy candidate retains three direct SelectVariantDisplay calls; the
    corrected retained-source candidate expands those paths into nine direct
    SetImageBorrowedAndInvalidate calls.  Each candidate shape is independently
    proved against its exact COD/COFF topology without changing retail truth.
    """
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CALLER_START:
        return indexes, {}, {}

    caller_symbol_id = (
        _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CALLER_IDENTITY.removeprefix("symbol:")
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
    candidate_variant = (
        "legacy-select-variant"
        if caller is not None and len(caller.data) == 0x100
        else (
            "corrected-retained-source"
            if caller is not None and len(caller.data) == 0x2C0
            else (
                "retail-shaped-19call"
                if caller is not None and len(caller.data) == 0x340
                else ""
            )
        )
    )
    target = candidate.target
    caller_contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CALLER_START
    ]
    select_variant_contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECT_VARIANT_ADDRESS
    ]
    if (
        caller_identity != _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(normalized_start) != caller_identity
        or caller_identity in indexes.provider_ids
        or not isinstance(caller_row, Mapping)
        or caller_row.get("binary") != "recoil"
        or caller_row.get("kind") != "function"
        or caller_row.get("pipeline_class") != "authored"
        or caller_row.get("ownership_state") != "primary-owned"
        or caller_row.get("address") != normalized_start
        or caller_row.get("end_exclusive")
        != _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CALLER_END_EXCLUSIVE
        or caller_row.get("extent_state") != "known"
        or caller_row.get("size") != 0x340
        or caller_row.get("navigation_name")
        != "HudUiMessage::UpdateSelectedWeaponDisplay"
        or caller_row.get("output_section_id") != "recoil:section:.text"
        or caller_row.get("physical_block_id") != "recoil:block:0x404ca0"
        or (
            "recoil:vc5-target:hud_404ca0_415ab0_authored_order"
            not in caller_row.get("verification_target_ids", ())
        )
        or not isinstance(trace, Mapping)
        or trace.get("state") != "resolved"
        or trace.get("reason_code") not in {None, ""}
        or not isinstance(source_edges, list)
        or len(source_edges) != 1
        or not isinstance(source_edges[0], Mapping)
        or source_edges[0].get("relation") != "defines"
        or source_edges[0].get("anchor_id")
        != _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
        or caller is None
        or caller.symbol != _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CALLER_SYMBOL
        or not candidate_variant
        or len(caller.relocation_mask) != len(caller.data)
        or target is None
        or getattr(target, "name", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_TARGET_NAME
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
        or len(caller_contribution_rows) != 1
        or len(select_variant_contribution_rows) != 1
    ):
        raise ValueError(
            "HUD message UpdateSelectedWeaponDisplay bridge requires the exact "
            "authored caller, source edge, candidate body, and target "
            "contribution authority"
        )
    contribution, contribution_row = caller_contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") not in {"", None}
        or getattr(contribution_row, "symbol_regex", "")
        != r"\?UpdateSelectedWeaponDisplay@HudUiMessage@@.*"
        or getattr(contribution_row, "name", "")
        != "HudUiMessage::UpdateSelectedWeaponDisplay"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD message UpdateSelectedWeaponDisplay bridge rejects target "
            "contribution-row drift"
        )
    (
        select_variant_contribution,
        select_variant_contribution_row,
    ) = select_variant_contribution_rows[0]
    if (
        getattr(select_variant_contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(select_variant_contribution, "order_scope", "")
        != "authored"
        or getattr(select_variant_contribution_row, "symbol", None) != ""
        or getattr(select_variant_contribution_row, "symbol_regex", None)
        != r"\?SelectVariantDisplay@HudUiMessage@@.*"
        or getattr(select_variant_contribution_row, "name", "")
        != "HudUiMessage::SelectVariantDisplay"
        or getattr(select_variant_contribution_row, "pipeline_class", "")
        != "authored"
        or getattr(
            select_variant_contribution_row,
            "authored_order_role",
            "",
        )
        != "authored-body"
        or getattr(
            select_variant_contribution_row,
            "required_presence",
            None,
        )
        is not True
        or getattr(
            select_variant_contribution_row,
            "full_order_gate",
            None,
        )
        is not True
    ):
        raise ValueError(
            "HUD message UpdateSelectedWeaponDisplay bridge rejects exact "
            "SelectVariantDisplay target contribution-row authority drift"
        )
    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    aggregate_containers = [
        row
        for row in indexes.storage_containers
        if row.identity == aggregate_identity
    ]
    aggregate_row = document.collection("symbols").get(
        _cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID
    )
    if (
        not isinstance(aggregate_row, Mapping)
        or aggregate_row.get("binary") != "recoil"
        or aggregate_row.get("kind") != "data"
        or aggregate_row.get("address") != _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
        or aggregate_row.get("disposition") != "authored"
        or aggregate_row.get("navigation_name") != _cc_catalog.HUD_UI_MGR_AGGREGATE_NAME
        or aggregate_row.get("output_section_id") != "recoil:section:.data"
        or aggregate_row.get("storage_contribution_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_STORAGE_ID]
        or indexes.storage_by_address.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
        != aggregate_identity
        or len(aggregate_containers) != 1
        or aggregate_containers[0].start
        != address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
        or aggregate_containers[0].end_exclusive <= 0x4EAF14
        or aggregate_identity in indexes.provider_ids
    ):
        raise ValueError(
            "HUD message UpdateSelectedWeaponDisplay bridge requires the "
            "reviewed aggregate storage authority through fixed slot 1"
        )

    set_image_identity = indexes.by_address.get(
        _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SET_IMAGE_ADDRESS,
        "",
    )
    select_variant_identity = indexes.by_address.get(
        _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECT_VARIANT_ADDRESS,
        "",
    )
    ftol_identity = indexes.by_address.get(
        _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_FTOL_ADDRESS,
        "",
    )
    if (
        set_image_identity
        != _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SET_IMAGE_IDENTITY
        or indexes.by_candidate_name.get(
            _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SET_IMAGE_SYMBOL
        )
        != set_image_identity
        or set_image_identity in indexes.provider_ids
        or select_variant_identity
        != _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECT_VARIANT_IDENTITY
        or select_variant_identity in indexes.provider_ids
        or not ftol_identity
        or ftol_identity not in indexes.provider_ids
    ):
        raise ValueError(
            "HUD message UpdateSelectedWeaponDisplay bridge requires exact "
            "SetImageBorrowedAndInvalidate, authored SelectVariantDisplay, "
            "and ftol target/provider authority"
        )

    by_candidate_name = dict(indexes.by_candidate_name)
    existing_select_variant_identity = by_candidate_name.get(
        _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECT_VARIANT_SYMBOL,
        "",
    )
    if (
        existing_select_variant_identity
        and existing_select_variant_identity != select_variant_identity
    ):
        raise ValueError(
            "HUD message UpdateSelectedWeaponDisplay bridge rejects "
            "conflicting SelectVariantDisplay candidate-name identity"
        )
    by_candidate_name[
        _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECT_VARIANT_SYMBOL
    ] = select_variant_identity
    storage_by_address = dict(indexes.storage_by_address)
    storage_by_name = dict(indexes.storage_by_name)
    for key, existing in (
        (
            _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CEIL_IAT_ADDRESS,
            storage_by_address.get(
                _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CEIL_IAT_ADDRESS,
                "",
            ),
        ),
        ("ceil", storage_by_name.get("ceil", "")),
        ("__imp__ceil", storage_by_name.get("__imp__ceil", "")),
    ):
        if existing and existing != _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CEIL_IAT_IDENTITY:
            raise ValueError(
                "HUD message UpdateSelectedWeaponDisplay ceil bridge rejects "
                f"conflicting existing storage identity for {key!r}"
            )
    storage_by_address[
        _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CEIL_IAT_ADDRESS
    ] = _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CEIL_IAT_IDENTITY
    storage_by_name["ceil"] = _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CEIL_IAT_IDENTITY
    storage_by_name[
        "__imp__ceil"
    ] = _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CEIL_IAT_IDENTITY
    derived_indexes = replace(
        indexes,
        by_candidate_name=by_candidate_name,
        storage_by_address=storage_by_address,
        storage_by_name=storage_by_name,
    )

    def instruction_maps(
        instructions: Sequence[Instruction],
        *,
        source: str,
    ) -> tuple[dict[int, Instruction], dict[int, int]]:
        addresses = (
            _cc_cfg._instruction_runtime_addresses(
                instructions,
                source="bn",
                caller_start=address_value(normalized_start),
            )
            if source == "bn"
            else _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
        )
        counts: dict[int, int] = {}
        for address in addresses:
            if address is not None:
                counts[address] = counts.get(address, 0) + 1
        by_address = {
            address: instruction
            for instruction, address in zip(instructions, addresses)
            if address is not None and counts.get(address) == 1
        }
        index_by_address = {
            address: index
            for index, address in enumerate(addresses)
            if address is not None and counts.get(address) == 1
        }
        return by_address, index_by_address

    def encoded(instruction: Instruction) -> bytes:
        return bytes(int(value, 16) for value in instruction.bytes)

    retail_by_address, retail_index = instruction_maps(
        retail_instructions,
        source="bn",
    )
    retail_exact = {
        0x4129DC: b"\x8b\x8e\xe0\x00\x00\x00",
        0x4129E2: b"\x8d\x86\xe0\x00\x00\x00",
        0x4129E8: b"\x68\x08\xae\x4d\x00",
        0x4129ED: b"\x50",
        0x4129EE: b"\xff\x51\x74",
        0x4129F1: b"\x83\xc4\x08",
        0x4129F9: b"\xc2\x04\x00",
        0x412A14: b"\x8b\x96\xe0\x00\x00\x00",
        0x412A1A: b"\x8d\x8e\xe0\x00\x00\x00",
        0x412A20: b"\x50",
        0x412A21: b"\x68\xbc\xac\x4d\x00",
        0x412A26: b"\x51",
        0x412A27: b"\xff\x52\x74",
        0x412A2A: b"\x8b\x06",
        0x412A2C: b"\x83\xc4\x0c",
        0x412A2F: b"\x8b\xce",
        0x412A31: b"\xff\x50\x20",
        0x412A39: b"\xc2\x04\x00",
        0x412AE8: b"\xa1\x14\xaf\x4e\x00",
        0x412AED: b"\x68\x08\xae\x4d\x00",
        0x412AF2: b"\x68\x14\xaf\x4e\x00",
        0x412AF7: b"\xff\x50\x74",
        0x412AFA: b"\x83\xc4\x08",
        0x412B02: b"\xc2\x04\x00",
        0x412B1D: b"\x8b\x0d\x14\xaf\x4e\x00",
        0x412B23: b"\x50",
        0x412B24: b"\x68\xbc\xac\x4d\x00",
        0x412B29: b"\x68\x14\xaf\x4e\x00",
        0x412B2E: b"\xff\x51\x74",
        0x412B31: b"\x8b\x15\x34\xae\x4e\x00",
        0x412B37: b"\x83\xc4\x0c",
        0x412B3A: b"\xb9\x34\xae\x4e\x00",
        0x412B3F: b"\xff\x52\x20",
        0x412B47: b"\xc2\x04\x00",
        0x412B5D: b"\xc2\x04\x00",
    }
    if any(
        address not in retail_by_address
        or encoded(retail_by_address[address]) != expected
        for address, expected in retail_exact.items()
    ):
        raise ValueError(
            "HUD message UpdateSelectedWeaponDisplay bridge rejects exact "
            "retail selected/fixed SetTextFmt, Invalidate, or return bytes"
        )
    retail_terminal_addresses = {
        address
        for address, instruction in retail_by_address.items()
        if _cc_cfg._instruction_mnemonic(instruction) in {"ret", "retn"}
    }
    if retail_terminal_addresses != {
        0x4129F9,
        0x412A39,
        0x412B02,
        0x412B47,
        0x412B5D,
    }:
        raise ValueError(
            "HUD message UpdateSelectedWeaponDisplay bridge rejects the "
            "complete retail ret-4 terminal census"
        )
    retail_calls = tuple(
        address
        for address, instruction in zip(
            _cc_cfg._instruction_runtime_addresses(
                retail_instructions,
                source="bn",
                caller_start=address_value(normalized_start),
            ),
            retail_instructions,
        )
        if address is not None
        and _cc_cfg._instruction_mnemonic(instruction) == "call"
    )
    if (
        retail_calls != _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_RETAIL_CALL_ORDER
        or any(
            _cc_cfg._cleanup_after(retail_instructions, retail_index[address])
            != cleanup
            for address, _register, cleanup in (
                _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_RETAIL_SET_TEXT_CALLS
            )
        )
        or _cc_cfg._cleanup_after(retail_instructions, retail_index[0x412A06]) != 8
        or _cc_cfg._cleanup_after(retail_instructions, retail_index[0x412B0F]) != 8
    ):
        raise ValueError(
            "HUD message UpdateSelectedWeaponDisplay bridge rejects retail "
            "19-call order or caller-cleanup topology"
        )

    candidate_by_offset, candidate_index = instruction_maps(
        candidate.instructions,
        source="cod",
    )
    if candidate_variant == "legacy-select-variant":
        candidate_exact = {
            0x8D: b"\x8b\x96\xe0\x00\x00\x00",
            0x93: b"\x8d\x86\xe0\x00\x00\x00",
            0x99: b"\x68\x00\x00\x00\x00",
            0x9E: b"\x50",
            0x9F: b"\xff\x52\x74",
            0xA2: b"\x83\xc4\x08",
            0xA8: b"\xc2\x04\x00",
            0xAF: b"\x8b\x9e\xe0\x00\x00\x00",
            0xB5: b"\x83\xec\x08",
            0xB8: b"\x8d\xbe\xe0\x00\x00\x00",
            0xBE: b"\xdd\x1c\x24",
            0xC1: b"\xff\x15\x00\x00\x00\x00",
            0xC7: b"\x83\xc4\x08",
            0xCA: b"\xe8\x00\x00\x00\x00",
            0xCF: b"\x50",
            0xD0: b"\x68\x00\x00\x00\x00",
            0xD5: b"\x57",
            0xD6: b"\xff\x53\x74",
            0xD9: b"\x8b\x06",
            0xDB: b"\x83\xc4\x0c",
            0xDE: b"\x8b\xce",
            0xE0: b"\xff\x50\x20",
            0xE6: b"\xc2\x04\x00",
            0xF8: b"\xc2\x04\x00",
        }
        candidate_terminal_offsets = {0xA8, 0xE6, 0xF8}
        expected_candidate_calls = (
            _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CANDIDATE_CALL_ORDER
        )
        candidate_cleanup_expectations = (
            (0x9F, 8),
            (0xC1, 8),
            (0xD6, 12),
            (0xE0, None),
        )
    elif candidate_variant == "corrected-retained-source":
        candidate_exact = {
            **{
                offset: b"\xe8\x00\x00\x00\x00"
                for offset in (
                    0x40,
                    0x69,
                    0xA2,
                    0x107,
                    0x12E,
                    0x16B,
                    0x1A0,
                    0x1C6,
                    0x1FC,
                )
            },
            0x247: b"\x8b\x8e\xe0\x00\x00\x00",
            0x24D: b"\x8d\x86\xe0\x00\x00\x00",
            0x253: b"\x68\x00\x00\x00\x00",
            0x258: b"\x50",
            0x259: b"\xff\x51\x74",
            0x25C: b"\x83\xc4\x08",
            0x25F: b"\x5f",
            0x260: b"\x5e",
            0x261: b"\x5d",
            0x262: b"\x5b",
            0x263: b"\x59",
            0x264: b"\xc2\x04\x00",
            0x267: b"\xd9\x44\x24\x18",
            0x26B: b"\x8b\x9e\xe0\x00\x00\x00",
            0x271: b"\x83\xec\x08",
            0x274: b"\x8d\xbe\xe0\x00\x00\x00",
            0x27A: b"\xdd\x1c\x24",
            0x27D: b"\xff\x15\x00\x00\x00\x00",
            0x283: b"\x83\xc4\x08",
            0x286: b"\xe8\x00\x00\x00\x00",
            0x28B: b"\x50",
            0x28C: b"\x68\x00\x00\x00\x00",
            0x291: b"\x57",
            0x292: b"\xff\x53\x74",
            0x295: b"\x8b\x16",
            0x297: b"\x83\xc4\x0c",
            0x29A: b"\x8b\xce",
            0x29C: b"\xff\x52\x20",
            0x29F: b"\x5f",
            0x2A0: b"\x5e",
            0x2A1: b"\x5d",
            0x2A2: b"\x5b",
            0x2A3: b"\x59",
            0x2A4: b"\xc2\x04\x00",
            0x2B5: b"\x5f",
            0x2B6: b"\x5e",
            0x2B7: b"\x5d",
            0x2B8: b"\x5b",
            0x2B9: b"\x59",
            0x2BA: b"\xc2\x04\x00",
        }
        candidate_terminal_offsets = {0x264, 0x2A4, 0x2BA}
        expected_candidate_calls = (
            _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CORRECTED_CANDIDATE_CALL_ORDER
        )
        candidate_cleanup_expectations = (
            (0x259, 8),
            (0x27D, 8),
            (0x292, 12),
            (0x29C, None),
        )
    else:
        candidate_exact = {
            **{
                offset: b"\xe8\x00\x00\x00\x00"
                for offset in (
                    0x3E, 0x67, 0xA0, 0x103, 0x12A, 0x167,
                    0x22A, 0x252, 0x28C,
                )
            },
            0x1C6: b"\xff\x51\x74",
            0x1C9: b"\x83\xc4\x08",
            0x1D1: b"\xc2\x04\x00",
            0x1EA: b"\xff\x15\x00\x00\x00\x00",
            0x1F0: b"\x83\xc4\x08",
            0x1F3: b"\xe8\x00\x00\x00\x00",
            0x1FF: b"\xff\x53\x74",
            0x204: b"\x83\xc4\x0c",
            0x209: b"\xff\x52\x20",
            0x211: b"\xc2\x04\x00",
            0x2CF: b"\xff\x52\x74",
            0x2D2: b"\x83\xc4\x08",
            0x2DA: b"\xc2\x04\x00",
            0x2E7: b"\xff\x15\x00\x00\x00\x00",
            0x2ED: b"\x83\xc4\x08",
            0x2F0: b"\xe8\x00\x00\x00\x00",
            0x305: b"\xff\x50\x74",
            0x30E: b"\x83\xc4\x0c",
            0x316: b"\xff\x52\x20",
            0x31E: b"\xc2\x04\x00",
            0x334: b"\xc2\x04\x00",
        }
        candidate_terminal_offsets = {0x1D1, 0x211, 0x2DA, 0x31E, 0x334}
        expected_candidate_calls = (
            _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_RETAIL_SHAPED_CANDIDATE_CALL_ORDER
        )
        candidate_cleanup_expectations = (
            (0x1C6, 8), (0x1EA, 8), (0x1FF, 12), (0x209, None),
            (0x2CF, 8), (0x2E7, 8), (0x305, 12), (0x316, None),
        )
    candidate_exact_failures = [
        offset
        for offset, expected in candidate_exact.items()
        if (
        offset not in candidate_by_offset
        or encoded(candidate_by_offset[offset]) != expected
        or caller.data[offset : offset + len(expected)] != expected
        )
    ]
    if (
        candidate_variant == "corrected-retained-source"
        and caller.data[0x2BD:0x2C0] != b"\x90\x90\x90"
    ):
        candidate_exact_failures.append(0x2BD)
    if (
        candidate_variant == "retail-shaped-19call"
        and caller.data[0x337:0x340] != b"\x90" * 9
    ):
        candidate_exact_failures.append(0x337)
    if candidate_exact_failures:
        if candidate_variant == "legacy-select-variant":
            raise ValueError(
                "HUD message UpdateSelectedWeaponDisplay bridge rejects "
                "current candidate selected-message receiver, call, cleanup, "
                "or return COD/COFF bytes"
            )
        raise ValueError(
            "HUD message UpdateSelectedWeaponDisplay bridge rejects exact "
            f"{candidate_variant} image/SetTextFmt/Invalidate/ceil/ftol/"
            "cleanup/terminal COD/COFF bytes at "
            f"{[hex(offset) for offset in candidate_exact_failures]!r}"
        )
    observed_candidate_terminal_offsets = {
        offset
        for offset, instruction in candidate_by_offset.items()
        if _cc_cfg._instruction_mnemonic(instruction) in {"ret", "retn"}
    }
    if observed_candidate_terminal_offsets != candidate_terminal_offsets:
        raise ValueError(
            "HUD message UpdateSelectedWeaponDisplay bridge rejects the "
            "complete candidate ret-4 terminal census"
        )
    candidate_calls = tuple(
        _cc_callable_identity._candidate_complete_instruction_offsets(candidate)[index]
        for index in _cc_callable_identity._candidate_static_invocation_indices(
            candidate,
            caller_start=normalized_start,
            caller_end_exclusive=caller_end_exclusive,
        )
    )
    if (
        candidate_calls != expected_candidate_calls
        or any(
            _cc_cfg._cleanup_after(candidate.instructions, candidate_index[offset])
            != expected_cleanup
            for offset, expected_cleanup in candidate_cleanup_expectations
        )
    ):
        expected_count = len(expected_candidate_calls)
        raise ValueError(
            "HUD message UpdateSelectedWeaponDisplay bridge rejects candidate "
            f"{expected_count}-call order or caller-cleanup topology"
        )

    candidate_zero_float_symbol = (
        _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_RETAIL_SHAPED_ZERO_FLOAT_SYMBOL
    )
    candidate_clear_float_symbol = (
        _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CLEAR_FLOAT_SYMBOL
    )
    if candidate_variant == "retail-shaped-19call":
        zero_float_relocations = [
            row
            for row in caller.relocations
            if row.offset == 0xC4
        ]
        zero_float_relocation = (
            zero_float_relocations[0]
            if len(zero_float_relocations) == 1
            else None
        )
        zero_float_symbol = (
            zero_float_relocation.symbol_name
            if zero_float_relocation is not None
            else ""
        )
        zero_float_local_definitions = tuple(
            row
            for row in caller.local_static_definitions
            if row.name == zero_float_symbol
        )
        exact_zero_float_local_definitions = tuple(
            row
            for row in caller.local_static_definitions
            if row.name == zero_float_symbol
            and _cc_candidate._candidate_compiler_local_scalar_matches(
                row, b"\x00\x00\x00\x00"
            )
        )
        external_symbol_population = (
            caller.undefined_external_functions
            + caller.defined_external_functions
            + caller.undefined_external_data
            + caller.defined_external_data
        )
        if (
            zero_float_relocation is None
            or zero_float_relocation.type != IMAGE_REL_I386_DIR32
            or not zero_float_symbol
            or len(zero_float_local_definitions) != 1
            or len(exact_zero_float_local_definitions) != 1
            or zero_float_local_definitions[0]
            != exact_zero_float_local_definitions[0]
            or external_symbol_population.count(zero_float_symbol) != 0
            or struct.unpack_from("<I", caller.data, 0xC4)[0] != 0
            or not all(caller.relocation_mask[0xC4:0xC8])
        ):
            raise ValueError(
                "HUD message UpdateSelectedWeaponDisplay bridge requires "
                "exactly one caller-local retail-shaped zero-float "
                "compiler-local identity at relocation +0xc4"
            )
        candidate_zero_float_symbol = zero_float_symbol

        clear_float_offsets = {0x1A9, 0x2B4}
        clear_float_site_relocations = tuple(
            row
            for row in caller.relocations
            if row.offset in clear_float_offsets
        )
        clear_float_symbol_names = {
            row.symbol_name for row in clear_float_site_relocations
        }
        semantic_relocations = tuple(
            row
            for row in caller.relocations
            if _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CLEAR_FLOAT_PRIVATE_SYMBOL_RE.fullmatch(
                row.symbol_name
            )
        )
        semantic_local_definitions = tuple(
            row
            for row in caller.local_static_definitions
            if _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CLEAR_FLOAT_PRIVATE_SYMBOL_RE.fullmatch(
                row.name
            )
        )
        exact_semantic_local_definitions = tuple(
            row
            for row in semantic_local_definitions
            if _cc_candidate._candidate_local_read_only_scalar_matches(
                row, b"\xa3\x79\xeb\x4c"
            )
        )
        external_symbol_population = (
            caller.undefined_external_functions
            + caller.defined_external_functions
            + caller.undefined_external_data
            + caller.defined_external_data
        )
        external_semantic_names = {
            name
            for name in external_symbol_population
            if _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CLEAR_FLOAT_PRIVATE_SYMBOL_RE.fullmatch(
                name
            )
        }
        if (
            len(clear_float_site_relocations) != 2
            or {row.offset for row in clear_float_site_relocations}
            != clear_float_offsets
            or len(clear_float_symbol_names) != 1
            or any(
                row.type != IMAGE_REL_I386_DIR32
                or struct.unpack_from("<I", caller.data, row.offset)[0] != 0
                or not all(
                    caller.relocation_mask[
                        row.offset : row.offset + relocation_size(row.type)
                    ]
                )
                for row in clear_float_site_relocations
            )
            or tuple(clear_float_site_relocations) != semantic_relocations
            or len(semantic_local_definitions) != 1
            or len(exact_semantic_local_definitions) != 1
            or semantic_local_definitions[0]
            != exact_semantic_local_definitions[0]
            or semantic_local_definitions[0].name
            != next(iter(clear_float_symbol_names), "")
            or external_semantic_names
        ):
            raise ValueError(
                "HUD message UpdateSelectedWeaponDisplay bridge rejects "
                "compiler-local ClearSpecialToken constant semantic identity, "
                "private suffix, relocation, section/storage, or collision drift"
            )
        candidate_clear_float_symbol = next(iter(clear_float_symbol_names))

    expected_relocations = {
        0x0E: (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x4B14,
        ),
        0x14: (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x4B10,
        ),
        0x19: (
            IMAGE_REL_I386_REL32,
            _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECT_VARIANT_SYMBOL,
            0,
        ),
        0x23: (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_ZERO_FLOAT_SYMBOL,
            0,
        ),
        0x29: (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x4B10,
        ),
        0x2F: (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x4B14,
        ),
        0x40: (
            IMAGE_REL_I386_REL32,
            _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECT_VARIANT_SYMBOL,
            0,
        ),
        0x55: (
            IMAGE_REL_I386_REL32,
            _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECT_VARIANT_SYMBOL,
            0,
        ),
        0x6D: (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x4E9C,
        ),
        0x76: (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x4B18,
        ),
        0x82: (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CLEAR_FLOAT_SYMBOL,
            0,
        ),
        0x9A: (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CLEAR_TOKEN_SYMBOL,
            0,
        ),
        0xC3: (IMAGE_REL_I386_DIR32, "__imp__ceil", 0),
        0xCB: (IMAGE_REL_I386_REL32, "__ftol", 0),
        0xD1: (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_FORMAT_SYMBOL,
            0,
        ),
        0xEC: (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x4B10,
        ),
        0xF1: (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x4B14,
        ),
    }
    if candidate_variant == "corrected-retained-source":
        expected_relocations = {
            0x19: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0x4B10,
            ),
            0x1F: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0x4B14,
            ),
            0x32: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0x4B18,
            ),
            0x41: (
                IMAGE_REL_I386_REL32,
                _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SET_IMAGE_SYMBOL,
                0,
            ),
            0x6A: (
                IMAGE_REL_I386_REL32,
                _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SET_IMAGE_SYMBOL,
                0,
            ),
            0xA3: (
                IMAGE_REL_I386_REL32,
                _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SET_IMAGE_SYMBOL,
                0,
            ),
            0xC6: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CORRECTED_ZERO_FLOAT_SYMBOL,
                0,
            ),
            0xD0: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0x4B10,
            ),
            0xD6: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0x4B14,
            ),
            0xF9: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0x4B18,
            ),
            0x108: (
                IMAGE_REL_I386_REL32,
                _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SET_IMAGE_SYMBOL,
                0,
            ),
            0x12F: (
                IMAGE_REL_I386_REL32,
                _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SET_IMAGE_SYMBOL,
                0,
            ),
            0x16C: (
                IMAGE_REL_I386_REL32,
                _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SET_IMAGE_SYMBOL,
                0,
            ),
            0x194: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0x4F64,
            ),
            0x19B: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0x5020,
            ),
            0x1A1: (
                IMAGE_REL_I386_REL32,
                _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SET_IMAGE_SYMBOL,
                0,
            ),
            0x1B2: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0x5038,
            ),
            0x1B7: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0x503C,
            ),
            0x1BD: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0x52F4,
            ),
            0x1C2: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0x5034,
            ),
            0x1C7: (
                IMAGE_REL_I386_REL32,
                _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SET_IMAGE_SYMBOL,
                0,
            ),
            0x1CD: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0x52E8,
            ),
            0x1D8: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0x52E8,
            ),
            0x1E6: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0x5034,
            ),
            0x1EC: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0x5040,
            ),
            0x1F2: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0x52F4,
            ),
            0x1F8: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0x5038,
            ),
            0x1FD: (
                IMAGE_REL_I386_REL32,
                _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SET_IMAGE_SYMBOL,
                0,
            ),
            0x203: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0x52E8,
            ),
            0x20E: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0x52E8,
            ),
            0x227: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0x4E9C,
            ),
            0x22E: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0x4B18,
            ),
            0x23C: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CLEAR_FLOAT_SYMBOL,
                0,
            ),
            0x254: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CLEAR_TOKEN_SYMBOL,
                0,
            ),
            0x27F: (IMAGE_REL_I386_DIR32, "__imp__ceil", 0),
            0x287: (IMAGE_REL_I386_REL32, "__ftol", 0),
            0x28D: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_FORMAT_SYMBOL,
                0,
            ),
            0x2AB: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0x4B10,
            ),
            0x2B1: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                0x4B14,
            ),
        }
    elif candidate_variant == "retail-shaped-19call":
        aggregate_addends = {
            0x17: 0x4B10, 0x1D: 0x4B14, 0x30: 0x4B18,
            0xCE: 0x4B10, 0xD4: 0x4B14, 0xF5: 0x4B18,
            0x190: 0x4E9C, 0x199: 0x4B18,
            0x21E: 0x4F64, 0x225: 0x5020,
            0x23C: 0x503C, 0x242: 0x5038, 0x248: 0x5034,
            0x24E: 0x52F4, 0x259: 0x52E8, 0x264: 0x52E8,
            0x278: 0x5034, 0x27D: 0x5040, 0x283: 0x52F4,
            0x288: 0x5038, 0x293: 0x52E8, 0x29E: 0x52E8,
            0x2A4: 0x52E8, 0x2C1: 0x5044, 0x2CB: 0x5044,
            0x2F7: 0x5044, 0x301: 0x5044, 0x30A: 0x4F64,
            0x312: 0x4F64, 0x325: 0x4B10, 0x32B: 0x4B14,
        }
        expected_relocations = {
            **{
                offset: (
                    IMAGE_REL_I386_DIR32,
                    _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                    addend,
                )
                for offset, addend in aggregate_addends.items()
            },
            **{
                offset: (
                    IMAGE_REL_I386_REL32,
                    _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SET_IMAGE_SYMBOL,
                    0,
                )
                for offset in (
                    0x3F, 0x68, 0xA1, 0x104, 0x12B, 0x168,
                    0x22B, 0x253, 0x28D,
                )
            },
            0xC4: (
                IMAGE_REL_I386_DIR32,
                candidate_zero_float_symbol,
                0,
            ),
            0x1A9: (
                IMAGE_REL_I386_DIR32,
                candidate_clear_float_symbol,
                0,
            ),
            0x1C1: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CLEAR_TOKEN_SYMBOL,
                0,
            ),
            0x1EC: (IMAGE_REL_I386_DIR32, "__imp__ceil", 0),
            0x1F4: (IMAGE_REL_I386_REL32, "__ftol", 0),
            0x1FA: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_FORMAT_SYMBOL,
                0,
            ),
            0x2B4: (
                IMAGE_REL_I386_DIR32,
                candidate_clear_float_symbol,
                0,
            ),
            0x2C6: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CLEAR_TOKEN_SYMBOL,
                0,
            ),
            0x2E9: (IMAGE_REL_I386_DIR32, "__imp__ceil", 0),
            0x2F1: (IMAGE_REL_I386_REL32, "__ftol", 0),
            0x2FC: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_FORMAT_SYMBOL,
                0,
            ),
        }
    expected_mask = {
        position
        for offset in expected_relocations
        for position in range(offset, offset + 4)
    }
    if (
        len(caller.relocations) != len(expected_relocations)
        or {row.offset for row in caller.relocations}
        != set(expected_relocations)
        or any(
            (
                row.type,
                row.symbol_name,
                struct.unpack_from("<I", caller.data, row.offset)[0],
            )
            != expected_relocations[row.offset]
            for row in caller.relocations
        )
        or {
            index
            for index, masked in enumerate(caller.relocation_mask)
            if masked
        }
        != expected_mask
    ):
        observed_relocations = {
            row.offset: (
                row.type,
                row.symbol_name,
                struct.unpack_from("<I", caller.data, row.offset)[0],
            )
            for row in caller.relocations
        }
        relocation_drift = {
            hex(offset): {
                "expected": expected_relocations.get(offset),
                "candidate": observed_relocations.get(offset),
            }
            for offset in set(expected_relocations) | set(observed_relocations)
            if expected_relocations.get(offset)
            != observed_relocations.get(offset)
        }
        raise ValueError(
            "HUD message UpdateSelectedWeaponDisplay bridge rejects candidate "
            "COFF relocation identity/type/addend/mask drift: "
            f"rows={relocation_drift!r}"
        )
    direct_candidate_symbol = (
        _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECT_VARIANT_SYMBOL
        if candidate_variant == "legacy-select-variant"
        else _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SET_IMAGE_SYMBOL
    )
    direct_candidate_class = (
        caller.defined_external_functions
        if candidate_variant == "legacy-select-variant"
        else caller.undefined_external_functions
    )
    relevant_symbol_classes = {
        direct_candidate_symbol: (
            direct_candidate_class,
            (
                (
                    caller.undefined_external_functions
                    if candidate_variant == "legacy-select-variant"
                    else caller.defined_external_functions
                )
                + caller.undefined_external_data
                + caller.defined_external_data
            ),
        ),
        "__ftol": (
            caller.undefined_external_functions,
            (
                caller.defined_external_functions
                + caller.undefined_external_data
                + caller.defined_external_data
            ),
        ),
        "__imp__ceil": (
            caller.undefined_external_functions,
            (
                caller.defined_external_functions
                + caller.undefined_external_data
                + caller.defined_external_data
            ),
        ),
        _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL: (
            caller.undefined_external_data,
            (
                caller.undefined_external_functions
                + caller.defined_external_functions
                + caller.defined_external_data
            ),
        ),
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CLEAR_TOKEN_SYMBOL: (
            caller.undefined_external_data,
            (
                caller.undefined_external_functions
                + caller.defined_external_functions
                + caller.defined_external_data
            ),
        ),
        _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_NUMERIC_FORMAT_SYMBOL: (
            caller.defined_external_data,
            (
                caller.undefined_external_functions
                + caller.defined_external_functions
                + caller.undefined_external_data
            ),
        ),
    }
    if candidate_variant != "legacy-select-variant":
        relevant_symbol_classes[
            _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECT_VARIANT_SYMBOL
        ] = (
            caller.defined_external_functions,
            (
                caller.undefined_external_functions
                + caller.undefined_external_data
                + caller.defined_external_data
            ),
        )
    if any(
        expected_class.count(name) != 1
        or competing_classes.count(name) != 0
        for name, (
            expected_class,
            competing_classes,
        ) in relevant_symbol_classes.items()
    ):
        raise ValueError(
            "HUD message UpdateSelectedWeaponDisplay bridge rejects candidate "
            "COFF external function/data symbol-class provenance drift"
        )
    local_static_by_name = {
        row.name: row for row in caller.local_static_definitions
    }
    zero_float_symbol = (
        _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_ZERO_FLOAT_SYMBOL
        if candidate_variant == "legacy-select-variant"
        else (
            _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CORRECTED_ZERO_FLOAT_SYMBOL
            if candidate_variant == "corrected-retained-source"
            else candidate_zero_float_symbol
        )
    )
    expected_local_statics = {
        zero_float_symbol: b"\x00\x00\x00\x00",
        candidate_clear_float_symbol: b"\xa3\x79\xeb\x4c",
    }
    if (
        len(caller.local_static_definitions) != len(expected_local_statics)
        or len(local_static_by_name) != len(expected_local_statics)
        or set(local_static_by_name) != set(expected_local_statics)
        or any(
            not _cc_candidate._candidate_local_read_only_scalar_matches(row, expected)
            for name, expected in expected_local_statics.items()
            for row in (local_static_by_name.get(name),)
            if row is not None
        )
    ):
        raise ValueError(
            "HUD message UpdateSelectedWeaponDisplay bridge rejects candidate "
            "COFF local-static identity/section/class/data provenance drift"
        )

    retail_bridges = {
        normalize_address("0x4129ee"): ReviewedLoopVptrStorageBridge(
            register="ecx",
            storage_identity=(
                _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECTED_PANEL_STORAGE_IDENTITY
            ),
            slot_displacement=0x74,
            assembly_source="bn",
        ),
        normalize_address("0x412a27"): ReviewedLoopVptrStorageBridge(
            register="edx",
            storage_identity=(
                _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECTED_PANEL_STORAGE_IDENTITY
            ),
            slot_displacement=0x74,
            assembly_source="bn",
        ),
        normalize_address("0x412a31"): ReviewedLoopVptrStorageBridge(
            register="eax",
            storage_identity=(
                _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECTED_MESSAGE_VPTR_IDENTITY
            ),
            slot_displacement=0x20,
            assembly_source="bn",
        ),
        normalize_address("0x412af7"): ReviewedLoopVptrStorageBridge(
            register="eax",
            storage_identity=(
                _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_FIXED_PANEL_STORAGE_IDENTITY
            ),
            slot_displacement=0x74,
            assembly_source="bn",
        ),
        normalize_address("0x412b2e"): ReviewedLoopVptrStorageBridge(
            register="ecx",
            storage_identity=(
                _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_FIXED_PANEL_STORAGE_IDENTITY
            ),
            slot_displacement=0x74,
            assembly_source="bn",
        ),
        normalize_address("0x412b3f"): ReviewedLoopVptrStorageBridge(
            register="edx",
            storage_identity=(
                _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_FIXED_MESSAGE_VPTR_IDENTITY
            ),
            slot_displacement=0x20,
            assembly_source="bn",
        ),
    }
    if candidate_variant == "legacy-select-variant":
        candidate_bridges = {
            normalize_address("0x9f"): ReviewedLoopVptrStorageBridge(
                register="edx",
                storage_identity=(
                    _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECTED_PANEL_STORAGE_IDENTITY
                ),
                slot_displacement=0x74,
                assembly_source="cod",
            ),
            normalize_address("0xd6"): ReviewedLoopVptrStorageBridge(
                register="ebx",
                storage_identity=(
                    _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECTED_PANEL_STORAGE_IDENTITY
                ),
                slot_displacement=0x74,
                assembly_source="cod",
            ),
            normalize_address("0xe0"): ReviewedLoopVptrStorageBridge(
                register="eax",
                storage_identity=(
                    _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECTED_MESSAGE_VPTR_IDENTITY
                ),
                slot_displacement=0x20,
                assembly_source="cod",
            ),
        }
    elif candidate_variant == "corrected-retained-source":
        candidate_bridges = {
            normalize_address("0x259"): ReviewedLoopVptrStorageBridge(
                register="ecx",
                storage_identity=(
                    _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECTED_PANEL_STORAGE_IDENTITY
                ),
                slot_displacement=0x74,
                assembly_source="cod",
            ),
            normalize_address("0x292"): ReviewedLoopVptrStorageBridge(
                register="ebx",
                storage_identity=(
                    _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECTED_PANEL_STORAGE_IDENTITY
                ),
                slot_displacement=0x74,
                assembly_source="cod",
            ),
            normalize_address("0x29c"): ReviewedLoopVptrStorageBridge(
                register="edx",
                storage_identity=(
                    _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECTED_MESSAGE_VPTR_IDENTITY
                ),
                slot_displacement=0x20,
                assembly_source="cod",
            ),
        }
    else:
        candidate_bridges = {
            normalize_address("0x1c6"): ReviewedLoopVptrStorageBridge(
                register="ecx",
                storage_identity=_cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECTED_PANEL_STORAGE_IDENTITY,
                slot_displacement=0x74,
                assembly_source="cod",
            ),
            normalize_address("0x1ff"): ReviewedLoopVptrStorageBridge(
                register="ebx",
                storage_identity=_cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECTED_PANEL_STORAGE_IDENTITY,
                slot_displacement=0x74,
                assembly_source="cod",
            ),
            normalize_address("0x209"): ReviewedLoopVptrStorageBridge(
                register="edx",
                storage_identity=_cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECTED_MESSAGE_VPTR_IDENTITY,
                slot_displacement=0x20,
                assembly_source="cod",
            ),
            normalize_address("0x2cf"): ReviewedLoopVptrStorageBridge(
                register="edx",
                storage_identity=_cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_FIXED_PANEL_STORAGE_IDENTITY,
                slot_displacement=0x74,
                assembly_source="cod",
            ),
            normalize_address("0x305"): ReviewedLoopVptrStorageBridge(
                register="eax",
                storage_identity=_cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_FIXED_PANEL_STORAGE_IDENTITY,
                slot_displacement=0x74,
                assembly_source="cod",
            ),
            normalize_address("0x316"): ReviewedLoopVptrStorageBridge(
                register="edx",
                storage_identity=_cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_FIXED_MESSAGE_VPTR_IDENTITY,
                slot_displacement=0x20,
                assembly_source="cod",
            ),
        }

    retail_contract = _cc_extraction.extract_invocation_contract(
        retail_instructions,
        source="bn",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=derived_indexes,
        bridge_names=bridge_names,
        reviewed_loop_vptr_storage_bridges=retail_bridges,
    )
    candidate_contract = _cc_extraction.extract_invocation_contract(
        candidate.instructions,
        source="cod",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=derived_indexes,
        bridge_names=bridge_names,
        compiler_generated_bridges={"__ftol": ftol_identity},
        reviewed_loop_vptr_storage_bridges=candidate_bridges,
        local_control_flow_indices=candidate.local_control_flow_indices,
        local_control_flow_targets=candidate.local_control_flow_targets,
    )

    def direct_contract(ordinal: int, identity: str) -> dict[str, Any]:
        return {
            "ordinal": ordinal,
            "form": "call",
            "dispatch": "direct",
            "identity_kind": (
                "provider"
                if identity in derived_indexes.provider_ids
                else "direct"
            ),
            "target_identity": identity,
            "storage_identity": "",
            "slot_displacement": None,
            "cleanup_bytes": None,
        }

    def virtual_contract(
        ordinal: int,
        storage_identity: str,
        slot: int,
        cleanup: int | None,
    ) -> dict[str, Any]:
        return {
            "ordinal": ordinal,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": storage_identity,
            "slot_displacement": slot,
            "cleanup_bytes": cleanup,
        }

    def ceil_contract(ordinal: int) -> dict[str, Any]:
        return {
            "ordinal": ordinal,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "iat",
            "target_identity": _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CEIL_IAT_IDENTITY,
            "storage_identity": _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_CEIL_IAT_IDENTITY,
            "slot_displacement": None,
            "cleanup_bytes": 8,
        }

    expected_retail_contract = [
        *[
            direct_contract(ordinal, set_image_identity)
            for ordinal in range(6)
        ],
        virtual_contract(
            6,
            _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECTED_PANEL_STORAGE_IDENTITY,
            0x74,
            8,
        ),
        ceil_contract(7),
        direct_contract(8, ftol_identity),
        virtual_contract(
            9,
            _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECTED_PANEL_STORAGE_IDENTITY,
            0x74,
            12,
        ),
        virtual_contract(
            10,
            _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECTED_MESSAGE_VPTR_IDENTITY,
            0x20,
            None,
        ),
        *[
            direct_contract(ordinal, set_image_identity)
            for ordinal in range(11, 14)
        ],
        virtual_contract(
            14,
            _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_FIXED_PANEL_STORAGE_IDENTITY,
            0x74,
            8,
        ),
        ceil_contract(15),
        direct_contract(16, ftol_identity),
        virtual_contract(
            17,
            _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_FIXED_PANEL_STORAGE_IDENTITY,
            0x74,
            12,
        ),
        virtual_contract(
            18,
            _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_FIXED_MESSAGE_VPTR_IDENTITY,
            0x20,
            None,
        ),
    ]
    if candidate_variant == "legacy-select-variant":
        expected_candidate_contract = [
            *[
                direct_contract(ordinal, select_variant_identity)
                for ordinal in range(3)
            ],
            virtual_contract(
                3,
                _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECTED_PANEL_STORAGE_IDENTITY,
                0x74,
                8,
            ),
            ceil_contract(4),
            direct_contract(5, ftol_identity),
            virtual_contract(
                6,
                _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECTED_PANEL_STORAGE_IDENTITY,
                0x74,
                12,
            ),
            virtual_contract(
                7,
                _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECTED_MESSAGE_VPTR_IDENTITY,
                0x20,
                None,
            ),
        ]
    elif candidate_variant == "corrected-retained-source":
        expected_candidate_contract = [
            *[
                direct_contract(ordinal, set_image_identity)
                for ordinal in range(9)
            ],
            virtual_contract(
                9,
                _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECTED_PANEL_STORAGE_IDENTITY,
                0x74,
                8,
            ),
            ceil_contract(10),
            direct_contract(11, ftol_identity),
            virtual_contract(
                12,
                _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECTED_PANEL_STORAGE_IDENTITY,
                0x74,
                12,
            ),
            virtual_contract(
                13,
                _cc_catalog.HUD_UI_MESSAGE_UPDATE_WEAPON_SELECTED_MESSAGE_VPTR_IDENTITY,
                0x20,
                None,
            ),
        ]
    else:
        expected_candidate_contract = expected_retail_contract
    if (
        retail_contract != expected_retail_contract
        or candidate_contract != expected_candidate_contract
    ):
        retail_derivation = _cc_comparison.compare_call_contracts(
            expected_retail_contract,
            retail_contract,
        )
        candidate_derivation = _cc_comparison.compare_call_contracts(
            expected_candidate_contract,
            candidate_contract,
        )
        raise ValueError(
            "HUD message UpdateSelectedWeaponDisplay bridge cannot "
            "independently derive the exact retail 19-call and candidate "
            f"{len(expected_candidate_contract)}-call contracts: "
            f"retail={retail_derivation['first_divergence']!r}; "
            f"candidate={candidate_derivation['first_divergence']!r}"
        )
    population_comparison = _cc_comparison.compare_call_contracts(
        retail_contract,
        candidate_contract,
    )
    expected_population_divergence_ordinal = (
        0 if candidate_variant == "legacy-select-variant" else 6
    )
    population_shape_valid = (
        population_comparison["passed"]
        if candidate_variant == "retail-shaped-19call"
        else (
            not population_comparison["passed"]
            and population_comparison["first_divergence"].get("ordinal")
            == expected_population_divergence_ordinal
            and population_comparison["first_divergence"].get("kind")
            == "mismatch"
        )
    )
    if not population_shape_valid:
        raise ValueError(
            "HUD message UpdateSelectedWeaponDisplay bridge must preserve the "
            "concrete retail/candidate call-population divergence"
        )
    return derived_indexes, retail_bridges, candidate_bridges
