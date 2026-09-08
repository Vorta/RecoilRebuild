"""Recoil call-contract recoil hud visibility evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import extraction as _cc_extraction
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract import receiver_instructions as _cc_receiver_instructions
from _recoil.call_contract import receiver_candidate as _cc_receiver_candidate
from _recoil.call_contract import recoil_hud_layout as _cc_recoil_hud_layout
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        IdentityIndexes,
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedMemberVptrStorageBridge,
        ReviewedStaticStorageReferenceBridge,
        ReviewedVptrStorageBridge,
    )

import re
import struct
from pathlib import Path
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_DIR32,
    IMAGE_REL_I386_REL32,
    Instruction,
)
from _recoil.lib.authored_icf import (
    exact_required_target_membership,
    exact_selected_target_membership,
)
from _recoil.lib.progress import ProgressDocument, address_value, normalize_address


def _hud_ui_mgr_selected_progress_set_visible_register_storage_bridges(
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[dict[str, str], dict[str, str]]:
    """Prove UpdateSelectedProgressMeter's first sensor-meter SetVisible."""
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALLER_START:
        return {}, {}

    caller_symbol_id = (
        _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALLER_IDENTITY.removeprefix(
            "symbol:"
        )
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
    definition = candidate.caller_definition
    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALLER_START
    ]
    if (
        caller_identity
        != _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(normalized_start) != caller_identity
        or caller_identity in indexes.provider_ids
        or not isinstance(caller_row, Mapping)
        or caller_row.get("binary") != "recoil"
        or caller_row.get("kind") != "function"
        or caller_row.get("pipeline_class") != "authored"
        or caller_row.get("ownership_state") != "primary-owned"
        or caller_row.get("address") != normalized_start
        or caller_row.get("end_exclusive")
        != _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALLER_END_EXCLUSIVE
        or caller_row.get("extent_state") != "known"
        or caller_row.get("size") != 0x170
        or caller_row.get("navigation_name")
        != "HudUiMgrTarget::UpdateSelectedProgressMeter"
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
        != _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
        or definition is None
        or definition.symbol
        != _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALLER_SYMBOL
        or not definition.data
        or len(definition.data) != len(definition.relocation_mask)
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
        or len(contribution_rows) != 1
    ):
        raise ValueError(
            "HUD selected-progress SetVisible bridge requires the exact "
            "authored caller, source edge, symbol, extent, and candidate "
            "contribution authority"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != r"\?UpdateSelectedProgressMeter@HudUiMgrTarget@@.*"
        or re.fullmatch(
            str(getattr(contribution_row, "symbol_regex", "")),
            definition.symbol,
        )
        is None
        or getattr(contribution_row, "name", "")
        != "HudUiMgrTarget::UpdateSelectedProgressMeter"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD selected-progress SetVisible bridge requires the exact "
            "authored hud.cpp contribution row"
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
    aggregate_row = symbols.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID)
    field_address = normalize_address(
        aggregate_start + _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_DISPLACEMENT
    )
    exact_containers = [
        row
        for row in indexes.storage_containers
        if (
            row.identity == aggregate_identity
            and row.start == aggregate_start
            and row.end_exclusive == aggregate_start + _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
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
        or aggregate_storage.get("output_section_id") != "recoil:section:.data"
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
        or aggregate_target.get("symbol_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID]
        or aggregate_target.get("unresolved_addresses") not in (None, [])
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
        or field_address
        != _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_RETAIL_RECEIVER_ADDRESS
        or normalize_address(
            aggregate_start
            + _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_TRACKED_SLOT_DISPLACEMENT
        )
        != _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_TRACKED_SLOT_ADDRESS
        or indexes.storage_by_address.get(field_address, "") != ""
        or (
            _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_DISPLACEMENT + 4
            > _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
        )
        or (
            _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_TRACKED_SLOT_DISPLACEMENT + 4
            > _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
        )
    ):
        raise ValueError(
            "HUD selected-progress SetVisible bridge requires the exact "
            "reviewed aggregate symbol, storage, target, owner, and bounded "
            "uncatalogued +0xd9c sensor-meter authority"
        )

    field_identity = _cc_receiver_instructions._abstract_with_displacement(
        aggregate_identity,
        _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_DISPLACEMENT,
    )
    expected_storage = f"load({field_identity})"

    def instruction_bytes(instruction: Instruction) -> bytes:
        try:
            return bytes(int(value, 16) for value in instruction.bytes)
        except (TypeError, ValueError) as exc:
            raise ValueError(
                "HUD selected-progress SetVisible bridge requires exact "
                "instruction bytes"
            ) from exc

    def split_operands(instruction: Instruction) -> tuple[str, str]:
        parts = [
            item.strip().lower()
            for item in _cc_cfg._instruction_operand(instruction).split(",", 1)
        ]
        if len(parts) != 2:
            return "", ""
        return parts[0], _cc_targets._exact_memory_expression(parts[1])

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
        _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_RETAIL_VPTR: (
            "mov",
            b"\x8b\x15\x6c\x6c\x4e\x00",
        ),
        _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_RETAIL_FALSE: (
            "push",
            b"\x6a\x00",
        ),
        _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_RETAIL_RECEIVER: (
            "mov",
            b"\xb9\x6c\x6c\x4e\x00",
        ),
        _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_RETAIL_CALL: (
            "call",
            b"\xff\x52\x60",
        ),
        _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_RETAIL_SUCCESSOR: (
            "mov",
            b"\x8b\x15\xf0\x6a\x4e\x00",
        ),
    }
    if any(
        address not in retail_by_address
        or _cc_cfg._instruction_mnemonic(retail_by_address[address]) != mnemonic
        or instruction_bytes(retail_by_address[address]) != body
        for address, (mnemonic, body) in retail_fixed.items()
    ):
        raise ValueError(
            "HUD selected-progress SetVisible bridge requires the exact "
            "retail vptr/false/receiver/slot/successor unit"
        )
    retail_indices = [
        retail_index_by_address[address] for address in retail_fixed
    ]
    retail_vptr = retail_by_address[
        _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_RETAIL_VPTR
    ]
    retail_false = retail_by_address[
        _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_RETAIL_FALSE
    ]
    retail_receiver = retail_by_address[
        _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_RETAIL_RECEIVER
    ]
    retail_call = retail_by_address[
        _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_RETAIL_CALL
    ]
    retail_successor = retail_by_address[
        _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_RETAIL_SUCCESSOR
    ]
    retail_call_index = retail_indices[3]
    if (
        retail_indices
        != list(range(retail_indices[0], retail_indices[0] + 5))
        or split_operands(retail_vptr)
        != ("edx", _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_RETAIL_RECEIVER_ADDRESS)
        or _cc_cfg._instruction_operand(retail_false).strip().lower()
        not in {"0", "0x0"}
        or split_operands(retail_receiver)
        != ("ecx", _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_RETAIL_RECEIVER_ADDRESS)
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(retail_call))
        not in {"edx+96", "edx+0x60"}
        or split_operands(retail_successor)
        != ("edx", _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_TRACKED_SLOT_ADDRESS)
        or sum(
            _cc_cfg._instruction_mnemonic(instruction) == "call"
            for instruction in retail_instructions[:retail_call_index]
        )
        != _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_RETAIL_ORDINAL
        or _cc_cfg._cleanup_after(retail_instructions, retail_call_index) is not None
    ):
        raise ValueError(
            "HUD selected-progress SetVisible bridge rejects retail identity, "
            "register, argument, ordinal, slot, cleanup, or successor drift"
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
        _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_CANDIDATE_VPTR: (
            "mov",
            b"\x8b\x15\x9c\x0d\x00\x00",
        ),
        _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_CANDIDATE_FALSE: (
            "push",
            b"\x6a\x00",
        ),
        _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_CANDIDATE_RECEIVER: (
            "mov",
            b"\xb9\x9c\x0d\x00\x00",
        ),
        _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_CANDIDATE_CALL: (
            "call",
            b"\xff\x52\x60",
        ),
        _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_CANDIDATE_SUCCESSOR: (
            "mov",
            b"\x8b\x15\x20\x0c\x00\x00",
        ),
    }
    if any(
        offset not in candidate_by_offset
        or _cc_cfg._instruction_mnemonic(candidate_by_offset[offset]) != mnemonic
        or instruction_bytes(candidate_by_offset[offset]) != body
        for offset, (mnemonic, body) in candidate_fixed.items()
    ):
        raise ValueError(
            "HUD selected-progress SetVisible bridge requires the exact "
            "candidate +0xa0 vptr/false/receiver/slot/successor unit"
        )
    candidate_indices = [
        candidate_index_by_offset[offset] for offset in candidate_fixed
    ]
    candidate_vptr = candidate_by_offset[
        _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_CANDIDATE_VPTR
    ]
    candidate_false = candidate_by_offset[
        _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_CANDIDATE_FALSE
    ]
    candidate_receiver = candidate_by_offset[
        _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_CANDIDATE_RECEIVER
    ]
    candidate_call = candidate_by_offset[
        _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_CANDIDATE_CALL
    ]
    candidate_successor = candidate_by_offset[
        _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_CANDIDATE_SUCCESSOR
    ]
    expected_expressions = {
        (
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
            f"+{_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_DISPLACEMENT}"
        ).lower(),
        (
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
            f"+0x{_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_DISPLACEMENT:x}"
        ).lower(),
    }
    candidate_vptr_expression = split_operands(candidate_vptr)[1]
    candidate_receiver_expression = split_operands(candidate_receiver)[
        1
    ].removeprefix("offsetflat:")
    candidate_successor_expression = split_operands(candidate_successor)[1]
    tracked_slot_expressions = {
        (
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
            f"+{_cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_TRACKED_SLOT_DISPLACEMENT}"
        ).lower(),
        (
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
            f"+0x{_cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_TRACKED_SLOT_DISPLACEMENT:x}"
        ).lower(),
    }
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    ordinal_by_index = {
        index: ordinal for ordinal, index in enumerate(invocation_indices)
    }
    candidate_call_index = candidate_indices[3]
    bounded_indices = set(
        range(candidate_indices[0], candidate_indices[-1] + 1)
    )
    if (
        candidate_indices
        != list(range(candidate_indices[0], candidate_indices[0] + 5))
        or split_operands(candidate_vptr)[0] != "edx"
        or candidate_vptr_expression not in expected_expressions
        or _cc_cfg._instruction_operand(candidate_false).strip().lower()
        not in {"0", "0x0"}
        or split_operands(candidate_receiver)[0] != "ecx"
        or candidate_receiver_expression != candidate_vptr_expression
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(candidate_call))
        not in {"edx+96", "edx+0x60"}
        or split_operands(candidate_successor)[0] != "edx"
        or candidate_successor_expression not in tracked_slot_expressions
        or ordinal_by_index.get(candidate_call_index)
        != _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_RETAIL_ORDINAL
        or _cc_cfg._cleanup_after(candidate.instructions, candidate_call_index)
        is not None
        or candidate.local_control_flow_indices & bounded_indices
        or any(
            target_index in bounded_indices
            for target_indices in candidate.local_control_flow_targets.values()
            for target_index in target_indices
        )
    ):
        raise ValueError(
            "HUD selected-progress SetVisible bridge rejects candidate "
            "identity, reaching definition, register, argument, ordinal, "
            "slot, cleanup, successor, or topology drift"
        )

    unit_start = _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_CANDIDATE_VPTR
    unit_end = (
        _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_CANDIDATE_SUCCESSOR + 6
    )
    expected_relocations = {
        _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_CANDIDATE_VPTR + 2:
            _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_DISPLACEMENT,
        _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_CANDIDATE_RECEIVER + 1:
            _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_DISPLACEMENT,
        _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_CANDIDATE_SUCCESSOR + 2:
            _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_TRACKED_SLOT_DISPLACEMENT,
    }
    bounded_relocations = [
        relocation
        for relocation in definition.relocations
        if unit_start <= relocation.offset < unit_end
    ]
    expected_mask = {
        index
        for offset in expected_relocations
        for index in range(offset, offset + 4)
    }
    if (
        definition.data[unit_start:unit_end]
        != b"".join(body for _mnemonic, body in candidate_fixed.values())
        or {row.offset for row in bounded_relocations}
        != set(expected_relocations)
        or len(bounded_relocations) != len(expected_relocations)
        or any(
            row.type != IMAGE_REL_I386_DIR32
            or row.symbol_name != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
            or struct.unpack_from("<I", definition.data, row.offset)[0]
            != expected_relocations.get(row.offset)
            for row in bounded_relocations
        )
        or {
            index
            for index in range(unit_start, unit_end)
            if definition.relocation_mask[index]
        }
        != expected_mask
        or definition.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in definition.defined_external_data
    ):
        raise ValueError(
            "HUD selected-progress SetVisible bridge rejects missing, extra, "
            "malformed, aliased, wrong-target, wrong-addend, or wrong-mask "
            "candidate +0xd9c/+0xc20 DIR32 relocations"
        )

    retail_bridges = {
        _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_RETAIL_RECEIVER_ADDRESS:
            expected_storage,
    }
    candidate_bridges = {
        _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL: aggregate_identity,
    }
    required_row = {
        "ordinal": _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_RETAIL_ORDINAL,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": expected_storage,
        "slot_displacement": _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_SLOT,
        "cleanup_bytes": None,
    }
    retail_contract = _cc_extraction.extract_invocation_contract(
        tuple(retail_by_address[address] for address in retail_fixed),
        source="bn",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        reviewed_register_storage_bridges=retail_bridges,
    )
    candidate_contract = _cc_extraction.extract_invocation_contract(
        tuple(candidate_by_offset[offset] for offset in candidate_fixed),
        source="cod",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        reviewed_register_storage_bridges=candidate_bridges,
    )
    if retail_contract != [required_row] or candidate_contract != [required_row]:
        raise ValueError(
            "HUD selected-progress SetVisible bridge cannot derive the exact "
            "retail/candidate sensor-meter storage identity and virtual call"
        )
    return retail_bridges, candidate_bridges


def _hud_ui_mgr_hide_tracked_progress_set_visible_register_storage_bridges(
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[dict[str, str], dict[str, str]]:
    """Prove HideTrackedProgressMeterIfOwnerMatches' guarded SetVisible."""
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_HIDE_TRACKED_PROGRESS_CALLER_START:
        return {}, {}

    caller_symbol_id = (
        _cc_catalog.HUD_UI_MGR_HIDE_TRACKED_PROGRESS_CALLER_IDENTITY.removeprefix(
            "symbol:"
        )
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
    definition = candidate.caller_definition
    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_HIDE_TRACKED_PROGRESS_CALLER_START
    ]
    if (
        caller_identity != _cc_catalog.HUD_UI_MGR_HIDE_TRACKED_PROGRESS_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_HIDE_TRACKED_PROGRESS_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(normalized_start) != caller_identity
        or caller_identity in indexes.provider_ids
        or not isinstance(caller_row, Mapping)
        or caller_row.get("binary") != "recoil"
        or caller_row.get("kind") != "function"
        or caller_row.get("pipeline_class") != "authored"
        or caller_row.get("ownership_state") != "primary-owned"
        or caller_row.get("address") != normalized_start
        or caller_row.get("end_exclusive")
        != _cc_catalog.HUD_UI_MGR_HIDE_TRACKED_PROGRESS_CALLER_END_EXCLUSIVE
        or caller_row.get("extent_state") != "known"
        or caller_row.get("size") != 0x30
        or caller_row.get("navigation_name")
        != "HudUiMgr::HideTrackedProgressMeterIfOwnerMatches"
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
        != _cc_catalog.HUD_UI_MGR_HIDE_TRACKED_PROGRESS_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
        or definition is None
        or definition.symbol != _cc_catalog.HUD_UI_MGR_HIDE_TRACKED_PROGRESS_CALLER_SYMBOL
        or not definition.data
        or len(definition.data) != len(definition.relocation_mask)
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
            "HUD hide-tracked-progress SetVisible bridge requires the exact "
            "authored caller, source edge, symbol, tracker interval, and "
            "candidate contribution authority"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != r"\?HideTrackedProgressMeterIfOwnerMatches@HudUiMgr@@.*"
        or re.fullmatch(
            str(getattr(contribution_row, "symbol_regex", "")),
            definition.symbol,
        )
        is None
        or getattr(contribution_row, "name", "")
        != "HudUiMgr::HideTrackedProgressMeterIfOwnerMatches"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD hide-tracked-progress SetVisible bridge requires the exact "
            "authored hud.cpp contribution row"
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
    meter_address = normalize_address(
        aggregate_start + _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_DISPLACEMENT
    )
    tracked_slot_address = normalize_address(
        aggregate_start
        + _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_TRACKED_SLOT_DISPLACEMENT
    )
    exact_containers = [
        row
        for row in indexes.storage_containers
        if (
            row.identity == aggregate_identity
            and row.start == aggregate_start
            and row.end_exclusive == aggregate_start + _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
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
        or aggregate_storage.get("output_section_id") != "recoil:section:.data"
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
        or aggregate_target.get("symbol_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID]
        or aggregate_target.get("unresolved_addresses") not in (None, [])
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
        or meter_address
        != _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_RETAIL_RECEIVER_ADDRESS
        or tracked_slot_address
        != _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_TRACKED_SLOT_ADDRESS
        or indexes.storage_by_address.get(meter_address, "") != ""
        or indexes.storage_by_address.get(tracked_slot_address, "") != ""
        or (
            _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_DISPLACEMENT + 4
            > _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
        )
        or (
            _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_TRACKED_SLOT_DISPLACEMENT + 4
            > _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
        )
    ):
        raise ValueError(
            "HUD hide-tracked-progress SetVisible bridge requires the exact "
            "reviewed aggregate symbol, storage, target, owner, and bounded "
            "uncatalogued +0xc20/+0xd9c field authority"
        )

    meter_field_identity = _cc_receiver_instructions._abstract_with_displacement(
        aggregate_identity,
        _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_DISPLACEMENT,
    )
    expected_storage = f"load({meter_field_identity})"

    def instruction_bytes(instruction: Instruction) -> bytes:
        try:
            return bytes(int(value, 16) for value in instruction.bytes)
        except (TypeError, ValueError) as exc:
            raise ValueError(
                "HUD hide-tracked-progress SetVisible bridge requires exact "
                "instruction bytes"
            ) from exc

    def split_operands(instruction: Instruction) -> tuple[str, str]:
        parts = [
            item.strip().lower()
            for item in _cc_cfg._instruction_operand(instruction).split(",", 1)
        ]
        if len(parts) != 2:
            return "", ""
        return parts[0], _cc_targets._exact_memory_expression(parts[1])

    retail_addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(normalized_start),
    )
    retail_offsets = tuple(
        address - address_value(normalized_start)
        if address is not None
        else None
        for address in retail_addresses
    )
    retail_fixed = (
        (0x00, "mov", b"\xa1\xf0\x6a\x4e\x00"),
        (0x05, "test", b"\x85\xc0"),
        (0x07, "je", b"\x74\x18"),
        (0x09, "mov", b"\x8b\x40\x38"),
        (0x0C, "cmp", b"\x39\x48\x04"),
        (0x0F, "jne", b"\x75\x10"),
        (0x11, "mov", b"\x8b\x15\x6c\x6c\x4e\x00"),
        (0x17, "push", b"\x6a\x00"),
        (0x19, "mov", b"\xb9\x6c\x6c\x4e\x00"),
        (0x1E, "call", b"\xff\x52\x60"),
        (0x21, "retn", b"\xc3"),
    )
    retail_expected_offsets = tuple(row[0] for row in retail_fixed)
    if (
        retail_offsets != retail_expected_offsets
        or len(retail_instructions) != len(retail_fixed)
        or any(
            _cc_cfg._instruction_mnemonic(instruction) != mnemonic
            or instruction_bytes(instruction) != body
            for instruction, (_offset, mnemonic, body) in zip(
                retail_instructions,
                retail_fixed,
            )
        )
        or (
            address_value(normalized_start)
            + retail_expected_offsets[-1]
            + len(retail_fixed[-1][2])
        )
        != _cc_catalog.HUD_UI_MGR_HIDE_TRACKED_PROGRESS_RETAIL_BODY_END_EXCLUSIVE
    ):
        raise ValueError(
            "HUD hide-tracked-progress SetVisible bridge requires the exact "
            "retail 0x412620..0x412642 body bytes and instruction offsets; "
            f"actual_offsets={retail_offsets!r}, "
            "actual_units="
            f"{tuple((_cc_cfg._instruction_mnemonic(row), instruction_bytes(row)) for row in retail_instructions)!r}"
        )
    (
        retail_tracked_load,
        retail_test,
        retail_null_branch,
        retail_track_node_load,
        retail_owner_compare,
        retail_owner_branch,
        retail_vptr,
        retail_false,
        retail_receiver,
        retail_call,
        retail_return,
    ) = retail_instructions
    if (
        split_operands(retail_tracked_load)
        != ("eax", _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_TRACKED_SLOT_ADDRESS)
        or split_operands(retail_test) != ("eax", "eax")
        or _cc_cfg._instruction_operand(retail_null_branch).strip().lower()
        != "0x412641"
        or split_operands(retail_track_node_load)
        not in {("eax", "eax+56"), ("eax", "eax+0x38")}
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(retail_owner_compare).split(",", 1)[0]
        )
        not in {"eax+4", "eax+0x4"}
        or split_operands(retail_owner_compare)[1] != "ecx"
        or _cc_cfg._instruction_operand(retail_owner_branch).strip().lower()
        != "0x412641"
        or split_operands(retail_vptr)
        != ("edx", _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_RETAIL_RECEIVER_ADDRESS)
        or _cc_cfg._instruction_operand(retail_false).strip().lower()
        not in {"0", "0x0"}
        or split_operands(retail_receiver)
        != ("ecx", _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_RETAIL_RECEIVER_ADDRESS)
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(retail_call))
        not in {"edx+96", "edx+0x60"}
        or _cc_cfg._instruction_operand(retail_return).strip() not in {"", "0"}
        or sum(
            _cc_cfg._instruction_mnemonic(instruction) == "call"
            for instruction in retail_instructions
        )
        != 1
        or _cc_cfg._cleanup_after(retail_instructions, 9) is not None
    ):
        raise ValueError(
            "HUD hide-tracked-progress SetVisible bridge rejects retail "
            "storage, reaching definition, owner guard, receiver/vptr, "
            "argument, ordinal, slot, cleanup, or return-topology drift"
        )

    candidate_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    candidate_fixed = (
        (0x00, "mov", b"\xa1\x20\x0c\x00\x00"),
        (0x05, "test", b"\x85\xc0"),
        (0x07, "je", b"\x74\x18"),
        (0x09, "mov", b"\x8b\x40\x38"),
        (0x0C, "cmp", b"\x39\x48\x04"),
        (0x0F, "jne", b"\x75\x10"),
        (0x11, "mov", b"\x8b\x15\x9c\x0d\x00\x00"),
        (0x17, "push", b"\x6a\x00"),
        (0x19, "mov", b"\xb9\x9c\x0d\x00\x00"),
        (0x1E, "call", b"\xff\x52\x60"),
        (0x21, "ret", b"\xc3"),
    )
    candidate_expected_offsets = tuple(row[0] for row in candidate_fixed)
    if (
        candidate_offsets != candidate_expected_offsets
        or len(candidate.instructions) != len(candidate_fixed)
        or any(
            _cc_cfg._instruction_mnemonic(instruction) != mnemonic
            or instruction_bytes(instruction) != body
            for instruction, (_offset, mnemonic, body) in zip(
                candidate.instructions,
                candidate_fixed,
            )
        )
    ):
        raise ValueError(
            "HUD hide-tracked-progress SetVisible bridge requires the exact "
            "candidate +0x0..+0x22 body bytes and instruction offsets"
        )
    (
        candidate_tracked_load,
        candidate_test,
        _candidate_null_branch,
        candidate_track_node_load,
        candidate_owner_compare,
        _candidate_owner_branch,
        candidate_vptr,
        candidate_false,
        candidate_receiver,
        candidate_call,
        candidate_return,
    ) = candidate.instructions
    tracked_expressions = {
        (
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
            f"+{_cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_TRACKED_SLOT_DISPLACEMENT}"
        ).lower(),
        (
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
            f"+0x{_cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_TRACKED_SLOT_DISPLACEMENT:x}"
        ).lower(),
    }
    meter_expressions = {
        (
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
            f"+{_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_DISPLACEMENT}"
        ).lower(),
        (
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
            f"+0x{_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_DISPLACEMENT:x}"
        ).lower(),
    }
    candidate_vptr_expression = split_operands(candidate_vptr)[1]
    candidate_receiver_expression = split_operands(candidate_receiver)[
        1
    ].removeprefix("offsetflat:")

    def short_branch_target(index: int) -> int | None:
        encoded = instruction_bytes(candidate.instructions[index])
        if len(encoded) != 2:
            return None
        displacement = struct.unpack("b", encoded[1:2])[0]
        return candidate_expected_offsets[index] + len(encoded) + displacement

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    if (
        split_operands(candidate_tracked_load)[0] != "eax"
        or split_operands(candidate_tracked_load)[1] not in tracked_expressions
        or split_operands(candidate_test) != ("eax", "eax")
        or short_branch_target(2) != 0x21
        or split_operands(candidate_track_node_load)
        not in {("eax", "eax+56"), ("eax", "eax+0x38")}
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(candidate_owner_compare).split(",", 1)[0]
        )
        not in {"eax+4", "eax+0x4"}
        or split_operands(candidate_owner_compare)[1] != "ecx"
        or short_branch_target(5) != 0x21
        or split_operands(candidate_vptr)[0] != "edx"
        or candidate_vptr_expression not in meter_expressions
        or _cc_cfg._instruction_operand(candidate_false).strip().lower()
        not in {"0", "0x0"}
        or split_operands(candidate_receiver)[0] != "ecx"
        or candidate_receiver_expression != candidate_vptr_expression
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(candidate_call))
        not in {"edx+96", "edx+0x60"}
        or _cc_cfg._instruction_operand(candidate_return).strip() not in {"", "0"}
        or invocation_indices != (9,)
        or _cc_cfg._cleanup_after(candidate.instructions, 9) is not None
        or candidate.local_control_flow_indices
        or candidate.local_control_flow_targets
    ):
        raise ValueError(
            "HUD hide-tracked-progress SetVisible bridge rejects candidate "
            "storage, reaching definition, owner guard, receiver/vptr, "
            "argument, ordinal, slot, cleanup, or immediate-return topology "
            "drift"
        )

    expected_relocations = {
        0x01: _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_TRACKED_SLOT_DISPLACEMENT,
        0x13: _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_DISPLACEMENT,
        0x1A: _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_DISPLACEMENT,
    }
    expected_mask = {
        index
        for offset in expected_relocations
        for index in range(offset, offset + 4)
    }
    if (
        len(definition.data) != 0x30
        or definition.data[:0x22]
        != b"".join(body for _offset, _mnemonic, body in candidate_fixed)
        or definition.data[0x22:] != b"\x90" * 0x0E
        or {row.offset for row in definition.relocations}
        != set(expected_relocations)
        or len(definition.relocations) != len(expected_relocations)
        or any(
            row.type != IMAGE_REL_I386_DIR32
            or row.symbol_name != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
            or struct.unpack_from("<I", definition.data, row.offset)[0]
            != expected_relocations.get(row.offset)
            for row in definition.relocations
        )
        or {
            index
            for index, value in enumerate(definition.relocation_mask)
            if value
        }
        != expected_mask
        or definition.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in definition.defined_external_data
    ):
        raise ValueError(
            "HUD hide-tracked-progress SetVisible bridge rejects missing, "
            "extra, malformed, aliased, wrong-target, wrong-addend, or "
            "wrong-mask candidate +0xc20/+0xd9c DIR32 relocations; "
            f"data_len={len(definition.data)}, "
            f"data={definition.data.hex()}, "
            "relocations="
            f"{tuple((row.offset, row.type, row.symbol_name) for row in definition.relocations)!r}, "
            "mask="
            f"{tuple(index for index, value in enumerate(definition.relocation_mask) if value)!r}, "
            "aggregate_undefined_count="
            f"{definition.undefined_external_data.count(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)}"
        )

    retail_bridges = {
        _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_RETAIL_RECEIVER_ADDRESS:
            expected_storage,
    }
    candidate_bridges = {
        _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL: aggregate_identity,
    }
    required_row = {
        "ordinal": _cc_catalog.HUD_UI_MGR_HIDE_TRACKED_PROGRESS_RETAIL_CALL_ORDINAL,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": expected_storage,
        "slot_displacement": _cc_catalog.HUD_UI_MGR_HIDE_TRACKED_PROGRESS_SLOT,
        "cleanup_bytes": None,
    }
    retail_contract = _cc_extraction.extract_invocation_contract(
        retail_instructions,
        source="bn",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        reviewed_register_storage_bridges=retail_bridges,
    )
    candidate_contract = _cc_extraction.extract_invocation_contract(
        candidate.instructions,
        source="cod",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        reviewed_register_storage_bridges=candidate_bridges,
    )
    if retail_contract != [required_row] or candidate_contract != [required_row]:
        raise ValueError(
            "HUD hide-tracked-progress SetVisible bridge cannot derive the "
            "exact retail/candidate sensor-meter storage identity and "
            "virtual call"
        )
    return retail_bridges, candidate_bridges


def _hud_ui_mgr_objective_visibility_candidate_vptr_bridges(
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
    dict[str, ReviewedAbsoluteStorageLoadBridge],
    dict[str, ReviewedVptrStorageBridge],
    dict[str, ReviewedMemberVptrStorageBridge],
]:
    """Bridge only 0x411760's four exact slot-0x60 visibility calls."""
    from _recoil.call_contract.records import (
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedMemberVptrStorageBridge,
        ReviewedStaticStorageReferenceBridge,
        ReviewedVptrStorageBridge,
    )
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_OBJECTIVE_CEIL_CALLER_START:
        return {}, {}, {}, {}

    caller_symbol_id = (
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_CEIL_CALLER_IDENTITY.removeprefix("symbol:")
    )
    caller_symbol = document.collection("symbols").get(caller_symbol_id)
    caller_trace = (
        caller_symbol.get("source_traceability")
        if isinstance(caller_symbol, Mapping)
        else None
    )
    source_edges = (
        caller_trace.get("source_edges")
        if isinstance(caller_trace, Mapping)
        else None
    )
    aggregate_symbol = document.collection("symbols").get(
        _cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID
    )
    aggregate_storage = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    caller_addresses = sorted(
        address
        for address, identity in indexes.by_address.items()
        if identity == caller_identity
    )
    if (
        caller_identity != _cc_catalog.HUD_UI_MGR_OBJECTIVE_CEIL_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_CEIL_CALLER_END_EXCLUSIVE
        or caller_addresses
        != [_cc_catalog.HUD_UI_MGR_OBJECTIVE_CEIL_CALLER_START]
        or caller_identity in indexes.provider_ids
        or not isinstance(caller_symbol, Mapping)
        or caller_symbol.get("binary") != "recoil"
        or caller_symbol.get("kind") != "function"
        or caller_symbol.get("pipeline_class") != "authored"
        or caller_symbol.get("ownership_state") != "primary-owned"
        or caller_symbol.get("address")
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_CEIL_CALLER_START
        or caller_symbol.get("end_exclusive")
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_CEIL_CALLER_END_EXCLUSIVE
        or caller_symbol.get("extent_state") != "known"
        or caller_symbol.get("size") != 0x90
        or caller_symbol.get("navigation_name")
        != "HudUiMgrObjective::SetVisibleAndResetMeterFill"
        or caller_symbol.get("output_section_id") != "recoil:section:.text"
        or caller_symbol.get("physical_block_id")
        != "recoil:block:0x404ca0"
        or not exact_selected_target_membership(
            caller_symbol.get("verification_target_ids", ()),
            "recoil:vc5-target:hud_404ca0_415ab0_authored_order",
        )
        or not isinstance(caller_trace, Mapping)
        or caller_trace.get("state") != "resolved"
        or caller_trace.get("reason_code") not in {None, ""}
        or not isinstance(source_edges, list)
        or len(source_edges) != 1
        or not isinstance(source_edges[0], Mapping)
        or source_edges[0].get("relation") != "defines"
        or source_edges[0].get("anchor_id")
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_CEIL_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": "src/Battlesport/hud.cpp"}
        or not isinstance(aggregate_symbol, Mapping)
        or aggregate_symbol.get("binary") != "recoil"
        or aggregate_symbol.get("kind") != "data"
        or aggregate_symbol.get("disposition") != "authored"
        or aggregate_symbol.get("address") != _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
        or aggregate_symbol.get("navigation_name")
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_NAME
        or aggregate_symbol.get("extent_state") != "unknown"
        or aggregate_symbol.get("storage_contribution_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_STORAGE_ID]
        or aggregate_symbol.get("verification_target_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_ID]
        or indexes.storage_by_address.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
        != aggregate_storage
        or aggregate_storage in indexes.provider_ids
    ):
        raise ValueError(
            "HUD objective visibility vptr bridge requires the exact "
            "authored caller/source and aggregate storage authority"
        )

    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_OBJECTIVE_CEIL_CALLER_START
    ]
    if (
        target is None
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
        or len(contribution_rows) != 1
    ):
        raise ValueError(
            "HUD objective visibility vptr bridge requires one exact current "
            "HUD listing/source authority"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != r"\?SetVisibleAndResetMeterFill@HudUiMgrObjective@@.*"
        or re.fullmatch(
            r"\?SetVisibleAndResetMeterFill@HudUiMgrObjective@@.*",
            str(getattr(candidate.caller_definition, "symbol", "")),
        )
        is None
        or getattr(contribution_row, "name", "")
        != "HudUiMgrObjective::SetVisibleAndResetMeterFill"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD objective visibility vptr bridge requires the exact authored "
            "hud.cpp contribution row"
        )

    label_storage = _cc_catalog.HUD_UI_MGR_OBJECTIVE_LABEL_STORAGE_IDENTITY
    meter_storage = _cc_catalog.HUD_UI_MGR_OBJECTIVE_METER_STORAGE_IDENTITY
    expected_rows = {
        0: {
            "ordinal": 0,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": label_storage,
            "slot_displacement": (
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_VISIBILITY_SLOT_DISPLACEMENT
            ),
            "cleanup_bytes": None,
        },
        1: {
            "ordinal": 1,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": meter_storage,
            "slot_displacement": (
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_VISIBILITY_SLOT_DISPLACEMENT
            ),
            "cleanup_bytes": None,
        },
        5: {
            "ordinal": 5,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": label_storage,
            "slot_displacement": (
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_VISIBILITY_SLOT_DISPLACEMENT
            ),
            "cleanup_bytes": None,
        },
        6: {
            "ordinal": 6,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": meter_storage,
            "slot_displacement": (
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_VISIBILITY_SLOT_DISPLACEMENT
            ),
            "cleanup_bytes": None,
        },
    }
    if (
        len(expected) != 7
        or any(expected[ordinal] != row for ordinal, row in expected_rows.items())
    ):
        raise ValueError(
            "HUD objective visibility vptr bridge requires the exact "
            "immutable retail ordinal-0/1/5/6 storage and slot contracts"
        )

    caller = candidate.caller_definition
    caller_symbol_name = _cc_catalog.HUD_UI_MGR_OBJECTIVE_VISIBILITY_CALLER_SYMBOL
    exact_body = bytes.fromhex(
        "51 85 c9 74 6a "
        "8b 0d 28 08 00 00 56 6a 01 8b 01 ff 50 60 "
        "8b 15 2c 08 00 00 b9 2c 08 00 00 6a 01 ff 52 60 "
        "6a 00 6a 00 ff 15 00 00 00 00 83 c4 08 "
        "e8 00 00 00 00 d9 05 70 08 00 00 8b f0 "
        "e8 00 00 00 00 2b c6 "
        "c7 05 6c 09 00 00 00 00 00 00 "
        "89 44 24 04 "
        "c7 05 70 09 00 00 01 00 00 00 "
        "db 44 24 04 5e "
        "d9 15 64 08 00 00 "
        "d9 1d 88 08 00 00 "
        "59 c3 "
        "8b 0d 28 08 00 00 6a 00 8b 01 ff 50 60 "
        "8b 15 2c 08 00 00 b9 2c 08 00 00 6a 00 ff 52 60 "
        "59 c3 90 90"
    )
    expected_relocations = (
        (
            0x07,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_LABEL_DISPLACEMENT,
        ),
        (
            0x15,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_METER_DISPLACEMENT,
        ),
        (
            0x1A,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_METER_DISPLACEMENT,
        ),
        (
            0x29,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_CANDIDATE_IMPORT_SYMBOL,
            0,
        ),
        (
            0x31,
            IMAGE_REL_I386_REL32,
            _cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL,
            0,
        ),
        (
            0x37,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x870,
        ),
        (
            0x3E,
            IMAGE_REL_I386_REL32,
            _cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL,
            0,
        ),
        (
            0x46,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x96C,
        ),
        (
            0x54,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x970,
        ),
        (
            0x63,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x864,
        ),
        (
            0x69,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x888,
        ),
        (
            0x71,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_LABEL_DISPLACEMENT,
        ),
        (
            0x7E,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_METER_DISPLACEMENT,
        ),
        (
            0x83,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_METER_DISPLACEMENT,
        ),
    )
    bounded_relocations = (
        tuple(row for row in caller.relocations if row.offset < len(exact_body))
        if caller is not None
        else ()
    )
    actual_relocations = (
        tuple(
            (
                row.offset,
                row.type,
                row.symbol_name,
                struct.unpack_from("<I", caller.data, row.offset)[0],
            )
            for row in bounded_relocations
        )
        if caller is not None
        and all(row.offset + 4 <= len(caller.data) for row in bounded_relocations)
        else ()
    )
    expected_mask = {
        offset
        for relocation_offset, _, _, _ in expected_relocations
        for offset in range(relocation_offset, relocation_offset + 4)
    }
    if (
        caller is None
        or caller.symbol != caller_symbol_name
        or caller.data != exact_body
        or len(caller.relocation_mask) != len(caller.data)
        or actual_relocations != expected_relocations
        or {
            offset
            for offset in range(len(exact_body))
            if caller.relocation_mask[offset]
        }
        != expected_mask
        or (
            caller.undefined_external_data + caller.defined_external_data
        ).count(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
        != 1
    ):
        raise ValueError(
            "HUD objective visibility vptr bridge rejects exact 0x90 caller "
            "symbol/body, DIR32/REL32 target/addend/order, external identity, "
            "or complete relocation-mask drift"
        )

    expected_instructions = (
        (0x00, "push", ("51",)),
        (0x01, "test", ("85", "c9")),
        (0x03, "je", ("74", "6a")),
        (0x05, "mov", ("8b", "0d", "28", "08", "00", "00")),
        (0x0B, "push", ("56",)),
        (0x0C, "push", ("6a", "01")),
        (0x0E, "mov", ("8b", "01")),
        (0x10, "call", ("ff", "50", "60")),
        (0x13, "mov", ("8b", "15", "2c", "08", "00", "00")),
        (0x19, "mov", ("b9", "2c", "08", "00", "00")),
        (0x1E, "push", ("6a", "01")),
        (0x20, "call", ("ff", "52", "60")),
        (0x23, "push", ("6a", "00")),
        (0x25, "push", ("6a", "00")),
        (0x27, "call", ("ff", "15", "00", "00", "00", "00")),
        (0x2D, "add", ("83", "c4", "08")),
        (0x30, "call", ("e8", "00", "00", "00", "00")),
        (0x35, "fld", ("d9", "05", "70", "08", "00", "00")),
        (0x3B, "mov", ("8b", "f0")),
        (0x3D, "call", ("e8", "00", "00", "00", "00")),
        (0x42, "sub", ("2b", "c6")),
        (
            0x44,
            "mov",
            ("c7", "05", "6c", "09", "00", "00", "00", "00", "00", "00"),
        ),
        (0x4E, "mov", ("89", "44", "24", "04")),
        (
            0x52,
            "mov",
            ("c7", "05", "70", "09", "00", "00", "01", "00", "00", "00"),
        ),
        (0x5C, "fild", ("db", "44", "24", "04")),
        (0x60, "pop", ("5e",)),
        (0x61, "fst", ("d9", "15", "64", "08", "00", "00")),
        (0x67, "fstp", ("d9", "1d", "88", "08", "00", "00")),
        (0x6D, "pop", ("59",)),
        (0x6E, "ret", ("c3",)),
        (0x6F, "mov", ("8b", "0d", "28", "08", "00", "00")),
        (0x75, "push", ("6a", "00")),
        (0x77, "mov", ("8b", "01")),
        (0x79, "call", ("ff", "50", "60")),
        (0x7C, "mov", ("8b", "15", "2c", "08", "00", "00")),
        (0x82, "mov", ("b9", "2c", "08", "00", "00")),
        (0x87, "push", ("6a", "00")),
        (0x89, "call", ("ff", "52", "60")),
        (0x8C, "pop", ("59",)),
        (0x8D, "ret", ("c3",)),
    )
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    counts: dict[int, int] = {}
    for offset in offsets:
        if offset is not None:
            counts[offset] = counts.get(offset, 0) + 1
    index_by_offset = {
        offset: index
        for index, offset in enumerate(offsets)
        if offset is not None and counts.get(offset) == 1
    }
    instruction_by_offset = {
        offset: candidate.instructions[index]
        for offset, index in index_by_offset.items()
    }
    bounded_rows = tuple(
        (
            offset,
            _cc_cfg._instruction_mnemonic(instruction_by_offset[offset]),
            tuple(value.lower() for value in instruction_by_offset[offset].bytes),
        )
        for offset in sorted(index_by_offset)
        if offset < len(exact_body)
    )
    if bounded_rows != expected_instructions:
        raise ValueError(
            "HUD objective visibility vptr bridge requires the exact complete "
            "retail-ordered listing, branch, call, FPU, and epilogue topology"
        )

    aggregate = _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
    label_specs = tuple(
        (
            reference_offset,
            argument_offset,
            vptr_offset,
            call_offset,
            register,
            value,
        )
        for (
            ordinal,
            reference_offset,
            _,
            argument_offset,
            vptr_offset,
            call_offset,
            register,
            value,
        ) in _cc_catalog.HUD_UI_MGR_OBJECTIVE_VISIBILITY_CALL_SPECS
        if ordinal in {0, 5}
    )
    for reference_offset, argument_offset, vptr_offset, call_offset, register, value in label_specs:
        reference = _cc_cfg._instruction_operand(
            instruction_by_offset[reference_offset]
        ).split(",", 1)
        vptr = _cc_cfg._instruction_operand(
            instruction_by_offset[vptr_offset]
        ).split(",", 1)
        if (
            len(reference) != 2
            or reference[0].strip().lower() != "ecx"
            or _cc_targets._exact_memory_expression(reference[1])
            not in {
                f"{aggregate}+{_cc_catalog.HUD_UI_MGR_OBJECTIVE_LABEL_DISPLACEMENT}",
                (
                    f"{aggregate}"
                    f"+0x{_cc_catalog.HUD_UI_MGR_OBJECTIVE_LABEL_DISPLACEMENT:x}"
                ),
            }
            or _cc_cfg._instruction_operand(
                instruction_by_offset[argument_offset]
            ).strip()
            not in {str(value), hex(value)}
            or len(vptr) != 2
            or vptr[0].strip().lower() != register
            or _cc_targets._exact_memory_expression(vptr[1]) != "ecx"
            or _cc_targets._exact_memory_expression(
                _cc_cfg._instruction_operand(instruction_by_offset[call_offset])
            )
            not in {f"{register}+96", f"{register}+0x60"}
            or not (
                index_by_offset[reference_offset]
                < index_by_offset[argument_offset]
                < index_by_offset[vptr_offset]
                < index_by_offset[call_offset]
            )
        ):
            raise ValueError(
                "HUD objective visibility vptr bridge rejects label "
                "receiver/argument/one-load-vptr/slot reaching-definition drift"
            )

    meter_specs = tuple(
        (
            load_offset,
            receiver_offset,
            argument_offset,
            call_offset,
            register,
            value,
        )
        for (
            ordinal,
            load_offset,
            receiver_offset,
            argument_offset,
            _,
            call_offset,
            register,
            value,
        ) in _cc_catalog.HUD_UI_MGR_OBJECTIVE_VISIBILITY_CALL_SPECS
        if ordinal in {1, 6}
    )
    for load_offset, receiver_offset, argument_offset, call_offset, register, value in meter_specs:
        load = _cc_cfg._instruction_operand(
            instruction_by_offset[load_offset]
        ).split(",", 1)
        receiver = _cc_cfg._instruction_operand(
            instruction_by_offset[receiver_offset]
        ).split(",", 1)
        meter_expressions = {
            f"{aggregate}+{_cc_catalog.HUD_UI_MGR_OBJECTIVE_METER_DISPLACEMENT}",
            (
                f"{aggregate}"
                f"+0x{_cc_catalog.HUD_UI_MGR_OBJECTIVE_METER_DISPLACEMENT:x}"
            ),
        }
        if (
            len(load) != 2
            or load[0].strip().lower() != register
            or _cc_targets._exact_memory_expression(load[1]) not in meter_expressions
            or len(receiver) != 2
            or receiver[0].strip().lower() != "ecx"
            or _cc_targets._exact_memory_expression(receiver[1])
            not in (
                meter_expressions
                | {f"OFFSETFLAT:{value}" for value in meter_expressions}
            )
            or _cc_cfg._instruction_operand(
                instruction_by_offset[argument_offset]
            ).strip()
            not in {str(value), hex(value)}
            or _cc_targets._exact_memory_expression(
                _cc_cfg._instruction_operand(instruction_by_offset[call_offset])
            )
            not in {f"{register}+96", f"{register}+0x60"}
            or not (
                index_by_offset[load_offset]
                < index_by_offset[receiver_offset]
                < index_by_offset[argument_offset]
                < index_by_offset[call_offset]
            )
        ):
            raise ValueError(
                "HUD objective visibility vptr bridge rejects embedded-meter "
                "vptr/receiver/argument/slot reaching-definition drift"
            )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    visibility_call_offsets = tuple(
        call_offset
        for _, _, _, _, _, call_offset, _, _ in (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_VISIBILITY_CALL_SPECS
        )
    )
    all_call_offsets = tuple(
        offset
        for _, offset in _cc_catalog.HUD_UI_MGR_OBJECTIVE_CANDIDATE_INVOCATION_SPECS
    )
    if (
        len(invocation_indices)
        != len(_cc_catalog.HUD_UI_MGR_OBJECTIVE_CANDIDATE_INVOCATION_SPECS)
        or any(
            invocation_indices[ordinal] != index_by_offset[offset]
            for ordinal, offset in (
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_CANDIDATE_INVOCATION_SPECS
            )
        )
        or [
            offset
            for offset in sorted(index_by_offset)
            if offset < len(exact_body)
            and _cc_cfg._instruction_mnemonic(instruction_by_offset[offset])
            in {"call", "jmp"}
        ]
        != list(all_call_offsets)
        or any(
            _cc_cfg._cleanup_after(
                candidate.instructions,
                index_by_offset[offset],
            )
            is not None
            for offset in visibility_call_offsets
        )
        or _cc_cfg._cleanup_after(
            candidate.instructions,
            index_by_offset[0x27],
        )
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_CEIL_CLEANUP_BYTES
        or any(
            _cc_cfg._cleanup_after(
                candidate.instructions,
                index_by_offset[offset],
            )
            is not None
            for offset in (0x30, 0x3D)
        )
        or candidate.local_control_flow_indices
        & frozenset(index_by_offset.values())
        or any(
            target in frozenset(index_by_offset.values())
            for targets in candidate.local_control_flow_targets.values()
            for target in targets
        )
    ):
        raise ValueError(
            "HUD objective visibility vptr bridge rejects visibility/ceil/"
            "_ftol call order, cleanup, or alternate local CFG drift"
        )

    static_bridges = {
        f"{aggregate}+2088": ReviewedStaticStorageReferenceBridge(
            aggregate_symbol=aggregate,
            displacement=_cc_catalog.HUD_UI_MGR_OBJECTIVE_LABEL_DISPLACEMENT,
            access_width=4,
            storage_identity=aggregate_storage,
        )
    }
    absolute_bridges = {
        normalize_address(hex(load_offset)): ReviewedAbsoluteStorageLoadBridge(
            register="edx",
            aggregate_symbol=aggregate,
            displacement=_cc_catalog.HUD_UI_MGR_OBJECTIVE_METER_DISPLACEMENT,
            access_width=4,
            storage_identity=aggregate_storage,
        )
        for ordinal, load_offset, _, _, _, _, _, _ in (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_VISIBILITY_CALL_SPECS
        )
        if ordinal in {1, 6}
    }
    vptr_bridges = {
        normalize_address(hex(call_offset)): ReviewedVptrStorageBridge(
            register=register,
            provenance=meter_storage,
            storage_identity=meter_storage,
            slot_displacement=(
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_VISIBILITY_SLOT_DISPLACEMENT
            ),
            identity_kind="virtual-slot",
        )
        for ordinal, _, _, _, _, call_offset, register, _ in (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_VISIBILITY_CALL_SPECS
        )
        if ordinal in {1, 6}
    }
    member_vptr_bridges = {
        normalize_address(hex(vptr_offset)): ReviewedMemberVptrStorageBridge(
            register="eax",
            source_register="ecx",
            source_provenance=aggregate_storage,
            receiver_register="ecx",
            receiver_provenance=aggregate_storage,
            storage_identity=label_storage,
            slot_displacement=_cc_catalog.HUD_UI_MGR_OBJECTIVE_VISIBILITY_SLOT_DISPLACEMENT,
            call_address=normalize_address(hex(call_offset)),
        )
        for ordinal, _, _, _, vptr_offset, call_offset, _, _ in (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_VISIBILITY_CALL_SPECS
        )
        if ordinal in {0, 5}
    }
    return (
        static_bridges,
        absolute_bridges,
        vptr_bridges,
        member_vptr_bridges,
    )


def _hud_ui_mgr_disable_visibility_cluster_candidate_bridges(
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
    dict[str, ReviewedAbsoluteStorageLoadBridge],
    dict[str, ReviewedVptrStorageBridge],
    dict[str, ReviewedMemberVptrStorageBridge],
]:
    """Bridge DisableHud's exact eight-call visibility/timer cluster."""
    from _recoil.call_contract.records import (
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedMemberVptrStorageBridge,
        ReviewedStaticStorageReferenceBridge,
        ReviewedVptrStorageBridge,
    )
    caller_start = normalize_address(caller_start)
    if caller_start != _cc_catalog.HUD_UI_MGR_DISABLE_CALLER_START:
        return {}, {}, {}, {}

    current_static, current_member = (
        _cc_recoil_hud_layout._hud_ui_mgr_disable_current_layout_candidate_bridges(
            expected,
            candidate,
            document=document,
            caller_identity=caller_identity,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            indexes=indexes,
        )
    )
    exact_current_static = {
        (
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
            f"+{_cc_catalog.HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_DISPLACEMENT}"
        ): ReviewedStaticStorageReferenceBridge(
            aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            displacement=_cc_catalog.HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_DISPLACEMENT,
            access_width=4,
            storage_identity=f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}",
        )
    }
    exact_current_member = {
        normalize_address(
            hex(_cc_catalog.HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_VPTR_LOAD_OFFSET)
        ): ReviewedMemberVptrStorageBridge(
            register="edx",
            source_register="ecx",
            source_provenance=f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}",
            receiver_register="ecx",
            receiver_provenance=f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}",
            storage_identity=(
                _cc_catalog.HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_STORAGE_IDENTITY
            ),
            slot_displacement=(
                _cc_catalog.HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_SLOT_DISPLACEMENT
            ),
            call_address=normalize_address(
                hex(_cc_catalog.HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_CALL_OFFSET)
            ),
        )
    }
    if (
        current_static != exact_current_static
        or current_member != exact_current_member
    ):
        raise ValueError(
            "HUD DisableHud visibility-cluster bridge requires WSI-007's "
            "exact caller/source/aggregate/current-layout authority"
        )

    expected_suffix = [
        {
            "ordinal": ordinal,
            "form": "call",
            "dispatch": dispatch,
            "identity_kind": identity_kind,
            "target_identity": target_identity,
            "storage_identity": storage_identity,
            "slot_displacement": slot_displacement,
            "cleanup_bytes": None,
        }
        for (
            ordinal,
            dispatch,
            identity_kind,
            target_identity,
            storage_identity,
            slot_displacement,
        ) in (
            (
                5,
                "indirect",
                "virtual-slot",
                "",
                "load(storage:recoil:data:0x4e5ed0+0x6ac)",
                0x60,
            ),
            (
                6,
                "indirect",
                "virtual-slot",
                "",
                "load(load(storage:recoil:data:0x4e5ed0+0x974))",
                0x60,
            ),
            (
                7,
                "indirect",
                "virtual-slot",
                "",
                "load(storage:recoil:data:0x4e5ed0+0x978)",
                0x60,
            ),
            (
                8,
                "indirect",
                "virtual-slot",
                "",
                "load(storage:recoil:data:0x4e5ed0+0x768)",
                0x60,
            ),
            (
                9,
                "indirect",
                "virtual-slot",
                "",
                "load(load(storage:recoil:data:0x4e5ed0+0x824))",
                0x60,
            ),
            (
                10,
                "indirect",
                "virtual-slot",
                "",
                "load(load(storage:recoil:data:0x4e5ed0+0x828))",
                0x60,
            ),
            (
                11,
                "indirect",
                "virtual-slot",
                "",
                "load(storage:recoil:data:0x4e5ed0+0x82c)",
                0x60,
            ),
            (
                12,
                "direct",
                "direct",
                "symbol:recoil:function:0x408310",
                "",
                None,
            ),
            (
                13,
                "direct",
                "direct",
                "symbol:recoil:function:0x490600",
                "",
                None,
            ),
            (
                14,
                "direct",
                "direct",
                "symbol:recoil:function:0x408360",
                "",
                None,
            ),
            (
                15,
                "indirect",
                "virtual-slot",
                "",
                "load(storage:recoil:data:0x4ea654)",
                0x60,
            ),
        )
    ]
    if (
        len(expected) != 16
        or list(expected[5:]) != expected_suffix
        or any(
            sum(row == expected_row for row in expected) != 1
            for expected_row in expected_suffix
        )
    ):
        raise ValueError(
            "HUD DisableHud visibility-cluster bridge requires the exact "
            "immutable ordinal-5-through-15 retail contract"
        )

    symbols = document.collection("symbols")
    storage_rows = document.collection("storage_contributions")
    targets = document.collection("verification_targets")
    timer_symbol = symbols.get(_cc_catalog.HUD_UI_MGR_DISABLE_TIMER_PANEL_SYMBOL_ID)
    timer_storage = storage_rows.get(
        _cc_catalog.HUD_UI_MGR_DISABLE_TIMER_PANEL_STORAGE_ID
    )
    timer_target = targets.get(_cc_catalog.HUD_UI_MGR_DISABLE_TIMER_PANEL_TARGET_ID)
    timer_registration = (
        timer_target.get("registration")
        if isinstance(timer_target, Mapping)
        else None
    )
    if (
        not isinstance(timer_symbol, Mapping)
        or _cc_identity._symbol_identity(
            _cc_catalog.HUD_UI_MGR_DISABLE_TIMER_PANEL_SYMBOL_ID,
            timer_symbol,
        )
        != f"symbol:{_cc_catalog.HUD_UI_MGR_DISABLE_TIMER_PANEL_SYMBOL_ID}"
        or timer_symbol.get("binary") != "recoil"
        or timer_symbol.get("kind") != "data"
        or timer_symbol.get("disposition") != "authored"
        or timer_symbol.get("navigation_name") != "g_HudUiMgrTimerPanel"
        or timer_symbol.get("extent_state") != "unknown"
        or normalize_address(str(timer_symbol.get("address", "")))
        != _cc_catalog.HUD_UI_MGR_DISABLE_TIMER_PANEL_ADDRESS
        or timer_symbol.get("output_section_id") != "recoil:section:.data"
        or timer_symbol.get("storage_contribution_ids")
        != [_cc_catalog.HUD_UI_MGR_DISABLE_TIMER_PANEL_STORAGE_ID]
        or timer_symbol.get("verification_target_ids")
        != [_cc_catalog.HUD_UI_MGR_DISABLE_TIMER_PANEL_TARGET_ID]
        or not isinstance(timer_storage, Mapping)
        or timer_storage.get("binary") != "recoil"
        or timer_storage.get("kind") != "data-symbol"
        or timer_storage.get("output_section_id") != "recoil:section:.data"
        or timer_storage.get("overlap") != "none"
        or timer_storage.get("parent_contribution_id") is not None
        or timer_storage.get("owner_ids")
        != ["recoil:owner:hud_ui.hud_ui_timer_panel_class"]
        or timer_storage.get("symbol_ids")
        != [_cc_catalog.HUD_UI_MGR_DISABLE_TIMER_PANEL_SYMBOL_ID]
        or timer_storage.get("reference")
        != {
            "address": _cc_catalog.HUD_UI_MGR_DISABLE_TIMER_PANEL_ADDRESS,
            "evidence_ids": [],
            "extent_state": "unknown",
        }
        or not isinstance(timer_target, Mapping)
        or timer_target.get("binary") != "recoil"
        or timer_target.get("kind") != "vc5"
        or timer_target.get("name")
        != "hud_ui_timer_panel_global_accessors_data"
        or timer_target.get("symbol_ids")
        != [_cc_catalog.HUD_UI_MGR_DISABLE_TIMER_PANEL_SYMBOL_ID]
        or timer_target.get("unresolved_addresses") != []
        or not isinstance(timer_registration, Mapping)
        or timer_registration.get("manifest_path")
        != (
            "tools/vc5_verify_targets/"
            "hud_ui_timer_panel_global_accessors_data.json"
        )
        or timer_registration.get("source_from")
        != "src/GameZRecoil/zUI/zui.cpp"
        or timer_registration.get("data_addresses")
        != [_cc_catalog.HUD_UI_MGR_DISABLE_TIMER_PANEL_ADDRESS]
        or indexes.storage_by_address.get(
            _cc_catalog.HUD_UI_MGR_DISABLE_TIMER_PANEL_ADDRESS
        )
        != f"storage:{_cc_catalog.HUD_UI_MGR_DISABLE_TIMER_PANEL_SYMBOL_ID}"
    ):
        raise ValueError(
            "HUD DisableHud visibility-cluster bridge requires the exact "
            "typed timer-panel data/storage/verification-target authority"
        )

    caller = candidate.caller_definition
    cluster_start = _cc_catalog.HUD_UI_MGR_DISABLE_VISIBILITY_CLUSTER_START
    cluster_end = _cc_catalog.HUD_UI_MGR_DISABLE_VISIBILITY_CLUSTER_END_EXCLUSIVE
    exact_body = bytes.fromhex(
        "a1 ac 06 00 00 b9 ac 06 00 00 6a 00 ff 50 60 "
        "8b 0d 74 09 00 00 6a 00 8b 11 ff 52 60 "
        "a1 78 09 00 00 b9 78 09 00 00 6a 00 ff 50 60 "
        "8b 15 68 07 00 00 b9 68 07 00 00 6a 00 ff 52 60 "
        "8b 0d 24 08 00 00 6a 00 8b 01 ff 50 60 "
        "8b 0d 28 08 00 00 6a 00 8b 11 ff 52 60 "
        "a1 2c 08 00 00 b9 2c 08 00 00 6a 00 ff 50 60 "
        "c7 05 00 00 00 00 00 00 00 00 "
        "e8 00 00 00 00 85 c0 75 05 e8 00 00 00 00 "
        "e8 00 00 00 00 83 f8 02 75 05 a3 ec 02 00 00 "
        "8b 0d 84 47 00 00 6a 01 8b 11 ff 52 60 "
        "8b c3 5e 5b c3"
    )
    if (
        caller is None
        or caller.data[cluster_start:cluster_end] != exact_body
    ):
        raise ValueError(
            "HUD DisableHud visibility-cluster bridge requires the exact "
            "bounded object body through the epilogue"
        )

    expected_relocations = (
        (0x69, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x6AC),
        (0x6E, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x6AC),
        (0x79, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x974),
        (0x85, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x978),
        (0x8A, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x978),
        (0x95, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x768),
        (0x9A, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x768),
        (0xA5, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x824),
        (0xB2, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x828),
        (0xBE, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x82C),
        (0xC3, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x82C),
        (0xCE, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_DISABLE_ALT_CLIP_SYMBOL, 0),
        (0xD7, IMAGE_REL_I386_REL32, _cc_catalog.HUD_UI_MGR_DISABLE_DIRECT_CALLS[0][2], 0),
        (0xE0, IMAGE_REL_I386_REL32, _cc_catalog.HUD_UI_MGR_DISABLE_DIRECT_CALLS[1][2], 0),
        (0xE5, IMAGE_REL_I386_REL32, _cc_catalog.HUD_UI_MGR_DISABLE_DIRECT_CALLS[2][2], 0),
        (0xEF, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x2EC),
        (0xF5, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x4784),
    )
    bounded_relocations = tuple(
        row
        for row in caller.relocations
        if cluster_start <= row.offset < cluster_end
    )
    actual_relocations = tuple(
        (
            row.offset,
            row.type,
            row.symbol_name,
            struct.unpack_from("<I", caller.data, row.offset)[0],
        )
        for row in bounded_relocations
    )
    expected_mask = {
        offset
        for relocation_offset, _, _, _ in expected_relocations
        for offset in range(relocation_offset, relocation_offset + 4)
    }
    if (
        actual_relocations != expected_relocations
        or {
            offset
            for offset in range(cluster_start, cluster_end)
            if caller.relocation_mask[offset]
        }
        != expected_mask
    ):
        raise ValueError(
            "HUD DisableHud visibility-cluster bridge rejects missing, "
            "duplicate, extra, reordered, malformed, or mis-masked COFF "
            "relocations"
        )

    expected_instructions = (
        (0x68, "mov", ("a1", "ac", "06", "00", "00")),
        (0x6D, "mov", ("b9", "ac", "06", "00", "00")),
        (0x72, "push", ("6a", "00")),
        (0x74, "call", ("ff", "50", "60")),
        (0x77, "mov", ("8b", "0d", "74", "09", "00", "00")),
        (0x7D, "push", ("6a", "00")),
        (0x7F, "mov", ("8b", "11")),
        (0x81, "call", ("ff", "52", "60")),
        (0x84, "mov", ("a1", "78", "09", "00", "00")),
        (0x89, "mov", ("b9", "78", "09", "00", "00")),
        (0x8E, "push", ("6a", "00")),
        (0x90, "call", ("ff", "50", "60")),
        (0x93, "mov", ("8b", "15", "68", "07", "00", "00")),
        (0x99, "mov", ("b9", "68", "07", "00", "00")),
        (0x9E, "push", ("6a", "00")),
        (0xA0, "call", ("ff", "52", "60")),
        (0xA3, "mov", ("8b", "0d", "24", "08", "00", "00")),
        (0xA9, "push", ("6a", "00")),
        (0xAB, "mov", ("8b", "01")),
        (0xAD, "call", ("ff", "50", "60")),
        (0xB0, "mov", ("8b", "0d", "28", "08", "00", "00")),
        (0xB6, "push", ("6a", "00")),
        (0xB8, "mov", ("8b", "11")),
        (0xBA, "call", ("ff", "52", "60")),
        (0xBD, "mov", ("a1", "2c", "08", "00", "00")),
        (0xC2, "mov", ("b9", "2c", "08", "00", "00")),
        (0xC7, "push", ("6a", "00")),
        (0xC9, "call", ("ff", "50", "60")),
        (0xCC, "mov", ("c7", "05", "00", "00", "00", "00", "00", "00", "00", "00")),
        (0xD6, "call", ("e8", "00", "00", "00", "00")),
        (0xDB, "test", ("85", "c0")),
        (0xDD, "jne", ("75", "05")),
        (0xDF, "call", ("e8", "00", "00", "00", "00")),
        (0xE4, "call", ("e8", "00", "00", "00", "00")),
        (0xE9, "cmp", ("83", "f8", "02")),
        (0xEC, "jne", ("75", "05")),
        (0xEE, "mov", ("a3", "ec", "02", "00", "00")),
        (0xF3, "mov", ("8b", "0d", "84", "47", "00", "00")),
        (0xF9, "push", ("6a", "01")),
        (0xFB, "mov", ("8b", "11")),
        (0xFD, "call", ("ff", "52", "60")),
        (0x100, "mov", ("8b", "c3")),
        (0x102, "pop", ("5e",)),
        (0x103, "pop", ("5b",)),
        (0x104, "ret", ("c3",)),
    )
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    bounded_rows = tuple(
        (
            offset,
            _cc_cfg._instruction_mnemonic(candidate.instructions[index]),
            tuple(
                byte.lower()
                for byte in candidate.instructions[index].bytes
            ),
        )
        for index, offset in enumerate(offsets)
        if offset is not None and cluster_start <= offset < cluster_end
    )
    if bounded_rows != expected_instructions:
        raise ValueError(
            "HUD DisableHud visibility-cluster bridge requires exact, unique, "
            "contiguous candidate instructions and tail topology"
        )

    index_by_offset = {
        offset: index
        for index, offset in enumerate(offsets)
        if offset is not None
    }
    instruction_by_offset = {
        offset: candidate.instructions[index]
        for offset, index in index_by_offset.items()
    }
    embedded_semantics = (
        (0x68, 0x6D, 0x72, 0x74, "eax", 0x6AC),
        (0x84, 0x89, 0x8E, 0x90, "eax", 0x978),
        (0x93, 0x99, 0x9E, 0xA0, "edx", 0x768),
        (0xBD, 0xC2, 0xC7, 0xC9, "eax", 0x82C),
    )
    for (
        load_offset,
        receiver_offset,
        argument_offset,
        call_offset,
        register,
        displacement,
    ) in embedded_semantics:
        expressions = {
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}+{displacement}",
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}+0x{displacement:x}",
        }
        receiver_expressions = expressions | {
            f"OFFSETFLAT:{expression}" for expression in expressions
        }
        load_operands = _cc_cfg._instruction_operand(
            instruction_by_offset[load_offset]
        ).split(",", 1)
        receiver_operands = _cc_cfg._instruction_operand(
            instruction_by_offset[receiver_offset]
        ).split(",", 1)
        if (
            len(load_operands) != 2
            or load_operands[0].strip().lower() != register
            or _cc_targets._exact_memory_expression(load_operands[1]) not in expressions
            or len(receiver_operands) != 2
            or receiver_operands[0].strip().lower() != "ecx"
            or _cc_targets._exact_memory_expression(receiver_operands[1])
            not in receiver_expressions
            or _cc_cfg._instruction_operand(
                instruction_by_offset[argument_offset]
            ).strip()
            not in {"0", "0x0"}
            or _cc_targets._exact_memory_expression(
                _cc_cfg._instruction_operand(instruction_by_offset[call_offset])
            )
            not in {f"{register}+96", f"{register}+0x60"}
        ):
            raise ValueError(
                "HUD DisableHud visibility-cluster bridge rejects embedded "
                "storage/register/receiver/argument/slot semantic drift"
            )

    pointer_semantics = (
        (0x77, 0x7D, 0x7F, 0x81, "edx", 0x974),
        (0xA3, 0xA9, 0xAB, 0xAD, "eax", 0x824),
        (0xB0, 0xB6, 0xB8, 0xBA, "edx", 0x828),
        (0xF3, 0xF9, 0xFB, 0xFD, "edx", 0x4784),
    )
    for (
        reference_offset,
        argument_offset,
        vptr_offset,
        call_offset,
        register,
        displacement,
    ) in pointer_semantics:
        expressions = {
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}+{displacement}",
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}+0x{displacement:x}",
        }
        reference_operands = _cc_cfg._instruction_operand(
            instruction_by_offset[reference_offset]
        ).split(",", 1)
        vptr_operands = _cc_cfg._instruction_operand(
            instruction_by_offset[vptr_offset]
        ).split(",", 1)
        expected_argument = {"1", "0x1"} if call_offset == 0xFD else {"0", "0x0"}
        if (
            len(reference_operands) != 2
            or reference_operands[0].strip().lower() != "ecx"
            or _cc_targets._exact_memory_expression(reference_operands[1])
            not in expressions
            or _cc_cfg._instruction_operand(
                instruction_by_offset[argument_offset]
            ).strip()
            not in expected_argument
            or len(vptr_operands) != 2
            or vptr_operands[0].strip().lower() != register
            or _cc_targets._exact_memory_expression(vptr_operands[1]) != "ecx"
            or _cc_targets._exact_memory_expression(
                _cc_cfg._instruction_operand(instruction_by_offset[call_offset])
            )
            not in {f"{register}+96", f"{register}+0x60"}
        ):
            raise ValueError(
                "HUD DisableHud visibility-cluster bridge rejects pointer "
                "storage/register/receiver/argument/slot semantic drift"
            )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    expected_invocations = (
        (5, 0x74),
        (6, 0x81),
        (7, 0x90),
        (8, 0xA0),
        (9, 0xAD),
        (10, 0xBA),
        (11, 0xC9),
        (12, 0xD6),
        (13, 0xDF),
        (14, 0xE4),
        (15, 0xFD),
    )
    if (
        len(invocation_indices) != 16
        or any(
            invocation_indices[ordinal] != index_by_offset[offset]
            for ordinal, offset in expected_invocations
        )
        or candidate.local_control_flow_indices
        & frozenset(index_by_offset.values())
        or any(
            target in frozenset(index_by_offset.values())
            for targets_by_source in candidate.local_control_flow_targets.values()
            for target in targets_by_source
        )
    ):
        raise ValueError(
            "HUD DisableHud visibility-cluster bridge rejects invocation "
            "ordinal or alternate CFG drift"
        )
    if (
        candidate.instructions[index_by_offset[0xDD]].bytes[-1] != "05"
        or candidate.instructions[index_by_offset[0xEC]].bytes[-1] != "05"
    ):
        raise ValueError(
            "HUD DisableHud visibility-cluster bridge requires exact tail "
            "branches to +0xe4 and +0xf3"
        )

    function_names = (
        caller.undefined_external_functions
        + caller.defined_external_functions
    )
    for call_offset, ordinal, name, address, identity in (
        _cc_catalog.HUD_UI_MGR_DISABLE_DIRECT_CALLS
    ):
        instruction = candidate.instructions[index_by_offset[call_offset]]
        if (
            function_names.count(name) != 1
            or indexes.by_candidate_name.get(name) != identity
            or indexes.by_address.get(address) != identity
            or identity in indexes.provider_ids
            or name not in _cc_cfg._instruction_operand(instruction)
            or invocation_indices[ordinal] != index_by_offset[call_offset]
        ):
            raise ValueError(
                "HUD DisableHud visibility-cluster bridge requires exact "
                "authored direct-call identity and order"
            )
    data_names = caller.undefined_external_data + caller.defined_external_data
    if (
        data_names.count(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL) != 1
        or data_names.count(_cc_catalog.HUD_UI_MGR_DISABLE_ALT_CLIP_SYMBOL) != 1
        or _cc_catalog.HUD_UI_MGR_DISABLE_ALT_CLIP_SYMBOL
        not in _cc_cfg._instruction_operand(
            candidate.instructions[index_by_offset[0xCC]]
        )
    ):
        raise ValueError(
            "HUD DisableHud visibility-cluster bridge requires exact "
            "aggregate and alternate-clip data references"
        )

    embedded_specs = (
        (0x68, 0x74, "eax", 0x6AC),
        (0x84, 0x90, "eax", 0x978),
        (0x93, 0xA0, "edx", 0x768),
        (0xBD, 0xC9, "eax", 0x82C),
    )
    pointer_specs = (
        (0x77, 0x7F, 0x81, "edx", 0x974, f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"),
        (0xA3, 0xAB, 0xAD, "eax", 0x824, f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"),
        (0xB0, 0xB8, 0xBA, "edx", 0x828, f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"),
        (
            0xF3,
            0xFB,
            0xFD,
            "edx",
            0x4784,
            f"storage:{_cc_catalog.HUD_UI_MGR_DISABLE_TIMER_PANEL_SYMBOL_ID}",
        ),
    )
    aggregate_storage = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    absolute_bridges = {
        normalize_address(hex(load_offset)): ReviewedAbsoluteStorageLoadBridge(
            register=register,
            aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            displacement=displacement,
            access_width=4,
            storage_identity=aggregate_storage,
        )
        for load_offset, _, register, displacement in embedded_specs
    }
    vptr_bridges = {
        normalize_address(hex(call_offset)): ReviewedVptrStorageBridge(
            register=register,
            provenance=f"load({aggregate_storage}+0x{displacement:x})",
            storage_identity=f"load({aggregate_storage}+0x{displacement:x})",
            slot_displacement=_cc_catalog.HUD_UI_MGR_DISABLE_VISIBILITY_SLOT_DISPLACEMENT,
            identity_kind="virtual-slot",
        )
        for _, call_offset, register, displacement in embedded_specs
    }
    static_bridges: dict[
        str, ReviewedStaticStorageReferenceBridge
    ] = {}
    member_bridges: dict[str, ReviewedMemberVptrStorageBridge] = {}
    for (
        reference_offset,
        vptr_offset,
        call_offset,
        register,
        displacement,
        source_storage,
    ) in pointer_specs:
        expression = (
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}+{displacement}"
        )
        static_bridges[expression] = ReviewedStaticStorageReferenceBridge(
            aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            displacement=displacement,
            access_width=4,
            storage_identity=source_storage,
        )
        storage_identity = (
            f"load({source_storage})"
            if displacement == 0x4784
            else f"load(load({source_storage}+0x{displacement:x}))"
        )
        member_bridges[normalize_address(hex(vptr_offset))] = (
            ReviewedMemberVptrStorageBridge(
                register=register,
                source_register="ecx",
                source_provenance=source_storage,
                receiver_register="ecx",
                receiver_provenance=source_storage,
                storage_identity=storage_identity,
                slot_displacement=(
                    _cc_catalog.HUD_UI_MGR_DISABLE_VISIBILITY_SLOT_DISPLACEMENT
                ),
                call_address=normalize_address(hex(call_offset)),
            )
        )
        if index_by_offset[reference_offset] >= index_by_offset[vptr_offset]:
            raise ValueError(
                "HUD DisableHud visibility-cluster bridge rejects pointer "
                "field/vptr reaching-definition order"
            )
    return (
        static_bridges,
        absolute_bridges,
        vptr_bridges,
        member_bridges,
    )


def _hud_ui_mgr_set_float_timer_visible_bridges(
    expected: Sequence[Mapping[str, Any]],
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedStaticStorageReferenceBridge],
    dict[str, ReviewedVptrStorageBridge],
]:
    """Resolve only SetFloatTimerVisible's exact three-site provenance."""
    from _recoil.call_contract.records import (
        ReviewedStaticStorageReferenceBridge,
        ReviewedVptrStorageBridge,
        StorageContainer,
    )
    caller_start = normalize_address(caller_start)
    if caller_start != _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALLER_START:
        return {}, {}

    caller = candidate.caller_definition
    caller_addresses = sorted(
        address
        for address, identity in indexes.by_address.items()
        if identity == caller_identity
    )
    caller_name_rows = [
        (name, identity)
        for name, identity in indexes.by_candidate_name.items()
        if name.casefold()
        == _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALLER_SYMBOL.casefold()
    ]
    if (
        caller_identity != _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALLER_END_EXCLUSIVE
        or caller_addresses
        != [_cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALLER_START]
        or caller_identity in indexes.provider_ids
        or caller_name_rows
        not in (
            [],
            [
                (
                    _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALLER_SYMBOL,
                    _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALLER_IDENTITY,
                )
            ],
        )
        or caller is None
        or caller.symbol != _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALLER_SYMBOL
        or len(caller.data) not in {0x20, 0x30}
        or len(caller.relocation_mask) != len(caller.data)
    ):
        raise ValueError(
            "HUD SetFloatTimerVisible bridge requires the exact reviewed "
            "authored caller identity, extent, symbol, and body"
        )

    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALLER_START
    ]
    if (
        target is None
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
            "HUD SetFloatTimerVisible bridge requires one exact current HUD "
            "source authority"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "")
        != _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALLER_SYMBOL
        or getattr(contribution_row, "symbol_regex", None) is not None
        or getattr(contribution_row, "name", "")
        != "HudUiMgr::SetFloatTimerVisible"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD SetFloatTimerVisible bridge requires the exact authored "
            "hud.cpp contribution row"
        )

    caller_symbol_id = caller_identity.removeprefix("symbol:")
    caller_symbol = document.collection("symbols").get(caller_symbol_id)
    trace = (
        caller_symbol.get("source_traceability")
        if isinstance(caller_symbol, Mapping)
        else None
    )
    source_edges = (
        trace.get("source_edges") if isinstance(trace, Mapping) else None
    )
    if (
        not isinstance(caller_symbol, Mapping)
        or _cc_identity._symbol_identity(caller_symbol_id, caller_symbol)
        != caller_identity
        or caller_symbol.get("binary") != "recoil"
        or caller_symbol.get("kind") != "function"
        or caller_symbol.get("pipeline_class") != "authored"
        or caller_symbol.get("ownership_state") != "primary-owned"
        or caller_symbol.get("extent_state") != "known"
        or normalize_address(str(caller_symbol.get("address", "")))
        != _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALLER_START
        or normalize_address(str(caller_symbol.get("end_exclusive", "")))
        != _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALLER_END_EXCLUSIVE
        or caller_symbol.get("size") != 0x30
        or caller_symbol.get("navigation_name")
        != "HudUiMgr::SetFloatTimerVisible"
        or caller_symbol.get("output_section_id") != "recoil:section:.text"
        or caller_symbol.get("physical_block_id") != "recoil:block:0x404ca0"
        or not exact_required_target_membership(
            caller_symbol.get("verification_target_ids", ()),
            _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_VERIFICATION_TARGET_IDS,
        )
        or caller_symbol.get("logical_identity_key") not in {None, ""}
        or caller_symbol.get("icf_fold_status") not in {None, ""}
        or bool(caller_symbol.get("logical_aliases"))
        or not isinstance(trace, Mapping)
        or trace.get("state") != "resolved"
        or trace.get("reason_code") not in {None, ""}
        or not isinstance(source_edges, list)
        or len(source_edges) != 1
        or not isinstance(source_edges[0], Mapping)
        or source_edges[0].get("relation") != "defines"
        or source_edges[0].get("anchor_id")
        != _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
    ):
        raise ValueError(
            "HUD SetFloatTimerVisible bridge requires one exact unaliased "
            "reviewed caller and resolved source edge"
        )

    symbols = document.collection("symbols")
    storage_rows = document.collection("storage_contributions")
    aggregate_symbol = symbols.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID)
    aggregate_storage = storage_rows.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_STORAGE_ID)
    aggregate_target = document.collection("verification_targets").get(
        _cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_ID
    )
    aggregate_registration = (
        aggregate_target.get("registration")
        if isinstance(aggregate_target, Mapping)
        else None
    )
    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    aggregate_containers = [
        row
        for row in indexes.storage_containers
        if row.identity == aggregate_identity
    ]
    if (
        not isinstance(aggregate_symbol, Mapping)
        or aggregate_symbol.get("binary") != "recoil"
        or aggregate_symbol.get("kind") != "data"
        or normalize_address(str(aggregate_symbol.get("address", "")))
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
        or aggregate_symbol.get("disposition") != "authored"
        or aggregate_symbol.get("navigation_name") != _cc_catalog.HUD_UI_MGR_AGGREGATE_NAME
        or aggregate_symbol.get("output_section_id") != "recoil:section:.data"
        or aggregate_symbol.get("storage_contribution_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_STORAGE_ID]
        or aggregate_symbol.get("verification_target_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_ID]
        or aggregate_symbol.get("extent_state") != "unknown"
        or not isinstance(aggregate_storage, Mapping)
        or aggregate_storage.get("binary") != "recoil"
        or aggregate_storage.get("kind") != "data-symbol"
        or aggregate_storage.get("output_section_id") != "recoil:section:.data"
        or aggregate_storage.get("overlap") != "none"
        or aggregate_storage.get("symbol_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID]
        or not isinstance(aggregate_storage.get("reference"), Mapping)
        or normalize_address(
            str(aggregate_storage["reference"].get("address", ""))
        )
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
        or not isinstance(aggregate_target, Mapping)
        or aggregate_target.get("binary") != "recoil"
        or aggregate_target.get("kind") != "vc5"
        or aggregate_target.get("name") != "hud_ui_mgr_data"
        or aggregate_target.get("symbol_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID]
        or aggregate_target.get("unresolved_addresses") not in (None, [])
        or not isinstance(aggregate_registration, Mapping)
        or aggregate_registration.get("manifest_path")
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_MANIFEST
        or aggregate_registration.get("data_addresses")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS]
        or indexes.storage_by_address.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
        != aggregate_identity
        or aggregate_containers
        != [
            StorageContainer(
                address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS),
                address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
                + _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE,
                aggregate_identity,
            )
        ]
        or _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_DISPLACEMENT + 4
        > _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
    ):
        raise ValueError(
            "HUD SetFloatTimerVisible bridge requires the exact HUD aggregate "
            "symbol, timer-panel field, storage, target, and container authority"
        )

    timer_storage = _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_STORAGE_IDENTITY
    expected_rows = [
        {
            "ordinal": ordinal,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": timer_storage,
            "slot_displacement": _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_SLOT_DISPLACEMENT,
            "cleanup_bytes": None,
        }
        for ordinal in (0, 1)
    ]
    expected_rows.append(
        {
            "ordinal": 2,
            "form": "call",
            "dispatch": "direct",
            "identity_kind": "direct",
            "target_identity": _cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALLER_IDENTITY,
            "storage_identity": "",
            "slot_displacement": None,
            "cleanup_bytes": None,
        }
    )
    if list(expected) != expected_rows:
        raise ValueError(
            "HUD SetFloatTimerVisible bridge requires the exact immutable "
            "two-path SetVisible plus conditional successor retail contract"
        )

    retail_addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(caller_start),
    )
    retail_bytes = tuple(
        tuple(value.lower() for value in instruction.bytes)
        for instruction in retail_instructions
    )
    if (
        retail_addresses
        != (
            0x413770, 0x413771, 0x413773, 0x413779, 0x41377B,
            0x41377D, 0x41377F, 0x413781, 0x413784, 0x413786,
            0x413788, 0x41378A, 0x41378D, 0x41378F, 0x413791,
            0x413796, 0x413797,
        )
        or retail_bytes
        != (
            ("56",), ("8b", "f1"),
            ("8b", "0d", "58", "a6", "4e", "00"),
            ("85", "f6"), ("74", "09"), ("8b", "01"),
            ("6a", "01"), ("ff", "50", "60"), ("eb", "07"),
            ("8b", "11"), ("6a", "00"), ("ff", "52", "60"),
            ("85", "f6"), ("75", "05"),
            ("e8", "9a", "fe", "ff", "ff"), ("5e",), ("c3",),
        )
        or tuple(_cc_cfg._instruction_mnemonic(row) for row in retail_instructions)
        != (
            "push", "mov", "mov", "test", "je", "mov", "push",
            "call", "jmp", "mov", "push", "call", "test", "jne",
            "call", "pop", "retn",
        )
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(retail_instructions[2]).split(",", 1)[-1]
        )
        != "0x4ea658"
        or _cc_cfg._instruction_operand(retail_instructions[3]).replace(" ", "").lower()
        != "esi,esi"
        or _cc_cfg._instruction_operand(retail_instructions[4]).strip().lower()
        != "0x413786"
        or _cc_cfg._instruction_operand(retail_instructions[6]).strip().lower()
        not in {"1", "0x1"}
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(retail_instructions[7]))
        not in {"eax+96", "eax+0x60"}
        or _cc_cfg._instruction_operand(retail_instructions[8]).strip().lower()
        != "0x41378d"
        or _cc_cfg._instruction_operand(retail_instructions[10]).strip().lower()
        not in {"0", "0x0"}
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(retail_instructions[11]))
        not in {"edx+96", "edx+0x60"}
        or _cc_cfg._instruction_operand(retail_instructions[12]).replace(" ", "").lower()
        != "esi,esi"
        or _cc_cfg._instruction_operand(retail_instructions[13]).strip().lower()
        != "0x413796"
    ):
        raise ValueError(
            "HUD SetFloatTimerVisible bridge rejects immutable retail timer "
            "storage, boolean paths, receivers, slot, cleanup, successor order, "
            "or control-flow topology drift"
        )

    exact_tail_candidate_body = bytes.fromhex(
        "85 c9 8b 0d 88 47 00 00 74 08 8b 01 6a 01 ff 50 60 c3 "
        "8b 11 6a 00 ff 52 60 e9 00 00 00 00 90 90"
    )
    exact_call_candidate_body = bytes.fromhex(
        "56 8b f1 8b 0d 88 47 00 00 85 f6 74 09 8b 01 6a 01 "
        "ff 50 60 eb 07 8b 11 6a 00 ff 52 60 85 f6 75 05 e8 "
        "00 00 00 00 5e c3 90 90 90 90 90 90 90 90"
    )
    tail_relocations = (
        (
            _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_TAIL_RELOCATION_OFFSET,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_DISPLACEMENT,
        ),
        (
            _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_TAIL_DIRECT_CALL_OFFSET + 1,
            IMAGE_REL_I386_REL32,
            _cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALLER_SYMBOL,
            0,
        ),
    )
    call_relocations = (
        (
            _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALL_RELOCATION_OFFSET,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_DISPLACEMENT,
        ),
        (
            _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALL_DIRECT_CALL_OFFSET + 1,
            IMAGE_REL_I386_REL32,
            _cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALLER_SYMBOL,
            0,
        ),
    )
    if caller.data == exact_call_candidate_body:
        candidate_shape = "ordinary-call"
        expected_relocations = call_relocations
    elif caller.data == exact_tail_candidate_body:
        candidate_shape = "tail-call"
        expected_relocations = tail_relocations
    else:
        candidate_shape = ""
        expected_relocations = ()
    observed_relocations = tuple(
        (
            row.offset,
            row.type,
            row.symbol_name,
            (
                struct.unpack_from("<I", caller.data, row.offset)[0]
                if row.offset + 4 <= len(caller.data)
                else None
            ),
        )
        for row in caller.relocations
    )
    expected_mask = {
        index
        for offset, _, _, _ in expected_relocations
        for index in range(offset, offset + 4)
    }
    if (
        not candidate_shape
        or observed_relocations != expected_relocations
        or {
            index
            for index, masked in enumerate(caller.relocation_mask)
            if masked
        }
        != expected_mask
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or caller.defined_external_functions.count(
            _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALLER_SYMBOL
        )
        != 1
        or caller.defined_external_functions.count(
            _cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALLER_SYMBOL
        )
        != 1
        or (
            caller.defined_external_data
            + caller.undefined_external_functions
            + caller.defined_external_functions
        ).count(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
        != 0
    ):
        raise ValueError(
            "HUD SetFloatTimerVisible bridge requires the exact complete "
            "candidate body, timer-pointer/direct-call COFF rows and addends, "
            "relocation mask, and external population"
        )

    candidate_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    candidate_bytes = tuple(
        tuple(value.lower() for value in instruction.bytes)
        for instruction in candidate.instructions
    )
    expected_expression = (
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
        f"+{_cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_DISPLACEMENT}"
    )
    candidate_private_labels = tuple(
        _cc_cfg._instruction_operand(candidate.instructions[index]).strip()
        for index in (
            ((2,) if candidate_shape == "tail-call" else (4, 8, 13))
        )
    )
    exact_private_labels = (
        all(re.fullmatch(r"\$L[0-9]+", label) for label in candidate_private_labels)
        and len(set(candidate_private_labels)) == len(candidate_private_labels)
    )
    tail_instruction_shape = (
        candidate_shape == "tail-call"
        and exact_private_labels
        and candidate_offsets
        == (
            0x00, 0x02, 0x08, 0x0A, 0x0C, 0x0E,
            0x11, 0x12, 0x14, 0x16, 0x19,
        )
        and candidate_bytes
        == (
            ("85", "c9"),
            ("8b", "0d", "88", "47", "00", "00"),
            ("74", "08"), ("8b", "01"), ("6a", "01"),
            ("ff", "50", "60"), ("c3",), ("8b", "11"),
            ("6a", "00"), ("ff", "52", "60"),
            ("e9", "00", "00", "00", "00"),
        )
        and tuple(_cc_cfg._instruction_mnemonic(row) for row in candidate.instructions)
        == (
            "test", "mov", "je", "mov", "push", "call", "ret",
            "mov", "push", "call", "jmp",
        )
        and _cc_cfg._instruction_operand(candidate.instructions[0]).replace(" ", "").lower()
        == "ecx,ecx"
        and _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(candidate.instructions[1]).split(",", 1)[-1]
        )
        in {
            expected_expression,
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}+0x4788",
        }
        and _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(candidate.instructions[3]).split(",", 1)[-1]
        )
        == "ecx"
        and _cc_cfg._instruction_operand(candidate.instructions[4]).strip().lower()
        == "1"
        and _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(candidate.instructions[5]))
        in {"eax+96", "eax+0x60"}
        and _cc_cfg._instruction_operand(candidate.instructions[6]).strip() in {"", "0"}
        and _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(candidate.instructions[7]).split(",", 1)[-1]
        )
        == "ecx"
        and _cc_cfg._instruction_operand(candidate.instructions[8]).strip().lower()
        == "0"
        and _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(candidate.instructions[9]))
        in {"edx+96", "edx+0x60"}
        and _cc_cfg._instruction_operand(candidate.instructions[10]).strip()
        == _cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALLER_SYMBOL
    )
    ordinary_call_instruction_shape = (
        candidate_shape == "ordinary-call"
        and exact_private_labels
        and candidate_offsets
        == (
            0x00, 0x01, 0x03, 0x09, 0x0B, 0x0D, 0x0F, 0x11,
            0x14, 0x16, 0x18, 0x1A, 0x1D, 0x1F, 0x21, 0x26, 0x27,
        )
        and candidate_bytes
        == (
            ("56",), ("8b", "f1"),
            ("8b", "0d", "88", "47", "00", "00"),
            ("85", "f6"), ("74", "09"), ("8b", "01"),
            ("6a", "01"), ("ff", "50", "60"), ("eb", "07"),
            ("8b", "11"), ("6a", "00"), ("ff", "52", "60"),
            ("85", "f6"), ("75", "05"),
            ("e8", "00", "00", "00", "00"), ("5e",), ("c3",),
        )
        and tuple(_cc_cfg._instruction_mnemonic(row) for row in candidate.instructions)
        == (
            "push", "mov", "mov", "test", "je", "mov", "push",
            "call", "jmp", "mov", "push", "call", "test", "jne",
            "call", "pop", "ret",
        )
        and _cc_cfg._instruction_operand(candidate.instructions[0]).strip().lower()
        == "esi"
        and _cc_cfg._instruction_operand(candidate.instructions[1]).replace(" ", "").lower()
        == "esi,ecx"
        and _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(candidate.instructions[2]).split(",", 1)[-1]
        )
        in {
            expected_expression,
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}+0x4788",
        }
        and _cc_cfg._instruction_operand(candidate.instructions[3]).replace(" ", "").lower()
        == "esi,esi"
        and _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(candidate.instructions[5]).split(",", 1)[-1]
        )
        == "ecx"
        and _cc_cfg._instruction_operand(candidate.instructions[6]).strip().lower()
        == "1"
        and _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(candidate.instructions[7]))
        in {"eax+96", "eax+0x60"}
        and _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(candidate.instructions[9]).split(",", 1)[-1]
        )
        == "ecx"
        and _cc_cfg._instruction_operand(candidate.instructions[10]).strip().lower()
        == "0"
        and _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(candidate.instructions[11]))
        in {"edx+96", "edx+0x60"}
        and _cc_cfg._instruction_operand(candidate.instructions[12]).replace(" ", "").lower()
        == "esi,esi"
        and _cc_cfg._instruction_operand(candidate.instructions[14]).strip()
        == _cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALLER_SYMBOL
        and _cc_cfg._instruction_operand(candidate.instructions[15]).strip().lower()
        == "esi"
        and _cc_cfg._instruction_operand(candidate.instructions[16]).strip() in {"", "0"}
    )
    if not (tail_instruction_shape or ordinary_call_instruction_shape):
        raise ValueError(
            "HUD SetFloatTimerVisible bridge rejects candidate timer storage, "
            "true/false boolean arguments, receivers, vptrs, slots, return/"
            "successor form, or complete instruction topology drift"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    if candidate_shape == "ordinary-call":
        true_call_index = 7
        false_call_index = 11
        successor_call_index = 14
        true_definition_end = 7
        false_definition_end = 11
        expected_invocation_indices = (7, 11, 14)
        expected_true_ecx_definitions = [2]
        expected_true_eax_definitions = [5]
        expected_false_ecx_definitions = [2, 7]
        expected_false_edx_definitions = [7, 9]
        visible_esi_definitions = [
            index
            for index in range(0, 15)
            if _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index], "esi"
            )
        ]
        true_call_offset = (
            _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALL_TRUE_CALL_OFFSET
        )
        false_call_offset = (
            _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALL_FALSE_CALL_OFFSET
        )
    else:
        true_call_index = 5
        false_call_index = 9
        successor_call_index = 10
        true_definition_end = 5
        false_definition_end = 9
        expected_invocation_indices = (5, 9, 10)
        expected_true_ecx_definitions = [1]
        expected_true_eax_definitions = [3]
        expected_false_ecx_definitions = [1, 5]
        expected_false_edx_definitions = [5, 7]
        visible_esi_definitions = []
        true_call_offset = (
            _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_TAIL_TRUE_CALL_OFFSET
        )
        false_call_offset = (
            _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_TAIL_FALSE_CALL_OFFSET
        )
    true_ecx_definitions = [
        index
        for index in range(0, true_definition_end)
        if _cc_cfg._instruction_may_clobber_register(candidate.instructions[index], "ecx")
    ]
    true_eax_definitions = [
        index
        for index in range(0, true_definition_end)
        if _cc_cfg._instruction_may_clobber_register(candidate.instructions[index], "eax")
    ]
    false_ecx_definitions = [
        index
        for index in range(0, false_definition_end)
        if _cc_cfg._instruction_may_clobber_register(candidate.instructions[index], "ecx")
    ]
    false_edx_definitions = [
        index
        for index in range(0, false_definition_end)
        if _cc_cfg._instruction_may_clobber_register(candidate.instructions[index], "edx")
    ]
    if (
        invocation_indices != expected_invocation_indices
        or true_ecx_definitions != expected_true_ecx_definitions
        or true_eax_definitions != expected_true_eax_definitions
        or false_ecx_definitions != expected_false_ecx_definitions
        or false_edx_definitions != expected_false_edx_definitions
        or (
            candidate_shape == "ordinary-call"
            and visible_esi_definitions != [1]
        )
        or candidate.local_control_flow_indices != frozenset()
        or dict(candidate.local_control_flow_targets) != {}
        or _cc_cfg._cleanup_after(candidate.instructions, true_call_index) is not None
        or _cc_cfg._cleanup_after(candidate.instructions, false_call_index) is not None
        or _cc_cfg._cleanup_after(candidate.instructions, successor_call_index) is not None
    ):
        raise ValueError(
            "HUD SetFloatTimerVisible bridge rejects candidate call ordinals, "
            "receiver/vptr reaching definitions, cleanup, alternate entry, "
            "or conditional successor topology drift"
        )

    static_bridge = ReviewedStaticStorageReferenceBridge(
        aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
        displacement=_cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_DISPLACEMENT,
        access_width=4,
        storage_identity=aggregate_identity,
    )
    vptr_bridges = {
        normalize_address(
            hex(true_call_offset)
        ): ReviewedVptrStorageBridge(
            register="eax",
            provenance=f"load({aggregate_identity})",
            storage_identity=timer_storage,
            slot_displacement=(
                _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_SLOT_DISPLACEMENT
            ),
            identity_kind="virtual-slot",
        ),
        normalize_address(
            hex(false_call_offset)
        ): ReviewedVptrStorageBridge(
            register="edx",
            provenance=f"load({aggregate_identity})",
            storage_identity=timer_storage,
            slot_displacement=(
                _cc_catalog.HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_SLOT_DISPLACEMENT
            ),
            identity_kind="virtual-slot",
        ),
    }
    return (
        {expected_expression: static_bridge},
        vptr_bridges,
    )


def _hud_ui_mgr_set_aux_overlay_visible_bridges(
    expected: Sequence[Mapping[str, Any]],
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedStaticStorageReferenceBridge],
    dict[str, ReviewedVptrStorageBridge],
]:
    """Resolve only SetAuxOverlayVisible's exact two-path candidate shape."""
    from _recoil.call_contract.records import (
        ReviewedStaticStorageReferenceBridge,
        ReviewedVptrStorageBridge,
        StorageContainer,
    )
    caller_start = normalize_address(caller_start)
    if caller_start != _cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_CALLER_START:
        return {}, {}

    caller = candidate.caller_definition
    caller_addresses = sorted(
        address
        for address, identity in indexes.by_address.items()
        if identity == caller_identity
    )
    caller_name_rows = [
        (name, identity)
        for name, identity in indexes.by_candidate_name.items()
        if name.casefold()
        == _cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_CALLER_SYMBOL.casefold()
    ]
    if (
        caller_identity != _cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_CALLER_END_EXCLUSIVE
        or caller_addresses
        != [_cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_CALLER_START]
        or caller_identity in indexes.provider_ids
        or caller_name_rows
        not in (
            [],
            [
                (
                    _cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_CALLER_SYMBOL,
                    _cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_CALLER_IDENTITY,
                )
            ],
        )
        or caller is None
        or caller.symbol != _cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_CALLER_SYMBOL
        or len(caller.data) != 0x20
        or len(caller.relocation_mask) != len(caller.data)
    ):
        raise ValueError(
            "HUD SetAuxOverlayVisible bridge requires the exact reviewed "
            "authored caller identity, extent, symbol, and body"
        )

    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_CALLER_START
    ]
    if (
        target is None
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
            "HUD SetAuxOverlayVisible bridge requires one exact current HUD "
            "source authority"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "")
        != _cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_CALLER_SYMBOL
        or getattr(contribution_row, "symbol_regex", None) is not None
        or getattr(contribution_row, "name", "")
        != "HudUiMgr::SetAuxOverlayVisible"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD SetAuxOverlayVisible bridge requires the exact authored "
            "hud.cpp contribution row"
        )

    caller_symbol_id = caller_identity.removeprefix("symbol:")
    caller_symbol = document.collection("symbols").get(caller_symbol_id)
    trace = (
        caller_symbol.get("source_traceability")
        if isinstance(caller_symbol, Mapping)
        else None
    )
    source_edges = (
        trace.get("source_edges") if isinstance(trace, Mapping) else None
    )
    if (
        not isinstance(caller_symbol, Mapping)
        or _cc_identity._symbol_identity(caller_symbol_id, caller_symbol)
        != caller_identity
        or caller_symbol.get("binary") != "recoil"
        or caller_symbol.get("kind") != "function"
        or caller_symbol.get("pipeline_class") != "authored"
        or caller_symbol.get("ownership_state") != "primary-owned"
        or caller_symbol.get("extent_state") != "known"
        or normalize_address(str(caller_symbol.get("address", "")))
        != _cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_CALLER_START
        or normalize_address(str(caller_symbol.get("end_exclusive", "")))
        != _cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_CALLER_END_EXCLUSIVE
        or caller_symbol.get("size") != 0x20
        or caller_symbol.get("navigation_name")
        != "HudUiMgr::SetAuxOverlayVisible"
        or caller_symbol.get("output_section_id") != "recoil:section:.text"
        or caller_symbol.get("physical_block_id") != "recoil:block:0x404ca0"
        or not exact_required_target_membership(
            caller_symbol.get("verification_target_ids", ()),
            _cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_VERIFICATION_TARGET_IDS,
        )
        or caller_symbol.get("logical_identity_key") not in {None, ""}
        or caller_symbol.get("icf_fold_status") not in {None, ""}
        or bool(caller_symbol.get("logical_aliases"))
        or not isinstance(trace, Mapping)
        or trace.get("state") != "resolved"
        or trace.get("reason_code") not in {None, ""}
        or not isinstance(source_edges, list)
        or len(source_edges) != 1
        or not isinstance(source_edges[0], Mapping)
        or source_edges[0].get("relation") != "defines"
        or source_edges[0].get("anchor_id")
        != _cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
    ):
        raise ValueError(
            "HUD SetAuxOverlayVisible bridge requires one exact unaliased "
            "reviewed caller and resolved source edge"
        )

    verification_targets = document.collection("verification_targets")
    symbols = document.collection("symbols")
    storage_rows = document.collection("storage_contributions")
    aggregate_symbol = symbols.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID)
    aggregate_storage = storage_rows.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_STORAGE_ID)
    aggregate_target = verification_targets.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_ID)
    aggregate_registration = (
        aggregate_target.get("registration")
        if isinstance(aggregate_target, Mapping)
        else None
    )
    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    aggregate_containers = [
        row
        for row in indexes.storage_containers
        if row.identity == aggregate_identity
    ]
    if (
        not isinstance(aggregate_symbol, Mapping)
        or aggregate_symbol.get("binary") != "recoil"
        or aggregate_symbol.get("kind") != "data"
        or normalize_address(str(aggregate_symbol.get("address", "")))
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
        or aggregate_symbol.get("disposition") != "authored"
        or aggregate_symbol.get("navigation_name") != _cc_catalog.HUD_UI_MGR_AGGREGATE_NAME
        or aggregate_symbol.get("output_section_id") != "recoil:section:.data"
        or aggregate_symbol.get("storage_contribution_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_STORAGE_ID]
        or aggregate_symbol.get("verification_target_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_ID]
        or aggregate_symbol.get("extent_state") != "unknown"
        or not isinstance(aggregate_storage, Mapping)
        or aggregate_storage.get("binary") != "recoil"
        or aggregate_storage.get("kind") != "data-symbol"
        or aggregate_storage.get("output_section_id") != "recoil:section:.data"
        or aggregate_storage.get("overlap") != "none"
        or aggregate_storage.get("symbol_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID]
        or not isinstance(aggregate_storage.get("reference"), Mapping)
        or normalize_address(
            str(aggregate_storage["reference"].get("address", ""))
        )
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
        or not isinstance(aggregate_target, Mapping)
        or aggregate_target.get("binary") != "recoil"
        or aggregate_target.get("kind") != "vc5"
        or aggregate_target.get("name") != "hud_ui_mgr_data"
        or aggregate_target.get("symbol_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID]
        or aggregate_target.get("unresolved_addresses") not in (None, [])
        or not isinstance(aggregate_registration, Mapping)
        or aggregate_registration.get("manifest_path")
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_MANIFEST
        or aggregate_registration.get("data_addresses")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS]
        or indexes.storage_by_address.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
        != aggregate_identity
        or aggregate_containers
        != [
            StorageContainer(
                address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS),
                address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
                + _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE,
                aggregate_identity,
            )
        ]
        or _cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_DISPLACEMENT + 4
        > _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
    ):
        raise ValueError(
            "HUD SetAuxOverlayVisible bridge requires the exact HUD aggregate "
            "string-menu field, storage, target, and container authority"
        )

    expected_rows = [
        {
            "ordinal": ordinal,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": _cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_STORAGE_IDENTITY,
            "slot_displacement": (
                _cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_SLOT_DISPLACEMENT
            ),
            "cleanup_bytes": None,
        }
        for ordinal in (0, 1)
    ]
    if list(expected) != expected_rows:
        raise ValueError(
            "HUD SetAuxOverlayVisible bridge requires the exact immutable "
            "two-path SetEnabled retail contract"
        )

    retail_addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(caller_start),
    )
    retail_bytes = tuple(
        tuple(value.lower() for value in instruction.bytes)
        for instruction in retail_instructions
    )
    if (
        retail_addresses
        != (
            0x4137A0, 0x4137A2, 0x4137A8, 0x4137AA, 0x4137AC,
            0x4137AE, 0x4137B1, 0x4137B2, 0x4137B4, 0x4137B6,
            0x4137B9,
        )
        or retail_bytes
        != (
            ("85", "c9"),
            ("8b", "0d", "b0", "6d", "4e", "00"),
            ("74", "08"), ("8b", "01"), ("6a", "01"),
            ("ff", "50", "04"), ("c3",), ("8b", "11"),
            ("6a", "00"), ("ff", "52", "04"), ("c3",),
        )
        or tuple(_cc_cfg._instruction_mnemonic(row) for row in retail_instructions)
        != (
            "test", "mov", "je", "mov", "push", "call", "retn",
            "mov", "push", "call", "retn",
        )
        or _cc_cfg._instruction_operand(retail_instructions[0]).replace(" ", "").lower()
        != "ecx,ecx"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(retail_instructions[1]).split(",", 1)[-1]
        )
        != _cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_RETAIL_STORAGE_ADDRESS
        or _cc_cfg._instruction_operand(retail_instructions[2]).strip().lower()
        != "0x4137b2"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(retail_instructions[3]).split(",", 1)[-1]
        )
        != "ecx"
        or _cc_cfg._instruction_operand(retail_instructions[4]).strip().lower()
        not in {"1", "0x1"}
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(retail_instructions[5]))
        not in {"eax+4", "eax+0x4"}
        or _cc_cfg._instruction_operand(retail_instructions[6]).strip() not in {"", "0"}
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(retail_instructions[7]).split(",", 1)[-1]
        )
        != "ecx"
        or _cc_cfg._instruction_operand(retail_instructions[8]).strip().lower()
        not in {"0", "0x0"}
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(retail_instructions[9]))
        not in {"edx+4", "edx+0x4"}
        or _cc_cfg._instruction_operand(retail_instructions[10]).strip() not in {"", "0"}
        or _cc_cfg._cleanup_after(retail_instructions, 5) is not None
        or _cc_cfg._cleanup_after(retail_instructions, 9) is not None
    ):
        raise ValueError(
            "HUD SetAuxOverlayVisible bridge rejects immutable retail string-"
            "menu storage, boolean paths, receivers, slot, cleanup, return, "
            "or control-flow topology drift"
        )

    exact_candidate_body = bytes.fromhex(
        "85 c9 8b 0d e0 0e 00 00 74 08 8b 01 6a 01 ff 50 "
        "04 c3 8b 11 6a 00 ff 52 04 c3 90 90 90 90 90 90"
    )
    expected_relocations = (
        (
            _cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_RELOCATION_OFFSET,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_DISPLACEMENT,
        ),
    )
    observed_relocations = tuple(
        (
            row.offset,
            row.type,
            row.symbol_name,
            (
                struct.unpack_from("<I", caller.data, row.offset)[0]
                if row.offset + 4 <= len(caller.data)
                else None
            ),
        )
        for row in caller.relocations
    )
    expected_mask = {
        index
        for offset, _, _, _ in expected_relocations
        for index in range(offset, offset + 4)
    }
    if (
        caller.data != exact_candidate_body
        or observed_relocations != expected_relocations
        or {
            index
            for index, masked in enumerate(caller.relocation_mask)
            if masked
        }
        != expected_mask
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or caller.defined_external_functions.count(
            _cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_CALLER_SYMBOL
        )
        != 1
        or (
            caller.defined_external_data
            + caller.undefined_external_functions
            + caller.defined_external_functions
        ).count(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
        != 0
    ):
        raise ValueError(
            "HUD SetAuxOverlayVisible bridge requires the exact complete "
            "candidate body, aggregate COFF row/addend, relocation mask, "
            "padding, and external population"
        )

    candidate_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    candidate_bytes = tuple(
        tuple(value.lower() for value in instruction.bytes)
        for instruction in candidate.instructions
    )
    expected_expression = (
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
        f"+{_cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_DISPLACEMENT}"
    )
    if (
        candidate_offsets
        != (0x00, 0x02, 0x08, 0x0A, 0x0C, 0x0E, 0x11, 0x12, 0x14, 0x16, 0x19)
        or candidate_bytes
        != (
            ("85", "c9"),
            ("8b", "0d", "e0", "0e", "00", "00"),
            ("74", "08"), ("8b", "01"), ("6a", "01"),
            ("ff", "50", "04"), ("c3",), ("8b", "11"),
            ("6a", "00"), ("ff", "52", "04"), ("c3",),
        )
        or tuple(_cc_cfg._instruction_mnemonic(row) for row in candidate.instructions)
        != (
            "test", "mov", "je", "mov", "push", "call", "ret",
            "mov", "push", "call", "ret",
        )
        or _cc_cfg._instruction_operand(candidate.instructions[0]).replace(" ", "").lower()
        != "ecx,ecx"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(candidate.instructions[1]).split(",", 1)[-1]
        )
        not in {
            expected_expression,
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}+0xee0",
        }
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(candidate.instructions[3]).split(",", 1)[-1]
        )
        != "ecx"
        or _cc_cfg._instruction_operand(candidate.instructions[4]).strip().lower()
        not in {"1", "0x1"}
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(candidate.instructions[5]))
        not in {"eax+4", "eax+0x4"}
        or _cc_cfg._instruction_operand(candidate.instructions[6]).strip() not in {"", "0"}
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(candidate.instructions[7]).split(",", 1)[-1]
        )
        != "ecx"
        or _cc_cfg._instruction_operand(candidate.instructions[8]).strip().lower()
        not in {"0", "0x0"}
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(candidate.instructions[9]))
        not in {"edx+4", "edx+0x4"}
        or _cc_cfg._instruction_operand(candidate.instructions[10]).strip() not in {"", "0"}
    ):
        raise ValueError(
            "HUD SetAuxOverlayVisible bridge rejects candidate string-menu "
            "storage, true/false boolean arguments, receiver/vptr, slot, "
            "return, or complete two-path instruction topology drift"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    true_path_indices = (0, 1, 2, 3, 4)
    false_path_indices = (0, 1, 2, 7, 8)
    true_ecx_definitions = [
        index
        for index in true_path_indices
        if _cc_cfg._instruction_may_clobber_register(candidate.instructions[index], "ecx")
    ]
    true_eax_definitions = [
        index
        for index in true_path_indices
        if _cc_cfg._instruction_may_clobber_register(candidate.instructions[index], "eax")
    ]
    false_ecx_definitions = [
        index
        for index in false_path_indices
        if _cc_cfg._instruction_may_clobber_register(candidate.instructions[index], "ecx")
    ]
    false_edx_definitions = [
        index
        for index in false_path_indices
        if _cc_cfg._instruction_may_clobber_register(candidate.instructions[index], "edx")
    ]
    if (
        invocation_indices != (5, 9)
        or true_ecx_definitions != [1]
        or true_eax_definitions != [3]
        or false_ecx_definitions != [1]
        or false_edx_definitions != [7]
        or candidate.local_control_flow_indices != frozenset()
        or dict(candidate.local_control_flow_targets) != {}
        or _cc_cfg._cleanup_after(candidate.instructions, 5) is not None
        or _cc_cfg._cleanup_after(candidate.instructions, 9) is not None
    ):
        raise ValueError(
            "HUD SetAuxOverlayVisible bridge rejects candidate call population, "
            "true/false receiver/vptr reaching definitions, cleanup, or "
            "alternate-entry topology drift"
        )

    static_bridge = ReviewedStaticStorageReferenceBridge(
        aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
        displacement=_cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_DISPLACEMENT,
        access_width=4,
        storage_identity=aggregate_identity,
    )
    def vptr_bridge(register: str) -> ReviewedVptrStorageBridge:
        return ReviewedVptrStorageBridge(
            register=register,
            provenance=f"load({aggregate_identity})",
            storage_identity=_cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_STORAGE_IDENTITY,
            slot_displacement=(
                _cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_SLOT_DISPLACEMENT
            ),
            identity_kind="virtual-slot",
        )
    return (
        {expected_expression: static_bridge},
        {
            normalize_address(
                hex(_cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_TRUE_CALL_OFFSET)
            ): vptr_bridge("eax"),
            normalize_address(
                hex(_cc_catalog.HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_FALSE_CALL_OFFSET)
            ): vptr_bridge("edx"),
        },
    )


def _hud_ui_mgr_stats_list_set_visible_retail_guard(
    instructions: Sequence[Instruction],
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> None:
    """Fail closed unless retail has the reviewed stats-list slot call."""
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_CALLER_START:
        return
    if (
        caller_identity != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_CALLER_END_EXCLUSIVE
        or indexes.storage_by_address.get(_cc_catalog.HUD_UI_MGR_STATS_LIST_ADDRESS)
        != _cc_catalog.HUD_UI_MGR_STATS_LIST_IDENTITY
        or [
            address
            for address, identity in indexes.storage_by_address.items()
            if identity == _cc_catalog.HUD_UI_MGR_STATS_LIST_IDENTITY
        ]
        != [_cc_catalog.HUD_UI_MGR_STATS_LIST_ADDRESS]
        or _cc_catalog.HUD_UI_MGR_STATS_LIST_IDENTITY in indexes.provider_ids
    ):
        raise ValueError(
            "HUD stats-list SetVisible retail guard requires the exact "
            "reviewed caller, extent, and unique authored storage identity"
        )

    start = address_value(normalized_start)
    addresses = _cc_cfg._instruction_runtime_addresses(
        instructions,
        source="bn",
        caller_start=start,
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
    exact_rows = {
        _cc_catalog.HUD_UI_MGR_STATS_LIST_RETAIL_LOAD_ADDRESS: (
            b"\x8b\x0d\xe0\xd4\x4e\x00",
            r"mov\s+ecx\s*,\s*(?:dword\s+)?\[0x4ed4e0\]",
        ),
        _cc_catalog.HUD_UI_MGR_STATS_LIST_RETAIL_FLAG_ADDRESS: (
            b"\x89\x3d\xe0\x5e\x4e\x00",
            r"mov\s+(?:dword\s+)?\[0x4e5ee0\]\s*,\s*edi",
        ),
        _cc_catalog.HUD_UI_MGR_STATS_LIST_RETAIL_ARGUMENT_ADDRESS: (
            b"\x57",
            r"push\s+edi",
        ),
        _cc_catalog.HUD_UI_MGR_STATS_LIST_RETAIL_VPTR_LOAD_ADDRESS: (
            b"\x8b\x11",
            r"mov\s+edx\s*,\s*(?:dword\s+)?\[ecx\]",
        ),
        _cc_catalog.HUD_UI_MGR_STATS_LIST_RETAIL_CALL_ADDRESS: (
            b"\xff\x52\x60",
            r"call\s+(?:dword\s+)?\[edx\+0x60\]",
        ),
    }
    if (
        any(counts.get(address) != 1 for address in exact_rows)
        or not set(exact_rows).issubset(by_address)
    ):
        raise ValueError(
            "HUD stats-list SetVisible retail guard requires one unique "
            "instruction at every reviewed address"
        )
    for address, (body, pattern) in exact_rows.items():
        instruction = by_address[address]
        if (
            bytes(int(value, 16) for value in instruction.bytes) != body
            or re.fullmatch(
                pattern,
                instruction.raw_text.strip(),
                flags=re.IGNORECASE,
            )
            is None
        ):
            raise ValueError(
                "HUD stats-list SetVisible retail guard requires exact "
                f"instruction bytes and operands at {hex(address)}"
            )
    edi_writes = [
        (address, instruction)
        for address, instruction in by_address.items()
        if (
            address < _cc_catalog.HUD_UI_MGR_STATS_LIST_RETAIL_ARGUMENT_ADDRESS
            and _cc_cfg._instruction_may_clobber_register(instruction, "edi")
        )
    ]
    if (
        not edi_writes
        or edi_writes[-1][0]
        != _cc_catalog.HUD_UI_MGR_STATS_LIST_RETAIL_ARGUMENT_SEED_ADDRESS
        or bytes(
            int(value, 16) for value in edi_writes[-1][1].bytes
        )
        != b"\xbf\x01\x00\x00\x00"
        or re.fullmatch(
            r"mov\s+edi\s*,\s*(?:0x)?1",
            edi_writes[-1][1].raw_text.strip(),
            flags=re.IGNORECASE,
        )
        is None
    ):
        raise ValueError(
            "HUD stats-list SetVisible retail guard requires the exact "
            "unclobbered EDI argument value 1"
        )
    window = [
        (address, instruction)
        for address, instruction in by_address.items()
        if (
            _cc_catalog.HUD_UI_MGR_STATS_LIST_RETAIL_LOAD_ADDRESS
            <= address
            <= _cc_catalog.HUD_UI_MGR_STATS_LIST_RETAIL_CALL_ADDRESS
        )
    ]
    if (
        sum(
            1
            for _, instruction in window
            if (
                _cc_cfg._instruction_mnemonic(instruction) == "call"
                and _cc_targets._memory_slot(_cc_cfg._instruction_operand(instruction))[1]
                == _cc_catalog.HUD_UI_MGR_STATS_LIST_VPTR_SLOT_DISPLACEMENT
            )
        )
        != 1
        or any(
            _cc_cfg._instruction_mnemonic(instruction).startswith("j")
            for address, instruction in window
            if address != _cc_catalog.HUD_UI_MGR_STATS_LIST_RETAIL_CALL_ADDRESS
        )
        or any(
            _cc_cfg._instruction_may_clobber_register(instruction, "ecx")
            for address, instruction in window
            if (
                _cc_catalog.HUD_UI_MGR_STATS_LIST_RETAIL_LOAD_ADDRESS
                < address
                <= _cc_catalog.HUD_UI_MGR_STATS_LIST_RETAIL_VPTR_LOAD_ADDRESS
            )
        )
        or _cc_cfg._cleanup_after(
            instructions,
            addresses.index(_cc_catalog.HUD_UI_MGR_STATS_LIST_RETAIL_CALL_ADDRESS),
        )
        is not None
    ):
        raise ValueError(
            "HUD stats-list SetVisible retail guard rejects alternate calls, "
            "CFG, cleanup, or receiver clobbers"
        )


def _hud_ui_mgr_stats_list_set_visible_candidate_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedStaticStorageReferenceBridge],
    dict[str, ReviewedMemberVptrStorageBridge],
]:
    """Prove one aggregate-field load feeding the final stats-list slot call."""
    from _recoil.call_contract.records import (
        ReviewedMemberVptrStorageBridge,
        ReviewedStaticStorageReferenceBridge,
    )
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_CALLER_START:
        return {}, {}
    if (
        caller_identity != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_CALLER_END_EXCLUSIVE
    ):
        raise ValueError(
            "HUD stats-list SetVisible candidate bridge requires the exact "
            "reviewed InitHudLayouts caller and extent"
        )

    definition = candidate.caller_definition
    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_CALLER_START
    ]
    if (
        definition is None
        or definition.symbol != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_CALLER_SYMBOL
        or len(definition.data) != len(definition.relocation_mask)
        or not definition.data
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
        or len(contribution_rows) != 1
    ):
        raise ValueError(
            "HUD stats-list SetVisible candidate bridge requires the exact "
            "registered HUD target, caller definition, and contribution"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_TARGET_SYMBOL_REGEX
        or re.fullmatch(
            _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_TARGET_SYMBOL_REGEX,
            definition.symbol,
        )
        is None
        or getattr(contribution_row, "name", "")
        != "HudUiMgr::InitHudLayouts"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD stats-list SetVisible candidate bridge requires the exact "
            "authored InitHudLayouts hud.cpp contribution identity"
        )

    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    expected_storage = f"load({_cc_catalog.HUD_UI_MGR_STATS_LIST_IDENTITY})"
    if (
        indexes.storage_by_address.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
        != aggregate_identity
        or indexes.storage_by_address.get(_cc_catalog.HUD_UI_MGR_STATS_LIST_ADDRESS)
        != _cc_catalog.HUD_UI_MGR_STATS_LIST_IDENTITY
        or [
            address
            for address, identity in indexes.storage_by_address.items()
            if identity == _cc_catalog.HUD_UI_MGR_STATS_LIST_IDENTITY
        ]
        != [_cc_catalog.HUD_UI_MGR_STATS_LIST_ADDRESS]
        or _cc_catalog.HUD_UI_MGR_STATS_LIST_IDENTITY in indexes.provider_ids
    ):
        raise ValueError(
            "HUD stats-list SetVisible candidate bridge rejects missing, "
            "aliased, or provider aggregate/field storage"
        )
    expected_rows = [
        row
        for row in expected
        if (
            row.get("identity_kind") == "virtual-slot"
            and row.get("storage_identity") == expected_storage
            and row.get("slot_displacement")
            == _cc_catalog.HUD_UI_MGR_STATS_LIST_VPTR_SLOT_DISPLACEMENT
        )
    ]
    if (
        len(expected_rows) != 1
        or any(
            expected_rows[0].get(key) != value
            for key, value in {
                "form": "call",
                "dispatch": "indirect",
                "target_identity": "",
                "cleanup_bytes": None,
            }.items()
        )
    ):
        raise ValueError(
            "HUD stats-list SetVisible candidate bridge requires one exact "
            "retail virtual-slot contract"
        )

    start = address_value(normalized_start)
    instructions = candidate.instructions
    addresses = _cc_cfg._instruction_runtime_addresses(
        instructions, source="cod", caller_start=start,
    )
    proven = _cc_receiver_candidate._candidate_aggregate_field_vptr_calls(
        instructions, addresses=addresses, caller_start=start, definition=definition,
        aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
        displacement=_cc_catalog.HUD_UI_MGR_STATS_LIST_DISPLACEMENT,
        slot_displacement=_cc_catalog.HUD_UI_MGR_STATS_LIST_VPTR_SLOT_DISPLACEMENT,
    )
    if len(proven) != 1:
        raise ValueError(
            "HUD stats-list SetVisible candidate bridge requires one exact "
            "COFF field receiver and vptr on every CFG path to the slot call"
        )
    vptr_index, call_index, vptr_register = proven[0]
    vptr_load_offset = addresses[vptr_index] - start
    call_offset = addresses[call_index] - start

    expression = (
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
        f"+{_cc_catalog.HUD_UI_MGR_STATS_LIST_DISPLACEMENT}"
    )
    return (
        {
            expression: ReviewedStaticStorageReferenceBridge(
                aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                displacement=_cc_catalog.HUD_UI_MGR_STATS_LIST_DISPLACEMENT,
                access_width=_cc_catalog.HUD_UI_MGR_STATS_LIST_ACCESS_WIDTH,
                storage_identity=_cc_catalog.HUD_UI_MGR_STATS_LIST_IDENTITY,
            )
        },
        {
            normalize_address(vptr_load_offset): ReviewedMemberVptrStorageBridge(
                register=vptr_register,
                source_register="ecx",
                source_provenance=_cc_catalog.HUD_UI_MGR_STATS_LIST_IDENTITY,
                receiver_register="ecx",
                receiver_provenance=_cc_catalog.HUD_UI_MGR_STATS_LIST_IDENTITY,
                storage_identity=expected_storage,
                slot_displacement=(
                    _cc_catalog.HUD_UI_MGR_STATS_LIST_VPTR_SLOT_DISPLACEMENT
                ),
                call_address=normalize_address(call_offset),
            )
        },
    )
