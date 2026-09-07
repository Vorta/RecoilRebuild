"""Recoil call-contract recoil hud frame evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        IdentityIndexes,
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedExactIndirectStorageBridge,
        ReviewedMemberVptrStorageBridge,
        ReviewedStaticStorageReferenceBridge,
    )

import re
import struct
from pathlib import Path
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import IMAGE_REL_I386_DIR32, IMAGE_REL_I386_REL32
from _recoil.lib.progress import ProgressDocument, address_value, normalize_address


def _hud_ui_mgr_update_frame_current_layout_candidate_bridges(
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
    """Bridge only UpdateFrame's exact current-layout pre-update call."""
    from _recoil.call_contract.records import (
        ReviewedMemberVptrStorageBridge,
        ReviewedStaticStorageReferenceBridge,
        StorageContainer,
    )
    caller_start = normalize_address(caller_start)
    if caller_start != _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CALLER_START:
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
        == _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CALLER_SYMBOL.casefold()
    ]
    if (
        caller_identity != _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CALLER_END_EXCLUSIVE
        or caller_addresses != [_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CALLER_START]
        or caller_identity in indexes.provider_ids
        or caller_name_rows
        not in (
            [],
            [
                (
                    _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CALLER_SYMBOL,
                    _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CALLER_IDENTITY,
                )
            ],
        )
        or caller is None
        or caller.symbol != _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CALLER_SYMBOL
        or len(caller.data)
        < _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_LAYOUT_CALL_OFFSET + 3
        or len(caller.relocation_mask) != len(caller.data)
    ):
        raise ValueError(
            "HUD UpdateFrame current-layout bridge requires the exact "
            "reviewed authored caller identity, extent, symbol, and body"
        )

    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CALLER_START
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
            "HUD UpdateFrame current-layout bridge requires one exact "
            "current HUD source authority"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != r"\?UpdateFrame@HudUiMgr@@.*"
        or re.fullmatch(
            r"\?UpdateFrame@HudUiMgr@@.*",
            caller.symbol,
        )
        is None
        or getattr(contribution_row, "name", "")
        != "HudUiMgr::UpdateFrame"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD UpdateFrame current-layout bridge requires the exact "
            "authored hud.cpp contribution row"
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
        or caller_symbol.get("extent_state") != "known"
        or normalize_address(str(caller_symbol.get("address", "")))
        != _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CALLER_START
        or normalize_address(str(caller_symbol.get("end_exclusive", "")))
        != _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CALLER_END_EXCLUSIVE
        or caller_symbol.get("size")
        != address_value(_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CALLER_END_EXCLUSIVE)
        - address_value(_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CALLER_START)
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
        != _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
    ):
        raise ValueError(
            "HUD UpdateFrame current-layout bridge requires one exact "
            "unaliased reviewed caller and resolved source edge"
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
    if (
        not isinstance(aggregate_symbol, Mapping)
        or aggregate_symbol.get("binary") != "recoil"
        or aggregate_symbol.get("kind") != "data"
        or normalize_address(str(aggregate_symbol.get("address", "")))
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
        or [
            row
            for row in indexes.storage_containers
            if row.identity == aggregate_identity
        ]
        != [
            StorageContainer(
                address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS),
                address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
                + _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE,
                aggregate_identity,
            )
        ]
    ):
        raise ValueError(
            "HUD UpdateFrame current-layout bridge requires the exact HUD "
            "aggregate symbol, storage, target, and container authority"
        )

    expected_row = {
        "ordinal": _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_LAYOUT_CALL_ORDINAL,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": (
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_LAYOUT_STORAGE_IDENTITY
        ),
        "slot_displacement": (
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_LAYOUT_SLOT_DISPLACEMENT
        ),
        "cleanup_bytes": None,
    }
    if (
        not expected
        or expected[0] != expected_row
        or sum(row == expected_row for row in expected) != 1
    ):
        raise ValueError(
            "HUD UpdateFrame current-layout bridge requires the exact "
            "immutable ordinal-0 retail virtual-slot contract"
        )

    reference_offset = _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_LAYOUT_REFERENCE_OFFSET
    relocation_offset = _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_LAYOUT_RELOCATION_OFFSET
    vptr_offset = _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_LAYOUT_VPTR_OFFSET
    call_offset = _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_LAYOUT_CALL_OFFSET
    bounded_end = call_offset + 3
    exact_body = bytes.fromhex(
        "8b 0d 18 00 00 00 56 57 8b 01 ff 50 0c"
    )
    if (
        caller.data[reference_offset:bounded_end] != exact_body
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in caller.defined_external_data
    ):
        raise ValueError(
            "HUD UpdateFrame current-layout bridge requires the exact "
            "bounded field/prologue/vptr/call object body"
        )
    bounded_relocations = tuple(
        row
        for row in caller.relocations
        if reference_offset <= row.offset < bounded_end
    )
    relocation = (
        bounded_relocations[0]
        if len(bounded_relocations) == 1
        else None
    )
    if (
        relocation is None
        or relocation.offset != relocation_offset
        or relocation.type != IMAGE_REL_I386_DIR32
        or relocation.symbol_name
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        or struct.unpack_from("<I", caller.data, relocation.offset)[0]
        != _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_LAYOUT_DISPLACEMENT
    ):
        raise ValueError(
            "HUD UpdateFrame current-layout bridge rejects a missing, "
            "duplicate, extra, or malformed field DIR32 relocation"
        )
    expected_mask = set(range(relocation_offset, relocation_offset + 4))
    if {
        offset
        for offset in range(reference_offset, bounded_end)
        if caller.relocation_mask[offset]
    } != expected_mask:
        raise ValueError(
            "HUD UpdateFrame current-layout bridge requires the exact "
            "bounded field relocation mask"
        )

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    index_by_offset = {
        offset: index
        for index, offset in enumerate(offsets)
        if offset is not None
    }
    explicit_sequence_offsets = (0x06, 0x07, 0x08, 0x0A)
    if set(explicit_sequence_offsets) - set(index_by_offset):
        raise ValueError(
            "HUD UpdateFrame current-layout bridge requires exact candidate "
            "instruction offsets"
        )
    explicit_sequence_indices = tuple(
        index_by_offset[offset] for offset in explicit_sequence_offsets
    )
    first_index = explicit_sequence_indices[0] - 1
    sequence_indices = (first_index, *explicit_sequence_indices)
    if (
        first_index < 0
        or sequence_indices
        != tuple(range(sequence_indices[0], sequence_indices[0] + 5))
    ):
        raise ValueError(
            "HUD UpdateFrame current-layout bridge rejects a missing, "
            "duplicate, reordered, or noncontiguous candidate chain"
        )
    sequence = tuple(
        candidate.instructions[index] for index in sequence_indices
    )
    expected_expression = (
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
        f"+{_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_LAYOUT_DISPLACEMENT}"
    )
    expected_hex_expression = (
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
        f"+0x{_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_LAYOUT_DISPLACEMENT:x}"
    )
    if (
        tuple(instruction.bytes for instruction in sequence)
        != (
            ("8b", "0d", "18", "00", "00", "00"),
            ("56",),
            ("57",),
            ("8b", "01"),
            ("ff", "50", "0c"),
        )
        or tuple(
            _cc_cfg._instruction_mnemonic(instruction)
            for instruction in sequence
        )
        != ("mov", "push", "push", "mov", "call")
        or _cc_cfg._instruction_operand(sequence[0]).split(",", 1)[0]
        .strip()
        .lower()
        != "ecx"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(sequence[0]).split(",", 1)[-1]
        )
        not in {expected_expression, expected_hex_expression}
        or _cc_cfg._instruction_operand(sequence[1]).strip().lower() != "esi"
        or _cc_cfg._instruction_operand(sequence[2]).strip().lower() != "edi"
        or _cc_cfg._instruction_operand(sequence[3]).split(",", 1)[0]
        .strip()
        .lower()
        != "eax"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(sequence[3]).split(",", 1)[-1]
        )
        != "ecx"
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(sequence[4]))
        not in {"eax+12", "eax+0xc"}
    ):
        raise ValueError(
            "HUD UpdateFrame current-layout bridge rejects field/symbol/"
            "addend/register/prologue/receiver/slot/byte drift"
        )

    exact_sequence = tuple(instruction.bytes for instruction in sequence)
    matching_starts = [
        index
        for index in range(len(candidate.instructions) - 4)
        if tuple(
            instruction.bytes
            for instruction in candidate.instructions[index : index + 5]
        )
        == exact_sequence
    ]
    if matching_starts != [sequence_indices[0]]:
        raise ValueError(
            "HUD UpdateFrame current-layout bridge requires one unique exact "
            "field/prologue/EAX-vptr/slot-0xc chain"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    call_index = sequence_indices[-1]
    if not invocation_indices or invocation_indices[0] != call_index:
        raise ValueError(
            "HUD UpdateFrame current-layout bridge requires the exact "
            "candidate ordinal-0 invocation"
        )
    ecx_definitions = [
        index
        for index in range(sequence_indices[0], call_index)
        if _cc_cfg._instruction_may_clobber_register(
            candidate.instructions[index],
            "ecx",
        )
    ]
    eax_definitions = [
        index
        for index in range(sequence_indices[0], call_index)
        if _cc_cfg._instruction_may_clobber_register(
            candidate.instructions[index],
            "eax",
        )
    ]
    bounded_indices = frozenset(sequence_indices)
    if (
        ecx_definitions != [sequence_indices[0]]
        or eax_definitions != [sequence_indices[3]]
        or bool(candidate.local_control_flow_indices & bounded_indices)
        or any(
            target_index in bounded_indices
            for targets in candidate.local_control_flow_targets.values()
            for target_index in targets
        )
        or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
    ):
        raise ValueError(
            "HUD UpdateFrame current-layout bridge rejects reaching-"
            "definition, CFG, receiver, argument, or cleanup ambiguity"
        )
    next_invocation = (
        invocation_indices[1]
        if len(invocation_indices) > 1
        else len(candidate.instructions)
    )
    for instruction in candidate.instructions[
        call_index + 1 : next_invocation
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
                    "HUD UpdateFrame current-layout bridge rejects "
                    "unexpected call-result consumption"
                )
        if _cc_cfg._instruction_may_clobber_register(instruction, "eax"):
            break

    static_bridge = ReviewedStaticStorageReferenceBridge(
        aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
        displacement=_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_LAYOUT_DISPLACEMENT,
        access_width=4,
        storage_identity=aggregate_identity,
    )
    member_bridge = ReviewedMemberVptrStorageBridge(
        register="eax",
        source_register="ecx",
        source_provenance=aggregate_identity,
        receiver_register="ecx",
        receiver_provenance=aggregate_identity,
        storage_identity=_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_LAYOUT_STORAGE_IDENTITY,
        slot_displacement=_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_LAYOUT_SLOT_DISPLACEMENT,
        call_address=normalize_address(hex(call_offset)),
    )
    return (
        {expected_expression: static_bridge},
        {normalize_address(hex(vptr_offset)): member_bridge},
    )


def _hud_ui_mgr_update_frame_objective_timer_candidate_bridges(
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
    """Bridge UpdateFrame's exact objective-panel and timer call cluster."""
    from _recoil.call_contract.records import (
        ReviewedMemberVptrStorageBridge,
        ReviewedStaticStorageReferenceBridge,
    )
    caller_start = normalize_address(caller_start)
    if caller_start != _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CALLER_START:
        return {}, {}

    static_bridges, member_bridges = (
        _hud_ui_mgr_update_frame_current_layout_candidate_bridges(
            expected,
            candidate,
            document=document,
            caller_identity=caller_identity,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            indexes=indexes,
        )
    )
    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    if (
        static_bridges
        != {
            (
                f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
                f"+{_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_LAYOUT_DISPLACEMENT}"
            ): ReviewedStaticStorageReferenceBridge(
                aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                displacement=_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_LAYOUT_DISPLACEMENT,
                access_width=4,
                storage_identity=aggregate_identity,
            )
        }
        or member_bridges
        != {
            normalize_address(
                hex(_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_LAYOUT_VPTR_OFFSET)
            ): ReviewedMemberVptrStorageBridge(
                register="eax",
                source_register="ecx",
                source_provenance=aggregate_identity,
                receiver_register="ecx",
                receiver_provenance=aggregate_identity,
                storage_identity=(
                    _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_LAYOUT_STORAGE_IDENTITY
                ),
                slot_displacement=(
                    _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_LAYOUT_SLOT_DISPLACEMENT
                ),
                call_address=normalize_address(
                    hex(_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_LAYOUT_CALL_OFFSET)
                ),
            )
        }
    ):
        raise ValueError(
            "HUD UpdateFrame objective/timer bridge requires the exact "
            "current-layout predecessor authority"
        )

    direct_row = {
        "ordinal": 1,
        "form": "call",
        "dispatch": "direct",
        "identity_kind": "direct",
        "target_identity": _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_START_HIDE_IDENTITY,
        "storage_identity": "",
        "slot_displacement": None,
        "cleanup_bytes": None,
    }
    objective_rows = [
        {
            "ordinal": ordinal,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": storage_identity,
            "slot_displacement": slot_displacement,
            "cleanup_bytes": None,
        }
        for (
            _,
            _,
            _,
            _,
            ordinal,
            _,
            storage_identity,
            slot_displacement,
        ) in _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_OBJECTIVE_PANEL_CALLS
    ]
    timer_row = {
        "ordinal": _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIMER_CALL_ORDINAL,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIMER_STORAGE_IDENTITY,
        "slot_displacement": (
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIMER_SLOT_DISPLACEMENT
        ),
        "cleanup_bytes": None,
    }
    reviewed_rows = [direct_row, *objective_rows, timer_row]
    if (
        len(expected) < 5
        or list(expected[1:5]) != reviewed_rows
        or any(
            sum(row == reviewed_row for row in expected) != 1
            for reviewed_row in reviewed_rows
        )
    ):
        raise ValueError(
            "HUD UpdateFrame objective/timer bridge requires the exact "
            "immutable ordinal-1-through-4 retail contract"
        )

    symbols = document.collection("symbols")
    storage_rows = document.collection("storage_contributions")
    targets = document.collection("verification_targets")
    direct_symbol_id = _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_START_HIDE_IDENTITY.removeprefix(
        "symbol:"
    )
    direct_symbol = symbols.get(direct_symbol_id)
    if (
        indexes.by_address.get(_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_START_HIDE_ADDRESS)
        != _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_START_HIDE_IDENTITY
        or indexes.by_candidate_name.get(
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_START_HIDE_SYMBOL
        )
        not in {
            None,
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_START_HIDE_IDENTITY,
        }
        or _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_START_HIDE_IDENTITY in indexes.provider_ids
        or not isinstance(direct_symbol, Mapping)
        or _cc_identity._symbol_identity(direct_symbol_id, direct_symbol)
        != _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_START_HIDE_IDENTITY
        or direct_symbol.get("binary") != "recoil"
        or direct_symbol.get("kind") != "function"
        or direct_symbol.get("pipeline_class") != "authored"
        or direct_symbol.get("extent_state") != "known"
        or normalize_address(str(direct_symbol.get("address", "")))
        != _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_START_HIDE_ADDRESS
        or direct_symbol.get("navigation_name")
        != "HudUiMgrObjective::StartHide"
    ):
        raise ValueError(
            "HUD UpdateFrame objective/timer bridge requires the exact "
            "reviewed StartHide direct-call identity"
        )

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
    timer_identity = f"storage:{_cc_catalog.HUD_UI_MGR_DISABLE_TIMER_PANEL_SYMBOL_ID}"
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
        != timer_identity
    ):
        raise ValueError(
            "HUD UpdateFrame objective/timer bridge requires the exact "
            "typed timer-panel data/storage/verification-target authority"
        )

    time_symbol = symbols.get(_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_SYMBOL_ID)
    time_storage = storage_rows.get(_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_STORAGE_ID)
    time_target = targets.get(_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_TARGET_ID)
    time_registration = (
        time_target.get("registration")
        if isinstance(time_target, Mapping)
        else None
    )
    time_trace = (
        time_symbol.get("source_traceability")
        if isinstance(time_symbol, Mapping)
        else None
    )
    time_identity = f"storage:{_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_SYMBOL_ID}"
    if (
        not isinstance(time_symbol, Mapping)
        or _cc_identity._symbol_identity(
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_SYMBOL_ID,
            time_symbol,
        )
        != f"symbol:{_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_SYMBOL_ID}"
        or time_symbol.get("binary") != "recoil"
        or time_symbol.get("kind") != "data"
        or time_symbol.get("disposition") != "authored"
        or time_symbol.get("navigation_name")
        != _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_NAME
        or time_symbol.get("extent_state") != "unknown"
        or normalize_address(str(time_symbol.get("address", "")))
        != _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_ADDRESS
        or time_symbol.get("output_section_id") != "recoil:section:.data"
        or time_symbol.get("storage_contribution_ids")
        != [_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_STORAGE_ID]
        or time_symbol.get("verification_target_ids")
        != [_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_TARGET_ID]
        or not isinstance(time_trace, Mapping)
        or time_trace.get("state") != "resolved"
        or time_trace.get("reason_code") not in {None, ""}
        or time_trace.get("source_edges")
        != [
            {
                "anchor_id": (
                    "recoil:anchor:gamezrecoil-time-time-"
                    "g-time-unscaleddeltatimesec"
                ),
                "emission_context": {
                    "translation_unit": "src/GameZRecoil/zTime/Time.cpp"
                },
                "evidence_ids": [],
                "relation": "defines",
            }
        ]
        or not isinstance(time_storage, Mapping)
        or time_storage.get("binary") != "recoil"
        or time_storage.get("kind") != "data-symbol"
        or time_storage.get("output_section_id") != "recoil:section:.data"
        or time_storage.get("overlap") != "none"
        or time_storage.get("parent_contribution_id") is not None
        or time_storage.get("owner_ids")
        != ["recoil:owner:engine.time_runtime_globals"]
        or time_storage.get("symbol_ids")
        != [_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_SYMBOL_ID]
        or time_storage.get("reference")
        != {
            "address": _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_ADDRESS,
            "evidence_ids": [],
            "extent_state": "unknown",
        }
        or not isinstance(time_target, Mapping)
        or time_target.get("binary") != "recoil"
        or time_target.get("kind") != "vc5"
        or time_target.get("name") != "time_runtime_bss_globals"
        or time_target.get("registered_addresses")
        != ["0x56b424", "0x56b428", "0x56b42c", "0x56b430"]
        or not isinstance(time_registration, Mapping)
        or time_registration.get("manifest_path")
        != "tools/vc5_verify_targets/time_runtime_bss_globals.json"
        or time_registration.get("source_from")
        != "src/GameZRecoil/zSys/zsys.cpp"
        or time_registration.get("data_addresses")
        != ["0x56b424", "0x56b428", "0x56b42c", "0x56b430"]
        or indexes.storage_by_address.get(
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_ADDRESS
        )
        != time_identity
        or indexes.storage_by_name.get(
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_NAME
        )
        != time_identity
    ):
        raise ValueError(
            "HUD UpdateFrame objective/timer bridge requires the exact "
            "typed unscaled-delta data/storage/source/target authority"
        )

    caller = candidate.caller_definition
    bounded_start = (
        _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_OBJECTIVE_TIMER_BOUNDED_START
    )
    bounded_end = (
        _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_OBJECTIVE_TIMER_BOUNDED_END_EXCLUSIVE
    )
    exact_body = bytes.fromhex(
        "a1 04 00 00 00 33 ff 3b c7 74 0f "
        "39 3d 90 06 00 00 74 36 "
        "e8 00 00 00 00 eb 2f "
        "39 3d b4 0a 00 00 74 16 "
        "8b 0d 24 08 00 00 8b 11 ff 52 04 "
        "8b 0d 74 09 00 00 8b 01 ff 50 04 "
        "8b 0d 84 47 00 00 a1 00 00 00 00 "
        "50 8b 11 ff 52 24"
    )
    if (
        caller is None
        or len(caller.data) < bounded_end
        or len(caller.relocation_mask) != len(caller.data)
        or caller.data[bounded_start:bounded_end] != exact_body
        or caller.defined_external_functions.count(
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_START_HIDE_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_START_HIDE_SYMBOL
        in caller.undefined_external_functions
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_OBJECT_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in caller.defined_external_data
        or _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_OBJECT_SYMBOL
        in caller.defined_external_data
    ):
        raise ValueError(
            "HUD UpdateFrame objective/timer bridge requires the exact "
            "bounded branch/objective/timer object body and extern census"
        )

    expected_relocations = (
        (
            0x0E,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x04,
        ),
        (
            0x1A,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x690,
        ),
        (
            0x21,
            IMAGE_REL_I386_REL32,
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_START_HIDE_SYMBOL,
            0,
        ),
        (
            0x29,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0xAB4,
        ),
        (
            0x31,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x824,
        ),
        (
            0x3C,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x974,
        ),
        (
            0x47,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0x4784,
        ),
        (
            0x4C,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_OBJECT_SYMBOL,
            0,
        ),
    )
    bounded_relocations = tuple(
        row
        for row in caller.relocations
        if bounded_start <= row.offset < bounded_end
    )
    actual_relocations = tuple(
        (
            row.offset,
            row.type,
            row.symbol_name,
            (
                struct.unpack_from("<I", caller.data, row.offset)[0]
                if 0 <= row.offset <= len(caller.data) - 4
                else None
            ),
        )
        for row in bounded_relocations
    )
    if actual_relocations != expected_relocations:
        raise ValueError(
            "HUD UpdateFrame objective/timer bridge rejects a missing, "
            "duplicate, extra, reordered, or malformed bounded relocation"
        )
    expected_mask = {
        offset
        for relocation_offset, _, _, _ in expected_relocations
        for offset in range(relocation_offset, relocation_offset + 4)
    }
    if {
        offset
        for offset in range(bounded_start, bounded_end)
        if caller.relocation_mask[offset]
    } != expected_mask:
        raise ValueError(
            "HUD UpdateFrame objective/timer bridge requires the exact "
            "bounded DIR32/REL32 relocation mask"
        )

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    index_by_offset = {
        offset: index
        for index, offset in enumerate(offsets)
        if offset is not None
    }
    sequence_offsets = (
        0x0D,
        0x12,
        0x14,
        0x16,
        0x18,
        0x1E,
        0x20,
        0x25,
        0x27,
        0x2D,
        0x2F,
        0x35,
        0x37,
        0x3A,
        0x40,
        0x42,
        0x45,
        0x4B,
        0x50,
        0x51,
        0x53,
    )
    if (
        set(sequence_offsets) - set(index_by_offset)
        or {
            offset
            for offset in offsets
            if offset is not None and bounded_start <= offset < bounded_end
        }
        != set(sequence_offsets)
    ):
        raise ValueError(
            "HUD UpdateFrame objective/timer bridge requires the exact "
            "bounded candidate instruction offsets"
        )
    sequence_indices = tuple(
        index_by_offset[offset] for offset in sequence_offsets
    )
    if sequence_indices != tuple(
        range(sequence_indices[0], sequence_indices[0] + len(sequence_indices))
    ):
        raise ValueError(
            "HUD UpdateFrame objective/timer bridge rejects a missing, "
            "duplicate, reordered, or noncontiguous candidate sequence"
        )
    sequence = tuple(
        candidate.instructions[index] for index in sequence_indices
    )
    expected_instruction_bytes = (
        ("a1", "04", "00", "00", "00"),
        ("33", "ff"),
        ("3b", "c7"),
        ("74", "0f"),
        ("39", "3d", "90", "06", "00", "00"),
        ("74", "36"),
        ("e8", "00", "00", "00", "00"),
        ("eb", "2f"),
        ("39", "3d", "b4", "0a", "00", "00"),
        ("74", "16"),
        ("8b", "0d", "24", "08", "00", "00"),
        ("8b", "11"),
        ("ff", "52", "04"),
        ("8b", "0d", "74", "09", "00", "00"),
        ("8b", "01"),
        ("ff", "50", "04"),
        ("8b", "0d", "84", "47", "00", "00"),
        ("a1", "00", "00", "00", "00"),
        ("50",),
        ("8b", "11"),
        ("ff", "52", "24"),
    )
    expected_mnemonics = (
        "mov",
        "xor",
        "cmp",
        "je",
        "cmp",
        "je",
        "call",
        "jmp",
        "cmp",
        "je",
        "mov",
        "mov",
        "call",
        "mov",
        "mov",
        "call",
        "mov",
        "mov",
        "push",
        "mov",
        "call",
    )
    if (
        tuple(instruction.bytes for instruction in sequence)
        != expected_instruction_bytes
        or tuple(
            _cc_cfg._instruction_mnemonic(instruction) for instruction in sequence
        )
        != expected_mnemonics
    ):
        raise ValueError(
            "HUD UpdateFrame objective/timer bridge rejects candidate "
            "branch/order/opcode/byte drift"
        )

    aggregate_expressions = {
        displacement: {
            (
                f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
                f"+{displacement}"
            ),
            (
                f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
                f"+0x{displacement:x}"
            ),
        }
        for displacement in (0x04, 0x690, 0xAB4, 0x824, 0x974, 0x4784)
    }

    def exact_operand(offset: int) -> str:
        return _cc_cfg._instruction_operand(
            candidate.instructions[index_by_offset[offset]]
        )

    def exact_memory_source(offset: int) -> tuple[str, str]:
        operand = exact_operand(offset)
        destination, source = operand.split(",", 1)
        return destination.strip().lower(), _cc_targets._exact_memory_expression(source)

    for offset, register, displacement in (
        (0x0D, "eax", 0x04),
        (0x2F, "ecx", 0x824),
        (0x3A, "ecx", 0x974),
        (0x45, "ecx", 0x4784),
    ):
        destination, expression = exact_memory_source(offset)
        if (
            destination != register
            or expression not in aggregate_expressions[displacement]
        ):
            raise ValueError(
                "HUD UpdateFrame objective/timer bridge rejects aggregate "
                "field/register/addend semantic drift"
            )
    for offset, displacement in ((0x18, 0x690), (0x27, 0xAB4)):
        operand = exact_operand(offset)
        destination, source = operand.split(",", 1)
        if (
            _cc_targets._exact_memory_expression(destination)
            not in aggregate_expressions[displacement]
            or source.strip().lower() != "edi"
        ):
            raise ValueError(
                "HUD UpdateFrame objective/timer bridge rejects condition "
                "field/register/addend semantic drift"
            )
    if (
        exact_operand(0x12).replace(" ", "").lower() != "edi,edi"
        or exact_operand(0x14).replace(" ", "").lower() != "eax,edi"
        or exact_operand(0x20).strip()
        != _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_START_HIDE_SYMBOL
        or exact_memory_source(0x35) != ("edx", "ecx")
        or _cc_targets._exact_memory_expression(exact_operand(0x37))
        not in {"edx+4", "edx+0x4"}
        or exact_memory_source(0x40) != ("eax", "ecx")
        or _cc_targets._exact_memory_expression(exact_operand(0x42))
        not in {"eax+4", "eax+0x4"}
        or exact_memory_source(0x4B)
        != ("eax", _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_OBJECT_SYMBOL)
        or exact_operand(0x50).strip().lower() != "eax"
        or exact_memory_source(0x51) != ("edx", "ecx")
        or _cc_targets._exact_memory_expression(exact_operand(0x53))
        not in {"edx+36", "edx+0x24"}
    ):
        raise ValueError(
            "HUD UpdateFrame objective/timer bridge rejects direct target, "
            "receiver, argument, vptr, or slot semantic drift"
        )

    exact_sequence = tuple(instruction.bytes for instruction in sequence)
    matching_starts = [
        index
        for index in range(
            len(candidate.instructions) - len(sequence) + 1
        )
        if tuple(
            instruction.bytes
            for instruction in candidate.instructions[
                index : index + len(sequence)
            ]
        )
        == exact_sequence
    ]
    if matching_starts != [sequence_indices[0]]:
        raise ValueError(
            "HUD UpdateFrame objective/timer bridge requires one unique "
            "exact bounded candidate sequence"
        )

    bounded_indices = frozenset(sequence_indices)
    if candidate.local_control_flow_indices & bounded_indices:
        raise ValueError(
            "HUD UpdateFrame objective/timer bridge rejects alternate "
            "bounded branch classification"
        )
    if any(
        any(target_index in bounded_indices for target_index in targets_)
        for source_index, targets_ in candidate.local_control_flow_targets.items()
    ):
        raise ValueError(
            "HUD UpdateFrame objective/timer bridge rejects an alternate "
            "entry into the reviewed bounded CFG"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    expected_call_indices = tuple(
        index_by_offset[offset]
        for offset in (0x0A, 0x20, 0x37, 0x42, 0x53)
    )
    if invocation_indices[:5] != expected_call_indices:
        raise ValueError(
            "HUD UpdateFrame objective/timer bridge requires the exact "
            "candidate ordinal-0-through-4 invocation order"
        )

    for (
        displacement,
        load_offset,
        vptr_offset,
        call_offset,
        _,
        vptr_register,
        storage_identity,
        slot_displacement,
    ) in _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_OBJECTIVE_PANEL_CALLS:
        load_index = index_by_offset[load_offset]
        vptr_index = index_by_offset[vptr_offset]
        call_index = index_by_offset[call_offset]
        ecx_definitions = [
            index
            for index in range(load_index, call_index)
            if _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index],
                "ecx",
            )
        ]
        vptr_definitions = [
            index
            for index in range(load_index, call_index)
            if _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index],
                vptr_register,
            )
        ]
        if (
            ecx_definitions != [load_index]
            or vptr_definitions != [vptr_index]
            or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
        ):
            raise ValueError(
                "HUD UpdateFrame objective/timer bridge rejects panel "
                "reaching-definition, receiver, argument, or cleanup drift"
            )
        expression = (
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}+{displacement}"
        )
        bridge = ReviewedStaticStorageReferenceBridge(
            aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            displacement=displacement,
            access_width=4,
            storage_identity=aggregate_identity,
        )
        if expression in static_bridges and static_bridges[expression] != bridge:
            raise ValueError(
                "HUD UpdateFrame objective/timer panel static bridge "
                "conflicts with its current-layout predecessor"
            )
        static_bridges[expression] = bridge
        member_bridge = ReviewedMemberVptrStorageBridge(
            register=vptr_register,
            source_register="ecx",
            source_provenance=aggregate_identity,
            receiver_register="ecx",
            receiver_provenance=aggregate_identity,
            storage_identity=storage_identity,
            slot_displacement=slot_displacement,
            call_address=normalize_address(hex(call_offset)),
        )
        vptr_address = normalize_address(hex(vptr_offset))
        if (
            vptr_address in member_bridges
            and member_bridges[vptr_address] != member_bridge
        ):
            raise ValueError(
                "HUD UpdateFrame objective/timer panel member-vptr bridge "
                "conflicts with its current-layout predecessor"
            )
        member_bridges[vptr_address] = member_bridge

    timer_load_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIMER_LOAD_OFFSET
    ]
    time_load_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_LOAD_OFFSET
    ]
    time_push_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_PUSH_OFFSET
    ]
    timer_vptr_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIMER_VPTR_OFFSET
    ]
    timer_call_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIMER_CALL_OFFSET
    ]
    if (
        [
            index
            for index in range(timer_load_index, timer_call_index)
            if _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index],
                "ecx",
            )
        ]
        != [timer_load_index]
        or [
            index
            for index in range(timer_load_index, timer_call_index)
            if _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index],
                "eax",
            )
        ]
        != [time_load_index]
        or [
            index
            for index in range(timer_load_index, timer_call_index)
            if _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index],
                "edx",
            )
        ]
        != [timer_vptr_index]
        or time_push_index != time_load_index + 1
        or _cc_cfg._cleanup_after(
            candidate.instructions,
            timer_call_index,
        )
        is not None
    ):
        raise ValueError(
            "HUD UpdateFrame objective/timer bridge rejects timer "
            "receiver/vptr/float-argument reaching-definition or cleanup drift"
        )

    for call_offset in (0x37, 0x42, 0x53):
        call_index = index_by_offset[call_offset]
        ordinal_index = expected_call_indices.index(call_index)
        next_invocation = (
            invocation_indices[ordinal_index + 1]
            if ordinal_index + 1 < len(invocation_indices)
            else len(candidate.instructions)
        )
        for instruction in candidate.instructions[
            call_index + 1 : next_invocation
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
                        "HUD UpdateFrame objective/timer bridge rejects "
                        "unexpected call-result consumption"
                    )
            if _cc_cfg._instruction_may_clobber_register(instruction, "eax"):
                break

    timer_expression = (
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
        f"+{_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIMER_FIELD_DISPLACEMENT}"
    )
    timer_static_bridge = ReviewedStaticStorageReferenceBridge(
        aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
        displacement=_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIMER_FIELD_DISPLACEMENT,
        access_width=4,
        storage_identity=timer_identity,
    )
    if (
        timer_expression in static_bridges
        and static_bridges[timer_expression] != timer_static_bridge
    ):
        raise ValueError(
            "HUD UpdateFrame objective/timer timer-field bridge conflicts "
            "with its predecessor composition"
        )
    static_bridges[timer_expression] = timer_static_bridge
    timer_member_bridge = ReviewedMemberVptrStorageBridge(
        register="edx",
        source_register="ecx",
        source_provenance=timer_identity,
        receiver_register="ecx",
        receiver_provenance=timer_identity,
        storage_identity=_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIMER_STORAGE_IDENTITY,
        slot_displacement=_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIMER_SLOT_DISPLACEMENT,
        call_address=normalize_address(
            hex(_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIMER_CALL_OFFSET)
        ),
    )
    timer_vptr_address = normalize_address(
        hex(_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIMER_VPTR_OFFSET)
    )
    if (
        timer_vptr_address in member_bridges
        and member_bridges[timer_vptr_address] != timer_member_bridge
    ):
        raise ValueError(
            "HUD UpdateFrame objective/timer timer member-vptr bridge "
            "conflicts with its predecessor composition"
        )
    member_bridges[timer_vptr_address] = timer_member_bridge
    return static_bridges, member_bridges


def _hud_ui_mgr_update_frame_stack_menu_candidate_bridges(
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
    dict[str, ReviewedMemberVptrStorageBridge],
]:
    """Bridge UpdateFrame's exact container/stack/string-menu call cluster."""
    from _recoil.call_contract.records import (
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedMemberVptrStorageBridge,
        ReviewedStaticStorageReferenceBridge,
    )
    caller_start = normalize_address(caller_start)
    if caller_start != _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CALLER_START:
        return {}, {}, {}

    static_bridges, member_bridges = (
        _hud_ui_mgr_update_frame_objective_timer_candidate_bridges(
            expected,
            candidate,
            document=document,
            caller_identity=caller_identity,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            indexes=indexes,
        )
    )
    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    expected_predecessor_static = {
        (
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
            f"+{displacement}"
        )
        for displacement in (
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_LAYOUT_DISPLACEMENT,
            *(
                spec[0]
                for spec in _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_OBJECTIVE_PANEL_CALLS
            ),
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIMER_FIELD_DISPLACEMENT,
        )
    }
    expected_predecessor_member = {
        normalize_address(hex(offset))
        for offset in (
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_LAYOUT_VPTR_OFFSET,
            *(
                spec[2]
                for spec in _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_OBJECTIVE_PANEL_CALLS
            ),
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIMER_VPTR_OFFSET,
        )
    }
    if (
        set(static_bridges) != expected_predecessor_static
        or set(member_bridges) != expected_predecessor_member
    ):
        raise ValueError(
            "HUD UpdateFrame stack/menu bridge requires the exact "
            "current-layout and objective/timer predecessor authority"
        )

    direct_row = {
        "ordinal": _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_CALL_ORDINAL,
        "form": "call",
        "dispatch": "direct",
        "identity_kind": "direct",
        "target_identity": (
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_UPDATE_IDENTITY
        ),
        "storage_identity": "",
        "slot_displacement": None,
        "cleanup_bytes": None,
    }
    stack_rows = [
        {
            "ordinal": call_ordinal,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": f"load(storage:{symbol_id})",
            "slot_displacement": 0,
            "cleanup_bytes": None,
        }
        for (
            _,
            symbol_id,
            _,
            _,
            _,
            _,
            _,
            _,
            _,
            _,
            _,
            _,
            _,
            _,
            call_ordinal,
        ) in _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STACK_CALLS
    ]
    string_menu_row = {
        "ordinal": _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_CALL_ORDINAL,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": (
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_STORAGE_IDENTITY
        ),
        "slot_displacement": 0,
        "cleanup_bytes": None,
    }
    reviewed_rows = [direct_row, *stack_rows, string_menu_row]
    if (
        len(expected)
        <= _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_CALL_ORDINAL
        or list(
            expected[
                _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_CALL_ORDINAL :
                _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_CALL_ORDINAL + 1
            ]
        )
        != reviewed_rows
        or any(
            sum(row == reviewed_row for row in expected) != 1
            for reviewed_row in reviewed_rows
        )
    ):
        raise ValueError(
            "HUD UpdateFrame stack/menu bridge requires the exact immutable "
            "ordinal-9-through-12 retail call contract"
        )

    symbols = document.collection("symbols")
    storage_rows = document.collection("storage_contributions")
    targets = document.collection("verification_targets")
    update_symbol_id = (
        _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_UPDATE_IDENTITY.removeprefix(
            "symbol:"
        )
    )
    update_symbol = symbols.get(update_symbol_id)
    update_trace = (
        update_symbol.get("source_traceability")
        if isinstance(update_symbol, Mapping)
        else None
    )
    update_edges = (
        update_trace.get("source_edges")
        if isinstance(update_trace, Mapping)
        else None
    )
    if (
        indexes.by_address.get(
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_UPDATE_ADDRESS
        )
        != _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_UPDATE_IDENTITY
        or indexes.by_candidate_name.get(
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_UPDATE_SYMBOL
        )
        not in {
            None,
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_UPDATE_IDENTITY,
        }
        or (
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_UPDATE_IDENTITY
            in indexes.provider_ids
        )
        or not isinstance(update_symbol, Mapping)
        or _cc_identity._symbol_identity(update_symbol_id, update_symbol)
        != _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_UPDATE_IDENTITY
        or update_symbol.get("binary") != "recoil"
        or update_symbol.get("kind") != "function"
        or update_symbol.get("pipeline_class") != "authored"
        or update_symbol.get("extent_state") != "known"
        or normalize_address(str(update_symbol.get("address", "")))
        != _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_UPDATE_ADDRESS
        or normalize_address(str(update_symbol.get("end_exclusive", "")))
        != "0x4bc930"
        or update_symbol.get("size") != 0x30
        or update_symbol.get("navigation_name")
        != "HudUiContainer::UpdateAll"
        or update_symbol.get("logical_identity_key") not in {None, ""}
        or update_symbol.get("icf_fold_status") not in {None, ""}
        or bool(update_symbol.get("logical_aliases"))
        or not isinstance(update_trace, Mapping)
        or update_trace.get("state") != "resolved"
        or update_trace.get("reason_code") not in {None, ""}
        or update_edges
        != [
            {
                "anchor_id": (
                    "recoil:anchor:gamezrecoil-zui-zui-"
                    "huduicontainer-updateall"
                ),
                "emission_context": {
                    "translation_unit": "src/GameZRecoil/zUI/zui.cpp"
                },
                "evidence_ids": [],
                "relation": "defines",
            }
        ]
    ):
        raise ValueError(
            "HUD UpdateFrame stack/menu bridge requires the exact reviewed "
            "authored HudUiContainer::UpdateAll identity and source edge"
        )

    stack_authorities: list[
        tuple[
            str,
            str,
            str,
            str,
            str,
            str,
            int,
            int,
            int,
            int,
            int,
            int,
        ]
    ] = []
    for (
        label,
        symbol_id,
        storage_id,
        address,
        navigation_name,
        object_symbol,
        target_id,
        owner_id,
        anchor_id,
        load_offset,
        time_load_offset,
        time_push_offset,
        vptr_offset,
        call_offset,
        call_ordinal,
    ) in _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STACK_CALLS:
        symbol = symbols.get(symbol_id)
        storage = storage_rows.get(storage_id)
        target = targets.get(target_id)
        registration = (
            target.get("registration")
            if isinstance(target, Mapping)
            else None
        )
        trace = (
            symbol.get("source_traceability")
            if isinstance(symbol, Mapping)
            else None
        )
        if (
            not isinstance(symbol, Mapping)
            or _cc_identity._symbol_identity(symbol_id, symbol)
            != f"symbol:{symbol_id}"
            or symbol.get("binary") != "recoil"
            or symbol.get("kind") != "data"
            or symbol.get("disposition") != "authored"
            or symbol.get("extent_state") != "unknown"
            or normalize_address(str(symbol.get("address", "")))
            != address
            or symbol.get("navigation_name") != navigation_name
            or symbol.get("output_section_id") != "recoil:section:.data"
            or symbol.get("storage_contribution_ids") != [storage_id]
            or symbol.get("verification_target_ids") != [target_id]
            or not isinstance(trace, Mapping)
            or trace.get("state") != "resolved"
            or trace.get("reason_code") not in {None, ""}
            or trace.get("source_edges")
            != [
                {
                    "anchor_id": anchor_id,
                    "emission_context": {
                        "translation_unit": (
                            "src/GameZRecoil/zUI/zui_widgets.cpp"
                        )
                    },
                    "evidence_ids": [],
                    "relation": "defines",
                }
            ]
            or not isinstance(storage, Mapping)
            or storage.get("binary") != "recoil"
            or storage.get("kind") != "data-symbol"
            or storage.get("output_section_id") != "recoil:section:.data"
            or storage.get("overlap") != "none"
            or storage.get("parent_contribution_id") is not None
            or storage.get("owner_ids") != [owner_id]
            or storage.get("symbol_ids") != [symbol_id]
            or storage.get("reference")
            != {
                "address": address,
                "evidence_ids": [],
                "extent_state": "unknown",
            }
            or not isinstance(target, Mapping)
            or target.get("binary") != "recoil"
            or target.get("kind") != "vc5"
            or target.get("name")
            != target_id.removeprefix("recoil:vc5-target:")
            or target.get("symbol_ids") != [symbol_id]
            or target.get("unresolved_addresses") != []
            or not isinstance(registration, Mapping)
            or registration.get("manifest_path")
            != (
                "tools/vc5_verify_targets/"
                f"{target_id.removeprefix('recoil:vc5-target:')}.json"
            )
            or registration.get("source_from")
            != "src/GameZRecoil/zUI/zui.cpp"
            or registration.get("data_addresses") != [address]
            or indexes.storage_by_address.get(address)
            != f"storage:{symbol_id}"
            or indexes.storage_by_name.get(navigation_name)
            != f"storage:{symbol_id}"
        ):
            raise ValueError(
                f"HUD UpdateFrame stack/menu bridge requires the exact typed "
                f"{label} stack symbol/storage/source/target authority"
            )
        stack_authorities.append(
            (
                label,
                symbol_id,
                object_symbol,
                f"storage:{symbol_id}",
                f"load(storage:{symbol_id})",
                f"exact-load(ecx,load(storage:{symbol_id}))",
                load_offset,
                time_load_offset,
                time_push_offset,
                vptr_offset,
                call_offset,
                call_ordinal,
            )
        )

    caller = candidate.caller_definition
    bounded_start = _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STACK_MENU_BOUNDED_START
    bounded_end = _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STACK_MENU_BOUNDED_END_EXCLUSIVE
    exact_body = bytes.fromhex(
        "8b 0d 00 00 00 00 51 b9 00 00 00 00 "
        "e8 00 00 00 00 "
        "8b 0d 00 00 00 00 a1 00 00 00 00 50 8b 11 ff 12 "
        "8b 0d 00 00 00 00 a1 00 00 00 00 50 8b 11 ff 12 "
        "8b 0d e0 0e 00 00 a1 00 00 00 00 50 8b 11 ff 12"
    )
    stack_symbols = tuple(spec[5] for spec in _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STACK_CALLS)
    if (
        caller is None
        or len(caller.data)
        != address_value(_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CALLER_END_EXCLUSIVE)
        - address_value(_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CALLER_START)
        or len(caller.relocation_mask) != len(caller.data)
        or caller.data[bounded_start:bounded_end] != exact_body
        or caller.undefined_external_functions.count(
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_UPDATE_SYMBOL
        )
        != 1
        or (
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_UPDATE_SYMBOL
            in caller.defined_external_functions
        )
        or any(
            caller.undefined_external_data.count(symbol) != 1
            or symbol in caller.defined_external_data
            for symbol in stack_symbols
        )
    ):
        raise ValueError(
            "HUD UpdateFrame stack/menu bridge requires the exact bounded "
            "container/stack/menu object body and external-symbol census"
        )

    expected_relocations = (
        (
            0x84,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_OBJECT_SYMBOL,
            0,
        ),
        (
            0x8A,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            0,
        ),
        (
            0x8F,
            IMAGE_REL_I386_REL32,
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_UPDATE_SYMBOL,
            0,
        ),
        (
            0x95,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STACK_CALLS[0][5],
            0,
        ),
        (
            0x9A,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_OBJECT_SYMBOL,
            0,
        ),
        (
            0xA5,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STACK_CALLS[1][5],
            0,
        ),
        (
            0xAA,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_OBJECT_SYMBOL,
            0,
        ),
        (
            0xB5,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_DISPLACEMENT,
        ),
        (
            0xBA,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_OBJECT_SYMBOL,
            0,
        ),
    )
    bounded_relocations = tuple(
        row
        for row in caller.relocations
        if bounded_start <= row.offset < bounded_end
    )
    actual_relocations = tuple(
        (
            row.offset,
            row.type,
            row.symbol_name,
            (
                struct.unpack_from("<I", caller.data, row.offset)[0]
                if 0 <= row.offset <= len(caller.data) - 4
                else None
            ),
        )
        for row in bounded_relocations
    )
    if actual_relocations != expected_relocations:
        raise ValueError(
            "HUD UpdateFrame stack/menu bridge rejects a missing, duplicate, "
            "extra, reordered, or malformed bounded DIR32/REL32 relocation"
        )
    expected_mask = {
        offset
        for relocation_offset, _, _, _ in expected_relocations
        for offset in range(relocation_offset, relocation_offset + 4)
    }
    if {
        offset
        for offset in range(bounded_start, bounded_end)
        if caller.relocation_mask[offset]
    } != expected_mask:
        raise ValueError(
            "HUD UpdateFrame stack/menu bridge requires the exact bounded "
            "relocation mask"
        )

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    index_by_offset = {
        offset: index
        for index, offset in enumerate(offsets)
        if offset is not None
    }
    sequence_offsets = (
        _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_TIME_LOAD_OFFSET,
        _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_TIME_PUSH_OFFSET,
        _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_RECEIVER_OFFSET,
        _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_CALL_OFFSET,
        *(
            offset
            for spec in _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STACK_CALLS
            for offset in spec[9:14]
        ),
        _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_LOAD_OFFSET,
        _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_TIME_LOAD_OFFSET,
        _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_TIME_PUSH_OFFSET,
        _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_VPTR_OFFSET,
        _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_CALL_OFFSET,
    )
    if (
        set(sequence_offsets) - set(index_by_offset)
        or {
            offset
            for offset in offsets
            if offset is not None and bounded_start <= offset < bounded_end
        }
        != set(sequence_offsets)
    ):
        raise ValueError(
            "HUD UpdateFrame stack/menu bridge requires the exact bounded "
            "candidate instruction offsets"
        )
    sequence_indices = tuple(
        index_by_offset[offset] for offset in sequence_offsets
    )
    if sequence_indices != tuple(
        range(sequence_indices[0], sequence_indices[0] + len(sequence_indices))
    ):
        raise ValueError(
            "HUD UpdateFrame stack/menu bridge rejects a missing, duplicate, "
            "reordered, or noncontiguous candidate sequence"
        )
    sequence = tuple(
        candidate.instructions[index] for index in sequence_indices
    )
    expected_instruction_bytes = (
        ("8b", "0d", "00", "00", "00", "00"),
        ("51",),
        ("b9", "00", "00", "00", "00"),
        ("e8", "00", "00", "00", "00"),
        ("8b", "0d", "00", "00", "00", "00"),
        ("a1", "00", "00", "00", "00"),
        ("50",),
        ("8b", "11"),
        ("ff", "12"),
        ("8b", "0d", "00", "00", "00", "00"),
        ("a1", "00", "00", "00", "00"),
        ("50",),
        ("8b", "11"),
        ("ff", "12"),
        ("8b", "0d", "e0", "0e", "00", "00"),
        ("a1", "00", "00", "00", "00"),
        ("50",),
        ("8b", "11"),
        ("ff", "12"),
    )
    expected_mnemonics = (
        "mov",
        "push",
        "mov",
        "call",
        "mov",
        "mov",
        "push",
        "mov",
        "call",
        "mov",
        "mov",
        "push",
        "mov",
        "call",
        "mov",
        "mov",
        "push",
        "mov",
        "call",
    )
    if (
        tuple(instruction.bytes for instruction in sequence)
        != expected_instruction_bytes
        or tuple(
            _cc_cfg._instruction_mnemonic(instruction) for instruction in sequence
        )
        != expected_mnemonics
    ):
        raise ValueError(
            "HUD UpdateFrame stack/menu bridge rejects candidate "
            "opcode/byte/order drift"
        )

    def exact_operand(offset: int) -> str:
        return _cc_cfg._instruction_operand(
            candidate.instructions[index_by_offset[offset]]
        )

    def exact_memory_source(offset: int) -> tuple[str, str]:
        destination, source = exact_operand(offset).split(",", 1)
        return destination.strip().lower(), _cc_targets._exact_memory_expression(source)

    if (
        exact_memory_source(
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_TIME_LOAD_OFFSET
        )
        != ("ecx", _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_OBJECT_SYMBOL)
        or exact_operand(
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_TIME_PUSH_OFFSET
        ).strip().lower()
        != "ecx"
        or re.fullmatch(
            r"ecx\s*,\s*OFFSET\s+FLAT:"
            + re.escape(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL),
            exact_operand(
                _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_RECEIVER_OFFSET
            ).strip(),
            flags=re.IGNORECASE,
        )
        is None
        or exact_operand(
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_CALL_OFFSET
        ).strip()
        != _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_UPDATE_SYMBOL
    ):
        raise ValueError(
            "HUD UpdateFrame stack/menu bridge rejects the direct base-call "
            "time argument, receiver, target, or register boundary"
        )

    for (
        _,
        _,
        object_symbol,
        _,
        _,
        _,
        load_offset,
        time_load_offset,
        time_push_offset,
        vptr_offset,
        call_offset,
        _,
    ) in stack_authorities:
        if (
            exact_memory_source(load_offset) != ("ecx", object_symbol)
            or exact_memory_source(time_load_offset)
            != ("eax", _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_OBJECT_SYMBOL)
            or exact_operand(time_push_offset).strip().lower() != "eax"
            or exact_memory_source(vptr_offset) != ("edx", "ecx")
            or _cc_targets._exact_memory_expression(exact_operand(call_offset)) != "edx"
        ):
            raise ValueError(
                "HUD UpdateFrame stack/menu bridge rejects stack receiver, "
                "time argument, EDX-vptr, or slot-zero semantic drift"
            )
    menu_expression = (
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
        f"+{_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_DISPLACEMENT}"
    )
    menu_hex_expression = (
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
        f"+0x{_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_DISPLACEMENT:x}"
    )
    if (
        exact_memory_source(
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_LOAD_OFFSET
        )
        != ("ecx", menu_expression)
        and exact_memory_source(
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_LOAD_OFFSET
        )
        != ("ecx", menu_hex_expression)
        or exact_memory_source(
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_TIME_LOAD_OFFSET
        )
        != ("eax", _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_OBJECT_SYMBOL)
        or exact_operand(
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_TIME_PUSH_OFFSET
        ).strip().lower()
        != "eax"
        or exact_memory_source(
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_VPTR_OFFSET
        )
        != ("edx", "ecx")
        or _cc_targets._exact_memory_expression(
            exact_operand(_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_CALL_OFFSET)
        )
        != "edx"
    ):
        raise ValueError(
            "HUD UpdateFrame stack/menu bridge rejects string-menu field, "
            "time argument, EDX-vptr, or slot-zero semantic drift"
        )

    bounded_indices = frozenset(sequence_indices)
    if (
        candidate.local_control_flow_indices & bounded_indices
        or any(
            target_index in bounded_indices
            for targets_ in candidate.local_control_flow_targets.values()
            for target_index in targets_
        )
    ):
        raise ValueError(
            "HUD UpdateFrame stack/menu bridge rejects alternate bounded CFG"
        )
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    expected_call_indices = tuple(
        index_by_offset[offset]
        for offset in (
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_CALL_OFFSET,
            *(spec[13] for spec in _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STACK_CALLS),
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_CALL_OFFSET,
        )
    )
    if (
        tuple(
            invocation_indices[
                _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_CALL_ORDINAL :
                _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_CALL_ORDINAL + 1
            ]
        )
        != expected_call_indices
    ):
        raise ValueError(
            "HUD UpdateFrame stack/menu bridge requires the exact candidate "
            "ordinal-9-through-12 call order"
        )

    chain_specs = (
        (
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_TIME_LOAD_OFFSET,
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_TIME_PUSH_OFFSET,
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_RECEIVER_OFFSET,
            None,
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_CALL_OFFSET,
            "ecx",
            None,
        ),
        *(
            (
                load_offset,
                time_push_offset,
                load_offset,
                vptr_offset,
                call_offset,
                "eax",
                time_load_offset,
            )
            for (
                _,
                _,
                _,
                _,
                _,
                _,
                load_offset,
                time_load_offset,
                time_push_offset,
                vptr_offset,
                call_offset,
                _,
            ) in stack_authorities
        ),
        (
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_LOAD_OFFSET,
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_TIME_PUSH_OFFSET,
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_LOAD_OFFSET,
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_VPTR_OFFSET,
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_CALL_OFFSET,
            "eax",
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_TIME_LOAD_OFFSET,
        ),
    )
    for (
        chain_start_offset,
        argument_push_offset,
        receiver_load_offset,
        vptr_offset,
        call_offset,
        argument_register,
        argument_load_offset,
    ) in chain_specs:
        chain_start_index = index_by_offset[chain_start_offset]
        call_index = index_by_offset[call_offset]
        ecx_expected = (
            [
                chain_start_index,
                index_by_offset[receiver_load_offset],
            ]
            if receiver_load_offset != chain_start_offset
            else [chain_start_index]
        )
        ecx_definitions = [
            index
            for index in range(chain_start_index, call_index)
            if _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index], "ecx"
            )
        ]
        argument_definitions = [
            index
            for index in range(chain_start_index, call_index)
            if _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index], argument_register
            )
        ]
        if (
            ecx_definitions != ecx_expected
            or (
                argument_load_offset is not None
                and argument_definitions
                != [index_by_offset[argument_load_offset]]
            )
            or (
                vptr_offset is not None
                and [
                    index
                    for index in range(chain_start_index, call_index)
                    if _cc_cfg._instruction_may_clobber_register(
                        candidate.instructions[index], "edx"
                    )
                ]
                != [index_by_offset[vptr_offset]]
            )
            or index_by_offset[argument_push_offset]
            != (
                index_by_offset[
                    (
                        argument_load_offset
                        if argument_load_offset is not None
                        else chain_start_offset
                    )
                ]
                + 1
            )
            or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
        ):
            raise ValueError(
                "HUD UpdateFrame stack/menu bridge rejects receiver/vptr/"
                "argument reaching definitions or cleanup drift"
            )

    for ordinal, call_index in enumerate(
        expected_call_indices,
        start=_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CONTAINER_CALL_ORDINAL,
    ):
        next_invocation = (
            invocation_indices[ordinal + 1]
            if ordinal + 1 < len(invocation_indices)
            else len(candidate.instructions)
        )
        for instruction in candidate.instructions[
            call_index + 1 : next_invocation
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
                        "HUD UpdateFrame stack/menu bridge rejects unexpected "
                        "call-result consumption"
                    )
            if _cc_cfg._instruction_may_clobber_register(instruction, "eax"):
                break

    absolute_bridges: dict[str, ReviewedAbsoluteStorageLoadBridge] = {}
    for (
        _,
        _,
        object_symbol,
        storage_identity,
        expected_storage,
        receiver_provenance,
        load_offset,
        _,
        _,
        vptr_offset,
        call_offset,
        _,
    ) in stack_authorities:
        absolute_bridge = ReviewedAbsoluteStorageLoadBridge(
            register="ecx",
            aggregate_symbol=object_symbol,
            displacement=0,
            access_width=4,
            storage_identity=storage_identity,
        )
        load_address = normalize_address(hex(load_offset))
        if load_address in absolute_bridges:
            raise ValueError(
                "HUD UpdateFrame stack/menu absolute-load bridge collision"
            )
        absolute_bridges[load_address] = absolute_bridge
        member_bridge = ReviewedMemberVptrStorageBridge(
            register="edx",
            source_register="ecx",
            source_provenance=receiver_provenance,
            receiver_register="ecx",
            receiver_provenance=receiver_provenance,
            storage_identity=expected_storage,
            slot_displacement=0,
            call_address=normalize_address(hex(call_offset)),
        )
        vptr_address = normalize_address(hex(vptr_offset))
        if (
            vptr_address in member_bridges
            and member_bridges[vptr_address] != member_bridge
        ):
            raise ValueError(
                "HUD UpdateFrame stack/menu member-vptr bridge collision"
            )
        member_bridges[vptr_address] = member_bridge

    menu_static = ReviewedStaticStorageReferenceBridge(
        aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
        displacement=_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_DISPLACEMENT,
        access_width=4,
        storage_identity=aggregate_identity,
    )
    if menu_expression in static_bridges and static_bridges[
        menu_expression
    ] != menu_static:
        raise ValueError(
            "HUD UpdateFrame stack/menu string-menu static bridge collision"
        )
    static_bridges[menu_expression] = menu_static
    menu_member = ReviewedMemberVptrStorageBridge(
        register="edx",
        source_register="ecx",
        source_provenance=aggregate_identity,
        receiver_register="ecx",
        receiver_provenance=aggregate_identity,
        storage_identity=_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_STORAGE_IDENTITY,
        slot_displacement=0,
        call_address=normalize_address(
            hex(_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_CALL_OFFSET)
        ),
    )
    menu_vptr_address = normalize_address(
        hex(_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_VPTR_OFFSET)
    )
    if (
        menu_vptr_address in member_bridges
        and member_bridges[menu_vptr_address] != menu_member
    ):
        raise ValueError(
            "HUD UpdateFrame stack/menu string-menu member bridge collision"
        )
    member_bridges[menu_vptr_address] = menu_member
    return static_bridges, absolute_bridges, member_bridges


def _hud_ui_mgr_update_frame_tail_candidate_bridges(
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
    dict[str, ReviewedMemberVptrStorageBridge],
    dict[str, ReviewedExactIndirectStorageBridge],
]:
    """Bridge UpdateFrame's exact floating/reticle/marker tail."""
    from _recoil.call_contract.records import (
        ReviewedExactIndirectStorageBridge,
        ReviewedMemberVptrStorageBridge,
    )
    caller_start = normalize_address(caller_start)
    if caller_start != _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CALLER_START:
        return {}, {}, {}, {}

    static_bridges, absolute_bridges, member_bridges = (
        _hud_ui_mgr_update_frame_stack_menu_candidate_bridges(
            expected,
            candidate,
            document=document,
            caller_identity=caller_identity,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            indexes=indexes,
        )
    )
    aggregate = _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    if (
        set(static_bridges)
        != {
            f"{aggregate}+24",
            f"{aggregate}+2084",
            f"{aggregate}+2420",
            f"{aggregate}+3808",
            f"{aggregate}+18308",
        }
        or set(absolute_bridges) != {"0x93", "0xa3"}
        or set(member_bridges)
        != {
            "0x8",
            "0x35",
            "0x40",
            "0x51",
            "0x9f",
            "0xaf",
            "0xbf",
        }
    ):
        raise ValueError(
            "HUD UpdateFrame tail bridge requires the exact stack/menu "
            "predecessor authority"
        )

    caller = candidate.caller_definition
    current_layout_start = 0x72
    current_layout_end = 0x82
    current_layout_body = bytes.fromhex(
        "8b 0d 18 00 00 00 a1 00 00 00 00 50 8b 11 ff 12"
    )
    current_layout_relocations = (
        (
            0x74,
            IMAGE_REL_I386_DIR32,
            aggregate,
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_LAYOUT_DISPLACEMENT,
        ),
        (
            0x79,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_OBJECT_SYMBOL,
            0,
        ),
    )
    if (
        caller is None
        or caller.data[current_layout_start:current_layout_end]
        != current_layout_body
        or tuple(
            (
                row.offset,
                row.type,
                row.symbol_name,
                struct.unpack_from("<I", caller.data, row.offset)[0],
            )
            for row in caller.relocations
            if current_layout_start <= row.offset < current_layout_end
        )
        != current_layout_relocations
        or {
            offset
            for offset in range(current_layout_start, current_layout_end)
            if caller.relocation_mask[offset]
        }
        != {
            offset
            for relocation_offset, _, _, _ in current_layout_relocations
            for offset in range(relocation_offset, relocation_offset + 4)
        }
    ):
        raise ValueError(
            "HUD UpdateFrame tail composition requires the exact repeated "
            "current-layout object bytes, DIR32 rows, addends, and mask"
        )

    reviewed_rows = [
        {
            "ordinal": 13,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": (
                "load(load(storage:recoil:data:0x4e5ed0+0x4788))"
            ),
            "slot_displacement": 4,
            "cleanup_bytes": None,
        },
        {
            "ordinal": 14,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": (
                "load(storage:recoil:data:0x4e5ed0+0x364)"
            ),
            "slot_displacement": 0x24,
            "cleanup_bytes": None,
        },
        {
            "ordinal": 15,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": (
                "load(load(storage:recoil:data:0x4e5ed0+0xf44)+0xbc)"
            ),
            "slot_displacement": 0x60,
            "cleanup_bytes": None,
        },
        {
            "ordinal": 16,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": (
                "load(load(storage:recoil:data:0x4e5ed0+0xf44))"
            ),
            "slot_displacement": 0x60,
            "cleanup_bytes": None,
        },
    ]
    if (
        len(expected) <= 16
        or list(expected[13:17]) != reviewed_rows
        or any(
            sum(row == reviewed_row for row in expected) != 1
            for reviewed_row in reviewed_rows
        )
    ):
        raise ValueError(
            "HUD UpdateFrame tail bridge requires the exact immutable "
            "ordinal-13-through-16 retail call contract"
        )

    bounded_start = 0x129
    bounded_end = 0x17B
    exact_body = bytes.fromhex(
        "8b 0d 88 47 00 00 dd d8 f6 41 0c 10 75 05 8b 11 "
        "ff 52 04 a1 00 00 00 00 8b 15 64 03 00 00 50 "
        "b9 64 03 00 00 ff 52 24 be 44 0f 00 00 "
        "8b 86 bc 00 00 00 8d 8e bc 00 00 00 57 ff 50 60 "
        "8b 16 8b ce 57 ff 52 60 81 c6 c0 01 00 00 "
        "81 fe 44 47 00 00 7c da"
    )
    if (
        len(caller.data)
        != address_value(_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CALLER_END_EXCLUSIVE)
        - address_value(_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CALLER_START)
        or len(caller.relocation_mask) != len(caller.data)
        or caller.data[bounded_start:bounded_end] != exact_body
    ):
        raise ValueError(
            "HUD UpdateFrame tail bridge requires the exact bounded "
            "floating/reticle/marker object body"
        )

    expected_relocations = (
        (0x12B, IMAGE_REL_I386_DIR32, aggregate, 0x4788),
        (
            0x13D,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_OBJECT_SYMBOL,
            0,
        ),
        (0x143, IMAGE_REL_I386_DIR32, aggregate, 0x364),
        (0x149, IMAGE_REL_I386_DIR32, aggregate, 0x364),
        (0x151, IMAGE_REL_I386_DIR32, aggregate, 0xF44),
        (0x175, IMAGE_REL_I386_DIR32, aggregate, 0x4744),
    )
    actual_relocations = tuple(
        (
            row.offset,
            row.type,
            row.symbol_name,
            (
                struct.unpack_from("<I", caller.data, row.offset)[0]
                if 0 <= row.offset <= len(caller.data) - 4
                else None
            ),
        )
        for row in caller.relocations
        if bounded_start <= row.offset < bounded_end
    )
    if actual_relocations != expected_relocations:
        raise ValueError(
            "HUD UpdateFrame tail bridge rejects a missing, duplicate, "
            "extra, reordered, wrong-target, or wrong-addend DIR32 relocation"
        )
    expected_mask = {
        offset
        for relocation_offset, _, _, _ in expected_relocations
        for offset in range(relocation_offset, relocation_offset + 4)
    }
    if {
        offset
        for offset in range(bounded_start, bounded_end)
        if caller.relocation_mask[offset]
    } != expected_mask:
        raise ValueError(
            "HUD UpdateFrame tail bridge requires the exact bounded "
            "relocation mask"
        )

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    index_by_offset = {
        offset: index
        for index, offset in enumerate(offsets)
        if offset is not None
    }
    current_layout_offsets = (0x72, 0x78, 0x7D, 0x7E, 0x80)
    current_layout_row = {
        "ordinal": 8,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_LAYOUT_STORAGE_IDENTITY,
        "slot_displacement": 0,
        "cleanup_bytes": None,
    }
    if (
        len(expected) <= 8
        or expected[8] != current_layout_row
        or sum(row == current_layout_row for row in expected) != 1
        or set(current_layout_offsets) - set(index_by_offset)
        or {
            offset
            for offset in offsets
            if (
                offset is not None
                and current_layout_start <= offset < current_layout_end
            )
        }
        != set(current_layout_offsets)
    ):
        raise ValueError(
            "HUD UpdateFrame tail composition requires the exact immutable "
            "ordinal-8 repeated current-layout contract and offsets"
        )
    current_layout_indices = tuple(
        index_by_offset[offset] for offset in current_layout_offsets
    )
    current_layout_sequence = tuple(
        candidate.instructions[index]
        for index in current_layout_indices
    )
    if (
        current_layout_indices
        != tuple(
            range(
                current_layout_indices[0],
                current_layout_indices[0] + len(current_layout_indices),
            )
        )
        or tuple(
            instruction.bytes
            for instruction in current_layout_sequence
        )
        != (
            ("8b", "0d", "18", "00", "00", "00"),
            ("a1", "00", "00", "00", "00"),
            ("50",),
            ("8b", "11"),
            ("ff", "12"),
        )
        or tuple(
            _cc_cfg._instruction_mnemonic(instruction)
            for instruction in current_layout_sequence
        )
        != ("mov", "mov", "push", "mov", "call")
    ):
        raise ValueError(
            "HUD UpdateFrame tail composition rejects repeated "
            "current-layout instruction/byte/order drift"
        )
    sequence_offsets = (
        0x129,
        0x12F,
        0x131,
        0x135,
        0x137,
        0x139,
        0x13C,
        0x141,
        0x147,
        0x148,
        0x14D,
        0x150,
        0x155,
        0x15B,
        0x161,
        0x162,
        0x165,
        0x167,
        0x169,
        0x16A,
        0x16D,
        0x173,
        0x179,
    )
    if (
        0x12 not in index_by_offset
        or set(sequence_offsets) - set(index_by_offset)
        or {
            offset
            for offset in offsets
            if (
                offset is not None
                and bounded_start <= offset < bounded_end
            )
        }
        != set(sequence_offsets)
    ):
        raise ValueError(
            "HUD UpdateFrame tail bridge requires the exact bounded "
            "candidate instruction offsets and EDI-zero predecessor"
        )
    sequence_indices = tuple(
        index_by_offset[offset] for offset in sequence_offsets
    )
    if sequence_indices != tuple(
        range(sequence_indices[0], sequence_indices[0] + len(sequence_indices))
    ):
        raise ValueError(
            "HUD UpdateFrame tail bridge rejects a missing, duplicate, "
            "reordered, or noncontiguous candidate sequence"
        )
    sequence = tuple(
        candidate.instructions[index] for index in sequence_indices
    )
    expected_instruction_bytes = (
        ("8b", "0d", "88", "47", "00", "00"),
        ("dd", "d8"),
        ("f6", "41", "0c", "10"),
        ("75", "05"),
        ("8b", "11"),
        ("ff", "52", "04"),
        ("a1", "00", "00", "00", "00"),
        ("8b", "15", "64", "03", "00", "00"),
        ("50",),
        ("b9", "64", "03", "00", "00"),
        ("ff", "52", "24"),
        ("be", "44", "0f", "00", "00"),
        ("8b", "86", "bc", "00", "00", "00"),
        ("8d", "8e", "bc", "00", "00", "00"),
        ("57",),
        ("ff", "50", "60"),
        ("8b", "16"),
        ("8b", "ce"),
        ("57",),
        ("ff", "52", "60"),
        ("81", "c6", "c0", "01", "00", "00"),
        ("81", "fe", "44", "47", "00", "00"),
        ("7c", "da"),
    )
    expected_mnemonics = (
        "mov",
        "fstp",
        "test",
        "jne",
        "mov",
        "call",
        "mov",
        "mov",
        "push",
        "mov",
        "call",
        "mov",
        "mov",
        "lea",
        "push",
        "call",
        "mov",
        "mov",
        "push",
        "call",
        "add",
        "cmp",
        "jl",
    )
    if (
        tuple(instruction.bytes for instruction in sequence)
        != expected_instruction_bytes
        or tuple(
            _cc_cfg._instruction_mnemonic(instruction) for instruction in sequence
        )
        != expected_mnemonics
    ):
        raise ValueError(
            "HUD UpdateFrame tail bridge rejects candidate "
            "opcode/byte/order drift"
        )

    def exact_operand(offset: int) -> str:
        return _cc_cfg._instruction_operand(
            candidate.instructions[index_by_offset[offset]]
        ).strip()

    def exact_memory_source(offset: int) -> tuple[str, str]:
        destination, source = exact_operand(offset).split(",", 1)
        return destination.strip().lower(), _cc_targets._exact_memory_expression(source)

    aggregate_decimal = re.escape(aggregate)
    if (
        exact_memory_source(0x72)
        not in {
            ("ecx", f"{aggregate}+24"),
            ("ecx", f"{aggregate}+0x18"),
        }
        or exact_memory_source(0x78)
        != ("eax", _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_OBJECT_SYMBOL)
        or exact_operand(0x7D).lower() != "eax"
        or exact_memory_source(0x7E) != ("edx", "ecx")
        or _cc_targets._exact_memory_expression(exact_operand(0x80)) != "edx"
        or exact_memory_source(0x129)
        not in {
            ("ecx", f"{aggregate}+18312"),
            ("ecx", f"{aggregate}+0x4788"),
        }
        or re.fullmatch(
            r"(?:byte\s+(?:ptr\s+)?)?\[ecx\+(?:12|0xc)\]\s*,\s*"
            r"(?:16|0x10)",
            exact_operand(0x131),
            flags=re.IGNORECASE,
        )
        is None
        or exact_memory_source(0x137) != ("edx", "ecx")
        or _cc_targets._exact_memory_expression(exact_operand(0x139))
        not in {"edx+4", "edx+0x4"}
        or exact_memory_source(0x13C)
        != ("eax", _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_TIME_OBJECT_SYMBOL)
        or exact_memory_source(0x141)
        not in {
            ("edx", f"{aggregate}+868"),
            ("edx", f"{aggregate}+0x364"),
        }
        or exact_operand(0x147).lower() != "eax"
        or re.fullmatch(
            rf"ecx\s*,\s*OFFSET\s+FLAT:{aggregate_decimal}"
            r"\+(?:868|0x364)",
            exact_operand(0x148),
            flags=re.IGNORECASE,
        )
        is None
        or _cc_targets._exact_memory_expression(exact_operand(0x14D))
        not in {"edx+36", "edx+0x24"}
        or re.fullmatch(
            rf"esi\s*,\s*OFFSET\s+FLAT:{aggregate_decimal}"
            r"\+(?:3908|0xf44)",
            exact_operand(0x150),
            flags=re.IGNORECASE,
        )
        is None
        or exact_memory_source(0x155)
        not in {("eax", "esi+188"), ("eax", "esi+0xbc")}
        or exact_memory_source(0x15B)
        not in {("ecx", "esi+188"), ("ecx", "esi+0xbc")}
        or exact_operand(0x161).lower() != "edi"
        or _cc_targets._exact_memory_expression(exact_operand(0x162))
        not in {"eax+96", "eax+0x60"}
        or exact_memory_source(0x165) != ("edx", "esi")
        or re.fullmatch(
            r"ecx\s*,\s*esi",
            exact_operand(0x167),
            flags=re.IGNORECASE,
        )
        is None
        or exact_operand(0x169).lower() != "edi"
        or _cc_targets._exact_memory_expression(exact_operand(0x16A))
        not in {"edx+96", "edx+0x60"}
        or re.fullmatch(
            r"esi\s*,\s*(?:448|0x1c0)",
            exact_operand(0x16D),
            flags=re.IGNORECASE,
        )
        is None
        or re.fullmatch(
            rf"esi\s*,\s*OFFSET\s+FLAT:{aggregate_decimal}"
            r"\+(?:18244|0x4744)",
            exact_operand(0x173),
            flags=re.IGNORECASE,
        )
        is None
    ):
        raise ValueError(
            "HUD UpdateFrame tail bridge rejects floating visibility, "
            "receiver, argument, vptr, slot, marker, stride, or end drift"
        )

    cfg_specs = {
        0x135: ("conditional", 0x13C),
        0x179: ("conditional", 0x155),
    }
    exact_cfg = {
        source: _cc_cfg._exact_local_direct_branch(
            candidate.instructions[index_by_offset[source]],
            instruction_index=index_by_offset[source],
            instruction_addresses=offsets,
            instruction_index_by_address=index_by_offset,
            source="cod",
            caller_start=0,
            caller_end=(
                address_value(_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CALLER_END_EXCLUSIVE)
                - address_value(_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_CALLER_START)
            ),
        )
        for source in cfg_specs
    }
    scoped_indices = frozenset(
        (*current_layout_indices, *sequence_indices)
    )
    incoming_scoped_targets = {
        (source, target_index)
        for source, targets in candidate.local_control_flow_targets.items()
        for target_index in targets
        if target_index in scoped_indices
    }
    if (
        any(
            exact_cfg[source]
            != (kind, index_by_offset[target])
            for source, (kind, target) in cfg_specs.items()
        )
        or bool(candidate.local_control_flow_indices & scoped_indices)
        or incoming_scoped_targets
    ):
        raise ValueError(
            "HUD UpdateFrame tail bridge requires the exact visibility-skip "
            "and 0x1c0-stride marker-loop CFG with no alternate entry"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    call_offsets = (0x139, 0x14D, 0x162, 0x16A)
    call_indices = tuple(index_by_offset[offset] for offset in call_offsets)
    if (
        invocation_indices[8] != index_by_offset[0x80]
        or tuple(invocation_indices[13:17]) != call_indices
    ):
        raise ValueError(
            "HUD UpdateFrame tail bridge requires the exact candidate "
            "ordinal-8 and ordinal-13-through-16 call order"
        )

    reaching_definitions = (
        ("ecx", 0x72, 0x80, (0x72,)),
        ("eax", 0x72, 0x80, (0x78,)),
        ("edx", 0x72, 0x80, (0x7E,)),
        ("ecx", 0x129, 0x139, (0x129,)),
        ("edx", 0x129, 0x139, (0x137,)),
        ("eax", 0x13C, 0x14D, (0x13C,)),
        ("edx", 0x13C, 0x14D, (0x141,)),
        ("ecx", 0x13C, 0x14D, (0x148,)),
        ("esi", 0x150, 0x16A, (0x150,)),
        ("eax", 0x150, 0x162, (0x155,)),
        ("ecx", 0x150, 0x162, (0x15B,)),
        ("edx", 0x165, 0x16A, (0x165,)),
        ("ecx", 0x165, 0x16A, (0x167,)),
    )
    if any(
        [
            index
            for index in range(
                index_by_offset[start_offset],
                index_by_offset[end_offset],
            )
            if _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index], register
            )
        ]
        != [index_by_offset[offset] for offset in desired_offsets]
        for register, start_offset, end_offset, desired_offsets
        in reaching_definitions
    ):
        raise ValueError(
            "HUD UpdateFrame tail bridge rejects receiver/vptr/argument "
            "reaching-definition drift"
        )
    edi_zero_index = index_by_offset[0x12]
    if (
        candidate.instructions[edi_zero_index].bytes != ("33", "ff")
        or _cc_cfg._instruction_mnemonic(
            candidate.instructions[edi_zero_index]
        )
        != "xor"
        or re.fullmatch(
            r"edi\s*,\s*edi",
            _cc_cfg._instruction_operand(candidate.instructions[edi_zero_index]),
            flags=re.IGNORECASE,
        )
        is None
        or [
            index
            for index in range(edi_zero_index, index_by_offset[0x16A])
            if _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index], "edi"
            )
        ]
        != [edi_zero_index]
        or _cc_cfg._cleanup_after(
            candidate.instructions, index_by_offset[0x80]
        )
        is not None
        or _cc_cfg._cleanup_after(
            candidate.instructions, index_by_offset[0x139]
        )
        is not None
        or _cc_cfg._cleanup_after(
            candidate.instructions, index_by_offset[0x14D]
        )
        is not None
        or _cc_cfg._cleanup_after(
            candidate.instructions, index_by_offset[0x162]
        )
        is not None
        or _cc_cfg._cleanup_after(
            candidate.instructions, index_by_offset[0x16A]
        )
        is not None
    ):
        raise ValueError(
            "HUD UpdateFrame tail bridge requires the exact persistent "
            "EDI-zero arguments and caller-cleanup behavior"
        )

    reviewed_result_calls = (
        (8, index_by_offset[0x80]),
        *tuple(enumerate(call_indices, start=13)),
    )
    for ordinal, call_index in reviewed_result_calls:
        next_invocation = (
            invocation_indices[ordinal + 1]
            if ordinal + 1 < len(invocation_indices)
            else len(candidate.instructions)
        )
        for instruction in candidate.instructions[
            call_index + 1 : next_invocation
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
                        "HUD UpdateFrame tail bridge rejects unexpected "
                        "call-result consumption"
                    )
            if _cc_cfg._instruction_may_clobber_register(instruction, "eax"):
                break

    repeated_layout_member = ReviewedMemberVptrStorageBridge(
        register="edx",
        source_register="ecx",
        source_provenance=aggregate_identity,
        receiver_register="ecx",
        receiver_provenance=aggregate_identity,
        storage_identity=_cc_catalog.HUD_UI_MGR_UPDATE_FRAME_LAYOUT_STORAGE_IDENTITY,
        slot_displacement=0,
        call_address="0x80",
    )
    if (
        "0x7e" in member_bridges
        and member_bridges["0x7e"] != repeated_layout_member
    ):
        raise ValueError(
            "HUD UpdateFrame tail repeated current-layout member-vptr "
            "bridge conflicts with its predecessor composition"
        )
    member_bridges["0x7e"] = repeated_layout_member

    bridges = {
        "0x139": ReviewedExactIndirectStorageBridge(
            register="edx",
            storage_identity=reviewed_rows[0]["storage_identity"],
            slot_displacement=4,
            assembly_source="cod",
        ),
        "0x14d": ReviewedExactIndirectStorageBridge(
            register="edx",
            storage_identity=reviewed_rows[1]["storage_identity"],
            slot_displacement=0x24,
            assembly_source="cod",
        ),
        "0x162": ReviewedExactIndirectStorageBridge(
            register="eax",
            storage_identity=reviewed_rows[2]["storage_identity"],
            slot_displacement=0x60,
            assembly_source="cod",
        ),
        "0x16a": ReviewedExactIndirectStorageBridge(
            register="edx",
            storage_identity=reviewed_rows[3]["storage_identity"],
            slot_displacement=0x60,
            assembly_source="cod",
        ),
    }
    return static_bridges, absolute_bridges, member_bridges, bridges
