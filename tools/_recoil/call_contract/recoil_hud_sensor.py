"""Recoil call-contract recoil hud sensor evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import extraction as _cc_extraction
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract import instructions as _cc_instructions
from _recoil.call_contract import recoil_hud_objectives as _cc_recoil_hud_objectives
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        IdentityIndexes,
        ReviewedAbsoluteStorageLoadBridge,
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
    CoffRelocation,
    Instruction,
)
from _recoil.lib.authored_icf import exact_selected_target_membership
from _recoil.lib.progress import ProgressDocument, address_value, normalize_address
from _recoil.lib.tooling import REPO_ROOT


def _hud_ui_mgr_sensor_shield_meter_affine_storage_bridges(
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, str],
    dict[str, ReviewedStaticStorageReferenceBridge],
]:
    """Bridge only SetShieldMessageRatio's reviewed meter pointer chain."""
    from _recoil.call_contract.records import ReviewedStaticStorageReferenceBridge
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_CEIL_CALLER_START:
        return {}, {}

    caller_symbol_id = (
        _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_CEIL_CALLER_IDENTITY.removeprefix(
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
        == _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_CEIL_CALLER_START
    ]
    if (
        caller_identity
        != _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_CEIL_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_CEIL_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(normalized_start) != caller_identity
        or caller_identity in indexes.provider_ids
        or not isinstance(caller_row, Mapping)
        or caller_row.get("binary") != "recoil"
        or caller_row.get("kind") != "function"
        or caller_row.get("pipeline_class") != "authored"
        or caller_row.get("ownership_state") != "primary-owned"
        or caller_row.get("address") != normalized_start
        or caller_row.get("end_exclusive")
        != _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_CEIL_CALLER_END_EXCLUSIVE
        or caller_row.get("extent_state") != "known"
        or caller_row.get("size") != 0x140
        or caller_row.get("navigation_name")
        != "HudUiMgrSensor::SetShieldMessageRatio"
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
        != _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_CEIL_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
        or caller is None
        or caller.symbol
        != _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_CEIL_CALLER_SYMBOL
        or not caller.data
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
            "HUD sensor shield-meter affine bridge requires the exact "
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
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD sensor shield-meter affine bridge requires the exact "
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
        and relationship.get("symbol_id")
        == _cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID
    ]
    reference = (
        aggregate_storage.get("reference")
        if isinstance(aggregate_storage, Mapping)
        else None
    )
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
        or _cc_catalog.HUD_SHIELD_LAYOUT_POINTER_ADDRESS
        in indexes.storage_by_address
        or _cc_catalog.HUD_SHIELD_LAYOUT_POINTER_IDENTITY in indexes.provider_ids
        or address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
        + _cc_catalog.HUD_SHIELD_LAYOUT_POINTER_DISPLACEMENT
        != address_value(_cc_catalog.HUD_SHIELD_LAYOUT_POINTER_ADDRESS)
        or _cc_catalog.HUD_SHIELD_LAYOUT_POINTER_DISPLACEMENT
        + _cc_catalog.HUD_SHIELD_LAYOUT_POINTER_ACCESS_WIDTH
        > _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
    ):
        raise ValueError(
            "HUD sensor shield-meter affine bridge requires the exact "
            "reviewed aggregate symbol, storage, target, owner, and bounded "
            "uncatalogued pointer-field authority"
        )

    retail_offsets = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(normalized_start),
    )
    retail_by_address = {
        offset: instruction
        for offset, instruction in zip(
            retail_offsets,
            retail_instructions,
        )
        if offset is not None
    }
    retail_fixed = {
        _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_METER_RETAIL_LOAD: (
            "mov",
            ("8b", "0d", "ac", "6d", "4e", "00"),
        ),
        _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_METER_RETAIL_ADD: (
            "add",
            ("81", "c1", "7c", "03", "00", "00"),
        ),
        _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_METER_RETAIL_VPTR: (
            "mov",
            ("8b", "11"),
        ),
        _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_METER_RETAIL_CALL: (
            "call",
            ("ff", "52", "20"),
        ),
    }
    if any(
        address not in retail_by_address
        or _cc_cfg._instruction_mnemonic(retail_by_address[address]) != mnemonic
        or retail_by_address[address].bytes != body
        for address, (mnemonic, body) in retail_fixed.items()
    ):
        raise ValueError(
            "HUD sensor shield-meter affine bridge requires the exact "
            "retail load/add/EDX-vptr/slot-0x20 chain"
        )
    retail_call_index = retail_instructions.index(
        retail_by_address[
            _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_METER_RETAIL_CALL
        ]
    )
    if (
        len(
            [
                instruction
                for instruction in retail_instructions[:retail_call_index]
                if _cc_cfg._instruction_mnemonic(instruction) == "call"
            ]
        )
        != _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_METER_RETAIL_ORDINAL
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(
                retail_by_address[
                    _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_METER_RETAIL_CALL
                ]
            )
        )
        not in {"edx+32", "edx+0x20"}
        or _cc_cfg._cleanup_after(retail_instructions, retail_call_index)
        is not None
    ):
        raise ValueError(
            "HUD sensor shield-meter affine bridge requires retail ordinal-5, "
            "EDX slot 0x20, ECX receiver, and no caller cleanup"
        )

    candidate_offsets = _cc_cfg._instruction_runtime_addresses(
        candidate.instructions,
        source="cod",
        caller_start=0,
    )
    aggregate_relocations = [
        relocation
        for relocation in caller.relocations
        if relocation.symbol_name
        == _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
    ]
    if (
        not aggregate_relocations
        or len({row.offset for row in aggregate_relocations})
        != len(aggregate_relocations)
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in caller.defined_external_data
    ):
        raise ValueError(
            "HUD sensor shield-meter affine bridge requires one or more "
            "distinct candidate aggregate+0xedc relocation owners and one "
            "undefined aggregate symbol"
        )

    register_codes = {
        "eax": 0,
        "ecx": 1,
        "edx": 2,
        "ebx": 3,
        "esp": 4,
        "ebp": 5,
        "esi": 6,
        "edi": 7,
    }

    def candidate_body_matches(
        index: int,
        allowed_relocations: Sequence[CoffRelocation] = (),
    ) -> bool:
        offset = candidate_offsets[index]
        if offset is None:
            return False
        try:
            encoded = bytes(
                int(value, 16)
                for value in candidate.instructions[index].bytes
            )
        except (TypeError, ValueError):
            return False
        allowed_mask = {
            position
            for row in allowed_relocations
            for position in range(row.offset, row.offset + 4)
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

    expected_expressions = {
        (
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
            f"+{_cc_catalog.HUD_SHIELD_LAYOUT_POINTER_DISPLACEMENT}"
        ),
        (
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
            f"+0x{_cc_catalog.HUD_SHIELD_LAYOUT_POINTER_DISPLACEMENT:x}"
        ),
    }

    def encoded_body(index: int) -> bytes:
        return bytes(
            int(value, 16)
            for value in candidate.instructions[index].bytes
        )

    load_anchors: list[tuple[int, str, str]] = []
    for relocation in aggregate_relocations:
        if (
            relocation.type != IMAGE_REL_I386_DIR32
            or relocation.offset + 4 > len(caller.data)
            or struct.unpack_from("<I", caller.data, relocation.offset)[0]
            != _cc_catalog.HUD_SHIELD_LAYOUT_POINTER_DISPLACEMENT
            or not all(
                caller.relocation_mask[position]
                for position in range(
                    relocation.offset,
                    relocation.offset + 4,
                )
            )
        ):
            raise ValueError(
                "HUD sensor shield-meter affine bridge requires every "
                "candidate aggregate relocation to be exact DIR32 "
                "aggregate+0xedc with a complete mask"
            )
        relocation_owners = [
            (index, instruction, offset)
            for index, (instruction, offset) in enumerate(
                zip(candidate.instructions, candidate_offsets)
            )
            if (
                offset is not None
                and offset <= relocation.offset
                < offset + len(instruction.bytes)
            )
        ]
        if len(relocation_owners) != 1:
            raise ValueError(
                "HUD sensor shield-meter affine bridge requires every "
                "aggregate+0xedc relocation to have one unique candidate "
                "instruction owner"
            )
        load_index, load_instruction, load_offset = relocation_owners[0]
        load_operands = [
            item.strip()
            for item in _cc_cfg._instruction_operand(load_instruction).split(",", 1)
        ]
        try:
            load_encoded = encoded_body(load_index)
        except (TypeError, ValueError) as exc:
            raise ValueError(
                "HUD sensor shield-meter affine bridge requires exact "
                "candidate pointer-load instruction bytes"
            ) from exc
        pointer_register = (
            load_operands[0].lower() if len(load_operands) == 2 else ""
        )
        candidate_expression = (
            _cc_targets._exact_memory_expression(load_operands[1])
            if len(load_operands) == 2
            else ""
        )
        encoded_load_is_exact = False
        if (
            pointer_register in register_codes
            and pointer_register != "esp"
            and candidate_expression in expected_expressions
            and _cc_cfg._instruction_mnemonic(load_instruction) == "mov"
        ):
            if (
                pointer_register == "eax"
                and len(load_encoded) == 5
                and load_encoded[0] == 0xA1
                and relocation.offset == load_offset + 1
            ):
                encoded_load_is_exact = True
            elif (
                len(load_encoded) == 6
                and load_encoded[0] == 0x8B
                and load_encoded[1] & 0xC7 == 0x05
                and (load_encoded[1] >> 3) & 0x7
                == register_codes[pointer_register]
                and relocation.offset == load_offset + 2
            ):
                encoded_load_is_exact = True
        if (
            not encoded_load_is_exact
            or not candidate_body_matches(
                load_index,
                (relocation,),
            )
        ):
            raise ValueError(
                "HUD sensor shield-meter affine bridge requires every "
                "relocation-owned aggregate+0xedc reference to be an exact "
                "candidate pointer MOV"
            )
        load_anchors.append(
            (
                load_index,
                pointer_register,
                candidate_expression,
            )
        )
    if len({load_index for load_index, _, _ in load_anchors}) != len(
        load_anchors
    ):
        raise ValueError(
            "HUD sensor shield-meter affine bridge rejects duplicate "
            "aggregate relocations sharing one pointer-load owner"
        )
    candidate_pointer_reference_indices = {
        index
        for index, instruction in enumerate(candidate.instructions)
        if (
            _cc_cfg._instruction_mnemonic(instruction) == "mov"
            and len(
                _cc_cfg._instruction_operand(instruction).split(",", 1)
            )
            == 2
            and _cc_targets._exact_memory_expression(
                _cc_cfg._instruction_operand(instruction).split(",", 1)[1]
            )
            in expected_expressions
        )
    }
    if candidate_pointer_reference_indices != {
        load_index for load_index, _, _ in load_anchors
    }:
        raise ValueError(
            "HUD sensor shield-meter affine bridge requires every exact "
            "candidate aggregate+0xedc pointer reference to have one matching "
            "well-formed aggregate relocation owner"
        )

    def split_operands(instruction: Instruction) -> tuple[str, str]:
        parts = [
            item.strip()
            for item in _cc_cfg._instruction_operand(instruction).split(",", 1)
        ]
        if len(parts) != 2:
            return "", ""
        return parts[0].lower(), _cc_targets._exact_memory_expression(parts[1])

    chains: list[
        tuple[int, int, int, int, int, str, str, str]
    ] = []
    for load_index, pointer_register, candidate_expression in load_anchors:
        lea_candidates: list[tuple[int, str]] = []
        for index in range(load_index + 1, len(candidate.instructions)):
            instruction = candidate.instructions[index]
            destination, source = split_operands(instruction)
            if (
                _cc_cfg._instruction_mnemonic(instruction) != "lea"
                or destination not in register_codes
                or destination in {"esp", pointer_register}
                or source
                not in {
                    (
                        f"{pointer_register}"
                        f"+{_cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_METER_DISPLACEMENT}"
                    ),
                    (
                        f"{pointer_register}"
                        f"+0x{_cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_METER_DISPLACEMENT:x}"
                    ),
                }
                or not candidate_body_matches(index)
            ):
                continue
            encoded = encoded_body(index)
            if (
                len(encoded) != 6
                or encoded[0] != 0x8D
                or encoded[1] >> 6 != 0x2
                or encoded[1] & 0x7 != register_codes[pointer_register]
                or (encoded[1] >> 3) & 0x7
                != register_codes[destination]
                or struct.unpack_from("<I", encoded, 2)[0]
                != _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_METER_DISPLACEMENT
                or any(
                    _cc_cfg._instruction_may_clobber_register(
                        candidate.instructions[between],
                        pointer_register,
                    )
                    for between in range(load_index + 1, index)
                )
            ):
                continue
            lea_candidates.append((index, destination))

        for lea_index, meter_register in lea_candidates:
            receiver_candidates = [
                index
                for index in range(
                    lea_index + 1,
                    len(candidate.instructions),
                )
                if (
                    _cc_cfg._instruction_mnemonic(candidate.instructions[index])
                    == "mov"
                    and split_operands(candidate.instructions[index])
                    == ("ecx", meter_register)
                    and candidate_body_matches(index)
                    and encoded_body(index)
                    == bytes(
                        (
                            0x8B,
                            0xC0
                            | (
                                register_codes["ecx"] << 3
                            )
                            | register_codes[meter_register],
                        )
                    )
                    and not any(
                        _cc_cfg._instruction_may_clobber_register(
                            candidate.instructions[between],
                            meter_register,
                        )
                        for between in range(lea_index + 1, index)
                    )
                )
            ]
            for receiver_index in receiver_candidates:
                vptr_candidates: list[tuple[int, str]] = []
                for index in range(
                    receiver_index + 1,
                    len(candidate.instructions),
                ):
                    destination, source = split_operands(
                        candidate.instructions[index]
                    )
                    if (
                        _cc_cfg._instruction_mnemonic(candidate.instructions[index])
                        != "mov"
                        or destination not in register_codes
                        or destination in {"esp", "ecx", meter_register}
                        or source != meter_register
                        or not candidate_body_matches(index)
                    ):
                        continue
                    encoded = encoded_body(index)
                    if (
                        len(encoded) == 2
                        and encoded[0] == 0x8B
                        and encoded[1] >> 6 == 0
                        and encoded[1] & 0x7
                        == register_codes[meter_register]
                        and (encoded[1] >> 3) & 0x7
                        == register_codes[destination]
                        and not any(
                            _cc_cfg._instruction_may_clobber_register(
                                candidate.instructions[between],
                                meter_register,
                            )
                            for between in range(
                                receiver_index + 1,
                                index,
                            )
                        )
                        and not any(
                            _cc_cfg._instruction_may_clobber_register(
                                candidate.instructions[between],
                                "ecx",
                            )
                            for between in range(
                                receiver_index + 1,
                                index,
                            )
                        )
                    ):
                        vptr_candidates.append((index, destination))
                for vptr_index, vptr_register in vptr_candidates:
                    for call_index in range(
                        vptr_index + 1,
                        len(candidate.instructions),
                    ):
                        instruction = candidate.instructions[call_index]
                        encoded = encoded_body(call_index)
                        if (
                            _cc_cfg._instruction_mnemonic(instruction) != "call"
                            or _cc_targets._exact_memory_expression(
                                _cc_cfg._instruction_operand(instruction)
                            )
                            not in {
                                (
                                    f"{vptr_register}"
                                    f"+{_cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_METER_SLOT}"
                                ),
                                (
                                    f"{vptr_register}"
                                    f"+0x{_cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_METER_SLOT:x}"
                                ),
                            }
                            or not candidate_body_matches(call_index)
                            or len(encoded) not in {3, 6}
                            or encoded[0] != 0xFF
                            or (encoded[1] >> 3) & 0x7 != 2
                            or encoded[1] & 0x7
                            != register_codes[vptr_register]
                            or (
                                len(encoded) == 3
                                and (
                                    encoded[1] >> 6 != 1
                                    or encoded[2]
                                    != _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_METER_SLOT
                                )
                            )
                            or (
                                len(encoded) == 6
                                and (
                                    encoded[1] >> 6 != 2
                                    or struct.unpack_from(
                                        "<I",
                                        encoded,
                                        2,
                                    )[0]
                                    != _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_METER_SLOT
                                )
                            )
                            or any(
                                _cc_cfg._instruction_may_clobber_register(
                                    candidate.instructions[between],
                                    vptr_register,
                                )
                                for between in range(
                                    vptr_index + 1,
                                    call_index,
                                )
                            )
                            or any(
                                _cc_cfg._instruction_may_clobber_register(
                                    candidate.instructions[between],
                                    "ecx",
                                )
                                for between in range(
                                    vptr_index + 1,
                                    call_index,
                                )
                            )
                            or _cc_cfg._cleanup_after(
                                candidate.instructions,
                                call_index,
                            )
                            is not None
                            or call_index
                            in candidate.local_control_flow_indices
                        ):
                            continue
                        chains.append(
                            (
                                load_index,
                                lea_index,
                                receiver_index,
                                vptr_index,
                                call_index,
                                meter_register,
                                vptr_register,
                                candidate_expression,
                            )
                        )

    if len(chains) != 1:
        raise ValueError(
            "HUD sensor shield-meter affine bridge requires one unique "
            "connected candidate +0x37c meter/ECX/vptr/slot-0x20 chain "
            "across all exact aggregate+0xedc pointer loads"
        )

    (
        load_index,
        lea_index,
        receiver_index,
        vptr_index,
        call_index,
        _meter_register,
        _vptr_register,
        candidate_expression,
    ) = chains[0]
    candidate_bridge = ReviewedStaticStorageReferenceBridge(
        aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
        displacement=_cc_catalog.HUD_SHIELD_LAYOUT_POINTER_DISPLACEMENT,
        access_width=_cc_catalog.HUD_SHIELD_LAYOUT_POINTER_ACCESS_WIDTH,
        storage_identity=_cc_catalog.HUD_SHIELD_LAYOUT_POINTER_IDENTITY,
    )
    structural_instructions = tuple(
        candidate.instructions[index]
        for index in (
            load_index,
            lea_index,
            receiver_index,
            vptr_index,
            call_index,
        )
    )
    structural_contract = _cc_extraction.extract_invocation_contract(
        structural_instructions,
        source="cod",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        reviewed_static_storage_reference_bridges={
            candidate_expression: candidate_bridge
        },
    )
    if structural_contract != [
        {
            "ordinal": 0,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": (
                "load(storage:recoil:data:0x4e5ed0+0xedc+0x37c)"
            ),
            "slot_displacement": _cc_catalog.HUD_UI_MGR_SENSOR_SHIELD_METER_SLOT,
            "cleanup_bytes": None,
        }
    ]:
        raise ValueError(
            "HUD sensor shield-meter affine bridge cannot derive the exact "
            "candidate meter virtual-call provenance"
        )

    return (
        {
            _cc_catalog.HUD_SHIELD_LAYOUT_POINTER_ADDRESS: (
                _cc_catalog.HUD_SHIELD_LAYOUT_POINTER_IDENTITY
            )
        },
        {
            expression: candidate_bridge
            for _load_index, _register, expression in load_anchors
        },
    )


def _hud_ui_mgr_objective_show_sensor_overlay_candidate_vptr_bridges(
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
    """Bridge only 0x411900's exact sensor-overlay SetVisible call."""
    from _recoil.call_contract.records import (
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedVptrStorageBridge,
    )
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CALLER_START:
        return {}, {}

    # Preserve and reuse the cumulative WSI-034/035 caller, source,
    # contribution, preceding-call, and exact COFF-prefix authority.
    _cc_recoil_hud_objectives._hud_ui_mgr_objective_show_desc_candidate_vptr_bridges(
        expected,
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )

    overlay_storage = (
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_SENSOR_OVERLAY_STORAGE_IDENTITY
    )
    exact_expected = {
        "ordinal": 2,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": overlay_storage,
        "slot_displacement": (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_SENSOR_OVERLAY_SLOT_DISPLACEMENT
        ),
        "cleanup_bytes": None,
    }
    if len(expected) <= 2 or expected[2] != exact_expected:
        observed = expected[2] if len(expected) > 2 else None
        raise ValueError(
            "HUD objective Show sensor-overlay vptr bridge requires the "
            "immutable retail ordinal-2 embedded-overlay/slot-0x60/"
            f"callee-cleanup contract: observed={observed!r}"
        )

    caller = candidate.caller_definition
    exact_window = bytes.fromhex(
        "a1 e0 0c 00 00 "
        "83 c4 08 "
        "b9 e0 0c 00 00 "
        "6a 00 "
        "ff 50 60"
    )
    window_start = _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_SENSOR_OVERLAY_LOAD_OFFSET
    window_end = window_start + len(exact_window)
    window_relocation_specs = (
        (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_SENSOR_OVERLAY_LOAD_OFFSET + 1,
            _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_OVERLAY_DISPLACEMENT,
        ),
        (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_SENSOR_OVERLAY_RECEIVER_OFFSET + 1,
            _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_OVERLAY_DISPLACEMENT,
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
            "HUD objective Show sensor-overlay vptr bridge rejects exact "
            "0x120 caller symbol/window, paired DIR32 target/addend/order, "
            "external identity, or window relocation-mask drift"
        )

    expected_instructions = (
        (0x40, "mov", ("a1", "e0", "0c", "00", "00")),
        (0x45, "add", ("83", "c4", "08")),
        (0x48, "mov", ("b9", "e0", "0c", "00", "00")),
        (0x4D, "push", ("6a", "00")),
        (0x4F, "call", ("ff", "50", "60")),
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
            "HUD objective Show sensor-overlay vptr bridge requires the "
            "exact candidate vptr-load/prior-cleanup/receiver/argument/call "
            "listing and instruction bytes"
        )

    aggregate = _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
    aggregate_storage = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    load = _cc_cfg._instruction_operand(
        instruction_by_offset[
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_SENSOR_OVERLAY_LOAD_OFFSET
        ]
    ).split(",", 1)
    receiver = _cc_cfg._instruction_operand(
        instruction_by_offset[
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_SENSOR_OVERLAY_RECEIVER_OFFSET
        ]
    ).split(",", 1)
    argument = _cc_cfg._instruction_operand(instruction_by_offset[0x4D])
    call = instruction_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_SENSOR_OVERLAY_CALL_OFFSET
    ]
    overlay_expressions = {
        (
            f"{aggregate}+"
            f"{_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_OVERLAY_DISPLACEMENT}"
        ),
        (
            f"{aggregate}+0x"
            f"{_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_OVERLAY_DISPLACEMENT:x}"
        ),
    }
    if (
        len(load) != 2
        or load[0].strip().lower() != "eax"
        or _cc_targets._exact_memory_expression(load[1]) not in overlay_expressions
        or len(receiver) != 2
        or receiver[0].strip().lower() != "ecx"
        or _cc_targets._exact_memory_expression(receiver[1])
        not in (
            overlay_expressions
            | {
                f"OFFSETFLAT:{expression}"
                for expression in overlay_expressions
            }
        )
        or argument.strip().lower() not in {"0", "0x0"}
        or _cc_cfg._instruction_mnemonic(call) != "call"
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(call))
        not in {"eax+96", "eax+0x60"}
        or not (
            index_by_offset[
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_SENSOR_OVERLAY_LOAD_OFFSET
            ]
            < index_by_offset[
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_SENSOR_OVERLAY_RECEIVER_OFFSET
            ]
            < index_by_offset[0x4D]
            < index_by_offset[
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_SENSOR_OVERLAY_CALL_OFFSET
            ]
        )
    ):
        raise ValueError(
            "HUD objective Show sensor-overlay vptr bridge rejects exact "
            "EAX-vptr/ECX-receiver/zero-argument/slot reaching-definition "
            "drift"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    desc_call_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_DESC_CALL_OFFSET
    ]
    call_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_SENSOR_OVERLAY_CALL_OFFSET
    ]
    reviewed_window_indices = {
        index_by_offset[offset]
        for offset, _, _ in expected_instructions
    }
    if (
        len(invocation_indices) <= 2
        or tuple(invocation_indices[1:3])
        != (desc_call_index, call_index)
        or _cc_cfg._cleanup_after(candidate.instructions, desc_call_index)
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_CLEANUP_BYTES
        or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
        or any(
            _cc_cfg._instruction_mnemonic(candidate.instructions[index]).startswith(
                "j"
            )
            for index in range(desc_call_index + 1, call_index)
        )
        or any(
            target in reviewed_window_indices
            for targets in candidate.local_control_flow_targets.values()
            for target in targets
        )
        or candidate.local_control_flow_indices
    ):
        raise ValueError(
            "HUD objective Show sensor-overlay vptr bridge rejects call "
            "ordinal, prior/callee cleanup, or alternate local "
            "control-flow drift"
        )

    absolute_bridges = {
        normalize_address(
            hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_SENSOR_OVERLAY_LOAD_OFFSET)
        ): ReviewedAbsoluteStorageLoadBridge(
            register="eax",
            aggregate_symbol=aggregate,
            displacement=(
                _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_OVERLAY_DISPLACEMENT
            ),
            access_width=4,
            storage_identity=aggregate_storage,
        )
    }
    vptr_bridges = {
        normalize_address(
            hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_SENSOR_OVERLAY_CALL_OFFSET)
        ): ReviewedVptrStorageBridge(
            register="eax",
            provenance=overlay_storage,
            storage_identity=overlay_storage,
            slot_displacement=(
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_SHOW_SENSOR_OVERLAY_SLOT_DISPLACEMENT
            ),
            identity_kind="virtual-slot",
        )
    }
    return absolute_bridges, vptr_bridges


def _hud_ui_mgr_objective_begin_sensor_candidate_vptr_bridges(
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
    """Bridge only 0x411a20's exact embedded sensor-rect SetVisible call."""
    from _recoil.call_contract.records import (
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedVptrStorageBridge,
    )
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_CALLER_START:
        return {}, {}

    # Reuse WSI-040's complete caller/source/manifest authority, exact 0xa0
    # object prefix, seven preceding relocations, and ordinal-0/1 call proof.
    _cc_recoil_hud_objectives._hud_ui_mgr_objective_begin_desc_candidate_vptr_bridges(
        expected,
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )

    sensor_storage = _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_STORAGE_IDENTITY
    exact_expected = {
        "ordinal": 2,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": sensor_storage,
        "slot_displacement": _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SLOT_DISPLACEMENT,
        "cleanup_bytes": None,
    }
    if len(expected) <= 2 or expected[2] != exact_expected:
        observed = expected[2] if len(expected) > 2 else None
        raise ValueError(
            "HUD objective Begin sensor-rect vptr bridge requires the "
            "immutable retail ordinal-2 embedded-sensor/slot-0x60/"
            f"no-cleanup contract: observed={observed!r}"
        )

    caller = candidate.caller_definition
    exact_window = bytes.fromhex(
        "a1 68 07 00 00 "
        "b9 68 07 00 00 "
        "6a 00 "
        "ff 50 60"
    )
    window_start = _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_LOAD_OFFSET
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
        (0x44, 0x974),
        (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_LOAD_OFFSET + 1,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_DISPLACEMENT,
        ),
        (
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_RECEIVER_OFFSET + 1,
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_DISPLACEMENT,
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
            "HUD objective Begin sensor-rect vptr bridge rejects exact 0xa0 "
            "caller symbol/window, nine DIR32 target/addend/order rows, "
            "external identity, or extended relocation-mask drift"
        )

    expected_instructions = (
        (0x4F, "mov", ("a1", "68", "07", "00", "00")),
        (0x54, "mov", ("b9", "68", "07", "00", "00")),
        (0x59, "push", ("6a", "00")),
        (0x5B, "call", ("ff", "50", "60")),
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
            "HUD objective Begin sensor-rect vptr bridge requires the exact "
            "candidate vptr-load/receiver/zero/call listing and instruction "
            "bytes"
        )

    aggregate = _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
    aggregate_storage = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    sensor_expressions = {
        (
            f"{aggregate}+"
            f"{_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_DISPLACEMENT}"
        ),
        (
            f"{aggregate}+0x"
            f"{_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_DISPLACEMENT:x}"
        ),
    }
    load = _cc_cfg._instruction_operand(
        instruction_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_LOAD_OFFSET]
    ).split(",", 1)
    receiver = _cc_cfg._instruction_operand(
        instruction_by_offset[
            _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_RECEIVER_OFFSET
        ]
    ).split(",", 1)
    argument = instruction_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_ARGUMENT_OFFSET
    ]
    call = instruction_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_CALL_OFFSET]
    if (
        len(load) != 2
        or load[0].strip().lower() != "eax"
        or _cc_targets._exact_memory_expression(load[1]) not in sensor_expressions
        or len(receiver) != 2
        or receiver[0].strip().lower() != "ecx"
        or _cc_targets._exact_memory_expression(receiver[1])
        not in (
            sensor_expressions
            | {
                f"OFFSETFLAT:{expression}"
                for expression in sensor_expressions
            }
        )
        or _cc_cfg._instruction_mnemonic(argument) != "push"
        or _cc_cfg._instruction_operand(argument).strip().lower() not in {"0", "0x0"}
        or _cc_cfg._instruction_mnemonic(call) != "call"
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(call))
        not in {"eax+96", "eax+0x60"}
        or not (
            index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_LOAD_OFFSET]
            < index_by_offset[
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_RECEIVER_OFFSET
            ]
            < index_by_offset[
                _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_ARGUMENT_OFFSET
            ]
            < index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_CALL_OFFSET]
        )
    ):
        raise ValueError(
            "HUD objective Begin sensor-rect vptr bridge rejects exact "
            "EAX-vptr/ECX-receiver/zero-argument/slot reaching-definition "
            "drift"
        )


    load_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_LOAD_OFFSET
    ]
    receiver_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_RECEIVER_OFFSET
    ]
    call_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_CALL_OFFSET
    ]
    if (
        any(
            _cc_instructions.may_clobber_register(candidate.instructions[index], "eax")
            for index in range(load_index + 1, call_index)
        )
        or any(
            _cc_instructions.may_clobber_register(candidate.instructions[index], "ecx")
            for index in range(receiver_index + 1, call_index)
        )
    ):
        raise ValueError(
            "HUD objective Begin sensor-rect vptr bridge rejects EAX vptr "
            "or ECX receiver clobber on the reviewed phase-2 path"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    desc_call_index = index_by_offset[
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_DESC_CALL_OFFSET
    ]
    reviewed_window_indices = {
        index_by_offset[offset]
        for offset, _, _ in expected_instructions
    }
    if (
        len(invocation_indices) <= 2
        or tuple(invocation_indices[:3])
        != (
            index_by_offset[_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_CALL_OFFSET],
            desc_call_index,
            call_index,
        )
        or _cc_cfg._cleanup_after(candidate.instructions, desc_call_index) is not None
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
            "HUD objective Begin sensor-rect vptr bridge rejects call "
            "ordinal, cleanup, straight-line uniqueness, or alternate local "
            "control-flow drift"
        )

    absolute_bridges = {
        normalize_address(
            hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_LOAD_OFFSET)
        ): ReviewedAbsoluteStorageLoadBridge(
            register="eax",
            aggregate_symbol=aggregate,
            displacement=_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_DISPLACEMENT,
            access_width=4,
            storage_identity=aggregate_storage,
        )
    }
    vptr_bridges = {
        normalize_address(
            hex(_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_CALL_OFFSET)
        ): ReviewedVptrStorageBridge(
            register="eax",
            provenance=sensor_storage,
            storage_identity=sensor_storage,
            slot_displacement=_cc_catalog.HUD_UI_MGR_OBJECTIVE_BEGIN_SLOT_DISPLACEMENT,
            identity_kind="virtual-slot",
        )
    }
    return absolute_bridges, vptr_bridges


def _require_hud_ui_mgr_ensure_sensor_center_authority(
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> None:
    """Require the reviewed caller, aggregate, and two typed virtual callees."""
    from _recoil.call_contract.records import StorageContainer
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START:
        return
    caller_addresses = sorted(
        address
        for address, identity in indexes.by_address.items()
        if identity == caller_identity
    )
    caller_name_rows = [
        (name, identity)
        for name, identity in indexes.by_candidate_name.items()
        if name.casefold() == _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_SYMBOL.casefold()
    ]
    if (
        caller_identity != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_END_EXCLUSIVE
        or caller_addresses != [_cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START]
        or caller_identity in indexes.provider_ids
        or caller_name_rows
        not in (
            [],
            [
                (
                    _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_SYMBOL,
                    _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_IDENTITY,
                )
            ],
        )
    ):
        raise ValueError(
            "HUD EnsureHudLoaded sensor-center bridge requires the exact "
            "reviewed authored caller identity, extent, and symbol"
        )

    symbols = document.collection("symbols")
    caller_symbol_id = caller_identity.removeprefix("symbol:")
    caller_symbol = symbols.get(caller_symbol_id)
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
        != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START
        or normalize_address(
            str(caller_symbol.get("end_exclusive", ""))
        )
        != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_END_EXCLUSIVE
        or caller_symbol.get("size")
        != address_value(_cc_catalog.HUD_UI_MGR_ENSURE_CALLER_END_EXCLUSIVE)
        - address_value(_cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START)
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
        != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
    ):
        raise ValueError(
            "HUD EnsureHudLoaded sensor-center bridge requires one exact "
            "unaliased authored caller and resolved source edge"
        )

    target_id = "recoil:vc5-target:hud_404ca0_415ab0_authored_order"
    target = document.collection("verification_targets").get(target_id)
    registration = (
        target.get("registration") if isinstance(target, Mapping) else None
    )
    contribution_rows: dict[str, tuple[Mapping[str, Any], Mapping[str, Any]]] = {}
    if isinstance(registration, Mapping):
        for contribution in registration.get(
            "translation_unit_function_order",
            [],
        ):
            if not isinstance(contribution, Mapping):
                continue
            for row in contribution.get("functions", []):
                if not isinstance(row, Mapping):
                    continue
                address = normalize_address(str(row.get("address", "")))
                if address in {
                    _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START,
                    _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_GET_CENTER_X_ADDRESS,
                    _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_GET_CENTER_Y_ADDRESS,
                }:
                    if address in contribution_rows:
                        raise ValueError(
                            "HUD EnsureHudLoaded sensor-center bridge rejects "
                            "duplicate typed target rows"
                        )
                    contribution_rows[address] = (contribution, row)
    if (
        not isinstance(target, Mapping)
        or target.get("binary") != "recoil"
        or target.get("kind") != "vc5"
        or target.get("name") != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_TARGET_NAME
        or not isinstance(registration, Mapping)
        or registration.get("manifest_path")
        != str(
            _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_TARGET_MANIFEST.relative_to(REPO_ROOT)
        ).replace("\\", "/")
        or registration.get("binary") != "recoil"
        or registration.get("name")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_TARGET_NAME
        or registration.get("check_translation_unit_function_order")
        is not True
        or set(contribution_rows)
        != {
            _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START,
            _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_GET_CENTER_X_ADDRESS,
            _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_GET_CENTER_Y_ADDRESS,
        }
        or target_id not in caller_symbol.get(
            "verification_target_ids",
            [],
        )
    ):
        raise ValueError(
            "HUD EnsureHudLoaded sensor-center bridge requires one exact "
            "current HUD target with typed caller and center-query rows"
        )
    expected_target_rows = {
        _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START: {
            "symbol": "",
            "symbol_regex": r"\?EnsureHudLoaded@HudUiMgr@@.*",
            "name": "HudUiMgr::EnsureHudLoaded",
        },
        _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_GET_CENTER_X_ADDRESS: {
            "symbol": _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_GET_CENTER_X_SYMBOL,
            "symbol_regex": None,
            "name": "HudUiWidget::GetCenterX",
        },
        _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_GET_CENTER_Y_ADDRESS: {
            "symbol": _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_GET_CENTER_Y_SYMBOL,
            "symbol_regex": None,
            "name": "HudUiWidget::GetCenterY",
        },
    }
    for address, expected_row in expected_target_rows.items():
        contribution, row = contribution_rows[address]
        if (
            contribution.get("source_from")
            != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
            or contribution.get("order_scope") != "authored"
            or row.get("symbol") != expected_row["symbol"]
            or row.get("symbol_regex") != expected_row["symbol_regex"]
            or row.get("name") != expected_row["name"]
            or row.get("pipeline_class") != "authored"
            or row.get("authored_order_role") != "authored-body"
            or row.get("required_presence") is not True
            or row.get("full_order_gate") is not True
        ):
            raise ValueError(
                "HUD EnsureHudLoaded sensor-center bridge requires exact "
                "authored hud.cpp caller/GetCenterX/GetCenterY target rows"
            )

    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    aggregate_symbol = symbols.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID)
    aggregate_storage = document.collection("storage_contributions").get(
        _cc_catalog.HUD_UI_MGR_AGGREGATE_STORAGE_ID
    )
    matching_containers = [
        row
        for row in indexes.storage_containers
        if row.identity == aggregate_identity
    ]
    if (
        not isinstance(aggregate_symbol, Mapping)
        or aggregate_symbol.get("binary") != "recoil"
        or aggregate_symbol.get("kind") != "data"
        or aggregate_symbol.get("disposition") != "authored"
        or normalize_address(str(aggregate_symbol.get("address", "")))
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
        or aggregate_symbol.get("navigation_name")
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_NAME
        or aggregate_symbol.get("storage_contribution_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_STORAGE_ID]
        or aggregate_symbol.get("verification_target_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_ID]
        or aggregate_symbol.get("extent_state") != "unknown"
        or not isinstance(aggregate_storage, Mapping)
        or aggregate_storage.get("binary") != "recoil"
        or aggregate_storage.get("kind") != "data-symbol"
        or aggregate_storage.get("overlap") != "none"
        or aggregate_storage.get("symbol_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID]
        or not isinstance(aggregate_storage.get("reference"), Mapping)
        or normalize_address(
            str(aggregate_storage["reference"].get("address", ""))
        )
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
        or indexes.storage_by_address.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
        != aggregate_identity
        or matching_containers
        != [
            StorageContainer(
                address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS),
                address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
                + _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE,
                aggregate_identity,
            )
        ]
        or address_value(_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_PANEL_ADDRESS)
        != address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
        + _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_PANEL_DISPLACEMENT
    ):
        raise ValueError(
            "HUD EnsureHudLoaded sensor-center bridge requires the exact "
            "typed HUD aggregate storage and +0xc24 sensor-panel receiver"
        )

    callee_specs = (
        (
            _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_GET_CENTER_X_IDENTITY,
            _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_GET_CENTER_X_ADDRESS,
            _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_GET_CENTER_X_SYMBOL,
            "HudUiWidget::GetCenterX",
            "recoil:anchor:battlesport.hud.huduiwidget-getcenterx",
        ),
        (
            _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_GET_CENTER_Y_IDENTITY,
            _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_GET_CENTER_Y_ADDRESS,
            _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_GET_CENTER_Y_SYMBOL,
            "HudUiWidget::GetCenterY",
            "recoil:anchor:battlesport.hud.huduiwidget-getcentery",
        ),
    )
    for identity, address, candidate_name, navigation_name, anchor_id in (
        callee_specs
    ):
        symbol_id = identity.removeprefix("symbol:")
        symbol = symbols.get(symbol_id)
        symbol_trace = (
            symbol.get("source_traceability")
            if isinstance(symbol, Mapping)
            else None
        )
        symbol_edges = (
            symbol_trace.get("source_edges")
            if isinstance(symbol_trace, Mapping)
            else None
        )
        matching_addresses = sorted(
            candidate_address
            for candidate_address, candidate_identity in (
                indexes.by_address.items()
            )
            if candidate_identity == identity
        )
        matching_names = [
            (name, candidate_identity)
            for name, candidate_identity in indexes.by_candidate_name.items()
            if name.casefold() == candidate_name.casefold()
        ]
        if (
            not isinstance(symbol, Mapping)
            or _cc_identity._symbol_identity(symbol_id, symbol) != identity
            or symbol.get("binary") != "recoil"
            or symbol.get("kind") != "function"
            or symbol.get("pipeline_class") != "authored"
            or symbol.get("extent_state") != "known"
            or normalize_address(str(symbol.get("address", ""))) != address
            or symbol.get("size") != 0x40
            or symbol.get("navigation_name") != navigation_name
            or symbol.get("logical_identity_key") not in {None, ""}
            or symbol.get("icf_fold_status") not in {None, ""}
            or bool(symbol.get("logical_aliases"))
            or not isinstance(symbol_trace, Mapping)
            or symbol_trace.get("state") != "resolved"
            or symbol_trace.get("reason_code") not in {None, ""}
            or not isinstance(symbol_edges, list)
            or len(symbol_edges) != 1
            or not isinstance(symbol_edges[0], Mapping)
            or symbol_edges[0].get("relation") != "defines"
            or symbol_edges[0].get("anchor_id") != anchor_id
            or symbol_edges[0].get("emission_context")
            != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
            or matching_addresses != [address]
            or matching_names != [(candidate_name, identity)]
            or identity in indexes.provider_ids
        ):
            raise ValueError(
                "HUD EnsureHudLoaded sensor-center bridge requires unique "
                f"typed authored {navigation_name} callee identity"
            )


def _hud_ui_mgr_ensure_sensor_center_retail_guard(
    instructions: Sequence[Instruction],
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> None:
    """Guard the exact retail sensor-panel GetCenterX/GetCenterY pair."""
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START:
        return
    _require_hud_ui_mgr_ensure_sensor_center_authority(
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
        address_value(_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_RETAIL_LOAD_X_ADDRESS): (
            ("8b", "15", "f4", "6a", "4e", "00"),
            r"mov\s+edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[?0x4e6af4\]?",
        ),
        address_value(_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_RETAIL_RECEIVER_X_ADDRESS): (
            ("b9", "f4", "6a", "4e", "00"),
            r"mov\s+ecx\s*,\s*0x4e6af4",
        ),
        address_value(_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_RETAIL_CALL_X_ADDRESS): (
            ("ff", "52", "64"),
            r"call\s+(?:dword\s+(?:ptr\s+)?)?"
            r"\[edx(?:\+100|\+0x64)\]",
        ),
        address_value(_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_RETAIL_RESULT_X_ADDRESS): (
            ("89", "45", "d4"),
            r"mov\s+(?:dword\s+(?:ptr\s+)?)?"
            r"\[ebp(?:-44|-0x2c)\]\s*,\s*eax",
        ),
        address_value(_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_RETAIL_LOAD_Y_ADDRESS): (
            ("a1", "f4", "6a", "4e", "00"),
            r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[?0x4e6af4\]?",
        ),
        address_value(_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_RETAIL_RECEIVER_Y_ADDRESS): (
            ("b9", "f4", "6a", "4e", "00"),
            r"mov\s+ecx\s*,\s*0x4e6af4",
        ),
        address_value(_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_RETAIL_CALL_Y_ADDRESS): (
            ("ff", "50", "68"),
            r"call\s+(?:dword\s+(?:ptr\s+)?)?"
            r"\[eax(?:\+104|\+0x68)\]",
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
            "HUD EnsureHudLoaded sensor-center retail guard requires the "
            "exact 0x4e6af4 EDX-slot-0x64/EAX-slot-0x68 pair"
        )
    ordered_addresses = list(fixed_rows)
    ordered_indices = [index_by_address[address] for address in ordered_addresses]
    call_x = address_value(_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_RETAIL_CALL_X_ADDRESS)
    call_y = address_value(_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_RETAIL_CALL_Y_ADDRESS)
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
        or bounded_calls != [call_x, call_y]
        or sum(
            _cc_cfg._instruction_mnemonic(instruction) == "call"
            for instruction in instructions[: index_by_address[call_x]]
        )
        != _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CALL_X_ORDINAL
        or sum(
            _cc_cfg._instruction_mnemonic(instruction) == "call"
            for instruction in instructions[: index_by_address[call_y]]
        )
        != _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CALL_Y_ORDINAL
        or _cc_cfg._cleanup_after(instructions, index_by_address[call_x]) is not None
        or _cc_cfg._cleanup_after(instructions, index_by_address[call_y]) is not None
    ):
        raise ValueError(
            "HUD EnsureHudLoaded sensor-center retail guard rejects missing, "
            "extra, reordered, aliased, or caller-cleanup pair rows"
        )


def _hud_ui_mgr_ensure_sensor_center_candidate_absolute_load_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedAbsoluteStorageLoadBridge]:
    """Bridge only the exact candidate sensor-panel center-query pair."""
    from _recoil.call_contract.records import ReviewedAbsoluteStorageLoadBridge
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START:
        return {}
    _require_hud_ui_mgr_ensure_sensor_center_authority(
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    definition = candidate.caller_definition
    target = candidate.target
    target_rows: dict[str, list[tuple[Any, Any]]] = {
        _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START: [],
        _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_GET_CENTER_X_ADDRESS: [],
        _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_GET_CENTER_Y_ADDRESS: [],
    }
    for contribution in getattr(
        target,
        "translation_unit_function_order",
        (),
    ):
        for row in getattr(contribution, "functions", ()):
            address = normalize_address(str(getattr(row, "address", "")))
            if address in target_rows:
                target_rows[address].append((contribution, row))
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
        or any(len(rows) != 1 for rows in target_rows.values())
    ):
        raise ValueError(
            "HUD EnsureHudLoaded sensor-center candidate bridge requires the "
            "nonempty candidate COMDAT and current HUD target"
        )
    expected_target_rows = {
        _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START: (
            "",
            r"\?EnsureHudLoaded@HudUiMgr@@.*",
            "HudUiMgr::EnsureHudLoaded",
        ),
        _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_GET_CENTER_X_ADDRESS: (
            _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_GET_CENTER_X_SYMBOL,
            None,
            "HudUiWidget::GetCenterX",
        ),
        _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_GET_CENTER_Y_ADDRESS: (
            _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_GET_CENTER_Y_SYMBOL,
            None,
            "HudUiWidget::GetCenterY",
        ),
    }
    for address, (symbol, symbol_regex, name) in expected_target_rows.items():
        contribution, row = target_rows[address][0]
        if (
            getattr(contribution, "source_from", "")
            != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
            or getattr(contribution, "order_scope", "") != "authored"
            or getattr(row, "symbol", "") != symbol
            or getattr(row, "symbol_regex", None) != symbol_regex
            or getattr(row, "name", "") != name
            or getattr(row, "pipeline_class", "") != "authored"
            or getattr(row, "authored_order_role", "") != "authored-body"
            or not bool(getattr(row, "required_presence", False))
            or not bool(getattr(row, "full_order_gate", False))
        ):
            raise ValueError(
                "HUD EnsureHudLoaded sensor-center candidate bridge requires "
                "exact caller/GetCenterX/GetCenterY hud.cpp target rows"
            )

    expected_pair = (
        {
            "ordinal": _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CALL_X_ORDINAL,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_STORAGE_IDENTITY,
            "slot_displacement": _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CALL_X_SLOT,
            "cleanup_bytes": None,
        },
        {
            "ordinal": _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CALL_Y_ORDINAL,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_STORAGE_IDENTITY,
            "slot_displacement": _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CALL_Y_SLOT,
            "cleanup_bytes": None,
        },
    )
    if (
        len(expected) <= _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CALL_Y_ORDINAL
        or tuple(
            expected[ordinal]
            for ordinal in (
                _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CALL_X_ORDINAL,
                _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CALL_Y_ORDINAL,
            )
        )
        != expected_pair
    ):
        raise ValueError(
            "HUD EnsureHudLoaded sensor-center candidate bridge requires "
            "exact independently retail-derived ordinal-24/25 contracts"
        )

    start = 0
    fixed = {
        "load_x": start + _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CANDIDATE_LOAD_X_OFFSET,
        "receiver_x": (
            start + _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CANDIDATE_RECEIVER_X_OFFSET
        ),
        "call_x": start + _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CANDIDATE_CALL_X_OFFSET,
        "result_x": (
            start + _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CANDIDATE_RESULT_X_OFFSET
        ),
        "load_y": start + _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CANDIDATE_LOAD_Y_OFFSET,
        "receiver_y": (
            start + _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CANDIDATE_RECEIVER_Y_OFFSET
        ),
        "call_y": start + _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CANDIDATE_CALL_Y_OFFSET,
    }
    aggregate_pattern = re.escape(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
    baseline_fixed = fixed
    baseline_expected_rows = {
        baseline_fixed["load_x"]: (
            ("8b", "15", "24", "0c", "00", "00"),
            rf"mov\s+edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            rf"{aggregate_pattern}\+(?:3108|0xc24)",
        ),
        baseline_fixed["receiver_x"]: (
            ("b9", "24", "0c", "00", "00"),
            rf"mov\s+ecx\s*,\s*(?:offset\s+(?:flat:)?)?"
            rf"{aggregate_pattern}\+(?:3108|0xc24)",
        ),
        baseline_fixed["call_x"]: (
            ("ff", "52", "64"),
            r"call\s+(?:dword\s+(?:ptr\s+)?)?"
            r"\[edx(?:\+100|\+0x64)\]",
        ),
        baseline_fixed["result_x"]: (
            ("8b", "f8"),
            r"mov\s+edi\s*,\s*eax",
        ),
        baseline_fixed["load_y"]: (
            ("a1", "24", "0c", "00", "00"),
            rf"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            rf"{aggregate_pattern}\+(?:3108|0xc24)",
        ),
        baseline_fixed["receiver_y"]: (
            ("b9", "24", "0c", "00", "00"),
            rf"mov\s+ecx\s*,\s*(?:offset\s+(?:flat:)?)?"
            rf"{aggregate_pattern}\+(?:3108|0xc24)",
        ),
        baseline_fixed["call_y"]: (
            ("ff", "50", "68"),
            r"call\s+(?:dword\s+(?:ptr\s+)?)?"
            r"\[eax(?:\+104|\+0x68)\]",
        ),
    }
    unit_shift, addresses, by_address, index_by_address = (
        _cc_callable_identity._locate_shifted_candidate_instruction_unit(
            candidate,
            baseline_expected_rows,
            label="HUD EnsureHudLoaded sensor-center candidate bridge",
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
    ordered_addresses = list(expected_rows)
    ordered_indices = [index_by_address[address] for address in ordered_addresses]
    if (
        ordered_addresses != sorted(ordered_addresses)
        or ordered_indices != sorted(ordered_indices)
        or len(set(ordered_indices)) != len(ordered_indices)
    ):
        raise ValueError(
            "HUD EnsureHudLoaded sensor-center candidate bridge rejects "
            "reordered or aliased pair instructions"
        )

    pair_start = (
        _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CANDIDATE_LOAD_X_OFFSET + unit_shift
    )
    pair_end = (
        _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CANDIDATE_CALL_Y_OFFSET + unit_shift + 3
    )
    exact_body = bytes.fromhex(
        "8b 15 24 0c 00 00 "
        "b9 24 0c 00 00 "
        "ff 52 64 "
        "8b f8 "
        "a1 24 0c 00 00 "
        "b9 24 0c 00 00 "
        "ff 50 68"
    )
    relocation_specs = {
        _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CANDIDATE_LOAD_X_OFFSET + unit_shift + 2,
        _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CANDIDATE_RECEIVER_X_OFFSET + unit_shift + 1,
        _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CANDIDATE_LOAD_Y_OFFSET + unit_shift + 1,
        _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CANDIDATE_RECEIVER_Y_OFFSET + unit_shift + 1,
    }
    bounded_relocations = [
        row
        for row in definition.relocations
        if pair_start <= row.offset < pair_end
    ]
    if (
        definition.data[pair_start:pair_end] != exact_body
        or {row.offset for row in bounded_relocations} != relocation_specs
        or len(bounded_relocations) != len(relocation_specs)
        or any(
            row.type != IMAGE_REL_I386_DIR32
            or row.symbol_name != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
            or struct.unpack_from("<I", definition.data, row.offset)[0]
            != _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_PANEL_DISPLACEMENT
            for row in bounded_relocations
        )
        or {
            index
            for index in range(pair_start, pair_end)
            if definition.relocation_mask[index]
        }
        != {
            index
            for offset in relocation_specs
            for index in range(offset, offset + 4)
        }
        or definition.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in definition.defined_external_data
    ):
        raise ValueError(
            "HUD EnsureHudLoaded sensor-center candidate bridge rejects "
            "missing, extra, aliased, or malformed DIR32 +0xc24 pair fields"
        )

    call_x_index = index_by_address[fixed["call_x"]]
    call_y_index = index_by_address[fixed["call_y"]]
    bounded_invocations = [
        address
        for address, instruction in zip(addresses, candidate.instructions)
        if (
            address is not None
            and fixed["load_x"] <= address <= fixed["call_y"]
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
    bounded_indices = {
        index
        for index, address in enumerate(addresses)
        if (
            address is not None
            and fixed["load_x"] <= address <= fixed["call_y"]
        )
    }
    if (
        bounded_invocations != [fixed["call_x"], fixed["call_y"]]
        or _cc_cfg._cleanup_after(candidate.instructions, call_x_index) is not None
        or _cc_cfg._cleanup_after(candidate.instructions, call_y_index) is not None
        or bool(candidate.local_control_flow_indices & bounded_indices)
        or any(
            target_index in bounded_indices
            for targets in candidate.local_control_flow_targets.values()
            for target_index in targets
        )
    ):
        raise ValueError(
            "HUD EnsureHudLoaded sensor-center candidate bridge rejects "
            "missing, extra, reordered, wrong-cleanup, or alternate-path pairs"
        )

    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    return {
        normalize_address(
            hex(_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CANDIDATE_LOAD_X_OFFSET + unit_shift)
        ): ReviewedAbsoluteStorageLoadBridge(
            register="edx",
            aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            displacement=_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_PANEL_DISPLACEMENT,
            access_width=4,
            storage_identity=aggregate_identity,
        ),
        normalize_address(
            hex(_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CANDIDATE_LOAD_Y_OFFSET + unit_shift)
        ): ReviewedAbsoluteStorageLoadBridge(
            register="eax",
            aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            displacement=_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_PANEL_DISPLACEMENT,
            access_width=4,
            storage_identity=aggregate_identity,
        ),
    }


def _hud_ui_mgr_ensure_sensor_meter_setclip_retail_guard(
    instructions: Sequence[Instruction],
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> None:
    """Guard retail's exact sensor-meter common SetClip semantic unit."""
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START:
        return
    _require_hud_ui_mgr_ensure_sensor_center_authority(
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    aggregate_base = address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
    if (
        address_value(_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_RETAIL_IMAGE_ADDRESS)
        != aggregate_base + _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_IMAGE_DISPLACEMENT
        or address_value(
            _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_RETAIL_RECEIVER_ADDRESS
        )
        != aggregate_base + _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_DISPLACEMENT
        or address_value(_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_RETAIL_COLOR_ADDRESS)
        != aggregate_base + _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_COLOR_DISPLACEMENT
    ):
        raise ValueError(
            "HUD EnsureHudLoaded sensor-meter SetClip retail guard requires "
            "the exact typed aggregate image/meter/color field addresses"
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
        0x41046A: (
            ("a1", "30", "6b", "4e", "00"),
            r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[?0x4e6b30\]?",
        ),
        0x41046F: (
            ("8d", "55", "ac"),
            r"lea\s+edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[ebp(?:-84|-0x54)\]",
        ),
        0x410472: (("52",), r"push\s+edx"),
        0x410473: (
            ("8b", "15", "6c", "6c", "4e", "00"),
            r"mov\s+edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[?0x4e6c6c\]?",
        ),
        0x410479: (("50",), r"push\s+eax"),
        0x41047A: (
            ("b9", "6c", "6c", "4e", "00"),
            r"mov\s+ecx\s*,\s*0x4e6c6c",
        ),
        0x41047F: (
            ("c7", "05", "a0", "6d", "4e", "00", "e0", "07", "00", "00"),
            r"mov\s+(?:dword\s+(?:ptr\s+)?)?"
            r"\[?0x4e6da0\]?\s*,\s*(?:2016|0x7e0)",
        ),
        0x410489: (
            ("ff", "52", "18"),
            r"call\s+(?:dword\s+(?:ptr\s+)?)?"
            r"\[edx(?:\+24|\+0x18)\]",
        ),
        0x41048C: (
            ("b9", "d0", "5e", "4e", "00"),
            r"mov\s+ecx\s*,\s*0x4e5ed0",
        ),
        0x410491: (
            ("68", "f4", "6a", "4e", "00"),
            r"push\s+(?:0x4e6af4|5171956)",
        ),
        0x410496: (
            ("e8", "25", "c3", "0a", "00"),
            r"call\s+(?:0x4bc7c0|HudUiContainer::AddChild)",
        ),
        0x41049B: (
            ("b9", "d0", "5e", "4e", "00"),
            r"mov\s+ecx\s*,\s*0x4e5ed0",
        ),
        0x4104A0: (
            ("68", "b0", "6b", "4e", "00"),
            r"push\s+(?:0x4e6bb0|5172144)",
        ),
        0x4104A5: (
            ("e8", "16", "c3", "0a", "00"),
            r"call\s+(?:0x4bc7c0|HudUiContainer::AddChild)",
        ),
        0x4104AA: (
            ("b9", "d0", "5e", "4e", "00"),
            r"mov\s+ecx\s*,\s*0x4e5ed0",
        ),
        0x4104AF: (
            ("68", "6c", "6c", "4e", "00"),
            r"push\s+(?:0x4e6c6c|5172332)",
        ),
        0x4104B4: (
            ("e8", "07", "c3", "0a", "00"),
            r"call\s+(?:0x4bc7c0|HudUiContainer::AddChild)",
        ),
        0x4104B9: (
            ("8b", "5d", "f0"),
            r"mov\s+ebx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[ebp(?:-16|-0x10)\]",
        ),
    }
    row_mismatches = [
        (
            f"{hex(address)}:"
            f"count={counts.get(address, 0)}:"
            f"bytes={tuple(value.lower() for value in by_address[address].bytes) if address in by_address else ()}:"
            f"text={by_address[address].raw_text.strip()!r}"
            if address in by_address
            else f"{hex(address)}:missing"
        )
        for address, (body, pattern) in fixed_rows.items()
        if (
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
        )
    ]
    if row_mismatches:
        raise ValueError(
            "HUD EnsureHudLoaded sensor-meter SetClip retail guard requires "
            "the exact image/rect/vptr/receiver/color/call/AddChild unit: "
            + "; ".join(row_mismatches)
        )
    ordered_addresses = list(fixed_rows)
    ordered_indices = [index_by_address[address] for address in ordered_addresses]
    call_index = index_by_address[
        address_value(_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_RETAIL_CALL_ADDRESS)
    ]
    bounded_calls = [
        address
        for address, instruction in zip(addresses, instructions)
        if (
            address is not None
            and ordered_addresses[0] <= address < ordered_addresses[-1]
            and _cc_cfg._instruction_mnemonic(instruction) in {"call", "jmp"}
        )
    ]
    if (
        ordered_addresses != sorted(ordered_addresses)
        or ordered_indices != sorted(ordered_indices)
        or len(set(ordered_indices)) != len(ordered_indices)
        or bounded_calls
        != [0x410489, 0x410496, 0x4104A5, 0x4104B4]
        or sum(
            _cc_cfg._instruction_mnemonic(instruction) == "call"
            for instruction in instructions[:call_index]
        )
        != _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_RETAIL_CALL_ORDINAL
        or _cc_cfg._cleanup_after(instructions, call_index) is not None
    ):
        raise ValueError(
            "HUD EnsureHudLoaded sensor-meter SetClip retail guard rejects "
            "missing, extra, reordered, aliased, or caller-cleanup rows"
        )


def _hud_ui_mgr_ensure_sensor_meter_setclip_candidate_absolute_load_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedAbsoluteStorageLoadBridge]:
    """Bridge only EnsureHudLoaded's exact sensor-meter SetClip callsite."""
    from _recoil.call_contract.records import ReviewedAbsoluteStorageLoadBridge
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START:
        return {}
    _require_hud_ui_mgr_ensure_sensor_center_authority(
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
            "HUD EnsureHudLoaded sensor-meter SetClip candidate bridge "
            "requires a nonempty candidate COMDAT and current HUD target"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != r"\?EnsureHudLoaded@HudUiMgr@@.*"
        or re.fullmatch(
            str(getattr(contribution_row, "symbol_regex", "")),
            definition.symbol,
        )
        is None
        or getattr(contribution_row, "name", "")
        != "HudUiMgr::EnsureHudLoaded"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD EnsureHudLoaded sensor-meter SetClip candidate bridge "
            "requires the exact authored hud.cpp contribution"
        )

    expected_row = {
        "ordinal": _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_RETAIL_CALL_ORDINAL,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_STORAGE_IDENTITY,
        "slot_displacement": (
            _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_SLOT_DISPLACEMENT
        ),
        "cleanup_bytes": None,
    }
    if (
        len(expected) <= _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_RETAIL_CALL_ORDINAL
        or expected[
            _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_RETAIL_CALL_ORDINAL
        ]
        != expected_row
    ):
        raise ValueError(
            "HUD EnsureHudLoaded sensor-meter SetClip candidate bridge "
            "requires exact independently retail-derived virtual "
            f"{_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_SLOT_IDENTITY} storage+slot"
        )

    add_child_identity = indexes.by_address.get("0x4bc7c0", "")
    add_child_addresses = sorted(
        address
        for address, identity in indexes.by_address.items()
        if identity == add_child_identity
    )
    add_child_names = [
        (name, identity)
        for name, identity in indexes.by_candidate_name.items()
        if name.casefold()
        == _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_ADD_CHILD_SYMBOL.casefold()
    ]
    if (
        not add_child_identity
        or add_child_identity in indexes.provider_ids
        or add_child_addresses != ["0x4bc7c0"]
        or add_child_names
        != [
            (
                _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_ADD_CHILD_SYMBOL,
                add_child_identity,
            )
        ]
    ):
        raise ValueError(
            "HUD EnsureHudLoaded sensor-meter SetClip candidate bridge "
            "requires one exact authored AddChild post-call identity"
        )

    start = 0
    aggregate = re.escape(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
    add_child = re.escape(_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_ADD_CHILD_SYMBOL)
    baseline_fixed = {
        "image": start + 0x387,
        "vptr": start + 0x38D,
        "rect": start + 0x393,
        "color": start + 0x397,
        "push_rect": start + 0x3A1,
        "push_image": start + 0x3A2,
        "receiver": start + 0x3A3,
        "call": start + 0x3A8,
        "panel_owner": start + 0x3AB,
        "panel_arg": start + 0x3B0,
        "panel_call": start + 0x3B5,
        "overlay_owner": start + 0x3BA,
        "overlay_arg": start + 0x3BF,
        "overlay_call": start + 0x3C4,
        "meter_owner": start + 0x3C9,
        "meter_arg": start + 0x3CE,
        "meter_call": start + 0x3D3,
    }
    baseline_expected_rows = {
        baseline_fixed["image"]: (
            ("8b", "0d", "60", "0c", "00", "00"),
            rf"mov\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            rf"{aggregate}\+(?:3168|0xc60)",
        ),
        baseline_fixed["vptr"]: (
            ("8b", "15", "9c", "0d", "00", "00"),
            rf"mov\s+edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            rf"{aggregate}\+(?:3484|0xd9c)",
        ),
        baseline_fixed["rect"]: (
            ("8d", "44", "24", "34"),
            r"lea\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"(?:_meterRect\$[A-Za-z0-9_]+\[esp\+148\]|\[esp\+52\])",
        ),
        baseline_fixed["color"]: (
            ("c7", "05", "d0", "0e", "00", "00", "e0", "07", "00", "00"),
            rf"mov\s+(?:dword\s+(?:ptr\s+)?)?"
            rf"{aggregate}\+(?:3792|0xed0)\s*,\s*(?:2016|0x7e0)",
        ),
        baseline_fixed["push_rect"]: (("50",), r"push\s+eax"),
        baseline_fixed["push_image"]: (("51",), r"push\s+ecx"),
        baseline_fixed["receiver"]: (
            ("b9", "9c", "0d", "00", "00"),
            rf"mov\s+ecx\s*,\s*(?:offset\s+(?:flat:)?)?"
            rf"{aggregate}\+(?:3484|0xd9c)",
        ),
        baseline_fixed["call"]: (
            ("ff", "52", "18"),
            r"call\s+(?:dword\s+(?:ptr\s+)?)?"
            r"\[edx(?:\+24|\+0x18)\]",
        ),
        baseline_fixed["panel_owner"]: (
            ("b9", "00", "00", "00", "00"),
            rf"mov\s+ecx\s*,\s*(?:offset\s+(?:flat:)?)?{aggregate}",
        ),
        baseline_fixed["panel_arg"]: (
            ("68", "24", "0c", "00", "00"),
            rf"push\s+(?:offset\s+(?:flat:)?)?"
            rf"{aggregate}\+(?:3108|0xc24)",
        ),
        baseline_fixed["panel_call"]: (
            ("e8", "00", "00", "00", "00"),
            rf"call\s+{add_child}(?:\s*;.*)?",
        ),
        baseline_fixed["overlay_owner"]: (
            ("b9", "00", "00", "00", "00"),
            rf"mov\s+ecx\s*,\s*(?:offset\s+(?:flat:)?)?{aggregate}",
        ),
        baseline_fixed["overlay_arg"]: (
            ("68", "e0", "0c", "00", "00"),
            rf"push\s+(?:offset\s+(?:flat:)?)?"
            rf"{aggregate}\+(?:3296|0xce0)",
        ),
        baseline_fixed["overlay_call"]: (
            ("e8", "00", "00", "00", "00"),
            rf"call\s+{add_child}(?:\s*;.*)?",
        ),
        baseline_fixed["meter_owner"]: (
            ("b9", "00", "00", "00", "00"),
            rf"mov\s+ecx\s*,\s*(?:offset\s+(?:flat:)?)?{aggregate}",
        ),
        baseline_fixed["meter_arg"]: (
            ("68", "9c", "0d", "00", "00"),
            rf"push\s+(?:offset\s+(?:flat:)?)?"
            rf"{aggregate}\+(?:3484|0xd9c)",
        ),
        baseline_fixed["meter_call"]: (
            ("e8", "00", "00", "00", "00"),
            rf"call\s+{add_child}(?:\s*;.*)?",
        ),
    }
    baseline_locator_rows = {
        address: row
        for address, row in baseline_expected_rows.items()
        if address != baseline_fixed["rect"]
    }
    unit_shift, addresses, by_address, index_by_address = (
        _cc_callable_identity._locate_shifted_candidate_instruction_unit(
            candidate,
            baseline_locator_rows,
            label=(
                "HUD EnsureHudLoaded sensor-meter SetClip candidate bridge"
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
    ordered_addresses = list(expected_rows)
    ordered_indices = [index_by_address[address] for address in ordered_addresses]
    if (
        ordered_addresses != sorted(ordered_addresses)
        or ordered_indices != sorted(ordered_indices)
        or len(set(ordered_indices)) != len(ordered_indices)
    ):
        raise ValueError(
            "HUD EnsureHudLoaded sensor-meter SetClip candidate bridge "
            "rejects reordered or aliased unit instructions"
        )

    unit_start = 0x387 + unit_shift
    unit_end = 0x3D8 + unit_shift
    normalized_body = (
        _cc_callable_identity._candidate_body_with_normalized_symbolic_stack_displacements(
            candidate,
            unit_start=unit_start,
            unit_end=unit_end,
            by_address=by_address,
            specs={
                fixed["rect"]: (
                    (0x8D, 0x44, 0x24),
                    expected_rows[fixed["rect"]][1],
                    0x34,
                )
            },
            label=(
                "HUD EnsureHudLoaded sensor-meter SetClip candidate bridge"
            ),
        )
    )
    exact_body = bytes.fromhex(
        "8b 0d 60 0c 00 00 "
        "8b 15 9c 0d 00 00 "
        "8d 44 24 34 "
        "c7 05 d0 0e 00 00 e0 07 00 00 "
        "50 51 "
        "b9 9c 0d 00 00 "
        "ff 52 18 "
        "b9 00 00 00 00 "
        "68 24 0c 00 00 "
        "e8 00 00 00 00 "
        "b9 00 00 00 00 "
        "68 e0 0c 00 00 "
        "e8 00 00 00 00 "
        "b9 00 00 00 00 "
        "68 9c 0d 00 00 "
        "e8 00 00 00 00"
    )
    dir32_specs = {
        0x389 + unit_shift: _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_IMAGE_DISPLACEMENT,
        0x38F + unit_shift: _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_DISPLACEMENT,
        0x399 + unit_shift: _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_COLOR_DISPLACEMENT,
        0x3A4 + unit_shift: _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_DISPLACEMENT,
        0x3AC + unit_shift: 0,
        0x3B1 + unit_shift: _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_PANEL_DISPLACEMENT,
        0x3BB + unit_shift: 0,
        0x3C0 + unit_shift: _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_OVERLAY_DISPLACEMENT,
        0x3CA + unit_shift: 0,
        0x3CF + unit_shift: _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_DISPLACEMENT,
    }
    rel32_offsets = {
        0x3B6 + unit_shift,
        0x3C5 + unit_shift,
        0x3D4 + unit_shift,
    }
    bounded_relocations = [
        row
        for row in definition.relocations
        if unit_start <= row.offset < unit_end
    ]
    bounded_by_offset: dict[int, list[CoffRelocation]] = {}
    for row in bounded_relocations:
        bounded_by_offset.setdefault(row.offset, []).append(row)
    expected_offsets = set(dir32_specs) | rel32_offsets
    expected_mask = {
        index
        for offset in expected_offsets
        for index in range(offset, offset + 4)
    }
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
            or bounded_by_offset[offset][0].symbol_name
            != _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_ADD_CHILD_SYMBOL
            or struct.unpack_from("<I", definition.data, offset)[0] != 0
            for offset in rel32_offsets
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
        or definition.undefined_external_functions.count(
            _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_ADD_CHILD_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_ADD_CHILD_SYMBOL
        in definition.defined_external_functions
    ):
        raise ValueError(
            "HUD EnsureHudLoaded sensor-meter SetClip candidate bridge "
            "rejects missing, extra, malformed, aliased, wrong-target, "
            "wrong-addend, or wrong-mask DIR32/REL32 fields"
        )

    call_index = index_by_address[fixed["call"]]
    bounded_invocations = [
        address
        for address, instruction in zip(addresses, candidate.instructions)
        if (
            address is not None
            and fixed["image"] <= address <= fixed["meter_call"]
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
    bounded_indices = {
        index
        for index, address in enumerate(addresses)
        if (
            address is not None
            and fixed["image"] <= address <= fixed["meter_call"]
        )
    }
    if (
        bounded_invocations
        != [
            fixed["call"],
            fixed["panel_call"],
            fixed["overlay_call"],
            fixed["meter_call"],
        ]
        or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
        or bool(candidate.local_control_flow_indices & bounded_indices)
        or any(
            target_index in bounded_indices
            for targets in candidate.local_control_flow_targets.values()
            for target_index in targets
        )
    ):
        raise ValueError(
            "HUD EnsureHudLoaded sensor-meter SetClip candidate bridge "
            "rejects missing, extra, reordered, wrong-cleanup, wrong "
            "post-call, or alternate-path unit rows"
        )

    return {
        normalize_address(
            hex(
                _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_CANDIDATE_LOAD_OFFSET
                + unit_shift
            )
        ): ReviewedAbsoluteStorageLoadBridge(
            register="edx",
            aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            displacement=_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_METER_DISPLACEMENT,
            access_width=4,
            storage_identity=f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}",
        )
    }


def _hud_ui_mgr_ensure_objective_sensor_center_retail_guard(
    instructions: Sequence[Instruction],
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> None:
    """Guard retail's unconditional objective-path sensor-center pair."""
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START:
        return
    _require_hud_ui_mgr_ensure_sensor_center_authority(
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    aggregate_base = address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
    if (
        address_value(_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_PHASE_DURATION_ADDRESS)
        != (
            aggregate_base
            + _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_PHASE_DURATION_DISPLACEMENT
        )
        or address_value(_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_ADDRESS)
        != (
            aggregate_base
            + _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_DISPLACEMENT
        )
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective sensor-center retail guard "
            "requires exact typed phase-duration/objective-widget addresses"
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
        0x4104BC: (
            ("ba", "6c", "ad", "4d", "00"),
            r"mov\s+edx\s*,\s*0x4dad6c",
        ),
        0x4104C1: (("8b", "cb"), r"mov\s+ecx\s*,\s*ebx"),
        0x4104C3: (
            ("e8", "a8", "ca", "07", "00"),
            r"call\s+(?:0x48cf70|zReader::zRdrGetNode)",
        ),
        0x4104C8: (("8b", "f0"), r"mov\s+esi\s*,\s*eax"),
        0x4104CA: (("3b", "f7"), r"cmp\s+esi\s*,\s*edi"),
        0x4104CC: (
            ("0f", "84", "00", "03", "00", "00"),
            r"(?:je|jz)\s+0x4107d2",
        ),
        0x4104D2: (
            ("8b", "46", "04"),
            r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[esi(?:\+4|\+0x4)\]",
        ),
        0x4104D5: (
            ("8b", "15", "f4", "6a", "4e", "00"),
            r"mov\s+edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[?0x4e6af4\]?",
        ),
        0x4104DB: (
            ("8b", "48", "0c"),
            r"mov\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[eax(?:\+12|\+0xc)\]",
        ),
        0x4104DE: (
            ("89", "0d", "6c", "65", "4e", "00"),
            r"mov\s+(?:dword\s+(?:ptr\s+)?)?"
            r"\[?0x4e656c\]?\s*,\s*ecx",
        ),
        0x4104E4: (
            ("b9", "f4", "6a", "4e", "00"),
            r"mov\s+ecx\s*,\s*0x4e6af4",
        ),
        0x4104E9: (
            ("ff", "52", "64"),
            r"call\s+(?:dword\s+(?:ptr\s+)?)?"
            r"\[edx(?:\+100|\+0x64)\]",
        ),
        0x4104EC: (
            ("89", "45", "e8"),
            r"mov\s+(?:dword\s+(?:ptr\s+)?)?"
            r"\[ebp(?:-24|-0x18)\]\s*,\s*eax",
        ),
        0x4104EF: (
            ("a1", "f4", "6a", "4e", "00"),
            r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[?0x4e6af4\]?",
        ),
        0x4104F4: (
            ("b9", "f4", "6a", "4e", "00"),
            r"mov\s+ecx\s*,\s*0x4e6af4",
        ),
        0x4104F9: (
            ("ff", "50", "68"),
            r"call\s+(?:dword\s+(?:ptr\s+)?)?"
            r"\[eax(?:\+104|\+0x68)\]",
        ),
        0x4104FC: (
            ("8d", "4d", "e8"),
            r"lea\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[ebp(?:-24|-0x18)\]",
        ),
        0x4104FF: (
            ("ba", "7c", "65", "4e", "00"),
            r"mov\s+edx\s*,\s*0x4e657c",
        ),
        0x410504: (("57",), r"push\s+edi"),
        0x410505: (("57",), r"push\s+edi"),
        0x410506: (("51",), r"push\s+ecx"),
        0x410507: (
            ("8b", "4e", "04"),
            r"mov\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[esi(?:\+4|\+0x4)\]",
        ),
        0x41050A: (("57",), r"push\s+edi"),
        0x41050B: (("57",), r"push\s+edi"),
        0x41050C: (
            ("83", "c1", "10"),
            r"add\s+ecx\s*,\s*(?:16|0x10)",
        ),
        0x41050F: (
            ("89", "45", "ec"),
            r"mov\s+(?:dword\s+(?:ptr\s+)?)?"
            r"\[ebp(?:-20|-0x14)\]\s*,\s*eax",
        ),
        0x410512: (
            ("e8", "19", "38", "00", "00"),
            r"call\s+(?:0x413d30|HudUiLayoutNode::ApplyImageWidget)",
        ),
    }
    row_mismatches = [
        (
            f"{hex(address)}:"
            f"count={counts.get(address, 0)}:"
            f"bytes={tuple(value.lower() for value in by_address[address].bytes) if address in by_address else ()}:"
            f"text={by_address[address].raw_text.strip()!r}"
            if address in by_address
            else f"{hex(address)}:missing"
        )
        for address, (body, pattern) in fixed_rows.items()
        if (
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
        )
    ]
    if row_mismatches:
        raise ValueError(
            "HUD EnsureHudLoaded objective sensor-center retail guard "
            "requires the exact lookup/guard/phase/center/ApplyImageWidget "
            "unit: "
            + "; ".join(row_mismatches)
        )

    ordered_addresses = list(fixed_rows)
    ordered_indices = [index_by_address[address] for address in ordered_addresses]
    call_x = address_value(
        _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_CALL_X_ADDRESS
    )
    call_y = address_value(
        _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_CALL_Y_ADDRESS
    )
    bounded_invocations = [
        address
        for address, instruction in zip(addresses, instructions)
        if (
            address is not None
            and ordered_addresses[0] <= address <= ordered_addresses[-1]
            and _cc_cfg._instruction_mnemonic(instruction) in {"call", "jmp"}
        )
    ]
    bounded_branches = [
        address
        for address, instruction in zip(addresses, instructions)
        if (
            address is not None
            and ordered_addresses[0] <= address <= ordered_addresses[-1]
            and _cc_cfg._instruction_mnemonic(instruction)
            in {
                "ja",
                "jae",
                "jb",
                "jbe",
                "je",
                "jg",
                "jge",
                "jl",
                "jle",
                "jne",
                "jno",
                "jnp",
                "jns",
                "jo",
                "jp",
                "js",
                "jz",
            }
        )
    ]
    if (
        ordered_addresses != sorted(ordered_addresses)
        or ordered_indices != sorted(ordered_indices)
        or len(set(ordered_indices)) != len(ordered_indices)
        or bounded_invocations
        != [0x4104C3, call_x, call_y, 0x410512]
        or bounded_branches != [0x4104CC]
        or sum(
            _cc_cfg._instruction_mnemonic(instruction) == "call"
            for instruction in instructions[: index_by_address[call_x]]
        )
        != _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_CALL_X_ORDINAL
        or sum(
            _cc_cfg._instruction_mnemonic(instruction) == "call"
            for instruction in instructions[: index_by_address[call_y]]
        )
        != _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_CALL_Y_ORDINAL
        or _cc_cfg._cleanup_after(instructions, index_by_address[call_x]) is not None
        or _cc_cfg._cleanup_after(instructions, index_by_address[call_y]) is not None
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective sensor-center retail guard "
            "requires one objective-absent guard, an unconditional "
            "GetCenterX/GetCenterY pair, and the following ApplyImageWidget"
        )


def _hud_ui_mgr_ensure_objective_sensor_center_candidate_absolute_load_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    zrd_payload_bridges: Mapping[str, str],
) -> dict[str, ReviewedAbsoluteStorageLoadBridge]:
    """Bridge only the candidate's conditional objective sensor-center pair."""
    from _recoil.call_contract.records import ReviewedAbsoluteStorageLoadBridge
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START:
        return {}
    _require_hud_ui_mgr_ensure_sensor_center_authority(
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
            "HUD EnsureHudLoaded objective sensor-center candidate bridge "
            "requires a nonempty candidate COMDAT and current HUD target"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != r"\?EnsureHudLoaded@HudUiMgr@@.*"
        or re.fullmatch(
            str(getattr(contribution_row, "symbol_regex", "")),
            definition.symbol,
        )
        is None
        or getattr(contribution_row, "name", "")
        != "HudUiMgr::EnsureHudLoaded"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective sensor-center candidate bridge "
            "requires the exact authored hud.cpp contribution"
        )

    expected_pair = (
        {
            "ordinal": _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_CALL_X_ORDINAL,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_STORAGE_IDENTITY,
            "slot_displacement": _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CALL_X_SLOT,
            "cleanup_bytes": None,
        },
        {
            "ordinal": _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_CALL_Y_ORDINAL,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_STORAGE_IDENTITY,
            "slot_displacement": _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_CALL_Y_SLOT,
            "cleanup_bytes": None,
        },
    )
    if (
        len(expected) <= _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_CALL_Y_ORDINAL
        or tuple(
            expected[ordinal]
            for ordinal in (
                _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_CALL_X_ORDINAL,
                _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_CALL_Y_ORDINAL,
            )
        )
        != expected_pair
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective sensor-center candidate bridge "
            "requires exact independently retail-derived 37th/38th "
            "GetCenterX/GetCenterY virtual contracts"
        )

    start = 0
    aggregate = re.escape(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
    objective_key = re.escape(_cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_KEY_SYMBOL)
    get_named = re.escape(
        _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_GET_NAMED_NODE_SYMBOL
    )
    apply_widget = re.escape(
        _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_APPLY_IMAGE_WIDGET_SYMBOL
    )
    fixed = {
        "root": start + 0x3D8,
        "key": start + 0x3DC,
        "lookup": start + 0x3E1,
        "payload_receiver": start + 0x3E6,
        "payload": start + 0x3E8,
        "payload_result": start + 0x3ED,
        "payload_test": start + 0x3EF,
        "payload_absent": start + 0x3F1,
        "phase_load": start + 0x3F7,
        "cached_x_test": start + 0x3FA,
        "phase_store": start + 0x3FC,
        "cached_x_branch": start + 0x401,
        "cached_x_store": start + 0x403,
        "cached_x_join": start + 0x407,
        "load_x": start + 0x409,
        "receiver_x": start + 0x40F,
        "call_x": start + 0x414,
        "result_x": start + 0x417,
        "cached_y_test": start + 0x41B,
        "cached_y_branch": start + 0x41D,
        "cached_y_store": start + 0x41F,
        "cached_y_join": start + 0x423,
        "load_y": start + 0x425,
        "receiver_y": start + 0x42A,
        "call_y": start + 0x42F,
        "result_y": start + 0x432,
        "apply_arg_0": start + 0x436,
        "panel_center": start + 0x437,
        "apply_arg_1": start + 0x43B,
        "apply_arg_2": start + 0x43C,
        "apply_arg_3": start + 0x43D,
        "apply_arg_4": start + 0x43E,
        "widget": start + 0x43F,
        "payload_widget": start + 0x444,
        "apply": start + 0x447,
    }
    expected_rows = {
        fixed["root"]: (
            ("8b", "4c", "24", "44"),
            r"mov\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"_root\$[A-Za-z0-9_]*\[esp\+148\]",
        ),
        fixed["key"]: (
            ("ba", "00", "00", "00", "00"),
            rf"mov\s+edx\s*,\s*(?:offset\s+(?:flat:)?)?{objective_key}"
            r"(?:\s*;.*)?",
        ),
        fixed["lookup"]: (
            ("e8", "00", "00", "00", "00"),
            rf"call\s+{get_named}(?:\s*;.*)?",
        ),
        fixed["payload_receiver"]: (
            ("8b", "c8"),
            r"mov\s+ecx\s*,\s*eax",
        ),
        fixed["payload"]: (
            ("e8", "00", "00", "00", "00"),
            r"call\s+\?HudUiZrdPayload@\?_tu_order\\"
            r"00_hud\.cpp\d+@@YIPAUNode@zReader@@PAU23@@Z(?:\s*;.*)?",
        ),
        fixed["payload_result"]: (
            ("8b", "f0"),
            r"mov\s+esi\s*,\s*eax",
        ),
        fixed["payload_test"]: (
            ("3b", "f3"),
            r"cmp\s+esi\s*,\s*ebx",
        ),
        fixed["payload_absent"]: (
            ("0f", "84", "3f", "03", "00", "00"),
            r"(?:je|jz)\s+\$L[A-Za-z0-9_]+",
        ),
        fixed["phase_load"]: (
            ("8b", "46", "0c"),
            r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[esi(?:\+12|\+0xc)\]",
        ),
        fixed["cached_x_test"]: (
            ("3b", "fb"),
            r"cmp\s+edi\s*,\s*ebx",
        ),
        fixed["phase_store"]: (
            ("a3", "9c", "06", "00", "00"),
            rf"mov\s+(?:dword\s+(?:ptr\s+)?)?"
            rf"{aggregate}\+(?:1692|0x69c)\s*,\s*eax",
        ),
        fixed["cached_x_branch"]: (
            ("74", "06"),
            r"(?:je|jz)\s+(?:short\s+)?\$L[A-Za-z0-9_]+",
        ),
        fixed["cached_x_store"]: (
            ("89", "7c", "24", "48"),
            r"mov\s+(?:dword\s+(?:ptr\s+)?)?"
            r"_panelCenter\$[A-Za-z0-9_]+\[esp\+148\]\s*,\s*edi",
        ),
        fixed["cached_x_join"]: (
            ("eb", "12"),
            r"jmp\s+(?:short\s+)?\$L[A-Za-z0-9_]+",
        ),
        fixed["load_x"]: (
            ("8b", "15", "24", "0c", "00", "00"),
            rf"mov\s+edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            rf"{aggregate}\+(?:3108|0xc24)",
        ),
        fixed["receiver_x"]: (
            ("b9", "24", "0c", "00", "00"),
            rf"mov\s+ecx\s*,\s*(?:offset\s+(?:flat:)?)?"
            rf"{aggregate}\+(?:3108|0xc24)",
        ),
        fixed["call_x"]: (
            ("ff", "52", "64"),
            r"call\s+(?:dword\s+(?:ptr\s+)?)?"
            r"\[edx(?:\+100|\+0x64)\]",
        ),
        fixed["result_x"]: (
            ("89", "44", "24", "48"),
            r"mov\s+(?:dword\s+(?:ptr\s+)?)?"
            r"_panelCenter\$[A-Za-z0-9_]+\[esp\+148\]\s*,\s*eax",
        ),
        fixed["cached_y_test"]: (
            ("3b", "eb"),
            r"cmp\s+ebp\s*,\s*ebx",
        ),
        fixed["cached_y_branch"]: (
            ("74", "06"),
            r"(?:je|jz)\s+(?:short\s+)?\$L[A-Za-z0-9_]+",
        ),
        fixed["cached_y_store"]: (
            ("89", "6c", "24", "4c"),
            r"mov\s+(?:dword\s+(?:ptr\s+)?)?"
            r"_panelCenter\$[A-Za-z0-9_]+\[esp\+152\]\s*,\s*ebp",
        ),
        fixed["cached_y_join"]: (
            ("eb", "11"),
            r"jmp\s+(?:short\s+)?\$L[A-Za-z0-9_]+",
        ),
        fixed["load_y"]: (
            ("a1", "24", "0c", "00", "00"),
            rf"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            rf"{aggregate}\+(?:3108|0xc24)",
        ),
        fixed["receiver_y"]: (
            ("b9", "24", "0c", "00", "00"),
            rf"mov\s+ecx\s*,\s*(?:offset\s+(?:flat:)?)?"
            rf"{aggregate}\+(?:3108|0xc24)",
        ),
        fixed["call_y"]: (
            ("ff", "50", "68"),
            r"call\s+(?:dword\s+(?:ptr\s+)?)?"
            r"\[eax(?:\+104|\+0x68)\]",
        ),
        fixed["result_y"]: (
            ("89", "44", "24", "4c"),
            r"mov\s+(?:dword\s+(?:ptr\s+)?)?"
            r"_panelCenter\$[A-Za-z0-9_]+\[esp\+152\]\s*,\s*eax",
        ),
        fixed["apply_arg_0"]: (("53",), r"push\s+ebx"),
        fixed["panel_center"]: (
            ("8d", "4c", "24", "4c"),
            r"lea\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"_panelCenter\$[A-Za-z0-9_]+\[esp\+152\]",
        ),
        fixed["apply_arg_1"]: (("53",), r"push\s+ebx"),
        fixed["apply_arg_2"]: (("51",), r"push\s+ecx"),
        fixed["apply_arg_3"]: (("53",), r"push\s+ebx"),
        fixed["apply_arg_4"]: (("53",), r"push\s+ebx"),
        fixed["widget"]: (
            ("ba", "ac", "06", "00", "00"),
            rf"mov\s+edx\s*,\s*(?:offset\s+(?:flat:)?)?"
            rf"{aggregate}\+(?:1708|0x6ac)",
        ),
        fixed["payload_widget"]: (
            ("8d", "4e", "10"),
            r"lea\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[esi(?:\+16|\+0x10)\]",
        ),
        fixed["apply"]: (
            ("e8", "00", "00", "00", "00"),
            rf"call\s+{apply_widget}(?:\s*;.*)?",
        ),
    }
    core_expected_rows = {
        address: row
        for address, row in expected_rows.items()
        if fixed["phase_store"] <= address <= fixed["apply"]
    }
    unit_shift, addresses, by_address, index_by_address = (
        _cc_callable_identity._locate_shifted_candidate_instruction_unit(
            candidate,
            core_expected_rows,
            label=(
                "HUD EnsureHudLoaded objective sensor-center candidate bridge"
            ),
        )
    )
    fixed = {
        key: address + unit_shift
        for key, address in fixed.items()
    }
    expected_rows = {
        address + unit_shift: row
        for address, row in core_expected_rows.items()
    }
    ordered_addresses = list(expected_rows)
    ordered_indices = [index_by_address[address] for address in ordered_addresses]
    if (
        ordered_addresses != sorted(ordered_addresses)
        or ordered_indices != sorted(ordered_indices)
        or len(set(ordered_indices)) != len(ordered_indices)
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective sensor-center candidate bridge "
            "rejects reordered or aliased unit instructions"
        )

    unit_start = 0x3FC + unit_shift
    unit_end = 0x44C + unit_shift
    exact_body = bytes.fromhex(
        "8b 4c 24 44 "
        "ba 00 00 00 00 "
        "e8 00 00 00 00 "
        "8b c8 "
        "e8 00 00 00 00 "
        "8b f0 3b f3 "
        "0f 84 3f 03 00 00 "
        "8b 46 0c 3b fb "
        "a3 9c 06 00 00 "
        "74 06 "
        "89 7c 24 48 "
        "eb 12 "
        "8b 15 24 0c 00 00 "
        "b9 24 0c 00 00 "
        "ff 52 64 "
        "89 44 24 48 "
        "3b eb "
        "74 06 "
        "89 6c 24 4c "
        "eb 11 "
        "a1 24 0c 00 00 "
        "b9 24 0c 00 00 "
        "ff 50 68 "
        "89 44 24 4c "
        "53 "
        "8d 4c 24 4c "
        "53 51 53 53 "
        "ba ac 06 00 00 "
        "8d 4e 10 "
        "e8 00 00 00 00"
    )
    exact_body = exact_body[0x3FC - 0x3D8 :]
    dir32_specs = {
        0x3FD + unit_shift: (
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_PHASE_DURATION_DISPLACEMENT,
        ),
        0x40B + unit_shift: (
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_PANEL_DISPLACEMENT,
        ),
        0x410 + unit_shift: (
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_PANEL_DISPLACEMENT,
        ),
        0x426 + unit_shift: (
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_PANEL_DISPLACEMENT,
        ),
        0x42B + unit_shift: (
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_PANEL_DISPLACEMENT,
        ),
        0x440 + unit_shift: (
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_DISPLACEMENT,
        ),
    }
    rel32_specs = {
        0x448 + unit_shift: _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_APPLY_IMAGE_WIDGET_SYMBOL,
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
    expected_mask = {
        index
        for offset in expected_offsets
        for index in range(offset, offset + 4)
    }
    if (
        definition.data[unit_start:unit_end] != exact_body
        or set(bounded_by_offset) != expected_offsets
        or any(len(rows) != 1 for rows in bounded_by_offset.values())
        or any(
            bounded_by_offset[offset][0].type != IMAGE_REL_I386_DIR32
            or bounded_by_offset[offset][0].symbol_name != symbol
            or struct.unpack_from("<I", definition.data, offset)[0]
            != addend
            for offset, (symbol, addend) in dir32_specs.items()
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
        != expected_mask
        or definition.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in definition.defined_external_data
        or definition.defined_external_functions.count(
            _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_APPLY_IMAGE_WIDGET_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_ENSURE_OBJECTIVE_APPLY_IMAGE_WIDGET_SYMBOL
        in definition.undefined_external_functions
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective sensor-center candidate bridge "
            "rejects missing, extra, malformed, aliased, wrong-target, "
            "wrong-addend, or wrong-mask DIR32/REL32 fields"
        )

    expected_cfg_addresses = {
        fixed["cached_x_branch"]: ("conditional", fixed["load_x"]),
        fixed["cached_x_join"]: ("unconditional", fixed["cached_y_test"]),
        fixed["cached_y_branch"]: ("conditional", fixed["load_y"]),
        fixed["cached_y_join"]: ("unconditional", fixed["apply_arg_0"]),
    }
    if any(
        source not in index_by_address
        or target not in index_by_address
        for source, (_kind, target) in expected_cfg_addresses.items()
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective sensor-center candidate bridge "
            "requires all exact conditional CFG source/target instructions"
        )
    exact_cfg = {
        source: _cc_cfg._exact_local_direct_branch(
            by_address[source],
            instruction_index=index_by_address[source],
            instruction_addresses=addresses,
            instruction_index_by_address=index_by_address,
            source="cod",
            caller_start=start,
            caller_end=address_value(caller_end_exclusive),
        )
        for source in expected_cfg_addresses
    }
    scoped_indices = {
        index
        for index, address in enumerate(addresses)
        if (
            address is not None
            and fixed["phase_store"] <= address <= fixed["apply"]
        )
    }
    incoming_scoped_targets = {
        (source, target_index)
        for source, targets in candidate.local_control_flow_targets.items()
        for target_index in targets
        if target_index in scoped_indices
    }
    if (
        any(
            exact_cfg[source]
            != (kind, index_by_address[target])
            for source, (kind, target) in expected_cfg_addresses.items()
        )
        or bool(candidate.local_control_flow_indices & scoped_indices)
        or incoming_scoped_targets
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective sensor-center candidate bridge "
            "requires the exact candidate-only objective/cached-center "
            "conditional CFG and preserves retail's unconditional difference"
        )

    call_x_index = index_by_address[fixed["call_x"]]
    call_y_index = index_by_address[fixed["call_y"]]
    apply_index = index_by_address[fixed["apply"]]
    bounded_invocations = [
        address
        for address, instruction in zip(addresses, candidate.instructions)
        if (
            address is not None
            and fixed["phase_store"] <= address <= fixed["apply"]
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
        bounded_invocations
        != [
            fixed["call_x"],
            fixed["call_y"],
            fixed["apply"],
        ]
        or _cc_cfg._cleanup_after(candidate.instructions, call_x_index) is not None
        or _cc_cfg._cleanup_after(candidate.instructions, call_y_index) is not None
        or _cc_cfg._cleanup_after(candidate.instructions, apply_index) is not None
    ):
        raise ValueError(
            "HUD EnsureHudLoaded objective sensor-center candidate bridge "
            "rejects missing, extra, reordered, wrong-cleanup, erased-center, "
            "or absorbed following ApplyImageWidget calls"
        )

    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    return {
        normalize_address(hex(0x409 + unit_shift)): ReviewedAbsoluteStorageLoadBridge(
            register="edx",
            aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            displacement=_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_PANEL_DISPLACEMENT,
            access_width=4,
            storage_identity=aggregate_identity,
        ),
        normalize_address(hex(0x425 + unit_shift)): ReviewedAbsoluteStorageLoadBridge(
            register="eax",
            aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            displacement=_cc_catalog.HUD_UI_MGR_ENSURE_SENSOR_PANEL_DISPLACEMENT,
            access_width=4,
            storage_identity=aggregate_identity,
        ),
    }
