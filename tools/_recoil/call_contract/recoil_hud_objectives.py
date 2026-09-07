"""Recoil call-contract recoil hud objectives evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import extraction as _cc_extraction
from _recoil.call_contract import instructions as _cc_instructions
from _recoil.call_contract import receiver_instructions as _cc_receiver_instructions
from _recoil.call_contract import recoil_hud_sensor as _cc_recoil_hud_sensor
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        IdentityIndexes,
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedLoopVptrStorageBridge,
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
    IMAGE_SYM_CLASS_STATIC,
    CoffRelocation,
    Instruction,
)
from _recoil.lib.authored_icf import exact_selected_target_membership
from _recoil.lib.progress import ProgressDocument, address_value, normalize_address


def _hud_ui_mgr_objective_refresh_counter_panel_storage_bridges(
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
    dict[str, ReviewedLoopVptrStorageBridge],
]:
    """Prove RefreshCounterText's aggregate-owned panel virtual call."""
    from _recoil.call_contract.records import (
        ReviewedMemberVptrStorageBridge,
        ReviewedStaticStorageReferenceBridge,
    )
    normalized_start = normalize_address(caller_start)
    if (
        normalized_start
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_CALLER_START
    ):
        return {}, {}

    caller_symbol_id = (
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_CALLER_IDENTITY.removeprefix(
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
        == _cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_CALLER_START
    ]
    if (
        caller_identity
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(normalized_start) != caller_identity
        or caller_identity in indexes.provider_ids
        or not isinstance(caller_row, Mapping)
        or caller_row.get("binary") != "recoil"
        or caller_row.get("kind") != "function"
        or caller_row.get("pipeline_class") != "authored"
        or caller_row.get("ownership_state") != "primary-owned"
        or caller_row.get("address") != normalized_start
        or caller_row.get("end_exclusive")
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_CALLER_END_EXCLUSIVE
        or caller_row.get("extent_state") != "known"
        or caller_row.get("size") != 0x20
        or caller_row.get("navigation_name")
        != "HudUiMgrObjective::RefreshCounterText"
        or caller_row.get("output_section_id") != "recoil:section:.text"
        or caller_row.get("physical_block_id")
        != "recoil:block:0x404ca0"
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
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
        or caller is None
        or caller.symbol
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_CALLER_SYMBOL
        or len(caller.data) != 0x20
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
            "HUD objective counter-panel bridge requires the exact authored "
            "caller, source edge, symbol, extent, and candidate contribution "
            "authority"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != r"\?RefreshCounterText@HudUiMgrObjective@@.*"
        or getattr(contribution_row, "name", "")
        != "HudUiMgrObjective::RefreshCounterText"
        or getattr(contribution_row, "pipeline_class", "")
        != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(
            getattr(contribution_row, "required_presence", False)
        )
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD objective counter-panel bridge requires the exact authored "
            "hud.cpp contribution row"
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
        and relationship.get("symbol_id")
        == _cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID
    ]
    reference = (
        aggregate_storage.get("reference")
        if isinstance(aggregate_storage, Mapping)
        else None
    )
    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    aggregate_start = address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
    field_address = normalize_address(
        aggregate_start
        + _cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_DISPLACEMENT
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
        or aggregate_symbol.get("address")
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
        or aggregate_symbol.get("disposition") != "authored"
        or aggregate_symbol.get("navigation_name")
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_NAME
        or aggregate_symbol.get("output_section_id")
        != "recoil:section:.data"
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
        or indexes.storage_by_address.get(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
        )
        != aggregate_identity
        or aggregate_addresses != [_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS]
        or len(exact_containers) != 1
        or aggregate_identity in indexes.provider_ids
        or field_address
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_ADDRESS
        or indexes.storage_by_address.get(field_address, "") != ""
        or (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_DISPLACEMENT
            + _cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_ACCESS_WIDTH
            > _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
        )
    ):
        raise ValueError(
            "HUD objective counter-panel bridge requires the exact reviewed "
            "aggregate symbol, storage, target, owner, and bounded "
            "uncatalogued +0xbc8 field authority"
        )

    field_provenance = _cc_receiver_instructions._abstract_with_displacement(
        aggregate_identity,
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_DISPLACEMENT,
    )
    receiver_provenance = f"load({field_provenance})"
    expected_storage = f"load({receiver_provenance})"
    expected_contract = [
        {
            "ordinal": 0,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": expected_storage,
            "slot_displacement":
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_SLOT,
            "cleanup_bytes":
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_CLEANUP_BYTES,
        },
        {
            "ordinal": 1,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": expected_storage,
            "slot_displacement":
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_NEXT_SLOT,
            "cleanup_bytes": None,
        },
    ]
    if list(expected) != expected_contract:
        raise ValueError(
            "HUD objective counter-panel bridge requires the exact retail "
            "slot-0x74/cleanup-12 then slot-0x78/no-cleanup call order"
        )

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
    retail_fixed = {
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_RETAIL_LOAD: (
            "mov",
            b"\xa1\x98\x6a\x4e\x00",
        ),
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_RETAIL_VPTR: (
            "mov",
            b"\x8b\x10",
        ),
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_RETAIL_RECEIVER: (
            "push",
            b"\x50",
        ),
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_RETAIL_RECEIVER_SAVE: (
            "mov",
            b"\x8b\xf0",
        ),
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_RETAIL_CALL: (
            "call",
            b"\xff\x52\x74",
        ),
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_RETAIL_NEXT_VPTR: (
            "mov",
            b"\x8b\x06",
        ),
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_RETAIL_CLEANUP: (
            "add",
            b"\x83\xc4\x0c",
        ),
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_RETAIL_NEXT_RECEIVER: (
            "mov",
            b"\x8b\xce",
        ),
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_RETAIL_NEXT_CALL: (
            "call",
            b"\xff\x50\x78",
        ),
    }
    if any(
        address not in retail_by_address
        or _cc_cfg._instruction_mnemonic(retail_by_address[address]) != mnemonic
        or bytes(
            int(value, 16)
            for value in retail_by_address[address].bytes
        )
        != body
        for address, (mnemonic, body) in retail_fixed.items()
    ):
        raise ValueError(
            "HUD objective counter-panel bridge requires the exact retail "
            "global-load/receiver/vptr/slot/cleanup call unit"
        )
    retail_call_index = retail_instructions.index(
        retail_by_address[
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_RETAIL_CALL
        ]
    )
    retail_next_call_index = retail_instructions.index(
        retail_by_address[
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_RETAIL_NEXT_CALL
        ]
    )
    retail_invocations = [
        index
        for index, instruction in enumerate(retail_instructions)
        if _cc_cfg._instruction_mnemonic(instruction) == "call"
    ]
    if (
        retail_invocations != [retail_call_index, retail_next_call_index]
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(
                retail_instructions[retail_call_index]
            )
        )
        not in {"edx+116", "edx+0x74"}
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(
                retail_instructions[retail_next_call_index]
            )
        )
        not in {"eax+120", "eax+0x78"}
        or _cc_cfg._cleanup_after(retail_instructions, retail_call_index)
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_CLEANUP_BYTES
        or _cc_cfg._cleanup_after(retail_instructions, retail_next_call_index)
        is not None
    ):
        raise ValueError(
            "HUD objective counter-panel bridge rejects retail invocation "
            "order, receiver/vptr slot, or cleanup drift"
        )

    aggregate_relocations = [
        relocation
        for relocation in caller.relocations
        if relocation.symbol_name
        == _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
    ]
    if (
        len(aggregate_relocations) != 1
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in caller.defined_external_data
    ):
        raise ValueError(
            "HUD objective counter-panel bridge requires one exact "
            "candidate aggregate+0xbc8 relocation and one undefined "
            "aggregate symbol"
        )
    aggregate_relocation = aggregate_relocations[0]
    if (
        aggregate_relocation.type != IMAGE_REL_I386_DIR32
        or aggregate_relocation.offset + 4 > len(caller.data)
        or struct.unpack_from(
            "<I",
            caller.data,
            aggregate_relocation.offset,
        )[0]
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_DISPLACEMENT
        or not all(
            caller.relocation_mask[position]
            for position in range(
                aggregate_relocation.offset,
                aggregate_relocation.offset + 4,
            )
        )
    ):
        raise ValueError(
            "HUD objective counter-panel bridge requires exact DIR32 "
            "aggregate+0xbc8 addend and complete relocation masking"
        )

    candidate_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)

    def encoded_body(index: int) -> bytes:
        return bytes(
            int(value, 16)
            for value in candidate.instructions[index].bytes
        )

    def candidate_body_matches(
        index: int,
        allowed_relocations: Sequence[CoffRelocation] = (),
    ) -> bool:
        offset = candidate_offsets[index]
        if offset is None:
            return False
        try:
            encoded = encoded_body(index)
        except (TypeError, ValueError):
            return False
        allowed_mask = {
            position
            for relocation in allowed_relocations
            for position in range(
                relocation.offset,
                relocation.offset + 4,
            )
        }
        return (
            offset + len(encoded) <= len(caller.data)
            and caller.data[offset : offset + len(encoded)] == encoded
            and {
                position
                for position in range(offset, offset + len(encoded))
                if caller.relocation_mask[position]
            }
            == allowed_mask
        )

    relocation_owners = [
        index
        for index, offset in enumerate(candidate_offsets)
        if (
            offset is not None
            and offset <= aggregate_relocation.offset
            < offset + len(candidate.instructions[index].bytes)
        )
    ]
    if len(relocation_owners) != 1:
        raise ValueError(
            "HUD objective counter-panel bridge requires one unique "
            "instruction owner for the aggregate+0xbc8 relocation"
        )
    load_index = relocation_owners[0]
    load_offset = candidate_offsets[load_index]
    assert load_offset is not None
    load_instruction = candidate.instructions[load_index]
    load_operands = [
        item.strip()
        for item in _cc_cfg._instruction_operand(load_instruction).split(",", 1)
    ]
    candidate_expression = (
        _cc_targets._exact_memory_expression(load_operands[1])
        if len(load_operands) == 2
        else ""
    )
    expected_expressions = {
        (
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
            f"+{_cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_DISPLACEMENT}"
        ),
        (
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
            f"+0x{_cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_DISPLACEMENT:x}"
        ),
    }
    if (
        _cc_cfg._instruction_mnemonic(load_instruction) != "mov"
        or len(load_operands) != 2
        or load_operands[0].lower() != "esi"
        or candidate_expression not in expected_expressions
        or encoded_body(load_index) != b"\x8b\x35\xc8\x0b\x00\x00"
        or aggregate_relocation.offset != load_offset + 2
        or not candidate_body_matches(
            load_index,
            (aggregate_relocation,),
        )
    ):
        raise ValueError(
            "HUD objective counter-panel bridge requires the exact "
            "relocation-owned aggregate+0xbc8 panel-pointer load into ESI"
        )
    candidate_pointer_reference_indices = {
        index
        for index, instruction in enumerate(candidate.instructions)
        if (
            _cc_cfg._instruction_mnemonic(instruction) == "mov"
            and len(_cc_cfg._instruction_operand(instruction).split(",", 1)) == 2
            and _cc_targets._exact_memory_expression(
                _cc_cfg._instruction_operand(instruction).split(",", 1)[1]
            )
            in expected_expressions
        )
    }
    if candidate_pointer_reference_indices != {load_index}:
        raise ValueError(
            "HUD objective counter-panel bridge requires the unique "
            "candidate aggregate+0xbc8 panel-pointer load to own the exact "
            "relocation"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=(
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_CALLER_END_EXCLUSIVE
        ),
    )
    ordinal_by_index = {
        instruction_index: ordinal
        for ordinal, instruction_index in enumerate(invocation_indices)
    }
    chains: list[tuple[int, int, int, int]] = []
    for vptr_index in range(load_index + 1, len(candidate.instructions)):
        vptr_operands = [
            item.strip()
            for item in _cc_cfg._instruction_operand(
                candidate.instructions[vptr_index]
            ).split(",", 1)
        ]
        if (
            _cc_cfg._instruction_mnemonic(candidate.instructions[vptr_index])
            != "mov"
            or len(vptr_operands) != 2
            or vptr_operands[0].lower() != "eax"
            or _cc_targets._exact_memory_expression(vptr_operands[1]) != "esi"
            or encoded_body(vptr_index) != b"\x8b\x06"
            or not candidate_body_matches(vptr_index)
            or any(
                _cc_cfg._instruction_may_clobber_register(
                    candidate.instructions[between],
                    "esi",
                )
                for between in range(load_index + 1, vptr_index)
            )
        ):
            continue
        receiver_index = vptr_index + 1
        call_index = receiver_index + 1
        if call_index >= len(candidate.instructions):
            continue
        receiver_instruction = candidate.instructions[receiver_index]
        call_instruction = candidate.instructions[call_index]
        if (
            _cc_cfg._instruction_mnemonic(receiver_instruction) != "push"
            or _cc_cfg._instruction_operand(receiver_instruction).strip().lower()
            != "esi"
            or encoded_body(receiver_index) != b"\x56"
            or not candidate_body_matches(receiver_index)
            or _cc_cfg._instruction_mnemonic(call_instruction) != "call"
            or _cc_targets._exact_memory_expression(
                _cc_cfg._instruction_operand(call_instruction)
            )
            not in {"eax+116", "eax+0x74"}
            or encoded_body(call_index) != b"\xff\x50\x74"
            or not candidate_body_matches(call_index)
            or any(
                _cc_cfg._instruction_may_clobber_register(
                    candidate.instructions[between],
                    register,
                )
                for between in range(vptr_index + 1, call_index)
                for register in ("eax", "esi")
            )
            or ordinal_by_index.get(call_index) != 0
            or call_index in candidate.local_control_flow_indices
            or _cc_cfg._cleanup_after(candidate.instructions, call_index)
            != _cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_CLEANUP_BYTES
        ):
            continue
        cleanup_index = next(
            (
                index
                for index in range(
                    call_index + 1,
                    len(candidate.instructions),
                )
                if _cc_catalog.STACK_CLEANUP_RE.fullmatch(
                    candidate.instructions[index].raw_text.strip().lower()
                )
            ),
            -1,
        )
        if (
            cleanup_index < 0
            or encoded_body(cleanup_index) != b"\x83\xc4\x0c"
            or not candidate_body_matches(cleanup_index)
        ):
            continue
        chains.append(
            (
                vptr_index,
                receiver_index,
                call_index,
                cleanup_index,
            )
        )
    if len(chains) != 1:
        raise ValueError(
            "HUD objective counter-panel bridge requires one unique "
            "connected candidate ESI-receiver/EAX-vptr/slot-0x74/"
            "cleanup-12 chain"
        )

    vptr_index, receiver_index, call_index, cleanup_index = chains[0]
    vptr_offset = candidate_offsets[vptr_index]
    call_offset = candidate_offsets[call_index]
    assert vptr_offset is not None
    assert call_offset is not None
    next_vptr_index = call_index + 1
    next_receiver_index = cleanup_index + 1
    next_call_index = cleanup_index + 2
    if (
        next_call_index >= len(candidate.instructions)
        or _cc_cfg._instruction_mnemonic(
            candidate.instructions[next_vptr_index]
        )
        != "mov"
        or _cc_cfg._instruction_operand(
            candidate.instructions[next_vptr_index]
        ).strip().lower()
        not in {
            "edx, dword [esi]",
            "edx, dword ptr [esi]",
        }
        or encoded_body(next_vptr_index) != b"\x8b\x16"
        or not candidate_body_matches(next_vptr_index)
        or _cc_cfg._instruction_mnemonic(
            candidate.instructions[next_receiver_index]
        )
        != "mov"
        or _cc_cfg._instruction_operand(
            candidate.instructions[next_receiver_index]
        ).strip().lower()
        != "ecx, esi"
        or encoded_body(next_receiver_index) != b"\x8b\xce"
        or not candidate_body_matches(next_receiver_index)
        or _cc_cfg._instruction_mnemonic(
            candidate.instructions[next_call_index]
        )
        != "call"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(
                candidate.instructions[next_call_index]
            )
        )
        not in {"edx+120", "edx+0x78"}
        or encoded_body(next_call_index) != b"\xff\x52\x78"
        or not candidate_body_matches(next_call_index)
        or ordinal_by_index.get(next_call_index) != 1
        or next_call_index in candidate.local_control_flow_indices
        or _cc_cfg._cleanup_after(candidate.instructions, next_call_index)
        is not None
    ):
        raise ValueError(
            "HUD objective counter-panel bridge requires the exact connected "
            "candidate EDX-vptr/ECX-receiver/slot-0x78 successor call"
        )
    next_vptr_offset = candidate_offsets[next_vptr_index]
    next_call_offset = candidate_offsets[next_call_index]
    assert next_vptr_offset is not None
    assert next_call_offset is not None

    candidate_bridge = ReviewedStaticStorageReferenceBridge(
        aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
        displacement=_cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_DISPLACEMENT,
        access_width=_cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_ACCESS_WIDTH,
        storage_identity=field_provenance,
    )
    first_member_bridge = ReviewedMemberVptrStorageBridge(
        register="eax",
        source_register="esi",
        source_provenance=field_provenance,
        receiver_register="esi",
        receiver_provenance=field_provenance,
        storage_identity=expected_storage,
        slot_displacement=_cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_SLOT,
        call_address=normalize_address(hex(call_offset)),
    )
    next_member_bridge = ReviewedMemberVptrStorageBridge(
        register="edx",
        source_register="esi",
        source_provenance=field_provenance,
        receiver_register="ecx",
        receiver_provenance=field_provenance,
        storage_identity=expected_storage,
        slot_displacement=_cc_catalog.HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_NEXT_SLOT,
        call_address=normalize_address(hex(next_call_offset)),
    )
    structural_contract = _cc_extraction.extract_invocation_contract(
        tuple(
            candidate.instructions[index]
            for index in (
                load_index,
                vptr_index,
                receiver_index,
                call_index,
                cleanup_index,
            )
        ),
        source="cod",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        reviewed_static_storage_reference_bridges={
            candidate_expression: candidate_bridge
        },
        reviewed_member_vptr_storage_bridges={
            normalize_address(hex(vptr_offset)): first_member_bridge
        },
    )
    if structural_contract != [expected_contract[0]]:
        raise ValueError(
            "HUD objective counter-panel bridge cannot derive the exact "
            "candidate aggregate-panel virtual-call provenance"
        )
    return (
        {candidate_expression: candidate_bridge},
        {
            normalize_address(hex(vptr_offset)): first_member_bridge,
            normalize_address(hex(next_vptr_offset)): next_member_bridge,
        },
    )


def _hud_ui_mgr_objective_update_meter_x_candidate_vptr_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedAbsoluteStorageLoadBridge],
    dict[str, ReviewedVptrStorageBridge],
]:
    """Bridge only 0x4118b0's exact embedded-widget slot-0x64 call."""
    from _recoil.call_contract.records import (
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedVptrStorageBridge,
    )
    normalized_start = normalize_address(caller_start)
    if (
        normalized_start
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_CALLER_START
    ):
        return {}, {}

    caller_symbol_id = (
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_CALLER_IDENTITY.removeprefix(
            "symbol:"
        )
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
        caller_identity
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_CALLER_END_EXCLUSIVE
        or caller_addresses
        != [_cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_CALLER_START]
        or caller_identity in indexes.provider_ids
        or not isinstance(caller_symbol, Mapping)
        or caller_symbol.get("binary") != "recoil"
        or caller_symbol.get("kind") != "function"
        or caller_symbol.get("pipeline_class") != "authored"
        or caller_symbol.get("ownership_state") != "primary-owned"
        or caller_symbol.get("address")
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_CALLER_START
        or caller_symbol.get("end_exclusive")
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_CALLER_END_EXCLUSIVE
        or caller_symbol.get("extent_state") != "known"
        or caller_symbol.get("size") != 0x50
        or caller_symbol.get("navigation_name")
        != "HudUiMgrObjective::UpdateMeterXPoints"
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
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_CALLER_ANCHOR_ID
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
            "HUD objective UpdateMeterXPoints vptr bridge requires the exact "
            "authored caller/source and aggregate storage authority"
        )

    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_CALLER_START
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
            "HUD objective UpdateMeterXPoints vptr bridge requires one exact "
            "current HUD listing/source authority"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != r"\?UpdateMeterXPoints@HudUiMgrObjective@@.*"
        or re.fullmatch(
            r"\?UpdateMeterXPoints@HudUiMgrObjective@@.*",
            str(getattr(candidate.caller_definition, "symbol", "")),
        )
        is None
        or getattr(contribution_row, "name", "")
        != "HudUiMgrObjective::UpdateMeterXPoints"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD objective UpdateMeterXPoints vptr bridge requires the exact "
            "authored hud.cpp contribution row"
        )

    widget_storage = _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_STORAGE_IDENTITY
    exact_expected = [
        {
            "ordinal": 0,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": widget_storage,
            "slot_displacement": (
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_SLOT_DISPLACEMENT
            ),
            "cleanup_bytes": None,
        }
    ]
    if list(expected) != exact_expected:
        raise ValueError(
            "HUD objective UpdateMeterXPoints vptr bridge requires the exact "
            "immutable retail ordinal-0 widget storage/slot contract"
        )

    caller = candidate.caller_definition
    exact_body = bytes.fromhex(
        "51 "
        "a1 ac 06 00 00 "
        "b9 ac 06 00 00 "
        "ff 50 64 "
        "89 44 24 00 "
        "db 44 24 00 "
        "d8 25 00 00 00 00 "
        "d9 c0 "
        "d8 25 00 00 00 00 "
        "d9 c1 "
        "d9 1d 60 08 00 00 "
        "d9 c9 "
        "d9 c1 "
        "d9 c9 "
        "d9 1d 6c 08 00 00 "
        "d9 1d 78 08 00 00 "
        "d9 1d 84 08 00 00 "
        "59 c3 "
    "90 90 90 90 90 90 90 90 90 90"
)
    expected_offsets = (0x02, 0x07, 0x18, 0x20, 0x28, 0x34, 0x3A, 0x40)
    aggregate_addends = {
        0x02: _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_DISPLACEMENT,
        0x07: _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_DISPLACEMENT,
        0x28: 0x860,
        0x34: 0x86C,
        0x3A: 0x878,
        0x40: 0x884,
    }
    relocations = (
        tuple(caller.relocations) if caller is not None else ()
    )
    relocation_by_offset = {
        row.offset: row
        for row in relocations
    }
    constant_symbols = tuple(
        relocation_by_offset[offset].symbol_name
        for offset in (0x18, 0x20)
        if offset in relocation_by_offset
    )
    expected_mask = {
        index
        for relocation_offset in expected_offsets
        for index in range(relocation_offset, relocation_offset + 4)
    }
    if (
        caller is None
        or caller.symbol
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_CALLER_SYMBOL
        or caller.data != exact_body
        or tuple(row.offset for row in relocations) != expected_offsets
        or len(relocation_by_offset) != len(expected_offsets)
        or any(
            row.type != IMAGE_REL_I386_DIR32
            or row.offset + 4 > len(caller.data)
            for row in relocations
        )
        or any(
            relocation_by_offset[offset].symbol_name
            != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
            or struct.unpack_from("<I", caller.data, offset)[0] != addend
            for offset, addend in aggregate_addends.items()
        )
        or len(constant_symbols) != 2
        or len(set(constant_symbols)) != 2
        or any(
            re.fullmatch(r"\$T\d+", symbol) is None
            for symbol in constant_symbols
        )
        or any(
            struct.unpack_from("<I", caller.data, offset)[0] != 0
            for offset in (0x18, 0x20)
        )
        or len(caller.relocation_mask) != len(caller.data)
        or {
            index
            for index, masked in enumerate(caller.relocation_mask)
            if masked
        }
        != expected_mask
        or (
            caller.undefined_external_data + caller.defined_external_data
        ).count(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
        != 1
    ):
        raise ValueError(
            "HUD objective UpdateMeterXPoints vptr bridge rejects exact "
            "0x50 caller symbol/body, DIR32 target/addend/order, external "
            "identity, or complete relocation-mask drift"
        )

    expected_instructions = (
        (0x00, "push", ("51",)),
        (0x01, "mov", ("a1", "ac", "06", "00", "00")),
        (0x06, "mov", ("b9", "ac", "06", "00", "00")),
        (0x0B, "call", ("ff", "50", "64")),
        (0x0E, "mov", ("89", "44", "24", "00")),
        (0x12, "fild", ("db", "44", "24", "00")),
        (0x16, "fsub", ("d8", "25", "00", "00", "00", "00")),
        (0x1C, "fld", ("d9", "c0")),
        (0x1E, "fsub", ("d8", "25", "00", "00", "00", "00")),
        (0x24, "fld", ("d9", "c1")),
        (0x26, "fstp", ("d9", "1d", "60", "08", "00", "00")),
        (0x2C, "fxch", ("d9", "c9")),
        (0x2E, "fld", ("d9", "c1")),
        (0x30, "fxch", ("d9", "c9")),
        (0x32, "fstp", ("d9", "1d", "6c", "08", "00", "00")),
        (0x38, "fstp", ("d9", "1d", "78", "08", "00", "00")),
        (0x3E, "fstp", ("d9", "1d", "84", "08", "00", "00")),
        (0x44, "pop", ("59",)),
        (0x45, "ret", ("c3",)),
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
    actual_instructions = tuple(
        (
            offset,
            _cc_cfg._instruction_mnemonic(instruction_by_offset[offset]),
            tuple(
                value.lower()
                for value in instruction_by_offset[offset].bytes
            ),
        )
        for offset in sorted(index_by_offset)
    )
    if actual_instructions != expected_instructions:
        raise ValueError(
            "HUD objective UpdateMeterXPoints vptr bridge requires the exact "
            "complete straight-line candidate listing and instruction bytes"
        )

    aggregate = _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
    load = _cc_cfg._instruction_operand(
        instruction_by_offset[
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_LOAD_OFFSET
        ]
    ).split(",", 1)
    receiver = _cc_cfg._instruction_operand(
        instruction_by_offset[
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_RECEIVER_OFFSET
        ]
    ).split(",", 1)
    call = instruction_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_CALL_OFFSET
    ]
    widget_expressions = {
        (
            f"{aggregate}"
            f"+{_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_DISPLACEMENT}"
        ),
        (
            f"{aggregate}"
            f"+0x{_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_DISPLACEMENT:x}"
        ),
    }
    if (
        len(load) != 2
        or load[0].strip().lower() != "eax"
        or _cc_targets._exact_memory_expression(load[1]) not in widget_expressions
        or len(receiver) != 2
        or receiver[0].strip().lower() != "ecx"
        or _cc_targets._exact_memory_expression(receiver[1])
        not in (
            widget_expressions
            | {
                f"OFFSETFLAT:{expression}"
                for expression in widget_expressions
            }
        )
        or _cc_cfg._instruction_mnemonic(call) != "call"
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(call))
        not in {"eax+100", "eax+0x64"}
        or not (
            index_by_offset[
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_LOAD_OFFSET
            ]
            < index_by_offset[
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_RECEIVER_OFFSET
            ]
            < index_by_offset[
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_CALL_OFFSET
            ]
        )
    ):
        raise ValueError(
            "HUD objective UpdateMeterXPoints vptr bridge rejects exact "
            "EAX-vptr/ECX-receiver/slot reaching-definition drift"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    call_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_CALL_OFFSET
    ]
    if (
        tuple(invocation_indices) != (call_index,)
        or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
        or candidate.local_control_flow_indices
        or candidate.local_control_flow_targets
    ):
        raise ValueError(
            "HUD objective UpdateMeterXPoints vptr bridge rejects call "
            "ordinal, cleanup, or alternate local control-flow drift"
        )

    absolute_bridges = {
        normalize_address(
            hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_LOAD_OFFSET)
        ): ReviewedAbsoluteStorageLoadBridge(
            register="eax",
            aggregate_symbol=aggregate,
            displacement=(
                _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_DISPLACEMENT
            ),
            access_width=4,
            storage_identity=aggregate_storage,
        )
    }
    vptr_bridges = {
        normalize_address(
            hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_CALL_OFFSET)
        ): ReviewedVptrStorageBridge(
            register="eax",
            provenance=widget_storage,
            storage_identity=widget_storage,
            slot_displacement=(
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_SLOT_DISPLACEMENT
            ),
            identity_kind="virtual-slot",
        )
    }
    return absolute_bridges, vptr_bridges


def _hud_ui_mgr_objective_show_candidate_vptr_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedAbsoluteStorageLoadBridge],
    dict[str, ReviewedMemberVptrStorageBridge],
]:
    """Bridge only 0x411900's exact summary-panel SetTextFmt call."""
    from _recoil.call_contract.records import (
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedMemberVptrStorageBridge,
    )
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CALLER_START:
        return {}, {}

    caller_symbol_id = (
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CALLER_IDENTITY.removeprefix("symbol:")
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
        caller_identity != _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CALLER_END_EXCLUSIVE
        or caller_addresses != [_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CALLER_START]
        or caller_identity in indexes.provider_ids
        or not isinstance(caller_symbol, Mapping)
        or caller_symbol.get("binary") != "recoil"
        or caller_symbol.get("kind") != "function"
        or caller_symbol.get("pipeline_class") != "authored"
        or caller_symbol.get("ownership_state") != "primary-owned"
        or caller_symbol.get("address")
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CALLER_START
        or caller_symbol.get("end_exclusive")
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CALLER_END_EXCLUSIVE
        or caller_symbol.get("extent_state") != "known"
        or caller_symbol.get("size") != 0x120
        or caller_symbol.get("navigation_name")
        != "HudUiMgrObjective::Show"
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
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CALLER_ANCHOR_ID
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
            "HUD objective Show vptr bridge requires the exact authored "
            "caller/source and aggregate storage authority"
        )

    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CALLER_START
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
            "HUD objective Show vptr bridge requires one exact current HUD "
            "listing/source authority"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != r"\?Show@HudUiMgrObjective@@.*"
        or re.fullmatch(
            r"\?Show@HudUiMgrObjective@@.*",
            str(getattr(candidate.caller_definition, "symbol", "")),
        )
        is None
        or getattr(contribution_row, "name", "")
        != "HudUiMgrObjective::Show"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD objective Show vptr bridge requires the exact authored "
            "hud.cpp contribution row"
        )

    summary_storage = (
        _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_SUMMARY_PANEL_STORAGE_IDENTITY
    )
    exact_expected = {
        "ordinal": 0,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": summary_storage,
        "slot_displacement": _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_SLOT_DISPLACEMENT,
        "cleanup_bytes": _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CLEANUP_BYTES,
    }
    if not expected or expected[0] != exact_expected:
        observed = expected[0] if expected else None
        raise ValueError(
            "HUD objective Show vptr bridge requires the immutable retail "
            "ordinal-0 summary-panel/slot-0x74/cleanup-8 contract: "
            f"observed={observed!r}"
        )

    caller = candidate.caller_definition
    exact_prefix = bytes.fromhex(
        "56 57 85 d2 8b f9 "
        "0f 84 02 01 00 00 "
        "8b 74 24 0c 85 f6 "
        "0f 84 f6 00 00 00 "
        "a1 b4 0a 00 00 85 c0 "
        "0f 85 e9 00 00 00 "
        "a1 24 08 00 00 "
        "52 50 "
        "8b 08 "
        "ff 51 74 "
        "a1 74 09 00 00 "
        "83 c4 08"
    )
    prefix_relocation_specs = (
        (0x19, 0xAB4),
        (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_LOAD_OFFSET + 1,
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_SUMMARY_PANEL_DISPLACEMENT,
        ),
        (0x32, _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_DESC_PANEL_DISPLACEMENT),
    )
    prefix_relocations = (
        tuple(
            row
            for row in caller.relocations
            if row.offset <= 0x32
        )
        if caller is not None
        else ()
    )
    expected_prefix_mask = {
        index
        for relocation_offset, _ in prefix_relocation_specs
        for index in range(relocation_offset, relocation_offset + 4)
    }
    if (
        caller is None
        or caller.symbol != _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CALLER_SYMBOL
        or len(caller.data) != 0x120
        or caller.data[: len(exact_prefix)] != exact_prefix
        or tuple(row.offset for row in prefix_relocations)
        != tuple(offset for offset, _ in prefix_relocation_specs)
        or any(
            row.type != IMAGE_REL_I386_DIR32
            or row.symbol_name != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
            for row in prefix_relocations
        )
        or any(
            struct.unpack_from("<I", caller.data, offset)[0] != addend
            for offset, addend in prefix_relocation_specs
        )
        or len(caller.relocation_mask) != len(caller.data)
        or {
            index
            for index, masked in enumerate(
                caller.relocation_mask[: len(exact_prefix)]
            )
            if masked
        }
        != expected_prefix_mask
        or (
            caller.undefined_external_data + caller.defined_external_data
        ).count(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
        != 1
    ):
        raise ValueError(
            "HUD objective Show vptr bridge rejects exact 0x120 caller "
            "symbol/prefix, DIR32 target/addend/order, external identity, "
            "or prefix relocation-mask drift"
        )

    expected_instructions = (
        (0x00, "push", ("56",)),
        (0x01, "push", ("57",)),
        (0x02, "test", ("85", "d2")),
        (0x04, "mov", ("8b", "f9")),
        (0x06, "je", ("0f", "84", "02", "01", "00", "00")),
        (0x0C, "mov", ("8b", "74", "24", "0c")),
        (0x10, "test", ("85", "f6")),
        (0x12, "je", ("0f", "84", "f6", "00", "00", "00")),
        (0x18, "mov", ("a1", "b4", "0a", "00", "00")),
        (0x1D, "test", ("85", "c0")),
        (0x1F, "jne", ("0f", "85", "e9", "00", "00", "00")),
        (0x25, "mov", ("a1", "24", "08", "00", "00")),
        (0x2A, "push", ("52",)),
        (0x2B, "push", ("50",)),
        (0x2C, "mov", ("8b", "08")),
        (0x2E, "call", ("ff", "51", "74")),
        (0x31, "mov", ("a1", "74", "09", "00", "00")),
        (0x36, "add", ("83", "c4", "08")),
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
    actual_instructions = tuple(
        (
            offset,
            _cc_cfg._instruction_mnemonic(instruction_by_offset[offset]),
            tuple(
                value.lower()
                for value in instruction_by_offset[offset].bytes
            ),
        )
        for offset, _, _ in expected_instructions
        if offset in instruction_by_offset
    )
    if (
        actual_instructions != expected_instructions
        or any(counts.get(offset) != 1 for offset, _, _ in expected_instructions)
    ):
        raise ValueError(
            "HUD objective Show vptr bridge requires the exact candidate "
            "guard/load/push/vptr/call/cleanup listing and instruction bytes"
        )

    aggregate = _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
    exact_load_provenance = (
        "exact-load(eax,"
        f"load({aggregate_storage}+0x"
        f"{_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_SUMMARY_PANEL_DISPLACEMENT:x}))"
    )
    load = _cc_cfg._instruction_operand(
        instruction_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_LOAD_OFFSET]
    ).split(",", 1)
    receiver_push = _cc_cfg._instruction_operand(instruction_by_offset[0x2B])
    vptr = _cc_cfg._instruction_operand(
        instruction_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_VPTR_OFFSET]
    ).split(",", 1)
    call = instruction_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CALL_OFFSET]
    summary_expressions = {
        (
            f"{aggregate}+"
            f"{_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_SUMMARY_PANEL_DISPLACEMENT}"
        ),
        (
            f"{aggregate}+0x"
            f"{_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_SUMMARY_PANEL_DISPLACEMENT:x}"
        ),
    }
    if (
        len(load) != 2
        or load[0].strip().lower() != "eax"
        or _cc_targets._exact_memory_expression(load[1]) not in summary_expressions
        or receiver_push.strip().lower() != "eax"
        or len(vptr) != 2
        or vptr[0].strip().lower() != "ecx"
        or _cc_targets._exact_memory_expression(vptr[1]) != "eax"
        or _cc_cfg._instruction_mnemonic(call) != "call"
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(call))
        not in {"ecx+116", "ecx+0x74"}
        or not (
            index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_LOAD_OFFSET]
            < index_by_offset[0x2B]
            < index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_VPTR_OFFSET]
            < index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CALL_OFFSET]
        )
    ):
        raise ValueError(
            "HUD objective Show vptr bridge rejects exact EAX receiver/"
            "ECX-vptr/slot reaching-definition drift"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    call_index = index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CALL_OFFSET]
    branch_indices = {
        index_by_offset[offset] for offset in (0x06, 0x12, 0x1F)
    }
    if (
        not invocation_indices
        or invocation_indices[0] != call_index
        or _cc_cfg._cleanup_after(candidate.instructions, call_index)
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CLEANUP_BYTES
        or {
            index
            for index, instruction in enumerate(
                candidate.instructions[:call_index]
            )
            if _cc_cfg._instruction_mnemonic(instruction).startswith("j")
        }
        != branch_indices
        or any(
            target in {
                index_by_offset[offset]
                for offset in range(
                    _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_LOAD_OFFSET,
                    0x39,
                )
                if offset in index_by_offset
            }
            for targets in candidate.local_control_flow_targets.values()
            for target in targets
        )
        or candidate.local_control_flow_indices
    ):
        raise ValueError(
            "HUD objective Show vptr bridge rejects call ordinal, cleanup, "
            "or alternate local control-flow drift"
        )

    absolute_bridges = {
        normalize_address(hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_LOAD_OFFSET)):
        ReviewedAbsoluteStorageLoadBridge(
            register="eax",
            aggregate_symbol=aggregate,
            displacement=(
                _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_SUMMARY_PANEL_DISPLACEMENT
            ),
            access_width=4,
            storage_identity=aggregate_storage,
        )
    }
    member_vptr_bridges = {
        normalize_address(hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_VPTR_OFFSET)):
        ReviewedMemberVptrStorageBridge(
            register="ecx",
            source_register="eax",
            source_provenance=exact_load_provenance,
            receiver_register="eax",
            receiver_provenance=exact_load_provenance,
            storage_identity=summary_storage,
            slot_displacement=_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_SLOT_DISPLACEMENT,
            call_address=normalize_address(
                hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CALL_OFFSET)
            ),
        )
    }
    return absolute_bridges, member_vptr_bridges


def _hud_ui_mgr_objective_show_desc_candidate_vptr_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedAbsoluteStorageLoadBridge],
    dict[str, ReviewedMemberVptrStorageBridge],
]:
    """Bridge only 0x411900's exact description-panel SetTextFmt call."""
    from _recoil.call_contract.records import (
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedMemberVptrStorageBridge,
    )
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CALLER_START:
        return {}, {}

    # Reuse the immediately preceding WSI-034 proof as the exact caller,
    # source, aggregate, contribution-row, prefix, and ordinal-0 authority.
    # This function returns only the non-overlapping WSI-035 bridges below.
    _hud_ui_mgr_objective_show_candidate_vptr_bridges(
        expected,
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )

    desc_storage = _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_DESC_PANEL_STORAGE_IDENTITY
    exact_expected = {
        "ordinal": 1,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": desc_storage,
        "slot_displacement": _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_SLOT_DISPLACEMENT,
        "cleanup_bytes": _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CLEANUP_BYTES,
    }
    if len(expected) <= 1 or expected[1] != exact_expected:
        observed = expected[1] if len(expected) > 1 else None
        raise ValueError(
            "HUD objective Show description vptr bridge requires the "
            "immutable retail ordinal-1 description-panel/slot-0x74/"
            f"cleanup-8 contract: observed={observed!r}"
        )

    caller = candidate.caller_definition
    exact_window = bytes.fromhex(
        "a1 74 09 00 00 "
        "83 c4 08 "
        "8b 10 "
        "56 50 "
        "ff 52 74 "
        "a1 e0 0c 00 00 "
        "83 c4 08"
    )
    window_start = _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_DESC_LOAD_OFFSET
    window_end = window_start + len(exact_window)
    window_relocation_specs = (
        (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_DESC_LOAD_OFFSET + 1,
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_DESC_PANEL_DISPLACEMENT,
        ),
        (0x41, 0xCE0),
    )
    window_relocations = (
        tuple(
            row
            for row in caller.relocations
            if window_start <= row.offset < window_end
        )
        if caller is not None
        else ()
    )
    expected_window_mask = {
        index
        for relocation_offset, _ in window_relocation_specs
        for index in range(relocation_offset, relocation_offset + 4)
    }
    if (
        caller is None
        or caller.symbol != _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CALLER_SYMBOL
        or len(caller.data) != 0x120
        or caller.data[window_start:window_end] != exact_window
        or tuple(row.offset for row in window_relocations)
        != tuple(offset for offset, _ in window_relocation_specs)
        or any(
            row.type != IMAGE_REL_I386_DIR32
            or row.symbol_name != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
            for row in window_relocations
        )
        or any(
            struct.unpack_from("<I", caller.data, offset)[0] != addend
            for offset, addend in window_relocation_specs
        )
        or len(caller.relocation_mask) != len(caller.data)
        or {
            index
            for index in range(window_start, window_end)
            if caller.relocation_mask[index]
        }
        != expected_window_mask
        or (
            caller.undefined_external_data + caller.defined_external_data
        ).count(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
        != 1
    ):
        raise ValueError(
            "HUD objective Show description vptr bridge rejects exact "
            "0x120 caller symbol/window, DIR32 target/addend/order, external "
            "identity, or window relocation-mask drift"
        )

    expected_instructions = (
        (0x31, "mov", ("a1", "74", "09", "00", "00")),
        (0x36, "add", ("83", "c4", "08")),
        (0x39, "mov", ("8b", "10")),
        (0x3B, "push", ("56",)),
        (0x3C, "push", ("50",)),
        (0x3D, "call", ("ff", "52", "74")),
        (0x40, "mov", ("a1", "e0", "0c", "00", "00")),
        (0x45, "add", ("83", "c4", "08")),
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
    actual_instructions = tuple(
        (
            offset,
            _cc_cfg._instruction_mnemonic(instruction_by_offset[offset]),
            tuple(
                value.lower()
                for value in instruction_by_offset[offset].bytes
            ),
        )
        for offset, _, _ in expected_instructions
        if offset in instruction_by_offset
    )
    if (
        actual_instructions != expected_instructions
        or any(counts.get(offset) != 1 for offset, _, _ in expected_instructions)
    ):
        raise ValueError(
            "HUD objective Show description vptr bridge requires the exact "
            "candidate load/prior-cleanup/vptr/push/call/cleanup listing "
            "and instruction bytes"
        )

    aggregate = _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
    aggregate_storage = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    exact_load_provenance = (
        "exact-load(eax,"
        f"load({aggregate_storage}+0x"
        f"{_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_DESC_PANEL_DISPLACEMENT:x}))"
    )
    load = _cc_cfg._instruction_operand(
        instruction_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_DESC_LOAD_OFFSET]
    ).split(",", 1)
    vptr = _cc_cfg._instruction_operand(
        instruction_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_DESC_VPTR_OFFSET]
    ).split(",", 1)
    argument_push = _cc_cfg._instruction_operand(instruction_by_offset[0x3B])
    receiver_push = _cc_cfg._instruction_operand(instruction_by_offset[0x3C])
    call = instruction_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_DESC_CALL_OFFSET]
    desc_expressions = {
        (
            f"{aggregate}+"
            f"{_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_DESC_PANEL_DISPLACEMENT}"
        ),
        (
            f"{aggregate}+0x"
            f"{_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_DESC_PANEL_DISPLACEMENT:x}"
        ),
    }
    if (
        len(load) != 2
        or load[0].strip().lower() != "eax"
        or _cc_targets._exact_memory_expression(load[1]) not in desc_expressions
        or len(vptr) != 2
        or vptr[0].strip().lower() != "edx"
        or _cc_targets._exact_memory_expression(vptr[1]) != "eax"
        or argument_push.strip().lower() != "esi"
        or receiver_push.strip().lower() != "eax"
        or _cc_cfg._instruction_mnemonic(call) != "call"
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(call))
        not in {"edx+116", "edx+0x74"}
        or not (
            index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_DESC_LOAD_OFFSET]
            < index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_DESC_VPTR_OFFSET]
            < index_by_offset[0x3B]
            < index_by_offset[0x3C]
            < index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_DESC_CALL_OFFSET]
        )
    ):
        raise ValueError(
            "HUD objective Show description vptr bridge rejects exact EAX "
            "receiver/EDX-vptr/ESI-argument/slot reaching-definition drift"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    first_call_index = index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CALL_OFFSET]
    call_index = index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_DESC_CALL_OFFSET]
    reviewed_window_indices = {
        index_by_offset[offset]
        for offset, _, _ in expected_instructions
    }
    if (
        len(invocation_indices) <= 1
        or tuple(invocation_indices[:2]) != (first_call_index, call_index)
        or _cc_cfg._cleanup_after(candidate.instructions, first_call_index)
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CLEANUP_BYTES
        or _cc_cfg._cleanup_after(candidate.instructions, call_index)
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CLEANUP_BYTES
        or any(
            _cc_cfg._instruction_mnemonic(candidate.instructions[index]).startswith(
                "j"
            )
            for index in range(first_call_index + 1, call_index)
        )
        or any(
            target in reviewed_window_indices
            for targets in candidate.local_control_flow_targets.values()
            for target in targets
        )
        or candidate.local_control_flow_indices
    ):
        raise ValueError(
            "HUD objective Show description vptr bridge rejects call "
            "ordinal, prior/current cleanup, or alternate local "
            "control-flow drift"
        )

    absolute_bridges = {
        normalize_address(hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_DESC_LOAD_OFFSET)):
        ReviewedAbsoluteStorageLoadBridge(
            register="eax",
            aggregate_symbol=aggregate,
            displacement=(
                _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_DESC_PANEL_DISPLACEMENT
            ),
            access_width=4,
            storage_identity=aggregate_storage,
        )
    }
    member_vptr_bridges = {
        normalize_address(hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_DESC_VPTR_OFFSET)):
        ReviewedMemberVptrStorageBridge(
            register="edx",
            source_register="eax",
            source_provenance=exact_load_provenance,
            receiver_register="eax",
            receiver_provenance=exact_load_provenance,
            storage_identity=desc_storage,
            slot_displacement=_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_SLOT_DISPLACEMENT,
            call_address=normalize_address(
                hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_DESC_CALL_OFFSET)
            ),
        )
    }
    return absolute_bridges, member_vptr_bridges


def _hud_ui_mgr_objective_show_widget_candidate_vptr_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedAbsoluteStorageLoadBridge],
    dict[str, ReviewedVptrStorageBridge],
]:
    """Bridge only 0x411900's exact objective-widget GetCenterX call."""
    from _recoil.call_contract.records import (
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedVptrStorageBridge,
    )
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CALLER_START:
        return {}, {}

    # Reuse every cumulative WSI-034..036 caller/source/COFF authority and
    # preceding indirect-call proof.  This helper adds only the later widget
    # control-flow diamond and ordinal-4 call.
    _cc_recoil_hud_sensor._hud_ui_mgr_objective_show_sensor_overlay_candidate_vptr_bridges(
        expected,
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )

    widget_storage = _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_STORAGE_IDENTITY
    exact_expected = {
        "ordinal": 4,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": widget_storage,
        "slot_displacement": _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_WIDGET_SLOT_DISPLACEMENT,
        "cleanup_bytes": None,
    }
    if len(expected) <= 4 or expected[4] != exact_expected:
        observed = expected[4] if len(expected) > 4 else None
        raise ValueError(
            "HUD objective Show widget vptr bridge requires the immutable "
            "retail ordinal-4 embedded-widget/slot-0x64/no-cleanup contract: "
            f"observed={observed!r}"
        )

    caller = candidate.caller_definition
    exact_window = bytes.fromhex(
        "74 06 "
        "0f bf 70 04 "
        "eb 02 "
        "33 f6 "
        "8b 15 ac 06 00 00 "
        "b9 ac 06 00 00 "
        "ff 52 64 "
        "03 c6"
    )
    window_start = _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_WIDGET_CFG_START_OFFSET
    window_end = window_start + len(exact_window)
    window_relocation_specs = (
        (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_WIDGET_LOAD_OFFSET + 2,
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_DISPLACEMENT,
        ),
        (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_WIDGET_RECEIVER_OFFSET + 1,
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_DISPLACEMENT,
        ),
    )
    window_relocations = (
        tuple(
            row
            for row in caller.relocations
            if window_start <= row.offset < window_end
        )
        if caller is not None
        else ()
    )
    expected_window_mask = {
        index
        for relocation_offset, _ in window_relocation_specs
        for index in range(relocation_offset, relocation_offset + 4)
    }
    if (
        caller is None
        or caller.symbol != _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CALLER_SYMBOL
        or len(caller.data) != 0x120
        or caller.data[window_start:window_end] != exact_window
        or tuple(row.offset for row in window_relocations)
        != tuple(offset for offset, _ in window_relocation_specs)
        or any(
            row.type != IMAGE_REL_I386_DIR32
            or row.symbol_name != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
            for row in window_relocations
        )
        or any(
            struct.unpack_from("<I", caller.data, offset)[0] != addend
            for offset, addend in window_relocation_specs
        )
        or len(caller.relocation_mask) != len(caller.data)
        or {
            index
            for index in range(window_start, window_end)
            if caller.relocation_mask[index]
        }
        != expected_window_mask
        or (
            caller.undefined_external_data + caller.defined_external_data
        ).count(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
        != 1
    ):
        raise ValueError(
            "HUD objective Show widget vptr bridge rejects exact 0x120 "
            "caller symbol/CFG-window, paired DIR32 target/addend/order, "
            "external identity, or window relocation-mask drift"
        )

    expected_instructions = (
        (0x9C, "je", ("74", "06")),
        (0x9E, "movsx", ("0f", "bf", "70", "04")),
        (0xA2, "jmp", ("eb", "02")),
        (0xA4, "xor", ("33", "f6")),
        (0xA6, "mov", ("8b", "15", "ac", "06", "00", "00")),
        (0xAC, "mov", ("b9", "ac", "06", "00", "00")),
        (0xB1, "call", ("ff", "52", "64")),
        (0xB4, "add", ("03", "c6")),
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
    actual_instructions = tuple(
        (
            offset,
            _cc_cfg._instruction_mnemonic(instruction_by_offset[offset]),
            tuple(
                value.lower()
                for value in instruction_by_offset[offset].bytes
            ),
        )
        for offset, _, _ in expected_instructions
        if offset in instruction_by_offset
    )
    if (
        actual_instructions != expected_instructions
        or any(counts.get(offset) != 1 for offset, _, _ in expected_instructions)
    ):
        raise ValueError(
            "HUD objective Show widget vptr bridge requires the exact "
            "candidate width-diamond/vptr-load/receiver/call/result listing "
            "and instruction bytes"
        )

    aggregate = _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
    aggregate_storage = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    load = _cc_cfg._instruction_operand(
        instruction_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_WIDGET_LOAD_OFFSET]
    ).split(",", 1)
    receiver = _cc_cfg._instruction_operand(
        instruction_by_offset[
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_WIDGET_RECEIVER_OFFSET
        ]
    ).split(",", 1)
    call = instruction_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_WIDGET_CALL_OFFSET]
    widget_expressions = {
        (
            f"{aggregate}+"
            f"{_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_DISPLACEMENT}"
        ),
        (
            f"{aggregate}+0x"
            f"{_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_DISPLACEMENT:x}"
        ),
    }
    if (
        len(load) != 2
        or load[0].strip().lower() != "edx"
        or _cc_targets._exact_memory_expression(load[1]) not in widget_expressions
        or len(receiver) != 2
        or receiver[0].strip().lower() != "ecx"
        or _cc_targets._exact_memory_expression(receiver[1])
        not in (
            widget_expressions
            | {
                f"OFFSETFLAT:{expression}"
                for expression in widget_expressions
            }
        )
        or _cc_cfg._instruction_mnemonic(call) != "call"
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(call))
        not in {"edx+100", "edx+0x64"}
        or not (
            index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_WIDGET_LOAD_OFFSET]
            < index_by_offset[
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_WIDGET_RECEIVER_OFFSET
            ]
            < index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_WIDGET_CALL_OFFSET]
        )
    ):
        raise ValueError(
            "HUD objective Show widget vptr bridge rejects exact EDX-vptr/"
            "ECX-receiver/slot reaching-definition drift"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    call_index = index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_WIDGET_CALL_OFFSET]
    branch_indices = {
        index_by_offset[0x9C],
        index_by_offset[0xA2],
    }
    reviewed_window_indices = {
        index_by_offset[offset]
        for offset, _, _ in expected_instructions
    }
    if (
        len(invocation_indices) <= 4
        or invocation_indices[4] != call_index
        or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
        or {
            index
            for index in range(
                index_by_offset[0x9C],
                call_index,
            )
            if _cc_cfg._instruction_mnemonic(
                candidate.instructions[index]
            ).startswith("j")
        }
        != branch_indices
        or any(
            target in reviewed_window_indices
            for targets in candidate.local_control_flow_targets.values()
            for target in targets
        )
        or candidate.local_control_flow_indices
    ):
        raise ValueError(
            "HUD objective Show widget vptr bridge rejects call ordinal, "
            "cleanup, width-diamond uniqueness, or alternate local "
            "control-flow drift"
        )

    absolute_bridges = {
        normalize_address(hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_WIDGET_LOAD_OFFSET)):
        ReviewedAbsoluteStorageLoadBridge(
            register="edx",
            aggregate_symbol=aggregate,
            displacement=_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_DISPLACEMENT,
            access_width=4,
            storage_identity=aggregate_storage,
        )
    }
    vptr_bridges = {
        normalize_address(hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_WIDGET_CALL_OFFSET)):
        ReviewedVptrStorageBridge(
            register="edx",
            provenance=widget_storage,
            storage_identity=widget_storage,
            slot_displacement=(
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_WIDGET_SLOT_DISPLACEMENT
            ),
            identity_kind="virtual-slot",
        )
    }
    return absolute_bridges, vptr_bridges


def _hud_ui_mgr_objective_show_bar_candidate_vptr_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedAbsoluteStorageLoadBridge],
    dict[str, ReviewedVptrStorageBridge],
]:
    """Bridge only 0x411900's exact objective-bar SetVisible call."""
    from _recoil.call_contract.records import (
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedVptrStorageBridge,
    )
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CALLER_START:
        return {}, {}

    # Reuse the complete WSI-034..037 caller/source/COFF authority and all
    # preceding calls.  This helper adds only the phase-0 objective-bar call.
    _hud_ui_mgr_objective_show_widget_candidate_vptr_bridges(
        expected,
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )

    bar_storage = _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_STORAGE_IDENTITY
    exact_expected = {
        "ordinal": 5,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": bar_storage,
        "slot_displacement": _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_SLOT_DISPLACEMENT,
        "cleanup_bytes": None,
    }
    if len(expected) <= 5 or expected[5] != exact_expected:
        observed = expected[5] if len(expected) > 5 else None
        raise ValueError(
            "HUD objective Show bar vptr bridge requires the immutable "
            "retail ordinal-5 embedded-bar/slot-0x60/no-cleanup contract: "
            f"observed={observed!r}"
        )

    caller = candidate.caller_definition
    exact_window = bytes.fromhex(
        "b9 78 09 00 00 "
        "a3 a8 06 00 00 "
        "a1 78 09 00 00 "
        "57 "
        "ff 50 60"
    )
    argument_seed_bytes = (
        caller.data[
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_ARGUMENT_SEED_OFFSET:
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_ARGUMENT_SEED_OFFSET + 5
        ]
        if caller is not None
        else b""
    )
    window_start = _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_RECEIVER_OFFSET
    window_end = window_start + len(exact_window)
    window_relocation_specs = (
        (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_RECEIVER_OFFSET + 1,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_DISPLACEMENT,
        ),
        (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_RESULT_STORE_OFFSET + 1,
            0x6A8,
        ),
        (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_LOAD_OFFSET + 1,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_DISPLACEMENT,
        ),
    )
    window_relocations = (
        tuple(
            row
            for row in caller.relocations
            if window_start <= row.offset < window_end
        )
        if caller is not None
        else ()
    )
    expected_window_mask = {
        index
        for relocation_offset, _ in window_relocation_specs
        for index in range(relocation_offset, relocation_offset + 4)
    }
    if (
        caller is None
        or caller.symbol != _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CALLER_SYMBOL
        or len(caller.data) != 0x120
        or argument_seed_bytes != bytes.fromhex("bf 01 00 00 00")
        or caller.data[window_start:window_end] != exact_window
        or tuple(row.offset for row in window_relocations)
        != tuple(offset for offset, _ in window_relocation_specs)
        or any(
            row.type != IMAGE_REL_I386_DIR32
            or row.symbol_name != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
            for row in window_relocations
        )
        or any(
            struct.unpack_from("<I", caller.data, offset)[0] != addend
            for offset, addend in window_relocation_specs
        )
        or len(caller.relocation_mask) != len(caller.data)
        or {
            index
            for index in range(window_start, window_end)
            if caller.relocation_mask[index]
        }
        != expected_window_mask
        or (
            caller.undefined_external_data + caller.defined_external_data
        ).count(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
        != 1
    ):
        raise ValueError(
            "HUD objective Show bar vptr bridge rejects exact 0x120 caller "
            "symbol/window, three DIR32 target/addend/order rows, external "
            "identity, or window relocation-mask drift"
        )

    expected_instructions = (
        (0x6B, "mov", ("bf", "01", "00", "00", "00")),
        (0xB6, "mov", ("b9", "78", "09", "00", "00")),
        (0xBB, "mov", ("a3", "a8", "06", "00", "00")),
        (0xC0, "mov", ("a1", "78", "09", "00", "00")),
        (0xC5, "push", ("57",)),
        (0xC6, "call", ("ff", "50", "60")),
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
    actual_instructions = tuple(
        (
            offset,
            _cc_cfg._instruction_mnemonic(instruction_by_offset[offset]),
            tuple(
                value.lower()
                for value in instruction_by_offset[offset].bytes
            ),
        )
        for offset, _, _ in expected_instructions
        if offset in instruction_by_offset
    )
    if (
        actual_instructions != expected_instructions
        or any(counts.get(offset) != 1 for offset, _, _ in expected_instructions)
    ):
        raise ValueError(
            "HUD objective Show bar vptr bridge requires the exact candidate "
            "argument-seed/receiver/result-store/vptr-load/argument/call "
            "listing and instruction bytes"
        )

    aggregate = _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
    aggregate_storage = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    argument_seed = _cc_cfg._instruction_operand(
        instruction_by_offset[
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_ARGUMENT_SEED_OFFSET
        ]
    ).split(",", 1)
    receiver = _cc_cfg._instruction_operand(
        instruction_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_RECEIVER_OFFSET]
    ).split(",", 1)
    result_store = _cc_cfg._instruction_operand(
        instruction_by_offset[
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_RESULT_STORE_OFFSET
        ]
    ).split(",", 1)
    load = _cc_cfg._instruction_operand(
        instruction_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_LOAD_OFFSET]
    ).split(",", 1)
    argument = instruction_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_ARGUMENT_OFFSET
    ]
    call = instruction_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_CALL_OFFSET]
    bar_expressions = {
        f"{aggregate}+{_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_DISPLACEMENT}",
        f"{aggregate}+0x{_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_DISPLACEMENT:x}",
    }
    result_expressions = {
        f"{aggregate}+1704",
        f"{aggregate}+0x6a8",
    }
    if (
        len(argument_seed) != 2
        or argument_seed[0].strip().lower() != "edi"
        or argument_seed[1].strip().lower() not in {"1", "0x1"}
        or len(receiver) != 2
        or receiver[0].strip().lower() != "ecx"
        or _cc_targets._exact_memory_expression(receiver[1])
        not in (
            bar_expressions
            | {
                f"OFFSETFLAT:{expression}"
                for expression in bar_expressions
            }
        )
        or len(result_store) != 2
        or _cc_targets._exact_memory_expression(result_store[0])
        not in result_expressions
        or result_store[1].strip().lower() != "eax"
        or len(load) != 2
        or load[0].strip().lower() != "eax"
        or _cc_targets._exact_memory_expression(load[1]) not in bar_expressions
        or _cc_cfg._instruction_mnemonic(argument) != "push"
        or _cc_cfg._instruction_operand(argument).strip().lower() != "edi"
        or _cc_cfg._instruction_mnemonic(call) != "call"
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(call))
        not in {"eax+96", "eax+0x60"}
        or not (
            index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_ARGUMENT_SEED_OFFSET]
            < index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_RECEIVER_OFFSET]
            < index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_RESULT_STORE_OFFSET]
            < index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_LOAD_OFFSET]
            < index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_ARGUMENT_OFFSET]
            < index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_CALL_OFFSET]
        )
    ):
        raise ValueError(
            "HUD objective Show bar vptr bridge rejects exact EDI-one/"
            "ECX-receiver/result-store/EAX-vptr/argument/slot "
            "reaching-definition drift"
        )


    seed_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_ARGUMENT_SEED_OFFSET
    ]
    receiver_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_RECEIVER_OFFSET
    ]
    load_index = index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_LOAD_OFFSET]
    argument_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_ARGUMENT_OFFSET
    ]
    call_index = index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_CALL_OFFSET]
    if (
        any(
            _cc_instructions.may_clobber_register(candidate.instructions[index], "edi")
            for index in range(seed_index + 1, argument_index)
        )
        or any(
            _cc_instructions.may_clobber_register(candidate.instructions[index], "ecx")
            for index in range(receiver_index + 1, call_index)
        )
        or any(
            _cc_instructions.may_clobber_register(candidate.instructions[index], "eax")
            for index in range(load_index + 1, call_index)
        )
    ):
        raise ValueError(
            "HUD objective Show bar vptr bridge rejects EDI argument, ECX "
            "receiver, or EAX vptr clobber along the reviewed phase-0 path"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    reviewed_window_indices = {
        index_by_offset[offset]
        for offset in range(
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_RECEIVER_OFFSET,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_CALL_OFFSET + 1,
        )
        if offset in index_by_offset
    }
    if (
        len(invocation_indices) <= 5
        or invocation_indices[5] != call_index
        or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
        or any(
            _cc_cfg._instruction_mnemonic(candidate.instructions[index]).startswith(
                "j"
            )
            for index in range(receiver_index, call_index)
        )
        or any(
            target in reviewed_window_indices
            for targets in candidate.local_control_flow_targets.values()
            for target in targets
        )
        or candidate.local_control_flow_indices
    ):
        raise ValueError(
            "HUD objective Show bar vptr bridge rejects call ordinal, cleanup, "
            "straight-line phase-0 uniqueness, or alternate local "
            "control-flow drift"
        )

    absolute_bridges = {
        normalize_address(hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_LOAD_OFFSET)):
        ReviewedAbsoluteStorageLoadBridge(
            register="eax",
            aggregate_symbol=aggregate,
            displacement=_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_DISPLACEMENT,
            access_width=4,
            storage_identity=aggregate_storage,
        )
    }
    vptr_bridges = {
        normalize_address(hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_CALL_OFFSET)):
        ReviewedVptrStorageBridge(
            register="eax",
            provenance=bar_storage,
            storage_identity=bar_storage,
            slot_displacement=(
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_BAR_SLOT_DISPLACEMENT
            ),
            identity_kind="virtual-slot",
        )
    }
    return absolute_bridges, vptr_bridges


def _hud_ui_mgr_objective_begin_summary_candidate_vptr_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedAbsoluteStorageLoadBridge],
    dict[str, ReviewedMemberVptrStorageBridge],
]:
    """Bridge only 0x411a20's exact summary-panel SetVisible call."""
    from _recoil.call_contract.records import (
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedMemberVptrStorageBridge,
    )
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_CALLER_START:
        return {}, {}

    caller_symbol_id = (
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_CALLER_IDENTITY.removeprefix("symbol:")
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
        caller_identity != _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_CALLER_END_EXCLUSIVE
        or caller_addresses != [_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_CALLER_START]
        or caller_identity in indexes.provider_ids
        or not isinstance(caller_symbol, Mapping)
        or caller_symbol.get("binary") != "recoil"
        or caller_symbol.get("kind") != "function"
        or caller_symbol.get("pipeline_class") != "authored"
        or caller_symbol.get("ownership_state") != "primary-owned"
        or caller_symbol.get("address")
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_CALLER_START
        or caller_symbol.get("end_exclusive")
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_CALLER_END_EXCLUSIVE
        or caller_symbol.get("extent_state") != "known"
        or caller_symbol.get("size") != 0xA0
        or caller_symbol.get("navigation_name")
        != "HudUiMgrObjective::Begin"
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
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_CALLER_ANCHOR_ID
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
            "HUD objective Begin summary vptr bridge requires the exact "
            "authored caller/source and aggregate storage authority"
        )

    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_CALLER_START
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
            "HUD objective Begin summary vptr bridge requires one exact "
            "current HUD listing/source authority"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != r"\?Begin@HudUiMgrObjective@@.*"
        or re.fullmatch(
            r"\?Begin@HudUiMgrObjective@@.*",
            str(getattr(candidate.caller_definition, "symbol", "")),
        )
        is None
        or getattr(contribution_row, "name", "")
        != "HudUiMgrObjective::Begin"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD objective Begin summary vptr bridge requires the exact "
            "authored hud.cpp contribution row"
        )

    summary_storage = (
        _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_SUMMARY_PANEL_STORAGE_IDENTITY
    )
    exact_expected = {
        "ordinal": 0,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": summary_storage,
        "slot_displacement": _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SLOT_DISPLACEMENT,
        "cleanup_bytes": None,
    }
    if not expected or expected[0] != exact_expected:
        observed = expected[0] if expected else None
        raise ValueError(
            "HUD objective Begin summary vptr bridge requires the immutable "
            "retail ordinal-0 summary-panel/slot-0x60/no-cleanup contract: "
            f"observed={observed!r}"
        )

    caller = candidate.caller_definition
    exact_prefix = bytes.fromhex(
        "a1 b4 0a 00 00 "
        "85 c0 "
        "0f 85 87 00 00 00 "
        "a1 94 06 00 00 "
        "83 f8 02 "
        "75 52 "
        "8b 0d 24 08 00 00 "
        "c7 05 90 06 00 00 01 00 00 00 "
        "c7 05 94 06 00 00 03 00 00 00 "
        "c7 05 98 06 00 00 00 00 00 00 "
        "8b 01 "
        "6a 00 "
        "ff 50 60"
    )
    prefix_relocation_specs = (
        (0x01, 0xAB4),
        (0x0E, 0x694),
        (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_LOAD_OFFSET + 2,
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_SUMMARY_PANEL_DISPLACEMENT,
        ),
        (0x1F, 0x690),
        (0x29, 0x694),
        (0x33, 0x698),
    )
    prefix_relocations = (
        tuple(
            row
            for row in caller.relocations
            if row.offset < len(exact_prefix)
        )
        if caller is not None
        else ()
    )
    expected_prefix_mask = {
        index
        for relocation_offset, _ in prefix_relocation_specs
        for index in range(relocation_offset, relocation_offset + 4)
    }
    if (
        caller is None
        or caller.symbol != _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_CALLER_SYMBOL
        or len(caller.data) != 0xA0
        or caller.data[: len(exact_prefix)] != exact_prefix
        or tuple(row.offset for row in prefix_relocations)
        != tuple(offset for offset, _ in prefix_relocation_specs)
        or any(
            row.type != IMAGE_REL_I386_DIR32
            or row.symbol_name != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
            for row in prefix_relocations
        )
        or any(
            struct.unpack_from("<I", caller.data, offset)[0] != addend
            for offset, addend in prefix_relocation_specs
        )
        or len(caller.relocation_mask) != len(caller.data)
        or {
            index
            for index, masked in enumerate(
                caller.relocation_mask[: len(exact_prefix)]
            )
            if masked
        }
        != expected_prefix_mask
        or (
            caller.undefined_external_data + caller.defined_external_data
        ).count(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
        != 1
    ):
        raise ValueError(
            "HUD objective Begin summary vptr bridge rejects exact 0xa0 "
            "caller symbol/prefix, six DIR32 target/addend/order rows, "
            "external identity, or prefix relocation-mask drift"
        )

    expected_instructions = (
        (0x00, "mov", ("a1", "b4", "0a", "00", "00")),
        (0x05, "test", ("85", "c0")),
        (0x07, "jne", ("0f", "85", "87", "00", "00", "00")),
        (0x0D, "mov", ("a1", "94", "06", "00", "00")),
        (0x12, "cmp", ("83", "f8", "02")),
        (0x15, "jne", ("75", "52")),
        (0x17, "mov", ("8b", "0d", "24", "08", "00", "00")),
        (
            0x1D,
            "mov",
            ("c7", "05", "90", "06", "00", "00", "01", "00", "00", "00"),
        ),
        (
            0x27,
            "mov",
            ("c7", "05", "94", "06", "00", "00", "03", "00", "00", "00"),
        ),
        (
            0x31,
            "mov",
            ("c7", "05", "98", "06", "00", "00", "00", "00", "00", "00"),
        ),
        (0x3B, "mov", ("8b", "01")),
        (0x3D, "push", ("6a", "00")),
        (0x3F, "call", ("ff", "50", "60")),
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
    actual_instructions = tuple(
        (
            offset,
            _cc_cfg._instruction_mnemonic(instruction_by_offset[offset]),
            tuple(
                value.lower()
                for value in instruction_by_offset[offset].bytes
            ),
        )
        for offset, _, _ in expected_instructions
        if offset in instruction_by_offset
    )
    if (
        actual_instructions != expected_instructions
        or any(counts.get(offset) != 1 for offset, _, _ in expected_instructions)
    ):
        raise ValueError(
            "HUD objective Begin summary vptr bridge requires the exact "
            "candidate guards/load/state-stores/vptr/zero/call listing and "
            "instruction bytes"
        )

    aggregate = _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
    exact_load_provenance = (
        "exact-load(ecx,"
        f"load({aggregate_storage}+0x"
        f"{_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_SUMMARY_PANEL_DISPLACEMENT:x}))"
    )
    load = _cc_cfg._instruction_operand(
        instruction_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_LOAD_OFFSET]
    ).split(",", 1)
    vptr = _cc_cfg._instruction_operand(
        instruction_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_VPTR_OFFSET]
    ).split(",", 1)
    argument = instruction_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_ARGUMENT_OFFSET
    ]
    call = instruction_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_CALL_OFFSET]
    summary_expressions = {
        (
            f"{aggregate}+"
            f"{_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_SUMMARY_PANEL_DISPLACEMENT}"
        ),
        (
            f"{aggregate}+0x"
            f"{_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_SUMMARY_PANEL_DISPLACEMENT:x}"
        ),
    }
    if (
        len(load) != 2
        or load[0].strip().lower() != "ecx"
        or _cc_targets._exact_memory_expression(load[1]) not in summary_expressions
        or len(vptr) != 2
        or vptr[0].strip().lower() != "eax"
        or _cc_targets._exact_memory_expression(vptr[1]) != "ecx"
        or _cc_cfg._instruction_mnemonic(argument) != "push"
        or _cc_cfg._instruction_operand(argument).strip().lower() not in {"0", "0x0"}
        or _cc_cfg._instruction_mnemonic(call) != "call"
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(call))
        not in {"eax+96", "eax+0x60"}
        or not (
            index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_LOAD_OFFSET]
            < index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_VPTR_OFFSET]
            < index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_ARGUMENT_OFFSET]
            < index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_CALL_OFFSET]
        )
    ):
        raise ValueError(
            "HUD objective Begin summary vptr bridge rejects exact ECX "
            "receiver/EAX-vptr/zero-argument/slot reaching-definition drift"
        )


    load_index = index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_LOAD_OFFSET]
    vptr_index = index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_VPTR_OFFSET]
    call_index = index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_CALL_OFFSET]
    if (
        any(
            _cc_instructions.may_clobber_register(candidate.instructions[index], "ecx")
            for index in range(load_index + 1, vptr_index)
        )
        or any(
            _cc_instructions.may_clobber_register(candidate.instructions[index], "eax")
            for index in range(vptr_index + 1, call_index)
        )
    ):
        raise ValueError(
            "HUD objective Begin summary vptr bridge rejects ECX receiver or "
            "EAX vptr clobber on the reviewed phase-2 path"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    branch_indices = {
        index_by_offset[offset] for offset in (0x07, 0x15)
    }
    reviewed_window_indices = {
        index_by_offset[offset]
        for offset in range(0x00, _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_CALL_OFFSET + 1)
        if offset in index_by_offset
    }
    if (
        not invocation_indices
        or invocation_indices[0] != call_index
        or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
        or {
            index
            for index, instruction in enumerate(
                candidate.instructions[:call_index]
            )
            if _cc_cfg._instruction_mnemonic(instruction).startswith("j")
        }
        != branch_indices
        or any(
            target in reviewed_window_indices
            for targets in candidate.local_control_flow_targets.values()
            for target in targets
        )
        or candidate.local_control_flow_indices
    ):
        raise ValueError(
            "HUD objective Begin summary vptr bridge rejects call ordinal, "
            "cleanup, guard uniqueness, or alternate local control-flow drift"
        )

    absolute_bridges = {
        normalize_address(hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_LOAD_OFFSET)):
        ReviewedAbsoluteStorageLoadBridge(
            register="ecx",
            aggregate_symbol=aggregate,
            displacement=(
                _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_SUMMARY_PANEL_DISPLACEMENT
            ),
            access_width=4,
            storage_identity=aggregate_storage,
        )
    }
    member_vptr_bridges = {
        normalize_address(hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_VPTR_OFFSET)):
        ReviewedMemberVptrStorageBridge(
            register="eax",
            source_register="ecx",
            source_provenance=exact_load_provenance,
            receiver_register="ecx",
            receiver_provenance=exact_load_provenance,
            storage_identity=summary_storage,
            slot_displacement=_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SLOT_DISPLACEMENT,
            call_address=normalize_address(
                hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_CALL_OFFSET)
            ),
        )
    }
    return absolute_bridges, member_vptr_bridges


def _hud_ui_mgr_objective_begin_desc_candidate_vptr_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedAbsoluteStorageLoadBridge],
    dict[str, ReviewedMemberVptrStorageBridge],
]:
    """Bridge only 0x411a20's exact description-panel SetVisible call."""
    from _recoil.call_contract.records import (
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedMemberVptrStorageBridge,
    )
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_CALLER_START:
        return {}, {}

    # Reuse WSI-039's complete caller/source/manifest authority, exact 0xa0
    # object prefix, six preceding relocations, and ordinal-0 call proof.
    _hud_ui_mgr_objective_begin_summary_candidate_vptr_bridges(
        expected,
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )

    desc_storage = _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_DESC_PANEL_STORAGE_IDENTITY
    exact_expected = {
        "ordinal": 1,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": desc_storage,
        "slot_displacement": _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SLOT_DISPLACEMENT,
        "cleanup_bytes": None,
    }
    if len(expected) <= 1 or expected[1] != exact_expected:
        observed = expected[1] if len(expected) > 1 else None
        raise ValueError(
            "HUD objective Begin description vptr bridge requires the "
            "immutable retail ordinal-1 description-panel/slot-0x60/"
            f"no-cleanup contract: observed={observed!r}"
        )

    caller = candidate.caller_definition
    exact_window = bytes.fromhex(
        "8b 0d 74 09 00 00 "
        "6a 00 "
        "8b 11 "
        "ff 52 60"
    )
    window_start = _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_DESC_LOAD_OFFSET
    window_end = window_start + len(exact_window)
    extended_relocations = (
        tuple(
            row
            for row in caller.relocations
            if row.offset < window_end
        )
        if caller is not None
        else ()
    )
    relocation_specs = (
        (0x01, 0xAB4),
        (0x0E, 0x694),
        (0x19, 0x824),
        (0x1F, 0x690),
        (0x29, 0x694),
        (0x33, 0x698),
        (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_DESC_LOAD_OFFSET + 2,
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_DESC_PANEL_DISPLACEMENT,
        ),
    )
    expected_mask = {
        index
        for relocation_offset, _ in relocation_specs
        for index in range(relocation_offset, relocation_offset + 4)
    }
    if (
        caller is None
        or caller.symbol != _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_CALLER_SYMBOL
        or len(caller.data) != 0xA0
        or caller.data[window_start:window_end] != exact_window
        or tuple(row.offset for row in extended_relocations)
        != tuple(offset for offset, _ in relocation_specs)
        or any(
            row.type != IMAGE_REL_I386_DIR32
            or row.symbol_name != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
            for row in extended_relocations
        )
        or any(
            struct.unpack_from("<I", caller.data, offset)[0] != addend
            for offset, addend in relocation_specs
        )
        or len(caller.relocation_mask) != len(caller.data)
        or {
            index
            for index in range(window_end)
            if caller.relocation_mask[index]
        }
        != expected_mask
        or (
            caller.undefined_external_data + caller.defined_external_data
        ).count(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
        != 1
    ):
        raise ValueError(
            "HUD objective Begin description vptr bridge rejects exact 0xa0 "
            "caller symbol/window, seven DIR32 target/addend/order rows, "
            "external identity, or extended relocation-mask drift"
        )

    expected_instructions = (
        (0x42, "mov", ("8b", "0d", "74", "09", "00", "00")),
        (0x48, "push", ("6a", "00")),
        (0x4A, "mov", ("8b", "11")),
        (0x4C, "call", ("ff", "52", "60")),
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
    actual_instructions = tuple(
        (
            offset,
            _cc_cfg._instruction_mnemonic(instruction_by_offset[offset]),
            tuple(
                value.lower()
                for value in instruction_by_offset[offset].bytes
            ),
        )
        for offset, _, _ in expected_instructions
        if offset in instruction_by_offset
    )
    if (
        actual_instructions != expected_instructions
        or any(counts.get(offset) != 1 for offset, _, _ in expected_instructions)
    ):
        raise ValueError(
            "HUD objective Begin description vptr bridge requires the exact "
            "candidate receiver-load/zero/vptr/call listing and instruction "
            "bytes"
        )

    aggregate = _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
    aggregate_storage = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    exact_load_provenance = (
        "exact-load(ecx,"
        f"load({aggregate_storage}+0x"
        f"{_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_DESC_PANEL_DISPLACEMENT:x}))"
    )
    load = _cc_cfg._instruction_operand(
        instruction_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_DESC_LOAD_OFFSET]
    ).split(",", 1)
    argument = instruction_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_DESC_ARGUMENT_OFFSET
    ]
    vptr = _cc_cfg._instruction_operand(
        instruction_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_DESC_VPTR_OFFSET]
    ).split(",", 1)
    call = instruction_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_DESC_CALL_OFFSET]
    desc_expressions = {
        (
            f"{aggregate}+"
            f"{_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_DESC_PANEL_DISPLACEMENT}"
        ),
        (
            f"{aggregate}+0x"
            f"{_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_DESC_PANEL_DISPLACEMENT:x}"
        ),
    }
    if (
        len(load) != 2
        or load[0].strip().lower() != "ecx"
        or _cc_targets._exact_memory_expression(load[1]) not in desc_expressions
        or _cc_cfg._instruction_mnemonic(argument) != "push"
        or _cc_cfg._instruction_operand(argument).strip().lower() not in {"0", "0x0"}
        or len(vptr) != 2
        or vptr[0].strip().lower() != "edx"
        or _cc_targets._exact_memory_expression(vptr[1]) != "ecx"
        or _cc_cfg._instruction_mnemonic(call) != "call"
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(call))
        not in {"edx+96", "edx+0x60"}
        or not (
            index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_DESC_LOAD_OFFSET]
            < index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_DESC_ARGUMENT_OFFSET]
            < index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_DESC_VPTR_OFFSET]
            < index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_DESC_CALL_OFFSET]
        )
    ):
        raise ValueError(
            "HUD objective Begin description vptr bridge rejects exact ECX "
            "receiver/zero-argument/EDX-vptr/slot reaching-definition drift"
        )


    load_index = index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_DESC_LOAD_OFFSET]
    vptr_index = index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_DESC_VPTR_OFFSET]
    call_index = index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_DESC_CALL_OFFSET]
    if (
        any(
            _cc_instructions.may_clobber_register(candidate.instructions[index], "ecx")
            for index in range(load_index + 1, vptr_index)
        )
        or any(
            _cc_instructions.may_clobber_register(candidate.instructions[index], "edx")
            for index in range(vptr_index + 1, call_index)
        )
    ):
        raise ValueError(
            "HUD objective Begin description vptr bridge rejects ECX receiver "
            "or EDX vptr clobber on the reviewed phase-2 path"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    reviewed_window_indices = {
        index_by_offset[offset]
        for offset in range(window_start, window_end)
        if offset in index_by_offset
    }
    if (
        len(invocation_indices) <= 1
        or invocation_indices[1] != call_index
        or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
        or any(
            _cc_cfg._instruction_mnemonic(candidate.instructions[index]).startswith(
                "j"
            )
            for index in range(load_index, call_index)
        )
        or any(
            target in reviewed_window_indices
            for targets in candidate.local_control_flow_targets.values()
            for target in targets
        )
        or candidate.local_control_flow_indices
    ):
        raise ValueError(
            "HUD objective Begin description vptr bridge rejects call "
            "ordinal, cleanup, straight-line uniqueness, or alternate local "
            "control-flow drift"
        )

    absolute_bridges = {
        normalize_address(hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_DESC_LOAD_OFFSET)):
        ReviewedAbsoluteStorageLoadBridge(
            register="ecx",
            aggregate_symbol=aggregate,
            displacement=(
                _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_DESC_PANEL_DISPLACEMENT
            ),
            access_width=4,
            storage_identity=aggregate_storage,
        )
    }
    member_vptr_bridges = {
        normalize_address(hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_DESC_VPTR_OFFSET)):
        ReviewedMemberVptrStorageBridge(
            register="edx",
            source_register="ecx",
            source_provenance=exact_load_provenance,
            receiver_register="ecx",
            receiver_provenance=exact_load_provenance,
            storage_identity=desc_storage,
            slot_displacement=_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SLOT_DISPLACEMENT,
            call_address=normalize_address(
                hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_DESC_CALL_OFFSET)
            ),
        )
    }
    return absolute_bridges, member_vptr_bridges


def _hud_ui_mgr_objective_start_hide_candidate_vptr_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedAbsoluteStorageLoadBridge],
    dict[str, ReviewedVptrStorageBridge],
]:
    """Bridge only StartHide's first objective-bar Invalidate call."""
    from _recoil.call_contract.records import (
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedVptrStorageBridge,
    )
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_CALLER_START:
        return {}, {}

    def reject(field: str, *, expected_value: Any, observed_value: Any) -> None:
        raise ValueError(
            "HUD objective StartHide bar-vptr bridge rejects "
            f"{field}: expected={expected_value!r}; observed={observed_value!r}"
        )

    def require(field: str, observed_value: Any, expected_value: Any) -> None:
        if observed_value != expected_value:
            reject(
                field,
                expected_value=expected_value,
                observed_value=observed_value,
            )

    caller_symbol_id = (
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_CALLER_IDENTITY.removeprefix(
            "symbol:"
        )
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
    require(
        "caller_identity",
        caller_identity,
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_CALLER_IDENTITY,
    )
    require(
        "caller_end_exclusive",
        normalize_address(caller_end_exclusive),
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_CALLER_END_EXCLUSIVE,
    )
    require(
        "caller_address_index",
        caller_addresses,
        [_cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_CALLER_START],
    )
    require("caller_provider_membership", caller_identity in indexes.provider_ids, False)
    require("caller_symbol_mapping", isinstance(caller_symbol, Mapping), True)
    assert isinstance(caller_symbol, Mapping)
    for field, expected_value in (
        ("binary", "recoil"),
        ("kind", "function"),
        ("pipeline_class", "authored"),
        ("ownership_state", "primary-owned"),
        ("address", _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_CALLER_START),
        ("end_exclusive", _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_CALLER_END_EXCLUSIVE),
        ("extent_state", "known"),
        ("size", 0x3F0),
        ("navigation_name", "HudUiMgrObjective::StartHide"),
        ("output_section_id", "recoil:section:.text"),
        ("physical_block_id", "recoil:block:0x404ca0"),
    ):
        require(
            f"caller_symbol.{field}",
            caller_symbol.get(field),
            expected_value,
        )
    require(
        "caller_symbol.selected_target_membership",
        exact_selected_target_membership(
            caller_symbol.get("verification_target_ids", ()),
            "recoil:vc5-target:hud_404ca0_415ab0_authored_order",
        ),
        True,
    )
    require("caller_source_traceability_mapping", isinstance(caller_trace, Mapping), True)
    assert isinstance(caller_trace, Mapping)
    require("caller_source_traceability.state", caller_trace.get("state"), "resolved")
    if caller_trace.get("reason_code") not in {None, ""}:
        reject(
            "caller_source_traceability.reason_code",
            expected_value=(None, ""),
            observed_value=caller_trace.get("reason_code"),
        )
    require("caller_source_edges_type", isinstance(source_edges, list), True)
    assert isinstance(source_edges, list)
    require("caller_source_edges_count", len(source_edges), 1)
    require("caller_source_edge_mapping", isinstance(source_edges[0], Mapping), True)
    source_edge = source_edges[0]
    assert isinstance(source_edge, Mapping)
    require("caller_source_edge.relation", source_edge.get("relation"), "defines")
    require(
        "caller_source_edge.anchor_id",
        source_edge.get("anchor_id"),
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_CALLER_ANCHOR_ID,
    )
    require(
        "caller_source_edge.emission_context",
        source_edge.get("emission_context"),
        {"translation_unit": "src/Battlesport/hud.cpp"},
    )
    require("aggregate_symbol_mapping", isinstance(aggregate_symbol, Mapping), True)
    assert isinstance(aggregate_symbol, Mapping)
    for field, expected_value in (
        ("binary", "recoil"),
        ("kind", "data"),
        ("disposition", "authored"),
        ("address", _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS),
        ("navigation_name", _cc_catalog.HUD_UI_MGR_AGGREGATE_NAME),
        ("extent_state", "unknown"),
        ("storage_contribution_ids", [_cc_catalog.HUD_UI_MGR_AGGREGATE_STORAGE_ID]),
        ("verification_target_ids", [_cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_ID]),
    ):
        require(
            f"aggregate_symbol.{field}",
            aggregate_symbol.get(field),
            expected_value,
        )
    require(
        "aggregate_storage_address_index",
        indexes.storage_by_address.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS),
        aggregate_storage,
    )
    require(
        "aggregate_storage_provider_membership",
        aggregate_storage in indexes.provider_ids,
        False,
    )

    caller = candidate.caller_definition
    require("candidate_caller_definition_presence", caller is not None, True)
    assert caller is not None

    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_CALLER_START
    ]
    require("target_presence", target is not None, True)
    assert target is not None
    require("target.name", getattr(target, "name", ""), _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_TARGET_NAME)
    require("target.target_binary", getattr(target, "target_binary", ""), "recoil")
    require(
        "target.manifest_path",
        Path(str(getattr(target, "manifest_path", ""))).resolve(),
        _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_TARGET_MANIFEST.resolve(),
    )
    require(
        "target.check_translation_unit_function_order",
        bool(getattr(target, "check_translation_unit_function_order", False)),
        True,
    )
    require(
        "target.hud_source_file_count",
        tuple(getattr(target, "source_files", ())).count(
            _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        ),
        1,
    )
    require("target.start_hide_contribution_count", len(contribution_rows), 1)
    contribution, contribution_row = contribution_rows[0]
    for field, observed_value, expected_value in (
        (
            "contribution.source_from",
            getattr(contribution, "source_from", ""),
            _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH,
        ),
        ("contribution.order_scope", getattr(contribution, "order_scope", ""), "authored"),
        ("contribution_row.symbol", getattr(contribution_row, "symbol", ""), ""),
        (
            "contribution_row.symbol_regex",
            getattr(contribution_row, "symbol_regex", None),
            r"\?StartHide@HudUiMgrObjective@@.*",
        ),
        (
            "candidate_caller_symbol_matches_contribution",
            re.fullmatch(
                r"\?StartHide@HudUiMgrObjective@@.*",
                caller.symbol,
            )
            is not None,
            True,
        ),
        (
            "contribution_row.name",
            getattr(contribution_row, "name", ""),
            "HudUiMgrObjective::StartHide",
        ),
        (
            "contribution_row.pipeline_class",
            getattr(contribution_row, "pipeline_class", ""),
            "authored",
        ),
        (
            "contribution_row.authored_order_role",
            getattr(contribution_row, "authored_order_role", ""),
            "authored-body",
        ),
        (
            "contribution_row.required_presence",
            bool(getattr(contribution_row, "required_presence", False)),
            True,
        ),
        (
            "contribution_row.full_order_gate",
            bool(getattr(contribution_row, "full_order_gate", False)),
            True,
        ),
    ):
        require(field, observed_value, expected_value)

    bar_storage = _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_STORAGE_IDENTITY
    exact_expected = {
        "ordinal": 0,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": bar_storage,
        "slot_displacement": (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_SLOT_DISPLACEMENT
        ),
        "cleanup_bytes": None,
    }
    require("retail_contract.ordinal_0_presence", bool(expected), True)
    observed_contract = expected[0]
    for field, expected_value in exact_expected.items():
        require(
            f"retail_contract.ordinal_0.{field}",
            observed_contract.get(field),
            expected_value,
        )
    require(
        "retail_contract.ordinal_0.fields",
        tuple(observed_contract),
        tuple(exact_expected),
    )

    local_static_definitions = caller.local_static_definitions
    positive_one_literals = tuple(
        item
        for item in local_static_definitions
        if re.fullmatch(r"\$T[0-9]+", item.name) is not None
        and item.section_name == ".rdata"
        and item.section_number == 4
        and item.value == 0x378
        and item.symbol_type == 0
        and item.storage_class == IMAGE_SYM_CLASS_STATIC
        and item.aux_count == 0
        and item.data == bytes.fromhex("00 00 80 3f")
    )
    require("positive_one_literal.match_count", len(positive_one_literals), 1)
    positive_one_literal = positive_one_literals[0]
    require(
        "positive_one_literal.name_count",
        sum(
            item.name == positive_one_literal.name
            for item in local_static_definitions
        ),
        1,
    )
    exact_prefix = bytes.fromhex(
        "d9 05 98 06 00 00 "
        "d8 05 00 00 00 00 "
        "a1 94 06 00 00 "
        "83 ec 08 "
        "48 "
        "56 "
        "d9 1d 98 06 00 00 "
        "0f 84 e4 01 00 00 "
        "48 "
        "0f 84 a7 01 00 00 "
        "48 "
        "0f 85 8a 03 00 00 "
        "d9 05 98 06 00 00 "
        "d8 1d 9c 06 00 00 "
        "df e0 "
        "f6 c4 01 "
        "0f 84 e5 00 00 00 "
        "d9 05 98 06 00 00 "
        "d8 35 9c 06 00 00 "
        "b9 78 09 00 00 "
        "d8 2d 00 00 00 00 "
        "d9 05 b0 0a 00 00 "
        "d9 c9 "
        "d9 5c 24 08 "
        "d8 4c 24 08 "
        "d8 05 b8 09 00 00 "
        "d9 5c 24 04 "
        "8b 44 24 04 "
        "a3 c4 09 00 00 "
        "8b d0 "
        "a1 78 09 00 00 "
        "89 15 d0 09 00 00 "
        "ff 50 20"
    )
    relocation_specs = (
        (0x02, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x698),
        (0x08, _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_OBJECT_SYMBOL, 0x0),
        (0x0D, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x694),
        (0x18, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x698),
        (0x32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x698),
        (0x38, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x69C),
        (0x49, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x698),
        (0x4F, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x69C),
        (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_RECEIVER_OFFSET + 1,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_DISPLACEMENT,
        ),
        (0x5A, positive_one_literal.name, 0x0),
        (0x60, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0xAB0),
        (0x70, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x9B8),
        (0x7D, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x9C4),
        (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_LOAD_OFFSET + 1,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_DISPLACEMENT,
        ),
        (0x8A, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x9D0),
    )
    prefix_relocations = tuple(
        row
        for row in caller.relocations
        if row.offset < len(exact_prefix)
    )
    expected_mask = {
        index
        for relocation_offset, _, _ in relocation_specs
        for index in range(relocation_offset, relocation_offset + 4)
    }
    require(
        "candidate_caller_definition.symbol",
        caller.symbol,
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_CALLER_SYMBOL,
    )
    require("candidate_caller_definition.data_size", len(caller.data), 0x400)
    require(
        "candidate_caller_definition.prefix_relocation_targets",
        tuple((row.offset, row.symbol_name) for row in prefix_relocations),
        tuple(
            (offset, symbol_name)
            for offset, symbol_name, _ in relocation_specs
        ),
    )
    require(
        "candidate_caller_definition.prefix_relocation_types",
        tuple(row.type for row in prefix_relocations),
        tuple(IMAGE_REL_I386_DIR32 for _ in relocation_specs),
    )
    require(
        "candidate_caller_definition.prefix_relocation_addends",
        tuple(
            struct.unpack_from("<I", caller.data, offset)[0]
            for offset, _, _ in relocation_specs
        ),
        tuple(addend for _, _, addend in relocation_specs),
    )
    require(
        "candidate_caller_definition.prefix_bytes",
        caller.data[: len(exact_prefix)].hex(" "),
        exact_prefix.hex(" "),
    )
    require(
        "candidate_caller_definition.relocation_mask_size",
        len(caller.relocation_mask),
        len(caller.data),
    )
    require(
        "candidate_caller_definition.prefix_relocation_mask",
        {
            index
            for index in range(len(exact_prefix))
            if caller.relocation_mask[index]
        },
        expected_mask,
    )
    external_data = caller.undefined_external_data + caller.defined_external_data
    require(
        "candidate_caller_definition.aggregate_external_count",
        external_data.count(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL),
        1,
    )
    require(
        "candidate_caller_definition.time_external_count",
        external_data.count(_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_OBJECT_SYMBOL),
        1,
    )

    expected_instructions = (
        (0x53, "mov", ("b9", "78", "09", "00", "00")),
        (0x58, "fsubr", ("d8", "2d", "00", "00", "00", "00")),
        (0x5E, "fld", ("d9", "05", "b0", "0a", "00", "00")),
        (0x64, "fxch", ("d9", "c9")),
        (0x66, "fstp", ("d9", "5c", "24", "08")),
        (0x6A, "fmul", ("d8", "4c", "24", "08")),
        (0x6E, "fadd", ("d8", "05", "b8", "09", "00", "00")),
        (0x74, "fstp", ("d9", "5c", "24", "04")),
        (0x78, "mov", ("8b", "44", "24", "04")),
        (0x7C, "mov", ("a3", "c4", "09", "00", "00")),
        (0x81, "mov", ("8b", "d0")),
        (0x83, "mov", ("a1", "78", "09", "00", "00")),
        (0x88, "mov", ("89", "15", "d0", "09", "00", "00")),
        (0x8E, "call", ("ff", "50", "20")),
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
    actual_instructions = tuple(
        (
            offset,
            _cc_cfg._instruction_mnemonic(instruction_by_offset[offset]),
            tuple(
                value.lower()
                for value in instruction_by_offset[offset].bytes
            ),
        )
        for offset, _, _ in expected_instructions
        if offset in instruction_by_offset
    )
    require(
        "candidate_instruction_offset_counts",
        tuple(counts.get(offset, 0) for offset, _, _ in expected_instructions),
        tuple(1 for _ in expected_instructions),
    )

    aggregate = _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
    bar_expressions = {
        (
            f"{aggregate}+"
            f"{_cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_DISPLACEMENT}"
        ),
        (
            f"{aggregate}+0x"
            f"{_cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_DISPLACEMENT:x}"
        ),
    }
    load = _cc_cfg._instruction_operand(
        instruction_by_offset[
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_LOAD_OFFSET
        ]
    ).split(",", 1)
    receiver = _cc_cfg._instruction_operand(
        instruction_by_offset[
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_RECEIVER_OFFSET
        ]
    ).split(",", 1)
    call = instruction_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_CALL_OFFSET
    ]
    require("candidate_load_operand_count", len(load), 2)
    require("candidate_load_register", load[0].strip().lower(), "eax")
    load_expression = _cc_targets._exact_memory_expression(load[1])
    require(
        "candidate_load_storage_expression",
        load_expression in bar_expressions,
        True,
    )
    require("candidate_receiver_operand_count", len(receiver), 2)
    require("candidate_receiver_register", receiver[0].strip().lower(), "ecx")
    receiver_expressions = bar_expressions | {
        f"OFFSETFLAT:{expression}" for expression in bar_expressions
    }
    receiver_expression = _cc_targets._exact_memory_expression(receiver[1])
    require(
        "candidate_receiver_storage_expression",
        receiver_expression in receiver_expressions,
        True,
    )
    require("candidate_call_mnemonic", _cc_cfg._instruction_mnemonic(call), "call")
    call_expression = _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(call))
    require(
        "candidate_call_slot_expression",
        call_expression in {"eax+32", "eax+0x20"},
        True,
    )
    require(
        "candidate_receiver_load_call_order",
        (
            index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_RECEIVER_OFFSET]
            < index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_LOAD_OFFSET]
            < index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_CALL_OFFSET]
        ),
        True,
    )
    require(
        "candidate_instruction_listing",
        actual_instructions,
        expected_instructions,
    )


    load_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_LOAD_OFFSET
    ]
    receiver_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_RECEIVER_OFFSET
    ]
    call_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_CALL_OFFSET
    ]
    eax_clobber_offsets = tuple(
        offsets[index]
        for index in range(load_index + 1, call_index)
        if _cc_instructions.may_clobber_register(candidate.instructions[index], "eax")
    )
    require("candidate_eax_clobber_offsets", eax_clobber_offsets, ())
    ecx_clobber_offsets = tuple(
        offsets[index]
        for index in range(receiver_index + 1, call_index)
        if _cc_instructions.may_clobber_register(candidate.instructions[index], "ecx")
    )
    require("candidate_ecx_clobber_offsets", ecx_clobber_offsets, ())

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    reviewed_window_indices = {
        index_by_offset[offset]
        for offset, _, _ in expected_instructions
    }
    require("candidate_invocation_presence", bool(invocation_indices), True)
    require("candidate_first_invocation_index", invocation_indices[0], call_index)
    require(
        "candidate_call_cleanup_bytes",
        _cc_cfg._cleanup_after(candidate.instructions, call_index),
        None,
    )
    jump_offsets = tuple(
        offsets[index]
        for index in range(load_index, call_index)
        if _cc_cfg._instruction_mnemonic(candidate.instructions[index]).startswith("j")
    )
    require("candidate_reviewed_window_jump_offsets", jump_offsets, ())
    incoming_reviewed_targets = tuple(
        target_index
        for targets in candidate.local_control_flow_targets.values()
        for target_index in targets
        if target_index in reviewed_window_indices
    )
    require(
        "candidate_incoming_reviewed_control_flow_targets",
        incoming_reviewed_targets,
        (),
    )
    require(
        "candidate_local_control_flow_indices",
        candidate.local_control_flow_indices,
        frozenset(),
    )

    absolute_bridges = {
        normalize_address(
            hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_LOAD_OFFSET)
        ): ReviewedAbsoluteStorageLoadBridge(
            register="eax",
            aggregate_symbol=aggregate,
            displacement=_cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_DISPLACEMENT,
            access_width=4,
            storage_identity=aggregate_storage,
        )
    }
    vptr_bridges = {
        normalize_address(
            hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_CALL_OFFSET)
        ): ReviewedVptrStorageBridge(
            register="eax",
            provenance=bar_storage,
            storage_identity=bar_storage,
            slot_displacement=(
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_SLOT_DISPLACEMENT
            ),
            identity_kind="virtual-slot",
        )
    }
    return absolute_bridges, vptr_bridges


def _hud_ui_mgr_objective_start_hide_widget_candidate_vptr_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedAbsoluteStorageLoadBridge],
    dict[str, ReviewedVptrStorageBridge],
]:
    """Bridge only StartHide's first objective-widget SetX call."""
    from _recoil.call_contract.records import (
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedVptrStorageBridge,
    )
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_CALLER_START:
        return {}, {}

    # Reuse the exact tracker/source/manifest authority, current caller
    # identity, bar receiver/vptr proof, and retail ordinal 0.
    _hud_ui_mgr_objective_start_hide_candidate_vptr_bridges(
        expected,
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )

    exact_ftol_expected = {
        "ordinal": 1,
        "form": "call",
        "dispatch": "direct",
        "identity_kind": "provider",
        "target_identity": _cc_catalog.MSVC_FTOL_PROVIDER_IDENTITY,
        "storage_identity": "",
        "slot_displacement": None,
        "cleanup_bytes": None,
    }
    widget_storage = _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_STORAGE_IDENTITY
    exact_widget_expected = {
        "ordinal": 2,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": widget_storage,
        "slot_displacement": (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_SLOT_DISPLACEMENT
        ),
        "cleanup_bytes": None,
    }
    if (
        len(expected) <= 2
        or expected[1] != exact_ftol_expected
        or expected[2] != exact_widget_expected
    ):
        observed = expected[:3]
        raise ValueError(
            "HUD objective StartHide widget-vptr bridge requires the "
            "immutable retail ordinal-1 _ftol provider and ordinal-2 "
            "embedded-widget/slot-0x10/no-cleanup contracts: "
            f"observed={observed!r}"
        )

    caller = candidate.caller_definition
    exact_prefix = bytes.fromhex(
        "d9 05 98 06 00 00 "
        "d8 05 00 00 00 00 "
        "a1 94 06 00 00 "
        "83 ec 08 "
        "48 "
        "56 "
        "d9 1d 98 06 00 00 "
        "0f 84 e4 01 00 00 "
        "48 "
        "0f 84 a7 01 00 00 "
        "48 "
        "0f 85 8a 03 00 00 "
        "d9 05 98 06 00 00 "
        "d8 1d 9c 06 00 00 "
        "df e0 "
        "f6 c4 01 "
        "0f 84 e5 00 00 00 "
        "d9 05 98 06 00 00 "
        "d8 35 9c 06 00 00 "
        "b9 78 09 00 00 "
        "d8 2d 00 00 00 00 "
        "d9 05 b0 0a 00 00 "
        "d9 c9 "
        "d9 5c 24 08 "
        "d8 4c 24 08 "
        "d8 05 b8 09 00 00 "
        "d9 5c 24 04 "
        "8b 44 24 04 "
        "a3 c4 09 00 00 "
        "8b d0 "
        "a1 78 09 00 00 "
        "89 15 d0 09 00 00 "
        "ff 50 20 "
        "d9 44 24 04 "
        "e8 00 00 00 00 "
        "8b 15 ac 06 00 00 "
        "48 "
        "b9 ac 06 00 00 "
        "50 "
        "ff 52 10"
    )
    positive_one_literal_names = tuple(
        item.name
        for item in caller.local_static_definitions
        if re.fullmatch(r"\$T[0-9]+", item.name) is not None
        and item.section_name == ".rdata"
        and item.section_number == 4
        and item.value == 0x378
        and item.symbol_type == 0
        and item.storage_class == IMAGE_SYM_CLASS_STATIC
        and item.aux_count == 0
        and item.data == bytes.fromhex("00 00 80 3f")
    ) if caller is not None else ()
    if len(positive_one_literal_names) != 1:
        raise ValueError(
            "HUD objective StartHide widget-vptr bridge rejects exact current "
            "governed local-static positive-one literal"
        )
    relocation_specs = (
        (0x02, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x698),
        (
            0x08,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_OBJECT_SYMBOL,
            0x0,
        ),
        (0x0D, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x694),
        (0x18, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x698),
        (0x32, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x698),
        (0x38, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x69C),
        (0x49, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x698),
        (0x4F, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x69C),
        (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_RECEIVER_OFFSET + 1,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_DISPLACEMENT,
        ),
        (
            0x5A,
            IMAGE_REL_I386_DIR32,
            positive_one_literal_names[0],
            0x0,
        ),
        (0x60, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0xAB0),
        (0x70, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x9B8),
        (0x7D, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x9C4),
        (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_LOAD_OFFSET + 1,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_DISPLACEMENT,
        ),
        (0x8A, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x9D0),
        (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_FTOL_CALL_OFFSET + 1,
            IMAGE_REL_I386_REL32,
            _cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL,
            0x0,
        ),
        (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_LOAD_OFFSET + 2,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_DISPLACEMENT,
        ),
        (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_RECEIVER_OFFSET + 1,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_DISPLACEMENT,
        ),
    )
    prefix_relocations = (
        tuple(
            row
            for row in caller.relocations
            if row.offset < len(exact_prefix)
        )
        if caller is not None
        else ()
    )
    expected_mask = {
        index
        for relocation_offset, _, _, _ in relocation_specs
        for index in range(relocation_offset, relocation_offset + 4)
    }
    if (
        caller is None
        or caller.symbol != _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_CALLER_SYMBOL
        or len(caller.data) != 0x400
        or caller.data[: len(exact_prefix)] != exact_prefix
        or tuple(
            (row.offset, row.type, row.symbol_name)
            for row in prefix_relocations
        )
        != tuple(
            (offset, relocation_type, symbol_name)
            for offset, relocation_type, symbol_name, _ in relocation_specs
        )
        or any(
            struct.unpack_from("<I", caller.data, offset)[0] != addend
            for offset, _, _, addend in relocation_specs
        )
        or len(caller.relocation_mask) != len(caller.data)
        or {
            index
            for index in range(len(exact_prefix))
            if caller.relocation_mask[index]
        }
        != expected_mask
        or (
            caller.undefined_external_data + caller.defined_external_data
        ).count(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
        != 1
        or (
            caller.undefined_external_data + caller.defined_external_data
        ).count(_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_OBJECT_SYMBOL)
        != 1
        or (
            caller.undefined_external_functions
            + caller.defined_external_functions
        ).count(_cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL)
        != 1
    ):
        raise ValueError(
            "HUD objective StartHide widget-vptr bridge rejects exact 0x400 "
            "cumulative caller prefix, eighteen DIR32/REL32 target/addend/"
            "order rows, external identities, or relocation-mask drift"
        )

    expected_instructions = (
        (0x91, "fld", ("d9", "44", "24", "04")),
        (0x95, "call", ("e8", "00", "00", "00", "00")),
        (0x9A, "mov", ("8b", "15", "ac", "06", "00", "00")),
        (0xA0, "dec", ("48",)),
        (0xA1, "mov", ("b9", "ac", "06", "00", "00")),
        (0xA6, "push", ("50",)),
        (0xA7, "call", ("ff", "52", "10")),
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
    actual_instructions = tuple(
        (
            offset,
            _cc_cfg._instruction_mnemonic(instruction_by_offset[offset]),
            tuple(
                value.lower()
                for value in instruction_by_offset[offset].bytes
            ),
        )
        for offset, _, _ in expected_instructions
        if offset in instruction_by_offset
    )
    if (
        actual_instructions != expected_instructions
        or any(counts.get(offset) != 1 for offset, _, _ in expected_instructions)
    ):
        raise ValueError(
            "HUD objective StartHide widget-vptr bridge requires the exact "
            "candidate _ftol/EDX-vptr/EAX-argument/ECX-receiver/call listing "
            "and instruction bytes"
        )

    aggregate = _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
    widget_expressions = {
        (
            f"{aggregate}+"
            f"{_cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_DISPLACEMENT}"
        ),
        (
            f"{aggregate}+0x"
            f"{_cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_DISPLACEMENT:x}"
        ),
    }
    ftol_call = instruction_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_FTOL_CALL_OFFSET
    ]
    load = _cc_cfg._instruction_operand(
        instruction_by_offset[
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_LOAD_OFFSET
        ]
    ).split(",", 1)
    adjust = instruction_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_ARGUMENT_ADJUST_OFFSET
    ]
    argument = instruction_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_ARGUMENT_OFFSET
    ]
    receiver = _cc_cfg._instruction_operand(
        instruction_by_offset[
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_RECEIVER_OFFSET
        ]
    ).split(",", 1)
    call = instruction_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_CALL_OFFSET
    ]
    if (
        _cc_cfg._instruction_mnemonic(ftol_call) != "call"
        or _cc_cfg._instruction_operand(ftol_call).strip()
        != _cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL
        or len(load) != 2
        or load[0].strip().lower() != "edx"
        or _cc_targets._exact_memory_expression(load[1]) not in widget_expressions
        or _cc_cfg._instruction_mnemonic(adjust) != "dec"
        or _cc_cfg._instruction_operand(adjust).strip().lower() != "eax"
        or _cc_cfg._instruction_mnemonic(argument) != "push"
        or _cc_cfg._instruction_operand(argument).strip().lower() != "eax"
        or len(receiver) != 2
        or receiver[0].strip().lower() != "ecx"
        or _cc_targets._exact_memory_expression(receiver[1])
        not in (
            widget_expressions
            | {
                f"OFFSETFLAT:{expression}"
                for expression in widget_expressions
            }
        )
        or _cc_cfg._instruction_mnemonic(call) != "call"
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(call))
        not in {"edx+16", "edx+0x10"}
        or not (
            index_by_offset[
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_FTOL_CALL_OFFSET
            ]
            < index_by_offset[
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_LOAD_OFFSET
            ]
            < index_by_offset[
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_ARGUMENT_ADJUST_OFFSET
            ]
            < index_by_offset[
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_RECEIVER_OFFSET
            ]
            < index_by_offset[
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_ARGUMENT_OFFSET
            ]
            < index_by_offset[
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_CALL_OFFSET
            ]
        )
    ):
        raise ValueError(
            "HUD objective StartHide widget-vptr bridge rejects exact "
            "_ftol-result/EAX-minus-one argument/EDX-vptr/ECX-receiver/slot "
            "reaching-definition drift"
        )


    ftol_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_FTOL_CALL_OFFSET
    ]
    load_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_LOAD_OFFSET
    ]
    adjust_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_ARGUMENT_ADJUST_OFFSET
    ]
    argument_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_ARGUMENT_OFFSET
    ]
    receiver_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_RECEIVER_OFFSET
    ]
    call_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_CALL_OFFSET
    ]
    if (
        any(
            _cc_instructions.may_clobber_register(candidate.instructions[index], "eax")
            for index in range(ftol_index + 1, adjust_index)
        )
        or any(
            _cc_instructions.may_clobber_register(candidate.instructions[index], "eax")
            for index in range(adjust_index + 1, argument_index)
        )
        or any(
            _cc_instructions.may_clobber_register(candidate.instructions[index], "edx")
            for index in range(load_index + 1, call_index)
        )
        or any(
            _cc_instructions.may_clobber_register(candidate.instructions[index], "ecx")
            for index in range(receiver_index + 1, call_index)
        )
    ):
        raise ValueError(
            "HUD objective StartHide widget-vptr bridge rejects EAX argument, "
            "EDX vptr, or ECX receiver clobber on the reviewed phase-1 path"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    reviewed_window_indices = {
        index_by_offset[offset]
        for offset, _, _ in expected_instructions
    }
    if (
        len(invocation_indices) <= 2
        or tuple(invocation_indices[:3])
        != (
            index_by_offset[
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_CALL_OFFSET
            ],
            ftol_index,
            call_index,
        )
        or _cc_cfg._cleanup_after(candidate.instructions, ftol_index) is not None
        or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
        or any(
            _cc_cfg._instruction_mnemonic(candidate.instructions[index]).startswith(
                "j"
            )
            for index in range(ftol_index, call_index)
        )
        or any(
            target in reviewed_window_indices
            for targets in candidate.local_control_flow_targets.values()
            for target in targets
        )
        or candidate.local_control_flow_indices
    ):
        raise ValueError(
            "HUD objective StartHide widget-vptr bridge rejects call order, "
            "cleanup, straight-line phase-1 uniqueness, or alternate local "
            "control-flow drift"
        )

    absolute_bridges = {
        normalize_address(
            hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_LOAD_OFFSET)
        ): ReviewedAbsoluteStorageLoadBridge(
            register="edx",
            aggregate_symbol=aggregate,
            displacement=_cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_DISPLACEMENT,
            access_width=4,
            storage_identity=f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}",
        )
    }
    vptr_bridges = {
        normalize_address(
            hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_CALL_OFFSET)
        ): ReviewedVptrStorageBridge(
            register="edx",
            provenance=widget_storage,
            storage_identity=widget_storage,
            slot_displacement=(
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_SLOT_DISPLACEMENT
            ),
            identity_kind="virtual-slot",
        )
    }
    return absolute_bridges, vptr_bridges


def _hud_ui_mgr_objective_start_hide_widget_center_candidate_vptr_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedAbsoluteStorageLoadBridge],
    dict[str, ReviewedVptrStorageBridge],
]:
    """Bridge only StartHide's first objective-widget GetCenterX call."""
    from _recoil.call_contract.records import (
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedVptrStorageBridge,
    )
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_CALLER_START:
        return {}, {}

    # Preserve the exact caller authority and reviewed ordinal-0..2 bridge
    # contracts before extending provenance to the later slot-0x64 call.
    _hud_ui_mgr_objective_start_hide_widget_candidate_vptr_bridges(
        expected,
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )

    widget_storage = _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_STORAGE_IDENTITY
    exact_expected = {
        "ordinal": 6,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": widget_storage,
        "slot_displacement": (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_CENTER_SLOT_DISPLACEMENT
        ),
        "cleanup_bytes": None,
    }
    if len(expected) <= 6 or expected[6] != exact_expected:
        observed = expected[6] if len(expected) > 6 else None
        raise ValueError(
            "HUD objective StartHide widget-center bridge requires the "
            "immutable retail ordinal-6 embedded-widget/slot-0x64/"
            "no-cleanup contract: "
            f"observed={observed!r}"
        )

    caller = candidate.caller_definition
    window_start = 0xC3
    exact_window = bytes.fromhex(
        "a1 e8 06 00 00 "
        "85 c0 "
        "74 06 "
        "0f bf 70 04 "
        "eb 02 "
        "33 f6 "
        "a1 ac 06 00 00 "
        "b9 ac 06 00 00 "
        "ff 50 64"
    )
    window_end = window_start + len(exact_window)
    relocation_specs = (
        (
            0xC4,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x6E8,
        ),
        (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_CENTER_LOAD_OFFSET + 1,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_DISPLACEMENT,
        ),
        (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_CENTER_RECEIVER_OFFSET + 1,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_DISPLACEMENT,
        ),
    )
    window_relocations = (
        tuple(
            row
            for row in caller.relocations
            if window_start <= row.offset < window_end
        )
        if caller is not None
        else ()
    )
    expected_mask = {
        index
        for relocation_offset, _, _, _ in relocation_specs
        for index in range(relocation_offset, relocation_offset + 4)
    }
    if (
        caller is None
        or caller.symbol != _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_CALLER_SYMBOL
        or len(caller.data) != 0x400
        or caller.data[window_start:window_end] != exact_window
        or tuple(
            (row.offset, row.type, row.symbol_name)
            for row in window_relocations
        )
        != tuple(
            (offset, relocation_type, symbol_name)
            for offset, relocation_type, symbol_name, _ in relocation_specs
        )
        or any(
            struct.unpack_from("<I", caller.data, offset)[0] != addend
            for offset, _, _, addend in relocation_specs
        )
        or len(caller.relocation_mask) != len(caller.data)
        or {
            index
            for index in range(window_start, window_end)
            if caller.relocation_mask[index]
        }
        != expected_mask
        or (
            caller.undefined_external_data + caller.defined_external_data
        ).count(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
        != 1
    ):
        raise ValueError(
            "HUD objective StartHide widget-center bridge rejects the exact "
            "0xc3..0xe1 caller window, three DIR32 target/addend/order rows, "
            "aggregate external identity, or relocation-mask drift"
        )

    expected_instructions = (
        (0xC3, "mov", ("a1", "e8", "06", "00", "00")),
        (0xC8, "test", ("85", "c0")),
        (0xCA, "je", ("74", "06")),
        (0xCC, "movsx", ("0f", "bf", "70", "04")),
        (0xD0, "jmp", ("eb", "02")),
        (0xD2, "xor", ("33", "f6")),
        (0xD4, "mov", ("a1", "ac", "06", "00", "00")),
        (0xD9, "mov", ("b9", "ac", "06", "00", "00")),
        (0xDE, "call", ("ff", "50", "64")),
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
    actual_instructions = tuple(
        (
            offset,
            _cc_cfg._instruction_mnemonic(instruction_by_offset[offset]),
            tuple(
                value.lower()
                for value in instruction_by_offset[offset].bytes
            ),
        )
        for offset, _, _ in expected_instructions
        if offset in instruction_by_offset
    )
    if (
        actual_instructions != expected_instructions
        or any(counts.get(offset) != 1 for offset, _, _ in expected_instructions)
    ):
        raise ValueError(
            "HUD objective StartHide widget-center bridge requires the exact "
            "image-null-width CFG, EAX-vptr, ECX-receiver, and slot-0x64 "
            "candidate listing and instruction bytes"
        )

    aggregate = _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
    widget_expressions = {
        (
            f"{aggregate}+"
            f"{_cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_DISPLACEMENT}"
        ),
        (
            f"{aggregate}+0x"
            f"{_cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_DISPLACEMENT:x}"
        ),
    }
    load = _cc_cfg._instruction_operand(
        instruction_by_offset[
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_CENTER_LOAD_OFFSET
        ]
    ).split(",", 1)
    receiver = _cc_cfg._instruction_operand(
        instruction_by_offset[
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_CENTER_RECEIVER_OFFSET
        ]
    ).split(",", 1)
    call = instruction_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_CENTER_CALL_OFFSET
    ]
    if (
        len(load) != 2
        or load[0].strip().lower() != "eax"
        or _cc_targets._exact_memory_expression(load[1]) not in widget_expressions
        or len(receiver) != 2
        or receiver[0].strip().lower() != "ecx"
        or _cc_targets._exact_memory_expression(receiver[1])
        not in (
            widget_expressions
            | {
                f"OFFSETFLAT:{expression}"
                for expression in widget_expressions
            }
        )
        or _cc_cfg._instruction_mnemonic(call) != "call"
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(call))
        not in {"eax+100", "eax+0x64"}
        or not (
            index_by_offset[
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_CENTER_LOAD_OFFSET
            ]
            < index_by_offset[
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_CENTER_RECEIVER_OFFSET
            ]
            < index_by_offset[
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_CENTER_CALL_OFFSET
            ]
        )
    ):
        raise ValueError(
            "HUD objective StartHide widget-center bridge rejects exact "
            "EAX-vptr/ECX-receiver/slot-0x64 reaching-definition drift"
        )

    load_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_CENTER_LOAD_OFFSET
    ]
    receiver_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_CENTER_RECEIVER_OFFSET
    ]
    call_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_CENTER_CALL_OFFSET
    ]
    if (
        any(
            _cc_instructions.may_clobber_register(candidate.instructions[index], 'eax')
            for index in range(load_index + 1, call_index)
        )
        or any(
            _cc_instructions.may_clobber_register(candidate.instructions[index], 'ecx')
            for index in range(receiver_index + 1, call_index)
        )
    ):
        raise ValueError(
            "HUD objective StartHide widget-center bridge rejects EAX vptr "
            "or ECX receiver clobber"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    expected_call_offsets = (
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_CALL_OFFSET,
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_FTOL_CALL_OFFSET,
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_CALL_OFFSET,
        0xAA,
        0xAF,
        0xBE,
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_CENTER_CALL_OFFSET,
    )
    if (
        len(invocation_indices) <= 6
        or tuple(invocation_indices[:7])
        != tuple(index_by_offset[offset] for offset in expected_call_offsets)
        or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
    ):
        raise ValueError(
            "HUD objective StartHide widget-center bridge rejects immutable "
            "retail call order, call form, or caller cleanup drift"
        )

    absolute_bridges = {
        normalize_address(
            hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_CENTER_LOAD_OFFSET)
        ): ReviewedAbsoluteStorageLoadBridge(
            register="eax",
            aggregate_symbol=aggregate,
            displacement=_cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_DISPLACEMENT,
            access_width=4,
            storage_identity=f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}",
        )
    }
    vptr_bridges = {
        normalize_address(
            hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_CENTER_CALL_OFFSET)
        ): ReviewedVptrStorageBridge(
            register="eax",
            provenance=widget_storage,
            storage_identity=widget_storage,
            slot_displacement=(
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_CENTER_SLOT_DISPLACEMENT
            ),
            identity_kind="virtual-slot",
        )
    }
    return absolute_bridges, vptr_bridges


def _hud_ui_mgr_objective_start_hide_topology_candidate_vptr_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedAbsoluteStorageLoadBridge],
    dict[str, ReviewedVptrStorageBridge],
]:
    """Bridge StartHide only after its complete high-noise topology matches."""
    from _recoil.call_contract.records import (
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedVptrStorageBridge,
    )
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_CALLER_START:
        return {}, {}

    # Retain the exact caller/source authority and the cumulative first-seven
    # call proof before extending provenance through the remaining switch.
    _hud_ui_mgr_objective_start_hide_widget_center_candidate_vptr_bridges(
        expected,
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )

    aggregate_storage = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"

    def direct_contract(ordinal: int, identity: str) -> dict[str, Any]:
        return {
            "ordinal": ordinal,
            "form": "call",
            "dispatch": "direct",
            "identity_kind": (
                "provider"
                if identity == _cc_catalog.MSVC_FTOL_PROVIDER_IDENTITY
                else "direct"
            ),
            "target_identity": identity,
            "storage_identity": "",
            "slot_displacement": None,
            "cleanup_bytes": None,
        }

    def vptr_contract(
        ordinal: int,
        storage: str,
        slot: int,
    ) -> dict[str, Any]:
        return {
            "ordinal": ordinal,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": storage,
            "slot_displacement": slot,
            "cleanup_bytes": None,
        }

    bar_storage = _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_STORAGE_IDENTITY
    widget_storage = _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_STORAGE_IDENTITY
    sensor_storage = f"load({aggregate_storage}+0x768)"
    overlay_storage = f"load({aggregate_storage}+0xce0)"
    summary_storage = f"load(load({aggregate_storage}+0x824))"
    description_storage = f"load(load({aggregate_storage}+0x974))"
    exact_expected = [
        vptr_contract(0, bar_storage, 0x20),
        direct_contract(1, _cc_catalog.MSVC_FTOL_PROVIDER_IDENTITY),
        vptr_contract(2, widget_storage, 0x10),
        direct_contract(
            3,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_UPDATE_METER_IDENTITY,
        ),
        direct_contract(
            4,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_GET_HUD_TYPE_IDENTITY,
        ),
        direct_contract(
            5,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_UPDATE_DIRTY_IDENTITY,
        ),
        vptr_contract(6, widget_storage, 0x64),
        direct_contract(
            7,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_DRAW_IDENTITY,
        ),
        direct_contract(8, _cc_catalog.MSVC_FTOL_PROVIDER_IDENTITY),
        vptr_contract(9, widget_storage, 0x10),
        direct_contract(
            10,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_UPDATE_METER_IDENTITY,
        ),
        direct_contract(
            11,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_GET_HUD_TYPE_IDENTITY,
        ),
        direct_contract(
            12,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_UPDATE_DIRTY_IDENTITY,
        ),
        vptr_contract(13, widget_storage, 0x64),
        vptr_contract(14, bar_storage, 0x60),
        vptr_contract(15, overlay_storage, 0x60),
        vptr_contract(16, summary_storage, 0x20),
        vptr_contract(17, description_storage, 0x20),
        vptr_contract(18, bar_storage, 0x20),
        vptr_contract(19, sensor_storage, 0x20),
        vptr_contract(20, bar_storage, 0x20),
        direct_contract(21, _cc_catalog.MSVC_FTOL_PROVIDER_IDENTITY),
        vptr_contract(22, widget_storage, 0x10),
        direct_contract(
            23,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_UPDATE_METER_IDENTITY,
        ),
        vptr_contract(24, widget_storage, 0x64),
        direct_contract(
            25,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_DRAW_IDENTITY,
        ),
        vptr_contract(26, sensor_storage, 0x60),
        direct_contract(
            27,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_DRAW_IDENTITY,
        ),
        vptr_contract(28, bar_storage, 0x20),
        direct_contract(29, _cc_catalog.MSVC_FTOL_PROVIDER_IDENTITY),
        vptr_contract(30, widget_storage, 0x10),
        direct_contract(
            31,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_UPDATE_METER_IDENTITY,
        ),
        vptr_contract(32, widget_storage, 0x64),
        vptr_contract(33, summary_storage, 0x60),
        vptr_contract(34, description_storage, 0x60),
        vptr_contract(35, sensor_storage, 0x60),
        direct_contract(
            36,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_BEGIN_IDENTITY,
        ),
    ]
    if list(expected) != exact_expected:
        raise ValueError(
            "HUD objective StartHide topology bridge requires the exact "
            "retail 37-call contract, including three DrawNoiseRect calls, "
            "the phase-1 no-dirty-call branch, and no selector/helper call"
        )

    caller = candidate.caller_definition
    local_static_definitions = (
        caller.local_static_definitions if caller is not None else ()
    )
    positive_one_literals = tuple(
        item
        for item in local_static_definitions
        if re.fullmatch(r"\$T[0-9]+", item.name) is not None
        and item.section_name == ".rdata"
        and item.section_number == 4
        and item.value == 0x378
        and item.symbol_type == 0
        and item.storage_class == IMAGE_SYM_CLASS_STATIC
        and item.aux_count == 0
        and item.data == bytes.fromhex("00 00 80 3f")
    )
    positive_two_literals = tuple(
        item
        for item in local_static_definitions
        if re.fullmatch(r"\$T[0-9]+", item.name) is not None
        and item.section_name == ".rdata"
        and item.section_number == 4
        and item.value == 0x3B0
        and item.symbol_type == 0
        and item.storage_class == IMAGE_SYM_CLASS_STATIC
        and item.aux_count == 0
        and item.data == bytes.fromhex("00 00 00 40")
    )
    if len(positive_one_literals) != 1 or len(positive_two_literals) != 1:
        raise ValueError(
            "HUD objective StartHide topology bridge rejects the exact "
            "governed positive-one/positive-two local-static literals"
        )
    positive_one = positive_one_literals[0].name
    positive_two = positive_two_literals[0].name

    phase_three_window_start = 0xF5
    phase_three_window = bytes.fromhex(
        "d9 44 24 08 "
        "dc c0 "
        "d9 54 24 04 "
        "d8 1d 00 00 00 00 "
        "df e0 "
        "f6 c4 01 "
        "74 19 "
        "d9 44 24 04 "
        "83 ec 08 "
        "b9 e0 0b 00 00 "
        "dd 1c 24 "
        "e8 00 00 00 00 "
        "e9 95 02 00 00 "
        "6a 00 "
        "e9 b7 01 00 00"
    )
    phase_one_window_start = 0x2B1
    phase_one_window = bytes.fromhex(
        "d9 44 24 08 "
        "dc c0 "
        "d9 54 24 04 "
        "d8 1d 00 00 00 00 "
        "df e0 "
        "f6 c4 01 "
        "74 19 "
        "d9 44 24 04 "
        "83 ec 08 "
        "b9 e0 0b 00 00 "
        "dd 1c 24 "
        "e8 00 00 00 00 "
        "e9 d9 00 00 00 "
        "6a 01 "
        "8b 15 68 07 00 00 "
        "b9 68 07 00 00 "
        "ff 52 60 "
        "d9 05 00 00 00 00 "
        "d8 64 24 04 "
        "83 ec 08 "
        "b9 e0 0b 00 00 "
        "dd 1c 24 "
        "e8 00 00 00 00 "
        "e9 aa 00 00 00"
    )
    phase_three_window_end = (
        phase_three_window_start + len(phase_three_window)
    )
    phase_one_window_end = phase_one_window_start + len(phase_one_window)
    topology_relocation_specs = (
        (0x101, IMAGE_REL_I386_DIR32, positive_one, 0x0),
        (
            0x114,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0xBE0,
        ),
        (
            0x11C,
            IMAGE_REL_I386_REL32,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_DRAW_SYMBOL,
            0x0,
        ),
        (0x2BD, IMAGE_REL_I386_DIR32, positive_one, 0x0),
        (
            0x2D0,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0xBE0,
        ),
        (
            0x2D8,
            IMAGE_REL_I386_REL32,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_DRAW_SYMBOL,
            0x0,
        ),
        (
            0x2E5,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x768,
        ),
        (
            0x2EA,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x768,
        ),
        (0x2F3, IMAGE_REL_I386_DIR32, positive_two, 0x0),
        (
            0x2FF,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0xBE0,
        ),
        (
            0x307,
            IMAGE_REL_I386_REL32,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_DRAW_SYMBOL,
            0x0,
        ),
    )
    topology_relocations = (
        tuple(
            row
            for row in caller.relocations
            if (
                phase_three_window_start
                <= row.offset
                < phase_three_window_end
            )
            or phase_one_window_start <= row.offset < phase_one_window_end
        )
        if caller is not None
        else ()
    )
    expected_mask = {
        index
        for relocation_offset, _, _, _ in topology_relocation_specs
        for index in range(relocation_offset, relocation_offset + 4)
    }
    if (
        caller is None
        or caller.symbol != _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_CALLER_SYMBOL
        or len(caller.data) != 0x400
        or caller.data[
            phase_three_window_start:phase_three_window_end
        ]
        != phase_three_window
        or caller.data[phase_one_window_start:phase_one_window_end]
        != phase_one_window
        or tuple(
            (row.offset, row.type, row.symbol_name)
            for row in topology_relocations
        )
        != tuple(
            (offset, relocation_type, symbol_name)
            for offset, relocation_type, symbol_name, _ in (
                topology_relocation_specs
            )
        )
        or any(
            struct.unpack_from("<I", caller.data, offset)[0] != addend
            for offset, _, _, addend in topology_relocation_specs
        )
        or len(caller.relocation_mask) != len(caller.data)
        or {
            index
            for index in (
                *range(
                    phase_three_window_start,
                    phase_three_window_end,
                ),
                *range(phase_one_window_start, phase_one_window_end),
            )
            if caller.relocation_mask[index]
        }
        != expected_mask
        or (
            caller.undefined_external_functions
            + caller.defined_external_functions
        ).count(_cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_DRAW_SYMBOL)
        != 1
    ):
        raise ValueError(
            "HUD objective StartHide topology bridge rejects the exact "
            "phase-3/phase-1 low/high-noise windows, three DrawNoiseRect "
            "REL32 rows, shared-sensor DIR32 rows, literals, externals, or "
            "relocation-mask topology"
        )

    invocation_offsets = (
        0x8E,
        0x95,
        0xA7,
        0xAA,
        0xAF,
        0xBE,
        0xDE,
        0x11B,
        0x150,
        0x160,
        0x163,
        0x168,
        0x177,
        0x198,
        0x1AE,
        0x1BE,
        0x1D8,
        0x1E3,
        0x1F0,
        0x1FE,
        0x25E,
        0x265,
        0x277,
        0x27A,
        0x29A,
        0x2D7,
        0x2EE,
        0x306,
        0x34A,
        0x351,
        0x363,
        0x366,
        0x386,
        0x39A,
        0x3A7,
        0x3B7,
        0x3E0,
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
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    if (
        len(invocation_indices) != 37
        or any(offset not in index_by_offset for offset in invocation_offsets)
        or tuple(invocation_indices)
        != tuple(index_by_offset[offset] for offset in invocation_offsets)
        or any(
            _cc_cfg._cleanup_after(candidate.instructions, index) is not None
            for index in invocation_indices
        )
    ):
        raise ValueError(
            "HUD objective StartHide topology bridge rejects the exact "
            "37-call offsets/order/form, missing or duplicate DrawNoiseRect, "
            "selector/helper insertion, or caller-cleanup drift"
        )

    instruction_by_offset = {
        offset: candidate.instructions[index]
        for offset, index in index_by_offset.items()
    }
    direct_specs = (
        (0x11B, _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_DRAW_SYMBOL),
        (0x2D7, _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_DRAW_SYMBOL),
        (0x306, _cc_catalog.HUD_UI_MGR_OBJECTIVE_START_HIDE_DRAW_SYMBOL),
    )
    if any(
        _cc_cfg._instruction_mnemonic(instruction_by_offset[offset]) != "call"
        or _cc_cfg._instruction_operand(instruction_by_offset[offset]).strip()
        != symbol
        or tuple(
            value.lower()
            for value in instruction_by_offset[offset].bytes
        )
        != ("e8", "00", "00", "00", "00")
        for offset, symbol in direct_specs
    ):
        raise ValueError(
            "HUD objective StartHide topology bridge rejects the exact three "
            "direct DrawNoiseRect callsites"
        )

    load_specs = (
        (0x156, "eax", 0x6AC, ("a1", "ac", "06", "00", "00")),
        (0x18D, "edx", 0x6AC, ("8b", "15", "ac", "06", "00", "00")),
        (0x1A7, "eax", 0x978, ("a1", "78", "09", "00", "00")),
        (0x1B1, "edx", 0xCE0, ("8b", "15", "e0", "0c", "00", "00")),
        (0x1D0, "ecx", 0x824, ("8b", "0d", "24", "08", "00", "00")),
        (0x1DB, "ecx", 0x974, ("8b", "0d", "74", "09", "00", "00")),
        (0x1E6, "eax", 0x978, ("a1", "78", "09", "00", "00")),
        (0x1F3, "edx", 0x768, ("8b", "15", "68", "07", "00", "00")),
        (0x253, "eax", 0x978, ("a1", "78", "09", "00", "00")),
        (0x26A, "edx", 0x6AC, ("8b", "15", "ac", "06", "00", "00")),
        (0x290, "eax", 0x6AC, ("a1", "ac", "06", "00", "00")),
        (0x2E3, "edx", 0x768, ("8b", "15", "68", "07", "00", "00")),
        (0x31C, "eax", 0x978, ("a1", "78", "09", "00", "00")),
        (0x356, "edx", 0x6AC, ("8b", "15", "ac", "06", "00", "00")),
        (0x37C, "eax", 0x6AC, ("a1", "ac", "06", "00", "00")),
        (0x389, "ecx", 0x824, ("8b", "0d", "24", "08", "00", "00")),
        (0x39D, "ecx", 0x974, ("8b", "0d", "74", "09", "00", "00")),
        (0x3AA, "edx", 0x768, ("8b", "15", "68", "07", "00", "00")),
    )
    call_specs = (
        (0x160, "eax", 0x10, widget_storage),
        (0x198, "edx", 0x64, widget_storage),
        (0x1AE, "eax", 0x60, bar_storage),
        (0x1BE, "edx", 0x60, overlay_storage),
        (0x1D8, "eax", 0x20, summary_storage),
        (0x1E3, "edx", 0x20, description_storage),
        (0x1F0, "eax", 0x20, bar_storage),
        (0x1FE, "edx", 0x20, sensor_storage),
        (0x25E, "eax", 0x20, bar_storage),
        (0x277, "edx", 0x10, widget_storage),
        (0x29A, "eax", 0x64, widget_storage),
        (0x2EE, "edx", 0x60, sensor_storage),
        (0x34A, "eax", 0x20, bar_storage),
        (0x363, "edx", 0x10, widget_storage),
        (0x386, "eax", 0x64, widget_storage),
        (0x39A, "edx", 0x60, summary_storage),
        (0x3A7, "eax", 0x60, description_storage),
        (0x3B7, "edx", 0x60, sensor_storage),
    )
    if any(
        counts.get(offset) != 1
        or _cc_cfg._instruction_mnemonic(instruction_by_offset[offset]) != "mov"
        or tuple(
            value.lower()
            for value in instruction_by_offset[offset].bytes
        )
        != encoded
        for offset, _, _, encoded in load_specs
    ) or any(
        counts.get(offset) != 1
        or _cc_cfg._instruction_mnemonic(instruction_by_offset[offset]) != "call"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(instruction_by_offset[offset])
        )
        not in {f"{register}+{slot}", f"{register}+0x{slot:x}"}
        or tuple(
            value.lower()
            for value in instruction_by_offset[offset].bytes
        )
        != (
            "ff",
            "50" if register == "eax" else "52",
            f"{slot:02x}",
        )
        for offset, register, slot, _ in call_specs
    ):
        raise ValueError(
            "HUD objective StartHide topology bridge rejects repeated "
            "aggregate load/register/slot instruction drift"
        )

    # The two pointer members require one additional natural vptr load.
    nested_specs = (
        (0x1D6, "eax", "ecx", ("8b", "01")),
        (0x1E1, "edx", "ecx", ("8b", "11")),
        (0x398, "edx", "ecx", ("8b", "11")),
        (0x3A5, "eax", "ecx", ("8b", "01")),
    )
    if any(
        counts.get(offset) != 1
        or _cc_cfg._instruction_mnemonic(instruction_by_offset[offset]) != "mov"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(instruction_by_offset[offset]).split(
                ",",
                1,
            )[1]
        )
        != source_register
        or tuple(
            value.lower()
            for value in instruction_by_offset[offset].bytes
        )
        != encoded
        for (
            offset,
            _destination_register,
            source_register,
            encoded,
        ) in nested_specs
    ):
        raise ValueError(
            "HUD objective StartHide topology bridge rejects pointer-member "
            "vptr-load provenance drift"
        )

    phase_three_push = instruction_by_offset.get(0x125)
    phase_three_join = instruction_by_offset.get(0x127)
    phase_one_push = instruction_by_offset.get(0x2E1)
    shared_load = instruction_by_offset.get(0x2E3)
    shared_call = instruction_by_offset.get(0x2EE)
    if (
        phase_three_push is None
        or _cc_cfg._instruction_mnemonic(phase_three_push) != "push"
        or _cc_cfg._instruction_operand(phase_three_push).strip() != "0"
        or tuple(value.lower() for value in phase_three_push.bytes)
        != ("6a", "00")
        or phase_three_join is None
        or _cc_cfg._instruction_mnemonic(phase_three_join) != "jmp"
        or tuple(value.lower() for value in phase_three_join.bytes)
        != ("e9", "b7", "01", "00", "00")
        or phase_one_push is None
        or _cc_cfg._instruction_mnemonic(phase_one_push) != "push"
        or _cc_cfg._instruction_operand(phase_one_push).strip() != "1"
        or tuple(value.lower() for value in phase_one_push.bytes)
        != ("6a", "01")
        or shared_load is None
        or shared_call is None
        or index_by_offset[0x125] >= index_by_offset[0x127]
        or index_by_offset[0x2E1] + 1 != index_by_offset[0x2E3]
        or index_by_offset[0x2E3] >= index_by_offset[0x2EE]
    ):
        raise ValueError(
            "HUD objective StartHide topology bridge rejects phase-3 push-0 "
            "jump, phase-1 push-1 fallthrough, or shared sensor SetVisible "
            "join drift"
        )

    aggregate = _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
    exact_relocation_rows = {
        (
            row.offset,
            row.type,
            row.symbol_name,
            struct.unpack_from("<I", caller.data, row.offset)[0],
        )
        for row in caller.relocations
    }
    required_load_relocations = {
        (
            offset + (1 if encoded[0] == "a1" else 2),
            IMAGE_REL_I386_DIR32,
            aggregate,
            displacement,
        )
        for offset, _, displacement, encoded in load_specs
    }
    if not required_load_relocations.issubset(exact_relocation_rows):
        raise ValueError(
            "HUD objective StartHide topology bridge rejects repeated "
            "aggregate-load DIR32 target/addend relocation drift"
        )

    absolute_bridges = {
        normalize_address(hex(offset)): ReviewedAbsoluteStorageLoadBridge(
            register=register,
            aggregate_symbol=aggregate,
            displacement=displacement,
            access_width=4,
            storage_identity=aggregate_storage,
            canonicalize_nested_load=displacement in {0x824, 0x974},
        )
        for offset, register, displacement, _ in load_specs
    }
    vptr_bridges = {
        normalize_address(hex(offset)): ReviewedVptrStorageBridge(
            register=register,
            provenance=storage,
            storage_identity=storage,
            slot_displacement=slot,
            identity_kind="virtual-slot",
        )
        for offset, register, slot, storage in call_specs
    }
    return absolute_bridges, vptr_bridges


def _hud_ui_mgr_objective_update_candidate_vptr_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedAbsoluteStorageLoadBridge],
    dict[str, ReviewedVptrStorageBridge],
]:
    """Bridge only Objective::Update's exact five SetVisible calls."""
    from _recoil.call_contract.records import (
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedVptrStorageBridge,
    )
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_CALLER_START:
        return {}, {}

    caller_symbol_id = (
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_CALLER_IDENTITY.removeprefix("symbol:")
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
        caller_identity != _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_CALLER_END_EXCLUSIVE
        or caller_addresses != [_cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_CALLER_START]
        or caller_identity in indexes.provider_ids
        or not isinstance(caller_symbol, Mapping)
        or caller_symbol.get("binary") != "recoil"
        or caller_symbol.get("kind") != "function"
        or caller_symbol.get("pipeline_class") != "authored"
        or caller_symbol.get("ownership_state") != "primary-owned"
        or caller_symbol.get("address")
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_CALLER_START
        or caller_symbol.get("end_exclusive")
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_CALLER_END_EXCLUSIVE
        or caller_symbol.get("extent_state") != "known"
        or caller_symbol.get("size") != 0x60
        or caller_symbol.get("navigation_name")
        != "HudUiMgrObjective::Update"
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
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_CALLER_ANCHOR_ID
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
            "HUD objective Update vptr bridge requires the exact authored "
            "caller/source and aggregate storage authority"
        )

    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_CALLER_START
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
            "HUD objective Update vptr bridge requires one exact current HUD "
            "listing/source authority"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != r"\?Update@HudUiMgrObjective@@.*"
        or re.fullmatch(
            r"\?Update@HudUiMgrObjective@@.*",
            str(getattr(candidate.caller_definition, "symbol", "")),
        )
        is None
        or getattr(contribution_row, "name", "")
        != "HudUiMgrObjective::Update"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD objective Update vptr bridge requires the exact authored "
            "hud.cpp contribution row"
        )

    storage_identities = (
        f"load({aggregate_storage}+0x6ac)",
        f"load({aggregate_storage}+0x978)",
        f"load(load({aggregate_storage}+0x824))",
        f"load(load({aggregate_storage}+0x974))",
        f"load({aggregate_storage}+0x768)",
    )
    candidate_provenances = (
        f"load({aggregate_storage}+0x6ac)",
        f"load({aggregate_storage}+0x978)",
        f"load(load({aggregate_storage}+0x974))",
        f"load(load({aggregate_storage}+0x828))",
        f"load({aggregate_storage}+0x768)",
    )
    exact_expected = [
        {
            "ordinal": ordinal,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": storage,
            "slot_displacement": 0x60,
            "cleanup_bytes": None,
        }
        for ordinal, storage in enumerate(storage_identities)
    ]
    if list(expected) != exact_expected:
        raise ValueError(
            "HUD objective Update vptr bridge requires the immutable retail "
            "five-call indirect/slot-0x60/storage/no-cleanup contract: "
            f"observed={list(expected)!r}"
        )

    caller = candidate.caller_definition
    exact_body = bytes.fromhex(
        "a1 ac 06 00 00 6a 01 b9 ac 06 00 00 ff 50 60 "
        "a1 94 06 00 00 85 c0 74 4a "
        "8b 15 78 09 00 00 6a 01 b9 78 09 00 00 ff 52 60 "
        "83 3d 94 06 00 00 02 75 31 "
        "8b 0d 74 09 00 00 85 c9 74 07 "
        "8b 01 6a 01 ff 50 60 "
        "8b 0d 28 08 00 00 85 c9 74 07 "
        "8b 11 6a 01 ff 52 60 "
        "a1 68 07 00 00 6a 01 b9 68 07 00 00 ff 50 60 "
        "c3 "
        "90 90 90 90 90 90 90 90 90 90 90 90 90"
    )
    aggregate = _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
    relocation_specs = (
        (0x01, IMAGE_REL_I386_DIR32, aggregate, 0x6AC),
        (0x08, IMAGE_REL_I386_DIR32, aggregate, 0x6AC),
        (0x10, IMAGE_REL_I386_DIR32, aggregate, 0x694),
        (0x1A, IMAGE_REL_I386_DIR32, aggregate, 0x978),
        (0x21, IMAGE_REL_I386_DIR32, aggregate, 0x978),
        (0x2A, IMAGE_REL_I386_DIR32, aggregate, 0x694),
        (0x33, IMAGE_REL_I386_DIR32, aggregate, 0x974),
        (0x44, IMAGE_REL_I386_DIR32, aggregate, 0x828),
        (0x54, IMAGE_REL_I386_DIR32, aggregate, 0x768),
        (0x5B, IMAGE_REL_I386_DIR32, aggregate, 0x768),
    )
    expected_mask = {
        index
        for relocation_offset, _, _, _ in relocation_specs
        for index in range(relocation_offset, relocation_offset + 4)
    }
    if (
        caller is None
        or caller.symbol != _cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_CALLER_SYMBOL
        or len(caller.data) != 0x70
        or caller.data != exact_body
        or tuple(
            (row.offset, row.type, row.symbol_name)
            for row in caller.relocations
        )
        != tuple(
            (offset, relocation_type, symbol_name)
            for offset, relocation_type, symbol_name, _ in relocation_specs
        )
        or any(
            struct.unpack_from("<I", caller.data, offset)[0] != addend
            for offset, _, _, addend in relocation_specs
        )
        or len(caller.relocation_mask) != len(caller.data)
        or {
            index
            for index, masked in enumerate(caller.relocation_mask)
            if masked
        }
        != expected_mask
        or (
            caller.undefined_external_data + caller.defined_external_data
        ).count(aggregate)
        != 1
        or (
            caller.undefined_external_functions
            + caller.defined_external_functions
        ).count(_cc_catalog.HUD_UI_MGR_OBJECTIVE_UPDATE_CALLER_SYMBOL)
        != 1
    ):
        raise ValueError(
            "HUD objective Update vptr bridge rejects the exact natural "
            "0x70 COFF extent/body/padding, ten DIR32 target/addend/order "
            "rows, relocation mask, or external identities"
        )

    load_specs = (
        (0x00, "eax", 0x6AC, ("a1", "ac", "06", "00", "00"), False),
        (
            0x18,
            "edx",
            0x978,
            ("8b", "15", "78", "09", "00", "00"),
            False,
        ),
        (
            0x31,
            "ecx",
            0x974,
            ("8b", "0d", "74", "09", "00", "00"),
            True,
        ),
        (
            0x42,
            "ecx",
            0x828,
            ("8b", "0d", "28", "08", "00", "00"),
            True,
        ),
        (0x53, "eax", 0x768, ("a1", "68", "07", "00", "00"), False),
    )
    nested_specs = (
        (0x3B, "eax", ("8b", "01")),
        (0x4C, "edx", ("8b", "11")),
    )
    push_specs = (
        (0x05, ("6a", "01")),
        (0x1E, ("6a", "01")),
        (0x3D, ("6a", "01")),
        (0x4E, ("6a", "01")),
        (0x58, ("6a", "01")),
    )
    receiver_specs = (
        (0x07, 0x6AC, ("b9", "ac", "06", "00", "00")),
        (0x20, 0x978, ("b9", "78", "09", "00", "00")),
        (0x5A, 0x768, ("b9", "68", "07", "00", "00")),
    )
    call_specs = (
        (0x0C, "eax", ("ff", "50", "60")),
        (0x25, "edx", ("ff", "52", "60")),
        (0x3F, "eax", ("ff", "50", "60")),
        (0x50, "edx", ("ff", "52", "60")),
        (0x5F, "eax", ("ff", "50", "60")),
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
    aggregate_expressions = lambda displacement: {
        f"{aggregate}+{displacement}",
        f"{aggregate}+0x{displacement:x}",
    }
    if any(
        counts.get(offset) != 1
        or _cc_cfg._instruction_mnemonic(instruction_by_offset[offset]) != "mov"
        or _cc_cfg._instruction_operand(
            instruction_by_offset[offset]
        ).split(",", 1)[0].strip().lower()
        != register
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(
                instruction_by_offset[offset]
            ).split(",", 1)[1]
        )
        not in aggregate_expressions(displacement)
        or tuple(
            value.lower()
            for value in instruction_by_offset[offset].bytes
        )
        != encoded
        for (
            offset,
            register,
            displacement,
            encoded,
            _nested,
        ) in load_specs
    ) or any(
        counts.get(offset) != 1
        or _cc_cfg._instruction_mnemonic(instruction_by_offset[offset]) != "mov"
        or _cc_cfg._instruction_operand(
            instruction_by_offset[offset]
        ).split(",", 1)[0].strip().lower()
        != register
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(
                instruction_by_offset[offset]
            ).split(",", 1)[1]
        )
        != "ecx"
        or tuple(
            value.lower()
            for value in instruction_by_offset[offset].bytes
        )
        != encoded
        for offset, register, encoded in nested_specs
    ) or any(
        counts.get(offset) != 1
        or _cc_cfg._instruction_mnemonic(instruction_by_offset[offset]) != "push"
        or _cc_cfg._instruction_operand(instruction_by_offset[offset]).strip() != "1"
        or tuple(
            value.lower()
            for value in instruction_by_offset[offset].bytes
        )
        != encoded
        for offset, encoded in push_specs
    ) or any(
        counts.get(offset) != 1
        or _cc_cfg._instruction_mnemonic(instruction_by_offset[offset]) != "mov"
        or _cc_cfg._instruction_operand(
            instruction_by_offset[offset]
        ).split(",", 1)[0].strip().lower()
        != "ecx"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(
                instruction_by_offset[offset]
            ).split(",", 1)[1]
        )
        not in (
            aggregate_expressions(displacement)
            | {
                f"OFFSETFLAT:{expression}"
                for expression in aggregate_expressions(displacement)
            }
        )
        or tuple(
            value.lower()
            for value in instruction_by_offset[offset].bytes
        )
        != encoded
        for offset, displacement, encoded in receiver_specs
    ) or any(
        counts.get(offset) != 1
        or _cc_cfg._instruction_mnemonic(instruction_by_offset[offset]) != "call"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(instruction_by_offset[offset])
        )
        not in {f"{register}+96", f"{register}+0x60"}
        or tuple(
            value.lower()
            for value in instruction_by_offset[offset].bytes
        )
        != encoded
        for offset, register, encoded in call_specs
    ):
        raise ValueError(
            "HUD objective Update vptr bridge rejects the exact aggregate "
            "load, EAX/EDX vptr origin, ECX receiver, push-one argument, or "
            "slot-0x60 call listing"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    call_indices = tuple(
        index_by_offset[offset] for offset, _, _ in call_specs
    )
    if (
        tuple(invocation_indices) != call_indices
        or len(invocation_indices) != 5
        or any(
            _cc_cfg._cleanup_after(candidate.instructions, index) is not None
            for index in invocation_indices
        )
        or candidate.local_control_flow_indices
        or candidate.local_control_flow_targets
    ):
        raise ValueError(
            "HUD objective Update vptr bridge rejects five-call count/order, "
            "indirect form, caller cleanup, or alternate local control flow"
        )

    absolute_bridges = {
        normalize_address(hex(offset)): ReviewedAbsoluteStorageLoadBridge(
            register=register,
            aggregate_symbol=aggregate,
            displacement=displacement,
            access_width=4,
            storage_identity=aggregate_storage,
            canonicalize_nested_load=nested,
        )
        for offset, register, displacement, _, nested in load_specs
    }
    vptr_bridges = {
        normalize_address(hex(offset)): ReviewedVptrStorageBridge(
            register=register,
            provenance=provenance,
            storage_identity=storage,
            slot_displacement=0x60,
            identity_kind="virtual-slot",
        )
        for (
            (offset, register, _),
            provenance,
            storage,
        ) in zip(
            call_specs,
            candidate_provenances,
            storage_identities,
        )
    }
    return absolute_bridges, vptr_bridges


def _hud_ui_mgr_ensure_objective_widget_center_retail_guard(
    instructions: Sequence[Instruction],
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> None:
    """Guard retail's exact objective-widget center-query pair."""
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START:
        return
    _cc_recoil_hud_sensor._require_hud_ui_mgr_ensure_sensor_center_authority(
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
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
    index_by_address = {
        address: index
        for index, address in enumerate(addresses)
        if address is not None and counts.get(address) == 1
    }
    fixed_rows = {
        0x410512: (
            ("e8", "19", "38", "00", "00"),
            r"call\s+(?:0x413d30|HudUiLayoutNode::ApplyImageWidget)",
        ),
        0x410517: (
            ("8b", "15", "7c", "65", "4e", "00"),
            r"mov\s+edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[?0x4e657c\]?",
        ),
        0x41051D: (
            ("b9", "7c", "65", "4e", "00"),
            r"mov\s+ecx\s*,\s*0x4e657c",
        ),
        0x410522: (
            ("ff", "52", "64"),
            r"call\s+(?:dword\s+(?:ptr\s+)?)?"
            r"\[edx(?:\+100|\+0x64)\]",
        ),
        0x410525: (
            ("89", "45", "e8"),
            r"mov\s+(?:dword\s+(?:ptr\s+)?)?"
            r"\[ebp(?:-24|-0x18)\]\s*,\s*eax",
        ),
        0x410528: (
            ("a1", "7c", "65", "4e", "00"),
            r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[?0x4e657c\]?",
        ),
        0x41052D: (
            ("b9", "7c", "65", "4e", "00"),
            r"mov\s+ecx\s*,\s*0x4e657c",
        ),
        0x410532: (
            ("ff", "50", "68"),
            r"call\s+(?:dword\s+(?:ptr\s+)?)?"
            r"\[eax(?:\+104|\+0x68)\]",
        ),
        0x410535: (
            ("8d", "4d", "cc"),
            r"lea\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[ebp(?:-52|-0x34)\]",
        ),
        0x410538: (
            ("8d", "55", "e8"),
            r"lea\s+edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[ebp(?:-24|-0x18)\]",
        ),
        0x41053B: (("51",), r"push\s+ecx"),
        0x41053C: (
            ("8b", "4e", "04"),
            r"mov\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[esi(?:\+4|\+0x4)\]",
        ),
        0x41053F: (("52",), r"push\s+edx"),
        0x410540: (
            ("ba", "48", "68", "4e", "00"),
            r"mov\s+edx\s*,\s*0x4e6848",
        ),
        0x410545: (
            ("83", "c1", "18"),
            r"add\s+ecx\s*,\s*(?:24|0x18)",
        ),
        0x410548: (
            ("89", "45", "ec"),
            r"mov\s+(?:dword\s+(?:ptr\s+)?)?"
            r"\[ebp(?:-20|-0x14)\]\s*,\s*eax",
        ),
        0x41054B: (
            ("e8", "c0", "35", "00", "00"),
            r"call\s+(?:0x413b10|HudUiLayoutNode::ApplyCornerTextQuad)",
        ),
    }
    if any(
        address not in by_address
        or counts.get(address) != 1
        or tuple(
            value.lower() for value in by_address[address].bytes
        )
        != body
        or re.fullmatch(
            pattern,
            by_address[address].raw_text.strip(),
            flags=re.IGNORECASE,
        )
        is None
        for address, (body, pattern) in fixed_rows.items()
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective-widget center retail guard "
            "requires exact ApplyImageWidget/receiver/center/result/"
            "ApplyCornerTextQuad instructions"
        )
    ordered_addresses = list(fixed_rows)
    ordered_indices = [index_by_address[address] for address in ordered_addresses]
    call_x = address_value(
        _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_RETAIL_CALL_X_ADDRESS
    )
    call_y = address_value(
        _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_RETAIL_CALL_Y_ADDRESS
    )
    bounded_calls = [
        address
        for address, instruction in zip(addresses, instructions)
        if (
            address is not None
            and ordered_addresses[0] <= address <= ordered_addresses[-1]
            and _cc_cfg._instruction_mnemonic(instruction) in {"call", "jmp"}
        )
    ]
    if (
        ordered_addresses != sorted(ordered_addresses)
        or ordered_indices != sorted(ordered_indices)
        or len(set(ordered_indices)) != len(ordered_indices)
        or bounded_calls != [0x410512, call_x, call_y, 0x41054B]
        or sum(
            _cc_cfg._instruction_mnemonic(instruction) == "call"
            for instruction in instructions[: index_by_address[call_x]]
        )
        != _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_RETAIL_CALL_X_ORDINAL
        or sum(
            _cc_cfg._instruction_mnemonic(instruction) == "call"
            for instruction in instructions[: index_by_address[call_y]]
        )
        != _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_RETAIL_CALL_Y_ORDINAL
        or _cc_cfg._cleanup_after(instructions, index_by_address[call_x]) is not None
        or _cc_cfg._cleanup_after(instructions, index_by_address[call_y]) is not None
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective-widget center retail guard rejects "
            "missing, extra, reordered, wrong-cleanup, or absorbed-neighbor "
            "pair rows"
        )


def _hud_ui_mgr_ensure_objective_widget_center_candidate_absolute_load_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedAbsoluteStorageLoadBridge]:
    """Bridge only the exact objective-widget center-query pair."""
    from _recoil.call_contract.records import ReviewedAbsoluteStorageLoadBridge
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START:
        return {}
    _cc_recoil_hud_sensor._require_hud_ui_mgr_ensure_sensor_center_authority(
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    definition = candidate.caller_definition
    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START
    ]
    if (
        definition is None
        or definition.symbol != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_SYMBOL
        or not definition.data
        or len(definition.relocation_mask) != len(definition.data)
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
            "HUD EnsureHudLoaded objective-widget center candidate bridge "
            "requires a nonempty candidate COMDAT and current HUD target"
        )
    expected_pair = (
        {
            "ordinal": (
                _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_RETAIL_CALL_X_ORDINAL
            ),
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": (
                _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_STORAGE_IDENTITY
            ),
            "slot_displacement": _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CALL_X_SLOT,
            "cleanup_bytes": None,
        },
        {
            "ordinal": (
                _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_RETAIL_CALL_Y_ORDINAL
            ),
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": (
                _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_STORAGE_IDENTITY
            ),
            "slot_displacement": _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CALL_Y_SLOT,
            "cleanup_bytes": None,
        },
    )
    if (
        len(expected)
        <= _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_RETAIL_CALL_Y_ORDINAL
        or tuple(
            expected[ordinal]
            for ordinal in (
                _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_RETAIL_CALL_X_ORDINAL,
                _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_RETAIL_CALL_Y_ORDINAL,
            )
        )
        != expected_pair
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective-widget center candidate bridge "
            "requires exact independently retail-derived storage/slot "
            "contracts"
        )

    start = 0
    aggregate = re.escape(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
    apply_image = re.escape(
        _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_APPLY_IMAGE_WIDGET_SYMBOL
    )
    apply_corner = re.escape(
        _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_APPLY_CORNER_TEXT_QUAD_SYMBOL
    )
    baseline_fixed = {
        "apply_image": start + 0x447,
        "load_x": start + 0x44C,
        "receiver_x": start + 0x452,
        "call_x": start + 0x457,
        "result_x": start + 0x45A,
        "load_y": start + 0x45E,
        "receiver_y": start + 0x463,
        "call_y": start + 0x468,
        "zero": start + 0x46B,
        "result_y": start + 0x46D,
        "rect_0": start + 0x471,
        "rect": start + 0x475,
        "rect_1": start + 0x479,
        "center": start + 0x47D,
        "rect_2": start + 0x481,
        "push_rect": start + 0x485,
        "push_center": start + 0x486,
        "bar": start + 0x487,
        "payload": start + 0x48C,
        "rect_3": start + 0x48F,
        "apply_corner": start + 0x493,
    }
    baseline_expected_rows = {
        baseline_fixed["apply_image"]: (
            ("e8", "00", "00", "00", "00"),
            rf"call\s+{apply_image}(?:\s*;.*)?",
        ),
        baseline_fixed["load_x"]: (
            ("8b", "15", "ac", "06", "00", "00"),
            rf"mov\s+edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            rf"{aggregate}\+(?:1708|0x6ac)",
        ),
        baseline_fixed["receiver_x"]: (
            ("b9", "ac", "06", "00", "00"),
            rf"mov\s+ecx\s*,\s*(?:offset\s+(?:flat:)?)?"
            rf"{aggregate}\+(?:1708|0x6ac)",
        ),
        baseline_fixed["call_x"]: (
            ("ff", "52", "64"),
            r"call\s+(?:dword\s+(?:ptr\s+)?)?"
            r"\[edx(?:\+100|\+0x64)\]",
        ),
        baseline_fixed["result_x"]: (
            ("89", "44", "24", "24"),
            r"mov\s+(?:dword\s+(?:ptr\s+)?)?"
            r"_objectiveCenter\$[A-Za-z0-9_]+\[esp\+148\]\s*,\s*eax",
        ),
        baseline_fixed["load_y"]: (
            ("a1", "ac", "06", "00", "00"),
            rf"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            rf"{aggregate}\+(?:1708|0x6ac)",
        ),
        baseline_fixed["receiver_y"]: (
            ("b9", "ac", "06", "00", "00"),
            rf"mov\s+ecx\s*,\s*(?:offset\s+(?:flat:)?)?"
            rf"{aggregate}\+(?:1708|0x6ac)",
        ),
        baseline_fixed["call_y"]: (
            ("ff", "50", "68"),
            r"call\s+(?:dword\s+(?:ptr\s+)?)?"
            r"\[eax(?:\+104|\+0x68)\]",
        ),
        baseline_fixed["zero"]: (("33", "c9"), r"xor\s+ecx\s*,\s*ecx"),
        baseline_fixed["result_y"]: (
            ("89", "44", "24", "28"),
            r"mov\s+(?:dword\s+(?:ptr\s+)?)?"
            r"_objectiveCenter\$[A-Za-z0-9_]+\[esp\+152\]\s*,\s*eax",
        ),
        baseline_fixed["rect_0"]: (
            ("89", "4c", "24", "54"),
            r"mov\s+(?:dword\s+(?:ptr\s+)?)?"
            r"_objectiveBarRect\$[A-Za-z0-9_]+\[esp\+152\]\s*,\s*ecx",
        ),
        baseline_fixed["rect"]: (
            ("8d", "54", "24", "50"),
            r"lea\s+edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"_objectiveBarRect\$[A-Za-z0-9_]+\[esp\+148\]",
        ),
        baseline_fixed["rect_1"]: (
            ("89", "4c", "24", "58"),
            r"mov\s+(?:dword\s+(?:ptr\s+)?)?"
            r"_objectiveBarRect\$[A-Za-z0-9_]+\[esp\+156\]\s*,\s*ecx",
        ),
        baseline_fixed["center"]: (
            ("8d", "44", "24", "24"),
            r"lea\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"_objectiveCenter\$[A-Za-z0-9_]+\[esp\+148\]",
        ),
        baseline_fixed["rect_2"]: (
            ("89", "4c", "24", "5c"),
            r"mov\s+(?:dword\s+(?:ptr\s+)?)?"
            r"_objectiveBarRect\$[A-Za-z0-9_]+\[esp\+160\]\s*,\s*ecx",
        ),
        baseline_fixed["push_rect"]: (("52",), r"push\s+edx"),
        baseline_fixed["push_center"]: (("50",), r"push\s+eax"),
        baseline_fixed["bar"]: (
            ("ba", "78", "09", "00", "00"),
            rf"mov\s+edx\s*,\s*(?:offset\s+(?:flat:)?)?"
            rf"{aggregate}\+(?:2424|0x978)",
        ),
        baseline_fixed["payload"]: (
            ("8d", "4e", "18"),
            r"lea\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[esi(?:\+24|\+0x18)\]",
        ),
        baseline_fixed["rect_3"]: (
            ("89", "5c", "24", "58"),
            r"mov\s+(?:dword\s+(?:ptr\s+)?)?"
            r"_objectiveBarRect\$[A-Za-z0-9_]+\[esp\+156\]\s*,\s*ebx",
        ),
        baseline_fixed["apply_corner"]: (
            ("e8", "00", "00", "00", "00"),
            rf"call\s+{apply_corner}(?:\s*;.*)?",
        ),
    }
    symbolic_stack_keys = {"result_x", "result_y", "center"}
    baseline_locator_rows = {
        address: row
        for address, row in baseline_expected_rows.items()
        if address
        not in {
            baseline_fixed[key]
            for key in symbolic_stack_keys
        }
    }
    unit_shift, addresses, by_address, index_by_address = (
        _cc_callable_identity._locate_shifted_candidate_instruction_unit(
            candidate,
            baseline_locator_rows,
            label=(
                "HUD EnsureHudLoaded objective-widget center candidate bridge"
            ),
        )
    )
    fixed = {
        key: address + unit_shift
        for key, address in baseline_fixed.items()
    }
    expected_rows = {
        address + unit_shift: row
        for address, row in baseline_expected_rows.items()
    }

    unit_start = 0x447 + unit_shift
    unit_end = 0x498 + unit_shift
    normalized_body = (
        _cc_callable_identity._candidate_body_with_normalized_symbolic_stack_displacements(
            candidate,
            unit_start=unit_start,
            unit_end=unit_end,
            by_address=by_address,
            specs={
                fixed["result_x"]: (
                    (0x89, 0x44, 0x24),
                    expected_rows[fixed["result_x"]][1],
                    0x24,
                ),
                fixed["result_y"]: (
                    (0x89, 0x44, 0x24),
                    expected_rows[fixed["result_y"]][1],
                    0x28,
                ),
                fixed["center"]: (
                    (0x8D, 0x44, 0x24),
                    expected_rows[fixed["center"]][1],
                    0x24,
                ),
            },
            label=(
                "HUD EnsureHudLoaded objective-widget center candidate bridge"
            ),
        )
    )
    exact_body = bytes.fromhex(
        "e8 00 00 00 00 "
        "8b 15 ac 06 00 00 "
        "b9 ac 06 00 00 "
        "ff 52 64 "
        "89 44 24 24 "
        "a1 ac 06 00 00 "
        "b9 ac 06 00 00 "
        "ff 50 68 "
        "33 c9 "
        "89 44 24 28 "
        "89 4c 24 54 "
        "8d 54 24 50 "
        "89 4c 24 58 "
        "8d 44 24 24 "
        "89 4c 24 5c "
        "52 50 "
        "ba 78 09 00 00 "
        "8d 4e 18 "
        "89 5c 24 58 "
        "e8 00 00 00 00"
    )
    dir32_specs = {
        0x44E + unit_shift: _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_DISPLACEMENT,
        0x453 + unit_shift: _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_DISPLACEMENT,
        0x45F + unit_shift: _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_DISPLACEMENT,
        0x464 + unit_shift: _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_DISPLACEMENT,
        0x488 + unit_shift: 0x978,
    }
    rel32_specs = {
        0x448 + unit_shift: _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_APPLY_IMAGE_WIDGET_SYMBOL,
        0x494 + unit_shift: _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_APPLY_CORNER_TEXT_QUAD_SYMBOL,
    }
    bounded_relocations = [
        row
        for row in definition.relocations
        if unit_start <= row.offset < unit_end
    ]
    bounded_by_offset: dict[int, list[CoffRelocation]] = {}
    for row in bounded_relocations:
        bounded_by_offset.setdefault(row.offset, []).append(row)
    expected_offsets = set(dir32_specs) | set(rel32_specs)
    if (
        normalized_body != exact_body
        or set(bounded_by_offset) != expected_offsets
        or any(len(rows) != 1 for rows in bounded_by_offset.values())
        or any(
            bounded_by_offset[offset][0].type != IMAGE_REL_I386_DIR32
            or bounded_by_offset[offset][0].symbol_name
            != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
            or struct.unpack_from("<I", definition.data, offset)[0]
            != addend
            for offset, addend in dir32_specs.items()
        )
        or any(
            bounded_by_offset[offset][0].type != IMAGE_REL_I386_REL32
            or bounded_by_offset[offset][0].symbol_name != symbol
            or struct.unpack_from("<I", definition.data, offset)[0] != 0
            for offset, symbol in rel32_specs.items()
        )
        or {
            index
            for index in range(unit_start, unit_end)
            if definition.relocation_mask[index]
        }
        != {
            index
            for offset in expected_offsets
            for index in range(offset, offset + 4)
        }
        or definition.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or definition.defined_external_functions.count(
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_APPLY_IMAGE_WIDGET_SYMBOL
        )
        != 1
        or definition.defined_external_functions.count(
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_APPLY_CORNER_TEXT_QUAD_SYMBOL
        )
        != 1
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective-widget center candidate bridge "
            "rejects malformed, aliased, wrong-target/addend/mask "
            "DIR32/REL32 fields"
        )

    call_x_index = index_by_address[fixed["call_x"]]
    call_y_index = index_by_address[fixed["call_y"]]
    bounded_calls = [
        address
        for address, instruction in zip(addresses, candidate.instructions)
        if (
            address is not None
            and fixed["apply_image"] <= address <= fixed["apply_corner"]
            and _cc_cfg._instruction_mnemonic(instruction) == "call"
        )
    ]
    ordinal_by_index = {
        index: ordinal
        for ordinal, index in enumerate(
            _cc_callable_identity._candidate_static_invocation_indices(
                candidate,
                caller_start=normalized_start,
                caller_end_exclusive=caller_end_exclusive,
            )
        )
    }
    if (
        bounded_calls
        != [
            fixed["apply_image"],
            fixed["call_x"],
            fixed["call_y"],
            fixed["apply_corner"],
        ]
        or _cc_cfg._cleanup_after(candidate.instructions, call_x_index) is not None
        or _cc_cfg._cleanup_after(candidate.instructions, call_y_index) is not None
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective-widget center candidate bridge "
            "rejects missing, extra, reordered, wrong-cleanup, or "
            "absorbed-neighbor calls"
        )

    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    return {
        normalize_address(hex(0x44C + unit_shift)): ReviewedAbsoluteStorageLoadBridge(
            register="edx",
            aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            displacement=_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_DISPLACEMENT,
            access_width=4,
            storage_identity=aggregate_identity,
        ),
        normalize_address(hex(0x45E + unit_shift)): ReviewedAbsoluteStorageLoadBridge(
            register="eax",
            aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            displacement=_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_DISPLACEMENT,
            access_width=4,
            storage_identity=aggregate_identity,
        ),
    }


def _hud_ui_mgr_ensure_objective_text_setpos_retail_guard(
    instructions: Sequence[Instruction],
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> None:
    """Guard the retail objective text-panel SetPos and word-wrap units."""
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START:
        return
    _cc_recoil_hud_sensor._require_hud_ui_mgr_ensure_sensor_center_authority(
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    aggregate_base = address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
    if (
        address_value(_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_SUMMARY_PANEL_ADDRESS)
        != aggregate_base
        + _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_SUMMARY_PANEL_DISPLACEMENT
        or address_value(_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_DESC_PANEL_ADDRESS)
        != aggregate_base
        + _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_DESC_PANEL_DISPLACEMENT
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective text SetPos retail guard requires "
            "the exact typed summary/description panel addresses"
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
    index_by_address = {
        address: index
        for index, address in enumerate(addresses)
        if address is not None and counts.get(address) == 1
    }
    fixed_rows = {
        0x41058E: (("8d", "4d", "fc"), r"lea\s+ecx\s*,.*\[ebp-0x4\]"),
        0x410591: (("57",), r"push\s+edi"),
        0x410597: (("51",), r"push\s+ecx"),
        0x410598: (("8b", "4e", "04"), r"mov\s+ecx\s*,.*\[esi\+0x4\]"),
        0x41059B: (("8d", "55", "f8"), r"lea\s+edx\s*,.*\[ebp-0x8\]"),
        0x41059E: (("83", "c1", "28"), r"add\s+ecx\s*,\s*0x28"),
        0x4105A1: (
            ("e8", "2a", "35", "00", "00"),
            r"call\s+(?:0x413ad0|HudUiLayoutNode::ReadInt3)",
        ),
        0x4105A6: (("8b", "45", "ec"), r"mov\s+eax\s*,.*\[ebp-0x14\]"),
        0x4105A9: (("8b", "5d", "fc"), r"mov\s+ebx\s*,.*\[ebp-0x4\]"),
        0x4105AC: (
            ("8b", "0d", "f4", "66", "4e", "00"),
            r"mov\s+ecx\s*,.*\[?0x4e66f4\]?",
        ),
        0x4105B2: (("03", "c3"), r"add\s+eax\s*,\s*ebx"),
        0x4105B4: (("8b", "5d", "f8"), r"mov\s+ebx\s*,.*\[ebp-0x8\]"),
        0x4105B7: (("50",), r"push\s+eax"),
        0x4105B8: (("8b", "45", "e8"), r"mov\s+eax\s*,.*\[ebp-0x18\]"),
        0x4105BB: (("8b", "11"), r"mov\s+edx\s*,.*\[ecx\]"),
        0x4105BD: (("03", "c3"), r"add\s+eax\s*,\s*ebx"),
        0x4105BF: (("50",), r"push\s+eax"),
        0x4105C0: (
            ("ff", "52", "0c"),
            r"call\s+.*\[edx\+(?:0xc|12)\]",
        ),
        0x4105C3: (("8d", "4d", "fc"), r"lea\s+ecx\s*,.*\[ebp-0x4\]"),
        0x4105C6: (("8d", "55", "f8"), r"lea\s+edx\s*,.*\[ebp-0x8\]"),
        0x4105C9: (("57",), r"push\s+edi"),
        0x4105CA: (("51",), r"push\s+ecx"),
        0x4105CB: (("8b", "4e", "04"), r"mov\s+ecx\s*,.*\[esi\+0x4\]"),
        0x4105CE: (("83", "c1", "30"), r"add\s+ecx\s*,\s*0x30"),
        0x4105D1: (
            ("e8", "fa", "34", "00", "00"),
            r"call\s+(?:0x413ad0|HudUiLayoutNode::ReadInt3)",
        ),
        0x4105D6: (("8b", "45", "ec"), r"mov\s+eax\s*,.*\[ebp-0x14\]"),
        0x4105D9: (("8b", "5d", "fc"), r"mov\s+ebx\s*,.*\[ebp-0x4\]"),
        0x4105DC: (
            ("8b", "0d", "44", "68", "4e", "00"),
            r"mov\s+ecx\s*,.*\[?0x4e6844\]?",
        ),
        0x4105E2: (("03", "c3"), r"add\s+eax\s*,\s*ebx"),
        0x4105E4: (("8b", "5d", "f8"), r"mov\s+ebx\s*,.*\[ebp-0x8\]"),
        0x4105E7: (("50",), r"push\s+eax"),
        0x4105E8: (("8b", "45", "e8"), r"mov\s+eax\s*,.*\[ebp-0x18\]"),
        0x4105EB: (("8b", "11"), r"mov\s+edx\s*,.*\[ecx\]"),
        0x4105ED: (("03", "c3"), r"add\s+eax\s*,\s*ebx"),
        0x4105EF: (("50",), r"push\s+eax"),
        0x4105F0: (
            ("ff", "52", "0c"),
            r"call\s+.*\[edx\+(?:0xc|12)\]",
        ),
        0x4105F3: (("8b", "4d", "f8"), r"mov\s+ecx\s*,.*\[ebp-0x8\]"),
        0x4105F6: (("8b", "45", "d4"), r"mov\s+eax\s*,.*\[ebp-0x2c\]"),
        0x4105F9: (("89", "7d", "ac"), r"mov\s+.*\[ebp-0x54\]\s*,\s*edi"),
        0x4105FC: (("89", "7d", "b0"), r"mov\s+.*\[ebp-0x50\]\s*,\s*edi"),
        0x4105FF: (("8d", "14", "09"), r"lea\s+edx\s*,.*\[ecx\+ecx\]"),
        0x410602: (("8b", "4d", "cc"), r"mov\s+ecx\s*,.*\[ebp-0x34\]"),
        0x410605: (("2b", "c2"), r"sub\s+eax\s*,\s*edx"),
        0x410607: (("2b", "c1"), r"sub\s+eax\s*,\s*ecx"),
        0x410609: (("8b", "4d", "d8"), r"mov\s+ecx\s*,.*\[ebp-0x28\]"),
        0x41060C: (("89", "45", "b4"), r"mov\s+.*\[ebp-0x4c\]\s*,\s*eax"),
        0x41060F: (("8b", "45", "d0"), r"mov\s+eax\s*,.*\[ebp-0x30\]"),
        0x410612: (("2b", "c8"), r"sub\s+ecx\s*,\s*eax"),
        0x410614: (("8d", "45", "ac"), r"lea\s+eax\s*,.*\[ebp-0x54\]"),
        0x410617: (("89", "4d", "b8"), r"mov\s+.*\[ebp-0x48\]\s*,\s*ecx"),
        0x41061A: (
            ("8b", "0d", "44", "68", "4e", "00"),
            r"mov\s+ecx\s*,.*\[?0x4e6844\]?",
        ),
        0x410620: (("50",), r"push\s+eax"),
        0x410621: (("8b", "11"), r"mov\s+edx\s*,.*\[ecx\]"),
        0x410623: (
            ("ff", "52", "6c"),
            r"call\s+.*\[edx\+(?:0x6c|108)\]",
        ),
        0x410626: (("8d", "4d", "cc"), r"lea\s+ecx\s*,.*\[ebp-0x34\]"),
        0x410629: (("8d", "55", "e8"), r"lea\s+edx\s*,.*\[ebp-0x18\]"),
        0x41062C: (("51",), r"push\s+ecx"),
        0x41062D: (("8b", "4e", "04"), r"mov\s+ecx\s*,.*\[esi\+0x4\]"),
        0x410630: (("52",), r"push\s+edx"),
        0x410631: (("57",), r"push\s+edi"),
        0x410632: (("57",), r"push\s+edi"),
        0x410633: (
            ("ba", "fc", "66", "4e", "00"),
            r"mov\s+edx\s*,\s*(?:0x4e66fc|.*\+0x82c)",
        ),
        0x410638: (("83", "c1", "38"), r"add\s+ecx\s*,\s*0x38"),
        0x41063B: (
            ("e8", "d0", "35", "00", "00"),
            r"call\s+(?:0x413c10|HudUiLayoutNode::ApplyMeterQuad)",
        ),
    }
    if any(
        address not in by_address
        or tuple(value.lower() for value in by_address[address].bytes) != body
        or re.fullmatch(
            pattern,
            by_address[address].raw_text.strip(),
            flags=re.IGNORECASE,
        )
        is None
        for address, (body, pattern) in fixed_rows.items()
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective text SetPos retail guard requires "
            "the exact ReadInt3/receiver/two-argument SetPos pairs, "
            "four-dword description word-wrap rectangle, one-pointer "
            "word-wrap call, and ApplyMeterQuad boundary"
        )

    ordered_indices = [index_by_address[address] for address in fixed_rows]
    invocation_addresses = {
        "read_summary": address_value(
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_READ_SUMMARY_ADDRESS
        ),
        "setpos_summary": address_value(
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_SETPOS_SUMMARY_ADDRESS
        ),
        "read_desc": address_value(
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_READ_DESC_ADDRESS
        ),
        "setpos_desc": address_value(
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_SETPOS_DESC_ADDRESS
        ),
        "word_wrap": address_value(
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_WORD_WRAP_ADDRESS
        ),
        "apply_meter_boundary": address_value(
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_APPLY_METER_ADDRESS
        ),
    }
    bounded_calls = [
        address
        for address, instruction in zip(addresses, instructions)
        if (
            address is not None
            and invocation_addresses["read_summary"]
            <= address
            < invocation_addresses["word_wrap"]
            and _cc_cfg._instruction_mnemonic(instruction) in {"call", "jmp"}
        )
    ]
    word_wrap_calls = [
        address
        for address, instruction in zip(addresses, instructions)
        if (
            address is not None
            and invocation_addresses["setpos_desc"]
            <= address
            < invocation_addresses["apply_meter_boundary"]
            and _cc_cfg._instruction_mnemonic(instruction) in {"call", "jmp"}
        )
    ]
    ordinal_by_index = {
        index: ordinal
        for ordinal, index in enumerate(
            index
            for index, instruction in enumerate(instructions)
            if _cc_cfg._instruction_mnemonic(instruction) in {"call", "jmp"}
        )
    }
    ordinal_specs = {
        "read_summary": _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_READ_SUMMARY_ORDINAL,
        "setpos_summary": (
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_SETPOS_SUMMARY_ORDINAL
        ),
        "read_desc": _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_READ_DESC_ORDINAL,
        "setpos_desc": _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_SETPOS_DESC_ORDINAL,
        "word_wrap": _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_WORD_WRAP_ORDINAL,
    }
    if (
        ordered_indices != sorted(ordered_indices)
        or len(set(ordered_indices)) != len(ordered_indices)
        or bounded_calls
        != [
            invocation_addresses["read_summary"],
            invocation_addresses["setpos_summary"],
            invocation_addresses["read_desc"],
            invocation_addresses["setpos_desc"],
        ]
        or word_wrap_calls
        != [
            invocation_addresses["setpos_desc"],
            invocation_addresses["word_wrap"],
        ]
        or any(
            ordinal_by_index.get(index_by_address[invocation_addresses[name]])
            != ordinal
            for name, ordinal in ordinal_specs.items()
        )
        or any(
            _cc_cfg._cleanup_after(
                instructions,
                index_by_address[invocation_addresses[name]],
            )
            is not None
            for name in ordinal_specs
        )
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective text SetPos retail guard rejects "
            "missing, extra, reordered, wrong-cleanup, or absorbed-boundary "
            "calls or consumed word-wrap result"
        )


def _hud_ui_mgr_ensure_objective_text_setpos_candidate_absolute_load_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedAbsoluteStorageLoadBridge]:
    """Bridge the three exact objective text-panel receiver loads."""
    from _recoil.call_contract.records import ReviewedAbsoluteStorageLoadBridge
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START:
        return {}
    _cc_recoil_hud_sensor._require_hud_ui_mgr_ensure_sensor_center_authority(
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    definition = candidate.caller_definition
    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START
    ]
    if (
        definition is None
        or definition.symbol != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_SYMBOL
        or not definition.data
        or len(definition.relocation_mask) != len(definition.data)
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
            "HUD EnsureHudLoaded objective text SetPos candidate bridge "
            "requires a nonempty candidate COMDAT and current HUD target"
        )

    read_contract = {
        "form": "call",
        "dispatch": "direct",
        "identity_kind": "direct",
        "target_identity": _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_READ_INT3_IDENTITY,
        "storage_identity": "",
        "slot_displacement": None,
        "cleanup_bytes": None,
    }
    expected_contracts = {
        _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_READ_SUMMARY_ORDINAL: {
            "ordinal": (
                _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_READ_SUMMARY_ORDINAL
            ),
            **read_contract,
        },
        _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_SETPOS_SUMMARY_ORDINAL: {
            "ordinal": (
                _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_SETPOS_SUMMARY_ORDINAL
            ),
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": (
                _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_SUMMARY_PANEL_STORAGE_IDENTITY
            ),
            "slot_displacement": _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_TEXT_SETPOS_SLOT,
            "cleanup_bytes": None,
        },
        _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_READ_DESC_ORDINAL: {
            "ordinal": _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_READ_DESC_ORDINAL,
            **read_contract,
        },
        _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_SETPOS_DESC_ORDINAL: {
            "ordinal": (
                _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_SETPOS_DESC_ORDINAL
            ),
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": (
                _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_DESC_PANEL_STORAGE_IDENTITY
            ),
            "slot_displacement": _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_TEXT_SETPOS_SLOT,
            "cleanup_bytes": None,
        },
        _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_WORD_WRAP_ORDINAL: {
            "ordinal": _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_WORD_WRAP_ORDINAL,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": (
                _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_DESC_PANEL_STORAGE_IDENTITY
            ),
            "slot_displacement": _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WORD_WRAP_SLOT,
            "cleanup_bytes": None,
        },
    }
    if (
        len(expected) <= _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_WORD_WRAP_ORDINAL
        or any(
            expected[ordinal] != contract
            for ordinal, contract in expected_contracts.items()
        )
    ):
        observed_contracts = {
            ordinal: expected[ordinal] if ordinal < len(expected) else None
            for ordinal in expected_contracts
        }
        raise ValueError(
            "HUD EnsureHudLoaded objective text SetPos candidate bridge "
            "requires exact independently retail-derived ReadInt3 and "
            f"storage/slot contracts: observed={observed_contracts!r}"
        )

    start = 0
    aggregate = re.escape(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
    read_int3 = re.escape(_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_READ_INT3_SYMBOL)
    baseline_fixed = {
        "read_summary": start
        + _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_READ_SUMMARY_OFFSET,
        "load_summary": start
        + _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_LOAD_SUMMARY_OFFSET,
        "setpos_summary": start
        + _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_SETPOS_SUMMARY_OFFSET,
        "read_desc": start
        + _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_READ_DESC_OFFSET,
        "load_desc": start
        + _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_LOAD_DESC_OFFSET,
        "setpos_desc": start
        + _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_SETPOS_DESC_OFFSET,
        "boundary_load": start
        + _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_WORD_WRAP_LOAD_OFFSET,
        "boundary": start
        + _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_WORD_WRAP_OFFSET,
        "right_boundary": start
        + _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_APPLY_METER_OFFSET,
    }
    baseline_expected_rows = {
        baseline_fixed["read_summary"]: (
            ("e8", "00", "00", "00", "00"),
            rf"call\s+{read_int3}(?:\s*;.*)?",
        ),
        baseline_fixed["setpos_summary"]: (
            ("ff", "50", "0c"),
            r"call\s+.*\[eax\+(?:12|0xc)\]",
        ),
        baseline_fixed["read_desc"]: (
            ("e8", "00", "00", "00", "00"),
            rf"call\s+{read_int3}(?:\s*;.*)?",
        ),
        baseline_fixed["load_desc"]: (
            ("8b", "0d", "74", "09", "00", "00"),
            rf"mov\s+ecx\s*,.*{aggregate}\+(?:2420|0x974)",
        ),
        baseline_fixed["setpos_desc"]: (
            ("ff", "52", "0c"),
            r"call\s+.*\[edx\+(?:12|0xc)\]",
        ),
        baseline_fixed["boundary_load"]: (
            ("8b", "0d", "74", "09", "00", "00"),
            rf"mov\s+ecx\s*,.*{aggregate}\+(?:2420|0x974)",
        ),
        baseline_fixed["boundary"]: (
            ("ff", "50", "6c"),
            r"call\s+.*\[eax\+(?:108|0x6c)\]",
        ),
        start + 0x5AD: (
            ("8d", "44", "24", "50"),
            r"lea\s+eax\s*,.*\[esp\+(?:0x50|80|0x94|148)\]",
        ),
        start + 0x5B1: (
            ("8d", "4c", "24", "24"),
            r"lea\s+ecx\s*,.*\[esp\+(?:0x24|36|0x94|148)\]",
        ),
        start + 0x5B5: (("50",), r"push\s+eax"),
        start + 0x5B6: (("51",), r"push\s+ecx"),
        start + 0x5B7: (("53",), r"push\s+ebx"),
        start + 0x5B8: (("53",), r"push\s+ebx"),
        start + 0x5B9: (
            ("ba", "2c", "08", "00", "00"),
            rf"mov\s+edx\s*,.*{aggregate}\+(?:2092|0x82c)",
        ),
        start + 0x5BE: (
            ("8d", "4e", "38"),
            r"lea\s+ecx\s*,.*\[esi\+(?:56|0x38)\]",
        ),
        baseline_fixed["right_boundary"]: (
            ("e8", "00", "00", "00", "00"),
            (
                rf"call\s+"
                rf"{re.escape(_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_APPLY_METER_QUAD_SYMBOL)}"
                rf"(?:\s*;.*)?"
            ),
        ),
    }
    locator_rows = {
        address: row
        for address, row in baseline_expected_rows.items()
        if address < 0x5AD
    }
    unit_shift, addresses, by_address, index_by_address = (
        _cc_callable_identity._locate_shifted_candidate_instruction_unit(
            candidate,
            locator_rows,
            label="HUD EnsureHudLoaded objective text SetPos candidate bridge",
        )
    )
    fixed = {
        key: address + unit_shift
        for key, address in baseline_fixed.items()
    }
    expected_rows = {
        address + unit_shift: row
        for address, row in baseline_expected_rows.items()
    }
    boundary_center_address = 0x5B1 + unit_shift
    if any(
        address not in by_address
        or tuple(
            value.lower() for value in by_address[address].bytes
        )
        != body
        or re.fullmatch(
            pattern,
            by_address[address].raw_text.strip(),
            flags=re.IGNORECASE,
        )
        is None
        for address, (body, pattern) in expected_rows.items()
        if address != boundary_center_address
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective text SetPos candidate bridge "
            "rejects shifted boundary instruction drift"
        )
    _cc_callable_identity._candidate_body_with_normalized_symbolic_stack_displacements(
        candidate,
        unit_start=boundary_center_address,
        unit_end=boundary_center_address + 4,
        by_address=by_address,
        specs={
            boundary_center_address: (
                (0x8D, 0x4C, 0x24),
                expected_rows[boundary_center_address][1],
                0x24,
            )
        },
        label="HUD EnsureHudLoaded objective text SetPos candidate bridge",
    )

    unit_start = 0x4EA + unit_shift
    unit_end = 0x5AD + unit_shift
    exact_body = bytes.fromhex(
        "8d 54 24 14 25 ff ff 00 00 53 52 8d 54 24 18 8d 4e 28 "
        "a3 ac 0a 00 00 89 5c 24 18 89 5c 24 1c e8 00 00 00 00 "
        "8b 7c 24 14 8b 0d 24 08 00 00 8b 54 24 28 8b 01 03 fa "
        "8b 54 24 24 57 8b 7c 24 14 03 fa 57 ff 50 0c "
        "8d 44 24 14 8d 54 24 10 53 50 8d 4e 30 e8 00 00 00 00 "
        "8b 0d 74 09 00 00 8b 44 24 28 8b 11 8b 7c 24 14 03 f8 "
        "8b 44 24 24 57 8b 7c 24 14 03 f8 57 ff 52 0c "
        "33 c9 89 5c 24 34 8b 54 24 10 89 4c 24 38 89 4c 24 3c "
        "89 5c 24 38 89 4c 24 40 8b 4c 24 48 8d 04 12 "
        "8b 54 24 50 2b c8 8b 44 24 5c 2b ca 8b 54 24 4c "
        "89 4c 24 3c 8b 0d 74 09 00 00 2b d0 89 54 24 40 "
        "8b 01 8d 54 24 34 52 ff 50 6c"
    )
    candidate_body = definition.data[unit_start:unit_end]
    if candidate_body != exact_body:
        semantic_rows = _cc_callable_identity._candidate_contiguous_coff_unit_instructions(
            candidate,
            offsets=addresses,
            unit_start=unit_start,
            unit_end=unit_end,
            label="HUD EnsureHudLoaded objective text SetPos candidate bridge",
        )

        read_summary_index = index_by_address[fixed["read_summary"]]
        setpos_summary_index = index_by_address[fixed["setpos_summary"]]
        summary_rows = tuple(
            range(read_summary_index + 1, setpos_summary_index + 1)
        )
        scalar_values: dict[str, frozenset[int]] = {}
        scalar_kinds: dict[int, tuple[str, int | None]] = {}
        register_kinds: dict[str, str] = {}
        pushed_values: list[frozenset[int]] = []
        receiver_loads: list[tuple[int, int]] = []
        vptr_loads: list[int] = []
        push_count = 0
        next_scalar_id = 0
        for index in summary_rows:
            instruction = candidate.instructions[index]
            offset = addresses[index]
            if offset is None:
                raise ValueError(
                    "HUD EnsureHudLoaded objective text SetPos candidate "
                    "bridge requires exact summary COD offsets"
                )
            mnemonic = _cc_cfg._instruction_mnemonic(instruction)
            operand = _cc_cfg._instruction_operand(instruction)
            if index == setpos_summary_index:
                expression, slot = _cc_targets._memory_slot(operand)
                call_register_match = re.fullmatch(
                    r"(?:eax|ebx|ecx|edx|esi|edi|ebp)(?:\+(?:12|0xc))?",
                    expression,
                    flags=re.IGNORECASE,
                )
                call_register = (
                    re.match(r"[a-z]+", expression, flags=re.IGNORECASE).group(0)
                    if call_register_match is not None
                    else ""
                )
                if (
                    mnemonic != "call"
                    or slot
                    != _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_TEXT_SETPOS_SLOT
                    or register_kinds.get(call_register.lower()) != "vptr"
                    or bytes(
                        int(value, 16) for value in instruction.bytes
                    )
                    != definition.data[
                        offset : offset + len(instruction.bytes)
                    ]
                    or any(
                        definition.relocation_mask[item]
                        for item in range(
                            offset,
                            offset + len(instruction.bytes),
                        )
                    )
                ):
                    raise ValueError(
                        "HUD EnsureHudLoaded objective text SetPos candidate "
                        "bridge rejects summary vptr/slot/call drift"
                    )
                continue

            if mnemonic == "mov" and "," in operand:
                destination, source = (
                    item.strip().lower()
                    for item in operand.split(",", 1)
                )
                if destination not in {
                    "eax",
                    "ebx",
                    "ecx",
                    "edx",
                    "esi",
                    "edi",
                    "ebp",
                }:
                    raise ValueError(
                        "HUD EnsureHudLoaded objective text SetPos candidate "
                        "bridge rejects non-register summary assignment"
                    )
                source_text = source.lower()
                if (
                    _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL.lower()
                    in source_text
                    and re.search(
                        r"\+(?:2084|0x824)\b",
                        source_text,
                        flags=re.IGNORECASE,
                    )
                ):
                    encoded = bytes(
                        int(value, 16) for value in instruction.bytes
                    )
                    if (
                        destination != "ecx"
                        or encoded[:2] != bytes.fromhex("8b 0d")
                        or len(encoded) != 6
                        or definition.data[offset : offset + 6] != encoded
                    ):
                        raise ValueError(
                            "HUD EnsureHudLoaded objective text SetPos "
                            "candidate bridge rejects summary receiver load"
                        )
                    receiver_loads.append((index, offset))
                    scalar_values.pop(destination, None)
                    register_kinds[destination] = "receiver"
                    continue
                expression, displacement = _cc_targets._memory_slot(source)
                if (
                    expression == "ecx"
                    and displacement in {None, 0}
                    and register_kinds.get("ecx") == "receiver"
                ):
                    encoded = bytes(
                        int(value, 16) for value in instruction.bytes
                    )
                    if (
                        len(encoded) != 2
                        or encoded[0] != 0x8B
                        or definition.data[offset : offset + 2] != encoded
                        or any(
                            definition.relocation_mask[item]
                            for item in range(offset, offset + 2)
                        )
                    ):
                        raise ValueError(
                            "HUD EnsureHudLoaded objective text SetPos "
                            "candidate bridge rejects summary vptr load"
                        )
                    vptr_loads.append(index)
                    scalar_values.pop(destination, None)
                    register_kinds[destination] = "vptr"
                    continue

                scalar_kind = ""
                if "_objectivecenter$" in source_text:
                    scalar_kind = "center"
                elif re.search(r"\b_x\$[a-z0-9_]*", source_text):
                    scalar_kind = "x"
                elif re.search(r"\b_y\$[a-z0-9_]*", source_text):
                    scalar_kind = "y"
                if scalar_kind:
                    encoded = bytes(
                        int(value, 16) for value in instruction.bytes
                    )
                    if (
                        len(encoded) != 4
                        or encoded[0] != 0x8B
                        or encoded[2] != 0x24
                        or definition.data[offset : offset + 4] != encoded
                        or any(
                            definition.relocation_mask[item]
                            for item in range(offset, offset + 4)
                        )
                    ):
                        raise ValueError(
                            "HUD EnsureHudLoaded objective text SetPos "
                            "candidate bridge rejects summary argument load"
                        )
                    stack_displacement = (
                        encoded[3] - 0x100
                        if encoded[3] & 0x80
                        else encoded[3]
                    )
                    scalar_id = next_scalar_id
                    next_scalar_id += 1
                    scalar_kinds[scalar_id] = (
                        scalar_kind,
                        (
                            stack_displacement - (4 * push_count)
                            if scalar_kind == "center"
                            else None
                        ),
                    )
                    scalar_values[destination] = frozenset({scalar_id})
                    register_kinds[destination] = "scalar"
                    continue
                raise ValueError(
                    "HUD EnsureHudLoaded objective text SetPos candidate "
                    "bridge rejects unreviewed summary mov"
                )

            if mnemonic == "add" and "," in operand:
                destination, source = (
                    item.strip().lower()
                    for item in operand.split(",", 1)
                )
                left = scalar_values.get(destination, frozenset())
                right = scalar_values.get(source, frozenset())
                combined = left | right
                combined_kinds = {
                    scalar_kinds[item][0] for item in combined
                }
                if (
                    len(left) != 1
                    or len(right) != 1
                    or left & right
                    or len(combined) != 2
                    or combined_kinds
                    not in ({"x", "center"}, {"y", "center"})
                ):
                    raise ValueError(
                        "HUD EnsureHudLoaded objective text SetPos candidate "
                        "bridge rejects summary argument origins"
                    )
                scalar_values[destination] = combined
                register_kinds[destination] = "scalar"
                continue

            if mnemonic == "push":
                register = operand.strip().lower()
                value = scalar_values.get(register, frozenset())
                if len(value) != 2:
                    raise ValueError(
                        "HUD EnsureHudLoaded objective text SetPos candidate "
                        "bridge rejects summary argument consumption"
                    )
                pushed_values.append(value)
                push_count += 1
                continue

            raise ValueError(
                "HUD EnsureHudLoaded objective text SetPos candidate bridge "
                "rejects unreviewed summary instruction"
            )

        pushed_kinds = [
            {scalar_kinds[item][0] for item in value}
            for value in pushed_values
        ]
        pushed_centers = [
            next(
                scalar_kinds[item][1]
                for item in value
                if scalar_kinds[item][0] == "center"
            )
            for value in pushed_values
        ] if len(pushed_values) == 2 else []
        if (
            len(receiver_loads) != 1
            or len(vptr_loads) != 1
            or len(scalar_kinds) != 4
            or pushed_kinds != [{"y", "center"}, {"x", "center"}]
            or pushed_values[0] & pushed_values[1]
            or pushed_values[0] | pushed_values[1]
            != frozenset(scalar_kinds)
            or pushed_centers[0] != pushed_centers[1] + 4
            or _cc_cfg._cleanup_after(
                candidate.instructions,
                setpos_summary_index,
            )
            is not None
        ):
            raise ValueError(
                "HUD EnsureHudLoaded objective text SetPos candidate bridge "
                "rejects summary receiver/argument/cleanup unique-consumption "
                "drift"
            )
        next_invocation = next(
            (
                index
                for index in range(
                    setpos_summary_index + 1,
                    len(candidate.instructions),
                )
                if _cc_cfg._instruction_mnemonic(candidate.instructions[index])
                in {"call", "jmp"}
            ),
            len(candidate.instructions),
        )
        for instruction in candidate.instructions[
            setpos_summary_index + 1 : next_invocation
        ]:
            operand = _cc_cfg._instruction_operand(instruction).lower()
            if re.search(r"\beax\b", operand):
                destination = (
                    operand.split(",", 1)[0].strip()
                    if "," in operand
                    else ""
                )
                if destination != "eax":
                    raise ValueError(
                        "HUD EnsureHudLoaded objective text SetPos candidate "
                        "bridge rejects consumed summary SetPos result"
                    )
            if _cc_cfg._instruction_may_clobber_register(instruction, "eax"):
                break

        fixed["load_summary"] = receiver_loads[0][1]
        read_desc_index = index_by_address[fixed["read_desc"]]
        setpos_desc_index = index_by_address[fixed["setpos_desc"]]
        desc_window_start = read_desc_index + 1
        desc_push_rows = [
            index
            for index in range(desc_window_start, setpos_desc_index)
            if _cc_cfg._instruction_mnemonic(candidate.instructions[index]) == "push"
        ]

        def exact_desc_unmasked(index: int) -> bytes:
            offset = addresses[index]
            if offset is None:
                raise ValueError(
                    "HUD EnsureHudLoaded objective text SetPos candidate "
                    "bridge requires exact description argument offsets"
                )
            encoded = bytes(
                int(value, 16)
                for value in candidate.instructions[index].bytes
            )
            if (
                not encoded
                or definition.data[offset : offset + len(encoded)] != encoded
                or any(
                    definition.relocation_mask[item]
                    for item in range(offset, offset + len(encoded))
                )
                or any(
                    relocation.offset < offset + len(encoded)
                    and relocation.offset + 4 > offset
                    for relocation in definition.relocations
                )
            ):
                raise ValueError(
                    "HUD EnsureHudLoaded objective text SetPos candidate "
                    "bridge rejects description argument byte/mask drift"
                )
            return encoded

        def desc_last_definition(
            register: str,
            start: int,
            end: int,
        ) -> int:
            return next(
                (
                    index
                    for index in range(end - 1, start - 1, -1)
                    if _cc_cfg._instruction_may_clobber_register(
                        candidate.instructions[index],
                        register,
                    )
                ),
                -1,
            )

        named_local_loads = {
            local_name: [
                index
                for index in range(desc_window_start, setpos_desc_index)
                if (
                    _cc_cfg._instruction_mnemonic(candidate.instructions[index])
                    == "mov"
                    and "," in _cc_cfg._instruction_operand(
                        candidate.instructions[index]
                    )
                    and re.search(
                        rf"\b_{local_name}\$[a-z0-9_]*",
                        _cc_cfg._instruction_operand(candidate.instructions[index])
                        .split(",", 1)[1]
                        .lower(),
                    )
                )
            ]
            for local_name in ("x", "y")
        }
        center_loads = [
            index
            for index in range(desc_window_start, setpos_desc_index)
            if (
                _cc_cfg._instruction_mnemonic(candidate.instructions[index]) == "mov"
                and "," in _cc_cfg._instruction_operand(candidate.instructions[index])
                and "_objectivecenter$"
                in _cc_cfg._instruction_operand(candidate.instructions[index])
                .split(",", 1)[1]
                .lower()
            )
        ]
        if (
            len(desc_push_rows) != 2
            or any(len(rows) != 1 for rows in named_local_loads.values())
            or len(center_loads) != 2
        ):
            raise ValueError(
                "HUD EnsureHudLoaded objective text SetPos candidate bridge "
                "requires unique named x/y and center definitions for two "
                "description arguments"
            )

        desc_center_displacements: list[int] = []
        for push_position, (push_index, local_name) in enumerate(
            zip(desc_push_rows, ("y", "x"))
        ):
            push_operand = _cc_cfg._instruction_operand(
                candidate.instructions[push_index]
            ).strip().lower()
            if (
                push_operand
                not in {"eax", "ebx", "ecx", "edx", "esi", "edi", "ebp"}
                or len(exact_desc_unmasked(push_index)) != 1
            ):
                raise ValueError(
                    "HUD EnsureHudLoaded objective text SetPos candidate "
                    "bridge rejects description argument push form"
                )
            add_index = desc_last_definition(
                push_operand,
                desc_window_start,
                push_index,
            )
            add_operand = (
                _cc_cfg._instruction_operand(candidate.instructions[add_index])
                if add_index >= 0
                else ""
            )
            if (
                add_index < 0
                or _cc_cfg._instruction_mnemonic(candidate.instructions[add_index])
                != "add"
                or "," not in add_operand
            ):
                raise ValueError(
                    "HUD EnsureHudLoaded objective text SetPos candidate "
                    "bridge requires one reaching coordinate add per push"
                )
            add_destination, center_register = (
                item.strip().lower()
                for item in add_operand.split(",", 1)
            )
            if (
                add_destination != push_operand
                or center_register
                not in {"eax", "ebx", "ecx", "edx", "esi", "edi", "ebp"}
            ):
                raise ValueError(
                    "HUD EnsureHudLoaded objective text SetPos candidate "
                    "bridge rejects coordinate-add register lineage"
                )
            local_index = desc_last_definition(
                push_operand,
                desc_window_start,
                add_index,
            )
            center_index = desc_last_definition(
                center_register,
                desc_window_start,
                add_index,
            )
            next_push_register_definition = next(
                (
                    index
                    for index in range(push_index + 1, setpos_desc_index)
                    if _cc_cfg._instruction_may_clobber_register(
                        candidate.instructions[index],
                        push_operand,
                    )
                ),
                setpos_desc_index,
            )
            if (
                local_index != named_local_loads[local_name][0]
                or center_index not in center_loads
                or local_index >= add_index
                or center_index >= add_index
                or any(
                    (
                        _cc_cfg._instruction_mnemonic(
                            candidate.instructions[index]
                        )
                        == "push"
                        and _cc_cfg._instruction_operand(
                            candidate.instructions[index]
                        )
                        .strip()
                        .lower()
                        == push_operand
                    )
                    for index in range(
                        push_index + 1,
                        next_push_register_definition,
                    )
                )
            ):
                raise ValueError(
                    "HUD EnsureHudLoaded objective text SetPos candidate "
                    f"bridge rejects named {local_name} reaching definition "
                    "or unique consumption"
                )
            local_bytes = exact_desc_unmasked(local_index)
            center_bytes = exact_desc_unmasked(center_index)
            exact_desc_unmasked(add_index)
            if (
                len(local_bytes) != 4
                or local_bytes[0] != 0x8B
                or local_bytes[2] != 0x24
                or len(center_bytes) != 4
                or center_bytes[0] != 0x8B
                or center_bytes[2] != 0x24
            ):
                raise ValueError(
                    "HUD EnsureHudLoaded objective text SetPos candidate "
                    "bridge rejects named-local or center load bytes"
                )
            raw_center_displacement = (
                center_bytes[3] - 0x100
                if center_bytes[3] & 0x80
                else center_bytes[3]
            )
            pushes_before_center = sum(
                1
                for index in desc_push_rows
                if index < center_index
            )
            desc_center_displacements.append(
                raw_center_displacement - (4 * pushes_before_center)
            )
            if push_position == 0 and local_name != "y":
                raise AssertionError("description y argument must be first")

        desc_call = candidate.instructions[setpos_desc_index]
        desc_call_expression, desc_call_slot = _cc_targets._memory_slot(
            _cc_cfg._instruction_operand(desc_call)
        )
        desc_call_register_match = re.match(
            r"(eax|ebx|ecx|edx|esi|edi|ebp)",
            desc_call_expression,
            flags=re.IGNORECASE,
        )
        desc_call_register = (
            desc_call_register_match.group(1).lower()
            if desc_call_register_match is not None
            else ""
        )
        desc_receiver_index = index_by_address[fixed["load_desc"]]
        desc_vptr_rows = [
            index
            for index in range(desc_receiver_index + 1, setpos_desc_index)
            if (
                _cc_cfg._instruction_mnemonic(candidate.instructions[index]) == "mov"
                and "," in _cc_cfg._instruction_operand(candidate.instructions[index])
                and _cc_cfg._instruction_operand(candidate.instructions[index])
                .split(",", 1)[0]
                .strip()
                .lower()
                == desc_call_register
                and _cc_targets._memory_slot(
                    _cc_cfg._instruction_operand(candidate.instructions[index])
                    .split(",", 1)[1]
                )[0]
                == "ecx"
                and _cc_targets._memory_slot(
                    _cc_cfg._instruction_operand(candidate.instructions[index])
                    .split(",", 1)[1]
                )[1]
                in {None, 0}
            )
        ]
        if (
            desc_center_displacements[0]
            != desc_center_displacements[1] + 4
            or len(desc_vptr_rows) != 1
            or desc_call_slot
            != _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_TEXT_SETPOS_SLOT
            or exact_desc_unmasked(desc_vptr_rows[0]) not in {
                b"\x8b\x01",
                b"\x8b\x09",
                b"\x8b\x11",
                b"\x8b\x19",
                b"\x8b\x29",
                b"\x8b\x31",
                b"\x8b\x39",
            }
            or _cc_cfg._cleanup_after(
                candidate.instructions,
                setpos_desc_index,
            )
            is not None
        ):
            raise ValueError(
                "HUD EnsureHudLoaded objective text SetPos candidate bridge "
                "rejects description receiver/vptr/slot/argument-order or "
                "cleanup drift"
            )
        desc_next_invocation = next(
            (
                index
                for index in range(
                    setpos_desc_index + 1,
                    len(candidate.instructions),
                )
                if _cc_cfg._instruction_mnemonic(candidate.instructions[index])
                in {"call", "jmp"}
            ),
            len(candidate.instructions),
        )
        for instruction in candidate.instructions[
            setpos_desc_index + 1 : desc_next_invocation
        ]:
            operand = _cc_cfg._instruction_operand(instruction).lower()
            if re.search(r"\beax\b", operand):
                destination = (
                    operand.split(",", 1)[0].strip()
                    if "," in operand
                    else ""
                )
                if destination != "eax":
                    raise ValueError(
                        "HUD EnsureHudLoaded objective text SetPos candidate "
                        "bridge rejects consumed description SetPos result"
                    )
            if _cc_cfg._instruction_may_clobber_register(instruction, "eax"):
                break

        _cc_callable_identity._require_ordered_candidate_semantic_patterns(
            semantic_rows,
            (
                rf"lea\s+edx\s*,.*_y\$[A-Za-z0-9_]+\[esp\+148\]",
                rf"lea\s+edx\s*,.*_x\$[A-Za-z0-9_]+\[esp\+156\]",
                rf"call\s+{read_int3}(?:\s*;.*)?",
                rf"lea\s+eax\s*,.*_y\$[A-Za-z0-9_]+\[esp\+148\]",
                rf"lea\s+edx\s*,.*_x\$[A-Za-z0-9_]+\[esp\+148\]",
                rf"call\s+{read_int3}(?:\s*;.*)?",
                rf"mov\s+ecx\s*,.*{aggregate}\+(?:2420|0x974)",
                r"call\s+.*\[edx\+(?:12|0xc)\]",
                rf"mov\s+.*_wrapRect\$[A-Za-z0-9_]+\[esp\+148\].*",
                rf"mov\s+\w+\s*,.*_x\$[A-Za-z0-9_]+\[esp\+148\]",
                (
                    r"mov\s+\w+\s*,.*_panelCenter\$"
                    r"[A-Za-z0-9_]+\[esp\+148\]"
                ),
                (
                    r"mov\s+.*_wrapRect\$"
                    r"[A-Za-z0-9_]+\[esp\+156\].*"
                ),
                rf"mov\s+ecx\s*,.*{aggregate}\+(?:2420|0x974)",
                (
                    r"mov\s+.*_wrapRect\$"
                    r"[A-Za-z0-9_]+\[esp\+160\].*"
                ),
                (
                    r"lea\s+edx\s*,.*_wrapRect\$"
                    r"[A-Za-z0-9_]+\[esp\+148\]"
                ),
                r"push\s+edx",
                r"call\s+.*\[eax\+(?:108|0x6c)\]",
            ),
            label="HUD EnsureHudLoaded objective text SetPos candidate bridge",
        )
    dir32_specs = {
        0x4FD + unit_shift: 0xAAC,
        fixed["load_summary"] + 2: (
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_SUMMARY_PANEL_DISPLACEMENT
        ),
        0x543 + unit_shift: _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_DESC_PANEL_DISPLACEMENT,
        0x599 + unit_shift: _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_DESC_PANEL_DISPLACEMENT,
    }
    rel32_specs = {
        0x50A + unit_shift: _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_READ_INT3_SYMBOL,
        0x53D + unit_shift: _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_READ_INT3_SYMBOL,
    }
    bounded_by_offset: dict[int, list[CoffRelocation]] = {}
    for row in definition.relocations:
        if unit_start <= row.offset < unit_end:
            bounded_by_offset.setdefault(row.offset, []).append(row)
    expected_offsets = set(dir32_specs) | set(rel32_specs)
    if (
        set(bounded_by_offset) != expected_offsets
        or any(len(rows) != 1 for rows in bounded_by_offset.values())
        or any(
            bounded_by_offset[offset][0].type != IMAGE_REL_I386_DIR32
            or bounded_by_offset[offset][0].symbol_name
            != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
            or struct.unpack_from("<I", definition.data, offset)[0] != addend
            for offset, addend in dir32_specs.items()
        )
        or any(
            bounded_by_offset[offset][0].type != IMAGE_REL_I386_REL32
            or bounded_by_offset[offset][0].symbol_name != symbol
            or struct.unpack_from("<I", definition.data, offset)[0] != 0
            for offset, symbol in rel32_specs.items()
        )
        or {
            index
            for index in range(unit_start, unit_end)
            if definition.relocation_mask[index]
        }
        != {
            index
            for offset in expected_offsets
            for index in range(offset, offset + 4)
        }
        or definition.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or definition.defined_external_functions.count(
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_READ_INT3_SYMBOL
        )
        != 1
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective text SetPos candidate bridge "
            "rejects malformed, aliased, wrong-target/addend/mask "
            "DIR32/REL32 fields"
        )

    invocation_specs = {
        "read_summary": (
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_READ_SUMMARY_ORDINAL
        ),
        "setpos_summary": (
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_SETPOS_SUMMARY_ORDINAL
        ),
        "read_desc": _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_READ_DESC_ORDINAL,
        "setpos_desc": (
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_SETPOS_DESC_ORDINAL
        ),
        "boundary": _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_WORD_WRAP_ORDINAL,
    }
    bounded_calls = [
        address
        for address, instruction in zip(addresses, candidate.instructions)
        if (
            address is not None
            and fixed["read_summary"] <= address < fixed["boundary"]
            and _cc_cfg._instruction_mnemonic(instruction) in {"call", "jmp"}
        )
    ]
    word_wrap_calls = [
        address
        for address, instruction in zip(addresses, candidate.instructions)
        if (
            address is not None
            and fixed["setpos_desc"] <= address < fixed["right_boundary"]
            and _cc_cfg._instruction_mnemonic(instruction) in {"call", "jmp"}
        )
    ]
    ordinal_by_index = {
        index: ordinal
        for ordinal, index in enumerate(
            _cc_callable_identity._candidate_static_invocation_indices(
                candidate,
                caller_start=normalized_start,
                caller_end_exclusive=caller_end_exclusive,
            )
        )
    }
    if (
        bounded_calls
        != [
            fixed["read_summary"],
            fixed["setpos_summary"],
            fixed["read_desc"],
            fixed["setpos_desc"],
        ]
        or any(
            _cc_cfg._cleanup_after(candidate.instructions, index_by_address[fixed[name]])
            is not None
            for name in invocation_specs
        )
        or word_wrap_calls
        != [fixed["setpos_desc"], fixed["boundary"]]
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective text SetPos candidate bridge "
            "rejects missing, extra, reordered, wrong-cleanup, or "
            "absorbed-boundary calls or consumed word-wrap result"
        )

    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    return {
        normalize_address(
            hex(fixed["load_summary"])
        ): ReviewedAbsoluteStorageLoadBridge(
            register="ecx",
            aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            displacement=(
                _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_SUMMARY_PANEL_DISPLACEMENT
            ),
            access_width=4,
            storage_identity=aggregate_identity,
            canonicalize_nested_load=True,
        ),
        normalize_address(
            hex(
                _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_LOAD_DESC_OFFSET
                + unit_shift
            )
        ): ReviewedAbsoluteStorageLoadBridge(
            register="ecx",
            aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            displacement=_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_DESC_PANEL_DISPLACEMENT,
            access_width=4,
            storage_identity=aggregate_identity,
            canonicalize_nested_load=True,
        ),
        normalize_address(
            hex(
                _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_WORD_WRAP_LOAD_OFFSET
                + unit_shift
            )
        ): ReviewedAbsoluteStorageLoadBridge(
            register="ecx",
            aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            displacement=_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_DESC_PANEL_DISPLACEMENT,
            access_width=4,
            storage_identity=aggregate_identity,
            canonicalize_nested_load=True,
        ),
    }


def _hud_ui_mgr_ensure_objective_label_setpos_retail_guard(
    instructions: Sequence[Instruction],
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> None:
    """Guard retail's exact objective-label SetPos provenance unit."""
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START:
        return
    _cc_recoil_hud_sensor._require_hud_ui_mgr_ensure_sensor_center_authority(
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    aggregate_base = address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
    if (
        address_value(_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_LABEL_PANEL_ADDRESS)
        != aggregate_base
        + _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_LABEL_PANEL_DISPLACEMENT
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective-label SetPos retail guard requires "
            "the exact typed label-panel address"
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
    index_by_address = {
        address: index
        for index, address in enumerate(addresses)
        if address is not None and counts.get(address) == 1
    }
    fixed_rows = {
        0x410698: (
            ("e8", "33", "34", "00", "00"),
            r"call\s+(?:0x413ad0|HudUiLayoutNode::ReadInt3)",
        ),
        0x41069D: (("8b", "45", "fc"), r"mov\s+eax\s*,.*\[ebp-0x4\]"),
        0x4106A0: (
            ("8b", "35", "f4", "61", "4e", "00"),
            r"mov\s+esi\s*,.*\[?0x4e61f4\]?",
        ),
        0x4106A6: (
            ("8b", "0d", "f8", "66", "4e", "00"),
            r"mov\s+ecx\s*,.*\[?0x4e66f8\]?",
        ),
        0x4106AC: (("03", "c6"), r"add\s+eax\s*,\s*esi"),
        0x4106AE: (("50",), r"push\s+eax"),
        0x4106AF: (("8b", "45", "f8"), r"mov\s+eax\s*,.*\[ebp-0x8\]"),
        0x4106B2: (("8b", "11"), r"mov\s+edx\s*,.*\[ecx\]"),
        0x4106B4: (("50",), r"push\s+eax"),
        0x4106B5: (
            ("ff", "52", "0c"),
            r"call\s+.*\[edx\+(?:0xc|12)\]",
        ),
        0x4106B8: (
            ("8b", "0d", "f8", "66", "4e", "00"),
            r"mov\s+ecx\s*,.*\[?0x4e66f8\]?",
        ),
        0x4106BE: (("8b", "31"), r"mov\s+esi\s*,.*\[ecx\]"),
        0x4106C0: (("b9", "06", "09", "00", "00"), r"mov\s+ecx\s*,\s*0x906"),
        0x4106C5: (
            ("e8", "26", "55", "09", "00"),
            r"call\s+(?:0x4a5bf0|zLoc::GetMessageString)",
        ),
    }
    if any(
        address not in by_address
        or tuple(value.lower() for value in by_address[address].bytes) != body
        or re.fullmatch(
            pattern,
            by_address[address].raw_text.strip(),
            flags=re.IGNORECASE,
        )
        is None
        for address, (body, pattern) in fixed_rows.items()
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective-label SetPos retail guard "
            "requires the exact ReadInt3/receiver/two-integer-argument/"
            "slot-0x0c unit and GetMessageString right boundary"
        )

    fixed_indices = [index_by_address[address] for address in fixed_rows]
    invocation_addresses = {
        "read": address_value(
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_READ_LABEL_ADDRESS
        ),
        "setpos": address_value(
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_SETPOS_LABEL_ADDRESS
        ),
        "right_boundary": address_value(
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_LABEL_MESSAGE_ADDRESS
        ),
    }
    bounded_calls = [
        address
        for address, instruction in zip(addresses, instructions)
        if (
            address is not None
            and invocation_addresses["read"]
            <= address
            <= invocation_addresses["right_boundary"]
            and _cc_cfg._instruction_mnemonic(instruction) in {"call", "jmp"}
        )
    ]
    ordinal_by_index = {
        index: ordinal
        for ordinal, index in enumerate(
            index
            for index, instruction in enumerate(instructions)
            if _cc_cfg._instruction_mnemonic(instruction) in {"call", "jmp"}
        )
    }
    ordinal_specs = {
        "read": _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_READ_LABEL_ORDINAL,
        "setpos": _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_SETPOS_LABEL_ORDINAL,
        "right_boundary": (
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_LABEL_MESSAGE_ORDINAL
        ),
    }
    if (
        fixed_indices != sorted(fixed_indices)
        or len(set(fixed_indices)) != len(fixed_indices)
        or bounded_calls
        != [
            invocation_addresses["read"],
            invocation_addresses["setpos"],
            invocation_addresses["right_boundary"],
        ]
        or any(
            ordinal_by_index.get(index_by_address[invocation_addresses[name]])
            != ordinal
            for name, ordinal in ordinal_specs.items()
        )
        or _cc_cfg._cleanup_after(
            instructions,
            index_by_address[invocation_addresses["setpos"]],
        )
        is not None
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective-label SetPos retail guard rejects "
            "missing, extra, reordered, wrong-cleanup, result-consuming, or "
            "absorbed-boundary calls"
        )


def _hud_ui_mgr_ensure_objective_label_setpos_candidate_absolute_load_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedAbsoluteStorageLoadBridge]:
    """Bridge only the exact objective-label receiver load."""
    from _recoil.call_contract.records import ReviewedAbsoluteStorageLoadBridge
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START:
        return {}
    _cc_recoil_hud_sensor._require_hud_ui_mgr_ensure_sensor_center_authority(
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    definition = candidate.caller_definition
    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START
    ]
    if (
        definition is None
        or definition.symbol != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_SYMBOL
        or not definition.data
        or len(definition.relocation_mask) != len(definition.data)
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
            "HUD EnsureHudLoaded objective-label SetPos candidate bridge "
            "requires a nonempty candidate COMDAT and current HUD target"
        )

    read_contract = {
        "ordinal": _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_READ_LABEL_ORDINAL,
        "form": "call",
        "dispatch": "direct",
        "identity_kind": "direct",
        "target_identity": _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_READ_INT3_IDENTITY,
        "storage_identity": "",
        "slot_displacement": None,
        "cleanup_bytes": None,
    }
    setpos_contract = {
        "ordinal": _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_SETPOS_LABEL_ORDINAL,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": (
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_LABEL_PANEL_STORAGE_IDENTITY
        ),
        "slot_displacement": _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_TEXT_SETPOS_SLOT,
        "cleanup_bytes": None,
    }
    expected_contracts = {
        _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_READ_LABEL_ORDINAL: read_contract,
        _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_SETPOS_LABEL_ORDINAL: (
            setpos_contract
        ),
    }
    if (
        len(expected) <= _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_SETPOS_LABEL_ORDINAL
        or any(
            expected[ordinal] != contract
            for ordinal, contract in expected_contracts.items()
        )
    ):
        observed_contracts = {
            ordinal: expected[ordinal] if ordinal < len(expected) else None
            for ordinal in expected_contracts
        }
        raise ValueError(
            "HUD EnsureHudLoaded objective-label SetPos candidate bridge "
            "requires exact independently retail-derived ReadInt3 and "
            "storage/virtual-slot contracts with no unconditional direct "
            f"target: observed={observed_contracts!r}"
        )

    start = 0
    aggregate = re.escape(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
    read_int3 = re.escape(_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_READ_INT3_SYMBOL)
    get_message = re.escape(
        _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_GET_MESSAGE_STRING_SYMBOL
    )
    fixed = {
        "read": start
        + _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_READ_LABEL_OFFSET,
        "load": start
        + _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_LOAD_LABEL_OFFSET,
        "setpos": start
        + _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_SETPOS_LABEL_OFFSET,
        "right_boundary": start
        + _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_LABEL_MESSAGE_OFFSET,
    }
    baseline_fixed = fixed
    baseline_expected_rows = {
        baseline_fixed["read"]: (
            ("e8", "00", "00", "00", "00"),
            rf"call\s+{read_int3}(?:\s*;.*)?",
        ),
        start + 0x624: (
            ("8b", "35", "24", "03", "00", "00"),
            rf"mov\s+esi\s*,.*{aggregate}\+(?:804|0x324)",
        ),
        baseline_fixed["load"]: (
            ("8b", "0d", "28", "08", "00", "00"),
            rf"mov\s+ecx\s*,.*{aggregate}\+(?:2088|0x828)",
        ),
        start + 0x630: (
            ("8b", "54", "24", "14"),
            r"mov\s+edx\s*,.*\[esp\+(?:20|0x14|148|0x94)\]",
        ),
        start + 0x634: (("8b", "01"), r"mov\s+eax\s*,.*\[ecx\]"),
        start + 0x636: (("03", "f2"), r"add\s+esi\s*,\s*edx"),
        start + 0x638: (
            ("8b", "54", "24", "10"),
            r"mov\s+edx\s*,.*\[esp\+(?:16|0x10|148|0x94)\]",
        ),
        start + 0x63C: (("56",), r"push\s+esi"),
        start + 0x63D: (("52",), r"push\s+edx"),
        baseline_fixed["setpos"]: (
            ("ff", "50", "0c"),
            r"call\s+.*\[eax\+(?:12|0xc)\]",
        ),
        start + 0x641: (
            ("a1", "28", "08", "00", "00"),
            rf"mov\s+eax\s*,.*{aggregate}\+(?:2088|0x828)",
        ),
        start + 0x646: (
            ("b9", "06", "09", "00", "00"),
            r"mov\s+ecx\s*,\s*(?:2310|0x906)",
        ),
        start + 0x64B: (("8b", "30"), r"mov\s+esi\s*,.*\[eax\]"),
        baseline_fixed["right_boundary"]: (
            ("e8", "00", "00", "00", "00"),
            rf"call\s+{get_message}(?:\s*;.*)?",
        ),
    }
    baseline_locator_rows = {
        baseline_fixed[key]: baseline_expected_rows[baseline_fixed[key]]
        for key in ("read", "setpos", "right_boundary")
    }
    unit_shift, addresses, by_address, index_by_address = (
        _cc_callable_identity._locate_shifted_candidate_instruction_unit(
            candidate,
            baseline_locator_rows,
            label="HUD EnsureHudLoaded objective-label SetPos candidate bridge",
        )
    )
    fixed = {
        key: address + unit_shift
        for key, address in baseline_fixed.items()
    }
    expected_rows = {
        address + unit_shift: row
        for address, row in baseline_expected_rows.items()
    }

    unit_start = (
        _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_READ_LABEL_OFFSET + unit_shift
    )
    unit_end = (
        _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_LABEL_MESSAGE_OFFSET + unit_shift
    )
    exact_body = bytes.fromhex(
        "e8 00 00 00 00 "
        "8b 35 24 03 00 00 "
        "8b 0d 28 08 00 00 "
        "8b 54 24 14 8b 01 03 f2 8b 54 24 10 56 52 ff 50 0c "
        "a1 28 08 00 00 b9 06 09 00 00 8b 30"
    )
    candidate_body = definition.data[unit_start:unit_end]
    if candidate_body != exact_body:
        semantic_rows = _cc_callable_identity._candidate_contiguous_coff_unit_instructions(
            candidate,
            offsets=addresses,
            unit_start=unit_start,
            unit_end=unit_end,
            label="HUD EnsureHudLoaded objective-label SetPos candidate bridge",
        )
        _cc_callable_identity._require_ordered_candidate_semantic_patterns(
            semantic_rows,
            (
                rf"call\s+{read_int3}(?:\s*;.*)?",
                rf"mov\s+\w+\s*,.*{aggregate}\+(?:804|0x324)",
                rf"mov\s+\w+\s*,.*_y\$[A-Za-z0-9_]+\[esp\+148\]",
                rf"mov\s+ecx\s*,.*{aggregate}\+(?:2088|0x828)",
                r"add\s+\w+\s*,\s*\w+",
                r"push\s+\w+",
                rf"mov\s+\w+\s*,.*_x\$[A-Za-z0-9_]+\[esp\+152\]",
                r"mov\s+eax\s*,.*\[ecx\]",
                r"push\s+\w+",
                r"call\s+.*\[eax\+(?:12|0xc)\]",
                rf"mov\s+eax\s*,.*{aggregate}\+(?:2088|0x828)",
                r"mov\s+ecx\s*,\s*(?:2310|0x906)",
                r"mov\s+esi\s*,.*\[eax\]",
            ),
            label="HUD EnsureHudLoaded objective-label SetPos candidate bridge",
        )
    aggregate_sites: list[tuple[int, int]] = []
    for instruction, address in zip(candidate.instructions, addresses):
        if address is None or not (unit_start <= address < unit_end):
            continue
        raw = instruction.raw_text.strip()
        addend = next(
            (
                value
                for pattern, value in (
                    (
                        rf"mov\s+\w+\s*,.*{aggregate}\+(?:804|0x324)",
                        0x324,
                    ),
                    (
                        rf"mov\s+\w+\s*,.*{aggregate}\+(?:2088|0x828)",
                        _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_LABEL_PANEL_DISPLACEMENT,
                    ),
                )
                if re.fullmatch(pattern, raw, flags=re.IGNORECASE) is not None
            ),
            None,
        )
        if addend is None:
            continue
        encoded = bytes(int(value, 16) for value in instruction.bytes)
        if (
            len(encoded) not in {5, 6}
            or definition.data[address : address + len(encoded)] != encoded
        ):
            raise ValueError(
                "HUD EnsureHudLoaded objective-label SetPos candidate bridge "
                "rejects malformed aggregate-load COD/COFF evidence"
            )
        aggregate_sites.append((address + len(encoded) - 4, addend))
    if [addend for _offset, addend in aggregate_sites] != [
        0x324,
        _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_LABEL_PANEL_DISPLACEMENT,
        _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_LABEL_PANEL_DISPLACEMENT,
    ]:
        raise ValueError(
            "HUD EnsureHudLoaded objective-label SetPos candidate bridge "
            "rejects aggregate-load identity/order drift"
        )
    label_load_addresses = [
        offset - 2
        for offset, addend in aggregate_sites
        if addend == _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_LABEL_PANEL_DISPLACEMENT
        and definition.data[offset - 2 : offset] == b"\x8b\x0d"
    ]
    if len(label_load_addresses) != 1:
        raise ValueError(
            "HUD EnsureHudLoaded objective-label SetPos candidate bridge "
            "requires one direct objective-label receiver load"
        )
    fixed["load"] = label_load_addresses[0]
    dir32_specs = dict(aggregate_sites)
    rel32_specs = {
        fixed["read"] + 1: _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_READ_INT3_SYMBOL,
    }
    bounded_by_offset: dict[int, list[CoffRelocation]] = {}
    for row in definition.relocations:
        if unit_start <= row.offset < unit_end:
            bounded_by_offset.setdefault(row.offset, []).append(row)
    expected_offsets = set(dir32_specs) | set(rel32_specs)
    if (
        set(bounded_by_offset) != expected_offsets
        or any(len(rows) != 1 for rows in bounded_by_offset.values())
        or any(
            bounded_by_offset[offset][0].type != IMAGE_REL_I386_DIR32
            or bounded_by_offset[offset][0].symbol_name
            != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
            or struct.unpack_from("<I", definition.data, offset)[0] != addend
            for offset, addend in dir32_specs.items()
        )
        or any(
            bounded_by_offset[offset][0].type != IMAGE_REL_I386_REL32
            or bounded_by_offset[offset][0].symbol_name != symbol
            or struct.unpack_from("<I", definition.data, offset)[0] != 0
            for offset, symbol in rel32_specs.items()
        )
        or {
            index
            for index in range(unit_start, unit_end)
            if definition.relocation_mask[index]
        }
        != {
            index
            for offset in expected_offsets
            for index in range(offset, offset + 4)
        }
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective-label SetPos candidate bridge "
            "rejects malformed, aliased, wrong-target/addend/mask "
            "DIR32/REL32 fields"
        )

    boundary_relocations = [
        row
        for row in definition.relocations
        if row.offset
        == _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_LABEL_MESSAGE_OFFSET
        + unit_shift
        + 1
    ]
    boundary_offset = (
        _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_LABEL_MESSAGE_OFFSET
        + unit_shift
        + 1
    )
    if (
        len(boundary_relocations) != 1
        or boundary_relocations[0].type != IMAGE_REL_I386_REL32
        or boundary_relocations[0].symbol_name
        != _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_GET_MESSAGE_STRING_SYMBOL
        or struct.unpack_from("<I", definition.data, boundary_offset)[0] != 0
        or {
            index
            for index in range(boundary_offset, boundary_offset + 4)
            if definition.relocation_mask[index]
        }
        != set(range(boundary_offset, boundary_offset + 4))
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective-label SetPos candidate bridge "
            "requires the exact GetMessageString REL32 right boundary"
        )

    invocation_specs = {
        "read": _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_READ_LABEL_ORDINAL,
        "setpos": (
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_SETPOS_LABEL_ORDINAL
        ),
        "right_boundary": (
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_LABEL_MESSAGE_ORDINAL
        ),
    }
    bounded_calls = [
        address
        for address, instruction in zip(addresses, candidate.instructions)
        if (
            address is not None
            and fixed["read"] <= address <= fixed["right_boundary"]
            and _cc_cfg._instruction_mnemonic(instruction) in {"call", "jmp"}
        )
    ]
    ordinal_by_index = {
        index: ordinal
        for ordinal, index in enumerate(
            _cc_callable_identity._candidate_static_invocation_indices(
                candidate,
                caller_start=normalized_start,
                caller_end_exclusive=caller_end_exclusive,
            )
        )
    }
    if (
        bounded_calls
        != [fixed["read"], fixed["setpos"], fixed["right_boundary"]]
        or _cc_cfg._cleanup_after(
            candidate.instructions,
            index_by_address[fixed["setpos"]],
        )
        is not None
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective-label SetPos candidate bridge "
            "rejects missing, extra, reordered, wrong-cleanup, "
            "result-consuming, or absorbed-boundary calls"
        )

    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    return {
        normalize_address(hex(fixed["load"])): ReviewedAbsoluteStorageLoadBridge(
            register="ecx",
            aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            displacement=_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_LABEL_PANEL_DISPLACEMENT,
            access_width=4,
            storage_identity=aggregate_identity,
            canonicalize_nested_load=True,
        )
    }


def _hud_ui_mgr_ensure_objective_label_settextfmt_retail_vptr_bridge(
    instructions: Sequence[Instruction],
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedLoopVptrStorageBridge]:
    """Bind only retail's cached objective-label SetTextFmt vptr.

    Retail caches the vptr from ``g_HudUiMgr+0x828`` in ESI before the
    localization call, then reloads the receiver and invokes slot +0x74.  The
    cached table value is deliberately not treated as an unconditional target:
    this bridge supplies only the reviewed storage and virtual-slot identity.
    """
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START:
        return {}
    _hud_ui_mgr_ensure_objective_label_setpos_retail_guard(
        instructions,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
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
    index_by_address = {
        address: index
        for index, address in enumerate(addresses)
        if address is not None and counts.get(address) == 1
    }
    fixed = {
        "left_boundary": 0x4106B5,
        "receiver_load": 0x4106B8,
        "vptr_cache": 0x4106BE,
        "message_id": 0x4106C0,
        "message": 0x4106C5,
        "receiver_reload": 0x4106CA,
        "message_argument": 0x4106D0,
        "receiver_argument": 0x4106D1,
        "call": address_value(
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_SETTEXTFMT_ADDRESS
        ),
        "eax_overwrite": 0x4106D5,
        "right_boundary": address_value(
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_NEXT_SETPOS_ADDRESS
        ),
    }
    expected_rows = {
        fixed["left_boundary"]: (
            ("ff", "52", "0c"),
            r"call\s+.*\[edx\+(?:12|0xc)\]",
        ),
        fixed["receiver_load"]: (
            ("8b", "0d", "f8", "66", "4e", "00"),
            r"mov\s+ecx\s*,.*\[?0x4e66f8\]?",
        ),
        fixed["vptr_cache"]: (
            ("8b", "31"),
            r"mov\s+esi\s*,.*\[ecx\]",
        ),
        fixed["message_id"]: (
            ("b9", "06", "09", "00", "00"),
            r"mov\s+ecx\s*,\s*0x906",
        ),
        fixed["message"]: (
            ("e8", "26", "55", "09", "00"),
            r"call\s+(?:0x4a5bf0|zLoc::GetMessageString)",
        ),
        fixed["receiver_reload"]: (
            ("8b", "15", "f8", "66", "4e", "00"),
            r"mov\s+edx\s*,.*\[?0x4e66f8\]?",
        ),
        fixed["message_argument"]: (("50",), r"push\s+eax"),
        fixed["receiver_argument"]: (("52",), r"push\s+edx"),
        fixed["call"]: (
            ("ff", "56", "74"),
            r"call\s+.*\[esi\+(?:116|0x74)\]",
        ),
        fixed["eax_overwrite"]: (
            ("a1", "5c", "db", "4e", "00"),
            r"mov\s+eax\s*,.*\[?0x4edb5c\]?",
        ),
        fixed["right_boundary"]: (
            ("ff", "52", "0c"),
            r"call\s+.*\[edx\+(?:12|0xc)\]",
        ),
    }
    if any(
        address not in by_address
        or tuple(value.lower() for value in by_address[address].bytes) != body
        or re.fullmatch(
            pattern,
            by_address[address].raw_text.strip(),
            flags=re.IGNORECASE,
        )
        is None
        for address, (body, pattern) in expected_rows.items()
    ):
        observed = {
            hex(address): (
                tuple(by_address[address].bytes),
                by_address[address].raw_text.strip(),
            )
            if address in by_address
            else None
            for address in expected_rows
        }
        raise ValueError(
            "HUD EnsureHudLoaded objective-label SetTextFmt retail bridge "
            "requires the exact cached-vptr, receiver reload, two-word "
            "argument, cleanup-8, and SetPos-boundary unit: "
            f"observed={observed!r}"
        )

    fixed_indices = [index_by_address[address] for address in fixed.values()]
    bounded_calls = [
        address
        for address, instruction in zip(addresses, instructions)
        if (
            address is not None
            and fixed["left_boundary"] <= address <= fixed["right_boundary"]
            and _cc_cfg._instruction_mnemonic(instruction) in {"call", "jmp"}
        )
    ]
    ordinal_by_index = {
        index: ordinal
        for ordinal, index in enumerate(
            index
            for index, instruction in enumerate(instructions)
            if _cc_cfg._instruction_mnemonic(instruction) in {"call", "jmp"}
        )
    }
    if (
        fixed_indices != sorted(fixed_indices)
        or len(set(fixed_indices)) != len(fixed_indices)
        or bounded_calls
        != [
            fixed["left_boundary"],
            fixed["message"],
            fixed["call"],
            fixed["right_boundary"],
        ]
        or ordinal_by_index.get(index_by_address[fixed["call"]])
        != _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_SETTEXTFMT_ORDINAL
        or ordinal_by_index.get(index_by_address[fixed["right_boundary"]])
        != _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_NEXT_SETPOS_ORDINAL
        or _cc_cfg._cleanup_after(instructions, index_by_address[fixed["call"]]) != 8
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective-label SetTextFmt retail bridge "
            "rejects missing, extra, reordered, wrong-cleanup, result-using, "
            "or absorbed-boundary calls"
        )

    return {
        _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_SETTEXTFMT_ADDRESS:
        ReviewedLoopVptrStorageBridge(
            register="esi",
            storage_identity=(
                _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_LABEL_PANEL_STORAGE_IDENTITY
            ),
            slot_displacement=_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_SETTEXTFMT_SLOT,
            assembly_source="bn",
        )
    }


def _hud_ui_mgr_ensure_objective_label_settextfmt_candidate_vptr_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedAbsoluteStorageLoadBridge],
    dict[str, ReviewedVptrStorageBridge],
]:
    """Bind the exact current or repaired cached objective-label vptr unit."""
    from _recoil.call_contract.records import (
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedVptrStorageBridge,
    )
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START:
        return {}, {}
    _hud_ui_mgr_ensure_objective_label_setpos_candidate_absolute_load_bridge(
        expected,
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    expected_contract = {
        "ordinal": _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_SETTEXTFMT_ORDINAL,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": (
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_LABEL_PANEL_STORAGE_IDENTITY
        ),
        "slot_displacement": _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_SETTEXTFMT_SLOT,
        "cleanup_bytes": 8,
    }
    ordinal = _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_SETTEXTFMT_ORDINAL
    if len(expected) <= ordinal or expected[ordinal] != expected_contract:
        observed = expected[ordinal] if ordinal < len(expected) else None
        raise ValueError(
            "HUD EnsureHudLoaded objective-label SetTextFmt candidate bridge "
            "requires the independently retail-derived storage/slot, two-"
            "word cleanup, and no unconditional direct target: "
            f"observed={observed!r}"
        )

    definition = candidate.caller_definition
    assert definition is not None
    start = 0
    aggregate = re.escape(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
    get_message = re.escape(
        _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_GET_MESSAGE_STRING_SYMBOL
    )
    baseline_common = {
        "left_boundary": start
        + _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_SETPOS_LABEL_OFFSET,
        "load": start + 0x641,
        "message_id": start + 0x646,
        "vptr_cache": start + 0x64B,
        "message": start
        + _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_LABEL_MESSAGE_OFFSET,
        "receiver_reload": start + 0x652,
        "message_argument": start + 0x658,
    }
    baseline_common_rows = {
        baseline_common["left_boundary"]: (
            ("ff", "50", "0c"),
            r"call\s+.*\[eax\+(?:12|0xc)\]",
        ),
        baseline_common["load"]: (
            ("a1", "28", "08", "00", "00"),
            rf"mov\s+eax\s*,.*{aggregate}\+(?:2088|0x828)",
        ),
        baseline_common["message_id"]: (
            ("b9", "06", "09", "00", "00"),
            r"mov\s+ecx\s*,\s*(?:2310|0x906)",
        ),
        baseline_common["vptr_cache"]: (
            ("8b", "30"),
            r"mov\s+esi\s*,.*\[eax\]",
        ),
        baseline_common["message"]: (
            ("e8", "00", "00", "00", "00"),
            rf"call\s+{get_message}(?:\s*;.*)?",
        ),
        baseline_common["receiver_reload"]: (
            ("8b", "0d", "28", "08", "00", "00"),
            rf"mov\s+ecx\s*,.*{aggregate}\+(?:2088|0x828)",
        ),
        baseline_common["message_argument"]: (("50",), r"push\s+eax"),
    }

    baseline_current = {
        "literal_argument": start + 0x659,
        "receiver_argument": start + 0x65E,
        "call": start
        + _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_SETTEXTFMT_OFFSET,
        "post_call": start + 0x662,
        "cleanup": start + 0x668,
        "eax_overwrite": start + 0x66B,
        "right_boundary": start
        + _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_NEXT_SETPOS_OFFSET,
    }
    baseline_repaired = {
        "receiver_argument": start + 0x659,
        "call": start
        + _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_REPAIRED_SETTEXTFMT_OFFSET,
        "post_call": start + 0x65D,
        "cleanup": start + 0x663,
        "eax_overwrite": start + 0x666,
        "right_boundary": start
        + _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_REPAIRED_NEXT_SETPOS_OFFSET,
    }
    current_rows = {
        baseline_current["literal_argument"]: (
            ("68", "00", "00", "00", "00"),
            r"push\s+(?:offset\s+flat:)?\?\?_C@_02[A-Za-z0-9_?$@]+",
        ),
        baseline_current["receiver_argument"]: (("51",), r"push\s+ecx"),
        baseline_current["call"]: (
            ("ff", "56", "74"),
            r"call\s+.*\[esi\+(?:116|0x74)\]",
        ),
        baseline_current["post_call"]: (
            ("8b", "15", "04", "00", "00", "00"),
            r"mov\s+edx\s*,.*",
        ),
        baseline_current["cleanup"]: (
            ("83", "c4", "0c"),
            r"add\s+esp\s*,\s*(?:12|0xc)",
        ),
        baseline_current["eax_overwrite"]: (
            ("a1", "00", "00", "00", "00"),
            r"mov\s+eax\s*,.*",
        ),
        baseline_current["right_boundary"]: (
            ("ff", "52", "0c"),
            r"call\s+.*\[edx\+(?:12|0xc)\]",
        ),
    }
    repaired_rows = {
        baseline_repaired["receiver_argument"]: (("51",), r"push\s+ecx"),
        baseline_repaired["call"]: (
            ("ff", "56", "74"),
            r"call\s+.*\[esi\+(?:116|0x74)\]",
        ),
        baseline_repaired["right_boundary"]: (
            ("ff", "52", "0c"),
            r"call\s+.*\[edx\+(?:12|0xc)\]",
        ),
    }

    located_shapes: list[
        tuple[
            bool,
            int,
            tuple[int | None, ...],
            dict[int, Instruction],
            dict[int, int],
        ]
    ] = []
    for is_current, rows in (
        (True, {**baseline_common_rows, **current_rows}),
        (False, {**baseline_common_rows, **repaired_rows}),
    ):
        try:
            located_shapes.append(
                (
                    is_current,
                    *_cc_callable_identity._locate_shifted_candidate_instruction_unit(
                        candidate,
                        rows,
                        label=(
                            "HUD EnsureHudLoaded objective-label SetTextFmt "
                            "candidate bridge"
                        ),
                    ),
                )
            )
        except ValueError:
            continue
    if len(located_shapes) != 1:
        raise ValueError(
            "HUD EnsureHudLoaded objective-label SetTextFmt candidate bridge "
            "requires exactly one unique current-extra-literal or repaired-"
            "two-word structural/COFF source shape"
        )
    (
        current_shape,
        unit_shift,
        addresses,
        by_address,
        index_by_address,
    ) = located_shapes[0]
    common = {
        key: address + unit_shift
        for key, address in baseline_common.items()
    }
    baseline_shape = baseline_current if current_shape else baseline_repaired
    shape = {
        key: address + unit_shift
        for key, address in baseline_shape.items()
    }
    cleanup_bytes = 12 if current_shape else 8
    call_offset = shape["call"] - start
    repaired_cleanup_index: int | None = None
    repaired_eax_overwrite_index: int | None = None
    if not current_shape:
        call_index = index_by_address[shape["call"]]
        right_boundary_index = index_by_address[shape["right_boundary"]]
        message_index = index_by_address[common["message"]]
        receiver_reload_index = index_by_address[common["receiver_reload"]]
        message_argument_index = index_by_address[common["message_argument"]]
        receiver_argument_index = index_by_address[shape["receiver_argument"]]

        def repaired_stack_mutation(instruction: Instruction) -> bool:
            mnemonic = _cc_cfg._instruction_mnemonic(instruction)
            if mnemonic in {
                "push", "pop", "pusha", "pushad", "popa", "popad",
                "enter", "leave",
            }:
                return True
            operands = [
                item.strip().lower()
                for item in _cc_cfg._instruction_operand(instruction).split(",")
            ]
            return bool(
                operands
                and (
                    operands[0] == "esp"
                    or (mnemonic == "xchg" and "esp" in operands)
                )
            )

        argument_stack_rows = [
            index
            for index in range(message_index + 1, call_index)
            if repaired_stack_mutation(candidate.instructions[index])
        ]
        cleanup_rows = [
            index
            for index in range(call_index + 1, right_boundary_index)
            if _cc_catalog.STACK_CLEANUP_RE.fullmatch(
                candidate.instructions[index].raw_text.strip().lower()
            )
            is not None
        ]
        if (
            argument_stack_rows
            != [message_argument_index, receiver_argument_index]
            or _cc_cfg._instruction_operand(
                candidate.instructions[message_argument_index]
            ).strip().lower()
            != "eax"
            or _cc_cfg._instruction_operand(
                candidate.instructions[receiver_argument_index]
            ).strip().lower()
            != "ecx"
            or len(cleanup_rows) != 1
            or _cc_cfg._cleanup_after(candidate.instructions, call_index) != 8
        ):
            raise ValueError(
                "HUD EnsureHudLoaded objective-label SetTextFmt repaired "
                "candidate bridge rejects wrong/duplicate arguments, stack "
                "mutation, or delayed cleanup"
            )
        repaired_cleanup_index = cleanup_rows[0]
        cleanup_match = _cc_catalog.STACK_CLEANUP_RE.fullmatch(
            candidate.instructions[
                repaired_cleanup_index
            ].raw_text.strip().lower()
        )
        if (
            cleanup_match is None
            or _cc_cfg._parse_unsigned_assembly_integer(cleanup_match.group(1)) != 8
        ):
            raise ValueError(
                "HUD EnsureHudLoaded objective-label SetTextFmt repaired "
                "candidate bridge requires exact cleanup 8"
            )

        for index in range(call_index + 1, repaired_cleanup_index):
            instruction = candidate.instructions[index]
            address = addresses[index]
            encoded = bytes(int(value, 16) for value in instruction.bytes)
            operand = _cc_cfg._instruction_operand(instruction).lower()
            if (
                address is None
                or definition.data[address : address + len(encoded)] != encoded
                or re.search(r"\beax\b", operand)
                or _cc_cfg._instruction_may_clobber_register(instruction, "eax")
            ):
                raise ValueError(
                    "HUD EnsureHudLoaded objective-label SetTextFmt repaired "
                    "candidate bridge rejects non-independent delayed-cleanup "
                    "instruction or result use"
                )

        for index in range(repaired_cleanup_index + 1, right_boundary_index):
            instruction = candidate.instructions[index]
            address = addresses[index]
            encoded = bytes(int(value, 16) for value in instruction.bytes)
            operand = _cc_cfg._instruction_operand(instruction).lower()
            if (
                address is None
                or definition.data[address : address + len(encoded)] != encoded
            ):
                raise ValueError(
                    "HUD EnsureHudLoaded objective-label SetTextFmt repaired "
                    "candidate bridge rejects post-cleanup instruction-byte "
                    "drift"
                )
            if re.search(r"\beax\b", operand):
                destination = (
                    operand.split(",", 1)[0].strip()
                    if "," in operand
                    else ""
                )
                if (
                    destination == "eax"
                    and _cc_cfg._instruction_may_clobber_register(instruction, "eax")
                ):
                    repaired_eax_overwrite_index = index
                    break
                raise ValueError(
                    "HUD EnsureHudLoaded objective-label SetTextFmt repaired "
                    "candidate bridge rejects consumed call result"
                )
            if _cc_cfg._instruction_may_clobber_register(instruction, "eax"):
                repaired_eax_overwrite_index = index
                break
        if repaired_eax_overwrite_index is None:
            raise ValueError(
                "HUD EnsureHudLoaded objective-label SetTextFmt repaired "
                "candidate bridge requires an unused result before the next "
                "EAX definition"
            )

    relocation_specs = {
        0x642 + unit_shift: (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_LABEL_PANEL_DISPLACEMENT,
        ),
        0x64E + unit_shift: (
            IMAGE_REL_I386_REL32,
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_GET_MESSAGE_STRING_SYMBOL,
            0,
        ),
        0x654 + unit_shift: (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_LABEL_PANEL_DISPLACEMENT,
        ),
    }
    for offset, (relocation_type, symbol, addend) in relocation_specs.items():
        rows = [row for row in definition.relocations if row.offset == offset]
        if (
            len(rows) != 1
            or rows[0].type != relocation_type
            or rows[0].symbol_name != symbol
            or struct.unpack_from("<I", definition.data, offset)[0] != addend
            or {
                index
                for index in range(offset, offset + 4)
                if definition.relocation_mask[index]
            }
            != set(range(offset, offset + 4))
        ):
            raise ValueError(
                "HUD EnsureHudLoaded objective-label SetTextFmt candidate "
                "bridge requires exact aggregate/message DIR32/REL32 "
                f"target, addend, and mask at +{hex(offset)}"
            )
    literal_rows = [
        row
        for row in definition.relocations
        if row.offset == 0x65A + unit_shift
    ]
    if current_shape:
        if (
            len(literal_rows) != 1
            or literal_rows[0].type != IMAGE_REL_I386_DIR32
            or not literal_rows[0].symbol_name.startswith("??_C@_02")
            or struct.unpack_from(
                "<I", definition.data, 0x65A + unit_shift
            )[0]
            != 0
            or {
                index
                for index in range(
                    0x65A + unit_shift,
                    0x65E + unit_shift,
                )
                if definition.relocation_mask[index]
            }
            != set(
                range(0x65A + unit_shift, 0x65E + unit_shift)
            )
        ):
            raise ValueError(
                "HUD EnsureHudLoaded objective-label SetTextFmt candidate "
                "bridge requires the exact current extra \"%s\" literal "
                "argument relocation"
            )
    elif literal_rows or any(
        definition.relocation_mask[index]
        for index in range(
            0x65A + unit_shift,
            0x65E + unit_shift,
        )
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective-label SetTextFmt repaired candidate "
            "bridge rejects a stale extra-literal relocation or mask"
        )
    if any(
        definition.relocation_mask[index]
        for index in range(call_offset, call_offset + 3)
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective-label SetTextFmt candidate bridge "
            "rejects a relocated or unconditional call target"
        )

    fixed_addresses = [
        *common.values(),
        shape["receiver_argument"],
        shape["call"],
    ]
    if current_shape:
        fixed_addresses.extend(
            (
                shape["post_call"],
                shape["cleanup"],
                shape["eax_overwrite"],
            )
        )
    fixed_indices = [
        index_by_address[address] for address in fixed_addresses
    ]
    if not current_shape:
        assert repaired_cleanup_index is not None
        assert repaired_eax_overwrite_index is not None
        fixed_indices.extend(
            (repaired_cleanup_index, repaired_eax_overwrite_index)
        )
    fixed_indices.append(index_by_address[shape["right_boundary"]])
    bounded_calls = [
        address
        for address, instruction in zip(addresses, candidate.instructions)
        if (
            address is not None
            and common["left_boundary"]
            <= address
            <= shape["right_boundary"]
            and _cc_cfg._instruction_mnemonic(instruction) in {"call", "jmp"}
        )
    ]
    ordinal_by_index = {
        index: ordinal
        for ordinal, index in enumerate(
            _cc_callable_identity._candidate_static_invocation_indices(
                candidate,
                caller_start=normalized_start,
                caller_end_exclusive=caller_end_exclusive,
            )
        )
    }
    if (
        fixed_indices != sorted(fixed_indices)
        or len(set(fixed_indices)) != len(fixed_indices)
        or bounded_calls
        != [
            common["left_boundary"],
            common["message"],
            shape["call"],
            shape["right_boundary"],
        ]
        or (
            current_shape
            and _cc_cfg._cleanup_after(
                candidate.instructions,
                index_by_address[shape["call"]],
            )
            != cleanup_bytes
        )
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective-label SetTextFmt candidate bridge "
            "rejects missing, extra, reordered, wrong-argument/cleanup, "
            "result-using, or absorbed-boundary calls"
        )

    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    absolute_loads = {
        normalize_address(hex(0x641 + unit_shift)): ReviewedAbsoluteStorageLoadBridge(
            register="eax",
            aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            displacement=_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_LABEL_PANEL_DISPLACEMENT,
            access_width=4,
            storage_identity=aggregate_identity,
        ),
        normalize_address(hex(0x652 + unit_shift)): ReviewedAbsoluteStorageLoadBridge(
            register="ecx",
            aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            displacement=_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_LABEL_PANEL_DISPLACEMENT,
            access_width=4,
            storage_identity=aggregate_identity,
        ),
    }
    vptr = {
        normalize_address(hex(call_offset)): ReviewedVptrStorageBridge(
            register="esi",
            provenance=(
                "load(exact-load(eax,"
                f"load({aggregate_identity}+0x"
                f"{_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_LABEL_PANEL_DISPLACEMENT:x})))"
            ),
            storage_identity=(
                _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_LABEL_PANEL_STORAGE_IDENTITY
            ),
            slot_displacement=_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_SETTEXTFMT_SLOT,
            identity_kind="virtual-slot",
        )
    }
    return absolute_loads, vptr
