"""Recoil call-contract recoil weapons evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import errors as _cc_errors
from _recoil.call_contract import receiver_instructions as _cc_receiver_instructions
from _recoil.call_contract import receiver_storage as _cc_receiver_storage

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        IdentityIndexes,
        ProviderNamedImportThunk,
        ProviderPeNamedImportThunk,
        ReviewedLoopVptrStorageBridge,
        ReviewedRegisterCallStorageBridge,
    )

import re
import struct
from collections import Counter
from pathlib import Path
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import IMAGE_REL_I386_REL32, Instruction
from _recoil.lib.progress import ProgressDocument, address_value, normalize_address
from _recoil.lib.tooling import REPO_ROOT


def _zwep_damage_feedback_retail_handler_vptr_bridge(
    retail_instructions: Sequence[Instruction],
    *,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedLoopVptrStorageBridge]:
    """Publish retail's exact hit-event handler context/slot-4 lineage."""
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge

    start = normalize_address(caller_start)
    if start != _cc_catalog.ZWEP_DAMAGE_FEEDBACK_CALLER_START:
        return {}
    if (
        normalize_address(caller_end_exclusive)
        != _cc_catalog.ZWEP_DAMAGE_FEEDBACK_CALLER_END
        or indexes.by_address.get(start)
        != _cc_catalog.ZWEP_DAMAGE_FEEDBACK_CALLER_IDENTITY
        or _cc_catalog.ZWEP_DAMAGE_FEEDBACK_CALLER_IDENTITY in indexes.provider_ids
    ):
        raise ValueError(
            "zWep damage-feedback retail provenance requires exact authored caller identity and extent"
        )
    addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(start),
    )
    counts = Counter(address for address in addresses if address is not None)
    by_address = {
        normalize_address(address): instruction
        for address, instruction in zip(addresses, retail_instructions)
        if address is not None and counts[address] == 1
    }
    required = {
        "0x4b2708": (
            b"\x8b\x7c\x24\x1c",
            r"mov\s+edi\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[esp\+(?:28|0x1c)\]",
        ),
        "0x4b271e": (
            b"\x8b\x47\x24",
            r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[edi\+(?:36|0x24)\]",
        ),
        "0x4b2721": (
            b"\x8b\xb0\xbc\x00\x00\x00",
            r"mov\s+esi\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[eax\+(?:188|0xbc)\]",
        ),
        "0x4b2727": (b"\x3b\xf3", r"cmp\s+esi\s*,\s*ebx"),
        "0x4b2729": (b"\x0f\x84\x38\x01\x00\x00", r"je\s+(?:0x)?4b2867"),
        "0x4b272f": (b"\x83\xfe\x01", r"cmp\s+esi\s*,\s*(?:1|0x1)"),
        "0x4b2746": (b"\x39\x5e\x04", r"cmp\s+(?:dword\s+(?:ptr\s+)?)?\[esi\+(?:4|0x4)\]\s*,\s*ebx"),
        "0x4b2749": (b"\x0f\x84\xfa\x00\x00\x00", r"je\s+(?:0x)?4b2849"),
        "0x4b2791": (b"\xa1\x9c\x9a\x77\x00", r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[.+\]"),
        "0x4b27b2": (b"\x8b\x57\x24", r"mov\s+edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[edi\+(?:36|0x24)\]"),
        "0x4b27b5": (b"\x53", r"push\s+ebx"),
        "0x4b27b6": (b"\x8b\xce", r"mov\s+ecx\s*,\s*esi"),
        "0x4b27b8": (b"\xff\xd0", r"call\s+eax"),
        "0x4b27ba": (b"\x8b\x0e", r"mov\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[esi\]"),
        "0x4b27bc": (b"\x53", r"push\s+ebx"),
        "0x4b27bd": (b"\x57", r"push\s+edi"),
        "0x4b27be": (b"\x8b\xd5", r"mov\s+edx\s*,\s*ebp"),
        "0x4b27c0": (b"\xff\x56\x04", r"call\s+(?:dword\s+(?:ptr\s+)?)?\[esi\+(?:4|0x4)\]"),
        "0x4b27c3": (b"\x8b\xd8", r"mov\s+ebx\s*,\s*eax"),
    }
    for address, (expected_body, expected_text) in required.items():
        row = by_address.get(address)
        try:
            body = bytes(int(item, 16) for item in row.bytes) if row else b""
        except (TypeError, ValueError):
            body = b""
        if (
            body != expected_body
            or re.fullmatch(
                expected_text,
                row.raw_text.strip() if row is not None else "",
                flags=re.IGNORECASE,
            )
            is None
        ):
            raise ValueError(
                "zWep damage-feedback retail handler lineage/CFG/slot bytes drifted at "
                f"{address}"
            )
    return {
        "0x4b27c0": ReviewedLoopVptrStorageBridge(
            register="esi",
            storage_identity=_cc_catalog.ZWEP_DAMAGE_FEEDBACK_HANDLER_VPTR_STORAGE,
            slot_displacement=4,
            assembly_source="bn",
        )
    }


def _zwep_damage_timer_retail_handler_vptr_bridge(
    retail_instructions: Sequence[Instruction],
    *,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedLoopVptrStorageBridge]:
    """Publish only retail's exact damage-handler context/slot provenance.

    Retail loads the handler lineage through ``[edx+0x24]`` then
    ``[eax+0xbc]``.  At the final dispatch it loads the receiver from
    ``[eax+8]`` and calls slot ``[eax+0xc]``.  This adapter supplies no
    candidate equivalence: an inverse candidate receiver/target layout must
    remain a normal semantic mismatch.
    """
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge

    start = normalize_address(caller_start)
    if start != _cc_catalog.ZWEP_DAMAGE_TIMER_CALLER_START:
        return {}
    if (
        normalize_address(caller_end_exclusive) != _cc_catalog.ZWEP_DAMAGE_TIMER_CALLER_END
        or indexes.by_address.get(start) != _cc_catalog.ZWEP_DAMAGE_TIMER_CALLER_IDENTITY
        or _cc_catalog.ZWEP_DAMAGE_TIMER_CALLER_IDENTITY in indexes.provider_ids
    ):
        raise ValueError(
            "zWep damage-timer retail provenance requires exact authored caller identity and extent"
        )
    addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(start),
    )
    counts = Counter(address for address in addresses if address is not None)
    by_address = {
        normalize_address(address): instruction
        for address, instruction in zip(addresses, retail_instructions)
        if address is not None and counts[address] == 1
    }
    required = {
        "0x4b2880": (b"\x8b\x42\x24", r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[edx\+(?:36|0x24)\]"),
        "0x4b2883": (b"\x56", r"push\s+esi"),
        "0x4b2884": (b"\x8b\x35\x68\x89\x77\x00", r"mov\s+esi\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[.+\]"),
        "0x4b288a": (b"\x8b\x80\xbc\x00\x00\x00", r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[eax\+(?:188|0xbc)\]"),
        "0x4b28cc": (b"\x8b\x4c\x24\x08", r"mov\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[esp\+(?:8|0x8)\]"),
        "0x4b28d0": (b"\x51", r"push\s+ecx"),
        "0x4b28d1": (b"\x8b\x48\x08", r"mov\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[eax\+(?:8|0x8)\]"),
        "0x4b28d4": (b"\xff\x50\x0c", r"call\s+(?:dword\s+(?:ptr\s+)?)?\[eax\+(?:12|0xc)\]"),
        "0x4b28d7": (b"\x5e", r"pop\s+esi"),
        "0x4b28d8": (b"\xc2\x04\x00", r"ret(?:n)?\s+(?:4|0x4)"),
    }
    for address, (expected_body, expected_text) in required.items():
        row = by_address.get(address)
        try:
            body = bytes(int(item, 16) for item in row.bytes) if row else b""
        except (TypeError, ValueError):
            body = b""
        if (
            body != expected_body
            or re.fullmatch(
                expected_text,
                row.raw_text.strip() if row is not None else "",
                flags=re.IGNORECASE,
            )
            is None
        ):
            raise ValueError(
                "zWep damage-timer retail handler lineage/slot bytes drifted at "
                f"{address}"
            )
    return {
        "0x4b28d4": ReviewedLoopVptrStorageBridge(
            register="eax",
            storage_identity=_cc_catalog.ZWEP_DAMAGE_TIMER_HANDLER_VPTR_STORAGE,
            slot_displacement=0x0C,
            assembly_source="bn",
        )
    }


def _ciasin_provider_candidate_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
) -> dict[str, str]:
    """Bind one exact VC5 ``__CIasin`` spelling to retail ``_CIasin``.

    The candidate COFF spelling is not provider identity authority.  Publish it
    only for the reviewed zMath CRT math-error caller after the independent
    retail contract, tracker provider row, selected source target, COD call, and
    COFF relocation all agree exactly.
    """

    if (
        caller_identity != _cc_catalog.MSVC_CIASIN_CALLER_IDENTITY
        or normalize_address(caller_start) != _cc_catalog.MSVC_CIASIN_CALLER_ADDRESS
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.MSVC_CIASIN_CALLER_END_EXCLUSIVE
    ):
        return {}

    mentions = [
        instruction
        for instruction in candidate.instructions
        if _cc_catalog.MSVC_CIASIN_CANDIDATE_SYMBOL.casefold()
        in _cc_cfg._instruction_operand(instruction).casefold()
    ]
    if not mentions:
        return {}
    if any(
        _cc_cfg._instruction_mnemonic(instruction) != "call"
        or _cc_cfg._instruction_operand(instruction).strip()
        != _cc_catalog.MSVC_CIASIN_CANDIDATE_SYMBOL
        for instruction in mentions
    ):
        raise ValueError(
            "MSVC __CIasin bridge accepts only one exact direct CALL operand; "
            "aliases, case drift, and indirect forms are forbidden"
        )

    caller_row = document.collection("symbols").get(
        _cc_catalog.MSVC_CIASIN_CALLER_IDENTITY.removeprefix("symbol:")
    )
    caller_trace = (
        caller_row.get("source_traceability")
        if isinstance(caller_row, Mapping)
        else None
    )
    caller_edges = (
        caller_trace.get("source_edges")
        if isinstance(caller_trace, Mapping)
        else None
    )
    caller_block = document.collection("physical_blocks").get(
        _cc_catalog.MSVC_CIASIN_CALLER_BLOCK_ID
    )
    if (
        not isinstance(caller_row, Mapping)
        or caller_row.get("binary") != "recoil"
        or caller_row.get("kind") != "function"
        or caller_row.get("pipeline_class") != "authored"
        or caller_row.get("authored_order_role")
        not in {None, "authored-body"}
        or caller_row.get("address") != _cc_catalog.MSVC_CIASIN_CALLER_ADDRESS
        or caller_row.get("end_exclusive")
        != _cc_catalog.MSVC_CIASIN_CALLER_END_EXCLUSIVE
        or caller_row.get("extent_state") != "known"
        or caller_row.get("size") != 0x1A0
        or caller_row.get("ownership_state") != "primary-owned"
        or caller_row.get("physical_block_id") != _cc_catalog.MSVC_CIASIN_CALLER_BLOCK_ID
        or caller_row.get("verification_target_ids", []).count(
            _cc_catalog.MSVC_CIASIN_TARGET_ID
        )
        != 1
        or not isinstance(caller_trace, Mapping)
        or caller_trace.get("state") != "resolved"
        or caller_trace.get("reason_code") is not None
        or caller_edges
        != [
            {
                "anchor_id": _cc_catalog.MSVC_CIASIN_CALLER_ANCHOR_ID,
                "emission_context": {
                    "translation_unit": _cc_catalog.MSVC_CIASIN_CALLER_SOURCE_PATH
                },
                "evidence_ids": [],
                "relation": "defines",
            }
        ]
        or indexes.by_address.get(_cc_catalog.MSVC_CIASIN_CALLER_ADDRESS)
        != _cc_catalog.MSVC_CIASIN_CALLER_IDENTITY
        or _cc_catalog.MSVC_CIASIN_CALLER_IDENTITY in indexes.provider_ids
        or not isinstance(caller_block, Mapping)
        or caller_block.get("binary") != "recoil"
        or caller_block.get("row_kind") != "physical-source-block"
        or caller_block.get("start") != "0x472670"
        or caller_block.get("end_exclusive") != "0x475c40"
        or caller_block.get("contribution_kind") != "authored"
        or caller_block.get("agent_source_path")
        != _cc_catalog.MSVC_CIASIN_CALLER_SOURCE_PATH
        or caller_block.get("original_source_path")
        != _cc_catalog.MSVC_CIASIN_CALLER_SOURCE_PATH
        or caller_block.get("source_path") != _cc_catalog.MSVC_CIASIN_CALLER_SOURCE_PATH
        or not isinstance(caller_block.get("contribution_ids"), list)
        or caller_block["contribution_ids"].count(
            _cc_catalog.MSVC_CIASIN_CALLER_IDENTITY.removeprefix("symbol:")
        )
        != 1
    ):
        raise ValueError(
            "MSVC __CIasin bridge requires its exact governed authored zMath "
            "caller and resolved source provenance"
        )

    target_row = document.collection("symbols").get(
        _cc_catalog.MSVC_CIASIN_TARGET_SYMBOL_ID
    )
    target_block = document.collection("physical_blocks").get(
        _cc_catalog.MSVC_CIASIN_TARGET_BLOCK_ID
    )
    target_mapping = (
        target_block.get("mapping")
        if isinstance(target_block, Mapping)
        else None
    )
    if (
        not isinstance(target_row, Mapping)
        or target_row.get("binary") != "recoil"
        or target_row.get("kind") != "function"
        or target_row.get("pipeline_class") != "non-authored"
        or target_row.get("authored_order_role") != "non-authored"
        or target_row.get("address") != _cc_catalog.MSVC_CIASIN_TARGET_ADDRESS
        or target_row.get("end_exclusive")
        != _cc_catalog.MSVC_CIASIN_TARGET_END_EXCLUSIVE
        or target_row.get("extent_state") != "known"
        or target_row.get("size") != 0x10
        or target_row.get("navigation_name") != _cc_catalog.MSVC_CIASIN_RETAIL_NAME
        or target_row.get("output_section_id") != "recoil:section:.text"
        or target_row.get("physical_block_id") != _cc_catalog.MSVC_CIASIN_TARGET_BLOCK_ID
        or target_row.get("disposition") != "unresolved"
        or target_row.get("ownership_state") != "unresolved"
        or any(
            key in target_row
            for key in (
                "import_dll",
                "import_name",
                "import_ordinal",
                "provider_object_identity",
            )
        )
        or indexes.by_address.get(_cc_catalog.MSVC_CIASIN_TARGET_ADDRESS)
        != _cc_catalog.MSVC_CIASIN_TARGET_IDENTITY
        or _cc_catalog.MSVC_CIASIN_TARGET_IDENTITY not in indexes.provider_ids
        or not isinstance(target_block, Mapping)
        or target_block.get("binary") != "recoil"
        or target_block.get("row_kind") != "physical-source-block"
        or target_block.get("start") != "0x4c60b0"
        or target_block.get("end_exclusive") != "0x4c637c"
        or target_block.get("contribution_kind") != "provider"
        or target_block.get("agent_source_path")
        != "provider:vc5-crt-startup-runtime"
        or target_block.get("source_path")
        != "provider:vc5-crt-startup-runtime"
        or not isinstance(target_block.get("contribution_ids"), list)
        or target_block["contribution_ids"].count(
            _cc_catalog.MSVC_CIASIN_TARGET_SYMBOL_ID
        )
        != 1
        or not isinstance(target_mapping, Mapping)
        or target_mapping.get("status") != "provider-boundary"
    ):
        raise ValueError(
            "MSVC __CIasin bridge lacks the exact distinct reviewed retail "
            "provider target"
        )

    retail_symbols = bridge_names.get(_cc_catalog.MSVC_CIASIN_RETAIL_NAME)
    if (
        not isinstance(retail_symbols, Sequence)
        or isinstance(retail_symbols, (str, bytes))
        or len(retail_symbols) != 1
    ):
        raise ValueError(
            "MSVC __CIasin bridge requires one exact retail _CIasin function "
            "symbol"
        )
    retail_symbol = retail_symbols[0]
    if (
        getattr(retail_symbol, "address", "")
        != _cc_catalog.MSVC_CIASIN_TARGET_ADDRESS
        or getattr(retail_symbol, "name", "") != _cc_catalog.MSVC_CIASIN_RETAIL_NAME
        or getattr(retail_symbol, "raw_name", "") != _cc_catalog.MSVC_CIASIN_RETAIL_NAME
        or getattr(retail_symbol, "full_name", "")
        not in {"", _cc_catalog.MSVC_CIASIN_RETAIL_NAME}
        or getattr(retail_symbol, "kind", "") != "import"
        or _cc_catalog.MSVC_CIASIN_CANDIDATE_SYMBOL in bridge_names
        or _cc_catalog.MSVC_CIASIN_CANDIDATE_SYMBOL in indexes.by_candidate_name
        or _cc_catalog.MSVC_CIASIN_CANDIDATE_SYMBOL in indexes.storage_by_name
    ):
        raise ValueError(
            "MSVC __CIasin bridge has retail-name, target-address, or "
            "candidate-name collision drift"
        )

    expected_rows = [
        row
        for row in expected
        if row.get("target_identity") == _cc_catalog.MSVC_CIASIN_TARGET_IDENTITY
    ]
    if (
        len(expected_rows) != 1
        or expected_rows[0].get("form") != "call"
        or expected_rows[0].get("dispatch") != "direct"
        or expected_rows[0].get("identity_kind") != "provider"
        or expected_rows[0].get("storage_identity") != ""
        or expected_rows[0].get("slot_displacement") is not None
        or expected_rows[0].get("cleanup_bytes") is not None
    ):
        raise ValueError(
            "MSVC __CIasin bridge lacks the exact candidate-independent retail "
            "direct provider invocation"
        )

    target = candidate.target
    target_rows = [
        row
        for row in getattr(target, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.MSVC_CIASIN_CALLER_ADDRESS
    ]
    if (
        target is None
        or getattr(target, "name", "") != _cc_catalog.MSVC_CIASIN_TARGET_NAME
        or getattr(target, "target_binary", "") != "recoil"
        or Path(str(getattr(target, "manifest_path", ""))).resolve()
        != _cc_catalog.MSVC_CIASIN_TARGET_MANIFEST.resolve()
        or getattr(target, "source_from", "")
        != _cc_catalog.MSVC_CIASIN_CALLER_SOURCE_PATH
        or tuple(getattr(target, "order_edit_paths", ()))
        != _cc_catalog.MSVC_CIASIN_ORDER_EDIT_PATHS
        or not bool(
            getattr(target, "check_translation_unit_function_order", False)
        )
        or len(target_rows) != 1
    ):
        raise ValueError(
            "MSVC __CIasin bridge requires the exact selected zMath source "
            "target and manifest"
        )
    selected_row = target_rows[0]
    if (
        getattr(selected_row, "symbol", "") != _cc_catalog.MSVC_CIASIN_CALLER_SYMBOL
        or getattr(selected_row, "symbol_regex", None) is not None
        or getattr(selected_row, "name", "") != "zMath::CrtMatherrHandler"
        or getattr(selected_row, "pipeline_class", "") != "authored"
        or getattr(selected_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(selected_row, "required_presence", False))
        or not bool(getattr(selected_row, "full_order_gate", False))
    ):
        raise ValueError(
            "MSVC __CIasin bridge requires the exact selected authored caller "
            "row"
        )

    registered_target = document.collection("verification_targets").get(
        _cc_catalog.MSVC_CIASIN_TARGET_ID
    )
    registration = (
        registered_target.get("registration")
        if isinstance(registered_target, Mapping)
        else None
    )
    if (
        not isinstance(registered_target, Mapping)
        or registered_target.get("binary") != "recoil"
        or registered_target.get("kind") != "vc5"
        or registered_target.get("name") != _cc_catalog.MSVC_CIASIN_TARGET_NAME
        or not isinstance(registration, Mapping)
        or registration.get("binary") != "recoil"
        or registration.get("name") != _cc_catalog.MSVC_CIASIN_TARGET_NAME
        or registration.get("manifest_path")
        != _cc_catalog.MSVC_CIASIN_TARGET_MANIFEST.relative_to(REPO_ROOT).as_posix()
        or registration.get("source_from")
        != _cc_catalog.MSVC_CIASIN_CALLER_SOURCE_PATH
        or registration.get("check_translation_unit_function_order") is not True
        or registration.get("function_order_scope") != "authored"
        or registration.get("function_addresses", []).count(
            _cc_catalog.MSVC_CIASIN_CALLER_ADDRESS
        )
        != 1
    ):
        raise ValueError(
            "MSVC __CIasin bridge requires the synchronized exact zMath "
            "verification registration"
        )

    definition = candidate.caller_definition
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    call_rows = [
        (index, instruction)
        for index, (instruction, offset) in enumerate(
            zip(candidate.instructions, offsets)
        )
        if offset == _cc_catalog.MSVC_CIASIN_CALL_OFFSET
    ]
    if (
        definition is None
        or definition.symbol != _cc_catalog.MSVC_CIASIN_CALLER_SYMBOL
        or len(definition.data) != 0x1A0
        or len(definition.relocation_mask) != len(definition.data)
        or definition.section_start != 0
        or definition.section_end != 0x1A0
        or len(mentions) != 1
        or len(call_rows) != 1
        or call_rows[0][1] is not mentions[0]
        or tuple(mentions[0].bytes) != ("e8", "00", "00", "00", "00")
        or definition.undefined_external_functions.count(
            _cc_catalog.MSVC_CIASIN_CANDIDATE_SYMBOL
        )
        != 1
        or _cc_catalog.MSVC_CIASIN_CANDIDATE_SYMBOL
        in (
            definition.defined_external_functions
            + definition.undefined_external_data
            + definition.defined_external_data
        )
    ):
        raise ValueError(
            "MSVC __CIasin bridge requires the exact offset-0xee direct COD "
            "call, complete caller COFF definition, and unique undefined "
            "provider spelling"
        )

    references = tuple(
        relocation
        for relocation in definition.relocations
        if relocation.symbol_name == _cc_catalog.MSVC_CIASIN_CANDIDATE_SYMBOL
    )
    if (
        len(references) != 1
        or references[0].offset != _cc_catalog.MSVC_CIASIN_RELOCATION_OFFSET
        or references[0].type != IMAGE_REL_I386_REL32
        or definition.data[_cc_catalog.MSVC_CIASIN_CALL_OFFSET]
        != 0xE8
        or struct.unpack_from(
            "<I", definition.data, _cc_catalog.MSVC_CIASIN_RELOCATION_OFFSET
        )[0]
        != 0
        or not all(
            definition.relocation_mask[index]
            for index in range(
                _cc_catalog.MSVC_CIASIN_RELOCATION_OFFSET,
                _cc_catalog.MSVC_CIASIN_RELOCATION_OFFSET + 4,
            )
        )
    ):
        raise ValueError(
            "MSVC __CIasin bridge requires one exact zero-addend fully masked "
            "offset-0xef E8 REL32 relocation"
        )

    return {_cc_catalog.MSVC_CIASIN_CANDIDATE_SYMBOL: _cc_catalog.MSVC_CIASIN_TARGET_IDENTITY}


def _require_zwep_call_contract_candidate_profile(
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> Mapping[str, Any] | None:
    """Require one exact selected zWep authored caller and governed target."""

    start = normalize_address(caller_start)
    profile = (
        _cc_catalog.ZWEP_CIASIN_CANDIDATE_PROFILES.get(start)
        or _cc_catalog.ZWEP_DAMAGE_CANDIDATE_PROFILES.get(start)
        or (
            {
                "end": _cc_catalog.ZWEP_ENTRY_CALLBACK_CALLER_END,
                "symbol": _cc_catalog.ZWEP_ENTRY_CALLBACK_CALLER_SYMBOL,
                "name": "zWeapon::LoadOptCatalogFromPath",
            }
            if start == _cc_catalog.ZWEP_ENTRY_CALLBACK_CALLER_START
            else None
        )
    )
    if profile is None and start != _cc_catalog.ZWEP_RUNTIME_CALLBACK_CALLER_START:
        return None
    expected_end = str(
        profile["end"] if profile is not None else _cc_catalog.ZWEP_RUNTIME_CALLBACK_CALLER_END
    )
    expected_symbol = str(
        profile["symbol"]
        if profile is not None
        else _cc_catalog.ZWEP_RUNTIME_CALLBACK_CALLER_SYMBOL
    )
    expected_name = str(
        profile["name"]
        if profile is not None
        else "OptCatalog::ProcessRuntimeInstances"
    )
    expected_identity = f"symbol:recoil:function:{start}"
    caller_row = document.collection("symbols").get(
        expected_identity.removeprefix("symbol:")
    )
    target = candidate.target
    target_rows = [
        row
        for row in getattr(target, "functions", ())
        if normalize_address(str(getattr(row, "address", ""))) == start
    ]
    registered = document.collection("verification_targets").get(
        _cc_catalog.ZWEP_CALL_CONTRACT_TARGET_ID
    )
    registration = (
        registered.get("registration")
        if isinstance(registered, Mapping)
        else None
    )
    definition = candidate.caller_definition
    if (
        caller_identity != expected_identity
        or normalize_address(caller_end_exclusive) != expected_end
        or indexes.by_address.get(start) != expected_identity
        or expected_identity in indexes.provider_ids
        or not isinstance(caller_row, Mapping)
        or caller_row.get("binary") != "recoil"
        or caller_row.get("kind") != "function"
        or caller_row.get("pipeline_class") != "authored"
        or caller_row.get("authored_order_role") not in {None, "authored-body"}
        or caller_row.get("address") != start
        or caller_row.get("end_exclusive") != expected_end
        or caller_row.get("extent_state") != "known"
        or caller_row.get("size")
        != address_value(expected_end) - address_value(start)
        or caller_row.get("verification_target_ids", []).count(
            _cc_catalog.ZWEP_CALL_CONTRACT_TARGET_ID
        )
        != 1
        or target is None
        or getattr(target, "name", "") != _cc_catalog.ZWEP_CALL_CONTRACT_TARGET_NAME
        or getattr(target, "target_binary", "") != "recoil"
        or Path(str(getattr(target, "manifest_path", ""))).resolve()
        != _cc_catalog.ZWEP_CALL_CONTRACT_TARGET_MANIFEST.resolve()
        or getattr(target, "source_from", "")
        != _cc_catalog.ZWEP_CALL_CONTRACT_SOURCE_PATH
        or tuple(getattr(target, "order_edit_paths", ()))
        != _cc_catalog.ZWEP_CALL_CONTRACT_ORDER_EDIT_PATHS
        or not bool(
            getattr(target, "check_translation_unit_function_order", False)
        )
        or len(target_rows) != 1
        or getattr(target_rows[0], "symbol", "") != expected_symbol
        or getattr(target_rows[0], "symbol_regex", None) is not None
        or getattr(target_rows[0], "name", "") != expected_name
        or getattr(target_rows[0], "pipeline_class", "") != "authored"
        or getattr(target_rows[0], "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(target_rows[0], "required_presence", False))
        or not bool(getattr(target_rows[0], "full_order_gate", False))
        or not isinstance(registered, Mapping)
        or registered.get("binary") != "recoil"
        or registered.get("kind") != "vc5"
        or registered.get("name") != _cc_catalog.ZWEP_CALL_CONTRACT_TARGET_NAME
        or not isinstance(registration, Mapping)
        or registration.get("manifest_path")
        != _cc_catalog.ZWEP_CALL_CONTRACT_TARGET_MANIFEST.relative_to(REPO_ROOT).as_posix()
        or registration.get("source_from") != _cc_catalog.ZWEP_CALL_CONTRACT_SOURCE_PATH
        or registration.get("check_translation_unit_function_order") is not True
        or registration.get("function_order_scope") != "authored"
        or registration.get("function_addresses", []).count(start) != 1
        or definition is None
        or definition.symbol != expected_symbol
        or definition.section_start != 0
        or definition.section_end != len(definition.data)
        or len(definition.relocation_mask) != len(definition.data)
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "zWep finite call-contract bridge rejects caller, extent, body, "
            "selected target, manifest, source, or registration drift"
        )
    return profile or {
        "end": expected_end,
        "symbol": expected_symbol,
        "name": expected_name,
    }


def _require_zwep_ciasin_retail_provider_authority(
    *,
    document: ProgressDocument,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
) -> None:
    """Require the existing reviewed unresolved-symbol/provider-block package."""

    target_row = document.collection("symbols").get(
        _cc_catalog.MSVC_CIASIN_TARGET_SYMBOL_ID
    )
    target_block = document.collection("physical_blocks").get(
        _cc_catalog.MSVC_CIASIN_TARGET_BLOCK_ID
    )
    target_mapping = (
        target_block.get("mapping")
        if isinstance(target_block, Mapping)
        else None
    )
    retail_symbols = bridge_names.get(_cc_catalog.MSVC_CIASIN_RETAIL_NAME)
    retail_symbol = (
        retail_symbols[0]
        if isinstance(retail_symbols, Sequence)
        and not isinstance(retail_symbols, (str, bytes))
        and len(retail_symbols) == 1
        else None
    )
    if (
        not isinstance(target_row, Mapping)
        or target_row.get("binary") != "recoil"
        or target_row.get("kind") != "function"
        or target_row.get("pipeline_class") != "non-authored"
        or target_row.get("authored_order_role") != "non-authored"
        or target_row.get("address") != _cc_catalog.MSVC_CIASIN_TARGET_ADDRESS
        or target_row.get("end_exclusive")
        != _cc_catalog.MSVC_CIASIN_TARGET_END_EXCLUSIVE
        or target_row.get("extent_state") != "known"
        or target_row.get("size") != 0x10
        or target_row.get("navigation_name") != _cc_catalog.MSVC_CIASIN_RETAIL_NAME
        or target_row.get("physical_block_id") != _cc_catalog.MSVC_CIASIN_TARGET_BLOCK_ID
        or target_row.get("disposition") != "unresolved"
        or target_row.get("ownership_state") != "unresolved"
        or indexes.by_address.get(_cc_catalog.MSVC_CIASIN_TARGET_ADDRESS)
        != _cc_catalog.MSVC_CIASIN_TARGET_IDENTITY
        or _cc_catalog.MSVC_CIASIN_TARGET_IDENTITY not in indexes.provider_ids
        or not isinstance(target_block, Mapping)
        or target_block.get("binary") != "recoil"
        or target_block.get("row_kind") != "physical-source-block"
        or target_block.get("start") != "0x4c60b0"
        or target_block.get("end_exclusive") != "0x4c637c"
        or target_block.get("contribution_kind") != "provider"
        or target_block.get("agent_source_path")
        != "provider:vc5-crt-startup-runtime"
        or target_block.get("source_path")
        != "provider:vc5-crt-startup-runtime"
        or target_block.get("contribution_ids", []).count(
            _cc_catalog.MSVC_CIASIN_TARGET_SYMBOL_ID
        )
        != 1
        or not isinstance(target_mapping, Mapping)
        or target_mapping.get("status") != "provider-boundary"
        or retail_symbol is None
        or getattr(retail_symbol, "address", "") != _cc_catalog.MSVC_CIASIN_TARGET_ADDRESS
        or getattr(retail_symbol, "name", "") != _cc_catalog.MSVC_CIASIN_RETAIL_NAME
        or getattr(retail_symbol, "raw_name", "") != _cc_catalog.MSVC_CIASIN_RETAIL_NAME
        or getattr(retail_symbol, "full_name", "")
        not in {"", _cc_catalog.MSVC_CIASIN_RETAIL_NAME}
        or getattr(retail_symbol, "kind", "") != "import"
        or _cc_catalog.MSVC_CIASIN_CANDIDATE_SYMBOL in bridge_names
        or _cc_catalog.MSVC_CIASIN_CANDIDATE_SYMBOL in indexes.by_candidate_name
        or _cc_catalog.MSVC_CIASIN_CANDIDATE_SYMBOL in indexes.storage_by_name
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "zWep asin bridge lacks the exact reviewed retail provider authority"
        )


def _zwep_ciasin_provider_candidate_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    named_import_thunks: Sequence[
        ProviderNamedImportThunk | ProviderPeNamedImportThunk
    ] = (),
    reviewed_provider_iat_equivalences_out: dict[str, str] | None = None,
) -> dict[str, str]:
    """Bind only the exact zWep compiler-helper populations to retail."""
    from _recoil.call_contract.records import (
        ProviderNamedImportThunk,
        ProviderPeNamedImportThunk,
    )

    start = normalize_address(caller_start)
    profile = _cc_catalog.ZWEP_CIASIN_CANDIDATE_PROFILES.get(start)
    if profile is None:
        return {}
    _require_zwep_call_contract_candidate_profile(
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    _require_zwep_ciasin_retail_provider_authority(
        document=document,
        indexes=indexes,
        bridge_names=bridge_names,
    )
    definition = candidate.caller_definition
    assert definition is not None
    if (
        len(definition.data) != int(profile["body_size"])
        or definition.section_end != int(profile["body_size"])
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "zWep asin bridge rejects exact caller body extent drift"
        )
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=start,
        caller_end_exclusive=caller_end_exclusive,
    )
    expected_sites = tuple(profile["sites"])
    actual_sites = tuple(
        (int(offset), _cc_cfg._instruction_operand(instruction).strip())
        for instruction, offset in zip(candidate.instructions, offsets)
        if offset is not None
        and _cc_cfg._instruction_operand(instruction).strip()
        in {_cc_catalog.MSVC_CIASIN_CANDIDATE_SYMBOL, "@asinf@4"}
    )
    if actual_sites != expected_sites:
        raise _cc_errors.CandidateCallContractEvidenceError(
            "zWep asin bridge candidate call-site population drifted"
        )
    index_by_offset = {
        int(offset): index
        for index, offset in enumerate(offsets)
        if offset is not None
    }
    fsqrt_offset = profile.get("fsqrt_offset")
    if fsqrt_offset is not None:
        fsqrt_index = index_by_offset.get(int(fsqrt_offset), -1)
        fsqrt_instruction = (
            candidate.instructions[fsqrt_index]
            if fsqrt_index >= 0
            else None
        )
        if (
            fsqrt_instruction is None
            or _cc_cfg._instruction_mnemonic(fsqrt_instruction) != "fsqrt"
            or bytes(int(item, 16) for item in fsqrt_instruction.bytes)
            != b"\xd9\xfa"
            or definition.data[int(fsqrt_offset) : int(fsqrt_offset) + 2]
            != b"\xd9\xfa"
            or any(
                row.offset in {int(fsqrt_offset), int(fsqrt_offset) + 1}
                for row in definition.relocations
            )
            or fsqrt_index in invocation_indices
        ):
            raise _cc_errors.CandidateCallContractEvidenceError(
                "zWep asin bridge rejects exact inline FSQRT provenance drift"
            )
    retail_ordinals = tuple(int(item) for item in profile["retail_ordinals"])
    retail_rows = tuple(
        row
        for row in expected
        if row.get("target_identity") == _cc_catalog.MSVC_CIASIN_TARGET_IDENTITY
    )
    if (
        tuple(sorted(int(row.get("ordinal", -1)) for row in retail_rows))
        != retail_ordinals
        or any(
            row.get("form") != "call"
            or row.get("dispatch") != "direct"
            or row.get("identity_kind") != "provider"
            or row.get("storage_identity") != ""
            or row.get("slot_displacement") is not None
            or row.get("cleanup_bytes") is not None
            for row in retail_rows
        )
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "zWep asin bridge rejects exact immutable retail provider "
            "population drift"
        )
    for offset, symbol in expected_sites:
        instruction_index = index_by_offset.get(int(offset), -1)
        instruction = (
            candidate.instructions[instruction_index]
            if instruction_index >= 0
            else None
        )
        relocation_rows = [
            row
            for row in definition.relocations
            if row.offset == int(offset) + 1 and row.symbol_name == symbol
        ]
        if (
            instruction is None
            or _cc_cfg._instruction_mnemonic(instruction) != "call"
            or bytes(int(item, 16) for item in instruction.bytes)
            != b"\xe8\x00\x00\x00\x00"
            or len(relocation_rows) != 1
            or relocation_rows[0].type != IMAGE_REL_I386_REL32
            or definition.data[int(offset) : int(offset) + 5]
            != b"\xe8\x00\x00\x00\x00"
            or not all(
                definition.relocation_mask[int(offset) + 1 : int(offset) + 5]
            )
            or instruction_index not in invocation_indices
        ):
            raise _cc_errors.CandidateCallContractEvidenceError(
                "zWep asin bridge rejects exact COD/COFF invocation drift"
            )
    published = {symbol for _offset, symbol in expected_sites}
    # A separately emitted asinf wrapper is not the physical CIasin target,
    # even when its body ultimately invokes that helper.  Keep such calls in
    # actual_sites above so they fail the direct-helper population proof.
    if published != {_cc_catalog.MSVC_CIASIN_CANDIDATE_SYMBOL}:
        raise _cc_errors.CandidateCallContractEvidenceError(
            "zWep asin bridge requires direct compiler-helper targets"
        )
    external_counts = {
        symbol: (
            definition.undefined_external_functions.count(symbol),
            definition.defined_external_functions.count(symbol),
            definition.undefined_external_data.count(symbol),
            definition.defined_external_data.count(symbol),
        )
        for symbol in published
    }
    if any(
        counts != (1, 0, 0, 0)
        for symbol, counts in external_counts.items()
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "zWep asin bridge rejects external symbol population drift"
        )
    result = {
        symbol: _cc_catalog.MSVC_CIASIN_TARGET_IDENTITY for symbol in sorted(published)
    }
    if start == _cc_catalog.ZWEP_RUNTIME_CALLBACK_CALLER_START:
        ciacos_thunks = [
            thunk
            for thunk in named_import_thunks
            if normalize_address(thunk.thunk_address)
            == _cc_catalog.ZWEP_CIACOS_TARGET_ADDRESS
        ]
        ciacos_thunk = ciacos_thunks[0] if len(ciacos_thunks) == 1 else None
        ciacos_rows = [
            (index, instruction)
            for index, (instruction, offset) in enumerate(
                zip(candidate.instructions, offsets)
            )
            if offset == _cc_catalog.ZWEP_CIACOS_CALL_OFFSET
        ]
        ciacos_relocations = [
            row
            for row in definition.relocations
            if row.offset == _cc_catalog.ZWEP_CIACOS_CALL_OFFSET + 1
            and row.symbol_name == _cc_catalog.ZWEP_CIACOS_CANDIDATE_SYMBOL
        ]
        expected_ciacos = [
            row
            for row in expected
            if row.get("target_identity") == _cc_catalog.ZWEP_CIACOS_IAT_IDENTITY
        ]
        ciacos_current_storage_drift = (
            isinstance(ciacos_thunk, ProviderNamedImportThunk)
            and (
                indexes.storage_by_address.get(_cc_catalog.ZWEP_CIACOS_IAT_ADDRESS)
                != _cc_catalog.ZWEP_CIACOS_IAT_IDENTITY
                or indexes.storage_by_name.get(
                    ciacos_thunk.iat_object_symbol
                )
                != _cc_catalog.ZWEP_CIACOS_IAT_IDENTITY
            )
        )
        expected_ciacos_row = {
            "ordinal": _cc_catalog.ZWEP_CIACOS_RETAIL_ORDINAL,
            "form": "call",
            "dispatch": "direct",
            "identity_kind": "iat",
            "target_identity": _cc_catalog.ZWEP_CIACOS_IAT_IDENTITY,
            "storage_identity": "",
            "slot_displacement": None,
            "cleanup_bytes": None,
        }
        if expected_ciacos != [expected_ciacos_row]:
            raise _cc_errors.CandidateCallContractEvidenceError(
                "zWep CIacos bridge rejects exact immutable retail ordinal/row drift"
            )
        if (
            ciacos_thunk is None
            or not isinstance(
                ciacos_thunk,
                (ProviderNamedImportThunk, ProviderPeNamedImportThunk),
            )
            or ciacos_thunk.provider_identity
            != _cc_catalog.ZWEP_CIACOS_TARGET_IDENTITY
            or ciacos_thunk.retail_name != _cc_catalog.ZWEP_CIACOS_RETAIL_NAME
            or normalize_address(ciacos_thunk.iat_address)
            != _cc_catalog.ZWEP_CIACOS_IAT_ADDRESS
            or ciacos_thunk.iat_identity != _cc_catalog.ZWEP_CIACOS_IAT_IDENTITY
            or ciacos_thunk.import_dll.casefold() != "msvcrt.dll"
            or ciacos_thunk.import_name != _cc_catalog.ZWEP_CIACOS_RETAIL_NAME
            or (
                isinstance(ciacos_thunk, ProviderNamedImportThunk)
                and ciacos_thunk.callable_symbol
                != _cc_catalog.ZWEP_CIACOS_CANDIDATE_SYMBOL
            )
        ):
            raise _cc_errors.CandidateCallContractEvidenceError(
                "zWep CIacos bridge rejects exact retail thunk/IAT/import package drift"
            )
        if (
            indexes.by_address.get(_cc_catalog.ZWEP_CIACOS_TARGET_ADDRESS)
            != _cc_catalog.ZWEP_CIACOS_TARGET_IDENTITY
            or _cc_catalog.ZWEP_CIACOS_TARGET_IDENTITY not in indexes.provider_ids
            or ciacos_current_storage_drift
        ):
            raise _cc_errors.CandidateCallContractEvidenceError(
                "zWep CIacos bridge rejects exact provider/current-storage authority drift"
            )
        if (
            len(ciacos_rows) != 1
            or ciacos_rows[0][0] not in invocation_indices
            or _cc_cfg._instruction_mnemonic(ciacos_rows[0][1]) != "call"
            or _cc_cfg._instruction_operand(ciacos_rows[0][1]).strip()
            != _cc_catalog.ZWEP_CIACOS_CANDIDATE_SYMBOL
            or bytes(int(item, 16) for item in ciacos_rows[0][1].bytes)
            != b"\xe8\x00\x00\x00\x00"
            or len(ciacos_relocations) != 1
            or ciacos_relocations[0].type != IMAGE_REL_I386_REL32
            or definition.data[
                _cc_catalog.ZWEP_CIACOS_CALL_OFFSET : _cc_catalog.ZWEP_CIACOS_CALL_OFFSET + 5
            ]
            != b"\xe8\x00\x00\x00\x00"
            or not all(
                definition.relocation_mask[
                    _cc_catalog.ZWEP_CIACOS_CALL_OFFSET + 1 :
                    _cc_catalog.ZWEP_CIACOS_CALL_OFFSET + 5
                ]
            )
            or definition.undefined_external_functions.count(
                _cc_catalog.ZWEP_CIACOS_CANDIDATE_SYMBOL
            )
            != 1
            or _cc_catalog.ZWEP_CIACOS_CANDIDATE_SYMBOL
            in (
                definition.defined_external_functions
                + definition.undefined_external_data
                + definition.defined_external_data
            )
        ):
            raise _cc_errors.CandidateCallContractEvidenceError(
                "zWep CIacos bridge rejects exact candidate COD/COFF drift"
            )
        result[_cc_catalog.ZWEP_CIACOS_CANDIDATE_SYMBOL] = (
            _cc_catalog.ZWEP_CIACOS_IAT_IDENTITY
        )
        if reviewed_provider_iat_equivalences_out is not None:
            if reviewed_provider_iat_equivalences_out:
                raise _cc_errors.CandidateCallContractEvidenceError(
                    "zWep CIacos bridge requires an empty comparison-scoped "
                    "provider/IAT equivalence sink"
                )
            reviewed_provider_iat_equivalences_out[
                _cc_catalog.ZWEP_CIACOS_IAT_IDENTITY
            ] = _cc_catalog.ZWEP_CIACOS_IAT_IDENTITY
    return result


def _zwep_runtime_callback_candidate_register_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedRegisterCallStorageBridge]:
    """Bind one exact current runtime+0x88 candidate callback unit."""
    from _recoil.call_contract.records import ReviewedRegisterCallStorageBridge

    if normalize_address(caller_start) != _cc_catalog.ZWEP_RUNTIME_CALLBACK_CALLER_START:
        return {}
    _require_zwep_call_contract_candidate_profile(
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    expected_callbacks = [
        row
        for row in expected
        if row.get("form") == "call"
        and row.get("dispatch") == "indirect"
        and row.get("identity_kind") == "callback"
        and row.get("target_identity") == ""
        and isinstance(row.get("storage_identity"), str)
        and bool(row.get("storage_identity"))
        and row.get("slot_displacement") is None
        and row.get("cleanup_bytes") is None
    ]
    if len(expected_callbacks) != 1:
        raise _cc_errors.CandidateCallContractEvidenceError(
            "zWep runtime callback bridge requires one exact targetless retail callback row"
        )
    definition = candidate.caller_definition
    shift, offsets, _by_offset, index_by_offset = (
        _cc_callable_identity._locate_shifted_candidate_instruction_unit(
            candidate,
            _cc_catalog.ZWEP_RUNTIME_CALLBACK_CANDIDATE_ROWS,
            label="zWep runtime callback",
        )
    )
    unit_start = min(_cc_catalog.ZWEP_RUNTIME_CALLBACK_CANDIDATE_ROWS)
    unit_end = max(
        offset + len(bytes_row)
        for offset, (bytes_row, _pattern) in
        _cc_catalog.ZWEP_RUNTIME_CALLBACK_CANDIDATE_ROWS.items()
    )
    if (
        shift != 0
        or definition is None
        or any(definition.relocation_mask[unit_start:unit_end])
        or any(
            unit_start <= row.offset < unit_end
            for row in definition.relocations
        )
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "zWep runtime callback rejects exact current offset or COFF "
            "relocation drift"
        )
    call_offset = _cc_catalog.ZWEP_RUNTIME_CALLBACK_BASELINE_CALL_OFFSET
    call_index = index_by_offset.get(call_offset, -1)
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    if (
        call_index < 0
        or call_index not in invocation_indices
        or offsets[call_index] != call_offset
        or candidate.local_control_flow_indices
        - frozenset(candidate.local_control_flow_targets)
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "zWep runtime callback bridge rejects invocation or CFG drift"
        )
    return {
        f"0x{call_offset:x}": ReviewedRegisterCallStorageBridge(
            register="eax",
            storage_identity=str(expected_callbacks[0]["storage_identity"]),
            identity_kind="callback",
            assembly_source="cod",
        )
    }


def _zwep_entry_callback_candidate_register_bridge(
    expected: Sequence[Mapping[str, Any]],
    retail_instructions: Sequence[Instruction],
    retail_invocation_call_sites: Sequence[str],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedRegisterCallStorageBridge]:
    """Normalize one exact entry-stack callback load across two VC5 frames."""
    from _recoil.call_contract.records import ReviewedRegisterCallStorageBridge

    if normalize_address(caller_start) != _cc_catalog.ZWEP_ENTRY_CALLBACK_CALLER_START:
        return {}
    _require_zwep_call_contract_candidate_profile(
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    expected_row = {
        "ordinal": _cc_catalog.ZWEP_ENTRY_CALLBACK_RETAIL_ORDINAL,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "callback",
        "target_identity": "",
        "storage_identity": "load(stack+0xc)",
        "slot_displacement": None,
        "cleanup_bytes": None,
    }
    if (
        len(expected) != _cc_catalog.ZWEP_ENTRY_CALLBACK_RETAIL_INVOCATION_COUNT
        or expected[_cc_catalog.ZWEP_ENTRY_CALLBACK_RETAIL_ORDINAL] != expected_row
        or len(retail_invocation_call_sites) != len(expected)
        or normalize_address(
            retail_invocation_call_sites[_cc_catalog.ZWEP_ENTRY_CALLBACK_RETAIL_ORDINAL]
        )
        != "0x4b1867"
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "zWep entry-callback bridge rejects exact immutable retail "
            "population, ordinal, call site, or storage row drift"
        )

    retail_addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(caller_start),
    )
    retail_counts: dict[int, int] = {}
    for address in retail_addresses:
        if address is not None:
            retail_counts[address] = retail_counts.get(address, 0) + 1
    retail_by_offset = {
        address - address_value(caller_start): instruction
        for instruction, address in zip(retail_instructions, retail_addresses)
        if address is not None and retail_counts.get(address) == 1
    }
    for offset, (body, pattern) in _cc_catalog.ZWEP_ENTRY_CALLBACK_RETAIL_ROWS.items():
        instruction = retail_by_offset.get(offset)
        if (
            instruction is None
            or tuple(value.lower() for value in instruction.bytes) != body
            or re.fullmatch(
                pattern,
                instruction.raw_text.strip(),
                flags=re.IGNORECASE,
            )
            is None
        ):
            raise _cc_errors.CandidateCallContractEvidenceError(
                "zWep entry-callback bridge rejects exact immutable retail "
                "prologue, entry-stack load, null branch, receiver, or call drift"
            )
    retail_entry_displacement = -4 + 0x0C
    if retail_entry_displacement != 8:
        raise _cc_errors.CandidateCallContractEvidenceError(
            "zWep entry-callback retail frame does not address entry S+8"
        )

    definition = candidate.caller_definition
    shift, offsets, by_offset, index_by_offset = (
        _cc_callable_identity._locate_shifted_candidate_instruction_unit(
            candidate,
            _cc_catalog.ZWEP_ENTRY_CALLBACK_CANDIDATE_ROWS,
            label="zWep entry callback",
        )
    )
    fixed_ranges = tuple(
        (offset, offset + len(body))
        for offset, (body, _pattern) in
        _cc_catalog.ZWEP_ENTRY_CALLBACK_CANDIDATE_ROWS.items()
    )
    if (
        shift != 0
        or definition is None
        or len(definition.data) != _cc_catalog.ZWEP_ENTRY_CALLBACK_CANDIDATE_BODY_SIZE
        or definition.section_start != 0
        or definition.section_end != _cc_catalog.ZWEP_ENTRY_CALLBACK_CANDIDATE_BODY_SIZE
        or any(
            any(definition.relocation_mask[start:end])
            for start, end in fixed_ranges
        )
        or any(
            start <= relocation.offset < end
            for relocation in definition.relocations
            for start, end in fixed_ranges
        )
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "zWep entry-callback bridge rejects exact candidate body, "
            "offset, or COFF relocation drift"
        )

    prologue = by_offset[0]
    symbolic_load = _cc_receiver_instructions._exact_vc5_symbolic_stack_slot_load(by_offset[0x6F5])
    candidate_entry_displacement = (
        -0x4C - 4 * 4 + int(symbolic_load[0])
        if symbolic_load is not None
        else None
    )
    if (
        not _cc_receiver_storage._is_exact_safe_stack_root_adjustment(
            prologue,
            operation="sub",
            parsed_immediate=0x4C,
        )
        or symbolic_load != (0x64, "eax", "_entryCallback$", 0x58)
        or candidate_entry_displacement != retail_entry_displacement
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "zWep entry-callback bridge rejects exact candidate entry-stack "
            "affine equivalence"
        )

    call_index = index_by_offset.get(
        _cc_catalog.ZWEP_ENTRY_CALLBACK_CANDIDATE_CALL_OFFSET,
        -1,
    )
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    exact_eax_calls = [
        index
        for index, instruction in enumerate(candidate.instructions)
        if tuple(value.lower() for value in instruction.bytes) == ("ff", "d0")
        and _cc_cfg._instruction_mnemonic(instruction) == "call"
        and _cc_cfg._instruction_operand(instruction).strip().lower() == "eax"
    ]
    bounded_indices = {
        index_by_offset[offset]
        for offset in (0x6F5, 0x6FF, 0x701, 0x703, 0x705, 0x707)
    }
    if (
        len(invocation_indices) != _cc_catalog.ZWEP_ENTRY_CALLBACK_CANDIDATE_INVOCATION_COUNT
        or call_index < 0
        or invocation_indices[_cc_catalog.ZWEP_ENTRY_CALLBACK_RETAIL_ORDINAL]
        != call_index
        or exact_eax_calls != [call_index]
        or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
        or candidate.local_control_flow_indices & bounded_indices
        or any(
            target in bounded_indices
            for targets in candidate.local_control_flow_targets.values()
            for target in targets
        )
        or candidate.local_control_flow_indices
        - frozenset(candidate.local_control_flow_targets)
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "zWep entry-callback bridge rejects exact candidate ordinal, "
            "unique call shape, cleanup, or CFG topology drift"
        )
    return {
        f"0x{_cc_catalog.ZWEP_ENTRY_CALLBACK_CANDIDATE_CALL_OFFSET:x}":
        ReviewedRegisterCallStorageBridge(
            register="eax",
            storage_identity=expected_row["storage_identity"],
            identity_kind="callback",
            assembly_source="cod",
        )
    }


def _zwep_damage_handler_candidate_vptr_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedLoopVptrStorageBridge]:
    """Bind only the two exact inline zWep handler record lineages."""
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge

    start = normalize_address(caller_start)
    profile = _cc_catalog.ZWEP_DAMAGE_CANDIDATE_PROFILES.get(start)
    if profile is None:
        return {}
    _require_zwep_call_contract_candidate_profile(
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    slot = int(profile["slot"])
    expected_rows = [
        row
        for row in expected
        if row.get("form") == "call"
        and row.get("dispatch") == "indirect"
        and row.get("identity_kind") == "virtual-slot"
        and row.get("target_identity") == ""
        and row.get("storage_identity")
        == _cc_catalog.ZWEP_DAMAGE_TIMER_HANDLER_VPTR_STORAGE
        and row.get("slot_displacement") == slot
        and row.get("cleanup_bytes") is None
    ]
    if len(expected_rows) != 1:
        raise _cc_errors.CandidateCallContractEvidenceError(
            "zWep inline damage-handler bridge requires one exact "
            "candidate-independent retail storage/slot row"
        )
    required_by_start: Mapping[
        str, Mapping[int, tuple[bytes, str]]
    ] = {
        _cc_catalog.ZWEP_DAMAGE_FEEDBACK_CALLER_START: {
            0x2E: (b"\x8b\x47\x24", r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[edi\+(?:36|0x24)\]"),
            0x31: (b"\x8b\xb0\xbc\x00\x00\x00", r"mov\s+esi\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[eax\+(?:188|0xbc)\]"),
            0x37: (b"\x3b\xf3", r"cmp\s+esi\s*,\s*ebx"),
            0x39: (b"\x75\x0a", r"jne\s+\S+"),
            0x3B: (b"\x33\xc0", r"xor\s+eax\s*,\s*eax"),
            0x3D: (b"\x5f", r"pop\s+edi"),
            0x3E: (b"\x5e", r"pop\s+esi"),
            0x3F: (b"\x5d", r"pop\s+ebp"),
            0x40: (b"\x5b", r"pop\s+ebx"),
            0x41: (b"\x59", r"pop\s+ecx"),
            0x42: (b"\xc2\x0c\x00", r"ret(?:n)?\s+(?:12|0xc)"),
            0x45: (b"\x83\xfe\x01", r"cmp\s+esi\s*,\s*(?:1|0x1)"),
            0x48: (b"\x75\x12", r"jne\s+\S+"),
            0x5C: (b"\x39\x5e\x04", r"cmp\s+(?:dword\s+(?:ptr\s+)?)?\[esi\+(?:4|0x4)\]\s*,\s*ebx"),
            0x5F: (b"\x0f\x84\xfa\x00\x00\x00", r"je\s+\S+"),
            0xC8: (b"\x8b\x57\x24", r"mov\s+edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[edi\+(?:36|0x24)\]"),
            0xCB: (b"\x53", r"push\s+ebx"),
            0xCC: (b"\x8b\xce", r"mov\s+ecx\s*,\s*esi"),
            0xCE: (b"\xff\xd0", r"call\s+eax"),
            0xD0: (b"\x8b\x0e", r"mov\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[esi\]"),
            0xD2: (b"\x53", r"push\s+ebx"),
            0xD3: (b"\x57", r"push\s+edi"),
            0xD4: (b"\x8b\xd5", r"mov\s+edx\s*,\s*ebp"),
            0xD6: (b"\xff\x56\x04", r"call\s+(?:dword\s+(?:ptr\s+)?)?\[esi\+(?:4|0x4)\]"),
            0xD9: (b"\x8b\xd8", r"mov\s+ebx\s*,\s*eax"),
        },
        _cc_catalog.ZWEP_DAMAGE_TIMER_CALLER_START: {
            0x00: (b"\x8b\x42\x24", r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[edx\+(?:36|0x24)\]"),
            0x03: (b"\x56", r"push\s+esi"),
            0x0A: (b"\x8b\x80\xbc\x00\x00\x00", r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[eax\+(?:188|0xbc)\]"),
            0x10: (b"\x83\xfe\x01", r"cmp\s+esi\s*,\s*(?:1|0x1)"),
            0x13: (b"\x75\x37", r"jne\s+\S+"),
            0x4C: (
                b"\x8b\x4c\x24\x08",
                r"mov\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
                r"(?:_damageAmount\$\[esp\]|\[esp\+(?:8|0x8)\])",
            ),
            0x50: (b"\x51", r"push\s+ecx"),
            0x51: (b"\x8b\x48\x08", r"mov\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[eax\+(?:8|0x8)\]"),
            0x54: (b"\xff\x50\x0c", r"call\s+(?:dword\s+(?:ptr\s+)?)?\[eax\+(?:12|0xc)\]"),
            0x57: (b"\x5e", r"pop\s+esi"),
            0x58: (b"\xc2\x04\x00", r"ret(?:n)?\s+(?:4|0x4)"),
        },
    }
    definition = candidate.caller_definition
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    counts = Counter(offset for offset in offsets if offset is not None)
    by_offset = {
        int(offset): (index, instruction)
        for index, (instruction, offset) in enumerate(
            zip(candidate.instructions, offsets)
        )
        if offset is not None and counts[offset] == 1
    }
    required = required_by_start[start]
    if (
        definition is None
        or definition.symbol != profile["symbol"]
        or len(definition.data) != int(profile["body_size"])
        or definition.section_start != 0
        or definition.section_end != int(profile["body_size"])
        or len(definition.relocation_mask) != len(definition.data)
        or any(
            offset not in by_offset
            or bytes(
                int(item, 16) for item in by_offset[offset][1].bytes
            )
            != body
            or definition.data[offset : offset + len(body)] != body
            or re.fullmatch(
                pattern,
                by_offset[offset][1].raw_text.strip(),
                flags=re.IGNORECASE,
            )
            is None
            for offset, (body, pattern) in required.items()
        )
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "zWep inline damage-handler bridge rejects exact candidate "
            "caller/COD/COFF lineage drift"
        )
    call_offset = int(profile["call_offset"])
    call_index = by_offset[call_offset][0]
    definition_offset = 0x31 if start == _cc_catalog.ZWEP_DAMAGE_FEEDBACK_CALLER_START else 0x0A
    definition_index = by_offset[definition_offset][0]
    unreachable_early_return_offsets = (
        frozenset(range(0x3B, 0x45))
        if start == _cc_catalog.ZWEP_DAMAGE_FEEDBACK_CALLER_START
        else frozenset()
    )
    if (
        call_index
        not in _cc_callable_identity._candidate_static_invocation_indices(
            candidate,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
        )
        or not definition_index < call_index
        or any(
            _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index], str(profile["register"])
            )
            for index in range(definition_index + 1, call_index)
            if offsets[index] not in unreachable_early_return_offsets
        )
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "zWep inline damage-handler bridge rejects exact bounded register "
            "lifetime or invocation drift"
        )
    return {
        f"0x{call_offset:x}": ReviewedLoopVptrStorageBridge(
            register=str(profile["register"]),
            storage_identity=_cc_catalog.ZWEP_DAMAGE_TIMER_HANDLER_VPTR_STORAGE,
            slot_displacement=slot,
            assembly_source="cod",
        )
    }
