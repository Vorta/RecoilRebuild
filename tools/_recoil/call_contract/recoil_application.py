"""Recoil call-contract recoil application evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import errors as _cc_errors
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract import source as _cc_source
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        AppFrameRunCatchFuncletProof,
        CandidateAssembly,
        CandidateCoffSymbolDefinition,
        IdentityIndexes,
        ReviewedExactIndirectStorageBridge,
        ReviewedLoopVptrStorageBridge,
    )

import re
import struct
from pathlib import Path
from typing import Any, Mapping, NoReturn, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_DIR32,
    IMAGE_REL_I386_REL32,
    IMAGE_SYM_CLASS_EXTERNAL,
    Instruction,
)
from _recoil.commands.vc5_verify import effective_source_compile_context
from _recoil.lib.authored_icf import exact_required_target_membership
from _recoil.lib.binja import BinaryNinjaBridge
from _recoil.lib.progress import ProgressDocument, address_value, normalize_address
from _recoil.lib.tooling import REPO_ROOT


def _recoil_main_menu_transition_blur_begin_eax_bridge(
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge: BinaryNinjaBridge,
) -> tuple[
    dict[str, ReviewedLoopVptrStorageBridge],
    dict[str, ReviewedLoopVptrStorageBridge],
]:
    """Prove the first stack-local blur Begin and Update vptr sources.

    This is a caller-specific immutable-retail exception.  The bridge is
    published only after the complete caller, its unique EAX lifetime, the
    constructor/vtable/Begin tuple, and the fresh candidate COD/COFF
    population have all matched their independently reviewed shapes.
    """
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge

    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_CALLER_START:
        return {}, {}

    caller_symbol_id = (
        _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_CALLER_IDENTITY.removeprefix("symbol:")
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
        == _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_CALLER_START
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
        caller_identity != _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(normalized_start) != caller_identity
        or caller_identity in indexes.provider_ids
        or not isinstance(caller_row, Mapping)
        or caller_row.get("binary") != "recoil"
        or caller_row.get("kind") != "function"
        or caller_row.get("pipeline_class") != "authored"
        or caller_row.get("ownership_state") != "primary-owned"
        or caller_row.get("address") != normalized_start
        or caller_row.get("end_exclusive")
        != _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_CALLER_END_EXCLUSIVE
        or caller_row.get("extent_state") != "known"
        or caller_row.get("size") != 0x150
        or caller_row.get("navigation_name")
        != "RecoilStateMainMenuTransition::OnTryBecomeCurrent"
        or caller_row.get("output_section_id") != "recoil:section:.text"
        or caller_row.get("physical_block_id") != "recoil:block:0x404ca0"
        or not exact_required_target_membership(
            caller_row.get("verification_target_ids", ()),
            _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_VERIFICATION_TARGET_IDS,
        )
        or not isinstance(trace, Mapping)
        or trace.get("state") != "resolved"
        or trace.get("reason_code") not in {None, ""}
        or not isinstance(source_edges, list)
        or len(source_edges) != 1
        or not isinstance(source_edges[0], Mapping)
        or source_edges[0].get("relation") != "defines"
        or source_edges[0].get("anchor_id")
        != _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
        or definition is None
        or definition.symbol != _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_CALLER_SYMBOL
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
            "Recoil main-menu transition EAX bridge requires the exact "
            "authored caller, source anchor/TU, extent, object identity, and "
            "governed target authority"
        )

    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_CONTRIBUTION_SYMBOL_RE
        or re.fullmatch(
            _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_CONTRIBUTION_SYMBOL_RE,
            definition.symbol,
        )
        is None
        or getattr(contribution_row, "name", "")
        != "RecoilStateMainMenuTransition::OnTryBecomeCurrent"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or getattr(contribution_row, "required_presence", None) is not True
        or getattr(contribution_row, "full_order_gate", None) is not True
    ):
        raise ValueError(
            "Recoil main-menu transition EAX bridge rejects exact hud.cpp "
            "contribution-row authority drift"
        )

    begin_identity = (
        f"symbol:recoil:function:{_cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_BEGIN_ADDRESS}"
    )
    update_identity = (
        f"symbol:recoil:function:{_cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_UPDATE_ADDRESS}"
    )
    end_identity = (
        f"symbol:recoil:function:{_cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_END_ADDRESS}"
    )
    constructor_identity = (
        f"symbol:recoil:function:{_cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_CTOR_ADDRESS}"
    )
    begin_row = document.collection("symbols").get(
        begin_identity.removeprefix("symbol:")
    )
    update_row = document.collection("symbols").get(
        update_identity.removeprefix("symbol:")
    )
    end_row = document.collection("symbols").get(
        end_identity.removeprefix("symbol:")
    )
    if (
        indexes.by_address.get(_cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_BEGIN_ADDRESS)
        != begin_identity
        or indexes.by_address.get(
            _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_UPDATE_ADDRESS
        )
        != update_identity
        or indexes.by_address.get(_cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_END_ADDRESS)
        != end_identity
        or indexes.by_address.get(_cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_CTOR_ADDRESS)
        != constructor_identity
        or begin_identity in indexes.provider_ids
        or update_identity in indexes.provider_ids
        or end_identity in indexes.provider_ids
        or constructor_identity in indexes.provider_ids
        or not isinstance(begin_row, Mapping)
        or begin_row.get("binary") != "recoil"
        or begin_row.get("kind") != "function"
        or begin_row.get("address")
        != _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_BEGIN_ADDRESS
        or begin_row.get("end_exclusive") != "0x463920"
        or begin_row.get("extent_state") != "known"
        or begin_row.get("size") != 0xB0
        or begin_row.get("pipeline_class") != "authored"
        or begin_row.get("ownership_state") != "primary-owned"
        or begin_row.get("navigation_name") != "zFMV_ActionBlur::Begin"
        or not isinstance(update_row, Mapping)
        or update_row.get("binary") != "recoil"
        or update_row.get("kind") != "function"
        or update_row.get("address")
        != _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_UPDATE_ADDRESS
        or update_row.get("end_exclusive") != "0x4639e0"
        or update_row.get("extent_state") != "known"
        or update_row.get("size") != 0x90
        or update_row.get("pipeline_class") != "authored"
        or update_row.get("ownership_state") != "primary-owned"
        or update_row.get("navigation_name") != "zFMV_ActionBlur::Update"
        or not isinstance(end_row, Mapping)
        or end_row.get("binary") != "recoil"
        or end_row.get("kind") != "function"
        or end_row.get("address")
        != _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_END_ADDRESS
        or end_row.get("end_exclusive") != "0x463950"
        or end_row.get("extent_state") != "known"
        or end_row.get("size") != 0x30
        or end_row.get("pipeline_class") != "authored"
        or end_row.get("ownership_state") != "primary-owned"
        or end_row.get("navigation_name") != "zFMV_ActionBlur::End"
    ):
        raise ValueError(
            "Recoil main-menu transition EAX bridge requires exact reviewed "
            "authored blur constructor, Begin, Update, and End target identities"
        )

    constructor_bytes = _cc_cfg._hexdump_bytes(
        bridge.hexdump(_cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_CTOR_ADDRESS, 0x20)
    )
    vtable_bytes = _cc_cfg._hexdump_bytes(
        bridge.hexdump(_cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_VTABLE_ADDRESS, 0x18)
    )
    if (
        constructor_bytes
        != bytes.fromhex(
            "8b 54 24 08 8b c1 8b 4c 24 04 c7 40 04 00 00 00 00 "
            "c7 00 e0 25 4d 00 89 48 08 89 50 0c c2 08 00"
        )
        or vtable_bytes
        != bytes.fromhex(
            "70 2e 46 00 50 39 46 00 70 38 46 00 "
            "20 39 46 00 30 2e 46 00 00 00 00 00"
        )
        or struct.unpack_from(
            "<I", vtable_bytes, _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_BEGIN_SLOT
        )[0]
        != address_value(_cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_BEGIN_ADDRESS)
        or struct.unpack_from(
            "<I", vtable_bytes, _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_UPDATE_SLOT
        )[0]
        != address_value(_cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_UPDATE_ADDRESS)
        or struct.unpack_from(
            "<I", vtable_bytes, _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_END_SLOT
        )[0]
        != address_value(_cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_END_ADDRESS)
    ):
        raise ValueError(
            "Recoil main-menu transition EAX bridge rejects immutable retail "
            "blur constructor, vtable extent, slot, Begin, Update, or End target drift"
        )

    def instruction_bytes(instruction: Instruction) -> bytes:
        try:
            return bytes(int(value, 16) for value in instruction.bytes)
        except (TypeError, ValueError) as exc:
            raise ValueError(
                "Recoil main-menu transition EAX bridge requires exact "
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
                "Recoil main-menu transition EAX bridge requires the exact "
                f"complete contiguous {label} instruction population and bytes"
            )

    require_contiguous_code(
        "retail",
        retail_instructions,
        retail_offsets,
        _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_RETAIL_CODE,
    )

    retail_invocation_indices = tuple(
        index
        for index, instruction in enumerate(retail_instructions)
        if _cc_cfg._instruction_mnemonic(instruction) == "call"
    )
    retail_invocations = tuple(
        retail_addresses[index] for index in retail_invocation_indices
    )
    retail_cleanup = tuple(
        4 if ordinal == 11 else None
        for ordinal in range(len(_cc_catalog.RECOIL_MAIN_MENU_TRANSITION_RETAIL_CALL_ORDER))
    )
    if (
        retail_invocations != _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_RETAIL_CALL_ORDER
        or tuple(
            _cc_cfg._cleanup_after(retail_instructions, index)
            for index in retail_invocation_indices
        )
        != retail_cleanup
    ):
        raise ValueError(
            "Recoil main-menu transition EAX bridge rejects complete retail "
            "sixteen-call order/form/cleanup drift"
        )

    retail_by_offset = {
        offset: retail_instructions[index]
        for index, offset in enumerate(retail_offsets)
    }
    load_index = retail_offsets.index(0x59)
    call_index = retail_offsets.index(0x6D)
    exact_chain = {
        0x50: ("lea", b"\x8d\x4c\x24\x10"),
        0x54: ("call", b"\xe8\xd7\xe5\x04\x00"),
        0x59: ("mov", b"\x8b\x44\x24\x08"),
        0x5D: ("push", b"\x6a\x00"),
        0x5F: ("push", b"\x6a\x00"),
        0x61: ("lea", b"\x8d\x4c\x24\x10"),
        0x65: ("mov", b"\xc7\x44\x24\x48\x00\x00\x00\x00"),
        0x6D: ("call", b"\xff\x50\x08"),
    }
    if (
        any(
            offset not in retail_by_offset
            or _cc_cfg._instruction_mnemonic(retail_by_offset[offset]) != mnemonic
            or instruction_bytes(retail_by_offset[offset]) != body
            for offset, (mnemonic, body) in exact_chain.items()
        )
        or not _cc_cfg._instruction_operand(retail_by_offset[0x59]).lower().startswith(
            "eax,"
        )
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(retail_by_offset[0x59]).split(",", 1)[1]
        )
        != "esp+0x8"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(retail_by_offset[0x6D])
        )
        != "eax+0x8"
        or any(
            _cc_cfg._instruction_may_clobber_register(instruction, "eax")
            for instruction in retail_instructions[load_index + 1 : call_index]
        )
        or any(
            _cc_cfg._instruction_mnemonic(instruction).startswith("j")
            for instruction in retail_instructions[load_index + 1 : call_index]
        )
    ):
        raise ValueError(
            "Recoil main-menu transition EAX bridge rejects the unique "
            "stack-local vptr load, receiver, two-argument lifetime, or slot"
        )

    update_load_index = retail_offsets.index(0x70)
    update_call_index = retail_offsets.index(0x7C)
    update_chain = {
        0x70: ("mov", b"\x8b\x54\x24\x08"),
        0x74: ("push", b"\x6a\x00"),
        0x76: ("push", b"\x6a\x00"),
        0x78: ("lea", b"\x8d\x4c\x24\x10"),
        0x7C: ("call", b"\xff\x52\x04"),
    }
    if (
        any(
            offset not in retail_by_offset
            or _cc_cfg._instruction_mnemonic(retail_by_offset[offset]) != mnemonic
            or instruction_bytes(retail_by_offset[offset]) != body
            for offset, (mnemonic, body) in update_chain.items()
        )
        or not _cc_cfg._instruction_operand(retail_by_offset[0x70]).lower().startswith(
            "edx,"
        )
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(retail_by_offset[0x70]).split(",", 1)[1]
        )
        != "esp+0x8"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(retail_by_offset[0x7C])
        )
        != "edx+0x4"
        or any(
            _cc_cfg._instruction_may_clobber_register(instruction, "edx")
            for instruction in retail_instructions[
                update_load_index + 1 : update_call_index
            ]
        )
        or any(
            _cc_cfg._instruction_mnemonic(instruction).startswith("j")
            for instruction in retail_instructions[
                update_load_index + 1 : update_call_index
            ]
        )
    ):
        raise ValueError(
            "Recoil main-menu transition EDX bridge rejects the unique "
            "stack-local vptr load, receiver, two-argument lifetime, or slot"
        )

    loop_load_index = retail_offsets.index(0x83)
    loop_call_index = retail_offsets.index(0x8F)
    loop_chain = {
        0x7F: ("test", b"\x85\xc0"),
        0x81: ("je", b"\x74\x13"),
        0x83: ("mov", b"\x8b\x44\x24\x08"),
        0x87: ("push", b"\x6a\x00"),
        0x89: ("push", b"\x6a\x00"),
        0x8B: ("lea", b"\x8d\x4c\x24\x10"),
        0x8F: ("call", b"\xff\x50\x04"),
        0x92: ("test", b"\x85\xc0"),
        0x94: ("jne", b"\x75\xed"),
    }
    if (
        any(
            offset not in retail_by_offset
            or _cc_cfg._instruction_mnemonic(retail_by_offset[offset]) != mnemonic
            or instruction_bytes(retail_by_offset[offset]) != body
            for offset, (mnemonic, body) in loop_chain.items()
        )
        or not _cc_cfg._instruction_operand(retail_by_offset[0x83]).lower().startswith(
            "eax,"
        )
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(retail_by_offset[0x83]).split(",", 1)[1]
        )
        != "esp+0x8"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(retail_by_offset[0x8F])
        )
        != "eax+0x4"
        or tuple(
            normalize_address(address)
            for address in _cc_catalog.ADDRESS_RE.findall(
                _cc_cfg._instruction_operand(retail_by_offset[0x81])
            )
        )
        != ("0x4152b6",)
        or tuple(
            normalize_address(address)
            for address in _cc_catalog.ADDRESS_RE.findall(
                _cc_cfg._instruction_operand(retail_by_offset[0x94])
            )
        )
        != ("0x4152a3",)
        or any(
            _cc_cfg._instruction_may_clobber_register(instruction, "eax")
            for instruction in retail_instructions[
                loop_load_index + 1 : loop_call_index
            ]
        )
        or any(
            _cc_cfg._instruction_mnemonic(instruction).startswith("j")
            for instruction in retail_instructions[
                loop_load_index + 1 : loop_call_index
            ]
        )
    ):
        raise ValueError(
            "Recoil main-menu transition loop EAX bridge rejects the exact "
            "per-iteration vptr reload, receiver, arguments, slot, or "
            "entry/backedge lifetime"
        )

    end_load_index = retail_offsets.index(0x96)
    end_call_index = retail_offsets.index(0x9E)
    end_chain = {
        0x96: ("mov", b"\x8b\x54\x24\x08"),
        0x9A: ("lea", b"\x8d\x4c\x24\x08"),
        0x9E: ("call", b"\xff\x52\x0c"),
    }
    if (
        any(
            offset not in retail_by_offset
            or _cc_cfg._instruction_mnemonic(retail_by_offset[offset]) != mnemonic
            or instruction_bytes(retail_by_offset[offset]) != body
            for offset, (mnemonic, body) in end_chain.items()
        )
        or tuple(
            normalize_address(address)
            for address in _cc_catalog.ADDRESS_RE.findall(
                _cc_cfg._instruction_operand(retail_by_offset[0x81])
            )
        )
        != ("0x4152b6",)
        or tuple(
            normalize_address(address)
            for address in _cc_catalog.ADDRESS_RE.findall(
                _cc_cfg._instruction_operand(retail_by_offset[0x94])
            )
        )
        != ("0x4152a3",)
        or not _cc_cfg._instruction_operand(retail_by_offset[0x96]).lower().startswith(
            "edx,"
        )
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(retail_by_offset[0x96]).split(",", 1)[1]
        )
        != "esp+0x8"
        or _cc_cfg._instruction_operand(retail_by_offset[0x9A]).lower()
        != "ecx, [esp+0x8]"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(retail_by_offset[0x9E])
        )
        != "edx+0xc"
        or any(
            _cc_cfg._instruction_may_clobber_register(instruction, "edx")
            for instruction in retail_instructions[
                end_load_index + 1 : end_call_index
            ]
        )
        or any(
            _cc_cfg._instruction_mnemonic(instruction) in {"push", "pop"}
            or _cc_cfg._instruction_mnemonic(instruction).startswith("j")
            for instruction in retail_instructions[
                end_load_index + 1 : end_call_index
            ]
        )
    ):
        raise ValueError(
            "Recoil main-menu transition End EDX bridge rejects Update-exit "
            "convergence, shared vptr load, zero-argument receiver, lifetime, "
            "or slot"
        )

    if len(candidate_offsets) != len(candidate.instructions) or any(
        offset is None for offset in candidate_offsets
    ):
        raise ValueError(
            "Recoil main-menu transition EAX bridge requires complete "
            "candidate instruction offsets"
        )
    candidate_offset_to_index = {
        int(offset): index for index, offset in enumerate(candidate_offsets)
    }

    def direct_stack_local_calls(
        symbol: str,
        *,
        argument_values: tuple[str, ...],
        expected_count: int,
    ) -> tuple[int, ...]:
        relocations = [
            row for row in definition.relocations if row.symbol_name == symbol
        ]
        if (
            len(relocations) != expected_count
            or definition.undefined_external_functions.count(symbol)
            + definition.defined_external_functions.count(symbol)
            != 1
            or symbol in definition.undefined_external_data
            or symbol in definition.defined_external_data
        ):
            raise ValueError(
                "Recoil main-menu transition EAX bridge rejects candidate "
                f"COFF function provenance for {symbol!r}"
            )
        result: list[int] = []
        for relocation in relocations:
            call_offset = relocation.offset - 1
            instruction_index = candidate_offset_to_index.get(call_offset)
            field_end = relocation.offset + 4
            if (
                instruction_index is None
                or relocation.type != IMAGE_REL_I386_REL32
                or relocation.offset < 1
                or field_end > len(definition.data)
                or field_end > len(definition.relocation_mask)
                or definition.data[call_offset : relocation.offset]
                != b"\xe8"
                or struct.unpack_from("<I", definition.data, relocation.offset)[0]
                != 0
                or not all(
                    definition.relocation_mask[index]
                    for index in range(relocation.offset, field_end)
                )
            ):
                raise ValueError(
                    "Recoil main-menu transition EAX bridge rejects candidate "
                    f"E8 REL32 provenance for {symbol!r}"
                )
            instruction = candidate.instructions[instruction_index]
            decorated = _cc_catalog.DECORATED_RE.search(
                _cc_cfg._instruction_operand(instruction).strip()
            )
            if (
                _cc_cfg._instruction_mnemonic(instruction) != "call"
                or len(instruction_bytes(instruction)) != 5
                or instruction_bytes(instruction)[:1] != b"\xe8"
                or decorated is None
                or decorated.group(0) != symbol
                or _cc_cfg._cleanup_after(candidate.instructions, instruction_index)
                is not None
            ):
                raise ValueError(
                    "Recoil main-menu transition EAX bridge rejects candidate "
                    f"direct call form or cleanup for {symbol!r}"
                )

            receiver_index = instruction_index - 1
            while receiver_index >= 0 and not _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[receiver_index], "ecx"
            ):
                if _cc_cfg._instruction_mnemonic(
                    candidate.instructions[receiver_index]
                ) in {"call", "jmp", "ret"}:
                    break
                receiver_index -= 1
            expected_receiver_bytes = bytes((
                0x8D,
                0x4C,
                0x24,
                8 + 4 * len(argument_values),
            ))
            receiver = (
                candidate.instructions[receiver_index]
                if receiver_index >= 0
                else None
            )
            if (
                receiver is None
                or _cc_cfg._instruction_mnemonic(receiver) != "lea"
                or not _cc_cfg._instruction_operand(receiver).lower().startswith(
                    "ecx,"
                )
                or instruction_bytes(receiver) != expected_receiver_bytes
                or receiver_index < len(argument_values)
                or tuple(
                    _cc_cfg._instruction_mnemonic(
                        candidate.instructions[index]
                    )
                    for index in range(
                        receiver_index - len(argument_values),
                        receiver_index,
                    )
                )
                != ("push",) * len(argument_values)
                or tuple(
                    _cc_cfg._instruction_operand(candidate.instructions[index]).lower()
                    for index in range(
                        receiver_index - len(argument_values),
                        receiver_index,
                    )
                )
                != argument_values
            ):
                raise ValueError(
                    "Recoil main-menu transition EAX bridge rejects candidate "
                    f"stack-local receiver reaching definition for {symbol!r}"
                )
            result.append(instruction_index)
        return tuple(result)

    constructor_indices = direct_stack_local_calls(
        "??0zFMV_ActionBlur@@QAE@HH@Z",
        argument_values=("1", "4"),
        expected_count=1,
    )
    method_specs = (
        ("?Begin@zFMV_ActionBlur@@UAEXN@Z", ("0", "0"), 1),
        ("?Update@zFMV_ActionBlur@@UAEHN@Z", ("0", "0"), 2),
        ("?End@zFMV_ActionBlur@@UAEXXZ", (), 1),
    )
    method_symbols = {symbol for symbol, _arguments, _count in method_specs}

    def require_unrelocated_instruction(
        instruction_index: int,
        *,
        label: str,
    ) -> None:
        offset = candidate_offsets[instruction_index]
        instruction = candidate.instructions[instruction_index]
        body = instruction_bytes(instruction)
        end = int(offset) + len(body) if offset is not None else -1
        if (
            offset is None
            or end > len(definition.data)
            or end > len(definition.relocation_mask)
            or definition.data[int(offset) : end] != body
            or any(definition.relocation_mask[int(offset) : end])
            or any(
                relocation.offset < end
                and relocation.offset + 4 > int(offset)
                for relocation in definition.relocations
            )
        ):
            raise ValueError(
                "Recoil main-menu transition EAX bridge rejects candidate "
                f"indirect {label} COD/COFF byte or relocation provenance"
            )

    def candidate_stack_local_virtual_calls() -> tuple[
        tuple[str, int, tuple[str, ...], int], ...
    ]:
        rows: list[tuple[str, int, tuple[str, ...], int]] = []
        load_bytes = {
            "eax": b"\x8b\x44\x24\x08",
            "edx": b"\x8b\x54\x24\x08",
        }
        for call_index, instruction in enumerate(candidate.instructions):
            if _cc_cfg._instruction_mnemonic(instruction) != "call":
                continue
            memory = _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(instruction))
            match = re.fullmatch(
                r"(eax|edx)\+(0x[0-9a-f]+|[0-9]+)",
                memory.lower(),
            )
            if match is None:
                continue
            vptr_register = match.group(1)
            slot_token = match.group(2)
            slot = int(
                slot_token[2:] if slot_token.startswith("0x") else slot_token,
                16 if slot_token.startswith("0x") else 10,
            )

            receiver_index = call_index - 1
            while receiver_index >= 0 and not _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[receiver_index], "ecx"
            ):
                if _cc_cfg._instruction_mnemonic(
                    candidate.instructions[receiver_index]
                ) in {"call", "jmp", "ret"}:
                    break
                receiver_index -= 1
            receiver = (
                candidate.instructions[receiver_index]
                if receiver_index >= 0
                else None
            )
            if (
                receiver is None
                or _cc_cfg._instruction_mnemonic(receiver) != "lea"
                or not _cc_cfg._instruction_operand(receiver).lower().startswith("ecx,")
                or instruction_bytes(receiver)
                not in {b"\x8d\x4c\x24\x08", b"\x8d\x4c\x24\x10"}
            ):
                continue

            vptr_index = call_index - 1
            while vptr_index >= 0 and not _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[vptr_index], vptr_register
            ):
                if _cc_cfg._instruction_mnemonic(
                    candidate.instructions[vptr_index]
                ) in {"call", "jmp", "ret"}:
                    break
                vptr_index -= 1
            vptr_load = (
                candidate.instructions[vptr_index]
                if vptr_index >= 0
                else None
            )
            if (
                vptr_load is None
                or _cc_cfg._instruction_mnemonic(vptr_load) != "mov"
                or not _cc_cfg._instruction_operand(vptr_load).lower().startswith(
                    f"{vptr_register},"
                )
                or instruction_bytes(vptr_load) != load_bytes[vptr_register]
                or vptr_index >= receiver_index
                or vptr_index <= constructor_indices[0]
            ):
                raise ValueError(
                    "Recoil main-menu transition EAX bridge rejects candidate "
                    "indirect stack-local vptr reaching definition"
                )

            receiver_bytes = instruction_bytes(receiver)
            arguments = ("0", "0") if receiver_bytes[-1] == 0x10 else ()
            if (
                receiver_index < len(arguments)
                or tuple(
                    _cc_cfg._instruction_mnemonic(candidate.instructions[index])
                    for index in range(
                        receiver_index - len(arguments), receiver_index
                    )
                )
                != ("push",) * len(arguments)
                or tuple(
                    _cc_cfg._instruction_operand(candidate.instructions[index]).lower()
                    for index in range(
                        receiver_index - len(arguments), receiver_index
                    )
                )
                != arguments
                or len(instruction_bytes(instruction)) != 3
                or instruction_bytes(instruction)
                != bytes((
                    0xFF,
                    0x50 if vptr_register == "eax" else 0x52,
                    slot,
                ))
                or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
            ):
                raise ValueError(
                    "Recoil main-menu transition EAX bridge rejects candidate "
                    "indirect stack-local receiver, arguments, slot, call form, "
                    "or cleanup"
                )
            require_unrelocated_instruction(vptr_index, label="vptr load")
            for argument_index in range(
                receiver_index - len(arguments), receiver_index
            ):
                require_unrelocated_instruction(
                    argument_index,
                    label="argument",
                )
            require_unrelocated_instruction(receiver_index, label="receiver")
            require_unrelocated_instruction(call_index, label="call")
            rows.append((vptr_register, slot, arguments, call_index))
        return tuple(rows)

    indirect_rows = candidate_stack_local_virtual_calls()
    direct_method_surface = any(
        relocation.symbol_name in method_symbols
        for relocation in definition.relocations
    ) or any(
        name in method_symbols
        for names in (
            definition.undefined_external_functions,
            definition.defined_external_functions,
            definition.undefined_external_data,
            definition.defined_external_data,
        )
        for name in names
    ) or any(
        (match := _cc_catalog.DECORATED_RE.search(_cc_cfg._instruction_operand(instruction)))
        is not None
        and match.group(0) in method_symbols
        for instruction in candidate.instructions
    )
    candidate_blur_vptr_bridges: dict[
        str, ReviewedLoopVptrStorageBridge
    ] = {}
    if direct_method_surface:
        if indirect_rows:
            raise ValueError(
                "Recoil main-menu transition EAX bridge rejects mixed direct "
                "and indirect blur method provenance"
            )
        for symbol, arguments, count in method_specs:
            direct_stack_local_calls(
                symbol,
                argument_values=arguments,
                expected_count=count,
            )
    else:
        expected_indirect = (
            ("eax", 8, ("0", "0")),
            ("edx", 4, ("0", "0")),
            ("eax", 4, ("0", "0")),
            ("edx", 12, ()),
        )
        if tuple(row[:3] for row in indirect_rows) != expected_indirect:
            raise ValueError(
                "Recoil main-menu transition EAX bridge rejects candidate "
                "indirect Begin/Update/Update/End population, register, slot, "
                "receiver, or order"
            )
        candidate_blur_vptr_bridges = {
            f"0x{int(candidate_offsets[call_index]):x}":
            ReviewedLoopVptrStorageBridge(
                register=register,
                storage_identity=(
                    _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_STORAGE_IDENTITY
                ),
                slot_displacement=slot,
                assembly_source="cod",
            )
            for register, slot, _arguments, call_index in indirect_rows
        }

    begin_candidate_symbol = "?Begin@zFMV_ActionBlur@@UAEXN@Z"
    indexed_begin = indexes.by_candidate_name.get(begin_candidate_symbol)
    if indexed_begin is not None and indexed_begin != begin_identity:
        raise ValueError(
            "Recoil main-menu transition EAX bridge rejects conflicting "
            "candidate blur Begin identity"
        )

    retail_blur_vptr_bridges = {
        _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_BEGIN_CALL:
        ReviewedLoopVptrStorageBridge(
            register="eax",
            storage_identity=(
                _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_STORAGE_IDENTITY
            ),
            slot_displacement=_cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_BEGIN_SLOT,
            assembly_source="bn",
        ),
        _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_UPDATE_CALL:
        ReviewedLoopVptrStorageBridge(
            register="edx",
            storage_identity=(
                _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_STORAGE_IDENTITY
            ),
            slot_displacement=_cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_UPDATE_SLOT,
            assembly_source="bn",
        ),
        _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_LOOP_UPDATE_CALL:
        ReviewedLoopVptrStorageBridge(
            register="eax",
            storage_identity=(
                _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_STORAGE_IDENTITY
            ),
            slot_displacement=_cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_UPDATE_SLOT,
            assembly_source="bn",
        ),
        _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_END_CALL:
        ReviewedLoopVptrStorageBridge(
            register="edx",
            storage_identity=(
                _cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_STORAGE_IDENTITY
            ),
            slot_displacement=_cc_catalog.RECOIL_MAIN_MENU_TRANSITION_BLUR_END_SLOT,
            assembly_source="bn",
        ),
    }
    return retail_blur_vptr_bridges, candidate_blur_vptr_bridges




def _appframe_run_rtti_symbol_representation_is_exact(
    rtti_symbol: CandidateCoffSymbolDefinition,
    rtti_name: str,
    symbols: Sequence[CandidateCoffSymbolDefinition],
    symbol_by_index: Mapping[int, CandidateCoffSymbolDefinition],
) -> bool:
    """Accept only the reviewed undefined or defined-data RTTI COFF form."""

    exact_name_population = tuple(
        row for row in symbols if row.name == rtti_name
    )
    if (
        len(exact_name_population) != 1
        or not _cc_callable_identity._same_candidate_coff_symbol_identity(
            rtti_symbol, exact_name_population[0]
        )
        or rtti_symbol.name != rtti_name
        or rtti_symbol.value != 0
        or rtti_symbol.symbol_type != 0
        or rtti_symbol.storage_class != IMAGE_SYM_CLASS_EXTERNAL
        or rtti_symbol.aux_count != 0
    ):
        return False

    if rtti_symbol.section_number == 0:
        return (
            rtti_symbol.section_name == ""
            and rtti_symbol.section_size == 0
            and rtti_symbol.natural_end == 0
            and rtti_symbol.section_characteristics == 0
            and rtti_symbol.section_data == b""
            and rtti_symbol.section_relocations == ()
            and rtti_symbol.section_snapshot_number == 0
        )

    expected_data = _cc_catalog.APPFRAME_RUN_V15_DEFINED_RTTI_DATA.get(rtti_name)
    section_external_population = tuple(
        row.name
        for row in symbols
        if row.section_number == rtti_symbol.section_number
        and row.storage_class == IMAGE_SYM_CLASS_EXTERNAL
        and row.aux_count == 0
    )
    type_info_population = tuple(
        row for row in symbols
        if row.name == _cc_catalog.APPFRAME_RUN_V15_TYPE_INFO_VFTABLE
    )
    if (
        expected_data is None
        or rtti_symbol.section_number <= 0
        or rtti_symbol.section_snapshot_number != rtti_symbol.section_number
        or rtti_symbol.section_name != ".data"
        or rtti_symbol.section_size != len(expected_data)
        or rtti_symbol.natural_end != len(expected_data)
        or rtti_symbol.section_characteristics
        != _cc_catalog.APPFRAME_RUN_V15_RTTI_SECTION_CHARACTERISTICS
        or rtti_symbol.section_data != expected_data
        or section_external_population != (rtti_name,)
        or len(rtti_symbol.section_relocations) != 1
        or len(type_info_population) != 1
    ):
        return False
    relocation = rtti_symbol.section_relocations[0]
    target = symbol_by_index.get(relocation.symbol_index)
    return (
        relocation.offset == 0
        and relocation.type == IMAGE_REL_I386_DIR32
        and relocation.symbol_name == _cc_catalog.APPFRAME_RUN_V15_TYPE_INFO_VFTABLE
        and target is not None
        and target.name == _cc_catalog.APPFRAME_RUN_V15_TYPE_INFO_VFTABLE
        and target.value == 0
        and target.section_number == 0
        and target.symbol_type == 0
        and target.storage_class == IMAGE_SYM_CLASS_EXTERNAL
        and target.aux_count == 0
        and _cc_callable_identity._same_candidate_coff_symbol_identity(
            target, type_info_population[0]
        )
    )


def _appframe_run_catch_funclet_proof(
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
) -> AppFrameRunCatchFuncletProof | None:
    """Derive only Run v9's three associative-xdata catch ranges.

    The six catch-local IAT calls remain in the ordinary COD inventory.  This
    proof merely partitions them from the authored Run contract after binding
    each range to one associative ``.xdata$x`` RTTI/handler pair, one local
    COFF entry symbol, a closed local CFG range, and its reviewed lifecycle
    retail address.
    """
    from _recoil.call_contract.records import AppFrameRunCatchFuncletProof

    if (
        caller_identity != _cc_catalog.APPFRAME_RUN_V9_CALLER_IDENTITY
        and normalize_address(caller_start) != _cc_catalog.APPFRAME_RUN_V9_START
    ):
        return None

    label = "AppFrame Run v9 catch-funclet partition"

    def reject(detail: str) -> NoReturn:
        raise _cc_errors.CandidateCallContractEvidenceError(f"{label} rejects {detail}")

    if (
        caller_identity != _cc_catalog.APPFRAME_RUN_V9_CALLER_IDENTITY
        or normalize_address(caller_start) != _cc_catalog.APPFRAME_RUN_V9_START
        or normalize_address(caller_end_exclusive) != _cc_catalog.APPFRAME_RUN_V9_END
    ):
        reject("caller identity, address, or authored extent drift")

    target = candidate.target
    try:
        target = _cc_source._appframe_v10_base_profile_target(
            _cc_catalog.APPFRAME_V9_TARGET_ID,
            _cc_catalog.APPFRAME_V9_MANIFEST_PATH,
            target,
        )
        profile, flags = effective_source_compile_context(
            target, _cc_catalog.APPFRAME_V9_SOURCE_PATH
        )
    except (AttributeError, TypeError, ValueError) as exc:
        raise _cc_errors.CandidateCallContractEvidenceError(
            f"{label} requires the exact base target profile"
        ) from exc
    queue_define_name = _cc_catalog.APPFRAME_V9_QUEUE_DEFINE[2:].upper()
    queue_defines = tuple(
        flag
        for flag in flags
        if str(flag).upper().startswith("/D")
        and str(flag)[2:].partition("=")[0].upper() == queue_define_name
    )
    if (
        target is None
        or getattr(target, "name", "") != _cc_catalog.APPFRAME_V9_TARGET_NAME
        or getattr(target, "source_from", "") != _cc_catalog.APPFRAME_V9_SOURCE_PATH
        or profile != _cc_catalog.APPFRAME_V9_PROFILE_NAME
        or tuple(flags) != _cc_catalog.APPFRAME_V9_BASE_FLAGS
        or queue_defines
    ):
        reject("target, TU, base profile, flag, or queue-define drift")

    definition = candidate.caller_definition
    if (
        definition is None
        or definition.symbol != _cc_catalog.APPFRAME_RUN_V9_SYMBOL
        or definition.section_index <= 0
        or definition.section_start != 0
        or definition.section_end <= 0
        or len(definition.data) != definition.section_end
        or len(definition.relocation_mask) != definition.section_end
    ):
        reject("the exact current Run COMDAT identity or extent")

    caller_symbols = tuple(
        row for row in definition.coff_symbols
        if row.name == _cc_catalog.APPFRAME_RUN_V9_SYMBOL
    )
    if (
        len(caller_symbols) != 1
        or caller_symbols[0].section_number != definition.section_index
        or caller_symbols[0].value != 0
        or caller_symbols[0].symbol_type != 0x20
        or caller_symbols[0].storage_class != IMAGE_SYM_CLASS_EXTERNAL
        or caller_symbols[0].aux_count != 0
        or caller_symbols[0].section_size != definition.section_end
        or caller_symbols[0].natural_end != definition.section_end
    ):
        reject("caller COFF symbol, section, extent, or collision drift")

    run_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    frame_indices = tuple(
        index for index, offset in enumerate(run_offsets) if offset == 0x19
    )
    frame_body = b""
    if len(frame_indices) == 1:
        try:
            frame_body = bytes(
                int(item, 16)
                for item in candidate.instructions[frame_indices[0]].bytes
            )
        except (TypeError, ValueError):
            frame_body = b""
    frame_size = frame_body[2] if len(frame_body) == 3 else -1
    if (
        len(frame_indices) != 1
        or frame_body[:2] != b"\x83\xec"
        or frame_size <= 0
        or _cc_cfg._instruction_mnemonic(candidate.instructions[frame_indices[0]])
        != "sub"
        or _cc_cfg._instruction_operand(candidate.instructions[frame_indices[0]]).strip()
        != f"esp, {frame_size}"
        or definition.data[0x19:0x1C] != frame_body
        or any(definition.relocation_mask[0x19:0x1C])
    ):
        reject("the exact Run local-frame allocation provenance")

    expected_xdata = bytearray(_cc_catalog.APPFRAME_RUN_V9_XDATA)
    for xdata_offset, local_offset in (
        (0x50, frame_size + 12),
        (0x60, frame_size + 8),
        (0x70, frame_size + 16),
    ):
        expected_xdata[xdata_offset : xdata_offset + 4] = struct.pack(
            "<i", -local_offset
        )

    associated_xdata = tuple(
        row for row in definition.associated_sections
        if row.name == ".xdata$x"
    )
    if len(associated_xdata) != 1:
        reject("one exact associative .xdata$x section")
    xdata = associated_xdata[0]
    expected_xdata_offsets = (
        0x08, 0x10, 0x40,
        *(offset for spec in _cc_catalog.APPFRAME_RUN_V13_CATCH_SPECS for offset in spec[:2]),
    )
    if (
        xdata.association_section_index != definition.section_index
        or xdata.section_index <= 0
        or not xdata.section_is_comdat
        or xdata.comdat_selection != 5
        or xdata.section_size != 0x78
        or xdata.data != bytes(expected_xdata)
        or len(xdata.relocation_mask) != 0x78
        or xdata.section_external_symbols
        or tuple(
            sorted(
                (
                    row.value,
                    row.section_number,
                    row.type,
                    row.storage_class,
                )
                for row in xdata.symbols
            )
        )
        != (
            (0x00, xdata.section_index, 0, 3),
            (0x20, xdata.section_index, 0, 3),
            (0x30, xdata.section_index, 0, 3),
            (0x48, xdata.section_index, 0, 3),
        )
        or len({row.name for row in xdata.symbols}) != 4
        or tuple(row.offset for row in xdata.relocations)
        != expected_xdata_offsets
        or any(
            row.type != IMAGE_REL_I386_DIR32
            or row.offset + 4 > len(xdata.data)
            or xdata.data[row.offset : row.offset + 4] != b"\0" * 4
            or not all(xdata.relocation_mask[row.offset : row.offset + 4])
            for row in xdata.relocations
        )
        or {
            index for index, masked in enumerate(xdata.relocation_mask) if masked
        }
        != {
            index
            for offset in expected_xdata_offsets
            for index in range(offset, offset + 4)
        }
    ):
        reject("xdata association, extent, relocation, addend, mask, or population drift")

    symbol_by_index = {row.index: row for row in definition.coff_symbols}
    if len(symbol_by_index) != len(definition.coff_symbols):
        reject("colliding COFF symbol indices")
    for associated_symbol in xdata.symbols:
        matches = tuple(
            row for row in definition.coff_symbols
            if row.name == associated_symbol.name
            and row.value == associated_symbol.value
            and row.section_number == associated_symbol.section_number
            and row.symbol_type == associated_symbol.type
            and row.storage_class == associated_symbol.storage_class
            and row.aux_count == 0
        )
        if len(matches) != 1:
            reject("xdata local-symbol COFF population drift")
    xdata_metadata_targets = {
        offset: target_value for offset, target_value in (
            (0x08, 0x20), (0x10, 0x30), (0x40, 0x48)
        )
    }
    xdata_symbol_names = {row.name for row in xdata.symbols}
    for offset, target_value in xdata_metadata_targets.items():
        relocation = next(row for row in xdata.relocations if row.offset == offset)
        target_symbol = symbol_by_index.get(relocation.symbol_index)
        if (
            target_symbol is None
            or relocation.symbol_name != target_symbol.name
            or target_symbol.name not in xdata_symbol_names
            or target_symbol.value != target_value
            or target_symbol.section_number != xdata.section_index
            or target_symbol.symbol_type != 0
            or target_symbol.storage_class != 3
            or target_symbol.aux_count != 0
        ):
            reject(f"xdata metadata-symbol population drift at +0x{offset:x}")
    local_by_value: dict[int, list[CandidateCoffSymbolDefinition]] = {}
    for row in definition.coff_symbols:
        if (
            row.section_number == definition.section_index
            and row.symbol_type == 0
            and row.storage_class == 6
            and row.aux_count == 0
        ):
            local_by_value.setdefault(row.value, []).append(row)

    handler_starts: list[int] = []
    lifecycle_addresses: list[str] = []
    defined_rtti_sections: list[int] = []
    for (
        rtti_offset,
        handler_offset,
        rtti_name,
        lifecycle_address,
    ) in _cc_catalog.APPFRAME_RUN_V13_CATCH_SPECS:
        rtti_rows = [row for row in xdata.relocations if row.offset == rtti_offset]
        handler_rows = [
            row for row in xdata.relocations if row.offset == handler_offset
        ]
        rtti_symbol = (
            symbol_by_index.get(rtti_rows[0].symbol_index)
            if len(rtti_rows) == 1 else None
        )
        handler_symbol = (
            symbol_by_index.get(handler_rows[0].symbol_index)
            if len(handler_rows) == 1 else None
        )
        exact_locals = (
            local_by_value.get(handler_symbol.value, [])
            if handler_symbol is not None else []
        )
        if (
            len(rtti_rows) != 1
            or len(handler_rows) != 1
            or rtti_symbol is None
            or handler_symbol is None
            or rtti_rows[0].symbol_name != rtti_name
            or not _appframe_run_rtti_symbol_representation_is_exact(
                rtti_symbol,
                rtti_name,
                definition.coff_symbols,
                symbol_by_index,
            )
            or handler_rows[0].symbol_name != handler_symbol.name
            or handler_symbol.value <= 0
            or handler_symbol.value >= definition.section_end
            or len(exact_locals) != 1
            or not _cc_callable_identity._same_candidate_coff_symbol_identity(
                handler_symbol, exact_locals[0]
            )
            or not handler_symbol.name
        ):
            reject(
                f"RTTI/handler/local-symbol identity drift at xdata +0x{handler_offset:x}"
            )
        handler_starts.append(handler_symbol.value)
        lifecycle_addresses.append(lifecycle_address)
        if rtti_symbol.section_number > 0:
            defined_rtti_sections.append(rtti_symbol.section_number)

    if len(set(defined_rtti_sections)) != len(defined_rtti_sections):
        reject("distinct defined-data RTTI COMDAT sections")

    if (
        len(handler_starts) != 3
        or len(set(handler_starts)) != 3
        or tuple(handler_starts) != tuple(sorted(handler_starts))
    ):
        reject("three strictly increasing xdata-derived handler starts")

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    if len(offsets) != len(candidate.instructions) or any(
        offset is None for offset in offsets
    ):
        reject("complete unique COD instruction coordinates")
    index_by_offset = {
        int(offset): index for index, offset in enumerate(offsets)
        if offset is not None
    }
    if len(index_by_offset) != len(candidate.instructions):
        reject("duplicate COD instruction coordinates")
    for boundary in handler_starts:
        if boundary not in index_by_offset:
            reject(f"one xdata-derived COD handler instruction at +0x{boundary:x}")

    ranges: list[tuple[int, int]] = []
    for ordinal, range_start in enumerate(handler_starts):
        if ordinal + 1 < len(handler_starts):
            range_end = handler_starts[ordinal + 1]
        else:
            terminal_candidates: list[tuple[int, int]] = []
            for index, offset in enumerate(offsets):
                if offset is None or int(offset) < range_start:
                    continue
                instruction = candidate.instructions[index]
                if (
                    _cc_cfg._instruction_mnemonic(instruction) in {"ret", "retn"}
                    and _cc_cfg._instruction_operand(instruction).strip() in {"", "0"}
                    and bytes(int(item, 16) for item in instruction.bytes)
                    == b"\xc3"
                    and local_by_value.get(int(offset) + 1)
                ):
                    terminal_candidates.append((index, int(offset)))
            if len(terminal_candidates) != 1:
                reject("one terminal RET followed by the local primary continuation")
            _terminal_index, terminal_offset = terminal_candidates[0]
            range_end = terminal_offset + 1
            continuation_symbols = local_by_value.get(range_end, [])
            if (
                len(continuation_symbols) != 1
                or not continuation_symbols[0].name
                or continuation_symbols[0].value != range_end
                or range_end not in index_by_offset
            ):
                reject("one derived local primary-continuation symbol and COD boundary")

        terminal_offset = range_end - 1
        terminal_index = index_by_offset.get(terminal_offset)
        terminal = (
            candidate.instructions[terminal_index]
            if terminal_index is not None else None
        )
        if (
            terminal is None
            or _cc_cfg._instruction_mnemonic(terminal) not in {"ret", "retn"}
            or _cc_cfg._instruction_operand(terminal).strip() not in {"", "0"}
            or bytes(int(item, 16) for item in terminal.bytes) != b"\xc3"
            or definition.data[terminal_offset : range_end] != b"\xc3"
            or definition.relocation_mask[terminal_offset]
            or int(offsets[terminal_index]) + 1 != range_end
        ):
            reject(
                f"unique exact terminal RET and adjacent boundary for range +0x{range_start:x}"
            )
        ranges.append((range_start, range_end))

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start="0x0",
        caller_end_exclusive=hex(definition.section_end),
    )
    excluded_indices: list[int] = []
    excluded_ordinals: list[int] = []
    excluded_offsets: list[int] = []
    catch_calls_by_range: list[tuple[int, int]] = []
    derived_iat_sites: dict[str, list[int]] = {
        "__imp__MessageBoxExA@20": [],
        "__imp__exit": [],
    }
    for range_start, range_end in ranges:
        range_invocations = tuple(
            (ordinal, index, int(offsets[index]))
            for ordinal, index in enumerate(invocation_indices)
            if range_start <= int(offsets[index]) < range_end
        )
        if len(range_invocations) != 2:
            reject(
                f"exact two-call catch population for range +0x{range_start:x}"
            )
        catch_calls = tuple(row[2] for row in range_invocations)
        for row, iat_name in zip(
            range_invocations,
            ("__imp__MessageBoxExA@20", "__imp__exit"),
        ):
            instruction = candidate.instructions[row[1]]
            if _cc_cfg._instruction_operand(instruction).strip() != f"dword {iat_name}":
                reject(
                    f"ordered MessageBoxExA/exit catch calls for range +0x{range_start:x}"
                )
            derived_iat_sites[iat_name].append(row[2])
        catch_calls_by_range.append((catch_calls[0], catch_calls[1]))
        excluded_ordinals.extend(row[0] for row in range_invocations)
        excluded_indices.extend(row[1] for row in range_invocations)
        excluded_offsets.extend(row[2] for row in range_invocations)

    for iat_name, derived_sites in derived_iat_sites.items():
        sites = tuple(derived_sites)
        coff_symbols = [row for row in definition.coff_symbols if row.name == iat_name]
        relocations = [
            row for row in definition.relocations if row.symbol_name == iat_name
        ]
        if (
            len(coff_symbols) != 1
            or definition.undefined_external_functions.count(iat_name) != 1
            or iat_name in definition.defined_external_functions
            or iat_name in definition.undefined_external_data
            or iat_name in definition.defined_external_data
            or tuple(row.offset for row in relocations)
            != tuple(site + 2 for site in sites)
        ):
            reject(f"one exact {iat_name} COFF consumer population")
        symbol = coff_symbols[0]
        if (
            symbol.section_number != 0
            or symbol.symbol_type != 0x20
            or symbol.storage_class != IMAGE_SYM_CLASS_EXTERNAL
            or symbol.aux_count != 0
        ):
            reject(f"the exact {iat_name} undefined external")
        for relocation, site in zip(relocations, sites):
            index = index_by_offset.get(site)
            instruction = (
                candidate.instructions[index] if index is not None else None
            )
            if (
                instruction is None
                or relocation.type != IMAGE_REL_I386_DIR32
                or relocation.symbol_index != symbol.index
                or bytes(int(item, 16) for item in instruction.bytes)
                != b"\xff\x15\0\0\0\0"
                or _cc_cfg._instruction_operand(instruction).strip() != f"dword {iat_name}"
                or definition.data[site : site + 6] != b"\xff\x15\0\0\0\0"
                or not all(definition.relocation_mask[site + 2 : site + 6])
            ):
                reject(f"exact {iat_name} FF15/DIR32 call provenance")

    parsed_cfg_targets = dict(candidate.local_control_flow_targets)
    classification_only_cfg_targets = dict(
        candidate.classification_only_local_control_flow_targets
    )
    cfg_targets = {
        **parsed_cfg_targets,
        **classification_only_cfg_targets,
    }
    if (
        set(parsed_cfg_targets) & set(classification_only_cfg_targets)
        or frozenset(cfg_targets) != candidate.local_control_flow_indices
        or any(
            type(source_index) is not int
            or source_index < 0
            or source_index >= len(candidate.instructions)
            or not isinstance(target_indices, tuple)
            or not target_indices
            or len(set(target_indices)) != len(target_indices)
            or any(
                type(target_index) is not int
                or target_index < 0
                or target_index >= len(candidate.instructions)
                for target_index in target_indices
            )
            for source_index, target_indices in cfg_targets.items()
        )
    ):
        reject("malformed or incomplete local CFG authority")
    for (range_start, range_end), catch_calls in zip(
        ranges, catch_calls_by_range
    ):
        for source_index, target_indices in cfg_targets.items():
            source_offset = int(offsets[source_index])
            source_inside = range_start <= source_offset < range_end
            for target_index in target_indices:
                target_offset = int(offsets[target_index])
                target_inside = range_start <= target_offset < range_end
                if source_inside != target_inside:
                    reject(
                        f"CFG edge crossing catch range +0x{range_start:x}"
                    )
        exit_offset = catch_calls[-1]
        if any(
            range_start <= int(offsets[index]) < range_end
            and int(offsets[index]) > exit_offset
            for index in invocation_indices
        ) or any(
            range_start <= int(offsets[index]) < range_end
            and int(offsets[index]) >= exit_offset
            for index in cfg_targets
        ):
            reject(f"nonterminal exit CFG for range +0x{range_start:x}")

    proof = AppFrameRunCatchFuncletProof(
        ranges=tuple(ranges),
        lifecycle_addresses=tuple(lifecycle_addresses),
        excluded_invocation_indices=tuple(excluded_indices),
        excluded_invocation_ordinals=tuple(excluded_ordinals),
        excluded_invocation_offsets=tuple(excluded_offsets),
        generic_invocation_count=len(invocation_indices),
    )
    if (
        len(proof.ranges) != 3
        or any(
            range_end != proof.ranges[index + 1][0]
            for index, (_range_start, range_end) in enumerate(proof.ranges[:-1])
        )
        or proof.lifecycle_addresses != ("0x44300b", "0x443032", "0x4430cc")
        or len(proof.excluded_invocation_indices) != 6
        or len(set(proof.excluded_invocation_indices)) != 6
        or tuple(proof.excluded_invocation_ordinals)
        != tuple(sorted(proof.excluded_invocation_ordinals))
        or len(set(proof.excluded_invocation_ordinals)) != 6
        or len(proof.excluded_invocation_offsets) != 6
        or len(set(proof.excluded_invocation_offsets)) != 6
    ):
        reject("derived range, lifecycle, exclusion, or inventory drift")
    return proof


def _appframe_run_primary_contract_projection(
    expected: Sequence[Mapping[str, Any]],
    candidate_contract: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    generic_call_sites: Sequence[str],
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
) -> list[dict[str, Any]]:
    """Exclude only proven catch calls from Run's primary contract view."""

    proof = _appframe_run_catch_funclet_proof(
        candidate,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    result = [dict(row) for row in candidate_contract]
    if proof is None:
        return result
    label = "AppFrame Run v9 primary-contract projection"
    if (
        len(expected) != 22
        or tuple(row.get("ordinal") for row in expected) != tuple(range(22))
        or len(result) != proof.generic_invocation_count
        or tuple(row.get("ordinal") for row in result)
        != tuple(range(proof.generic_invocation_count))
        or len(generic_call_sites) != proof.generic_invocation_count
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            f"{label} rejects retail, candidate, or generic inventory drift"
        )
    excluded = frozenset(proof.excluded_invocation_ordinals)
    expected_excluded_sites = tuple(
        normalize_address(address_value(_cc_catalog.APPFRAME_RUN_V9_START) + offset)
        for offset in proof.excluded_invocation_offsets
    )
    if tuple(
        generic_call_sites[index]
        for index in proof.excluded_invocation_ordinals
    ) != (
        expected_excluded_sites
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            f"{label} rejects exact catch invocation call-site drift"
        )
    projected = [
        dict(row) for ordinal, row in enumerate(result)
        if ordinal not in excluded
    ]
    for ordinal, row in enumerate(projected):
        row["ordinal"] = ordinal
    if (
        len(projected) != proof.generic_invocation_count - 6
        or len(generic_call_sites) != proof.generic_invocation_count
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            f"{label} produced an ambiguous primary-contract population"
        )
    return projected


def _recoil_app_mp_exit_deactivate_candidate_indirect_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedExactIndirectStorageBridge]:
    """Recover only OnDeactivate's exact delete-dialog virtual call.

    The natural ``delete dialog`` source leaves the candidate extractor with
    EAX holding a vptr loaded from the relocation-backed global receiver.  The
    bridge is available only for the complete reviewed caller/object shape and
    exact global storage authority.  It authorizes candidate-side provenance
    at one call site; retail plus reviewed tracker identities remain expected
    truth.
    """
    from _recoil.call_contract.records import ReviewedExactIndirectStorageBridge

    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_CALLER_START:
        return {}

    expected_row = {
        "ordinal": _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_CALL_ORDINAL,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": (
            f"load(storage:{_cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_SYMBOL_ID})"
        ),
        "slot_displacement": 8,
        "cleanup_bytes": None,
    }
    if (
        len(expected) <= _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_CALL_ORDINAL
        or dict(expected[_cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_CALL_ORDINAL])
        != expected_row
    ):
        raise ValueError(
            "Recoil MP-exit OnDeactivate bridge requires immutable retail "
            "ordinal-1 slot-8 delete-dialog truth"
        )

    symbols = document.collection("symbols")
    storage_rows = document.collection("storage_contributions")
    targets = document.collection("verification_targets")
    caller_symbol_id = (
        _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_CALLER_IDENTITY.removeprefix("symbol:")
    )
    caller_row = symbols.get(caller_symbol_id)
    caller_target = targets.get(_cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_TARGET_ID)
    caller_registration = (
        caller_target.get("registration")
        if isinstance(caller_target, Mapping)
        else None
    )
    focused_target = targets.get(
        _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_CALLER_TARGET_ID
    )
    focused_registration = (
        focused_target.get("registration")
        if isinstance(focused_target, Mapping)
        else None
    )
    definition = candidate.caller_definition
    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_CALLER_START
    ]
    if (
        caller_identity != _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(normalized_start) != caller_identity
        or indexes.by_candidate_name.get(
            _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_CALLER_SYMBOL
        )
        not in {None, caller_identity}
        or caller_identity in indexes.provider_ids
        or not isinstance(caller_row, Mapping)
        or _cc_identity._symbol_identity(caller_symbol_id, caller_row) != caller_identity
        or caller_row.get("binary") != "recoil"
        or caller_row.get("kind") != "function"
        or caller_row.get("pipeline_class") != "authored"
        or caller_row.get("ownership_state") != "primary-owned"
        or caller_row.get("extent_state") != "known"
        or normalize_address(str(caller_row.get("address", "")))
        != _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_CALLER_START
        or normalize_address(str(caller_row.get("end_exclusive", "")))
        != _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_CALLER_END_EXCLUSIVE
        or caller_row.get("size")
        != len(_cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_CANDIDATE_BODY)
        or caller_row.get("navigation_name")
        != "RecoilApp_MpExitDialogState::OnDeactivate"
        or caller_row.get("output_section_id") != "recoil:section:.text"
        or caller_row.get("physical_block_id") != "recoil:block:0x417350"
        or caller_row.get("verification_target_ids")
        != [
            _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_TARGET_ID,
            _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_CALLER_TARGET_ID,
        ]
        or not isinstance(caller_target, Mapping)
        or caller_target.get("binary") != "recoil"
        or caller_target.get("kind") != "vc5"
        or caller_target.get("name")
        != _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_TARGET_NAME
        or tuple(caller_target.get("registered_addresses", ())).count(
            _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_CALLER_START
        )
        != 1
        or not isinstance(caller_registration, Mapping)
        or caller_registration.get("binary") != "recoil"
        or caller_registration.get("name")
        != _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_TARGET_NAME
        or caller_registration.get("manifest_path")
        != str(
            _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_TARGET_MANIFEST.relative_to(
                REPO_ROOT
            )
        ).replace("\\", "/")
        or caller_registration.get("source_from")
        != _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_SOURCE_PATH
        or caller_registration.get("check_translation_unit_function_order")
        is not True
        or caller_registration.get("function_order_scope") != "authored"
        or not isinstance(focused_target, Mapping)
        or focused_target.get("binary") != "recoil"
        or focused_target.get("kind") != "vc5"
        or focused_target.get("name")
        != _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_CALLER_TARGET_NAME
        or focused_target.get("symbol_ids") != [caller_symbol_id]
        or focused_target.get("unresolved_addresses") != []
        or not isinstance(focused_registration, Mapping)
        or focused_registration.get("manifest_path")
        != (
            "tools/vc5_verify_targets/"
            "recoil_app_mp_exit_dialog_state_on_deactivate.json"
        )
        or focused_registration.get("source_from")
        != _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_SOURCE_PATH
        or focused_registration.get("function_addresses")
        != [_cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_CALLER_START]
        or definition is None
        or definition.symbol != _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_CALLER_SYMBOL
        or target is None
        or getattr(target, "name", "")
        != _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_TARGET_NAME
        or getattr(target, "target_binary", "") != "recoil"
        or Path(str(getattr(target, "manifest_path", ""))).resolve()
        != _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_TARGET_MANIFEST.resolve()
        or not bool(
            getattr(target, "check_translation_unit_function_order", False)
        )
        or len(contribution_rows) != 1
    ):
        raise ValueError(
            "Recoil MP-exit OnDeactivate bridge requires the exact authored "
            "caller, mission.cpp contribution, extent, and governed target "
            "authority"
        )

    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "")
        != _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_CALLER_SYMBOL
        or getattr(contribution_row, "symbol_regex", None) is not None
        or getattr(contribution_row, "name", "")
        != "RecoilApp_MpExitDialogState::OnDeactivate"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or getattr(contribution_row, "required_presence", None) is not True
        or getattr(contribution_row, "full_order_gate", None) is not True
    ):
        raise ValueError(
            "Recoil MP-exit OnDeactivate bridge rejects stale mission.cpp "
            "contribution-row authority"
        )

    dialog_symbol = symbols.get(_cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_SYMBOL_ID)
    dialog_storage = storage_rows.get(_cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_STORAGE_ID)
    dialog_target = targets.get(_cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_TARGET_ID)
    dialog_registration = (
        dialog_target.get("registration")
        if isinstance(dialog_target, Mapping)
        else None
    )
    dialog_trace = (
        dialog_symbol.get("source_traceability")
        if isinstance(dialog_symbol, Mapping)
        else None
    )
    dialog_identity = f"storage:{_cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_SYMBOL_ID}"
    if (
        not isinstance(dialog_symbol, Mapping)
        or _cc_identity._symbol_identity(
            _cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_SYMBOL_ID, dialog_symbol
        )
        != f"symbol:{_cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_SYMBOL_ID}"
        or dialog_symbol.get("binary") != "recoil"
        or dialog_symbol.get("kind") != "data"
        or dialog_symbol.get("disposition") != "authored"
        or dialog_symbol.get("extent_state") != "unknown"
        or normalize_address(str(dialog_symbol.get("address", "")))
        != _cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_ADDRESS
        or dialog_symbol.get("navigation_name")
        != _cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_NAME
        or dialog_symbol.get("output_section_id") != "recoil:section:.data"
        or dialog_symbol.get("storage_contribution_ids")
        != [_cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_STORAGE_ID]
        or dialog_symbol.get("verification_target_ids")
        != [
            (
                "recoil:vc5-target:"
                "hud_ui_mp_exit_dialog_singleton_and_strings_data"
            ),
            _cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_TARGET_ID,
            "recoil:vc5-target:hud_ui_mp_exit_dialog_table_cluster",
        ]
        or not isinstance(dialog_trace, Mapping)
        or dialog_trace.get("state") != "resolved"
        or dialog_trace.get("reason_code") not in {None, ""}
        or dialog_trace.get("source_edges")
        != [
            {
                "anchor_id": (
                    "recoil:anchor:battlesport-mission-"
                    "g-huduimpexitdialog"
                ),
                "emission_context": {
                    "translation_unit": _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_SOURCE_PATH
                },
                "evidence_ids": [],
                "relation": "defines",
            }
        ]
        or not isinstance(dialog_storage, Mapping)
        or dialog_storage.get("binary") != "recoil"
        or dialog_storage.get("kind") != "data-symbol"
        or dialog_storage.get("output_section_id") != "recoil:section:.data"
        or dialog_storage.get("overlap") != "none"
        or dialog_storage.get("parent_contribution_id") is not None
        or dialog_storage.get("owner_ids")
        != ["recoil:owner:hud_ui.hud_ui_mp_exit_dialog_class"]
        or dialog_storage.get("symbol_ids")
        != [_cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_SYMBOL_ID]
        or dialog_storage.get("reference")
        != {
            "address": _cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_ADDRESS,
            "evidence_ids": [],
            "extent_state": "unknown",
        }
        or not isinstance(dialog_target, Mapping)
        or dialog_target.get("binary") != "recoil"
        or dialog_target.get("kind") != "vc5"
        or dialog_target.get("name")
        != "hud_ui_mp_exit_dialog_singleton_data"
        or dialog_target.get("symbol_ids")
        != [_cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_SYMBOL_ID]
        or dialog_target.get("unresolved_addresses") != []
        or not isinstance(dialog_registration, Mapping)
        or dialog_registration.get("manifest_path")
        != (
            "tools/vc5_verify_targets/"
            "hud_ui_mp_exit_dialog_singleton_data.json"
        )
        or dialog_registration.get("source_from")
        != _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_SOURCE_PATH
        or dialog_registration.get("data_addresses")
        != [_cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_ADDRESS]
        or indexes.storage_by_address.get(
            _cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_ADDRESS
        )
        != dialog_identity
        or indexes.storage_by_name.get(_cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_NAME)
        != dialog_identity
        or dialog_identity in indexes.provider_ids
    ):
        raise ValueError(
            "Recoil MP-exit OnDeactivate bridge requires exact reviewed "
            "delete-dialog symbol, storage, source, and target authority"
        )

    assert definition is not None
    expected_relocations = (
        (0x02, IMAGE_REL_I386_DIR32, _cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_OBJECT_SYMBOL),
        (0x07, IMAGE_REL_I386_REL32, "?UnloadLayout@HudUiMpExitDialog@@QAEXXZ"),
        (0x0D, IMAGE_REL_I386_DIR32, _cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_OBJECT_SYMBOL),
        (0x1E, IMAGE_REL_I386_DIR32, _cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_OBJECT_SYMBOL),
        (0x27, IMAGE_REL_I386_REL32, "?BindMapContextPop@zInput@@YIXXZ"),
        (0x32, IMAGE_REL_I386_DIR32, "__imp__Sleep@4"),
        (0x37, IMAGE_REL_I386_DIR32, "?g_HudUiDialogSampleSetName@@3PADA"),
        (0x3C, IMAGE_REL_I386_REL32, "@zSndSampleSetDestroyByName@4"),
        (0x43, IMAGE_REL_I386_REL32, "?SetScaleAndRebuild@HudScoreboard@@YGXM@Z"),
    )
    observed_relocations = tuple(
        (row.offset, row.type, row.symbol_name)
        for row in definition.relocations
    )
    masked_offsets = {
        byte_offset
        for offset, _relocation_type, _symbol in expected_relocations
        for byte_offset in range(offset, offset + 4)
    }
    if (
        definition.data != _cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_CANDIDATE_BODY
        or observed_relocations != expected_relocations
        or len(definition.relocation_mask) != len(definition.data)
        or {
            index
            for index, masked in enumerate(definition.relocation_mask)
            if masked
        }
        != masked_offsets
        or any(
            struct.unpack_from("<I", definition.data, offset)[0] != 0
            for offset, _relocation_type, _symbol in expected_relocations
        )
    ):
        raise ValueError(
            "Recoil MP-exit OnDeactivate bridge rejects stale candidate "
            "body, complete COFF relocation population, addend, or "
            "relocation mask"
        )

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    expected_offsets = (
        0x00,
        0x06,
        0x0B,
        0x11,
        0x13,
        0x15,
        0x17,
        0x19,
        0x1C,
        0x26,
        0x2B,
        0x30,
        0x36,
        0x3B,
        0x40,
        0x42,
        0x47,
    )
    normalized_offsets = (
        (0 if index == 0 and offset is None else offset)
        for index, offset in enumerate(offsets)
    )
    if tuple(normalized_offsets) != expected_offsets or len(
        candidate.instructions
    ) != len(expected_offsets):
        raise ValueError(
            "Recoil MP-exit OnDeactivate bridge requires the exact complete "
            "candidate COD instruction topology"
        )
    by_offset = {
        offset: instruction
        for offset, instruction in zip(offsets, candidate.instructions)
        if offset is not None
    }
    exact_unit = {
        0x0B: (
            "mov",
            _cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_OBJECT_SYMBOL,
            b"\x8b\x0d\0\0\0\0",
        ),
        0x11: ("test", "ecx, ecx", b"\x85\xc9"),
        0x13: ("je", "", b"\x74\x07"),
        0x15: ("mov", "eax, dword [ecx]", b"\x8b\x01"),
        0x17: ("push", "1", b"\x6a\x01"),
        0x19: ("call", "dword [eax+8]", b"\xff\x50\x08"),
    }
    for offset, (mnemonic, operand_fragment, body) in exact_unit.items():
        instruction = by_offset.get(offset)
        try:
            encoded = bytes(int(value, 16) for value in instruction.bytes)
        except (AttributeError, TypeError, ValueError) as exc:
            raise ValueError(
                "Recoil MP-exit OnDeactivate bridge requires exact "
                "candidate delete-dialog bytes"
            ) from exc
        if (
            _cc_cfg._instruction_mnemonic(instruction) != mnemonic
            or (
                operand_fragment
                and operand_fragment.casefold()
                not in _cc_cfg._instruction_operand(instruction).casefold()
            )
            or encoded != body
        ):
            raise ValueError(
                "Recoil MP-exit OnDeactivate bridge rejects stale global/"
                f"null-check/vptr/flag/slot topology at +0x{offset:x}"
            )

    return {
        normalize_address(_cc_catalog.RECOIL_APP_MP_EXIT_DEACTIVATE_CALL_OFFSET): (
            ReviewedExactIndirectStorageBridge(
                register="eax",
                storage_identity=expected_row["storage_identity"],
                slot_displacement=8,
                assembly_source="cod",
            )
        )
    }


def _recoil_app_mp_exit_update_should_quit_candidate_indirect_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedExactIndirectStorageBridge]:
    """Recover only OnUpdateShouldQuit's exact dialog UpdateAll call.

    The source-faithful local ``dialog`` keeps the relocation-backed receiver
    in ECX while EAX carries its vptr at the slot-0 call.  This candidate-only
    bridge is available only for the complete reviewed caller/object/COD shape
    and exact dialog storage authority.  Immutable retail and reviewed tracker
    identities remain the expected invocation truth.
    """
    from _recoil.call_contract.records import ReviewedExactIndirectStorageBridge

    normalized_start = normalize_address(caller_start)
    if (
        normalized_start
        != _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALLER_START
    ):
        return {}

    expected_row = {
        "ordinal": _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALL_ORDINAL,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": (
            f"load(storage:{_cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_SYMBOL_ID})"
        ),
        "slot_displacement": 0,
        "cleanup_bytes": None,
    }
    if (
        len(expected) <= _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALL_ORDINAL
        or dict(
            expected[_cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALL_ORDINAL]
        )
        != expected_row
    ):
        raise ValueError(
            "Recoil MP-exit OnUpdateShouldQuit bridge requires immutable "
            "retail ordinal-2 slot-0 dialog UpdateAll truth"
        )

    symbols = document.collection("symbols")
    storage_rows = document.collection("storage_contributions")
    targets = document.collection("verification_targets")
    caller_symbol_id = (
        _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALLER_IDENTITY.removeprefix(
            "symbol:"
        )
    )
    caller_row = symbols.get(caller_symbol_id)
    caller_target = targets.get(
        _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_TARGET_ID
    )
    caller_registration = (
        caller_target.get("registration")
        if isinstance(caller_target, Mapping)
        else None
    )
    focused_target = targets.get(
        _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALLER_TARGET_ID
    )
    focused_registration = (
        focused_target.get("registration")
        if isinstance(focused_target, Mapping)
        else None
    )
    definition = candidate.caller_definition
    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALLER_START
    ]
    if (
        caller_identity
        != _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(normalized_start) != caller_identity
        or indexes.by_candidate_name.get(
            _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALLER_SYMBOL
        )
        not in {None, caller_identity}
        or caller_identity in indexes.provider_ids
        or not isinstance(caller_row, Mapping)
        or _cc_identity._symbol_identity(caller_symbol_id, caller_row) != caller_identity
        or caller_row.get("binary") != "recoil"
        or caller_row.get("kind") != "function"
        or caller_row.get("pipeline_class") != "authored"
        or caller_row.get("ownership_state") != "primary-owned"
        or caller_row.get("extent_state") != "known"
        or normalize_address(str(caller_row.get("address", "")))
        != _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALLER_START
        or normalize_address(str(caller_row.get("end_exclusive", "")))
        != _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALLER_END_EXCLUSIVE
        or caller_row.get("size")
        != len(_cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CANDIDATE_BODY)
        or caller_row.get("navigation_name")
        != "RecoilApp_MpExitDialogState::OnUpdateShouldQuit"
        or caller_row.get("output_section_id") != "recoil:section:.text"
        or caller_row.get("physical_block_id") != "recoil:block:0x417350"
        or caller_row.get("verification_target_ids")
        != [
            _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_TARGET_ID,
            _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALLER_TARGET_ID,
        ]
        or not isinstance(caller_target, Mapping)
        or caller_target.get("binary") != "recoil"
        or caller_target.get("kind") != "vc5"
        or caller_target.get("name")
        != _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_TARGET_NAME
        or tuple(caller_target.get("registered_addresses", ())).count(
            _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALLER_START
        )
        != 1
        or not isinstance(caller_registration, Mapping)
        or caller_registration.get("binary") != "recoil"
        or caller_registration.get("name")
        != _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_TARGET_NAME
        or caller_registration.get("manifest_path")
        != str(
            _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_TARGET_MANIFEST.relative_to(
                REPO_ROOT
            )
        ).replace("\\", "/")
        or caller_registration.get("source_from")
        != _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_SOURCE_PATH
        or caller_registration.get("check_translation_unit_function_order")
        is not True
        or caller_registration.get("function_order_scope") != "authored"
        or not isinstance(focused_target, Mapping)
        or focused_target.get("binary") != "recoil"
        or focused_target.get("kind") != "vc5"
        or focused_target.get("name")
        != _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALLER_TARGET_NAME
        or focused_target.get("symbol_ids") != [caller_symbol_id]
        or focused_target.get("unresolved_addresses") != []
        or not isinstance(focused_registration, Mapping)
        or focused_registration.get("manifest_path")
        != (
            "tools/vc5_verify_targets/"
            "recoil_app_mp_exit_dialog_state_on_update_should_quit.json"
        )
        or focused_registration.get("source_from")
        != _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_SOURCE_PATH
        or focused_registration.get("function_addresses")
        != [_cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALLER_START]
        or definition is None
        or definition.symbol
        != _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALLER_SYMBOL
        or target is None
        or getattr(target, "name", "")
        != _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_TARGET_NAME
        or getattr(target, "target_binary", "") != "recoil"
        or Path(str(getattr(target, "manifest_path", ""))).resolve()
        != _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_TARGET_MANIFEST.resolve()
        or not bool(
            getattr(target, "check_translation_unit_function_order", False)
        )
        or len(contribution_rows) != 1
    ):
        raise ValueError(
            "Recoil MP-exit OnUpdateShouldQuit bridge requires the exact "
            "authored caller, mission.cpp contribution, extent, and governed "
            "target authority"
        )

    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "")
        != _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALLER_SYMBOL
        or getattr(contribution_row, "symbol_regex", None) is not None
        or getattr(contribution_row, "name", "")
        != "RecoilApp_MpExitDialogState::OnUpdateShouldQuit"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or getattr(contribution_row, "required_presence", None) is not True
        or getattr(contribution_row, "full_order_gate", None) is not True
    ):
        raise ValueError(
            "Recoil MP-exit OnUpdateShouldQuit bridge rejects stale "
            "mission.cpp contribution-row authority"
        )

    dialog_symbol = symbols.get(_cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_SYMBOL_ID)
    dialog_storage = storage_rows.get(_cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_STORAGE_ID)
    dialog_target = targets.get(_cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_TARGET_ID)
    dialog_registration = (
        dialog_target.get("registration")
        if isinstance(dialog_target, Mapping)
        else None
    )
    dialog_trace = (
        dialog_symbol.get("source_traceability")
        if isinstance(dialog_symbol, Mapping)
        else None
    )
    dialog_identity = f"storage:{_cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_SYMBOL_ID}"
    if (
        not isinstance(dialog_symbol, Mapping)
        or _cc_identity._symbol_identity(
            _cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_SYMBOL_ID, dialog_symbol
        )
        != f"symbol:{_cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_SYMBOL_ID}"
        or dialog_symbol.get("binary") != "recoil"
        or dialog_symbol.get("kind") != "data"
        or dialog_symbol.get("disposition") != "authored"
        or dialog_symbol.get("extent_state") != "unknown"
        or normalize_address(str(dialog_symbol.get("address", "")))
        != _cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_ADDRESS
        or dialog_symbol.get("navigation_name")
        != _cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_NAME
        or dialog_symbol.get("output_section_id") != "recoil:section:.data"
        or dialog_symbol.get("storage_contribution_ids")
        != [_cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_STORAGE_ID]
        or dialog_symbol.get("verification_target_ids")
        != [
            (
                "recoil:vc5-target:"
                "hud_ui_mp_exit_dialog_singleton_and_strings_data"
            ),
            _cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_TARGET_ID,
            "recoil:vc5-target:hud_ui_mp_exit_dialog_table_cluster",
        ]
        or not isinstance(dialog_trace, Mapping)
        or dialog_trace.get("state") != "resolved"
        or dialog_trace.get("reason_code") not in {None, ""}
        or dialog_trace.get("source_edges")
        != [
            {
                "anchor_id": (
                    "recoil:anchor:battlesport-mission-"
                    "g-huduimpexitdialog"
                ),
                "emission_context": {
                    "translation_unit": (
                        _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_SOURCE_PATH
                    )
                },
                "evidence_ids": [],
                "relation": "defines",
            }
        ]
        or not isinstance(dialog_storage, Mapping)
        or dialog_storage.get("binary") != "recoil"
        or dialog_storage.get("kind") != "data-symbol"
        or dialog_storage.get("output_section_id") != "recoil:section:.data"
        or dialog_storage.get("overlap") != "none"
        or dialog_storage.get("parent_contribution_id") is not None
        or dialog_storage.get("owner_ids")
        != ["recoil:owner:hud_ui.hud_ui_mp_exit_dialog_class"]
        or dialog_storage.get("symbol_ids")
        != [_cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_SYMBOL_ID]
        or dialog_storage.get("reference")
        != {
            "address": _cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_ADDRESS,
            "evidence_ids": [],
            "extent_state": "unknown",
        }
        or not isinstance(dialog_target, Mapping)
        or dialog_target.get("binary") != "recoil"
        or dialog_target.get("kind") != "vc5"
        or dialog_target.get("name")
        != "hud_ui_mp_exit_dialog_singleton_data"
        or dialog_target.get("symbol_ids")
        != [_cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_SYMBOL_ID]
        or dialog_target.get("unresolved_addresses") != []
        or not isinstance(dialog_registration, Mapping)
        or dialog_registration.get("manifest_path")
        != (
            "tools/vc5_verify_targets/"
            "hud_ui_mp_exit_dialog_singleton_data.json"
        )
        or dialog_registration.get("source_from")
        != _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_SOURCE_PATH
        or dialog_registration.get("data_addresses")
        != [_cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_ADDRESS]
        or indexes.storage_by_address.get(
            _cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_ADDRESS
        )
        != dialog_identity
        or indexes.storage_by_name.get(_cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_NAME)
        != dialog_identity
        or dialog_identity in indexes.provider_ids
    ):
        raise ValueError(
            "Recoil MP-exit OnUpdateShouldQuit bridge requires exact reviewed "
            "dialog symbol, storage, source, and target authority"
        )

    assert definition is not None
    local_constant_sentinel = object()
    expected_relocations = (
        (0x09, IMAGE_REL_I386_REL32, "?PollActiveDevices@zInput@@YIXE@Z"),
        (0x0E, IMAGE_REL_I386_REL32, "?Tick@Time@@YAXXZ"),
        (0x14, IMAGE_REL_I386_DIR32, _cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_OBJECT_SYMBOL),
        (0x1A, IMAGE_REL_I386_DIR32, "_g_FrameDeltaTimeSec"),
        (0x24, IMAGE_REL_I386_DIR32, _cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_OBJECT_SYMBOL),
        (0x30, IMAGE_REL_I386_DIR32, local_constant_sentinel),
        (0x47, IMAGE_REL_I386_REL32, "?GetMessageString@zLoc@@YIPADI@Z"),
        (0x74, IMAGE_REL_I386_REL32, "?GetMessageString@zLoc@@YIPADI@Z"),
        (
            0x9F,
            IMAGE_REL_I386_REL32,
            _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_FLIP_TO_GDI_SYMBOL,
        ),
        (
            0xA4,
            IMAGE_REL_I386_REL32,
            _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_SHUTDOWN_SYMBOL,
        ),
        (
            0xA9,
            IMAGE_REL_I386_REL32,
            _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_SESSION_SHUTDOWN_SYMBOL,
        ),
        (
            0xAE,
            IMAGE_REL_I386_REL32,
            _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_VIDEO_SHUTDOWN_SYMBOL,
        ),
        (0xC0, IMAGE_REL_I386_DIR32, "??_C@_07OEOL@?$CFs?3?5?$CFs?6?$AA@"),
        (0xC6, IMAGE_REL_I386_DIR32, "__imp__printf"),
        (0xD4, IMAGE_REL_I386_DIR32, "__imp__Sleep@4"),
        (0xDC, IMAGE_REL_I386_DIR32, "__imp__MessageBeep@4"),
        (0xE2, IMAGE_REL_I386_DIR32, "_g_RecoilApp_hWndMain"),
        (0xF8, IMAGE_REL_I386_DIR32, "__imp__MessageBoxA@16"),
        (0xFF, IMAGE_REL_I386_REL32, "?ExitProcessWithCleanup@zSys@@YIXH@Z"),
    )
    local_constant = (
        definition.relocations[5]
        if len(definition.relocations) > 5
        else None
    )
    normalize_local_constant = (
        local_constant is not None
        and local_constant.offset == 0x30
        and local_constant.type == IMAGE_REL_I386_DIR32
        and re.fullmatch(r"\$T[0-9]+", local_constant.symbol_name) is not None
        and definition.data
        == _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CANDIDATE_BODY
        and len(definition.relocation_mask) == len(definition.data)
        and struct.unpack_from("<I", definition.data, 0x30)[0] == 0
        and all(definition.relocation_mask[0x30:0x34])
    )
    observed_relocations = tuple(
        (
            row.offset,
            row.type,
            (
                local_constant_sentinel
                if index == 5 and normalize_local_constant
                else row.symbol_name
            ),
        )
        for index, row in enumerate(definition.relocations)
    )
    masked_offsets = {
        byte_offset
        for offset, _relocation_type, _symbol in expected_relocations
        for byte_offset in range(offset, offset + 4)
    }
    if (
        definition.data
        != _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CANDIDATE_BODY
        or observed_relocations != expected_relocations
        or len(definition.relocation_mask) != len(definition.data)
        or {
            index
            for index, masked in enumerate(definition.relocation_mask)
            if masked
        }
        != masked_offsets
        or any(
            struct.unpack_from("<I", definition.data, offset)[0] != 0
            for offset, _relocation_type, _symbol in expected_relocations
        )
    ):
        raise ValueError(
            "Recoil MP-exit OnUpdateShouldQuit bridge rejects stale candidate "
            "body, complete COFF relocation population, addend, or relocation "
            "mask"
        )

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    normalized_offsets = tuple(
        0 if index == 0 and offset is None else offset
        for index, offset in enumerate(offsets)
    )
    if (
        normalized_offsets
        != _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_COD_OFFSETS
        or len(candidate.instructions)
        != len(_cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_COD_OFFSETS)
    ):
        raise ValueError(
            "Recoil MP-exit OnUpdateShouldQuit bridge requires the exact "
            "complete candidate COD instruction topology"
        )
    by_offset = {
        offset: instruction
        for offset, instruction in zip(normalized_offsets, candidate.instructions)
        if offset is not None
    }
    exact_unit = {
        0x12: (
            "mov",
            _cc_catalog.RECOIL_APP_MP_EXIT_DIALOG_OBJECT_SYMBOL,
            b"\x8b\x0d\0\0\0\0",
        ),
        0x18: ("mov", "edx, dword _g_FrameDeltaTimeSec", b"\x8b\x15\0\0\0\0"),
        0x1E: ("push", "edx", b"\x52"),
        0x1F: ("mov", "eax, dword [ecx]", b"\x8b\x01"),
        0x21: ("call", "dword [eax]", b"\xff\x10"),
    }
    for offset, (mnemonic, operand_fragment, body) in exact_unit.items():
        instruction = by_offset.get(offset)
        try:
            encoded = bytes(int(value, 16) for value in instruction.bytes)
        except (AttributeError, TypeError, ValueError) as exc:
            raise ValueError(
                "Recoil MP-exit OnUpdateShouldQuit bridge requires exact "
                "candidate dialog UpdateAll bytes"
            ) from exc
        if (
            _cc_cfg._instruction_mnemonic(instruction) != mnemonic
            or operand_fragment.casefold()
            not in _cc_cfg._instruction_operand(instruction).casefold()
            or encoded != body
        ):
            raise ValueError(
                "Recoil MP-exit OnUpdateShouldQuit bridge rejects stale "
                "global/time/argument/vptr/slot topology at "
                f"+0x{offset:x}"
            )

    return {
        normalize_address(
            _cc_catalog.RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALL_OFFSET
        ): ReviewedExactIndirectStorageBridge(
            register="eax",
            storage_identity=expected_row["storage_identity"],
            slot_displacement=0,
            assembly_source="cod",
        )
    }


def _recoilapp_terminal_frame_candidate_cleanup_projection(
    candidate_contract: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
) -> list[dict[str, Any]]:
    """Remove one exact candidate local-frame reversal from call cleanup."""

    result = [dict(row) for row in candidate_contract]
    if normalize_address(caller_start) != "0x42f8e0":
        return result
    exact_row = {
        "ordinal": 16,
        "form": "call",
        "dispatch": "direct",
        "identity_kind": "direct",
        "target_identity": "symbol:recoil:function:0x462630",
        "storage_identity": "",
        "slot_displacement": None,
        "cleanup_bytes": 0x24,
    }
    definition = candidate.caller_definition
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    by_offset = {
        offset: instruction
        for offset, instruction in zip(offsets, candidate.instructions)
        if offset is not None
    }
    cleanup_relocations = (
        []
        if definition is None
        else [
            row
            for row in definition.relocations
            if row.symbol_name == "?Cleanup@zFMV_Script@@QAEXXZ"
        ]
    )
    if (
        caller_identity != "symbol:recoil:function:0x42f8e0"
        or normalize_address(caller_end_exclusive) != "0x42f9d0"
        or len(result) != 17
        or result[16] != exact_row
        or definition is None
        or definition.symbol != "?OnDeactivate@CRecoilAppPlayState@@UAEXXZ"
        or bytes(int(item, 16) for item in by_offset[0x00].bytes)
        != b"\x83\xec\x24"
        or bytes(int(item, 16) for item in by_offset[0xB0].bytes)
        != b"\xe8\x00\x00\x00\x00"
        or bytes(int(item, 16) for item in by_offset[0xB5].bytes)
        != b"\x83\xc4\x24"
        or bytes(int(item, 16) for item in by_offset[0xB8].bytes) != b"\xc3"
        or len(cleanup_relocations) != 1
        or cleanup_relocations[0].offset != 0xB1
        or cleanup_relocations[0].type != IMAGE_REL_I386_REL32
        or cleanup_relocations[0].offset + 4 > len(definition.data)
        or definition.data[0xB1:0xB5] != b"\x00\x00\x00\x00"
        or not all(definition.relocation_mask[0xB1:0xB5])
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "RecoilApp PlayState candidate terminal-frame cleanup package drifted"
        )
    result[16]["cleanup_bytes"] = None
    return result
