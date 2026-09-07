"""Recoil call-contract recoil hud construction evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import extraction as _cc_extraction
from _recoil.call_contract import receiver_instructions as _cc_receiver_instructions
from _recoil.call_contract import recoil_hud_sensor as _cc_recoil_hud_sensor
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        IdentityIndexes,
        ReviewedExactIndirectStorageBridge,
    )

import re
import struct
from dataclasses import replace
from pathlib import Path
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_DIR32,
    IMAGE_REL_I386_REL32,
    Instruction,
)
from _recoil.commands.provider_target_mutation import retail_import_target
from _recoil.lib.authored_icf import exact_selected_target_membership
from _recoil.lib.binja import BinaryNinjaBridge
from _recoil.lib.progress import ProgressDocument, address_value, normalize_address


def _hud_ui_mgr_ensure_ceil_retail_iat_indexes(
    retail_instructions: Sequence[Instruction],
    *,
    candidate: CandidateAssembly | None = None,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_data_rows: Sequence[Any],
    bridge: BinaryNinjaBridge,
    reference: Path = _cc_catalog.DEFAULT_REFERENCE,
) -> IdentityIndexes:
    """Resolve only the reviewed HUD callers' exact ``ceil`` IAT calls."""
    normalized_start = normalize_address(caller_start)
    target_id = "recoil:vc5-target:hud_404ca0_415ab0_authored_order"
    if normalized_start == _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START:
        caller_symbol_id = _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_IDENTITY.removeprefix(
            "symbol:"
        )
        caller_identity_expected = _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_IDENTITY
        caller_end_expected = _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_END_EXCLUSIVE
        caller_size_expected = 0xBB0
        caller_navigation_name = "HudUiMgr::EnsureHudLoaded"
        caller_anchor_id = _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_ANCHOR_ID
        caller_source_path = _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        required_target_ids: tuple[str, ...] | None = None
        caller_label = "HUD EnsureHudLoaded"
        call_specs = (
            (
                _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_CALL_ADDRESS,
                _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_CALL_ORDINAL,
                _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_CLEANUP_BYTES,
            ),
        )
        fixed_rows = {
            0x410645: (
                "fild",
                ("db", "05", "34", "68", "4e", "00"),
            ),
            0x41064B: ("sub", ("83", "ec", "08")),
            0x41064E: ("fstp", ("dd", "1c", "24")),
            0x410651: (
                "call",
                ("ff", "15", "00", "c5", "4c", "00"),
            ),
            0x410657: ("add", ("83", "c4", "08")),
        }
    elif normalized_start == _cc_catalog.HUD_UI_MGR_OBJECTIVE_CEIL_CALLER_START:
        caller_symbol_id = (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_CEIL_CALLER_IDENTITY.removeprefix("symbol:")
        )
        caller_identity_expected = _cc_catalog.HUD_UI_MGR_OBJECTIVE_CEIL_CALLER_IDENTITY
        caller_end_expected = (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_CEIL_CALLER_END_EXCLUSIVE
        )
        caller_size_expected = 0x90
        caller_navigation_name = (
            "HudUiMgrObjective::SetVisibleAndResetMeterFill"
        )
        caller_anchor_id = _cc_catalog.HUD_UI_MGR_OBJECTIVE_CEIL_CALLER_ANCHOR_ID
        caller_source_path = "src/Battlesport/hud.cpp"
        required_target_ids = (
            target_id,
        )
        caller_label = "HUD objective SetVisibleAndResetMeterFill"
        call_specs = (
            (
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_CEIL_CALL_ADDRESS,
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_CEIL_CALL_ORDINAL,
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_CEIL_CLEANUP_BYTES,
            ),
        )
        fixed_rows = {
            0x411787: (
                "call",
                ("ff", "15", "00", "c5", "4c", "00"),
            ),
            0x41178D: ("add", ("83", "c4", "08")),
        }
    elif normalized_start == _cc_catalog.HUD_UI_MGR_OBJECTIVE_TICK_CEIL_CALLER_START:
        caller_symbol_id = (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_TICK_CEIL_CALLER_IDENTITY.removeprefix(
                "symbol:"
            )
        )
        caller_identity_expected = (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_TICK_CEIL_CALLER_IDENTITY
        )
        caller_end_expected = (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_TICK_CEIL_CALLER_END_EXCLUSIVE
        )
        caller_size_expected = 0xC0
        caller_navigation_name = (
            "HudUiMgrObjective::TickMeterFillAnimation"
        )
        caller_anchor_id = (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_TICK_CEIL_CALLER_ANCHOR_ID
        )
        caller_source_path = "src/Battlesport/hud.cpp"
        required_target_ids = (
            target_id,
        )
        caller_label = "HUD objective TickMeterFillAnimation"
        call_specs = _cc_catalog.HUD_UI_MGR_OBJECTIVE_TICK_CEIL_CALL_SPECS
        fixed_rows = {
            0x41181D: (
                "call",
                ("ff", "15", "00", "c5", "4c", "00"),
            ),
            0x411823: ("add", ("83", "c4", "08")),
            0x411875: (
                "call",
                ("ff", "15", "00", "c5", "4c", "00"),
            ),
            0x41187B: ("add", ("83", "c4", "08")),
        }
    elif normalized_start == _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_CEIL_CALLER_START:
        caller_symbol_id = (
            _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_CEIL_CALLER_IDENTITY.removeprefix(
                "symbol:"
            )
        )
        caller_identity_expected = (
            _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_CEIL_CALLER_IDENTITY
        )
        caller_end_expected = (
            _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_CEIL_CALLER_END_EXCLUSIVE
        )
        caller_size_expected = 0x140
        caller_navigation_name = (
            "HudUiMgrSensor::SetShieldMessageRatio"
        )
        caller_anchor_id = (
            _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_CEIL_CALLER_ANCHOR_ID
        )
        caller_source_path = "src/Battlesport/hud.cpp"
        required_target_ids = (
            target_id,
        )
        caller_label = "HUD sensor SetShieldMessageRatio"
        call_specs = _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_CEIL_CALL_SPECS
        fixed_rows = {
            0x411F98: ("sub", ("83", "ec", "08")),
            0x411FAB: ("fstp", ("dd", "1c", "24")),
            0x411FAE: (
                "call",
                ("ff", "15", "00", "c5", "4c", "00"),
            ),
            0x411FB4: ("add", ("83", "c4", "08")),
            0x411FF6: ("sub", ("83", "ec", "08")),
            0x411FFF: ("fstp", ("dd", "1c", "24")),
            0x412002: (
                "call",
                ("ff", "15", "00", "c5", "4c", "00"),
            ),
            0x412008: ("add", ("83", "c4", "08")),
        }
    elif normalized_start == _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALLER_START:
        caller_symbol_id = (
            _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALLER_IDENTITY.removeprefix(
                "symbol:"
            )
        )
        caller_identity_expected = (
            _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALLER_IDENTITY
        )
        caller_end_expected = (
            _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALLER_END_EXCLUSIVE
        )
        caller_size_expected = 0x170
        caller_navigation_name = (
            "HudUiMgrTarget::UpdateSelectedProgressMeter"
        )
        caller_anchor_id = (
            _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALLER_ANCHOR_ID
        )
        caller_source_path = "src/Battlesport/hud.cpp"
        required_target_ids = (
            target_id,
        )
        caller_label = "HUD target UpdateSelectedProgressMeter"
        call_specs = (
            (
                _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALL_ADDRESS,
                _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALL_ORDINAL,
                _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CLEANUP_BYTES,
            ),
        )
        fixed_rows = {
            0x4125BD: (
                "fild",
                ("db", "05", "a4", "6d", "4e", "00"),
            ),
            0x4125C3: ("sub", ("83", "ec", "08")),
            0x4125C6: ("fmul", ("d8", "c9")),
            0x4125C8: ("fstp", ("dd", "1c", "24")),
            0x4125CB: ("fstp", ("dd", "d8")),
            0x4125CD: (
                "call",
                ("ff", "15", "00", "c5", "4c", "00"),
            ),
            0x4125D3: ("add", ("83", "c4", "08")),
        }
    elif normalized_start == _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CALLER_START:
        caller_symbol_id = (
            _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CALLER_IDENTITY.removeprefix("symbol:")
        )
        caller_identity_expected = _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CALLER_IDENTITY
        caller_end_expected = _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CALLER_END_EXCLUSIVE
        caller_size_expected = 0x90
        caller_navigation_name = (
            "HudUiMessage::SetValueIfOwnerMatches"
        )
        caller_anchor_id = _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CALLER_ANCHOR_ID
        caller_source_path = _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        required_target_ids = (
            target_id,
            (
                "recoil:vc5-target:"
                "hud_ui_message_set_value_if_owner_matches"
            ),
        )
        caller_label = "HUD message SetValueIfOwnerMatches"
        call_specs = (
            (
                _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CEIL_CALL_ADDRESS,
                _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CEIL_CALL_ORDINAL,
                _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CEIL_CLEANUP_BYTES,
            ),
        )
        fixed_rows = {
            0x41269C: ("fld", ("d9", "44", "24", "08")),
            0x4126A0: ("sub", ("83", "ec", "08")),
            0x4126A3: ("fstp", ("dd", "1c", "24")),
            0x4126A6: (
                "call",
                ("ff", "15", "00", "c5", "4c", "00"),
            ),
            0x4126AC: ("add", ("83", "c4", "08")),
            0x4126AF: (
                "call",
                ("e8", "f2", "39", "0b", "00"),
            ),
        }
    else:
        return indexes
    caller = document.collection("symbols").get(caller_symbol_id)
    trace = (
        caller.get("source_traceability")
        if isinstance(caller, Mapping)
        else None
    )
    source_edges = (
        trace.get("source_edges") if isinstance(trace, Mapping) else None
    )
    if (
        caller_identity != caller_identity_expected
        or normalize_address(caller_end_exclusive) != caller_end_expected
        or not isinstance(caller, Mapping)
        or caller.get("binary") != "recoil"
        or caller.get("kind") != "function"
        or caller.get("pipeline_class") != "authored"
        or caller.get("ownership_state") != "primary-owned"
        or caller.get("address") != normalized_start
        or caller.get("end_exclusive") != caller_end_expected
        or caller.get("extent_state") != "known"
        or caller.get("size") != caller_size_expected
        or caller.get("navigation_name") != caller_navigation_name
        or caller.get("output_section_id") != "recoil:section:.text"
        or caller.get("physical_block_id") != "recoil:block:0x404ca0"
        or not all(
            exact_selected_target_membership(
                caller.get("verification_target_ids", ()), required_id,
            )
            for required_id in (required_target_ids or (target_id,))
        )
        or indexes.by_address.get(normalized_start)
        != caller_identity_expected
        or caller_identity_expected in indexes.provider_ids
        or not isinstance(trace, Mapping)
        or trace.get("state") != "resolved"
        or trace.get("reason_code") not in {None, ""}
        or not isinstance(source_edges, list)
        or len(source_edges) != 1
        or not isinstance(source_edges[0], Mapping)
        or source_edges[0].get("relation") != "defines"
        or source_edges[0].get("anchor_id") != caller_anchor_id
        or source_edges[0].get("emission_context")
        != {"translation_unit": caller_source_path}
    ):
        raise ValueError(
            f"{caller_label} ceil IAT bridge requires its exact "
            "current authored caller and source authority"
        )

    if normalized_start == _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_CEIL_CALLER_START:
        target = candidate.target if candidate is not None else None
        contribution_rows = [
            (entry, row)
            for entry in getattr(
                target,
                "translation_unit_function_order",
                (),
            )
            for row in getattr(entry, "functions", ())
            if normalize_address(str(getattr(row, "address", "")))
            == _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_CEIL_CALLER_START
        ]
        if (
            candidate is None
            or candidate.caller_definition is None
            or candidate.caller_definition.symbol
            != _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_CEIL_CALLER_SYMBOL
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
                f"{caller_label} ceil IAT bridge requires one exact current "
                "HUD candidate contribution authority"
            )
        contribution, contribution_row = contribution_rows[0]
        if (
            getattr(contribution, "source_from", "")
            != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
            or getattr(contribution, "order_scope", "") != "authored"
            or getattr(contribution_row, "symbol", "") != ""
            or getattr(contribution_row, "symbol_regex", None)
            != r"\?SetShieldMessageRatio@HudUiMgrSensor@@.*"
            or getattr(contribution_row, "name", "")
            != "HudUiMgrSensor::SetShieldMessageRatio"
            or getattr(contribution_row, "pipeline_class", "")
            != "authored"
            or getattr(contribution_row, "authored_order_role", "")
            != "authored-body"
            or not bool(
                getattr(contribution_row, "required_presence", False)
            )
            or not bool(
                getattr(contribution_row, "full_order_gate", False)
            )
        ):
            raise ValueError(
                f"{caller_label} ceil IAT bridge requires the exact authored "
                "hud.cpp contribution row"
            )

    if normalized_start == _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALLER_START:
        target = candidate.target if candidate is not None else None
        caller_definition = (
            candidate.caller_definition if candidate is not None else None
        )
        contribution_rows = [
            (entry, row)
            for entry in getattr(
                target,
                "translation_unit_function_order",
                (),
            )
            for row in getattr(entry, "functions", ())
            if normalize_address(str(getattr(row, "address", "")))
            == _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALLER_START
        ]
        if (
            candidate is None
            or caller_definition is None
            or caller_definition.symbol
            != _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALLER_SYMBOL
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
                f"{caller_label} ceil IAT bridge requires one exact current "
                "HUD candidate contribution authority"
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
                r"\?UpdateSelectedProgressMeter@HudUiMgrTarget@@.*",
                caller_definition.symbol,
            )
            is None
            or getattr(contribution_row, "name", "")
            != "HudUiMgrTarget::UpdateSelectedProgressMeter"
            or getattr(contribution_row, "pipeline_class", "")
            != "authored"
            or getattr(contribution_row, "authored_order_role", "")
            != "authored-body"
            or not bool(
                getattr(contribution_row, "required_presence", False)
            )
            or not bool(
                getattr(contribution_row, "full_order_gate", False)
            )
        ):
            raise ValueError(
                f"{caller_label} ceil IAT bridge requires the exact authored "
                "hud.cpp contribution row"
            )

        candidate_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
        offset_counts: dict[int, int] = {}
        for offset in candidate_offsets:
            if offset is not None:
                offset_counts[offset] = offset_counts.get(offset, 0) + 1
        candidate_index_by_offset = {
            offset: index
            for index, offset in enumerate(candidate_offsets)
            if offset is not None and offset_counts.get(offset) == 1
        }
        candidate_instruction_by_offset = {
            offset: candidate.instructions[index]
            for offset, index in candidate_index_by_offset.items()
        }
        call_offset = (
            _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CANDIDATE_CALL_OFFSET
        )
        relocation_offset = (
            _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CANDIDATE_RELOCATION_OFFSET
        )
        cleanup_offset = (
            _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CANDIDATE_CLEANUP_OFFSET
        )
        candidate_call = candidate_instruction_by_offset.get(call_offset)
        candidate_cleanup = candidate_instruction_by_offset.get(cleanup_offset)
        matching_relocations = tuple(
            relocation
            for relocation in caller_definition.relocations
            if relocation.offset == relocation_offset
        )
        candidate_invocations = _cc_callable_identity._candidate_static_invocation_indices(
            candidate,
            caller_start=normalized_start,
            caller_end_exclusive=caller_end_exclusive,
        )
        if (
            candidate_call is None
            or _cc_cfg._instruction_mnemonic(candidate_call) != "call"
            or tuple(value.lower() for value in candidate_call.bytes)
            != ("ff", "15", "00", "00", "00", "00")
            or _cc_targets._exact_memory_expression(
                _cc_cfg._instruction_operand(candidate_call)
            )
            != _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_CANDIDATE_IMPORT_SYMBOL
            or candidate_cleanup is None
            or _cc_cfg._instruction_mnemonic(candidate_cleanup) != "add"
            or tuple(value.lower() for value in candidate_cleanup.bytes)
            != ("83", "c4", "08")
            or candidate_index_by_offset.get(call_offset)
            not in candidate_invocations
            or candidate_invocations.index(
                candidate_index_by_offset[call_offset]
            )
            != _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALL_ORDINAL
            or _cc_cfg._cleanup_after(
                candidate.instructions,
                candidate_index_by_offset[call_offset],
            )
            != _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CLEANUP_BYTES
            or len(matching_relocations) != 1
            or matching_relocations[0].type != IMAGE_REL_I386_DIR32
            or matching_relocations[0].symbol_name
            != _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_CANDIDATE_IMPORT_SYMBOL
            or relocation_offset + 4 > len(caller_definition.data)
            or struct.unpack_from(
                "<I",
                caller_definition.data,
                relocation_offset,
            )[0]
            != 0
            or len(caller_definition.relocation_mask)
            != len(caller_definition.data)
            or not all(
                caller_definition.relocation_mask[offset]
                for offset in range(relocation_offset, relocation_offset + 4)
            )
            or caller_definition.undefined_external_functions.count(
                _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_CANDIDATE_IMPORT_SYMBOL
            )
            != 1
            or (
                caller_definition.defined_external_functions
                + caller_definition.undefined_external_data
                + caller_definition.defined_external_data
            ).count(_cc_catalog.HUD_UI_MGR_ENSURE_CEIL_CANDIDATE_IMPORT_SYMBOL)
            != 0
            or candidate_index_by_offset[call_offset]
            in candidate.local_control_flow_indices
            or any(
                candidate_index_by_offset[call_offset] in targets
                for targets in candidate.local_control_flow_targets.values()
            )
        ):
            raise ValueError(
                f"{caller_label} ceil IAT bridge rejects exact candidate "
                "ordinal-3 FF15 __imp__ceil call, DIR32 relocation/addend/"
                "external identity, relocation mask, or 8-byte cleanup drift"
            )

    if normalized_start == _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CALLER_START:
        caller_definition = (
            candidate.caller_definition if candidate is not None else None
        )
        if (
            candidate is None
            or caller_definition is None
            or caller_definition.symbol
            != _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CALLER_SYMBOL
            or len(caller_definition.data) != 0x90
            or len(caller_definition.relocation_mask)
            != len(caller_definition.data)
        ):
            raise ValueError(
                f"{caller_label} ceil IAT bridge requires the exact current "
                "0x90-byte candidate caller definition"
            )

        candidate_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
        offset_counts: dict[int, int] = {}
        for offset in candidate_offsets:
            if offset is not None:
                offset_counts[offset] = offset_counts.get(offset, 0) + 1
        candidate_index_by_offset = {
            offset: index
            for index, offset in enumerate(candidate_offsets)
            if offset is not None and offset_counts.get(offset) == 1
        }
        candidate_instruction_by_offset = {
            offset: candidate.instructions[index]
            for offset, index in candidate_index_by_offset.items()
        }
        candidate_fixed = {
            0x50: ("fld", ("d9", "44", "24", "10")),
            0x54: (
                "mov",
                ("8b", "9e", "e0", "00", "00", "00"),
            ),
            0x5A: ("sub", ("83", "ec", "08")),
            0x5D: (
                "lea",
                ("8d", "be", "e0", "00", "00", "00"),
            ),
            0x63: ("fstp", ("dd", "1c", "24")),
            _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CEIL_CANDIDATE_CALL_OFFSET: (
                "call",
                ("ff", "15", "00", "00", "00", "00"),
            ),
            _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CEIL_CANDIDATE_CLEANUP_OFFSET: (
                "add",
                ("83", "c4", "08"),
            ),
            _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_FTOL_CANDIDATE_CALL_OFFSET: (
                "call",
                ("e8", "00", "00", "00", "00"),
            ),
        }
        candidate_window = tuple(
            (
                offset,
                _cc_cfg._instruction_mnemonic(instruction),
                tuple(value.lower() for value in instruction.bytes),
                _cc_cfg._instruction_operand(instruction).strip(),
            )
            for offset, instruction in sorted(
                candidate_instruction_by_offset.items()
            )
            if 0x50 <= offset < 0x80
        )
        if any(
            offset not in candidate_instruction_by_offset
            or _cc_cfg._instruction_mnemonic(
                candidate_instruction_by_offset[offset]
            )
            != mnemonic
            or tuple(
                value.lower()
                for value in candidate_instruction_by_offset[offset].bytes
            )
            != body
            for offset, (mnemonic, body) in candidate_fixed.items()
        ):
            raise ValueError(
                f"{caller_label} ceil IAT bridge requires the exact candidate "
                "qword argument, FF15 ceil call, cleanup, and successor "
                f"__ftol instruction bytes; candidate_window={candidate_window!r}"
            )

        candidate_fixed_indices = [
            candidate_index_by_offset[offset] for offset in candidate_fixed
        ]
        ceil_index = candidate_index_by_offset[
            _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CEIL_CANDIDATE_CALL_OFFSET
        ]
        ftol_index = candidate_index_by_offset[
            _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_FTOL_CANDIDATE_CALL_OFFSET
        ]
        ceil_instruction = candidate.instructions[ceil_index]
        ftol_instruction = candidate.instructions[ftol_index]
        candidate_invocations = _cc_callable_identity._candidate_static_invocation_indices(
            candidate,
            caller_start=normalized_start,
            caller_end_exclusive=caller_end_exclusive,
        )
        candidate_invocation_offsets = tuple(
            candidate_offsets[index] for index in candidate_invocations
        )
        bounded_indices = set(
            range(
                candidate_fixed_indices[0],
                candidate_fixed_indices[-1] + 1,
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
            or _cc_targets._exact_memory_expression(
                _cc_cfg._instruction_operand(ceil_instruction)
            )
            != _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_CANDIDATE_IMPORT_SYMBOL
            or _cc_cfg._instruction_operand(ftol_instruction).strip()
            != _cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL
            or candidate_invocation_offsets
            != _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CANDIDATE_INVOCATION_ORDER
            or candidate_invocations.index(ceil_index)
            != _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CEIL_CALL_ORDINAL
            or candidate_invocations.index(ftol_index)
            != _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_FTOL_CALL_ORDINAL
            or _cc_cfg._cleanup_after(candidate.instructions, ceil_index)
            != _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CEIL_CLEANUP_BYTES
            or candidate.local_control_flow_indices & bounded_indices
            or any(
                target in bounded_indices
                for targets in candidate.local_control_flow_targets.values()
                for target in targets
            )
        ):
            raise ValueError(
                f"{caller_label} ceil IAT bridge rejects candidate caller, "
                "ordinal-1 ceil/ordinal-2 __ftol, 8-byte cleanup, or complete "
                "invocation-order topology drift"
            )

        expected_relocations = {
            _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_CEIL_CANDIDATE_RELOCATION_OFFSET: (
                IMAGE_REL_I386_DIR32,
                _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_CANDIDATE_IMPORT_SYMBOL,
            ),
            _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_FTOL_CANDIDATE_RELOCATION_OFFSET: (
                IMAGE_REL_I386_REL32,
                _cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL,
            ),
        }
        matching_relocations = [
            relocation
            for relocation in caller_definition.relocations
            if relocation.offset in expected_relocations
        ]
        expected_mask = {
            index
            for offset in expected_relocations
            for index in range(offset, offset + 4)
        }
        if (
            len(matching_relocations) != len(expected_relocations)
            or {row.offset for row in matching_relocations}
            != set(expected_relocations)
            or any(
                (row.type, row.symbol_name)
                != expected_relocations[row.offset]
                or struct.unpack_from(
                    "<I",
                    caller_definition.data,
                    row.offset,
                )[0]
                != 0
                for row in matching_relocations
            )
            or {
                index
                for index, value in enumerate(
                    caller_definition.relocation_mask
                )
                if value
            }
            & expected_mask
            != expected_mask
            or caller_definition.undefined_external_functions.count(
                _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_CANDIDATE_IMPORT_SYMBOL
            )
            != 1
            or caller_definition.undefined_external_functions.count(
                _cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL
            )
            != 1
            or (
                caller_definition.defined_external_functions
                + caller_definition.undefined_external_data
                + caller_definition.defined_external_data
            ).count(_cc_catalog.HUD_UI_MGR_ENSURE_CEIL_CANDIDATE_IMPORT_SYMBOL)
            != 0
            or (
                caller_definition.defined_external_functions
                + caller_definition.undefined_external_data
                + caller_definition.defined_external_data
            ).count(_cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL)
            != 0
        ):
            raise ValueError(
                f"{caller_label} ceil IAT bridge rejects candidate ceil "
                "DIR32/__imp__ceil or successor REL32/__ftol target identity, "
                "zero addend, external population, or relocation-mask drift"
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
        retail_index_by_address = {
            address: index
            for index, address in enumerate(retail_addresses)
            if address is not None and retail_counts.get(address) == 1
        }
        retail_fixed_indices = [
            retail_index_by_address[address]
            for address in fixed_rows
            if address in retail_index_by_address
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
            len(retail_fixed_indices) != len(fixed_rows)
            or retail_fixed_indices
            != list(
                range(
                    retail_fixed_indices[0],
                    retail_fixed_indices[0] + len(fixed_rows),
                )
            )
            or retail_invocation_order
            != _cc_catalog.HUD_UI_MESSAGE_SET_VALUE_RETAIL_INVOCATION_ORDER
        ):
            raise ValueError(
                f"{caller_label} ceil IAT bridge rejects exact retail qword/"
                "ceil/cleanup/__ftol adjacency or complete invocation-order "
                "drift"
            )

    retail_import, _directory_context = retail_import_target(
        reference=reference,
        address=_cc_catalog.HUD_UI_MGR_ENSURE_CEIL_IAT_ADDRESS,
        dll=_cc_catalog.HUD_UI_MGR_ENSURE_CEIL_IMPORT_DLL,
        import_name=_cc_catalog.HUD_UI_MGR_ENSURE_CEIL_IMPORT_NAME,
    )
    if (
        retail_import.address != _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_IAT_ADDRESS
        or retail_import.dll != _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_IMPORT_DLL
        or retail_import.import_name
        != _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_IMPORT_NAME
        or retail_import.import_ordinal is not None
    ):
        raise ValueError(
            f"{caller_label} ceil IAT bridge immutable retail import "
            "tuple drifted"
        )

    iat_rows = [
        row
        for row in bridge_data_rows
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_IAT_ADDRESS
    ]
    provider_rows = [
        row
        for row in bridge_data_rows
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_PROVIDER_ADDRESS
    ]
    if (
        len(iat_rows) != 1
        or getattr(iat_rows[0], "name", "")
        != _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_IMPORT_NAME
        or getattr(iat_rows[0], "raw_name", "")
        != _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_IMPORT_NAME
        or getattr(iat_rows[0], "type_text", "")
        != _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_IAT_TYPE
        or getattr(iat_rows[0], "size", 0) != 4
    ):
        raise ValueError(
            f"{caller_label} ceil IAT bridge requires one exact live "
            "retail typed IAT data row"
        )
    if (
        len(provider_rows) != 1
        or getattr(provider_rows[0], "name", "")
        != _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_IMPORT_NAME
        or getattr(provider_rows[0], "raw_name", "")
        != _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_IMPORT_NAME
        or getattr(provider_rows[0], "type_text", "")
        != _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_PROVIDER_TYPE
        or getattr(provider_rows[0], "size", -1) != 0
    ):
        raise ValueError(
            f"{caller_label} ceil IAT bridge requires one exact live "
            "retail bound provider row"
        )

    iat_bytes = _cc_cfg._hexdump_bytes(
        bridge.hexdump(_cc_catalog.HUD_UI_MGR_ENSURE_CEIL_IAT_ADDRESS, 4)
    )
    if (
        len(iat_bytes) != 4
        or normalize_address(struct.unpack("<I", iat_bytes)[0])
        != _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_PROVIDER_ADDRESS
    ):
        raise ValueError(
            f"{caller_label} ceil IAT bridge retail slot bytes do not "
            "bind the exact provider row"
        )

    instruction_addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(normalized_start),
    )
    address_counts: dict[int, int] = {}
    for address in instruction_addresses:
        if address is not None:
            address_counts[address] = address_counts.get(address, 0) + 1
    instruction_by_address = {
        address: instruction
        for address, instruction in zip(
            instruction_addresses,
            retail_instructions,
        )
        if address is not None and address_counts.get(address) == 1
    }
    index_by_address = {
        address: index
        for index, address in enumerate(instruction_addresses)
        if address is not None and address_counts.get(address) == 1
    }
    if any(
        address not in instruction_by_address
        or _cc_cfg._instruction_mnemonic(instruction_by_address[address])
        != mnemonic
        or tuple(
            value.lower()
            for value in instruction_by_address[address].bytes
        )
        != body
        for address, (mnemonic, body) in fixed_rows.items()
    ):
        raise ValueError(
            f"{caller_label} ceil IAT bridge requires the exact retail "
            "argument, FF15, and caller-cleanup body"
        )
    fixed_indices = [index_by_address[address] for address in fixed_rows]
    if fixed_indices != sorted(fixed_indices):
        raise ValueError(
            f"{caller_label} ceil IAT bridge rejects reordered retail "
            "call-neighbor instructions"
        )

    calls_through_slot = [
        instruction
        for instruction in retail_instructions
        if _cc_cfg._instruction_mnemonic(instruction) == "call"
        and _cc_targets._exact_ff15_absolute_address(instruction)
        == _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_IAT_ADDRESS
    ]
    calls_named_ceil = [
        instruction
        for instruction in retail_instructions
        if _cc_cfg._instruction_mnemonic(instruction) == "call"
        and _cc_cfg._instruction_operand(instruction).strip()
        == f"dword [{_cc_catalog.HUD_UI_MGR_ENSURE_CEIL_IMPORT_NAME}]"
    ]
    reviewed_calls = tuple(
        (
            call_address,
            call_ordinal,
            cleanup_bytes,
            index_by_address[address_value(call_address)],
        )
        for call_address, call_ordinal, cleanup_bytes in call_specs
    )
    first_call_index = reviewed_calls[0][3]
    prior_calls = [
        instruction
        for instruction in retail_instructions[:first_call_index]
        if _cc_cfg._instruction_mnemonic(instruction) == "call"
    ]
    prior_objective_slots_are_exact = (
        normalized_start != _cc_catalog.HUD_UI_MGR_OBJECTIVE_CEIL_CALLER_START
        or (
            len(prior_calls) == 2
            and all(
                _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(instruction))
                in {
                    "eax+96",
                    "eax+0x60",
                    "edx+96",
                    "edx+0x60",
                }
                for instruction in prior_calls
            )
        )
    )
    if (
        calls_through_slot != calls_named_ceil
        or tuple(
            _cc_cfg._source_instruction_address(instruction)
            for instruction in calls_through_slot
        )
        != tuple(call_address for call_address, _, _ in call_specs)
        or not prior_objective_slots_are_exact
        or (
            normalized_start
            not in {
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_TICK_CEIL_CALLER_START,
                _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_CEIL_CALLER_START,
                _cc_catalog.HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALLER_START,
            }
            and any(
                _cc_cfg._instruction_mnemonic(instruction) == "jmp"
                for instruction in retail_instructions[:first_call_index]
            )
        )
        or any(
            len(
                [
                    instruction
                    for instruction in retail_instructions[:call_index]
                    if _cc_cfg._instruction_mnemonic(instruction) == "call"
                ]
            )
            != call_ordinal
            or _cc_cfg._cleanup_after(retail_instructions, call_index)
            != cleanup_bytes
            for (
                _call_address,
                call_ordinal,
                cleanup_bytes,
                call_index,
            ) in reviewed_calls
        )
    ):
        if len(call_specs) == 1:
            _call_address, call_ordinal, cleanup_bytes = call_specs[0]
            raise ValueError(
                f"{caller_label} ceil IAT bridge requires exactly the "
                f"reviewed ordinal-{call_ordinal} FF15 ceil call, preceding "
                f"slot sequence, and {cleanup_bytes}-byte caller cleanup"
            )
        ordinal_summary = ", ".join(
            f"{call_address}=ordinal-{call_ordinal}/cleanup-{cleanup_bytes}"
            for call_address, call_ordinal, cleanup_bytes in call_specs
        )
        raise ValueError(
            f"{caller_label} ceil IAT bridge requires exactly the reviewed "
            f"FF15 ceil calls ({ordinal_summary}), preceding slot sequence, "
            "and caller cleanup"
        )

    storage_by_address = dict(indexes.storage_by_address)
    storage_by_name = dict(indexes.storage_by_name)
    for mapping, key in (
        (
            storage_by_address,
            _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_IAT_ADDRESS,
        ),
        (
            storage_by_name,
            _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_IMPORT_NAME,
        ),
        (
            storage_by_name,
            _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_CANDIDATE_IMPORT_SYMBOL,
        ),
    ):
        prior = mapping.get(key)
        if (
            prior is not None
            and prior != _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_IAT_IDENTITY
        ):
            raise ValueError(
                f"{caller_label} ceil IAT bridge conflicts with an "
                f"existing storage identity for {key!r}"
            )
        mapping[key] = _cc_catalog.HUD_UI_MGR_ENSURE_CEIL_IAT_IDENTITY
    return replace(
        indexes,
        storage_by_address=storage_by_address,
        storage_by_name=storage_by_name,
    )


def _hud_ui_mgr_ensure_remaining_indirect_candidate_bridges(
    expected: Sequence[Mapping[str, Any]],
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedExactIndirectStorageBridge]:
    """Prove the complete reviewed post-SetTextFmt indirect-call census.

    The returned rows recover receiver/storage provenance only.  They never
    turn a conditioned vtable value into an unconditional target identity.
    Candidate call offsets are discovered from the exact tail topology, so a
    retail-shaped source repair may shift code without weakening any receiver,
    relocation, slot, argument, cleanup, or result-use check.
    """
    from _recoil.call_contract.records import ReviewedExactIndirectStorageBridge
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
            "HUD remaining indirect census requires the exact registered "
            "nonempty EnsureHudLoaded candidate contribution"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != r"\?EnsureHudLoaded@HudUiMgr@@.*"
        or getattr(contribution_row, "name", "")
        != "HudUiMgr::EnsureHudLoaded"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD remaining indirect census requires the exact authored "
            "hud.cpp contribution identity"
        )

    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    if (
        indexes.storage_by_address.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
        != aggregate_identity
        or aggregate_identity in indexes.provider_ids
        or indexes.storage_by_address.get(_cc_catalog.HUD_LAYOUT_HW_ADDRESS)
        != _cc_catalog.HUD_LAYOUT_HW_STORAGE_IDENTITY
        or _cc_catalog.HUD_LAYOUT_HW_STORAGE_IDENTITY in indexes.provider_ids
        or indexes.storage_by_address.get(_cc_catalog.HUD_UI_MGR_TIMER_PANEL_ADDRESS)
        != _cc_catalog.HUD_UI_MGR_TIMER_PANEL_STORAGE_IDENTITY
        or _cc_catalog.HUD_UI_MGR_TIMER_PANEL_STORAGE_IDENTITY in indexes.provider_ids
    ):
        raise ValueError(
            "HUD remaining indirect census requires the exact authored HUD "
            "manager and HudLayoutHW storage roots"
        )

    # current offset, retail address/ordinal, slot, stack arguments, cleanup,
    # result use, lineage mode, root symbol, root identity/displacement,
    # expected exact root-load count, receiver mode/member displacement.
    specs = (
        ("objective-sensor-setpos", 0x67D, "0x4106f0", 58, 0x0C, 2, None, "unused", "direct", _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, aggregate_identity, 0x768, 1, "ecx", 0),
        ("objective-bar-visible", 0x6F9, "0x41076c", 66, 0x60, 1, None, "unused", "direct", _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, aggregate_identity, 0x978, 1, "ecx", 0),
        ("objective-description-setfont", 0x728, "0x4107a7", 67, 0x80, 7, None, "unused", "pointer", _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, aggregate_identity, 0x974, 1, "ecx", 0),
        ("objective-summary-setfont", 0x757, "0x4107c9", 68, 0x80, 7, None, "unused", "pointer", _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, aggregate_identity, 0x824, 1, "ecx", 0),
        ("reticle-invalidate", 0x7CF, "0x41083d", 74, 0x20, 0, None, "unused", "direct", _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, aggregate_identity, 0x364, 1, "ecx", 0),
        ("reticle-visible", 0x818, "0x41087d", 75, 0x60, 1, None, "unused", "direct", _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, aggregate_identity, 0x364, 1, "ecx", 0),
        ("layout-center-x", 0x842, "0x4108a1", 77, 0x64, 0, None, "ebp", "direct", _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL, _cc_catalog.HUD_LAYOUT_HW_STORAGE_IDENTITY, 0xEC, 1, "ecx", 0),
        ("layout-center-y", 0x852, "0x4108b0", 78, 0x68, 0, None, "stack", "direct", _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL, _cc_catalog.HUD_LAYOUT_HW_STORAGE_IDENTITY, 0xEC, 1, "ecx", 0),
        ("counter-setpos", 0x8A6, "0x4108f5", 80, 0x0C, 2, None, "unused", "pointer", _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, aggregate_identity, 0xBC8, 2, "ecx", 0),
        ("counter-setblt-source-clip", 0x8DE, "0x41092e", 81, 0x18, 2, None, "unused", "pointer", _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, aggregate_identity, 0xBC8, 2, "ecx", 0),
        ("counter-text-blank", 0x8F5, "0x41093e", 82, 0x74, 2, 8, "unused", "pointer", _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, aggregate_identity, 0xBC8, 1, "stack", 0),
        ("counter-bounds-blank", 0x903, "0x41094c", 83, 0x78, 0, None, "unused", "pointer", _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, aggregate_identity, 0xBC8, 1, "ecx", 0),
        ("counter-text-zero", 0x914, "0x410960", 84, 0x74, 3, 12, "unused", "pointer", _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, aggregate_identity, 0xBC8, 1, "stack", 0),
        ("counter-bounds-zero", 0x922, "0x41096a", 85, 0x78, 0, None, "unused", "pointer", _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, aggregate_identity, 0xBC8, 1, "ecx", 0),
        ("timer-setpos", 0x960, "0x4109a7", 87, 0x0C, 2, None, "unused", "pointer", _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, aggregate_identity, 0x4784, 2, "ecx", 0),
        ("timer-setblt-source-clip", 0x97E, "0x4109c1", 88, 0x18, 2, None, "unused", "pointer", _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, aggregate_identity, 0x4784, 1, "ecx", 0),
        ("timer-text-zero", 0x996, "0x4109d1", 89, 0x74, 2, 8, "unused", "pointer", _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, aggregate_identity, 0x4784, 1, "stack", 0),
        ("weapon-marker-invalidate", 0xB1A, "0x410be6", 106, 0x20, 0, None, "unused", "loop-root", _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, aggregate_identity, 0x1000, 1, "ecx", 0),
        ("weapon-slot-invalidate", 0xB34, "0x410bf9", 107, 0x20, 0, None, "unused", "loop-root", _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, aggregate_identity, 0xF44, 1, "ecx", 0),
        ("weapon-marker-visible", 0xB52, "0x410c32", 109, 0x60, 1, None, "unused", "loop-member", _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, aggregate_identity, 0xF44, 0, "ecx", 0xBC),
        ("weapon-slot-visible", 0xB5A, "0x410c3a", 110, 0x60, 1, None, "unused", "loop-member", _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, aggregate_identity, 0xF44, 0, "ecx", 0),
    )

    retail_start = address_value(caller_start)
    retail_end = address_value(caller_end_exclusive)
    retail_addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=0,
    )
    retail_address_counts: dict[int, int] = {}
    for retail_address in retail_addresses:
        if (
            retail_address is not None
            and retail_start <= retail_address < retail_end
        ):
            retail_address_counts[retail_address] = (
                retail_address_counts.get(retail_address, 0) + 1
            )
    retail_index_by_address = {
        retail_address: index
        for index, retail_address in enumerate(retail_addresses)
        if (
            retail_address is not None
            and retail_start <= retail_address < retail_end
            and retail_address_counts.get(retail_address) == 1
        )
    }
    retail_invocation_addresses: list[str] = []
    for index, (instruction, retail_address) in enumerate(
        zip(retail_instructions, retail_addresses)
    ):
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        if mnemonic == "call":
            if retail_address is None:
                raise ValueError(
                    "HUD remaining indirect census requires exact retail "
                    "invocation addresses"
                )
            retail_invocation_addresses.append(
                normalize_address(retail_address)
            )
            continue
        if mnemonic != "jmp":
            continue
        exact_branch = _cc_cfg._exact_local_direct_branch(
            instruction,
            instruction_index=index,
            instruction_addresses=retail_addresses,
            instruction_index_by_address=retail_index_by_address,
            source="bn",
            caller_start=retail_start,
            caller_end=retail_end,
        )
        if exact_branch is not None:
            continue
        operand = _cc_cfg._instruction_operand(instruction)
        address_matches = _cc_catalog.ADDRESS_RE.findall(operand)
        if (
            address_matches
            and retail_start
            <= address_value(address_matches[-1])
            < retail_end
        ):
            continue
        if (
            not address_matches
            and (
                operand.startswith("$")
                or operand.lower().startswith("short ")
                or _cc_catalog.DECORATED_RE.search(operand) is None
            )
        ):
            continue
        if retail_address is None:
            raise ValueError(
                "HUD remaining indirect census requires exact retail "
                "tail-invocation addresses"
            )
        retail_invocation_addresses.append(
            normalize_address(retail_address)
        )
    if any(
        retail_ordinal >= len(retail_invocation_addresses)
        or retail_invocation_addresses[retail_ordinal]
        != normalize_address(retail_address)
        for (
            _label,
            _current_offset,
            retail_address,
            retail_ordinal,
            *_rest,
        ) in specs
    ):
        raise ValueError(
            "HUD remaining indirect census rejects retail address/ordinal "
            "pair drift"
        )

    instructions = candidate.instructions
    addresses = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    counts: dict[int, int] = {}
    for address in addresses:
        if address is not None:
            counts[address] = counts.get(address, 0) + 1
    if any(
        address is None or counts.get(address) != 1
        for address in addresses
    ):
        raise ValueError(
            "HUD remaining indirect census requires unique COD offsets"
        )

    indirect_sites: list[tuple[int, int, str, int]] = []
    for index, (instruction, offset) in enumerate(
        zip(instructions, addresses)
    ):
        if (
            offset is None
            or _cc_cfg._instruction_mnemonic(instruction) != "call"
        ):
            continue
        expression, displacement = _cc_targets._memory_slot(
            _cc_cfg._instruction_operand(instruction)
        )
        base_match = re.search(
            r"\b(eax|ebx|ecx|edx|esi|edi|ebp)\b",
            expression,
        )
        if base_match is not None and displacement is not None:
            indirect_sites.append(
                (index, offset, base_match.group(1), displacement)
            )
    tail_slots = (0x74,) + tuple(spec[4] for spec in specs)
    slot_rows = tuple(row[3] for row in indirect_sites)
    matching_starts = [
        start_index
        for start_index in range(
            0,
            len(slot_rows) - len(tail_slots) + 1,
        )
        if slot_rows[
            start_index : start_index + len(tail_slots)
        ]
        == tail_slots
    ]
    if (
        len(matching_starts) != 1
        or matching_starts[0] + len(tail_slots) != len(indirect_sites)
    ):
        raise ValueError(
            "HUD remaining indirect census requires one exact complete "
            "post-SetTextFmt indirect tail with no extra indirect site"
        )
    reviewed_sites = indirect_sites[
        matching_starts[0] + 1 :
    ]
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    next_invocation_by_index = {
        instruction_index: (
            invocation_indices[position + 1]
            if position + 1 < len(invocation_indices)
            else len(instructions)
        )
        for position, instruction_index in enumerate(invocation_indices)
    }
    previous_invocation_by_index = {
        instruction_index: (
            invocation_indices[position - 1]
            if position
            else -1
        )
        for position, instruction_index in enumerate(invocation_indices)
    }

    def exact_expression(instruction: Instruction) -> str:
        operand = _cc_cfg._instruction_operand(instruction)
        source = operand.split(",", 1)[1] if "," in operand else operand
        expression = _cc_targets._exact_memory_expression(source)
        return re.sub(
            r"^(?:offset\s*flat:)?",
            "",
            expression,
            flags=re.IGNORECASE,
        )

    def exact_dir32_reference(
        instruction_index: int,
        *,
        symbol: str,
        displacement: int,
    ) -> None:
        offset = addresses[instruction_index]
        assert offset is not None
        instruction = instructions[instruction_index]
        body = bytes(int(value, 16) for value in instruction.bytes)
        if definition.data[offset : offset + len(body)] != body:
            raise ValueError(
                "HUD remaining indirect census rejects COD/object byte drift "
                f"at candidate offset 0x{offset:x}"
            )
        relocations = [
            relocation
            for relocation in definition.relocations
            if offset <= relocation.offset <= offset + len(body) - 4
        ]
        if (
            len(relocations) != 1
            or relocations[0].type != IMAGE_REL_I386_DIR32
            or relocations[0].symbol_name != symbol
            or struct.unpack_from(
                "<I",
                definition.data,
                relocations[0].offset,
            )[0]
            != displacement
            or not all(
                definition.relocation_mask[index]
                for index in range(
                    relocations[0].offset,
                    relocations[0].offset + 4,
                )
            )
            or any(
                definition.relocation_mask[index]
                for index in range(offset, offset + len(body))
                if not (
                    relocations[0].offset
                    <= index
                    < relocations[0].offset + 4
                )
            )
        ):
            raise ValueError(
                "HUD remaining indirect census requires one exact DIR32 "
                f"receiver/root reference at candidate offset 0x{offset:x}"
            )

    def exact_set_font_arguments(
        *,
        label: str,
        call_index: int,
        window_start: int,
    ) -> None:
        """Prove the repaired objective SetFont argument sources exactly."""
        if label not in {
            "objective-description-setfont",
            "objective-summary-setfont",
        }:
            return

        local_name, local_base = {
            "objective-description-setfont": (
                "_objectivedescriptionfont$",
                0x94,
            ),
            "objective-summary-setfont": (
                "_objectivesummaryfont$",
                0x94,
            ),
        }[label]
        push_rows = [
            index
            for index in range(window_start, call_index)
            if _cc_cfg._instruction_mnemonic(instructions[index]) == "push"
        ]
        if len(push_rows) != 7:
            raise ValueError(
                f"HUD remaining indirect {label} requires exactly seven "
                "ordered SetFont arguments"
            )

        def exact_unmasked_instruction(index: int) -> bytes:
            offset = addresses[index]
            if offset is None:
                raise ValueError(
                    f"HUD remaining indirect {label} requires exact COD "
                    "argument offsets"
                )
            encoded = bytes(
                int(value, 16) for value in instructions[index].bytes
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
                    f"HUD remaining indirect {label} rejects argument "
                    "instruction byte/relocation/mask drift"
                )
            return encoded

        first_push = instructions[push_rows[0]]
        if (
            exact_unmasked_instruction(push_rows[0]) != b"\x6a\x02"
            or re.fullmatch(
                r"push\s+(?:2|0x2)",
                first_push.raw_text.strip(),
                flags=re.IGNORECASE,
            )
            is None
        ):
            raise ValueError(
                f"HUD remaining indirect {label} requires SetFont mode 2 "
                "as its first pushed argument"
            )

        for push_index in push_rows[1:3]:
            instruction = instructions[push_index]
            if (
                exact_unmasked_instruction(push_index) != b"\x53"
                or re.fullmatch(
                    r"push\s+ebx",
                    instruction.raw_text.strip(),
                    flags=re.IGNORECASE,
                )
                is None
            ):
                raise ValueError(
                    f"HUD remaining indirect {label} requires two exact "
                    "zero EBX arguments after mode 2"
                )
        zero_definitions = [
            index
            for index in range(0, push_rows[1])
            if (
                tuple(
                    value.lower() for value in instructions[index].bytes
                )
                == ("33", "db")
                and re.fullmatch(
                    r"xor\s+ebx\s*,\s*ebx",
                    instructions[index].raw_text.strip(),
                    flags=re.IGNORECASE,
                )
                is not None
            )
        ]
        if (
            len(zero_definitions) != 1
            or exact_unmasked_instruction(zero_definitions[0]) != b"\x33\xdb"
        ):
            raise ValueError(
                f"HUD remaining indirect {label} rejects zero-argument "
                "origin or reaching-definition drift"
            )
        zero_index = zero_definitions[0]
        conflicting_ebx_writes = [
            index
            for index in range(zero_index + 1, push_rows[1])
            if (
                _cc_cfg._instruction_may_clobber_register(
                    instructions[index],
                    "ebx",
                )
                and not (
                    _cc_cfg._instruction_mnemonic(instructions[index]) == "pop"
                    and index + 2 < len(instructions)
                    and re.fullmatch(
                        r"add\s+esp\s*,\s*(?:132|0x84)",
                        instructions[index + 1].raw_text.strip(),
                        flags=re.IGNORECASE,
                    )
                    is not None
                    and _cc_cfg._exact_return_terminates(instructions[index + 2])
                )
            )
        ]
        if conflicting_ebx_writes:
            raise ValueError(
                f"HUD remaining indirect {label} rejects a reaching-path "
                "EBX clobber after its exact zero definition"
            )

        field_definitions: list[int] = []
        logical_displacements: list[int] = []
        for push_index in push_rows[3:]:
            push = instructions[push_index]
            register = _cc_cfg._instruction_operand(push).strip().lower()
            if (
                register
                not in {"eax", "ebx", "ecx", "edx", "esi", "edi", "ebp"}
                or len(exact_unmasked_instruction(push_index)) != 1
            ):
                raise ValueError(
                    f"HUD remaining indirect {label} requires register "
                    "consumption for each rectangle field"
                )
            definition_index = next(
                (
                    index
                    for index in range(
                        push_index - 1,
                        window_start - 1,
                        -1,
                    )
                    if _cc_cfg._instruction_may_clobber_register(
                        instructions[index],
                        register,
                    )
                ),
                -1,
            )
            if definition_index < 0:
                raise ValueError(
                    f"HUD remaining indirect {label} requires one reaching "
                    "rectangle-field definition per argument"
                )
            field_load = instructions[definition_index]
            operand = _cc_cfg._instruction_operand(field_load)
            if (
                _cc_cfg._instruction_mnemonic(field_load) != "mov"
                or "," not in operand
            ):
                raise ValueError(
                    f"HUD remaining indirect {label} rejects non-MOV "
                    "rectangle-field origin"
                )
            destination, source = (
                item.strip().lower()
                for item in operand.split(",", 1)
            )
            expression, displacement = _cc_targets._memory_slot(source)
            if (
                destination != register
                or local_name not in source
                or "esp" not in expression
                or displacement is None
            ):
                raise ValueError(
                    f"HUD remaining indirect {label} rejects rectangle-field "
                    "storage origin"
                )
            exact_unmasked_instruction(definition_index)
            prior_push_count = sum(
                1
                for earlier_push in push_rows
                if earlier_push < definition_index
            )
            field_definitions.append(definition_index)
            logical_displacements.append(
                displacement - 4 * prior_push_count
            )

        if (
            len(set(field_definitions)) != 4
            or logical_displacements
            != [
                local_base + 4,
                local_base + 12,
                local_base + 8,
                local_base,
            ]
        ):
            raise ValueError(
                f"HUD remaining indirect {label} rejects SetFont rectangle "
                "field order, origin, or unique consumption"
            )

    def exact_set_blt_source_clip_arguments(
        *,
        label: str,
        call_index: int,
        window_start: int,
    ) -> None:
        """Prove the repaired objective-counter and timer clip calls exactly."""
        if label == "counter-setblt-source-clip":
            clip_local = "_counterclip$"
            clip_name = "counterClip"
            clip_lea_bytes = b"\x8d\x54\x24\x38"
            call_register = "eax"
            call_bytes = b"\xff\x50\x18"
            vptr_bytes = b"\x8b\x01"
            vptr_position = "before-arguments"
        elif label == "timer-setblt-source-clip":
            clip_local = "_timerclip$"
            clip_name = "timerClip"
            clip_lea_bytes = b"\x8d\x44\x24\x50"
            call_register = "edx"
            call_bytes = b"\xff\x52\x18"
            vptr_bytes = b"\x8b\x11"
            vptr_position = "between-arguments"
        else:
            return
        diagnostic = f"HUD remaining indirect {label}"

        def exact_unmasked_instruction(index: int) -> bytes:
            offset = addresses[index]
            if offset is None:
                raise ValueError(
                    f"{diagnostic} requires exact COD argument offsets"
                )
            encoded = bytes(
                int(value, 16) for value in instructions[index].bytes
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
                    f"{diagnostic} rejects argument/vptr/call byte, "
                    "relocation, or mask drift"
                )
            return encoded

        push_rows = [
            index
            for index in range(window_start, call_index)
            if _cc_cfg._instruction_mnemonic(instructions[index]) == "push"
        ]
        if len(push_rows) != 2:
            raise ValueError(
                f"{diagnostic} requires exactly two ordered explicit arguments"
            )

        clip_push = instructions[push_rows[0]]
        clip_register = _cc_cfg._instruction_operand(clip_push).strip().lower()
        if (
            clip_register
            not in {"eax", "ebx", "ecx", "edx", "esi", "edi", "ebp"}
            or len(exact_unmasked_instruction(push_rows[0])) != 1
        ):
            raise ValueError(
                f"{diagnostic} requires one register-carried {clip_name} "
                "pointer argument"
            )
        clip_definition = next(
            (
                index
                for index in range(
                    push_rows[0] - 1,
                    window_start - 1,
                    -1,
                )
                if _cc_cfg._instruction_may_clobber_register(
                    instructions[index],
                    clip_register,
                )
            ),
            -1,
        )
        if clip_definition < 0:
            raise ValueError(
                f"{diagnostic} requires one reaching {clip_name} address "
                "definition"
            )
        clip_instruction = instructions[clip_definition]
        clip_operand = _cc_cfg._instruction_operand(clip_instruction)
        if (
            _cc_cfg._instruction_mnemonic(clip_instruction) != "lea"
            or "," not in clip_operand
        ):
            raise ValueError(
                f"{diagnostic} rejects a non-LEA {clip_name} pointer origin"
            )
        destination, source = (
            item.strip().lower()
            for item in clip_operand.split(",", 1)
        )
        expression, displacement = _cc_targets._memory_slot(source)
        if (
            destination != clip_register
            or clip_local not in source
            or "esp" not in expression
            or displacement != 0x94
            or exact_unmasked_instruction(clip_definition) != clip_lea_bytes
            or sum(
                1
                for index in range(clip_definition + 1, call_index)
                if (
                    _cc_cfg._instruction_mnemonic(instructions[index]) == "push"
                    and _cc_cfg._instruction_operand(instructions[index])
                    .strip()
                    .lower()
                    == clip_register
                )
            )
            != 1
        ):
            raise ValueError(
                f"{diagnostic} rejects {clip_name} storage origin or unique "
                "consumption"
            )

        zero_push = instructions[push_rows[1]]
        if (
            exact_unmasked_instruction(push_rows[1]) != b"\x53"
            or re.fullmatch(
                r"push\s+ebx",
                zero_push.raw_text.strip(),
                flags=re.IGNORECASE,
            )
            is None
        ):
            raise ValueError(
                f"{diagnostic} requires the exact zero source argument"
            )
        zero_definitions = [
            index
            for index in range(0, push_rows[1])
            if (
                tuple(
                    value.lower() for value in instructions[index].bytes
                )
                == ("33", "db")
                and re.fullmatch(
                    r"xor\s+ebx\s*,\s*ebx",
                    instructions[index].raw_text.strip(),
                    flags=re.IGNORECASE,
                )
                is not None
            )
        ]
        if (
            len(zero_definitions) != 1
            or exact_unmasked_instruction(zero_definitions[0]) != b"\x33\xdb"
            or any(
                _cc_cfg._instruction_may_clobber_register(
                    instructions[index],
                    "ebx",
                )
                and not (
                    _cc_cfg._instruction_mnemonic(instructions[index]) == "pop"
                    and index + 2 < len(instructions)
                    and re.fullmatch(
                        r"add\s+esp\s*,\s*(?:132|0x84)",
                        instructions[index + 1].raw_text.strip(),
                        flags=re.IGNORECASE,
                    )
                    is not None
                    and _cc_cfg._exact_return_terminates(instructions[index + 2])
                )
                for index in range(
                    zero_definitions[0] + 1,
                    push_rows[1],
                )
            )
        ):
            raise ValueError(
                f"{diagnostic} rejects the zero argument's reaching definition"
            )

        call = instructions[call_index]
        if (
            exact_unmasked_instruction(call_index) != call_bytes
            or re.fullmatch(
                r"call\s+(?:dword\s+(?:ptr\s+)?)?"
                rf"\[{call_register}\+(?:24|0x18)\]",
                call.raw_text.strip(),
                flags=re.IGNORECASE,
            )
            is None
        ):
            raise ValueError(
                f"{diagnostic} requires the exact targetless "
                f"{call_register.upper()} slot-0x18 call bytes"
            )

        vptr_rows = [
            index
            for index in range(window_start, call_index)
            if (
                _cc_cfg._instruction_mnemonic(instructions[index]) == "mov"
                and re.fullmatch(
                    rf"mov\s+{call_register}\s*,\s*"
                    r"(?:dword\s+(?:ptr\s+)?)?\[ecx\]",
                    instructions[index].raw_text.strip(),
                    flags=re.IGNORECASE,
                )
                is not None
            )
        ]
        if (
            len(vptr_rows) != 1
            or exact_unmasked_instruction(vptr_rows[0]) != vptr_bytes
            or (
                vptr_position == "before-arguments"
                and vptr_rows[0] >= push_rows[0]
            )
            or (
                vptr_position == "between-arguments"
                and not push_rows[0] < vptr_rows[0] < push_rows[1]
            )
        ):
            raise ValueError(
                f"{diagnostic} rejects the receiver-derived vptr load or "
                "ordered call topology"
            )

    bridges: dict[str, ReviewedExactIndirectStorageBridge] = {}
    previous_indirect_index = indirect_sites[matching_starts[0]][0]
    for spec, reviewed_site in zip(specs, reviewed_sites):
        (
            label,
            _current_offset,
            retail_address,
            retail_ordinal,
            slot,
            argument_count,
            cleanup_bytes,
            result_use,
            lineage_mode,
            root_symbol,
            root_identity,
            root_displacement,
            root_load_count,
            receiver_mode,
            member_displacement,
        ) = spec
        call_index, call_offset, call_register, actual_slot = reviewed_site
        expected_row = (
            expected[retail_ordinal]
            if len(expected) > retail_ordinal
            else None
        )
        if label.startswith("timer-"):
            desired_storage = (
                f"load({_cc_catalog.HUD_UI_MGR_TIMER_PANEL_STORAGE_IDENTITY})"
            )
        elif lineage_mode == "direct":
            desired_storage = (
                f"load({_cc_receiver_instructions._abstract_with_displacement(root_identity, root_displacement)})"
            )
        elif lineage_mode == "pointer":
            desired_storage = (
                "load("
                f"load({_cc_receiver_instructions._abstract_with_displacement(root_identity, root_displacement)})"
                ")"
            )
        else:
            desired_storage = (
                expected_row.get("storage_identity", "")
                if isinstance(expected_row, Mapping)
                else ""
            )
        expected_contract = {
            "ordinal": retail_ordinal,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": desired_storage,
            "slot_displacement": slot,
            "cleanup_bytes": cleanup_bytes,
        }
        if (
            not isinstance(expected_row, Mapping)
            or expected_row != expected_contract
            or actual_slot != slot
        ):
            raise ValueError(
                f"HUD remaining indirect {label} requires exact reviewed "
                f"retail ordinal {retail_ordinal}, empty target, storage, "
                "slot, and cleanup"
            )

        window = range(
            previous_invocation_by_index.get(
                call_index,
                previous_indirect_index,
            )
            + 1,
            call_index,
        )
        root_expression = (
            f"{root_symbol}+{root_displacement}"
        )
        root_rows = [
            index
            for index in window
            if (
                _cc_cfg._instruction_mnemonic(instructions[index])
                in {"mov", "lea"}
                and exact_expression(instructions[index])
                in {
                    root_expression,
                    f"{root_symbol}+0x{root_displacement:x}",
                }
            )
        ]
        load_rows = [
            index
            for index in root_rows
            if (
                _cc_cfg._instruction_mnemonic(instructions[index]) == "mov"
                and "OFFSET FLAT:" not in instructions[index].raw_text.upper()
            )
        ]
        address_rows = [
            index
            for index in root_rows
            if "OFFSET FLAT:" in instructions[index].raw_text.upper()
        ]
        if lineage_mode == "direct":
            vptr_rows = [
                index
                for index in load_rows
                if _cc_cfg._instruction_operand(instructions[index])
                .split(",", 1)[0]
                .strip()
                .lower()
                == call_register
            ]
            receiver_rows = [
                index
                for index in address_rows
                if _cc_cfg._instruction_operand(instructions[index])
                .split(",", 1)[0]
                .strip()
                .lower()
                == receiver_mode
                and (
                    len(vptr_rows) != 1
                    or index > vptr_rows[0]
                )
            ]
            if (
                len(load_rows) != root_load_count
                or len(vptr_rows) != 1
                or len(receiver_rows) != 1
                or vptr_rows[0] >= call_index
                or receiver_rows[0] >= call_index
            ):
                raise ValueError(
                    f"HUD remaining indirect {label} rejects receiver/vptr "
                    "lineage drift"
                )
            anchor_index = min(vptr_rows[0], receiver_rows[0])
            for index in (vptr_rows[0], receiver_rows[0]):
                exact_dir32_reference(
                    index,
                    symbol=root_symbol,
                    displacement=root_displacement,
                )
        elif lineage_mode == "pointer":
            if len(load_rows) != root_load_count or address_rows:
                raise ValueError(
                    f"HUD remaining indirect {label} rejects aggregate "
                    "pointer-load count or address-form drift"
                )
            loaded_registers = {
                _cc_cfg._instruction_operand(instructions[index])
                .split(",", 1)[0]
                .strip()
                .lower()
                for index in load_rows
            }
            vptr_rows = []
            for index in window:
                instruction = instructions[index]
                if _cc_cfg._instruction_mnemonic(instruction) != "mov":
                    continue
                operand = _cc_cfg._instruction_operand(instruction)
                if "," not in operand:
                    continue
                destination, source = (
                    item.strip().lower()
                    for item in operand.split(",", 1)
                )
                expression, displacement = _cc_targets._memory_slot(source)
                if (
                    destination == call_register
                    and displacement in {None, 0}
                    and expression in loaded_registers
                ):
                    vptr_rows.append(index)
            if len(vptr_rows) != 1:
                raise ValueError(
                    f"HUD remaining indirect {label} requires one exact "
                    "receiver-derived vptr load"
                )
            push_registers = {
                _cc_cfg._instruction_operand(instructions[index]).strip().lower()
                for index in window
                if _cc_cfg._instruction_mnemonic(instructions[index]) == "push"
            }
            if (
                receiver_mode == "ecx"
                and "ecx" not in loaded_registers
                and not any(
                    re.fullmatch(
                        r"mov\s+ecx\s*,\s*"
                        + re.escape(register),
                        instructions[index].raw_text.strip(),
                        flags=re.IGNORECASE,
                    )
                    for register in loaded_registers
                    for index in window
                )
            ) or (
                receiver_mode == "stack"
                and not (loaded_registers & push_registers)
            ):
                raise ValueError(
                    f"HUD remaining indirect {label} rejects receiver "
                    "argument lineage"
                )
            for index in load_rows:
                exact_dir32_reference(
                    index,
                    symbol=root_symbol,
                    displacement=root_displacement,
                )
            anchor_index = min((*load_rows, vptr_rows[0]))
        else:
            expression, displacement = _cc_targets._memory_slot(
                _cc_cfg._instruction_operand(instructions[call_index])
            )
            del expression
            loop_base_rows = address_rows
            if lineage_mode == "loop-root":
                if len(loop_base_rows) != root_load_count:
                    raise ValueError(
                        f"HUD remaining indirect {label} requires one exact "
                        "loop-base relocation"
                    )
                for index in loop_base_rows:
                    exact_dir32_reference(
                        index,
                        symbol=root_symbol,
                        displacement=root_displacement,
                    )
                anchor_index = loop_base_rows[0]
            else:
                anchor_index = previous_indirect_index + 1
            vptr_rows = [
                index
                for index in window
                if (
                    _cc_cfg._instruction_mnemonic(instructions[index]) == "mov"
                    and _cc_cfg._instruction_operand(instructions[index])
                    .split(",", 1)[0]
                    .strip()
                    .lower()
                    == call_register
                    and _cc_targets._memory_slot(
                        _cc_cfg._instruction_operand(instructions[index])
                        .split(",", 1)[1]
                    )[1]
                    in {None, member_displacement}
                    and "esi"
                    in _cc_targets._memory_slot(
                        _cc_cfg._instruction_operand(instructions[index])
                        .split(",", 1)[1]
                    )[0]
                )
            ]
            receiver_rows = [
                index
                for index in window
                if re.fullmatch(
                    (
                        r"(?:mov|lea)\s+ecx\s*,\s*"
                        r"(?:dword\s+(?:ptr\s+)?)?\[?esi"
                        + (
                            rf"\+{member_displacement}\]?"
                            if member_displacement
                            else r"\]?"
                        )
                    ),
                    instructions[index].raw_text.strip(),
                    flags=re.IGNORECASE,
                )
            ]
            if len(vptr_rows) != 1 or len(receiver_rows) != 1:
                raise ValueError(
                    f"HUD remaining indirect {label} rejects weapon-slot "
                    "loop receiver/vptr topology"
                )
            anchor_index = vptr_rows[0]

        actual_argument_count = sum(
            1
            for index in range(
                previous_invocation_by_index.get(call_index, anchor_index)
                + 1,
                call_index,
            )
            if _cc_cfg._instruction_mnemonic(instructions[index]) == "push"
        )
        if (
            actual_argument_count != argument_count
            or _cc_cfg._cleanup_after(instructions, call_index) != cleanup_bytes
            or any(
                definition.relocation_mask[index]
                for index in range(
                    call_offset,
                    call_offset
                    + len(instructions[call_index].bytes),
                )
            )
        ):
            raise ValueError(
                f"HUD remaining indirect {label} rejects actual argument "
                "count/dataflow, cleanup, or call-byte masking"
            )
        exact_set_font_arguments(
            label=label,
            call_index=call_index,
            window_start=(
                previous_invocation_by_index.get(
                    call_index,
                    anchor_index,
                )
                + 1
            ),
        )
        exact_set_blt_source_clip_arguments(
            label=label,
            call_index=call_index,
            window_start=(
                previous_invocation_by_index.get(
                    call_index,
                    anchor_index,
                )
                + 1
            ),
        )

        next_invocation = next_invocation_by_index.get(
            call_index,
            len(instructions),
        )
        result_rows = []
        if result_use in {"ebp", "stack"}:
            for instruction in instructions[
                call_index + 1 : next_invocation
            ]:
                if _cc_cfg._instruction_mnemonic(instruction) != "mov":
                    continue
                operand = _cc_cfg._instruction_operand(instruction)
                if "," not in operand:
                    continue
                destination, source = (
                    item.strip().lower()
                    for item in operand.rsplit(",", 1)
                )
                if source != "eax":
                    continue
                if (
                    result_use == "ebp"
                    and destination == "ebp"
                ) or (
                    result_use == "stack"
                    and re.search(r"\[(?:e|r)sp(?:[+\-][^\]]+)?\]", destination)
                ):
                    result_rows.append(instruction)
        if result_use in {"ebp", "stack"} and len(result_rows) != 1:
            raise ValueError(
                f"HUD remaining indirect {label} requires exact result use "
                f"{result_use}"
            )
        if result_use == "unused":
            result_read = False
            for instruction in instructions[
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
                        result_read = True
                        break
                if _cc_cfg._instruction_may_clobber_register(instruction, "eax"):
                    break
            if result_read:
                raise ValueError(
                    f"HUD remaining indirect {label} rejects unexpected "
                    "result consumption"
                )

        bridge_address = normalize_address(call_offset)
        if bridge_address in bridges:
            raise ValueError(
                "HUD remaining indirect census rejects duplicate call offset "
                f"{bridge_address}"
            )
        bridges[bridge_address] = ReviewedExactIndirectStorageBridge(
            register=call_register,
            storage_identity=desired_storage,
            slot_displacement=slot,
            assembly_source="cod",
        )
        previous_indirect_index = call_index

    if len(bridges) != len(specs):
        raise ValueError(
            "HUD remaining indirect census rejects omitted or duplicate "
            "reviewed entries"
        )
    return bridges


def _hud_triplet_ensure_capacity_tu_local_candidate_bridge(
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
    """Expose one current-source helper call against its inlined retail body.

    The current VC5 diagnostic defines ``HudUiTripletEnsureCapacity`` as a
    TU-local external function in the caller's COFF object and emits one direct
    REL32 call from ``HudUiTriplet::AddEntry``.  Retail has no helper body or
    callee identity: its allocation, copy, destruction, delete, and capacity
    update are physically inlined in the reviewed 0x40e590 body.

    The reviewed caller identity is therefore a candidate-only comparison
    sentinel, not a claimed retail target.  The bridge leaves the direct call
    row in the candidate contract so ordinary ordinal comparison reports the
    real call-order/form/target/cleanup divergence.
    """
    matching_instructions: list[Instruction] = []
    matching_symbols: set[str] = set()
    for instruction in candidate.instructions:
        if _cc_cfg._instruction_mnemonic(instruction) not in {"call", "jmp"}:
            continue
        operand = _cc_cfg._instruction_operand(instruction).strip()
        if _cc_catalog.HUD_TRIPLET_ENSURE_CAPACITY_TU_LOCAL_COD_RE.fullmatch(operand):
            matching_instructions.append(instruction)
            matching_symbols.add(operand)
            continue
        if "HudUiTripletEnsureCapacity" in operand:
            raise ValueError(
                "HUD triplet ensure-capacity TU-local bridge accepts only the "
                "exact helper name, 00_hud.cpp TU marker, decimal "
                "discriminator, and fastcall function signature"
            )
    if not matching_instructions:
        return {}
    if len(matching_instructions) != 1 or len(matching_symbols) != 1:
        raise ValueError(
            "HUD triplet ensure-capacity TU-local bridge requires one exact "
            "candidate helper call and decoration"
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
        caller_identity != _cc_catalog.HUD_TRIPLET_ENSURE_CAPACITY_CALLER_IDENTITY
        or normalized_start != _cc_catalog.HUD_TRIPLET_ENSURE_CAPACITY_CALLER_START
        or normalized_end
        != _cc_catalog.HUD_TRIPLET_ENSURE_CAPACITY_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(normalized_start) != caller_identity
        or caller_identity in indexes.provider_ids
        or definition is None
        or definition.symbol != _cc_catalog.HUD_TRIPLET_ENSURE_CAPACITY_CALLER_SYMBOL
        or not expected
        or any(
            row.get("target_identity") == caller_identity
            for row in expected
        )
    ):
        raise ValueError(
            "HUD triplet ensure-capacity TU-local bridge requires the exact "
            "reviewed authored 0x40e590 caller identity, extent, symbol, "
            "non-empty retail contract, and no retail sentinel target"
        )
    collision_names = {
        cod_symbol,
        coff_symbol,
    }
    if (
        any(name in indexes.by_candidate_name for name in collision_names)
        or any(name in bridge_names for name in collision_names)
        or any(name in indexes.storage_by_name for name in collision_names)
        or any(
            name in compiler_generated_bridges
            for name in collision_names
        )
    ):
        raise ValueError(
            "HUD triplet ensure-capacity TU-local bridge has a conflicting "
            "ordinary candidate, retail, storage, or compiler-generated "
            "identity"
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
            "HUD triplet ensure-capacity TU-local bridge requires exactly one "
            "same-object external function definition with the exact VC5 "
            "TU-local signature and no undefined or case-folded alias"
        )

    instruction = matching_instructions[0]
    raw_offset = _cc_cfg._source_instruction_address(instruction)
    if (
        _cc_cfg._instruction_mnemonic(instruction) != "call"
        or tuple(instruction.bytes[:1]) != ("e8",)
        or not raw_offset
        or address_value(raw_offset)
        != _cc_catalog.HUD_TRIPLET_ENSURE_CAPACITY_CALL_OFFSET
    ):
        raise ValueError(
            "HUD triplet ensure-capacity TU-local bridge requires the exact "
            "reviewed offset-bearing direct E8 CALL topology"
        )

    call_relocation_offset = _cc_catalog.HUD_TRIPLET_ENSURE_CAPACITY_CALL_OFFSET + 1
    helper_casefold_names = {
        cod_symbol.casefold(),
        coff_symbol.casefold(),
    }
    helper_references = [
        relocation
        for relocation in definition.relocations
        if relocation.symbol_name.casefold() in helper_casefold_names
    ]
    relocations_at_call = [
        relocation
        for relocation in definition.relocations
        if relocation.offset == call_relocation_offset
    ]
    field_end = call_relocation_offset + 4
    if (
        len(helper_references) != 1
        or len(relocations_at_call) != 1
        or helper_references[0] is not relocations_at_call[0]
        or helper_references[0].symbol_name != coff_symbol
        or helper_references[0].type != IMAGE_REL_I386_REL32
        or field_end > len(definition.data)
        or field_end > len(definition.relocation_mask)
        or definition.data[
            _cc_catalog.HUD_TRIPLET_ENSURE_CAPACITY_CALL_OFFSET:
            call_relocation_offset
        ]
        != b"\xe8"
        or struct.unpack_from(
            "<I",
            definition.data,
            call_relocation_offset,
        )[0]
        != 0
        or not all(
            definition.relocation_mask[index]
            for index in range(call_relocation_offset, field_end)
        )
    ):
        raise ValueError(
            "HUD triplet ensure-capacity TU-local bridge requires one exact "
            "zero-addend fully masked E8 REL32 COFF relocation to the "
            "same-object helper"
        )

    provisional_bridges = dict(compiler_generated_bridges)
    provisional_bridges[cod_symbol] = caller_identity
    provisional = _cc_extraction.extract_invocation_contract(
        candidate.instructions,
        source="cod",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge_names=bridge_names,
        compiler_generated_bridges=provisional_bridges,
        local_control_flow_indices=candidate.local_control_flow_indices,
        local_control_flow_targets=candidate.local_control_flow_targets,
    )
    bridged_rows = [
        row
        for row in provisional
        if row.get("target_identity") == caller_identity
    ]
    if (
        len(bridged_rows) != 1
        or bridged_rows[0].get("form") != "call"
        or bridged_rows[0].get("dispatch") != "direct"
        or bridged_rows[0].get("identity_kind") != "direct"
        or bridged_rows[0].get("storage_identity") != ""
        or bridged_rows[0].get("slot_displacement") is not None
        or bridged_rows[0].get("cleanup_bytes") is not None
    ):
        raise ValueError(
            "HUD triplet ensure-capacity TU-local bridge does not produce "
            "exactly one direct candidate call to the comparison sentinel"
        )
    return {
        cod_symbol: caller_identity,
    }
