"""Recoil call-contract recoil network evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import errors as _cc_errors
from _recoil.call_contract import extraction as _cc_extraction
from _recoil.call_contract import iat as _cc_iat
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        CandidateCallerDefinition,
        CandidateConstructorDefinition,
        IdentityIndexes,
    )

import re
import struct
from collections import Counter
from dataclasses import replace
from pathlib import Path
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_DIR32,
    IMAGE_REL_I386_REL32,
    IMAGE_SYM_CLASS_EXTERNAL,
    Instruction,
)
from _recoil.commands.provider_target_mutation import retail_import_target
from _recoil.lib.authored_icf import exact_selected_target_membership
from _recoil.lib.binja import BinaryNinjaBridge, Symbol
from _recoil.lib.progress import ProgressDocument, address_value, normalize_address
from _recoil.lib.tooling import REPO_ROOT


def _gamenet_chat_compose_candidate_register_storage_bridges(
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[dict[str, str], dict[str, str]]:
    """Prove ChatComposeKeyCallback's candidate ``esi`` vptr origin.

    Retail absolute addresses already resolve through the reviewed aggregate
    container.  The candidate bridge only maps the decorated aggregate name
    after proving the same complete caller, reaching-definition chain, call
    contract, body, and COFF relocation population.  It never supplies retail
    expected truth.
    """

    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.GAMENET_CHAT_COMPOSE_CALLER_START:
        return {}, {}

    caller_symbol_id = _cc_catalog.GAMENET_CHAT_COMPOSE_CALLER_IDENTITY.removeprefix(
        "symbol:"
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
        == _cc_catalog.GAMENET_CHAT_COMPOSE_CALLER_START
    ]
    tracker_target = document.collection("verification_targets").get(
        _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_ORDER_TARGET_ID
    )
    registration = (
        tracker_target.get("registration")
        if isinstance(tracker_target, Mapping)
        else None
    )
    if (
        caller_identity != _cc_catalog.GAMENET_CHAT_COMPOSE_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.GAMENET_CHAT_COMPOSE_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(normalized_start) != caller_identity
        or caller_identity in indexes.provider_ids
        or not isinstance(caller_row, Mapping)
        or caller_row.get("binary") != "recoil"
        or caller_row.get("kind") != "function"
        or caller_row.get("pipeline_class") != "authored"
        or caller_row.get("ownership_state") != "primary-owned"
        or caller_row.get("address") != normalized_start
        or caller_row.get("end_exclusive")
        != _cc_catalog.GAMENET_CHAT_COMPOSE_CALLER_END_EXCLUSIVE
        or caller_row.get("extent_state") != "known"
        or caller_row.get("size") != 0x40
        or caller_row.get("navigation_name")
        != "GameNet::ChatComposeKeyCallback"
        or caller_row.get("output_section_id") != "recoil:section:.text"
        or caller_row.get("physical_block_id") != "recoil:block:0x404ca0"
        or not exact_selected_target_membership(
            caller_row.get("verification_target_ids", ()),
            _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_ORDER_TARGET_ID,
        )
        or not isinstance(trace, Mapping)
        or trace.get("state") != "resolved"
        or trace.get("reason_code") not in {None, ""}
        or not isinstance(source_edges, list)
        or len(source_edges) != 1
        or not isinstance(source_edges[0], Mapping)
        or source_edges[0].get("relation") != "defines"
        or source_edges[0].get("anchor_id")
        != _cc_catalog.GAMENET_CHAT_COMPOSE_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
        or definition is None
        or definition.symbol != _cc_catalog.GAMENET_CHAT_COMPOSE_CALLER_SYMBOL
        or definition.data != _cc_catalog.GAMENET_CHAT_COMPOSE_CANDIDATE_CODE
        or len(definition.relocation_mask) != len(definition.data)
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
        or not isinstance(tracker_target, Mapping)
        or tracker_target.get("binary") != "recoil"
        or tracker_target.get("kind") != "vc5"
        or tracker_target.get("name") != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_TARGET_NAME
        or tuple(tracker_target.get("registered_addresses", ())).count(
            normalized_start
        )
        != 1
        or not isinstance(registration, Mapping)
        or registration.get("binary") != "recoil"
        or registration.get("name") != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_TARGET_NAME
        or registration.get("manifest_path")
        != str(
            _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_TARGET_MANIFEST.relative_to(REPO_ROOT)
        ).replace("\\", "/")
        or registration.get("source_from")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or registration.get("check_translation_unit_function_order")
        is not True
        or registration.get("function_order_scope") != "authored"
    ):
        raise ValueError(
            "GameNet chat-compose esi bridge requires the exact authored "
            "caller, source anchor/TU, extent, object symbol/body, and "
            "governed target authority"
        )

    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != _cc_catalog.GAMENET_CHAT_COMPOSE_CONTRIBUTION_SYMBOL_RE
        or re.fullmatch(
            _cc_catalog.GAMENET_CHAT_COMPOSE_CONTRIBUTION_SYMBOL_RE,
            definition.symbol,
        )
        is None
        or getattr(contribution_row, "name", "")
        != "GameNet::ChatComposeKeyCallback"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or getattr(contribution_row, "required_presence", None) is not True
        or getattr(contribution_row, "full_order_gate", None) is not True
    ):
        raise ValueError(
            "GameNet chat-compose esi bridge rejects exact hud.cpp "
            "contribution-row authority drift"
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
    reference = (
        aggregate_storage.get("reference")
        if isinstance(aggregate_storage, Mapping)
        else None
    )
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
    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    aggregate_start = address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
    exact_containers = [
        row
        for row in indexes.storage_containers
        if row.identity == aggregate_identity
        and row.start == aggregate_start
        and row.end_exclusive == aggregate_start + _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
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
        or _cc_catalog.GAMENET_CHAT_COMPOSE_AGGREGATE_POINTER_DISPLACEMENT + 4
        > _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
        or _cc_catalog.GAMENET_CHAT_COMPOSE_TEXT_INPUT_DISPLACEMENT + 4
        > _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
    ):
        raise ValueError(
            "GameNet chat-compose esi bridge requires the exact reviewed "
            "g_HudUiMgr aggregate symbol, storage, target, owner, gates, "
            "and bounded container authority"
        )

    def instruction_bytes(instruction: Instruction) -> bytes:
        try:
            return bytes(int(value, 16) for value in instruction.bytes)
        except (TypeError, ValueError) as exc:
            raise ValueError(
                "GameNet chat-compose esi bridge requires exact instruction bytes"
            ) from exc

    retail_addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(normalized_start),
    )
    candidate_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    expected_offsets = (
        0x00,
        0x05,
        0x07,
        0x09,
        0x0A,
        0x0B,
        0x10,
        0x15,
        0x1A,
        0x1F,
        0x21,
        0x26,
        0x2C,
        0x2D,
        0x2E,
        0x31,
        0x34,
        0x35,
    )
    retail_offsets = tuple(
        address - address_value(normalized_start)
        for address in retail_addresses
        if address is not None
    )
    if (
        retail_offsets != expected_offsets
        or tuple(offset for offset in candidate_offsets if offset is not None)
        != expected_offsets
        or b"".join(instruction_bytes(row) for row in retail_instructions)
        != _cc_catalog.GAMENET_CHAT_COMPOSE_RETAIL_CODE[:0x36]
        or b"".join(instruction_bytes(row) for row in candidate.instructions)
        != _cc_catalog.GAMENET_CHAT_COMPOSE_CANDIDATE_CODE[:0x36]
        or candidate.local_control_flow_indices
        or candidate.local_control_flow_targets
    ):
        raise ValueError(
            "GameNet chat-compose esi bridge requires the exact complete "
            "retail/candidate instruction population, bytes, and CFG"
        )

    retail_invocation_indices = tuple(
        index
        for index, instruction in enumerate(retail_instructions)
        if _cc_cfg._instruction_mnemonic(instruction) == "call"
    )
    candidate_invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    retail_invocations = tuple(
        retail_addresses[index] for index in retail_invocation_indices
    )
    candidate_invocations = tuple(
        candidate_offsets[index] for index in candidate_invocation_indices
    )
    if (
        retail_invocations != _cc_catalog.GAMENET_CHAT_COMPOSE_RETAIL_CALL_ORDER
        or candidate_invocations != _cc_catalog.GAMENET_CHAT_COMPOSE_CANDIDATE_CALL_ORDER
        or tuple(
            _cc_cfg._cleanup_after(retail_instructions, index)
            for index in retail_invocation_indices
        )
        != (None, None, None, 8)
        or tuple(
            _cc_cfg._cleanup_after(candidate.instructions, index)
            for index in candidate_invocation_indices
        )
        != (None, None, None, 8)
    ):
        raise ValueError(
            "GameNet chat-compose esi bridge rejects complete four-call "
            "population, order, form, or caller cleanup drift"
        )

    retail_by_offset = {
        offset: retail_instructions[index]
        for index, offset in enumerate(retail_offsets)
    }
    candidate_by_offset = {
        offset: candidate.instructions[index]
        for index, offset in enumerate(candidate_offsets)
        if offset is not None
    }
    for ordinal, (address, candidate_name, _retail_name) in enumerate(
        _cc_catalog.GAMENET_CHAT_COMPOSE_DIRECT_CALLS
    ):
        identity = indexes.by_address.get(address, "")
        if (
            not identity
            or identity in indexes.provider_ids
            or indexes.by_candidate_name.get(candidate_name) != identity
            or candidate_name
            not in _cc_cfg._instruction_operand(
                candidate_by_offset[
                    _cc_catalog.GAMENET_CHAT_COMPOSE_CANDIDATE_CALL_ORDER[ordinal]
                ]
            )
        ):
            raise ValueError(
                "GameNet chat-compose esi bridge requires exact authored "
                f"direct-call identity and order for {candidate_name!r}"
            )
    if (
        _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(retail_by_offset[0x2E])
        )
        not in {"esi+116", "esi+0x74"}
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(candidate_by_offset[0x2E])
        )
        not in {"esi+116", "esi+0x74"}
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(retail_by_offset[0x15]).split(",", 1)[1]
        )
        != "0x4e6844"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(retail_by_offset[0x1F]).split(",", 1)[1]
        )
        != "eax"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(retail_by_offset[0x26]).split(",", 1)[1]
        )
        != "0x4e6844"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(candidate_by_offset[0x15]).split(",", 1)[1]
        )
        not in {
            (
                f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}+"
                f"{_cc_catalog.GAMENET_CHAT_COMPOSE_AGGREGATE_POINTER_DISPLACEMENT}"
            ),
            (
                f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}+0x"
                f"{_cc_catalog.GAMENET_CHAT_COMPOSE_AGGREGATE_POINTER_DISPLACEMENT:x}"
            ),
        }
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(candidate_by_offset[0x1F]).split(",", 1)[1]
        )
        != "eax"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(candidate_by_offset[0x26]).split(",", 1)[1]
        )
        not in {
            (
                f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}+"
                f"{_cc_catalog.GAMENET_CHAT_COMPOSE_AGGREGATE_POINTER_DISPLACEMENT}"
            ),
            (
                f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}+0x"
                f"{_cc_catalog.GAMENET_CHAT_COMPOSE_AGGREGATE_POINTER_DISPLACEMENT:x}"
            ),
        }
    ):
        raise ValueError(
            "GameNet chat-compose esi bridge rejects the exact aggregate "
            "+0x974 pointer, esi vptr, slot +0x74, receiver, or argument chain"
        )

    expected_relocations = (
        (0x01, IMAGE_REL_I386_REL32, _cc_catalog.GAMENET_CHAT_COMPOSE_DIRECT_CALLS[0][1], 0),
        (
            0x0C,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.GAMENET_CHAT_COMPOSE_TEXT_INPUT_DISPLACEMENT,
        ),
        (0x11, IMAGE_REL_I386_REL32, _cc_catalog.GAMENET_CHAT_COMPOSE_DIRECT_CALLS[1][1], 0),
        (
            0x16,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.GAMENET_CHAT_COMPOSE_AGGREGATE_POINTER_DISPLACEMENT,
        ),
        (
            0x1B,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.GAMENET_CHAT_COMPOSE_TEXT_INPUT_DISPLACEMENT,
        ),
        (0x22, IMAGE_REL_I386_REL32, _cc_catalog.GAMENET_CHAT_COMPOSE_DIRECT_CALLS[2][1], 0),
        (
            0x28,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.GAMENET_CHAT_COMPOSE_AGGREGATE_POINTER_DISPLACEMENT,
        ),
    )
    actual_relocations = tuple(
        (
            row.offset,
            row.type,
            row.symbol_name,
            struct.unpack_from("<I", definition.data, row.offset)[0],
        )
        for row in definition.relocations
    )
    expected_mask = {
        index
        for offset, _type, _symbol, _addend in expected_relocations
        for index in range(offset, offset + 4)
    }
    if (
        actual_relocations != expected_relocations
        or {
            index
            for index, masked in enumerate(definition.relocation_mask)
            if masked
        }
        != expected_mask
        or definition.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in definition.defined_external_data
        or any(
            definition.undefined_external_functions.count(name) != 1
            or name in definition.defined_external_functions
            for _address, name, _retail_name in _cc_catalog.GAMENET_CHAT_COMPOSE_DIRECT_CALLS
        )
    ):
        raise ValueError(
            "GameNet chat-compose esi bridge requires the exact seven "
            "candidate DIR32/REL32 relocations, addends, masks, and external "
            "symbol provenance"
        )

    candidate_bridges = {
        _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL: aggregate_identity,
    }
    retail_bridge_names = {
        retail_name: Symbol(
            address=address,
            name=retail_name,
            raw_name=retail_name,
            full_name=retail_name,
        )
        for address, _candidate_name, retail_name in (
            _cc_catalog.GAMENET_CHAT_COMPOSE_DIRECT_CALLS
        )
    }
    expected_contract = _cc_extraction.extract_invocation_contract(
        retail_instructions,
        source="bn",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge_names=retail_bridge_names,
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
    expected_storage = (
        "load(load("
        f"{aggregate_identity}+0x"
        f"{_cc_catalog.GAMENET_CHAT_COMPOSE_AGGREGATE_POINTER_DISPLACEMENT:x}))"
    )
    if (
        expected_contract != candidate_contract
        or len(expected_contract) != 4
        or expected_contract[-1]
        != {
            "ordinal": 3,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": expected_storage,
            "slot_displacement": _cc_catalog.GAMENET_CHAT_COMPOSE_VIRTUAL_SLOT,
            "cleanup_bytes": 8,
        }
    ):
        raise ValueError(
            "GameNet chat-compose esi bridge cannot derive the exact "
            "candidate-independent retail and candidate four-call contract"
        )
    return {}, candidate_bridges


def _gamenet_end_chat_compose_strncat_register_storage_bridges(
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    reference: Path = _cc_catalog.DEFAULT_REFERENCE,
) -> tuple[dict[str, str], dict[str, str]]:
    """Prove 0x414590's two-call ``esi`` MSVCRT ``strncat`` lifetime.

    The retail IAT tuple and authored caller/TU authority are independent of
    the candidate.  Candidate COD/COFF may publish only the matching decorated
    storage name after the whole reviewed caller shape, relocation population,
    call order, cleanup, and unclobbered register lifetime converge.
    """
    from _recoil.call_contract.records import CandidateExactIatRegisterLoadProof

    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.GAMENET_END_CHAT_COMPOSE_CALLER_START:
        return {}, {}

    caller_symbol_id = _cc_catalog.GAMENET_END_CHAT_COMPOSE_CALLER_IDENTITY.removeprefix(
        "symbol:"
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
        == _cc_catalog.GAMENET_END_CHAT_COMPOSE_CALLER_START
    ]
    tracker_target = document.collection("verification_targets").get(
        _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_ORDER_TARGET_ID
    )
    registration = (
        tracker_target.get("registration")
        if isinstance(tracker_target, Mapping)
        else None
    )
    if (
        caller_identity != _cc_catalog.GAMENET_END_CHAT_COMPOSE_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.GAMENET_END_CHAT_COMPOSE_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(normalized_start) != caller_identity
        or caller_identity in indexes.provider_ids
        or not isinstance(caller_row, Mapping)
        or caller_row.get("binary") != "recoil"
        or caller_row.get("kind") != "function"
        or caller_row.get("pipeline_class") != "authored"
        or caller_row.get("ownership_state") != "primary-owned"
        or caller_row.get("address") != normalized_start
        or caller_row.get("end_exclusive")
        != _cc_catalog.GAMENET_END_CHAT_COMPOSE_CALLER_END_EXCLUSIVE
        or caller_row.get("extent_state") != "known"
        or caller_row.get("size") != 0xD0
        or caller_row.get("navigation_name")
        != "GameNet::EndChatComposeAndSend"
        or caller_row.get("output_section_id") != "recoil:section:.text"
        or caller_row.get("physical_block_id") != "recoil:block:0x404ca0"
        or not exact_selected_target_membership(
            caller_row.get("verification_target_ids", ()),
            _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_ORDER_TARGET_ID,
        )
        or not isinstance(trace, Mapping)
        or trace.get("state") != "resolved"
        or trace.get("reason_code") not in {None, ""}
        or not isinstance(source_edges, list)
        or len(source_edges) != 1
        or not isinstance(source_edges[0], Mapping)
        or source_edges[0].get("relation") != "defines"
        or source_edges[0].get("anchor_id")
        != _cc_catalog.GAMENET_END_CHAT_COMPOSE_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
        or definition is None
        or definition.symbol != _cc_catalog.GAMENET_END_CHAT_COMPOSE_CALLER_SYMBOL
        or definition.data != _cc_catalog.GAMENET_END_CHAT_COMPOSE_CANDIDATE_CODE
        or len(definition.relocation_mask) != len(definition.data)
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
        or not isinstance(tracker_target, Mapping)
        or tracker_target.get("binary") != "recoil"
        or tracker_target.get("kind") != "vc5"
        or tracker_target.get("name") != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_TARGET_NAME
        or tuple(tracker_target.get("registered_addresses", ())).count(
            normalized_start
        )
        != 1
        or not isinstance(registration, Mapping)
        or registration.get("binary") != "recoil"
        or registration.get("name") != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_TARGET_NAME
        or registration.get("manifest_path")
        != str(
            _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_TARGET_MANIFEST.relative_to(REPO_ROOT)
        ).replace("\\", "/")
        or registration.get("source_from")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or registration.get("check_translation_unit_function_order") is not True
        or registration.get("function_order_scope") != "authored"
    ):
        raise ValueError(
            "GameNet end-chat strncat esi bridge requires the exact authored "
            "caller, source anchor/TU, extent, object symbol/body, and governed "
            "target authority"
        )

    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != _cc_catalog.GAMENET_END_CHAT_COMPOSE_CONTRIBUTION_SYMBOL_RE
        or re.fullmatch(
            _cc_catalog.GAMENET_END_CHAT_COMPOSE_CONTRIBUTION_SYMBOL_RE,
            definition.symbol,
        )
        is None
        or getattr(contribution_row, "name", "")
        != "GameNet::EndChatComposeAndSend"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or getattr(contribution_row, "required_presence", None) is not True
        or getattr(contribution_row, "full_order_gate", None) is not True
    ):
        raise ValueError(
            "GameNet end-chat strncat esi bridge rejects exact hud.cpp "
            "contribution-row authority drift"
        )

    iat_identity = "iat:strncat"
    retail_import, _directory_context = retail_import_target(
        reference=reference,
        address=_cc_catalog.GAMENET_END_CHAT_COMPOSE_STRNCAT_IAT_ADDRESS,
        dll="MSVCRT.dll",
        import_name="strncat",
    )
    indexed_iat_identities = (
        indexes.storage_by_address.get(
            _cc_catalog.GAMENET_END_CHAT_COMPOSE_STRNCAT_IAT_ADDRESS
        ),
        indexes.storage_by_name.get("strncat"),
        indexes.storage_by_name.get(
            _cc_catalog.GAMENET_END_CHAT_COMPOSE_STRNCAT_CANDIDATE_SYMBOL
        ),
    )
    if (
        retail_import.address != _cc_catalog.GAMENET_END_CHAT_COMPOSE_STRNCAT_IAT_ADDRESS
        or retail_import.dll != "MSVCRT.dll"
        or retail_import.import_name != "strncat"
        or retail_import.import_ordinal is not None
        or any(
            indexed_identity is not None and indexed_identity != iat_identity
            for indexed_identity in indexed_iat_identities
        )
    ):
        raise ValueError(
            "GameNet end-chat strncat esi bridge requires the exact immutable "
            "retail MSVCRT strncat IAT tuple and rejects conflicting indexed "
            "identity"
        )

    def instruction_bytes(instruction: Instruction) -> bytes:
        try:
            return bytes(int(value, 16) for value in instruction.bytes)
        except (TypeError, ValueError) as exc:
            raise ValueError(
                "GameNet end-chat strncat esi bridge requires exact "
                "instruction bytes"
            ) from exc

    retail_addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(normalized_start),
    )
    retail_offsets = tuple(
        address - address_value(normalized_start)
        for address in retail_addresses
        if address is not None
    )
    candidate_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)

    def require_contiguous_code(
        label: str,
        instructions: Sequence[Instruction],
        offsets: Sequence[int | None],
        expected: bytes,
    ) -> None:
        if (
            len(offsets) != len(instructions)
            or any(offset is None for offset in offsets)
            or not offsets
            or offsets[0] != 0
            or any(
                int(offsets[index])
                + len(instruction_bytes(instructions[index]))
                != int(offsets[index + 1])
                for index in range(len(offsets) - 1)
            )
            or int(offsets[-1]) + len(instruction_bytes(instructions[-1]))
            != len(expected)
            or b"".join(instruction_bytes(row) for row in instructions)
            != expected
        ):
            raise ValueError(
                "GameNet end-chat strncat esi bridge requires the exact "
                f"complete contiguous {label} instruction population and bytes"
            )

    require_contiguous_code(
        "retail",
        retail_instructions,
        retail_offsets,
        _cc_catalog.GAMENET_END_CHAT_COMPOSE_RETAIL_CODE,
    )
    require_contiguous_code(
        "candidate",
        candidate.instructions,
        candidate_offsets,
        _cc_catalog.GAMENET_END_CHAT_COMPOSE_CANDIDATE_CODE[:0xC5],
    )
    if candidate.local_control_flow_indices or candidate.local_control_flow_targets:
        raise ValueError(
            "GameNet end-chat strncat esi bridge rejects candidate local-switch "
            "CFG drift"
        )

    retail_invocation_indices = tuple(
        index
        for index, instruction in enumerate(retail_instructions)
        if _cc_cfg._instruction_mnemonic(instruction) == "call"
    )
    candidate_invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    retail_invocations = tuple(
        retail_addresses[index] for index in retail_invocation_indices
    )
    candidate_invocations = tuple(
        candidate_offsets[index] for index in candidate_invocation_indices
    )
    cleanup_order = (None, None, None, 12, 12, None, 12, None, None)
    if (
        retail_invocations != _cc_catalog.GAMENET_END_CHAT_COMPOSE_RETAIL_CALL_ORDER
        or candidate_invocations
        != _cc_catalog.GAMENET_END_CHAT_COMPOSE_CANDIDATE_CALL_ORDER
        or tuple(
            _cc_cfg._cleanup_after(retail_instructions, index)
            for index in retail_invocation_indices
        )
        != cleanup_order
        or tuple(
            _cc_cfg._cleanup_after(candidate.instructions, index)
            for index in candidate_invocation_indices
        )
        != cleanup_order
    ):
        raise ValueError(
            "GameNet end-chat strncat esi bridge rejects complete nine-call "
            "population, order, form, or caller cleanup drift"
        )

    retail_by_offset = {
        offset: retail_instructions[index]
        for index, offset in enumerate(retail_offsets)
    }
    candidate_by_offset = {
        int(offset): candidate.instructions[index]
        for index, offset in enumerate(candidate_offsets)
        if offset is not None
    }
    load_and_calls = (
        (
            "retail",
            retail_instructions,
            retail_offsets,
            retail_by_offset,
            0x5B,
            (0x74, 0x9F),
            _cc_catalog.GAMENET_END_CHAT_COMPOSE_STRNCAT_IAT_ADDRESS,
            b"\x8b\x35\xf0\xc4\x4c\x00",
        ),
        (
            "candidate",
            candidate.instructions,
            candidate_offsets,
            candidate_by_offset,
            0x5F,
            (0x78, 0xA3),
            _cc_catalog.GAMENET_END_CHAT_COMPOSE_STRNCAT_CANDIDATE_SYMBOL,
            b"\x8b\x35\x00\x00\x00\x00",
        ),
    )
    for (
        label,
        instructions,
        offsets,
        by_offset,
        load_offset,
        call_offsets,
        storage_operand,
        load_body,
    ) in load_and_calls:
        load = by_offset.get(load_offset)
        calls = tuple(by_offset.get(offset) for offset in call_offsets)
        load_operand = (
            _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(load).split(",", 1)[1])
            if load is not None and "," in _cc_cfg._instruction_operand(load)
            else ""
        )
        if (
            load is None
            or _cc_cfg._instruction_mnemonic(load) != "mov"
            or not _cc_cfg._instruction_operand(load).lower().startswith("esi,")
            or load_operand != storage_operand.lower()
            or instruction_bytes(load) != load_body
            or any(call is None for call in calls)
            or any(
                _cc_cfg._instruction_mnemonic(call) != "call"
                or _cc_cfg._instruction_operand(call).lower() != "esi"
                or instruction_bytes(call) != b"\xff\xd6"
                for call in calls
                if call is not None
            )
        ):
            raise ValueError(
                "GameNet end-chat strncat esi bridge rejects exact "
                f"{label} load, storage, or two-call register form"
            )
        load_index = offsets.index(load_offset)
        last_call_index = offsets.index(call_offsets[-1])
        register_calls = [
            index
            for index, instruction in enumerate(instructions)
            if _cc_cfg._instruction_mnemonic(instruction) == "call"
            and _cc_cfg._instruction_operand(instruction).lower() == "esi"
        ]
        if (
            tuple(offsets[index] for index in register_calls) != call_offsets
            or any(
                _cc_cfg._instruction_may_clobber_register(instruction, "esi")
                for instruction in instructions[load_index + 1 : last_call_index]
                if _cc_cfg._instruction_mnemonic(instruction) != "call"
            )
        ):
            raise ValueError(
                "GameNet end-chat strncat esi bridge rejects clobbered, "
                f"ambiguous, or extra {label} register provenance"
            )

    expected_relocations = (
        (0x04, IMAGE_REL_I386_DIR32, "_g_GameStateOrMapTable", 0),
        (0x14, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0xAB4),
        (0x19, IMAGE_REL_I386_REL32, "?BindMapContextPop@zInput@@YIXXZ", 0),
        (0x1E, IMAGE_REL_I386_REL32, "?Begin@HudUiMgrObjective@@YIXXZ", 0),
        (0x23, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0xAB8),
        (0x28, IMAGE_REL_I386_REL32, "?GetBuffer@HudUiTextInput@@QAEPADXZ", 0),
        (0x4B, IMAGE_REL_I386_DIR32, "__imp__strncpy", 0),
        (
            0x61,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.GAMENET_END_CHAT_COMPOSE_STRNCAT_CANDIDATE_SYMBOL,
            0,
        ),
        (0x73, IMAGE_REL_I386_DIR32, "?g_HudUiMessage_SeparatorColon@@3PADA", 0),
        (0x93, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0xAB8),
        (0x99, IMAGE_REL_I386_REL32, "?GetBuffer@HudUiTextInput@@QAEPADXZ", 0),
        (0xB2, IMAGE_REL_I386_REL32, "?ShowChatLine@HudUi@@YIXPBDM@Z", 0),
        (
            0xBB,
            IMAGE_REL_I386_REL32,
            "?SendPkt0BChatMessage@GameNet@@YIXPBD@Z",
            0,
        ),
    )
    actual_relocations = tuple(
        (
            row.offset,
            row.type,
            row.symbol_name,
            struct.unpack_from("<I", definition.data, row.offset)[0],
        )
        for row in definition.relocations
    )
    expected_mask = {
        index
        for offset, _type, _symbol, _addend in expected_relocations
        for index in range(offset, offset + 4)
    }
    expected_defined_function_names = {
        "?Begin@HudUiMgrObjective@@YIXXZ",
        "?ShowChatLine@HudUi@@YIXPBDM@Z",
    }
    expected_undefined_function_names = {
        symbol
        for _offset, relocation_type, symbol, _addend in expected_relocations
        if relocation_type == IMAGE_REL_I386_REL32
        or symbol.startswith("__imp__")
    } - expected_defined_function_names
    expected_data_names = {
        symbol
        for _offset, relocation_type, symbol, _addend in expected_relocations
        if relocation_type == IMAGE_REL_I386_DIR32
        and not symbol.startswith("__imp__")
    }
    if (
        actual_relocations != expected_relocations
        or {
            index
            for index, masked in enumerate(definition.relocation_mask)
            if masked
        }
        != expected_mask
        or any(
            definition.undefined_external_functions.count(name) != 1
            or name in definition.defined_external_functions
            or name in definition.undefined_external_data
            or name in definition.defined_external_data
            for name in expected_undefined_function_names
        )
        or any(
            definition.defined_external_functions.count(name) != 1
            or name in definition.undefined_external_functions
            or name in definition.undefined_external_data
            or name in definition.defined_external_data
            for name in expected_defined_function_names
        )
        or any(
            definition.undefined_external_data.count(name) != 1
            or name in definition.defined_external_data
            or name in definition.undefined_external_functions
            or name in definition.defined_external_functions
            for name in expected_data_names
        )
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "GameNet end-chat strncat esi bridge requires the exact thirteen "
            "candidate DIR32/REL32 relocations, addends, masks, and external "
            "symbol provenance"
        )

    # Publish the immutable retail route only in this comparison-scoped view,
    # then require the generic candidate exact-IAT producer to re-prove the
    # COD/COFF definition and every reached register transfer.  The reviewed
    # name bridge alone must not bypass the schema-27 path-sensitive
    # multi-call, kill, join, partial-register, mixed-use, or unresolved-CFG
    # gates.
    proof_storage_by_address = dict(indexes.storage_by_address)
    proof_storage_by_name = dict(indexes.storage_by_name)
    for mapping, key in (
        (
            proof_storage_by_address,
            _cc_catalog.GAMENET_END_CHAT_COMPOSE_STRNCAT_IAT_ADDRESS,
        ),
        (proof_storage_by_name, "strncat"),
    ):
        prior = mapping.get(key)
        if prior is not None and prior != iat_identity:
            raise ValueError(
                "GameNet end-chat strncat typed proof conflicts with the "
                f"immutable retail identity for {key!r}"
            )
        mapping[key] = iat_identity
    proof_indexes = replace(
        indexes,
        storage_by_address=proof_storage_by_address,
        storage_by_name=proof_storage_by_name,
    )
    candidate_iat_proofs = _cc_iat._candidate_exact_iat_register_load_proofs(
        candidate,
        indexes=proof_indexes,
        retail_import_targets=(retail_import,),
    )
    expected_candidate_proof = CandidateExactIatRegisterLoadProof(
        definition_offset="0x5f",
        destination="esi",
        object_symbol=_cc_catalog.GAMENET_END_CHAT_COMPOSE_STRNCAT_CANDIDATE_SYMBOL,
        identity=iat_identity,
        transfer_offsets=("0x78", "0xa3"),
    )
    if candidate_iat_proofs != {"0x5f": expected_candidate_proof}:
        raise _cc_errors.CandidateCallContractEvidenceError(
            "GameNet end-chat strncat bridge requires one exact typed "
            "candidate IAT definition with its complete two-call CFG lifetime"
        )

    retail_bridges = {
        _cc_catalog.GAMENET_END_CHAT_COMPOSE_STRNCAT_IAT_ADDRESS: iat_identity,
    }
    candidate_bridges = {
        _cc_catalog.GAMENET_END_CHAT_COMPOSE_STRNCAT_CANDIDATE_SYMBOL: iat_identity,
    }
    candidate_names = dict(indexes.by_candidate_name)
    for direct_address, candidate_name in _cc_catalog.GAMENET_END_CHAT_COMPOSE_DIRECT_TARGETS:
        direct_identity = f"symbol:recoil:function:{direct_address}"
        indexed_name_identity = candidate_names.get(candidate_name)
        if (
            indexes.by_address.get(direct_address) != direct_identity
            or direct_identity in indexes.provider_ids
            or (
                indexed_name_identity is not None
                and indexed_name_identity != direct_identity
            )
        ):
            raise ValueError(
                "GameNet end-chat strncat esi bridge requires exact reviewed "
                "retail-address identities for every candidate direct-call name"
            )
        candidate_names[candidate_name] = direct_identity
    candidate_indexes = replace(
        proof_indexes,
        by_candidate_name=candidate_names,
    )
    expected_contract = _cc_extraction.extract_invocation_contract(
        retail_instructions,
        source="bn",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge_names=bridge_names,
        reviewed_register_storage_bridges=retail_bridges,
    )
    candidate_contract = _cc_extraction.extract_invocation_contract(
        candidate.instructions,
        source="cod",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=candidate_indexes,
        bridge_names=bridge_names,
        reviewed_register_storage_bridges=candidate_bridges,
        reviewed_iat_register_definition_offsets=frozenset(
            candidate_iat_proofs
        ),
        candidate_exact_iat_register_load_proofs=candidate_iat_proofs,
        local_control_flow_indices=candidate.local_control_flow_indices,
        local_control_flow_targets=candidate.local_control_flow_targets,
    )
    required_rows = {
        ordinal: {
            "ordinal": ordinal,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "iat",
            "target_identity": iat_identity,
            "storage_identity": iat_identity,
            "slot_displacement": None,
            "cleanup_bytes": 12,
        }
        for ordinal in (4, 6)
    }
    if (
        expected_contract != candidate_contract
        or len(expected_contract) != 9
        or any(
            expected_contract[ordinal] != required_row
            for ordinal, required_row in required_rows.items()
        )
        or sum(
            row.get("target_identity") == iat_identity
            for row in expected_contract
        )
        != 2
        or sum(
            row.get("target_identity") == iat_identity
            for row in candidate_contract
        )
        != 2
    ):
        raise ValueError(
            "GameNet end-chat strncat esi bridge cannot derive the exact "
            "candidate-independent retail and candidate nine-call contract"
        )
    return retail_bridges, candidate_bridges


def _znetwork_open_selected_session_retail_switch_targets(
    retail_instructions: Sequence[Instruction],
    *,
    caller_start: str,
    caller_end_exclusive: str,
    caller_execution_end_exclusive: str,
    bridge: BinaryNinjaBridge,
) -> dict[int, tuple[int, ...]]:
    """Publish the exact attached-table successors for OpenSelectedSession."""

    start = normalize_address(caller_start)
    if start != _cc_catalog._ZNETWORK_OPEN_SELECTED_SESSION_START:
        return {}
    execution_end = _cc_identity._retail_call_contract_comparison_end_exclusive(
        retail_instructions,
        caller_start=start,
        caller_end_exclusive=caller_end_exclusive,
    )
    if (
        execution_end != normalize_address(caller_execution_end_exclusive)
        or execution_end != _cc_catalog._ZNETWORK_OPEN_SELECTED_SESSION_EXECUTION_END
    ):
        raise ValueError(
            "zNetwork OpenSelectedSession switch proof execution extent drifted"
        )

    addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(start),
    )
    positions: dict[str, list[tuple[int, Instruction]]] = {}
    for index, (address, instruction) in enumerate(
        zip(addresses, retail_instructions)
    ):
        if address is not None:
            positions.setdefault(normalize_address(address), []).append(
                (index, instruction)
            )

    required_rows = {
        "0x48a7a8": bytes.fromhex("33 d2"),
        "0x48a7aa": bytes.fromhex("8a 90 2c a9 48 00"),
        "0x48a7b0": bytes.fromhex("ff 24 95 0c a9 48 00"),
    }
    for address, expected_body in required_rows.items():
        rows = positions.get(address, [])
        try:
            body = (
                bytes(int(item, 16) for item in rows[0][1].bytes)
                if len(rows) == 1
                else b""
            )
        except (TypeError, ValueError):
            body = b""
        if body != expected_body:
            raise ValueError(
                "zNetwork OpenSelectedSession switch proof exact instruction "
                f"drifted at {address}"
            )

    dispatch_index, dispatch = positions["0x48a7b0"][0]
    if (
        _cc_cfg._instruction_mnemonic(dispatch) != "jmp"
        or _cc_cfg._instruction_operand(dispatch).strip().casefold()
        != "dword [edx*4+0x48a90c]"
    ):
        raise ValueError(
            "zNetwork OpenSelectedSession switch proof dispatch rendering drifted"
        )

    tail = _cc_cfg._hexdump_bytes(
        bridge.hexdump(
            _cc_catalog._ZNETWORK_OPEN_SELECTED_SESSION_EXECUTION_END,
            address_value(_cc_catalog._ZNETWORK_OPEN_SELECTED_SESSION_ENVELOPE_END)
            - address_value(_cc_catalog._ZNETWORK_OPEN_SELECTED_SESSION_EXECUTION_END),
        )
    )
    if tail != _cc_catalog._ZNETWORK_OPEN_SELECTED_SESSION_TAIL:
        raise ValueError(
            "zNetwork OpenSelectedSession attached alignment, jump table, "
            "case map, or padding drifted"
        )
    table_targets = struct.unpack_from("<8I", tail, 3)
    expected_targets = (
        0x48A7F2,
        0x48A810,
        0x48A869,
        0x48A7D4,
        0x48A82D,
        0x48A7B7,
        0x48A84B,
        0x48A89A,
    )
    target_indices = tuple(
        positions.get(normalize_address(target), [(-1, dispatch)])[0][0]
        if len(positions.get(normalize_address(target), [])) == 1
        else -1
        for target in table_targets
    )
    if (
        table_targets != expected_targets
        or any(index <= dispatch_index for index in target_indices)
    ):
        raise ValueError(
            "zNetwork OpenSelectedSession attached switch target population "
            "drifted"
        )
    return {dispatch_index: target_indices}


def _mission_reviewed_ordinal_profile(
    *,
    callable_symbol: str,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    provider_identity: str,
    instruction_offsets: Sequence[str],
    candidate_rows: Sequence[Mapping[str, Any]],
    expected_rows: Sequence[Mapping[str, Any]],
) -> bool:
    """Recognize one complete Mission provider-call population reshape."""

    profiles = {
        "?SetWindowTextA@CWnd@@QAEXPBD@Z": (
            "provider:recoil:function:0x4c5c84",
            ("0x17c", "0x194", "0x1f2", "0x204"),
            (15, 17, 23, 25),
            (16, 18, 22, 24),
        ),
        "?EnableWindow@CWnd@@QAEHH@Z": (
            "provider:recoil:function:0x4c5c8a",
            ("0x10a", "0x117", "0x16a", "0x19d", "0x1dc"),
            (8, 9, 13, 18, 21),
            (8, 9, 14, 20, 25),
        ),
    }
    profile = profiles.get(callable_symbol)
    return bool(
        profile is not None
        and caller_identity == "symbol:recoil:function:0x41b2f0"
        and normalize_address(caller_start) == "0x41b2f0"
        and normalize_address(caller_end_exclusive) == "0x41b510"
        and provider_identity == profile[0]
        and tuple(instruction_offsets) == profile[1]
        and tuple(row.get("ordinal") for row in candidate_rows)
        == profile[2]
        and tuple(row.get("ordinal") for row in expected_rows)
        == profile[3]
        and len(candidate_rows) == len(expected_rows) == len(profile[1])
        and all(
            {
                key: value
                for key, value in candidate_row.items()
                if key != "ordinal"
            }
            == {
                key: value
                for key, value in expected_row.items()
                if key != "ordinal"
            }
            for candidate_row, expected_row in zip(
                candidate_rows, expected_rows
            )
        )
    )


def _mission_exact_relocations_with_local_eh(
    definition: CandidateCallerDefinition | CandidateConstructorDefinition,
    expected: Sequence[tuple[int, int, str]],
    *,
    eh_target_offset: int,
) -> bool:
    """Compare exact relocations while binding one numeric EH local by value."""

    observed = definition.relocations
    exact_mask = tuple(
        any(offset <= index < offset + 4 for offset, _type, _name in expected)
        for index in range(len(definition.data))
    )
    if (
        len(observed) != len(expected)
        or definition.section_index <= 0
        or definition.relocation_mask != exact_mask
    ):
        return False
    symbol_by_index = {row.index: row for row in definition.coff_symbols}
    if len(symbol_by_index) != len(definition.coff_symbols):
        return False
    for relocation, (offset, relocation_type, symbol_name) in zip(
        observed, expected
    ):
        if relocation.offset != offset or relocation.type != relocation_type:
            return False
        if offset != 0x03:
            if relocation.symbol_name != symbol_name:
                return False
            continue
        target = symbol_by_index.get(relocation.symbol_index)
        associated = tuple(
            row for row in definition.associated_sections
            if target is not None
            and row.section_index == target.section_number
            and row.association_section_index == definition.section_index
            and row.name == ".text$x"
        )
        if (
            _cc_catalog.MISSION_COMPILER_LOCAL_EH_SYMBOL_RE.fullmatch(symbol_name) is None
            or _cc_catalog.MISSION_COMPILER_LOCAL_EH_SYMBOL_RE.fullmatch(
                relocation.symbol_name
            ) is None
            or target is None
            or target.name != relocation.symbol_name
            or target.value != eh_target_offset
            or target.section_number == definition.section_index
            or target.symbol_type != 0
            or target.storage_class != 6
            or target.aux_count != 0
            or sum(
                row.name == relocation.symbol_name
                for row in definition.coff_symbols
            ) != 1
            or len(associated) != 1
            or associated[0].section_size != len(associated[0].data)
            or not 0 <= target.value < associated[0].section_size
            or sum(
                row.name == target.name
                and row.value == target.value
                and row.section_number == target.section_number
                and row.type == target.symbol_type
                and row.storage_class == target.storage_class
                for row in associated[0].symbols
            ) != 1
            or definition.data[offset : offset + 4] != b"\0" * 4
            or not all(definition.relocation_mask[offset : offset + 4])
        ):
            return False
    return True


def _mission_net_game_setup_candidate_shape(
    candidate: CandidateAssembly,
) -> str | None:
    """Return the exact current or observed retail-convergent 0x419aa0 shape."""

    definition = candidate.caller_definition
    if (
        definition is None
        or definition.symbol != _cc_catalog.HUD_NET_GAME_SETUP_CONSTRUCTOR_CALLER_SYMBOL
        or definition.section_start != 0
        or candidate.local_control_flow_indices
        or candidate.local_control_flow_targets
    ):
        return None

    current_body = _cc_catalog.HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY
    current_relocations = _cc_catalog.HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS
    current_offsets = _cc_catalog.HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_OFFSETS
    current_mnemonics = _cc_catalog.HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_MNEMONICS
    logical_end = 0x643
    observed_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    observed_mnemonics = tuple(
        _cc_cfg._instruction_mnemonic(instruction)
        for instruction in candidate.instructions
    )
    retained_body = next(
        (
            body
            for body in (
                _cc_catalog.HUD_NET_GAME_SETUP_RETAINED_RETAIL_BODY,
                _cc_catalog.HUD_NET_GAME_SETUP_CURRENT_SOURCE_RETAIL_CONVERGENT_BODY,
            )
            if definition.data == body
        ),
        b"",
    )
    retained_relocations = _cc_catalog.HUD_NET_GAME_SETUP_RETAINED_RETAIL_RELOCATIONS
    retained_invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start="0x0",
        caller_end_exclusive=hex(len(retained_body)),
    )
    retained_call_offsets = tuple(
        observed_offsets[index] for index in retained_invocation_indices
    )
    try:
        retained_cod_bytes_match = all(
            offset is not None
            and bytes(int(value, 16) for value in instruction.bytes)
            == retained_body[offset:offset + len(instruction.bytes)]
            for instruction, offset in zip(
                candidate.instructions, observed_offsets
            )
        )
    except (TypeError, ValueError):
        retained_cod_bytes_match = False
    if (
        definition.section_end == len(retained_body)
        and definition.data == retained_body
        and len(candidate.instructions) == 386
        and len(observed_offsets) == 386
        and all(offset is not None for offset in observed_offsets)
        and tuple(sorted(observed_offsets)) == observed_offsets
        and len(set(observed_offsets)) == len(observed_offsets)
        and observed_offsets[0] == 0
        and all(
            offset + len(instruction.bytes) == next_offset
            for instruction, offset, next_offset in zip(
                candidate.instructions,
                observed_offsets,
                observed_offsets[1:],
            )
        )
        and observed_offsets[-1] + len(candidate.instructions[-1].bytes)
        == len(retained_body)
        and retained_cod_bytes_match
        and retained_call_offsets
        == _cc_catalog.HUD_NET_GAME_SETUP_RETAINED_RETAIL_CALL_OFFSETS
        and _mission_exact_relocations_with_local_eh(
            definition,
            retained_relocations,
            eh_target_offset=0x112,
        )
        and all(
            struct.unpack_from("<I", definition.data, offset)[0] == 0
            for offset, _relocation_type, _symbol_name
            in retained_relocations
        )
        and _cc_cfg._instruction_mnemonic(candidate.instructions[-1]) == "ret"
        and bytes(
            int(value, 16) for value in candidate.instructions[-1].bytes
        ) == b"\xc2\x04\0"
    ):
        return "retained-retail-convergent"

    current_instruction_ends = current_offsets[1:] + (logical_end,)
    try:
        current_instruction_bytes = all(
            bytes(int(value, 16) for value in instruction.bytes)
            == current_body[offset:instruction_end]
            for instruction, offset, instruction_end in zip(
                candidate.instructions, current_offsets, current_instruction_ends
            )
        )
    except (TypeError, ValueError):
        current_instruction_bytes = False
    if (
        definition.section_end == len(current_body)
        and definition.data == current_body
        and observed_offsets == current_offsets
        and observed_mnemonics == current_mnemonics
        and len(candidate.instructions) == len(current_offsets)
        and current_instruction_bytes
        and _mission_exact_relocations_with_local_eh(
            definition,
            current_relocations,
            eh_target_offset=0x112,
        )
        and all(
            struct.unpack_from("<I", definition.data, offset)[0] == 0
            for offset, _relocation_type, _symbol_name in current_relocations
        )
        and definition.data[logical_end:] == b"\x90" * 13
    ):
        return "current"

    # No prefix/tail or call-deletion projection is admissible here.  A source
    # form not represented by one of the complete body/COD/relocation profiles
    # above remains unresolved and is compared without normalization.
    return None




def _mission_connect_provider_load_schedule_is_exact(
    candidate: CandidateAssembly,
    definition: CandidateCallerDefinition,
) -> bool:
    """Accept either invocation-neutral provider/IAT load ordering.

    Both schedules establish the same ESI provider record and EDI
    ``SendMessageA`` IAT value before the first subsequent invocation.  Exact
    instruction coordinates, bytes, operands, DIR32 target, zero addend, and
    relocation mask distinguish the two finite forms.
    """

    schedules = {
        _cc_catalog.MISSION_CONNECT_PROVIDER_OLD_LOAD_WINDOW: (
            (
                0x127,
                b"\x8b\x3d\0\0\0\0",
                "edi, dword __imp__SendMessageA@16",
            ),
            (
                0x12D,
                b"\x8b\x74\x24\x10",
                "esi, dword _providerInfo$[esp+532]",
            ),
            0x129,
        ),
        _cc_catalog.MISSION_CONNECT_PROVIDER_CURRENT_LOAD_WINDOW: (
            (
                0x127,
                b"\x8b\x74\x24\x10",
                "esi, dword _providerInfo$[esp+532]",
            ),
            (
                0x12B,
                b"\x8b\x3d\0\0\0\0",
                "edi, dword __imp__SendMessageA@16",
            ),
            0x12D,
        ),
    }
    window = definition.data[0x127:0x131]
    schedule = schedules.get(window)
    if schedule is None:
        return False
    first, second, relocation_offset = schedule
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    index_by_offset = {
        int(offset): index for index, offset in enumerate(offsets)
        if offset is not None
    }
    for offset, body, operand in (first, second):
        index = index_by_offset.get(offset)
        instruction = (
            candidate.instructions[index] if index is not None else None
        )
        try:
            cod_body = (
                bytes(int(value, 16) for value in instruction.bytes)
                if instruction is not None else b""
            )
        except (TypeError, ValueError):
            cod_body = b""
        if (
            instruction is None
            or cod_body != body
            or _cc_cfg._instruction_operand(instruction).strip() != operand
            or definition.data[offset : offset + len(body)] != body
        ):
            return False
    window_relocations = tuple(
        row for row in definition.relocations
        if 0x127 <= row.offset < 0x131
    )
    if len(window_relocations) != 1:
        return False
    relocation = window_relocations[0]
    symbol_by_index = {row.index: row for row in definition.coff_symbols}
    target = symbol_by_index.get(relocation.symbol_index)
    return (
        len(symbol_by_index) == len(definition.coff_symbols)
        and relocation.offset == relocation_offset
        and relocation.type == IMAGE_REL_I386_DIR32
        and relocation.symbol_name == "__imp__SendMessageA@16"
        and target is not None
        and target.name == relocation.symbol_name
        and target.value == 0
        and target.section_number == 0
        and target.symbol_type == 0x20
        and target.storage_class == IMAGE_SYM_CLASS_EXTERNAL
        and target.aux_count == 0
        and sum(
            row.name == relocation.symbol_name
            for row in definition.coff_symbols
        ) == 1
        and definition.data[
            relocation_offset:relocation_offset + 4
        ] == b"\0" * 4
        and all(
            definition.relocation_mask[
                relocation_offset:relocation_offset + 4
            ]
        )
        and {
            index for index in range(0x127, 0x131)
            if definition.relocation_mask[index]
        } == set(range(relocation_offset, relocation_offset + 4))
    )


def _mission_current_artifact_invocation_population_contract(
    expected: Sequence[Mapping[str, Any]],
    candidate_contract: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
) -> list[dict[str, Any]]:
    """Authenticate the finite connect-provider invocation population.

    The connect-provider caller admits only the coherent complete current
    source artifact and returns its raw rows unchanged. No permutation of
    candidate rows into retail order is allowed. The profile is gated by the
    exact caller body, complete relocation inventory,
    relocation mask, and complete COD invocation coordinates.
    """

    profiles = {
        (
            "symbol:recoil:function:0x41b2f0",
            "0x41b2f0",
            "0x41b510",
        ): (
            "?ConnectSelectedProvider@NetSessionBrowserDialog@@QAEXXZ",
            bytes.fromhex(
                "81ec04020000538bd955568b4320576a0250ff15000000008bab900100008b3d000000006a006a00684701000055ffd7"
                "83f8ff0f84d70100006a0050685001000055ffd78bf085f6897424100f84b00000008b4e10680000000051ff15000000"
                "0083c40885c00f84c5000000a10000000085c00f85b8000000b912000000c7050000000001000000e8000000008bf8"
                "83c9ff33c08d942414010000f2aef7d12bf98bc18bf78bfac1e902f3a58bc883e103f3a4b926000000e8000000008bf8"
                "83c9ff33c08d542414f2aef7d12bf98bc18bf78bfa8d542414c1e902f3a58bc883e103f3a48d8c2414010000e8000000"
                "0085c075335050684e01000055ff15000000006a008d8bb0000000e8000000006a008d8bf0000000e8000000005f5e5d"
                "5b81c404020000c38b3d000000008b7424108bcee8000000008b4e10680000000051ff150000000083c40885c074668b"
                "93500100006a006a00688401000052ffd78db3b00000006a018bcee800000000b935000000e800000000508bcee8000000"
                "00b9360000008db3f0000000e800000000508bcee8000000006a018bcee800000000c7436c010000005f5e5d5b81c404"
                "020000c38bcbe80000000085c07c138b43206a0068e80300006a0250ff15000000008db3f00000006a018bcee8000000"
                "00b937000000e800000000508d8bb0000000e800000000b938000000e800000000508bcee800000000c7436c00000000"
                "5f5e5d5b81c404020000c39090909090"
            ),
            (
                0x12, 0x2E, 0x42, 0x5B, 0x88, 0xB8, 0xEB, 0xFC,
                0x10A, 0x117, 0x133, 0x141, 0x15E, 0x16A, 0x174,
                0x17C, 0x18C, 0x194, 0x19D, 0x1B6, 0x1CC, 0x1DC,
                0x1E6, 0x1F2, 0x1FC, 0x204,
            ),
            (
                (0x14, IMAGE_REL_I386_DIR32, "__imp__KillTimer@8"),
                (0x20, IMAGE_REL_I386_DIR32, "__imp__SendMessageA@16"),
                (0x56, IMAGE_REL_I386_DIR32, "_g_zNetwork_ProviderName_TcpIp"),
                (0x5D, IMAGE_REL_I386_DIR32, "__imp__strstr"),
                (0x6D, IMAGE_REL_I386_DIR32, "_g_NetUiTcpIpProviderWarningShown"),
                (0x80, IMAGE_REL_I386_DIR32, "_g_NetUiTcpIpProviderWarningShown"),
                (0x89, IMAGE_REL_I386_REL32, "?GetMessageString@zLoc@@YIPADI@Z"),
                (0xB9, IMAGE_REL_I386_REL32, "?GetMessageString@zLoc@@YIPADI@Z"),
                (0xEC, IMAGE_REL_I386_REL32, "?VerifyWinsock2OrPromptContinue@NetUi@@YIHPBD0@Z"),
                (0xFE, IMAGE_REL_I386_DIR32, "__imp__SendMessageA@16"),
                (0x10B, IMAGE_REL_I386_REL32, "?EnableWindow@CWnd@@QAEHH@Z"),
                (0x118, IMAGE_REL_I386_REL32, "?EnableWindow@CWnd@@QAEHH@Z"),
                (0x129, IMAGE_REL_I386_DIR32, "__imp__SendMessageA@16"),
                (0x134, IMAGE_REL_I386_REL32, "?SelectServiceProviderAndInitConnection@zNetworkDPlay@@YIHPAUzNetworkDPlayServiceProviderInfo@@@Z"),
                (0x13C, IMAGE_REL_I386_DIR32, "_g_zNetwork_ProviderName_Modem"),
                (0x143, IMAGE_REL_I386_DIR32, "__imp__strstr"),
                (0x16B, IMAGE_REL_I386_REL32, "?EnableWindow@CWnd@@QAEHH@Z"),
                (0x175, IMAGE_REL_I386_REL32, "?GetMessageString@zLoc@@YIPADI@Z"),
                (0x17D, IMAGE_REL_I386_REL32, "?SetWindowTextA@CWnd@@QAEXPBD@Z"),
                (0x18D, IMAGE_REL_I386_REL32, "?GetMessageString@zLoc@@YIPADI@Z"),
                (0x195, IMAGE_REL_I386_REL32, "?SetWindowTextA@CWnd@@QAEXPBD@Z"),
                (0x19E, IMAGE_REL_I386_REL32, "?EnableWindow@CWnd@@QAEHH@Z"),
                (0x1B7, IMAGE_REL_I386_REL32, "?RefreshSessionList@NetSessionBrowserDialog@@QAEHXZ"),
                (0x1CE, IMAGE_REL_I386_DIR32, "__imp__SetTimer@16"),
                (0x1DD, IMAGE_REL_I386_REL32, "?EnableWindow@CWnd@@QAEHH@Z"),
                (0x1E7, IMAGE_REL_I386_REL32, "?GetMessageString@zLoc@@YIPADI@Z"),
                (0x1F3, IMAGE_REL_I386_REL32, "?SetWindowTextA@CWnd@@QAEXPBD@Z"),
                (0x1FD, IMAGE_REL_I386_REL32, "?GetMessageString@zLoc@@YIPADI@Z"),
                (0x205, IMAGE_REL_I386_REL32, "?SetWindowTextA@CWnd@@QAEXPBD@Z"),
            ),
        ),
    }
    key = (
        caller_identity,
        normalize_address(caller_start),
        normalize_address(caller_end_exclusive),
    )
    profile = profiles.get(key)
    result = [dict(row) for row in candidate_contract]
    if profile is None:
        return result

    caller_symbol, exact_body, exact_call_offsets, exact_relocations = profile
    definition = candidate.caller_definition
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start="0x0",
        caller_end_exclusive=hex(len(exact_body)),
    )
    instruction_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    observed_call_offsets = tuple(
        instruction_offsets[index] for index in invocation_indices
    )
    observed_relocations = tuple(
        (row.offset, row.type, row.symbol_name)
        for row in definition.relocations
    ) if definition is not None else ()
    connect_profile = key == (
        "symbol:recoil:function:0x41b2f0",
        "0x41b2f0",
        "0x41b510",
    )
    if connect_profile:
        # WSI-20260823-021: authenticate only the one complete artifact emitted
        # by the current non-modem-first source.  The former adapter combined
        # stale and current load windows with independently spliced relocation
        # tuples, including an impossible current body/+0x129 DIR32 pairing.
        # These constants were frozen from the fresh governed Mission object
        # and COD listing; no candidate row is deleted or derived from retail.
        exact_body = _cc_catalog.MISSION_CONNECT_PROVIDER_CURRENT_BODY
        exact_call_offsets = _cc_catalog.MISSION_CONNECT_PROVIDER_CURRENT_CALL_OFFSETS
        exact_relocations = _cc_catalog.MISSION_CONNECT_PROVIDER_CURRENT_RELOCATIONS
    observed_cod_rows = tuple(
        (offset, _cc_cfg._instruction_mnemonic(instruction))
        for offset, instruction in zip(
            instruction_offsets, candidate.instructions
        )
    )
    resolved_cod_offsets = tuple(
        0 if index == 0 and offset is None else offset
        for index, offset in enumerate(instruction_offsets)
    )
    try:
        complete_cod_bytes_match = bool(
            definition is not None
            and all(isinstance(offset, int) for offset in resolved_cod_offsets)
            and all(
                offset + len(instruction.bytes) == next_offset
                for instruction, offset, next_offset in zip(
                    candidate.instructions,
                    resolved_cod_offsets,
                    resolved_cod_offsets[1:],
                )
            )
            and resolved_cod_offsets[-1]
            + len(candidate.instructions[-1].bytes)
            <= len(definition.data)
            and all(
                bytes(int(value, 16) for value in instruction.bytes)
                == definition.data[offset:offset + len(instruction.bytes)]
                for instruction, offset in zip(
                    candidate.instructions, resolved_cod_offsets
                )
            )
            and definition.data[
                resolved_cod_offsets[-1]
                + len(candidate.instructions[-1].bytes):
            ]
            == b"\x90" * (
                len(definition.data)
                - resolved_cod_offsets[-1]
                - len(candidate.instructions[-1].bytes)
            )
            and len(definition.data)
            - resolved_cod_offsets[-1]
            - len(candidate.instructions[-1].bytes)
            < 0x10
        )
    except (IndexError, TypeError, ValueError):
        complete_cod_bytes_match = False
    body_profiles = (exact_body,)
    call_offset_profiles = (exact_call_offsets,)
    relocation_profiles = (exact_relocations,)
    cod_profiles = (_cc_catalog.MISSION_CONNECT_PROVIDER_CURRENT_COD_ROWS,)
    if connect_profile:
        body_profiles += (_cc_catalog.MISSION_CONNECT_PROVIDER_IAT_FIRST_BODY,)
        call_offset_profiles += (exact_call_offsets,)
        relocation_profiles += (
            _cc_catalog.MISSION_CONNECT_PROVIDER_IAT_FIRST_RELOCATIONS,
        )
        cod_profiles += (_cc_catalog.MISSION_CONNECT_PROVIDER_IAT_FIRST_COD_ROWS,)
    matched_profile_index = next(
        (
            index for index, (body, relocations) in enumerate(
                zip(body_profiles, relocation_profiles)
            )
            if definition is not None
            and definition.data == body
            and observed_relocations == relocations
        ),
        None,
    )
    matched_relocations = (
        relocation_profiles[matched_profile_index]
        if matched_profile_index is not None else exact_relocations
    )
    matched_call_offsets = (
        call_offset_profiles[matched_profile_index]
        if matched_profile_index is not None else exact_call_offsets
    )
    exact_mask = tuple(
        any(
            offset <= index < offset + 4
            for offset, _type, _name in matched_relocations
        )
        for index in range(len(exact_body))
    )
    contract_fields = (
        "form", "dispatch", "identity_kind", "target_identity",
        "storage_identity", "slot_displacement", "cleanup_bytes",
    )
    expected_population = Counter(
        tuple(row.get(field) for field in contract_fields) for row in expected
    )
    candidate_population = Counter(
        tuple(row.get(field) for field in contract_fields) for row in result
    )
    if (
        definition is None
        or definition.symbol != caller_symbol
        or definition.section_start != 0
        or definition.section_end != len(exact_body)
        or matched_profile_index is None
        or definition.relocation_mask != exact_mask
        or observed_call_offsets != matched_call_offsets
        or (
            connect_profile
            and (
                observed_cod_rows
                != cod_profiles[matched_profile_index]
                or not complete_cod_bytes_match
            )
        )
        or not expected
        or len(expected) != len(result)
        or tuple(row.get("ordinal") for row in expected) != tuple(range(len(expected)))
        or tuple(row.get("ordinal") for row in result) != tuple(range(len(result)))
        or expected_population != candidate_population
        or (
            connect_profile
            and not _mission_connect_provider_load_schedule_is_exact(
                candidate, definition
            )
        )
    ):
        raise ValueError(
            "Mission invocation-population profile rejects caller, exact VC5 "
            "body/relocations/COD sites, or immutable-retail multiset drift"
        )
    return result
