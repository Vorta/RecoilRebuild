"""Recoil call-contract recoil hud lifetimes evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import comparison as _cc_comparison
from _recoil.call_contract import extraction as _cc_extraction
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract import listing as _cc_listing
from _recoil.call_contract import receiver_instructions as _cc_receiver_instructions

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        CandidateAssociatedSection,
        IdentityIndexes,
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedMemberVptrStorageBridge,
    )

import re
import struct
from pathlib import Path
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import IMAGE_REL_I386_DIR32, IMAGE_REL_I386_REL32
from _recoil.lib.binja import BinaryNinjaBridge
from _recoil.lib.progress import ProgressDocument, address_value, normalize_address
from _recoil.lib.source_traceability import parse_source_trace_path
from _recoil.lib.tooling import REPO_ROOT


def _hud_ui_mgr_first_deleting_destructor_candidate_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedAbsoluteStorageLoadBridge],
    dict[str, ReviewedMemberVptrStorageBridge],
]:
    """Prove only the first manager-global virtual deleting-destructor call."""
    from _recoil.call_contract.records import (
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedMemberVptrStorageBridge,
    )
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_START:
        return {}, {}
    if (
        caller_identity != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_END_EXCLUSIVE
    ):
        raise ValueError(
            "HUD first deleting-destructor bridge requires the exact "
            "reviewed ShutdownResources caller and extent"
        )

    definition = candidate.caller_definition
    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_START
    ]
    if (
        definition is None
        or definition.symbol != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_SYMBOL
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
            "HUD first deleting-destructor bridge requires the exact "
            "registered HUD target, caller definition, and contribution"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != r"\?ShutdownResources@HudUiMgr@@.*"
        or re.fullmatch(
            r"\?ShutdownResources@HudUiMgr@@.*",
            definition.symbol,
        )
        is None
        or getattr(contribution_row, "name", "")
        != "HudUiMgr::ShutdownResources"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD first deleting-destructor bridge requires the exact "
            "authored ShutdownResources hud.cpp contribution identity"
        )

    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    aggregate_start = address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
    field_address = normalize_address(
        aggregate_start + _cc_catalog.HUD_UI_MGR_TIMER_PANEL_FLOAT_DISPLACEMENT
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
        indexes.storage_by_address.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
        != aggregate_identity
        or indexes.storage_by_address.get(field_address, "") != ""
        or len(exact_containers) != 1
        or aggregate_identity in indexes.provider_ids
    ):
        raise ValueError(
            "HUD first deleting-destructor bridge rejects missing, aliased, "
            "provider, or conflicting aggregate storage provenance"
        )
    field_provenance = _cc_receiver_instructions._abstract_with_displacement(
        aggregate_identity,
        _cc_catalog.HUD_UI_MGR_TIMER_PANEL_FLOAT_DISPLACEMENT,
    )
    expected_receiver = f"load({field_provenance})"
    expected_storage = f"load({expected_receiver})"
    expected_rows = [
        row
        for row in expected
        if (
            row.get("identity_kind") == "virtual-slot"
            and row.get("storage_identity") == expected_storage
            and row.get("slot_displacement") == 0
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
            "HUD first deleting-destructor bridge requires one exact "
            "retail virtual slot-zero aggregate-field contract"
        )

    start = address_value(normalized_start)
    end = address_value(caller_end_exclusive)
    instructions = candidate.instructions
    addresses = _cc_cfg._instruction_runtime_addresses(
        instructions,
        source="cod",
        caller_start=start,
    )
    counts: dict[int, int] = {}
    for address in addresses:
        if address is not None:
            counts[address] = counts.get(address, 0) + 1
    by_offset = {
        address - start: instruction
        for instruction, address in zip(instructions, addresses)
        if address is not None and counts.get(address) == 1
    }
    index_by_address = {
        address: index
        for index, address in enumerate(addresses)
        if address is not None and counts.get(address) == 1
    }
    symbol_pattern = re.escape(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
    relative_rows = {
        0x00: (
            b"\x8b\x0d\x88\x47\x00\x00",
            rf"mov\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            rf"{symbol_pattern}\+18312",
        ),
        0x06: (b"\x33\xff", r"xor\s+edi\s*,\s*edi"),
        0x08: (b"\x3b\xcf", r"cmp\s+ecx\s*,\s*edi"),
        0x0A: (
            b"\x74\x0c",
            r"je\s+(?:SHORT\s+)?\$L[0-9]+",
        ),
        0x0C: (
            b"\x8b\x01",
            r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[ecx\]",
        ),
        0x0E: (b"\x6a\x01", r"push\s+1"),
        0x10: (
            b"\xff\x10",
            r"call\s+(?:dword\s+(?:ptr\s+)?)?\[eax\]",
        ),
        0x12: (
            b"\x89\x3d\x88\x47\x00\x00",
            rf"mov\s+(?:dword\s+(?:ptr\s+)?)?"
            rf"{symbol_pattern}\+18312\s*,\s*edi",
        ),
    }
    load_offsets = [
        offset
        for offset in by_offset
        if all(
            (
                offset + relative in by_offset
                and bytes(
                    int(value, 16)
                    for value in by_offset[offset + relative].bytes
                )
                == body
                and re.fullmatch(
                    pattern,
                    by_offset[offset + relative].raw_text.strip(),
                    flags=re.IGNORECASE,
                )
                is not None
                and definition.data[
                    offset + relative :
                    offset + relative + len(body)
                ]
                == body
            )
            for relative, (body, pattern) in relative_rows.items()
        )
    ]
    if len(load_offsets) != 1:
        raise ValueError(
            "HUD first deleting-destructor bridge requires one unique "
            "bounded global-delete instruction structure"
        )
    load_offset = load_offsets[0]
    relocation_specs = (
        load_offset + 0x02,
        load_offset + 0x14,
    )
    for relocation_offset in relocation_specs:
        relocations = [
            relocation
            for relocation in definition.relocations
            if relocation.offset == relocation_offset
        ]
        if (
            len(relocations) != 1
            or relocations[0].type != IMAGE_REL_I386_DIR32
            or relocations[0].symbol_name
            != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
            or struct.unpack_from(
                "<I",
                definition.data,
                relocation_offset,
            )[0]
            != _cc_catalog.HUD_UI_MGR_TIMER_PANEL_FLOAT_DISPLACEMENT
            or not all(
                definition.relocation_mask[index]
                for index in range(relocation_offset, relocation_offset + 4)
            )
        ):
            raise ValueError(
                "HUD first deleting-destructor bridge requires exact "
                "aggregate-field DIR32 relocations"
            )
    actual_bounded_relocations = [
        relocation
        for relocation in definition.relocations
        if load_offset <= relocation.offset < load_offset + 0x18
    ]
    if (
        len(actual_bounded_relocations) != 2
        or {
            relocation.offset
            for relocation in actual_bounded_relocations
        }
        != set(relocation_specs)
        or definition.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in definition.defined_external_data
    ):
        raise ValueError(
            "HUD first deleting-destructor bridge rejects extra, duplicate, "
            "or conflicting global storage references"
        )

    branch_offset = load_offset + 0x0A
    vptr_load_offset = load_offset + 0x0C
    call_offset = load_offset + 0x10
    target_offset = load_offset + 0x18
    branch_index = index_by_address.get(start + branch_offset)
    vptr_index = index_by_address.get(start + vptr_load_offset)
    call_index = index_by_address.get(start + call_offset)
    branch = (
        _cc_cfg._exact_local_direct_branch(
            instructions[branch_index],
            instruction_index=branch_index,
            instruction_addresses=addresses,
            instruction_index_by_address=index_by_address,
            source="cod",
            caller_start=start,
            caller_end=end,
        )
        if branch_index is not None
        else None
    )
    if (
        vptr_index is None
        or call_index is None
        or start + target_offset not in index_by_address
        or branch is None
        or branch[0] != "conditional"
        or addresses[branch[1]] != start + target_offset
        or any(
            _cc_cfg._instruction_may_clobber_register(
                instructions[index],
                "ecx",
            )
            for index in range(
                index_by_address[start + load_offset] + 1,
                vptr_index,
            )
        )
        or any(
            _cc_cfg._instruction_may_clobber_register(
                instructions[index],
                register,
            )
            for index in range(vptr_index + 1, call_index)
            for register in ("eax", "ecx")
        )
        or _cc_cfg._cleanup_after(instructions, call_index) is not None
        or any(
            definition.relocation_mask[
                call_offset : call_offset + 2
            ]
        )
    ):
        raise ValueError(
            "HUD first deleting-destructor bridge rejects wrong CFG, direct "
            "call, cleanup, receiver/vptr clobber, or masked call bytes"
        )

    receiver_provenance = (
        f"exact-load(ecx,{expected_receiver})"
    )
    return (
        {
            normalize_address(load_offset):
                ReviewedAbsoluteStorageLoadBridge(
                    register="ecx",
                    aggregate_symbol=(
                        _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
                    ),
                    displacement=(
                        _cc_catalog.HUD_UI_MGR_TIMER_PANEL_FLOAT_DISPLACEMENT
                    ),
                    access_width=(
                        _cc_catalog.HUD_UI_MGR_TIMER_PANEL_FLOAT_ACCESS_WIDTH
                    ),
                    storage_identity=aggregate_identity,
                )
        },
        {
            normalize_address(vptr_load_offset):
                ReviewedMemberVptrStorageBridge(
                    register="eax",
                    source_register="ecx",
                    source_provenance=receiver_provenance,
                    receiver_register="ecx",
                    receiver_provenance=receiver_provenance,
                    storage_identity=expected_storage,
                    slot_displacement=0,
                    call_address=normalize_address(call_offset),
                )
        },
    )


def _hud_ui_mgr_timer_panel_deleting_destructor_candidate_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedAbsoluteStorageLoadBridge],
    dict[str, ReviewedMemberVptrStorageBridge],
]:
    """Prove only the later g_HudUiMgrTimerPanel slot-zero deletion."""
    from _recoil.call_contract.records import (
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedMemberVptrStorageBridge,
    )
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_START:
        return {}, {}
    if (
        caller_identity != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_END_EXCLUSIVE
    ):
        raise ValueError(
            "HUD timer-panel deleting-destructor bridge requires the exact "
            "reviewed ShutdownResources caller and extent"
        )

    definition = candidate.caller_definition
    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_START
    ]
    if (
        definition is None
        or definition.symbol != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_SYMBOL
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
            "HUD timer-panel deleting-destructor bridge requires the exact "
            "registered HUD target, caller definition, and contribution"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != r"\?ShutdownResources@HudUiMgr@@.*"
        or re.fullmatch(
            r"\?ShutdownResources@HudUiMgr@@.*",
            definition.symbol,
        )
        is None
        or getattr(contribution_row, "name", "")
        != "HudUiMgr::ShutdownResources"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD timer-panel deleting-destructor bridge requires the exact "
            "authored ShutdownResources hud.cpp contribution identity"
        )

    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    aggregate_start = address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
    field_address = normalize_address(
        aggregate_start + _cc_catalog.HUD_UI_MGR_TIMER_PANEL_DISPLACEMENT
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
        indexes.storage_by_address.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
        != aggregate_identity
        or field_address != _cc_catalog.HUD_UI_MGR_TIMER_PANEL_ADDRESS
        or indexes.storage_by_address.get(field_address)
        != _cc_catalog.HUD_UI_MGR_TIMER_PANEL_STORAGE_IDENTITY
        or len(exact_containers) != 1
        or aggregate_identity in indexes.provider_ids
        or _cc_catalog.HUD_UI_MGR_TIMER_PANEL_STORAGE_IDENTITY in indexes.provider_ids
    ):
        raise ValueError(
            "HUD timer-panel deleting-destructor bridge rejects missing, "
            "aliased, provider, or conflicting aggregate storage provenance"
        )
    field_provenance = _cc_receiver_instructions._abstract_with_displacement(
        aggregate_identity,
        _cc_catalog.HUD_UI_MGR_TIMER_PANEL_DISPLACEMENT,
    )
    expected_storage = f"load({_cc_catalog.HUD_UI_MGR_TIMER_PANEL_STORAGE_IDENTITY})"
    expected_rows = [
        row
        for row in expected
        if (
            row.get("identity_kind") == "virtual-slot"
            and row.get("storage_identity") == expected_storage
            and row.get("slot_displacement") == 0
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
            "HUD timer-panel deleting-destructor bridge requires one exact "
            "retail virtual slot-zero aggregate-field contract"
        )

    start = address_value(normalized_start)
    end = address_value(caller_end_exclusive)
    instructions = candidate.instructions
    addresses = _cc_cfg._instruction_runtime_addresses(
        instructions,
        source="cod",
        caller_start=start,
    )
    counts: dict[int, int] = {}
    for address in addresses:
        if address is not None:
            counts[address] = counts.get(address, 0) + 1
    by_offset = {
        address - start: instruction
        for instruction, address in zip(instructions, addresses)
        if address is not None and counts.get(address) == 1
    }
    index_by_address = {
        address: index
        for index, address in enumerate(addresses)
        if address is not None and counts.get(address) == 1
    }
    symbol_pattern = re.escape(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
    relative_rows = {
        0x00: (
            b"\x8b\x0d\x84\x47\x00\x00",
            rf"mov\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            rf"{symbol_pattern}\+18308",
        ),
        0x06: (b"\x3b\xcf", r"cmp\s+ecx\s*,\s*edi"),
        0x08: (
            b"\x74\x0c",
            r"je\s+(?:SHORT\s+)?\$L[0-9]+",
        ),
        0x0A: (
            b"\x8b\x01",
            r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[ecx\]",
        ),
        0x0C: (b"\x6a\x01", r"push\s+1"),
        0x0E: (
            b"\xff\x10",
            r"call\s+(?:dword\s+(?:ptr\s+)?)?\[eax\]",
        ),
        0x10: (
            b"\x89\x3d\x84\x47\x00\x00",
            rf"mov\s+(?:dword\s+(?:ptr\s+)?)?"
            rf"{symbol_pattern}\+18308\s*,\s*edi",
        ),
    }
    load_offsets = [
        offset
        for offset in by_offset
        if all(
            (
                offset + relative in by_offset
                and bytes(
                    int(value, 16)
                    for value in by_offset[offset + relative].bytes
                )
                == body
                and re.fullmatch(
                    pattern,
                    by_offset[offset + relative].raw_text.strip(),
                    flags=re.IGNORECASE,
                )
                is not None
                and definition.data[
                    offset + relative :
                    offset + relative + len(body)
                ]
                == body
            )
            for relative, (body, pattern) in relative_rows.items()
        )
    ]
    if len(load_offsets) != 1:
        raise ValueError(
            "HUD timer-panel deleting-destructor bridge requires one unique "
            "bounded global-delete instruction structure"
        )
    load_offset = load_offsets[0]
    relocation_specs = (
        load_offset + 0x02,
        load_offset + 0x12,
    )
    for relocation_offset in relocation_specs:
        relocations = [
            relocation
            for relocation in definition.relocations
            if relocation.offset == relocation_offset
        ]
        if (
            len(relocations) != 1
            or relocations[0].type != IMAGE_REL_I386_DIR32
            or relocations[0].symbol_name
            != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
            or struct.unpack_from(
                "<I",
                definition.data,
                relocation_offset,
            )[0]
            != _cc_catalog.HUD_UI_MGR_TIMER_PANEL_DISPLACEMENT
            or not all(
                definition.relocation_mask[index]
                for index in range(relocation_offset, relocation_offset + 4)
            )
        ):
            raise ValueError(
                "HUD timer-panel deleting-destructor bridge requires exact "
                "aggregate-field DIR32 relocations"
            )
    actual_bounded_relocations = [
        relocation
        for relocation in definition.relocations
        if load_offset <= relocation.offset < load_offset + 0x16
    ]
    if (
        len(actual_bounded_relocations) != 2
        or {
            relocation.offset
            for relocation in actual_bounded_relocations
        }
        != set(relocation_specs)
        or definition.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in definition.defined_external_data
    ):
        raise ValueError(
            "HUD timer-panel deleting-destructor bridge rejects extra, "
            "duplicate, or conflicting global storage references"
        )

    load_index = index_by_address.get(start + load_offset)
    branch_offset = load_offset + 0x08
    vptr_load_offset = load_offset + 0x0A
    call_offset = load_offset + 0x0E
    target_offset = load_offset + 0x16
    branch_index = index_by_address.get(start + branch_offset)
    vptr_index = index_by_address.get(start + vptr_load_offset)
    call_index = index_by_address.get(start + call_offset)
    branch = (
        _cc_cfg._exact_local_direct_branch(
            instructions[branch_index],
            instruction_index=branch_index,
            instruction_addresses=addresses,
            instruction_index_by_address=index_by_address,
            source="cod",
            caller_start=start,
            caller_end=end,
        )
        if branch_index is not None
        else None
    )
    edi_writes_before_load = [
        index
        for index in range(load_index if load_index is not None else 0)
        if _cc_cfg._instruction_may_clobber_register(instructions[index], "edi")
    ]
    zero_seed_index = (
        edi_writes_before_load[-1] if edi_writes_before_load else None
    )
    zero_seed = (
        instructions[zero_seed_index]
        if zero_seed_index is not None
        else None
    )
    if (
        load_index is None
        or vptr_index is None
        or call_index is None
        or start + target_offset not in index_by_address
        or zero_seed is None
        or bytes(int(value, 16) for value in zero_seed.bytes)
        != b"\x33\xff"
        or re.fullmatch(
            r"xor\s+edi\s*,\s*edi",
            zero_seed.raw_text.strip(),
            flags=re.IGNORECASE,
        )
        is None
        or any(
            _cc_cfg._instruction_may_clobber_register(
                instructions[index],
                "edi",
            )
            for index in range(zero_seed_index + 1, call_index + 1)
        )
        or branch is None
        or branch[0] != "conditional"
        or addresses[branch[1]] != start + target_offset
        or any(
            _cc_cfg._instruction_may_clobber_register(
                instructions[index],
                "ecx",
            )
            for index in range(load_index + 1, vptr_index)
        )
        or any(
            _cc_cfg._instruction_may_clobber_register(
                instructions[index],
                register,
            )
            for index in range(vptr_index + 1, call_index)
            for register in ("eax", "ecx")
        )
        or _cc_cfg._cleanup_after(instructions, call_index) is not None
        or any(definition.relocation_mask[call_offset : call_offset + 2])
    ):
        raise ValueError(
            "HUD timer-panel deleting-destructor bridge rejects wrong CFG, "
            "zero seed, direct call, cleanup, receiver/vptr clobber, or "
            "masked call bytes"
        )

    candidate_receiver = f"load({field_provenance})"
    receiver_provenance = f"exact-load(ecx,{candidate_receiver})"
    return (
        {
            normalize_address(load_offset):
                ReviewedAbsoluteStorageLoadBridge(
                    register="ecx",
                    aggregate_symbol=(
                        _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
                    ),
                    displacement=_cc_catalog.HUD_UI_MGR_TIMER_PANEL_DISPLACEMENT,
                    access_width=_cc_catalog.HUD_UI_MGR_TIMER_PANEL_ACCESS_WIDTH,
                    storage_identity=aggregate_identity,
                )
        },
        {
            normalize_address(vptr_load_offset):
                ReviewedMemberVptrStorageBridge(
                    register="eax",
                    source_register="ecx",
                    source_provenance=receiver_provenance,
                    receiver_register="ecx",
                    receiver_provenance=receiver_provenance,
                    storage_identity=expected_storage,
                    slot_displacement=0,
                    call_address=normalize_address(call_offset),
                )
        },
    )


def _hud_ui_mgr_objective_counter_deleting_destructor_candidate_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedAbsoluteStorageLoadBridge],
    dict[str, ReviewedMemberVptrStorageBridge],
]:
    """Prove only the 23rd (zero-based ordinal 22) objective-counter delete."""
    from _recoil.call_contract.records import (
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedMemberVptrStorageBridge,
    )
    normalized_start = normalize_address(caller_start)
    normalized_end = normalize_address(caller_end_exclusive)
    if normalized_start != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_START:
        return {}, {}
    if (
        caller_identity != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_IDENTITY
        or normalized_end != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_END_EXCLUSIVE
    ):
        raise ValueError(
            "HUD objective-counter deleting-destructor bridge requires the "
            "exact reviewed ShutdownResources caller and extent"
        )

    definition = candidate.caller_definition
    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_START
    ]
    if (
        definition is None
        or definition.symbol != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_SYMBOL
        or len(definition.data) != len(definition.relocation_mask)
        or len(definition.data)
        != address_value(_cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_END_EXCLUSIVE)
        - address_value(_cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_START)
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
            "HUD objective-counter deleting-destructor bridge requires the "
            "exact registered HUD target, caller definition, and contribution"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != r"\?ShutdownResources@HudUiMgr@@.*"
        or re.fullmatch(
            r"\?ShutdownResources@HudUiMgr@@.*",
            definition.symbol,
        )
        is None
        or getattr(contribution_row, "name", "")
        != "HudUiMgr::ShutdownResources"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD objective-counter deleting-destructor bridge requires the "
            "exact authored ShutdownResources hud.cpp contribution identity"
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
        field_address != _cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_ADDRESS
        or indexes.storage_by_address.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
        != aggregate_identity
        or aggregate_addresses != [_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS]
        or indexes.storage_by_address.get(field_address, "") != ""
        or len(exact_containers) != 1
        or aggregate_identity in indexes.provider_ids
    ):
        raise ValueError(
            "HUD objective-counter deleting-destructor bridge rejects "
            "missing, aliased, provider, or conflicting typed aggregate "
            "storage provenance"
        )
    field_provenance = _cc_receiver_instructions._abstract_with_displacement(
        aggregate_identity,
        _cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_DISPLACEMENT,
    )
    candidate_receiver = f"load({field_provenance})"
    expected_storage = f"load({candidate_receiver})"
    expected_row = (
        expected[_cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_DELETE_ORDINAL]
        if len(expected) > _cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_DELETE_ORDINAL
        else None
    )
    if (
        not isinstance(expected_row, Mapping)
        or expected_row
        != {
            "ordinal": _cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_DELETE_ORDINAL,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": expected_storage,
            "slot_displacement": 0,
            "cleanup_bytes": None,
        }
        or sum(
            1
            for row in expected
            if (
                row.get("storage_identity") == expected_storage
                and row.get("slot_displacement") == 0
            )
        )
        != 1
    ):
        raise ValueError(
            "HUD objective-counter deleting-destructor bridge requires the "
            "exact retail 23rd invocation (serialized ordinal 22), indirect "
            "slot-zero form, and cleanup contract"
        )

    start = address_value(normalized_start)
    end = address_value(normalized_end)
    instructions = candidate.instructions
    addresses = _cc_cfg._instruction_runtime_addresses(
        instructions,
        source="cod",
        caller_start=start,
    )
    counts: dict[int, int] = {}
    for address in addresses:
        if address is not None:
            counts[address] = counts.get(address, 0) + 1
    index_by_address = {
        address: index
        for index, address in enumerate(addresses)
        if address is not None and counts.get(address) == 1
    }
    load_offset = _cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_DELETE_LOAD_OFFSET
    vptr_offset = _cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_DELETE_VPTR_OFFSET
    call_offset = _cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_DELETE_CALL_OFFSET
    exact_rows = {
        0x122: (
            b"\x8b\x0d\xc8\x0b\x00\x00",
            (
                r"mov\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
                + re.escape(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
                + r"\+3016"
            ),
        ),
        0x128: (b"\x3b\xcf", r"cmp\s+ecx\s*,\s*edi"),
        0x12A: (b"\x74\x0c", r"je\s+(?:SHORT\s+)?\$L[0-9]+"),
        0x12C: (
            b"\x8b\x11",
            r"mov\s+edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[ecx\]",
        ),
        0x12E: (b"\x6a\x01", r"push\s+1"),
        0x130: (
            b"\xff\x12",
            r"call\s+(?:dword\s+(?:ptr\s+)?)?\[edx\]",
        ),
        0x132: (
            b"\x89\x3d\xc8\x0b\x00\x00",
            (
                r"mov\s+(?:dword\s+(?:ptr\s+)?)?"
                + re.escape(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
                + r"\+3016\s*,\s*edi"
            ),
        ),
    }
    for offset, (body, pattern) in exact_rows.items():
        index = index_by_address.get(start + offset)
        if index is None:
            raise ValueError(
                "HUD objective-counter deleting-destructor bridge requires "
                f"one exact instruction at candidate offset 0x{offset:x}"
            )
        instruction = instructions[index]
        if (
            bytes(int(value, 16) for value in instruction.bytes) != body
            or definition.data[offset : offset + len(body)] != body
            or re.fullmatch(
                pattern,
                instruction.raw_text.strip(),
                flags=re.IGNORECASE,
            )
            is None
        ):
            raise ValueError(
                "HUD objective-counter deleting-destructor bridge requires "
                f"exact candidate bytes and operands at offset 0x{offset:x}"
            )

    relocation_specs = (
        load_offset + 0x02,
        load_offset + 0x12,
    )
    for relocation_offset in relocation_specs:
        relocations = [
            relocation
            for relocation in definition.relocations
            if relocation.offset == relocation_offset
        ]
        if (
            len(relocations) != 1
            or relocations[0].type != IMAGE_REL_I386_DIR32
            or relocations[0].symbol_name
            != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
            or struct.unpack_from(
                "<I",
                definition.data,
                relocation_offset,
            )[0]
            != _cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_DISPLACEMENT
            or not all(
                definition.relocation_mask[index]
                for index in range(relocation_offset, relocation_offset + 4)
            )
        ):
            raise ValueError(
                "HUD objective-counter deleting-destructor bridge requires "
                "exact aggregate-field DIR32 relocations and addends"
            )
    bounded_relocations = [
        relocation
        for relocation in definition.relocations
        if load_offset <= relocation.offset < load_offset + 0x16
    ]
    if (
        len(bounded_relocations) != 2
        or {row.offset for row in bounded_relocations}
        != set(relocation_specs)
        or definition.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in definition.defined_external_data
    ):
        raise ValueError(
            "HUD objective-counter deleting-destructor bridge rejects extra, "
            "duplicate, or conflicting aggregate references"
        )

    load_index = index_by_address[start + load_offset]
    branch_index = index_by_address[start + 0x12A]
    vptr_index = index_by_address[start + vptr_offset]
    call_index = index_by_address[start + call_offset]
    branch = _cc_cfg._exact_local_direct_branch(
        instructions[branch_index],
        instruction_index=branch_index,
        instruction_addresses=addresses,
        instruction_index_by_address=index_by_address,
        source="cod",
        caller_start=start,
        caller_end=end,
    )
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=normalized_end,
    )
    ordinal_by_index = {
        instruction_index: ordinal
        for ordinal, instruction_index in enumerate(invocation_indices)
    }
    if (
        branch is None
        or branch[0] != "conditional"
        or addresses[branch[1]] != start + 0x138
        or ordinal_by_index.get(call_index)
        != _cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_DELETE_ORDINAL
        or any(
            _cc_cfg._instruction_may_clobber_register(
                instructions[index],
                "ecx",
            )
            for index in range(load_index + 1, vptr_index)
        )
        or any(
            _cc_cfg._instruction_may_clobber_register(
                instructions[index],
                register,
            )
            for index in range(vptr_index + 1, call_index)
            for register in ("edx", "ecx")
        )
        or _cc_cfg._cleanup_after(instructions, call_index) is not None
        or any(definition.relocation_mask[call_offset : call_offset + 2])
    ):
        raise ValueError(
            "HUD objective-counter deleting-destructor bridge rejects wrong "
            "serialized ordinal, CFG, receiver/vptr clobber, slot, direct "
            "call, or cleanup"
        )

    receiver_provenance = f"exact-load(ecx,{candidate_receiver})"
    return (
        {
            normalize_address(load_offset):
                ReviewedAbsoluteStorageLoadBridge(
                    register="ecx",
                    aggregate_symbol=(
                        _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
                    ),
                    displacement=(
                        _cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_DISPLACEMENT
                    ),
                    access_width=(
                        _cc_catalog.HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_ACCESS_WIDTH
                    ),
                    storage_identity=aggregate_identity,
                )
        },
        {
            normalize_address(vptr_offset):
                ReviewedMemberVptrStorageBridge(
                    register="edx",
                    source_register="ecx",
                    source_provenance=receiver_provenance,
                    receiver_register="ecx",
                    receiver_provenance=receiver_provenance,
                    storage_identity=expected_storage,
                    slot_displacement=0,
                    call_address=normalize_address(call_offset),
                )
        },
    )


def _hud_ui_mgr_remaining_deleting_destructor_candidate_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedAbsoluteStorageLoadBridge],
    dict[str, ReviewedMemberVptrStorageBridge],
]:
    """Prove the exact four-entry post-TimerPanel deleting-call table."""
    from _recoil.call_contract.records import (
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedMemberVptrStorageBridge,
    )
    normalized_start = normalize_address(caller_start)
    normalized_end = normalize_address(caller_end_exclusive)
    if normalized_start != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_START:
        return {}, {}
    if (
        caller_identity != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_IDENTITY
        or normalized_end != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_END_EXCLUSIVE
    ):
        raise ValueError(
            "HUD remaining deleting-destructor table requires the exact "
            "reviewed ShutdownResources caller and extent"
        )

    definition = candidate.caller_definition
    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_START
    ]
    if (
        definition is None
        or definition.symbol != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_SYMBOL
        or len(definition.data) != len(definition.relocation_mask)
        or len(definition.data)
        != address_value(_cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_END_EXCLUSIVE)
        - address_value(_cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_START)
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
            "HUD remaining deleting-destructor table requires the exact "
            "registered HUD target, caller definition, and contribution"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != r"\?ShutdownResources@HudUiMgr@@.*"
        or re.fullmatch(
            r"\?ShutdownResources@HudUiMgr@@.*",
            definition.symbol,
        )
        is None
        or getattr(contribution_row, "name", "")
        != "HudUiMgr::ShutdownResources"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD remaining deleting-destructor table requires the exact "
            "authored ShutdownResources hud.cpp contribution identity"
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
        indexes.storage_by_address.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
        != aggregate_identity
        or [
            address
            for address, identity in indexes.storage_by_address.items()
            if identity == aggregate_identity
        ]
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS]
        or len(exact_containers) != 1
        or aggregate_identity in indexes.provider_ids
    ):
        raise ValueError(
            "HUD remaining deleting-destructor table rejects missing, "
            "aliased, provider, or conflicting typed aggregate provenance"
        )
    if (
        definition.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in definition.defined_external_data
    ):
        raise ValueError(
            "HUD remaining deleting-destructor table requires one exact "
            "undefined aggregate symbol and no competing definition"
        )

    start = address_value(normalized_start)
    end = address_value(normalized_end)
    instructions = candidate.instructions
    addresses = _cc_cfg._instruction_runtime_addresses(
        instructions,
        source="cod",
        caller_start=start,
    )
    counts: dict[int, int] = {}
    for address in addresses:
        if address is not None:
            counts[address] = counts.get(address, 0) + 1
    index_by_address = {
        address: index
        for index, address in enumerate(addresses)
        if address is not None and counts.get(address) == 1
    }
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=normalized_end,
    )
    ordinal_by_index = {
        instruction_index: ordinal
        for ordinal, instruction_index in enumerate(invocation_indices)
    }

    aggregate_pattern = re.escape(
        _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
    )
    absolute_bridges: dict[str, ReviewedAbsoluteStorageLoadBridge] = {}
    member_bridges: dict[str, ReviewedMemberVptrStorageBridge] = {}
    expected_relocation_offsets: set[int] = set()
    table_start = _cc_catalog.HUD_UI_MGR_REMAINING_DELETE_PROVENANCE_TABLE[0][2]
    table_end = (
        _cc_catalog.HUD_UI_MGR_REMAINING_DELETE_PROVENANCE_TABLE[-1][2] + 0x16
    )
    for (
        label,
        ordinal,
        load_offset,
        displacement,
        vptr_register,
        field_address,
        field_identity,
    ) in _cc_catalog.HUD_UI_MGR_REMAINING_DELETE_PROVENANCE_TABLE:
        if normalize_address(aggregate_start + displacement) != field_address:
            raise ValueError(
                f"HUD {label} deleting-destructor table has inconsistent "
                "reviewed aggregate field address"
            )
        indexed_field_identity = indexes.storage_by_address.get(
            field_address,
            "",
        )
        if field_identity:
            field_addresses = [
                address
                for address, identity in indexes.storage_by_address.items()
                if identity == field_identity
            ]
            if (
                indexed_field_identity != field_identity
                or field_addresses != [field_address]
                or field_identity in indexes.provider_ids
            ):
                raise ValueError(
                    f"HUD {label} deleting-destructor table rejects missing, "
                    "aliased, provider, or conflicting field storage"
                )
        elif indexed_field_identity:
            raise ValueError(
                f"HUD {label} deleting-destructor table rejects an unresolved "
                "field alias over the typed aggregate container"
            )
        field_provenance = _cc_receiver_instructions._abstract_with_displacement(
            aggregate_identity,
            displacement,
        )
        candidate_receiver = f"load({field_provenance})"
        expected_storage = (
            f"load({field_identity})"
            if field_identity
            else f"load({candidate_receiver})"
        )
        expected_row = expected[ordinal] if len(expected) > ordinal else None
        if (
            not isinstance(expected_row, Mapping)
            or expected_row
            != {
                "ordinal": ordinal,
                "form": "call",
                "dispatch": "indirect",
                "identity_kind": "virtual-slot",
                "target_identity": "",
                "storage_identity": expected_storage,
                "slot_displacement": 0,
                "cleanup_bytes": None,
            }
            or sum(
                1
                for row in expected
                if (
                    row.get("storage_identity") == expected_storage
                    and row.get("slot_displacement") == 0
                )
            )
            != 1
        ):
            raise ValueError(
                f"HUD {label} deleting-destructor table requires one exact "
                f"retail serialized ordinal {ordinal}, slot-zero form, and "
                "cleanup contract"
            )

        vptr_offset = load_offset + 0x0A
        call_offset = load_offset + 0x0E
        target_offset = load_offset + 0x16
        displacement_bytes = struct.pack("<I", displacement)
        vptr_opcode = b"\x8b\x11" if vptr_register == "edx" else b"\x8b\x01"
        call_opcode = b"\xff\x12" if vptr_register == "edx" else b"\xff\x10"
        relative_rows = {
            0x00: (
                b"\x8b\x0d" + displacement_bytes,
                (
                    r"mov\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
                    + aggregate_pattern
                    + rf"\+{displacement}"
                ),
            ),
            0x06: (b"\x3b\xcf", r"cmp\s+ecx\s*,\s*edi"),
            0x08: (
                b"\x74\x0c",
                r"je\s+(?:SHORT\s+)?\$L[0-9]+",
            ),
            0x0A: (
                vptr_opcode,
                (
                    rf"mov\s+{vptr_register}\s*,\s*"
                    r"(?:dword\s+(?:ptr\s+)?)?\[ecx\]"
                ),
            ),
            0x0C: (b"\x6a\x01", r"push\s+1"),
            0x0E: (
                call_opcode,
                (
                    r"call\s+(?:dword\s+(?:ptr\s+)?)?"
                    rf"\[{vptr_register}\]"
                ),
            ),
            0x10: (
                b"\x89\x3d" + displacement_bytes,
                (
                    r"mov\s+(?:dword\s+(?:ptr\s+)?)?"
                    + aggregate_pattern
                    + rf"\+{displacement}\s*,\s*edi"
                ),
            ),
        }
        for relative, (body, pattern) in relative_rows.items():
            offset = load_offset + relative
            index = index_by_address.get(start + offset)
            if index is None:
                raise ValueError(
                    f"HUD {label} deleting-destructor table requires one "
                    f"exact instruction at candidate offset 0x{offset:x}"
                )
            instruction = instructions[index]
            if (
                bytes(int(value, 16) for value in instruction.bytes) != body
                or definition.data[offset : offset + len(body)] != body
                or re.fullmatch(
                    pattern,
                    instruction.raw_text.strip(),
                    flags=re.IGNORECASE,
                )
                is None
            ):
                raise ValueError(
                    f"HUD {label} deleting-destructor table requires exact "
                    f"candidate bytes and operands at offset 0x{offset:x}"
                )

        relocation_offsets = (
            load_offset + 0x02,
            load_offset + 0x12,
        )
        expected_relocation_offsets.update(relocation_offsets)
        for relocation_offset in relocation_offsets:
            relocations = [
                relocation
                for relocation in definition.relocations
                if relocation.offset == relocation_offset
            ]
            if (
                len(relocations) != 1
                or relocations[0].type != IMAGE_REL_I386_DIR32
                or relocations[0].symbol_name
                != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
                or struct.unpack_from(
                    "<I",
                    definition.data,
                    relocation_offset,
                )[0]
                != displacement
                or not all(
                    definition.relocation_mask[index]
                    for index in range(
                        relocation_offset,
                        relocation_offset + 4,
                    )
                )
            ):
                raise ValueError(
                    f"HUD {label} deleting-destructor table requires exact "
                    "DIR32 load/clear relocations, targets, and addends"
                )

        load_index = index_by_address[start + load_offset]
        branch_index = index_by_address[start + load_offset + 0x08]
        vptr_index = index_by_address[start + vptr_offset]
        call_index = index_by_address[start + call_offset]
        branch = _cc_cfg._exact_local_direct_branch(
            instructions[branch_index],
            instruction_index=branch_index,
            instruction_addresses=addresses,
            instruction_index_by_address=index_by_address,
            source="cod",
            caller_start=start,
            caller_end=end,
        )
        if (
            branch is None
            or branch[0] != "conditional"
            or addresses[branch[1]] != start + target_offset
            or ordinal_by_index.get(call_index) != ordinal
            or any(
                _cc_cfg._instruction_may_clobber_register(
                    instructions[index],
                    "ecx",
                )
                for index in range(load_index + 1, vptr_index)
            )
            or any(
                _cc_cfg._instruction_may_clobber_register(
                    instructions[index],
                    register,
                )
                for index in range(vptr_index + 1, call_index)
                for register in (vptr_register, "ecx")
            )
            or _cc_cfg._cleanup_after(instructions, call_index) is not None
            or any(
                definition.relocation_mask[
                    call_offset : call_offset + 2
                ]
            )
        ):
            raise ValueError(
                f"HUD {label} deleting-destructor table rejects wrong "
                "serialized ordinal, null branch, register flow, slot, "
                "direct call, or cleanup"
            )

        receiver_provenance = f"exact-load(ecx,{candidate_receiver})"
        absolute_bridges[normalize_address(load_offset)] = (
            ReviewedAbsoluteStorageLoadBridge(
                register="ecx",
                aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                displacement=displacement,
                access_width=4,
                storage_identity=aggregate_identity,
            )
        )
        member_bridges[normalize_address(vptr_offset)] = (
            ReviewedMemberVptrStorageBridge(
                register=vptr_register,
                source_register="ecx",
                source_provenance=receiver_provenance,
                receiver_register="ecx",
                receiver_provenance=receiver_provenance,
                storage_identity=expected_storage,
                slot_displacement=0,
                call_address=normalize_address(call_offset),
            )
        )

    bounded_relocations = [
        relocation
        for relocation in definition.relocations
        if table_start <= relocation.offset < table_end
    ]
    if (
        len(absolute_bridges)
        != len(_cc_catalog.HUD_UI_MGR_REMAINING_DELETE_PROVENANCE_TABLE)
        or len(member_bridges)
        != len(_cc_catalog.HUD_UI_MGR_REMAINING_DELETE_PROVENANCE_TABLE)
        or len(bounded_relocations) != len(expected_relocation_offsets)
        or {row.offset for row in bounded_relocations}
        != expected_relocation_offsets
    ):
        raise ValueError(
            "HUD remaining deleting-destructor table rejects omitted, "
            "duplicate, or extra entries and bounded relocations"
        )
    return absolute_bridges, member_bridges


def _prove_hud_ui_string_menu_generated_destructor_bridge(
    candidate_name: str,
    target_identity: str,
    *,
    expected: Sequence[Mapping[str, Any]],
    document: ProgressDocument,
    candidate: CandidateAssembly,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    bridge: BinaryNinjaBridge,
    invocation_row: Mapping[str, Any],
) -> bool:
    """Prove the one reviewed generated HudUiStringMenu ordinary destructor."""
    if candidate_name != _cc_catalog.HUD_UI_STRING_MENU_DESTRUCTOR_CANDIDATE_SYMBOL:
        return False
    if (
        normalize_address(caller_start) != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_START
        or caller_identity != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_END_EXCLUSIVE
    ):
        raise ValueError(
            "HUD string-menu generated destructor bridge requires the exact "
            "reviewed ShutdownResources caller and extent"
        )
    if (
        target_identity != _cc_catalog.HUD_UI_STRING_MENU_DESTRUCTOR_TARGET_IDENTITY
        or invocation_row.get("ordinal")
        != _cc_catalog.HUD_UI_STRING_MENU_DESTRUCTOR_CALL_ORDINAL
        or invocation_row.get("form") != "call"
        or invocation_row.get("dispatch") != "direct"
        or invocation_row.get("cleanup_bytes") is not None
    ):
        raise ValueError(
            "HUD string-menu generated destructor bridge requires the exact "
            "retail target, ordinal, direct-call form, and cleanup"
        )
    if len(expected) <= _cc_catalog.HUD_UI_STRING_MENU_DELETE_CALL_ORDINAL:
        raise ValueError(
            "HUD string-menu generated destructor bridge requires the "
            "following retail global-delete invocation"
        )
    delete_row = expected[_cc_catalog.HUD_UI_STRING_MENU_DELETE_CALL_ORDINAL]
    delete_identity = str(delete_row.get("target_identity", ""))
    if (
        delete_row.get("ordinal") != _cc_catalog.HUD_UI_STRING_MENU_DELETE_CALL_ORDINAL
        or delete_row.get("form") != "call"
        or delete_row.get("dispatch") != "direct"
        or delete_row.get("identity_kind") != "provider"
        or delete_identity not in indexes.provider_ids
        or delete_row.get("storage_identity") != ""
        or delete_row.get("slot_displacement") is not None
        or delete_row.get("cleanup_bytes") != 4
    ):
        raise ValueError(
            "HUD string-menu generated destructor bridge requires the exact "
            "following retail global-delete call and four-byte caller cleanup"
        )

    target = candidate.target
    caller = candidate.caller_definition
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_START
    ]
    if (
        caller is None
        or caller.symbol != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_SYMBOL
        or len(caller.data)
        != address_value(_cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_END_EXCLUSIVE)
        - address_value(_cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_START)
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
        or len(contribution_rows) != 1
    ):
        raise ValueError(
            "HUD string-menu generated destructor bridge requires the exact "
            "registered HUD target, caller definition, and contribution"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != r"\?ShutdownResources@HudUiMgr@@.*"
        or re.fullmatch(
            r"\?ShutdownResources@HudUiMgr@@.*",
            caller.symbol,
        )
        is None
        or getattr(contribution_row, "name", "")
        != "HudUiMgr::ShutdownResources"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD string-menu generated destructor bridge requires the exact "
            "authored ShutdownResources hud.cpp contribution identity"
        )

    target_addresses = sorted(
        address
        for address, identity in indexes.by_address.items()
        if identity == target_identity
    )
    target_symbol_id = target_identity.removeprefix("symbol:")
    target_symbol = document.collection("symbols").get(target_symbol_id)
    if (
        target_addresses != [_cc_catalog.HUD_UI_STRING_MENU_DESTRUCTOR_TARGET_ADDRESS]
        or not isinstance(target_symbol, Mapping)
        or _cc_identity._symbol_identity(target_symbol_id, target_symbol) != target_identity
        or target_symbol.get("binary") != "recoil"
        or target_symbol.get("kind") != "function"
        or target_symbol.get("pipeline_class") != "authored-lifecycle"
        or target_symbol.get("authored_order_role")
        != "compiler-generated-implicit-cleanup"
        or target_symbol.get("extent_state") != "known"
        or normalize_address(str(target_symbol.get("address", "")))
        != _cc_catalog.HUD_UI_STRING_MENU_DESTRUCTOR_TARGET_ADDRESS
        or normalize_address(str(target_symbol.get("end_exclusive", "")))
        != _cc_catalog.HUD_UI_STRING_MENU_DESTRUCTOR_TARGET_END_EXCLUSIVE
        or target_symbol.get("size")
        != address_value(_cc_catalog.HUD_UI_STRING_MENU_DESTRUCTOR_TARGET_END_EXCLUSIVE)
        - address_value(_cc_catalog.HUD_UI_STRING_MENU_DESTRUCTOR_TARGET_ADDRESS)
        or target_symbol.get("logical_identity_key") not in {None, ""}
        or target_symbol.get("icf_fold_status") not in {None, ""}
        or bool(target_symbol.get("logical_aliases"))
        or bool(target_symbol.get("icf_address_group"))
    ):
        raise ValueError(
            "HUD string-menu generated destructor bridge requires one exact "
            "unaliased reviewed retail lifecycle target and extent"
        )

    source_function_symbols = {
        *caller.undefined_external_functions,
        *caller.defined_external_functions,
        *candidate.complete_destructor_definitions.keys(),
    }
    if (
        _cc_catalog.HUD_UI_STRING_MENU_SUPERSEDED_DESTRUCTOR_CORE_SYMBOL
        in source_function_symbols
        or any(
            _cc_catalog.HUD_UI_STRING_MENU_SUPERSEDED_DESTRUCTOR_CORE_SYMBOL
            in relocation.symbol_name
            for definition in candidate.complete_destructor_definitions.values()
            for relocation in definition.relocations
        )
        or any(
            _cc_catalog.HUD_UI_STRING_MENU_SUPERSEDED_DESTRUCTOR_CORE_SYMBOL
            in _cc_cfg._instruction_operand(instruction)
            for instruction in candidate.instructions
        )
    ):
        raise ValueError(
            "HUD string-menu generated destructor bridge rejects the "
            "superseded authored DestructorCore source identity"
        )
    if (
        caller.undefined_external_functions.count(candidate_name) != 0
        or caller.defined_external_functions.count(candidate_name) != 1
        or caller.undefined_external_functions.count(
            _cc_catalog.HUD_UI_STRING_MENU_GLOBAL_DELETE_SYMBOL
        )
        != 1
        or caller.defined_external_functions.count(
            _cc_catalog.HUD_UI_STRING_MENU_GLOBAL_DELETE_SYMBOL
        )
        != 0
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in caller.defined_external_data
    ):
        raise ValueError(
            "HUD string-menu generated destructor bridge requires one "
            "same-object destructor, one external global delete, and one "
            "external HUD aggregate"
        )

    expected_caller_sequence = bytes.fromhex(
        "8b 35 e0 0e 00 00 3b f7 74 16 8b ce "
        "e8 00 00 00 00 56 e8 00 00 00 00 83 c4 04 "
        "89 3d e0 0e 00 00"
    )
    if (
        caller.data[
            _cc_catalog.HUD_UI_STRING_MENU_CALLER_SEQUENCE_START :
            _cc_catalog.HUD_UI_STRING_MENU_CALLER_SEQUENCE_END_EXCLUSIVE
        ]
        != expected_caller_sequence
    ):
        raise ValueError(
            "HUD string-menu generated destructor bridge requires the exact "
            "bounded load/null-test/destruct/delete/cleanup/clear caller body"
        )
    caller_relocation_specs = (
        (
            _cc_catalog.HUD_UI_STRING_MENU_CALLER_SEQUENCE_START + 2,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_STRING_MENU_AGGREGATE_DISPLACEMENT,
        ),
        (
            _cc_catalog.HUD_UI_STRING_MENU_CALLER_DESTRUCTOR_RELOCATION_OFFSET,
            IMAGE_REL_I386_REL32,
            candidate_name,
            0,
        ),
        (
            _cc_catalog.HUD_UI_STRING_MENU_CALLER_DELETE_RELOCATION_OFFSET,
            IMAGE_REL_I386_REL32,
            _cc_catalog.HUD_UI_STRING_MENU_GLOBAL_DELETE_SYMBOL,
            0,
        ),
        (
            _cc_catalog.HUD_UI_STRING_MENU_CALLER_SEQUENCE_END_EXCLUSIVE - 4,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_STRING_MENU_AGGREGATE_DISPLACEMENT,
        ),
    )
    bounded_caller_relocations = tuple(
        relocation
        for relocation in caller.relocations
        if (
            _cc_catalog.HUD_UI_STRING_MENU_CALLER_SEQUENCE_START
            <= relocation.offset
            < _cc_catalog.HUD_UI_STRING_MENU_CALLER_SEQUENCE_END_EXCLUSIVE
        )
    )
    if len(bounded_caller_relocations) != len(caller_relocation_specs):
        raise ValueError(
            "HUD string-menu generated destructor bridge rejects missing, "
            "duplicate, or extra bounded caller relocations"
        )
    expected_caller_mask: set[int] = set()
    for relocation, (
        offset,
        relocation_type,
        symbol_name,
        addend,
    ) in zip(bounded_caller_relocations, caller_relocation_specs):
        if (
            relocation.offset != offset
            or relocation.type != relocation_type
            or relocation.symbol_name != symbol_name
            or struct.unpack_from("<I", caller.data, offset)[0] != addend
        ):
            raise ValueError(
                "HUD string-menu generated destructor bridge rejects wrong "
                "caller relocation offset, type, target, or addend"
            )
        expected_caller_mask.update(range(offset, offset + 4))
    actual_caller_mask = {
        index
        for index in range(
            _cc_catalog.HUD_UI_STRING_MENU_CALLER_SEQUENCE_START,
            _cc_catalog.HUD_UI_STRING_MENU_CALLER_SEQUENCE_END_EXCLUSIVE,
        )
        if caller.relocation_mask[index]
    }
    if actual_caller_mask != expected_caller_mask:
        raise ValueError(
            "HUD string-menu generated destructor bridge requires an exact "
            "bounded caller relocation mask"
        )

    instruction_offsets = [
        (
            address_value(address)
            if (address := _cc_cfg._source_instruction_address(instruction))
            else None
        )
        for instruction in candidate.instructions
    ]
    offset_counts: dict[int, int] = {}
    for offset in instruction_offsets:
        if offset is not None:
            offset_counts[offset] = offset_counts.get(offset, 0) + 1
    index_by_offset = {
        offset: index
        for index, offset in enumerate(instruction_offsets)
        if offset is not None and offset_counts.get(offset) == 1
    }
    destructor_index = index_by_offset.get(
        _cc_catalog.HUD_UI_STRING_MENU_CALLER_DESTRUCTOR_RELOCATION_OFFSET - 1
    )
    delete_index = index_by_offset.get(
        _cc_catalog.HUD_UI_STRING_MENU_CALLER_DELETE_RELOCATION_OFFSET - 1
    )
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    ordinal_by_index = {
        index: ordinal for ordinal, index in enumerate(invocation_indices)
    }
    if (
        destructor_index is None
        or delete_index is None
        or ordinal_by_index.get(destructor_index)
        != _cc_catalog.HUD_UI_STRING_MENU_DESTRUCTOR_CALL_ORDINAL
        or ordinal_by_index.get(delete_index)
        != _cc_catalog.HUD_UI_STRING_MENU_DELETE_CALL_ORDINAL
        or destructor_index in candidate.local_control_flow_indices
        or delete_index in candidate.local_control_flow_indices
        or _cc_cfg._instruction_mnemonic(candidate.instructions[destructor_index])
        != "call"
        or _cc_cfg._instruction_operand(
            candidate.instructions[destructor_index]
        ).strip()
        != candidate_name
        or _cc_cfg._cleanup_after(candidate.instructions, destructor_index) is not None
        or _cc_cfg._instruction_mnemonic(candidate.instructions[delete_index])
        != "call"
        or _cc_cfg._instruction_operand(candidate.instructions[delete_index]).strip()
        != _cc_catalog.HUD_UI_STRING_MENU_GLOBAL_DELETE_SYMBOL
        or _cc_cfg._cleanup_after(candidate.instructions, delete_index) != 4
    ):
        raise ValueError(
            "HUD string-menu generated destructor bridge rejects wrong caller "
            "ordinal, call form, target, local-flow status, or cleanup"
        )

    delete_name_rows = bridge_names.get(
        _cc_catalog.HUD_UI_STRING_MENU_GLOBAL_DELETE_SYMBOL,
        (),
    )
    delete_name_rows = (
        list(delete_name_rows)
        if isinstance(
            delete_name_rows,
            (list, tuple, set, frozenset),
        )
        else [delete_name_rows]
    )
    if (
        len(delete_name_rows) != 1
        or normalize_address(
            str(getattr(delete_name_rows[0], "address", ""))
        )
        != _cc_catalog.HUD_UI_STRING_MENU_GLOBAL_DELETE_ADDRESS
        or _cc_catalog.HUD_UI_STRING_MENU_GLOBAL_DELETE_SYMBOL
        not in {
            str(getattr(delete_name_rows[0], "name", "")),
            str(getattr(delete_name_rows[0], "raw_name", "")),
            str(getattr(delete_name_rows[0], "full_name", "")),
        }
        or indexes.by_address.get(_cc_catalog.HUD_UI_STRING_MENU_GLOBAL_DELETE_ADDRESS)
        != delete_identity
    ):
        raise ValueError(
            "HUD string-menu generated destructor bridge requires one exact "
            "retail global operator-delete provider identity"
        )

    destructor = candidate.complete_destructor_definitions.get(candidate_name)
    target_size = (
        address_value(_cc_catalog.HUD_UI_STRING_MENU_DESTRUCTOR_TARGET_END_EXCLUSIVE)
        - address_value(_cc_catalog.HUD_UI_STRING_MENU_DESTRUCTOR_TARGET_ADDRESS)
    )
    if (
        destructor is None
        or not destructor.section_is_comdat
        or destructor.comdat_selection != 2
        or destructor.section_external_functions != (candidate_name,)
        or destructor.section_size != target_size
        or len(destructor.data) != target_size
        or len(destructor.relocation_mask) != target_size
    ):
        raise ValueError(
            "HUD string-menu generated destructor bridge requires one exact "
            "full-section VC5 COMDAT SELECT_ANY ordinary-destructor definition"
        )
    relocation_specs: tuple[
        tuple[int, int, str | re.Pattern[str]], ...
    ] = (
        (0x02, IMAGE_REL_I386_DIR32, "__except_list"),
        (0x09, IMAGE_REL_I386_DIR32, re.compile(r"\$L[0-9]+")),
        (0x11, IMAGE_REL_I386_DIR32, "__except_list"),
        (
            0x19,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_STRING_MENU_PANEL_DESTRUCTOR_SYMBOL,
        ),
        (0x31, IMAGE_REL_I386_REL32, _cc_catalog.MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL),
        (
            0x40,
            IMAGE_REL_I386_REL32,
            _cc_catalog.HUD_UI_STRING_MENU_CONTAINER_DESTRUCTOR_SYMBOL,
        ),
        (0x4C, IMAGE_REL_I386_DIR32, "__except_list"),
    )
    if len(destructor.relocations) != len(relocation_specs):
        raise ValueError(
            "HUD string-menu generated destructor bridge rejects missing, "
            "duplicate, or extra definition relocations"
        )
    expected_definition_mask: set[int] = set()
    for relocation, (offset, relocation_type, symbol_spec) in zip(
        destructor.relocations,
        relocation_specs,
    ):
        symbol_matches = (
            symbol_spec.fullmatch(relocation.symbol_name) is not None
            if isinstance(symbol_spec, re.Pattern)
            else relocation.symbol_name == symbol_spec
        )
        if (
            relocation.offset != offset
            or relocation.type != relocation_type
            or not symbol_matches
            or struct.unpack_from("<I", destructor.data, offset)[0] != 0
        ):
            raise ValueError(
                "HUD string-menu generated destructor bridge rejects wrong "
                "definition relocation offset, type, target, or addend"
            )
        expected_definition_mask.update(range(offset, offset + 4))
    actual_definition_mask = {
        index
        for index, masked in enumerate(destructor.relocation_mask)
        if masked
    }
    if actual_definition_mask != expected_definition_mask:
        raise ValueError(
            "HUD string-menu generated destructor bridge requires an exact "
            "definition relocation mask"
        )

    if (
        indexes.by_address.get(_cc_catalog.HUD_UI_STRING_MENU_PANEL_DESTRUCTOR_ADDRESS)
        != _cc_catalog.HUD_UI_STRING_MENU_PANEL_DESTRUCTOR_IDENTITY
        or indexes.by_address.get(
            _cc_catalog.HUD_UI_STRING_MENU_CONTAINER_DESTRUCTOR_ADDRESS
        )
        != _cc_catalog.HUD_UI_STRING_MENU_CONTAINER_DESTRUCTOR_IDENTITY
    ):
        raise ValueError(
            "HUD string-menu generated destructor bridge requires the exact "
            "retail panel callback and container cleanup identities"
        )
    retail_bytes = _cc_cfg._hexdump_bytes(
        bridge.hexdump(
            _cc_catalog.HUD_UI_STRING_MENU_DESTRUCTOR_TARGET_ADDRESS,
            target_size,
        )
    )
    if (
        len(retail_bytes) != target_size
        or struct.unpack_from("<I", retail_bytes, 0x19)[0]
        != address_value(_cc_catalog.HUD_UI_STRING_MENU_PANEL_DESTRUCTOR_ADDRESS)
        or any(
            candidate_byte != retail_byte
            for index, (candidate_byte, retail_byte) in enumerate(
                zip(destructor.data, retail_bytes)
            )
            if not destructor.relocation_mask[index]
        )
    ):
        raise ValueError(
            "HUD string-menu generated destructor bridge requires exact "
            "relocation-normalized retail body bytes without a vptr store"
        )

    retail_contract = _cc_extraction.extract_invocation_contract(
        _cc_listing.parse_assembly(
            bridge.assembly(_cc_catalog.HUD_UI_STRING_MENU_DESTRUCTOR_TARGET_ADDRESS),
            source="bn",
        ),
        source="bn",
        caller_identity=target_identity,
        caller_start=_cc_catalog.HUD_UI_STRING_MENU_DESTRUCTOR_TARGET_ADDRESS,
        caller_end_exclusive=(
            _cc_catalog.HUD_UI_STRING_MENU_DESTRUCTOR_TARGET_END_EXCLUSIVE
        ),
        indexes=_cc_identity._without_caller_scoped_retail_proofs(indexes),
        bridge_names=bridge_names,
    )
    if (
        len(retail_contract) != 2
        or retail_contract[0].get("ordinal") != 0
        or retail_contract[0].get("form") != "call"
        or retail_contract[0].get("dispatch") != "direct"
        or retail_contract[0].get("identity_kind") != "provider"
        or retail_contract[0].get("target_identity")
        not in indexes.provider_ids
        or retail_contract[0].get("cleanup_bytes") is not None
        or retail_contract[1]
        != {
            "ordinal": 1,
            "form": "call",
            "dispatch": "direct",
            "identity_kind": "direct",
            "target_identity": (
                _cc_catalog.HUD_UI_STRING_MENU_CONTAINER_DESTRUCTOR_IDENTITY
            ),
            "storage_identity": "",
            "slot_displacement": None,
            "cleanup_bytes": None,
        }
    ):
        raise ValueError(
            "HUD string-menu generated destructor bridge requires the exact "
            "retail EH-array and container cleanup call topology"
        )
    candidate_contract = _cc_extraction.extract_invocation_contract(
        destructor.instructions,
        source="cod",
        caller_identity=target_identity,
        caller_start=_cc_catalog.HUD_UI_STRING_MENU_DESTRUCTOR_TARGET_ADDRESS,
        caller_end_exclusive=(
            _cc_catalog.HUD_UI_STRING_MENU_DESTRUCTOR_TARGET_END_EXCLUSIVE
        ),
        indexes=indexes,
        bridge_names=bridge_names,
        compiler_generated_bridges={
            _cc_catalog.MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL: str(
                retail_contract[0]["target_identity"]
            ),
            _cc_catalog.HUD_UI_STRING_MENU_CONTAINER_DESTRUCTOR_SYMBOL: (
                _cc_catalog.HUD_UI_STRING_MENU_CONTAINER_DESTRUCTOR_IDENTITY
            ),
        },
        local_control_flow_indices=destructor.local_control_flow_indices,
        local_control_flow_targets=destructor.local_control_flow_targets,
    )
    if (
        len(candidate_contract) != 2
        or _cc_comparison.compare_call_contracts(retail_contract, candidate_contract)[
            "passed"
        ]
        is not True
    ):
        raise ValueError(
            "HUD string-menu generated destructor bridge requires an exact "
            "relocation-normalized candidate/retail call topology"
        )
    return True


def _prove_hud_ui_shield_message_generated_destructor_bridge(
    candidate_name: str,
    target_identity: str,
    *,
    expected: Sequence[Mapping[str, Any]],
    document: ProgressDocument,
    candidate: CandidateAssembly,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    bridge: BinaryNinjaBridge,
    invocation_row: Mapping[str, Any],
) -> bool:
    """Prove one reviewed VC5 HudUiShieldMessageWidget ordinary destructor."""
    if candidate_name != _cc_catalog.HUD_UI_SHIELD_MESSAGE_DESTRUCTOR_CANDIDATE_SYMBOL:
        return False
    if (
        normalize_address(caller_start) != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_START
        or caller_identity != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_END_EXCLUSIVE
    ):
        raise ValueError(
            "HUD shield-message generated destructor bridge requires the "
            "exact reviewed ShutdownResources caller and extent"
        )
    if (
        target_identity != _cc_catalog.HUD_UI_SHIELD_MESSAGE_DESTRUCTOR_TARGET_IDENTITY
        or invocation_row.get("ordinal")
        != _cc_catalog.HUD_UI_SHIELD_MESSAGE_DESTRUCTOR_CALL_ORDINAL
        or invocation_row.get("form") != "call"
        or invocation_row.get("dispatch") != "direct"
        or invocation_row.get("cleanup_bytes") is not None
    ):
        raise ValueError(
            "HUD shield-message generated destructor bridge requires the "
            "exact retail target, ordinal, direct-call form, and cleanup"
        )
    if len(expected) <= _cc_catalog.HUD_UI_SHIELD_MESSAGE_DELETE_CALL_ORDINAL:
        raise ValueError(
            "HUD shield-message generated destructor bridge requires the "
            "following retail global-delete invocation"
        )
    delete_row = expected[_cc_catalog.HUD_UI_SHIELD_MESSAGE_DELETE_CALL_ORDINAL]
    delete_identity = str(delete_row.get("target_identity", ""))
    if (
        delete_row.get("ordinal")
        != _cc_catalog.HUD_UI_SHIELD_MESSAGE_DELETE_CALL_ORDINAL
        or delete_row.get("form") != "call"
        or delete_row.get("dispatch") != "direct"
        or delete_row.get("identity_kind") != "provider"
        or delete_identity not in indexes.provider_ids
        or delete_row.get("storage_identity") != ""
        or delete_row.get("slot_displacement") is not None
        or delete_row.get("cleanup_bytes") != 4
    ):
        raise ValueError(
            "HUD shield-message generated destructor bridge requires the "
            "exact following retail global-delete call and caller cleanup"
        )

    target = candidate.target
    caller = candidate.caller_definition
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_START
    ]
    if (
        caller is None
        or caller.symbol != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_SYMBOL
        or len(caller.data)
        != address_value(_cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_END_EXCLUSIVE)
        - address_value(_cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_START)
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
        or len(contribution_rows) != 1
    ):
        raise ValueError(
            "HUD shield-message generated destructor bridge requires the "
            "exact registered HUD target, caller definition, and contribution"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != r"\?ShutdownResources@HudUiMgr@@.*"
        or re.fullmatch(
            r"\?ShutdownResources@HudUiMgr@@.*",
            caller.symbol,
        )
        is None
        or getattr(contribution_row, "name", "")
        != "HudUiMgr::ShutdownResources"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD shield-message generated destructor bridge requires the "
            "exact authored ShutdownResources hud.cpp contribution identity"
        )

    target_addresses = sorted(
        address
        for address, identity in indexes.by_address.items()
        if identity == target_identity
    )
    target_symbol_id = target_identity.removeprefix("symbol:")
    target_symbol = document.collection("symbols").get(target_symbol_id)
    target_size = (
        address_value(_cc_catalog.HUD_UI_SHIELD_MESSAGE_DESTRUCTOR_TARGET_END_EXCLUSIVE)
        - address_value(_cc_catalog.HUD_UI_SHIELD_MESSAGE_DESTRUCTOR_TARGET_ADDRESS)
    )
    if (
        target_addresses != [_cc_catalog.HUD_UI_SHIELD_MESSAGE_DESTRUCTOR_TARGET_ADDRESS]
        or not isinstance(target_symbol, Mapping)
        or _cc_identity._symbol_identity(target_symbol_id, target_symbol) != target_identity
        or target_symbol.get("binary") != "recoil"
        or target_symbol.get("kind") != "function"
        or target_symbol.get("pipeline_class") != "authored-lifecycle"
        or target_symbol.get("authored_order_role")
        != "compiler-generated-implicit-cleanup"
        or target_symbol.get("extent_state") != "known"
        or normalize_address(str(target_symbol.get("address", "")))
        != _cc_catalog.HUD_UI_SHIELD_MESSAGE_DESTRUCTOR_TARGET_ADDRESS
        or normalize_address(str(target_symbol.get("end_exclusive", "")))
        != _cc_catalog.HUD_UI_SHIELD_MESSAGE_DESTRUCTOR_TARGET_END_EXCLUSIVE
        or target_symbol.get("size") != target_size
        or target_symbol.get("logical_identity_key") not in {None, ""}
        or target_symbol.get("icf_fold_status") not in {None, ""}
        or bool(target_symbol.get("logical_aliases"))
        or bool(target_symbol.get("icf_address_group"))
    ):
        raise ValueError(
            "HUD shield-message generated destructor bridge requires one "
            "exact unaliased reviewed retail lifecycle target and extent"
        )

    superseded_names = {
        _cc_catalog.HUD_UI_SHIELD_MESSAGE_SUPERSEDED_DESTRUCTOR_SYMBOL,
        _cc_catalog.HUD_UI_SHIELD_MESSAGE_SUPERSEDED_DESTRUCTOR_CORE_SYMBOL,
    }
    source_function_symbols = {
        *caller.undefined_external_functions,
        *caller.defined_external_functions,
        *candidate.complete_destructor_definitions.keys(),
    }
    if (
        source_function_symbols & superseded_names
        or any(
            any(name in relocation.symbol_name for name in superseded_names)
            for definition in candidate.complete_destructor_definitions.values()
            for relocation in definition.relocations
        )
        or any(
            any(
                name in relocation.symbol_name
                for name in superseded_names
            )
            for definition in candidate.complete_destructor_definitions.values()
            for section in definition.associated_sections
            for relocation in section.relocations
        )
        or any(
            any(name in _cc_cfg._instruction_operand(instruction) for name in superseded_names)
            for instruction in candidate.instructions
        )
    ):
        raise ValueError(
            "HUD shield-message generated destructor bridge rejects the "
            "superseded authored Destructor/DestructorCore identities"
        )
    if (
        caller.undefined_external_functions.count(candidate_name) != 0
        or caller.defined_external_functions.count(candidate_name) != 1
        or caller.undefined_external_functions.count(
            _cc_catalog.HUD_UI_STRING_MENU_GLOBAL_DELETE_SYMBOL
        )
        != 1
        or caller.defined_external_functions.count(
            _cc_catalog.HUD_UI_STRING_MENU_GLOBAL_DELETE_SYMBOL
        )
        != 0
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in caller.defined_external_data
    ):
        raise ValueError(
            "HUD shield-message generated destructor bridge requires one "
            "same-object destructor, one global delete, and one HUD aggregate"
        )

    expected_caller_sequence = bytes.fromhex(
        "8b 35 dc 0e 00 00 3b f7 74 16 8b ce "
        "e8 00 00 00 00 56 e8 00 00 00 00 83 c4 04 "
        "89 3d dc 0e 00 00"
    )
    if (
        caller.data[
            _cc_catalog.HUD_UI_SHIELD_MESSAGE_CALLER_SEQUENCE_START :
            _cc_catalog.HUD_UI_SHIELD_MESSAGE_CALLER_SEQUENCE_END_EXCLUSIVE
        ]
        != expected_caller_sequence
    ):
        raise ValueError(
            "HUD shield-message generated destructor bridge requires the "
            "exact bounded load/destruct/delete/cleanup/clear caller body"
        )
    caller_relocation_specs = (
        (
            _cc_catalog.HUD_UI_SHIELD_MESSAGE_CALLER_SEQUENCE_START + 2,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_SHIELD_MESSAGE_AGGREGATE_DISPLACEMENT,
        ),
        (
            _cc_catalog.HUD_UI_SHIELD_MESSAGE_CALLER_DESTRUCTOR_RELOCATION_OFFSET,
            IMAGE_REL_I386_REL32,
            candidate_name,
            0,
        ),
        (
            _cc_catalog.HUD_UI_SHIELD_MESSAGE_CALLER_DELETE_RELOCATION_OFFSET,
            IMAGE_REL_I386_REL32,
            _cc_catalog.HUD_UI_STRING_MENU_GLOBAL_DELETE_SYMBOL,
            0,
        ),
        (
            _cc_catalog.HUD_UI_SHIELD_MESSAGE_CALLER_SEQUENCE_END_EXCLUSIVE - 4,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_SHIELD_MESSAGE_AGGREGATE_DISPLACEMENT,
        ),
    )
    bounded_caller_relocations = tuple(
        relocation
        for relocation in caller.relocations
        if (
            _cc_catalog.HUD_UI_SHIELD_MESSAGE_CALLER_SEQUENCE_START
            <= relocation.offset
            < _cc_catalog.HUD_UI_SHIELD_MESSAGE_CALLER_SEQUENCE_END_EXCLUSIVE
        )
    )
    if len(bounded_caller_relocations) != len(caller_relocation_specs):
        raise ValueError(
            "HUD shield-message generated destructor bridge rejects missing, "
            "duplicate, or extra bounded caller relocations"
        )
    expected_caller_mask: set[int] = set()
    for relocation, (
        offset,
        relocation_type,
        symbol_name,
        addend,
    ) in zip(bounded_caller_relocations, caller_relocation_specs):
        if (
            relocation.offset != offset
            or relocation.type != relocation_type
            or relocation.symbol_name != symbol_name
            or struct.unpack_from("<I", caller.data, offset)[0] != addend
        ):
            raise ValueError(
                "HUD shield-message generated destructor bridge rejects wrong "
                "caller relocation offset, type, target, or addend"
            )
        expected_caller_mask.update(range(offset, offset + 4))
    actual_caller_mask = {
        index
        for index in range(
            _cc_catalog.HUD_UI_SHIELD_MESSAGE_CALLER_SEQUENCE_START,
            _cc_catalog.HUD_UI_SHIELD_MESSAGE_CALLER_SEQUENCE_END_EXCLUSIVE,
        )
        if caller.relocation_mask[index]
    }
    if actual_caller_mask != expected_caller_mask:
        raise ValueError(
            "HUD shield-message generated destructor bridge requires an exact "
            "bounded caller relocation mask"
        )

    instruction_offsets = [
        (
            address_value(address)
            if (address := _cc_cfg._source_instruction_address(instruction))
            else None
        )
        for instruction in candidate.instructions
    ]
    offset_counts: dict[int, int] = {}
    for offset in instruction_offsets:
        if offset is not None:
            offset_counts[offset] = offset_counts.get(offset, 0) + 1
    index_by_offset = {
        offset: index
        for index, offset in enumerate(instruction_offsets)
        if offset is not None and offset_counts.get(offset) == 1
    }
    destructor_index = index_by_offset.get(
        _cc_catalog.HUD_UI_SHIELD_MESSAGE_CALLER_DESTRUCTOR_RELOCATION_OFFSET - 1
    )
    delete_index = index_by_offset.get(
        _cc_catalog.HUD_UI_SHIELD_MESSAGE_CALLER_DELETE_RELOCATION_OFFSET - 1
    )
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    ordinal_by_index = {
        index: ordinal for ordinal, index in enumerate(invocation_indices)
    }
    if (
        destructor_index is None
        or delete_index is None
        or ordinal_by_index.get(destructor_index)
        != _cc_catalog.HUD_UI_SHIELD_MESSAGE_DESTRUCTOR_CALL_ORDINAL
        or ordinal_by_index.get(delete_index)
        != _cc_catalog.HUD_UI_SHIELD_MESSAGE_DELETE_CALL_ORDINAL
        or destructor_index in candidate.local_control_flow_indices
        or delete_index in candidate.local_control_flow_indices
        or _cc_cfg._instruction_mnemonic(candidate.instructions[destructor_index])
        != "call"
        or _cc_cfg._instruction_operand(
            candidate.instructions[destructor_index]
        ).strip()
        != candidate_name
        or _cc_cfg._cleanup_after(candidate.instructions, destructor_index) is not None
        or _cc_cfg._instruction_mnemonic(candidate.instructions[delete_index])
        != "call"
        or _cc_cfg._instruction_operand(candidate.instructions[delete_index]).strip()
        != _cc_catalog.HUD_UI_STRING_MENU_GLOBAL_DELETE_SYMBOL
        or _cc_cfg._cleanup_after(candidate.instructions, delete_index) != 4
    ):
        raise ValueError(
            "HUD shield-message generated destructor bridge rejects wrong "
            "caller ordinal, form, target, local-flow status, or cleanup"
        )

    delete_name_rows = bridge_names.get(
        _cc_catalog.HUD_UI_STRING_MENU_GLOBAL_DELETE_SYMBOL,
        (),
    )
    delete_name_rows = (
        list(delete_name_rows)
        if isinstance(delete_name_rows, (list, tuple, set, frozenset))
        else [delete_name_rows]
    )
    if (
        len(delete_name_rows) != 1
        or normalize_address(
            str(getattr(delete_name_rows[0], "address", ""))
        )
        != _cc_catalog.HUD_UI_STRING_MENU_GLOBAL_DELETE_ADDRESS
        or _cc_catalog.HUD_UI_STRING_MENU_GLOBAL_DELETE_SYMBOL
        not in {
            str(getattr(delete_name_rows[0], "name", "")),
            str(getattr(delete_name_rows[0], "raw_name", "")),
            str(getattr(delete_name_rows[0], "full_name", "")),
        }
        or indexes.by_address.get(_cc_catalog.HUD_UI_STRING_MENU_GLOBAL_DELETE_ADDRESS)
        != delete_identity
    ):
        raise ValueError(
            "HUD shield-message generated destructor bridge requires one "
            "exact retail global operator-delete provider identity"
        )

    destructor = candidate.complete_destructor_definitions.get(candidate_name)
    if (
        destructor is None
        or not destructor.section_is_comdat
        or destructor.comdat_selection != 2
        or destructor.section_external_functions != (candidate_name,)
        or destructor.section_size != target_size
        or len(destructor.data) != target_size
        or len(destructor.relocation_mask) != target_size
    ):
        raise ValueError(
            "HUD shield-message generated destructor bridge requires one "
            "exact full-section VC5 COMDAT SELECT_ANY destructor definition"
        )
    expected_definition_body = bytes.fromhex(
        "64 a1 00 00 00 00 6a ff 68 00 00 00 00 50 64 89 "
        "25 00 00 00 00 56 8b f1 8d 8e d8 00 00 00 c7 44 "
        "24 0c 00 00 00 00 c7 86 7c 03 00 00 00 00 00 00 "
        "e8 00 00 00 00 8d 4e 1c c7 44 24 0c ff ff ff ff "
        "e8 00 00 00 00 8b 4c 24 04 5e 64 89 0d 00 00 00 "
        "00 83 c4 0c c3 90 90 90 90 90 90 90 90 90 90 90"
    )
    if (
        destructor.data != expected_definition_body
        or destructor.data[_cc_catalog.HUD_UI_SHIELD_MESSAGE_DESTRUCTOR_CODE_SIZE - 1]
        != 0xC3
        or set(
            destructor.data[
                _cc_catalog.HUD_UI_SHIELD_MESSAGE_DESTRUCTOR_CODE_SIZE :
            ]
        )
        != {0x90}
    ):
        raise ValueError(
            "HUD shield-message generated destructor bridge requires the "
            "exact 0x55 code definition and VC5 section padding"
        )
    relocation_specs: tuple[
        tuple[int, int, str | re.Pattern[str]], ...
    ] = (
        (0x02, IMAGE_REL_I386_DIR32, "__except_list"),
        (0x09, IMAGE_REL_I386_DIR32, re.compile(r"\$L[0-9]+")),
        (0x11, IMAGE_REL_I386_DIR32, "__except_list"),
        (
            0x2C,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_SHIELD_MESSAGE_VFTABLE_SYMBOL,
        ),
        (
            0x31,
            IMAGE_REL_I386_REL32,
            _cc_catalog.HUD_UI_SHIELD_MESSAGE_PANEL_DESTRUCTOR_SYMBOL,
        ),
        (
            0x41,
            IMAGE_REL_I386_REL32,
            _cc_catalog.HUD_UI_SHIELD_MESSAGE_WIDGET_DESTRUCTOR_SYMBOL,
        ),
        (0x4D, IMAGE_REL_I386_DIR32, "__except_list"),
    )
    if len(destructor.relocations) != len(relocation_specs):
        raise ValueError(
            "HUD shield-message generated destructor bridge rejects missing, "
            "duplicate, or extra definition relocations"
        )
    expected_definition_mask: set[int] = set()
    for relocation, (offset, relocation_type, symbol_spec) in zip(
        destructor.relocations,
        relocation_specs,
    ):
        symbol_matches = (
            symbol_spec.fullmatch(relocation.symbol_name) is not None
            if isinstance(symbol_spec, re.Pattern)
            else relocation.symbol_name == symbol_spec
        )
        if (
            relocation.offset != offset
            or relocation.type != relocation_type
            or not symbol_matches
            or struct.unpack_from("<I", destructor.data, offset)[0] != 0
        ):
            raise ValueError(
                "HUD shield-message generated destructor bridge rejects wrong "
                "definition relocation offset, type, target, or addend"
            )
        expected_definition_mask.update(range(offset, offset + 4))
    if {
        index
        for index, masked in enumerate(destructor.relocation_mask)
        if masked
    } != expected_definition_mask:
        raise ValueError(
            "HUD shield-message generated destructor bridge requires an exact "
            "definition relocation mask"
        )

    if (
        indexes.by_address.get(_cc_catalog.HUD_UI_SHIELD_MESSAGE_PANEL_DESTRUCTOR_ADDRESS)
        != _cc_catalog.HUD_UI_SHIELD_MESSAGE_PANEL_DESTRUCTOR_IDENTITY
        or indexes.by_address.get(
            _cc_catalog.HUD_UI_SHIELD_MESSAGE_WIDGET_DESTRUCTOR_ADDRESS
        )
        != _cc_catalog.HUD_UI_SHIELD_MESSAGE_WIDGET_DESTRUCTOR_IDENTITY
        or indexes.by_address.get(_cc_catalog.HUD_UI_SHIELD_MESSAGE_FRAME_HANDLER_ADDRESS)
        not in indexes.provider_ids
    ):
        raise ValueError(
            "HUD shield-message generated destructor bridge requires exact "
            "panel, widget, and frame-handler retail identities"
        )
    retail_bytes = _cc_cfg._hexdump_bytes(
        bridge.hexdump(
            _cc_catalog.HUD_UI_SHIELD_MESSAGE_DESTRUCTOR_TARGET_ADDRESS,
            target_size,
        )
    )
    if (
        len(retail_bytes) != target_size
        or struct.unpack_from("<I", retail_bytes, 0x09)[0]
        != address_value(_cc_catalog.HUD_UI_SHIELD_MESSAGE_FUNCLET_ADDRESS) + 0x0B
        or struct.unpack_from("<I", retail_bytes, 0x2C)[0]
        != address_value(_cc_catalog.HUD_UI_SHIELD_MESSAGE_VFTABLE_ADDRESS)
        or any(
            candidate_byte != retail_byte
            for index, (candidate_byte, retail_byte) in enumerate(
                zip(destructor.data, retail_bytes)
            )
            if not destructor.relocation_mask[index]
        )
    ):
        raise ValueError(
            "HUD shield-message generated destructor bridge requires exact "
            "relocation-normalized retail body, handler, and vptr reset"
        )

    runtime_associations = tuple(
        section
        for section in destructor.associated_sections
        if section.name.startswith(".text")
        or section.name.startswith(".xdata")
    )
    if (
        len(runtime_associations) != 2
        or sorted(section.name for section in runtime_associations)
        != [".text$x", ".xdata$x"]
        or any(
            not section.section_is_comdat
            or section.comdat_selection != 5
            or section.association_section_index <= 0
            or section.section_external_symbols
            for section in runtime_associations
        )
    ):
        raise ValueError(
            "HUD shield-message generated destructor bridge requires exactly "
            "one associative runtime funclet and one associative xdata section"
        )
    funclet = next(
        section for section in runtime_associations if section.name == ".text$x"
    )
    xdata = next(
        section for section in runtime_associations if section.name == ".xdata$x"
    )
    if (
        funclet.association_section_index != xdata.association_section_index
        or funclet.section_index == xdata.section_index
        or funclet.section_size != _cc_catalog.HUD_UI_SHIELD_MESSAGE_FUNCLET_SIZE
        or len(funclet.data) != _cc_catalog.HUD_UI_SHIELD_MESSAGE_FUNCLET_SIZE
        or xdata.section_size != _cc_catalog.HUD_UI_SHIELD_MESSAGE_XDATA_SIZE
        or len(xdata.data) != _cc_catalog.HUD_UI_SHIELD_MESSAGE_XDATA_SIZE
    ):
        raise ValueError(
            "HUD shield-message generated destructor bridge rejects wrong "
            "associative section identity, association, or extent"
        )
    expected_funclet_body = bytes.fromhex(
        "8b 4d e8 83 c1 1c e9 00 00 00 00 b8 00 00 00 00 "
        "e9 00 00 00 00"
    )
    expected_xdata_body = bytes.fromhex(
        "20 05 93 19 01 00 00 00 00 00 00 00 "
        "00 00 00 00 00 00 00 00 00 00 00 00 "
        "00 00 00 00 00 00 00 00 ff ff ff ff 00 00 00 00"
    )
    if funclet.data != expected_funclet_body or xdata.data != expected_xdata_body:
        raise ValueError(
            "HUD shield-message generated destructor bridge requires exact "
            "state0/state-1 funclet and xdata topology bytes"
        )
    funclet_relocation_specs = (
        (
            0x07,
            IMAGE_REL_I386_REL32,
            _cc_catalog.HUD_UI_SHIELD_MESSAGE_WIDGET_DESTRUCTOR_SYMBOL,
        ),
        (0x0C, IMAGE_REL_I386_DIR32, re.compile(r"\$T[0-9]+")),
        (
            0x11,
            IMAGE_REL_I386_REL32,
            _cc_catalog.HUD_UI_SHIELD_MESSAGE_FRAME_HANDLER_SYMBOL,
        ),
    )
    xdata_relocation_specs = (
        (0x08, IMAGE_REL_I386_DIR32, re.compile(r"\$T[0-9]+")),
        (0x24, IMAGE_REL_I386_DIR32, re.compile(r"\$L[0-9]+")),
    )

    def require_exact_relocations(
        section: CandidateAssociatedSection,
        specs: Sequence[tuple[int, int, str | re.Pattern[str]]],
    ) -> None:
        if len(section.relocations) != len(specs):
            raise ValueError(
                "HUD shield-message generated destructor bridge rejects "
                f"missing, duplicate, or extra {section.name} relocations"
            )
        expected_mask: set[int] = set()
        for relocation, (offset, relocation_type, symbol_spec) in zip(
            section.relocations,
            specs,
        ):
            symbol_matches = (
                symbol_spec.fullmatch(relocation.symbol_name) is not None
                if isinstance(symbol_spec, re.Pattern)
                else relocation.symbol_name == symbol_spec
            )
            if (
                relocation.offset != offset
                or relocation.type != relocation_type
                or not symbol_matches
                or struct.unpack_from("<I", section.data, offset)[0] != 0
            ):
                raise ValueError(
                    "HUD shield-message generated destructor bridge rejects "
                    f"wrong {section.name} relocation target/type/addend"
                )
            expected_mask.update(range(offset, offset + 4))
        if {
            index
            for index, masked in enumerate(section.relocation_mask)
            if masked
        } != expected_mask:
            raise ValueError(
                "HUD shield-message generated destructor bridge requires an "
                f"exact {section.name} relocation mask"
            )

    require_exact_relocations(funclet, funclet_relocation_specs)
    require_exact_relocations(xdata, xdata_relocation_specs)
    funclet_locals = {
        symbol.value: symbol.name
        for symbol in funclet.symbols
        if symbol.storage_class == 6
        and symbol.type == 0
        and re.fullmatch(r"\$L[0-9]+", symbol.name)
    }
    xdata_locals = {
        symbol.value: symbol.name
        for symbol in xdata.symbols
        if symbol.storage_class == 3
        and symbol.type == 0
        and re.fullmatch(r"\$T[0-9]+", symbol.name)
    }
    if (
        set(funclet_locals) != {0, 0x0B}
        or set(xdata_locals) != {0, 0x20}
        or destructor.relocations[1].symbol_name != funclet_locals[0x0B]
        or funclet.relocations[1].symbol_name != xdata_locals[0]
        or xdata.relocations[0].symbol_name != xdata_locals[0x20]
        or xdata.relocations[1].symbol_name != funclet_locals[0]
    ):
        raise ValueError(
            "HUD shield-message generated destructor bridge requires exact "
            "main/handler/state-table/funclet local-symbol linkage"
        )

    retail_funclet = _cc_cfg._hexdump_bytes(
        bridge.hexdump(
            _cc_catalog.HUD_UI_SHIELD_MESSAGE_FUNCLET_ADDRESS,
            _cc_catalog.HUD_UI_SHIELD_MESSAGE_FUNCLET_SIZE,
        )
    )
    retail_xdata = _cc_cfg._hexdump_bytes(
        bridge.hexdump(
            _cc_catalog.HUD_UI_SHIELD_MESSAGE_XDATA_ADDRESS,
            _cc_catalog.HUD_UI_SHIELD_MESSAGE_XDATA_SIZE,
        )
    )
    if (
        len(retail_funclet) != _cc_catalog.HUD_UI_SHIELD_MESSAGE_FUNCLET_SIZE
        or len(retail_xdata) != _cc_catalog.HUD_UI_SHIELD_MESSAGE_XDATA_SIZE
        or struct.unpack_from("<I", retail_funclet, 0x0C)[0]
        != address_value(_cc_catalog.HUD_UI_SHIELD_MESSAGE_XDATA_ADDRESS)
        or struct.unpack_from("<I", retail_xdata, 0x08)[0]
        != address_value(_cc_catalog.HUD_UI_SHIELD_MESSAGE_XDATA_ADDRESS) + 0x20
        or struct.unpack_from("<I", retail_xdata, 0x24)[0]
        != address_value(_cc_catalog.HUD_UI_SHIELD_MESSAGE_FUNCLET_ADDRESS)
        or any(
            candidate_byte != retail_byte
            for index, (candidate_byte, retail_byte) in enumerate(
                zip(funclet.data, retail_funclet)
            )
            if not funclet.relocation_mask[index]
        )
        or any(
            candidate_byte != retail_byte
            for index, (candidate_byte, retail_byte) in enumerate(
                zip(xdata.data, retail_xdata)
            )
            if not xdata.relocation_mask[index]
        )
    ):
        raise ValueError(
            "HUD shield-message generated destructor bridge requires exact "
            "relocation-normalized retail funclet and xdata state topology"
        )
    funclet_widget_target = (
        address_value(_cc_catalog.HUD_UI_SHIELD_MESSAGE_FUNCLET_ADDRESS)
        + 0x0B
        + struct.unpack_from("<i", retail_funclet, 0x07)[0]
    )
    funclet_handler_target = (
        address_value(_cc_catalog.HUD_UI_SHIELD_MESSAGE_FUNCLET_ADDRESS)
        + 0x15
        + struct.unpack_from("<i", retail_funclet, 0x11)[0]
    )
    if (
        funclet_widget_target
        != address_value(_cc_catalog.HUD_UI_SHIELD_MESSAGE_WIDGET_DESTRUCTOR_ADDRESS)
        or funclet_handler_target
        != address_value(_cc_catalog.HUD_UI_SHIELD_MESSAGE_FRAME_HANDLER_ADDRESS)
    ):
        raise ValueError(
            "HUD shield-message generated destructor bridge requires the "
            "exact this+0x1c unwind tail target and frame handler"
        )

    retail_contract = _cc_extraction.extract_invocation_contract(
        _cc_listing.parse_assembly(
            bridge.assembly(
                _cc_catalog.HUD_UI_SHIELD_MESSAGE_DESTRUCTOR_TARGET_ADDRESS
            ),
            source="bn",
        ),
        source="bn",
        caller_identity=target_identity,
        caller_start=_cc_catalog.HUD_UI_SHIELD_MESSAGE_DESTRUCTOR_TARGET_ADDRESS,
        caller_end_exclusive=(
            _cc_catalog.HUD_UI_SHIELD_MESSAGE_DESTRUCTOR_TARGET_END_EXCLUSIVE
        ),
        indexes=_cc_identity._without_caller_scoped_retail_proofs(indexes),
        bridge_names=bridge_names,
    )
    expected_retail_contract = [
        {
            "ordinal": 0,
            "form": "call",
            "dispatch": "direct",
            "identity_kind": "direct",
            "target_identity": _cc_catalog.HUD_UI_SHIELD_MESSAGE_PANEL_DESTRUCTOR_IDENTITY,
            "storage_identity": "",
            "slot_displacement": None,
            "cleanup_bytes": None,
        },
        {
            "ordinal": 1,
            "form": "call",
            "dispatch": "direct",
            "identity_kind": "direct",
            "target_identity": _cc_catalog.HUD_UI_SHIELD_MESSAGE_WIDGET_DESTRUCTOR_IDENTITY,
            "storage_identity": "",
            "slot_displacement": None,
            "cleanup_bytes": None,
        },
    ]
    if retail_contract != expected_retail_contract:
        raise ValueError(
            "HUD shield-message generated destructor bridge requires exact "
            "panel-reset then widget-cleanup retail call topology"
        )
    candidate_contract = _cc_extraction.extract_invocation_contract(
        destructor.instructions,
        source="cod",
        caller_identity=target_identity,
        caller_start=_cc_catalog.HUD_UI_SHIELD_MESSAGE_DESTRUCTOR_TARGET_ADDRESS,
        caller_end_exclusive=(
            _cc_catalog.HUD_UI_SHIELD_MESSAGE_DESTRUCTOR_TARGET_END_EXCLUSIVE
        ),
        indexes=indexes,
        bridge_names=bridge_names,
        compiler_generated_bridges={
            _cc_catalog.HUD_UI_SHIELD_MESSAGE_PANEL_DESTRUCTOR_SYMBOL: (
                _cc_catalog.HUD_UI_SHIELD_MESSAGE_PANEL_DESTRUCTOR_IDENTITY
            ),
            _cc_catalog.HUD_UI_SHIELD_MESSAGE_WIDGET_DESTRUCTOR_SYMBOL: (
                _cc_catalog.HUD_UI_SHIELD_MESSAGE_WIDGET_DESTRUCTOR_IDENTITY
            ),
        },
        local_control_flow_indices=destructor.local_control_flow_indices,
        local_control_flow_targets=destructor.local_control_flow_targets,
    )
    if (
        len(candidate_contract) != 2
        or _cc_comparison.compare_call_contracts(retail_contract, candidate_contract)[
            "passed"
        ]
        is not True
    ):
        raise ValueError(
            "HUD shield-message generated destructor bridge requires exact "
            "relocation-normalized candidate/retail call topology"
        )
    return True


def _prove_hud_ui_message_stack_generated_destructor_bridge(
    candidate_name: str,
    target_identity: str,
    *,
    expected: Sequence[Mapping[str, Any]],
    document: ProgressDocument,
    candidate: CandidateAssembly,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    bridge: BinaryNinjaBridge,
    invocation_row: Mapping[str, Any],
) -> bool:
    """Prove only the two reviewed implicit HUD message-stack destructors."""
    spec = _cc_catalog.HUD_UI_MESSAGE_STACK_DESTRUCTOR_BRIDGES.get(candidate_name)
    if spec is None:
        return False
    label = str(spec["label"])
    prefix = f"HUD {label} message-stack generated destructor bridge"
    target_address = str(spec["target_address"])
    target_end = str(spec["target_end_exclusive"])
    call_ordinal = int(spec["call_ordinal"])
    delete_ordinal = int(spec["delete_ordinal"])
    sequence_start = int(spec["caller_sequence_start"])
    sequence_end = sequence_start + 0x20
    destructor_relocation_offset = int(
        spec["caller_destructor_relocation_offset"]
    )
    delete_relocation_offset = int(spec["caller_delete_relocation_offset"])
    global_symbol = str(spec["global_symbol"])
    global_symbol_id = str(spec["global_symbol_id"])
    global_storage_id = str(spec["global_storage_id"])
    global_address = str(spec["global_address"])
    anchor_id = str(spec["anchor_id"])
    complete_type = str(spec["complete_type"])
    forbidden_core = str(spec["forbidden_core"])

    if (
        normalize_address(caller_start) != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_START
        or caller_identity != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_END_EXCLUSIVE
    ):
        raise ValueError(
            f"{prefix} requires the exact reviewed ShutdownResources caller "
            "and extent"
        )
    if (
        target_identity != spec["target_identity"]
        or invocation_row.get("ordinal") != call_ordinal
        or invocation_row.get("form") != "call"
        or invocation_row.get("dispatch") != "direct"
        or invocation_row.get("cleanup_bytes") is not None
    ):
        raise ValueError(
            f"{prefix} requires the exact retail target, ordinal, direct-call "
            "form, and cleanup"
        )
    if len(expected) <= delete_ordinal:
        raise ValueError(f"{prefix} requires the adjacent global-delete call")
    delete_row = expected[delete_ordinal]
    delete_identity = str(delete_row.get("target_identity", ""))
    if (
        delete_row.get("ordinal") != delete_ordinal
        or delete_row.get("form") != "call"
        or delete_row.get("dispatch") != "direct"
        or delete_row.get("identity_kind") != "provider"
        or delete_identity not in indexes.provider_ids
        or delete_row.get("storage_identity") != ""
        or delete_row.get("slot_displacement") is not None
        or delete_row.get("cleanup_bytes") != 4
    ):
        raise ValueError(
            f"{prefix} requires the exact adjacent operator-delete call and "
            "four-byte caller cleanup"
        )

    target = candidate.target
    caller = candidate.caller_definition
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_START
    ]
    if (
        caller is None
        or caller.symbol != _cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_SYMBOL
        or len(caller.data)
        != address_value(_cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_END_EXCLUSIVE)
        - address_value(_cc_catalog.HUD_UI_MGR_SHUTDOWN_CALLER_START)
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
        or len(contribution_rows) != 1
    ):
        raise ValueError(
            f"{prefix} requires one exact current HUD compile authority, "
            "caller definition, and contribution"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != r"\?ShutdownResources@HudUiMgr@@.*"
        or re.fullmatch(
            r"\?ShutdownResources@HudUiMgr@@.*",
            caller.symbol,
        )
        is None
        or getattr(contribution_row, "name", "")
        != "HudUiMgr::ShutdownResources"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            f"{prefix} requires the exact authored ShutdownResources hud.cpp "
            "contribution identity"
        )

    target_symbol_id = target_identity.removeprefix("symbol:")
    target_symbol = document.collection("symbols").get(target_symbol_id)
    target_size = address_value(target_end) - address_value(target_address)
    target_addresses = sorted(
        address
        for address, identity in indexes.by_address.items()
        if identity == target_identity
    )
    if (
        target_addresses != [target_address]
        or not isinstance(target_symbol, Mapping)
        or _cc_identity._symbol_identity(target_symbol_id, target_symbol) != target_identity
        or target_symbol.get("binary") != "recoil"
        or target_symbol.get("kind") != "function"
        or target_symbol.get("pipeline_class") != "authored-lifecycle"
        or target_symbol.get("authored_order_role")
        != "compiler-generated-implicit-cleanup"
        or target_symbol.get("extent_state") != "known"
        or normalize_address(str(target_symbol.get("address", "")))
        != target_address
        or normalize_address(str(target_symbol.get("end_exclusive", "")))
        != target_end
        or target_symbol.get("size") != target_size
        or target_symbol.get("logical_identity_key") not in {None, ""}
        or target_symbol.get("icf_fold_status") not in {None, ""}
        or bool(target_symbol.get("logical_aliases"))
        or bool(target_symbol.get("icf_address_group"))
    ):
        raise ValueError(
            f"{prefix} requires one exact unaliased reviewed lifecycle target "
            "and extent"
        )

    trace = target_symbol.get("source_traceability")
    source_edges = trace.get("source_edges") if isinstance(trace, Mapping) else None
    if (
        not isinstance(trace, Mapping)
        or trace.get("state") != "resolved"
        or trace.get("reason_code") not in {None, ""}
        or not isinstance(source_edges, list)
        or len(source_edges) != 1
        or not isinstance(source_edges[0], Mapping)
        or source_edges[0].get("relation") != "emits"
        or source_edges[0].get("anchor_id") != anchor_id
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
    ):
        raise ValueError(
            f"{prefix} requires one exact resolved emits source-trace edge"
        )
    trace_document = parse_source_trace_path(
        REPO_ROOT / _cc_catalog.HUD_UI_MESSAGE_STACK_SOURCE_PATH,
        repo_root=REPO_ROOT,
    )
    trace_anchors = [
        row for row in trace_document.anchors if row.anchor_id == anchor_id
    ]
    trace_artifacts = [
        row
        for row in trace_document.artifacts
        if row.artifact_id == target_symbol_id
    ]
    if (
        len(trace_anchors) != 1
        or len(trace_artifacts) != 1
        or trace_anchors[0].construct is None
        or trace_anchors[0].construct.kind != "type"
        or trace_anchors[0].construct.name != complete_type
        or trace_anchors[0].comment_style != "doxygen"
        or trace_anchors[0].attachment_status != "attached"
        or trace_artifacts[0].anchor_id != anchor_id
        or trace_artifacts[0].relation != "emits"
        or trace_artifacts[0].section != ".text"
        or not trace_artifacts[0].direct
        or trace_artifacts[0].construct is None
        or trace_artifacts[0].construct.kind != "type"
        or trace_artifacts[0].construct.name != complete_type
        or any(
            finding.anchor_id == anchor_id
            or finding.artifact_id == target_symbol_id
            for finding in trace_document.findings
        )
    ):
        raise ValueError(
            f"{prefix} requires one matching attached complete-type anchor"
        )

    global_row = document.collection("symbols").get(global_symbol_id)
    storage_row = document.collection("storage_contributions").get(
        global_storage_id
    )
    if (
        not isinstance(global_row, Mapping)
        or global_row.get("binary") != "recoil"
        or global_row.get("kind") != "data"
        or normalize_address(str(global_row.get("address", "")))
        != global_address
        or global_row.get("navigation_name")
        != spec["global_navigation_name"]
        or global_row.get("output_section_id") != "recoil:section:.data"
        or global_row.get("storage_contribution_ids") != [global_storage_id]
        or not isinstance(storage_row, Mapping)
        or storage_row.get("binary") != "recoil"
        or storage_row.get("kind") != "data-symbol"
        or storage_row.get("output_section_id") != "recoil:section:.data"
        or storage_row.get("overlap") != "none"
        or storage_row.get("symbol_ids") != [global_symbol_id]
        or not isinstance(storage_row.get("reference"), Mapping)
        or normalize_address(
            str(storage_row["reference"].get("address", ""))
        )
        != global_address
        or indexes.storage_by_address.get(global_address)
        != f"storage:{global_symbol_id}"
    ):
        raise ValueError(
            f"{prefix} requires the exact typed global storage identity"
        )

    all_function_symbols = {
        *caller.undefined_external_functions,
        *caller.defined_external_functions,
        *candidate.complete_destructor_definitions.keys(),
    }
    if (
        forbidden_core in all_function_symbols
        or any(
            forbidden_core in relocation.symbol_name
            for definition in candidate.complete_destructor_definitions.values()
            for relocation in definition.relocations
        )
        or any(
            forbidden_core in relocation.symbol_name
            for definition in candidate.complete_destructor_definitions.values()
            for section in definition.associated_sections
            for relocation in section.relocations
        )
        or any(
            forbidden_core in _cc_cfg._instruction_operand(instruction)
            for instruction in candidate.instructions
        )
    ):
        raise ValueError(
            f"{prefix} rejects the fake authored DestructorCore identity"
        )
    if (
        caller.undefined_external_functions.count(candidate_name) != 0
        or caller.defined_external_functions.count(candidate_name) != 1
        or caller.undefined_external_functions.count(
            _cc_catalog.HUD_UI_STRING_MENU_GLOBAL_DELETE_SYMBOL
        )
        != 1
        or caller.defined_external_functions.count(
            _cc_catalog.HUD_UI_STRING_MENU_GLOBAL_DELETE_SYMBOL
        )
        != 0
        or caller.undefined_external_data.count(global_symbol) != 1
        or global_symbol in caller.defined_external_data
    ):
        raise ValueError(
            f"{prefix} requires one same-object destructor, one external "
            "operator delete, and one external typed global"
        )

    expected_caller_sequence = bytes.fromhex(
        "8b 35 00 00 00 00 3b f7 74 16 8b ce "
        "e8 00 00 00 00 56 e8 00 00 00 00 83 c4 04 "
        "89 3d 00 00 00 00"
    )
    if caller.data[sequence_start:sequence_end] != expected_caller_sequence:
        raise ValueError(
            f"{prefix} requires the exact bounded global "
            "load/test/destruct/delete/cleanup/clear body"
        )
    caller_relocation_specs = (
        (sequence_start + 2, IMAGE_REL_I386_DIR32, global_symbol),
        (
            destructor_relocation_offset,
            IMAGE_REL_I386_REL32,
            candidate_name,
        ),
        (
            delete_relocation_offset,
            IMAGE_REL_I386_REL32,
            _cc_catalog.HUD_UI_STRING_MENU_GLOBAL_DELETE_SYMBOL,
        ),
        (sequence_end - 4, IMAGE_REL_I386_DIR32, global_symbol),
    )
    bounded_relocations = tuple(
        row
        for row in caller.relocations
        if sequence_start <= row.offset < sequence_end
    )
    expected_caller_mask: set[int] = set()
    if len(bounded_relocations) != len(caller_relocation_specs):
        raise ValueError(
            f"{prefix} rejects missing, duplicate, or extra caller relocations"
        )
    for relocation, (offset, relocation_type, symbol_name) in zip(
        bounded_relocations,
        caller_relocation_specs,
    ):
        if (
            relocation.offset != offset
            or relocation.type != relocation_type
            or relocation.symbol_name != symbol_name
            or struct.unpack_from("<I", caller.data, offset)[0] != 0
        ):
            raise ValueError(
                f"{prefix} rejects wrong caller relocation offset, type, "
                "target, or addend"
            )
        expected_caller_mask.update(range(offset, offset + 4))
    if {
        index
        for index in range(sequence_start, sequence_end)
        if caller.relocation_mask[index]
    } != expected_caller_mask:
        raise ValueError(f"{prefix} requires an exact caller relocation mask")

    instruction_offsets = [
        (
            address_value(address)
            if (address := _cc_cfg._source_instruction_address(instruction))
            else None
        )
        for instruction in candidate.instructions
    ]
    offset_counts: dict[int, int] = {}
    for offset in instruction_offsets:
        if offset is not None:
            offset_counts[offset] = offset_counts.get(offset, 0) + 1
    index_by_offset = {
        offset: index
        for index, offset in enumerate(instruction_offsets)
        if offset is not None and offset_counts.get(offset) == 1
    }
    destructor_index = index_by_offset.get(destructor_relocation_offset - 1)
    delete_index = index_by_offset.get(delete_relocation_offset - 1)
    ordinal_by_index = {
        index: ordinal
        for ordinal, index in enumerate(
            _cc_callable_identity._candidate_static_invocation_indices(
                candidate,
                caller_start=caller_start,
                caller_end_exclusive=caller_end_exclusive,
            )
        )
    }
    if (
        destructor_index is None
        or delete_index is None
        or ordinal_by_index.get(destructor_index) != call_ordinal
        or ordinal_by_index.get(delete_index) != delete_ordinal
        or destructor_index in candidate.local_control_flow_indices
        or delete_index in candidate.local_control_flow_indices
        or _cc_cfg._instruction_mnemonic(candidate.instructions[destructor_index])
        != "call"
        or _cc_cfg._instruction_operand(
            candidate.instructions[destructor_index]
        ).strip()
        != candidate_name
        or _cc_cfg._cleanup_after(candidate.instructions, destructor_index) is not None
        or _cc_cfg._instruction_mnemonic(candidate.instructions[delete_index])
        != "call"
        or _cc_cfg._instruction_operand(candidate.instructions[delete_index]).strip()
        != _cc_catalog.HUD_UI_STRING_MENU_GLOBAL_DELETE_SYMBOL
        or _cc_cfg._cleanup_after(candidate.instructions, delete_index) != 4
    ):
        raise ValueError(
            f"{prefix} rejects wrong caller ordinal, opcode offset, call "
            "target, local-flow status, or cleanup"
        )

    delete_name_rows = bridge_names.get(
        _cc_catalog.HUD_UI_STRING_MENU_GLOBAL_DELETE_SYMBOL,
        (),
    )
    delete_name_rows = (
        list(delete_name_rows)
        if isinstance(delete_name_rows, (list, tuple, set, frozenset))
        else [delete_name_rows]
    )
    if (
        len(delete_name_rows) != 1
        or normalize_address(
            str(getattr(delete_name_rows[0], "address", ""))
        )
        != _cc_catalog.HUD_UI_STRING_MENU_GLOBAL_DELETE_ADDRESS
        or indexes.by_address.get(_cc_catalog.HUD_UI_STRING_MENU_GLOBAL_DELETE_ADDRESS)
        != delete_identity
    ):
        raise ValueError(
            f"{prefix} requires one exact retail operator-delete provider"
        )

    destructor = candidate.complete_destructor_definitions.get(candidate_name)
    if (
        destructor is None
        or not destructor.section_is_comdat
        or destructor.comdat_selection != 2
        or destructor.section_external_functions != (candidate_name,)
        or destructor.section_size != target_size
        or len(destructor.data) != target_size
        or len(destructor.relocation_mask) != target_size
    ):
        raise ValueError(
            f"{prefix} requires one exact 0x60 VC5 SELECT_ANY COMDAT body"
        )
    relocation_specs: tuple[
        tuple[int, int, str | re.Pattern[str]], ...
    ] = (
        (0x02, IMAGE_REL_I386_DIR32, "__except_list"),
        (0x09, IMAGE_REL_I386_DIR32, re.compile(r"\$L[0-9]+")),
        (0x11, IMAGE_REL_I386_DIR32, "__except_list"),
        (
            0x19,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MESSAGE_STACK_PANEL_DESTRUCTOR_SYMBOL,
        ),
        (0x31, IMAGE_REL_I386_REL32, _cc_catalog.MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL),
        (
            0x40,
            IMAGE_REL_I386_REL32,
            _cc_catalog.HUD_UI_MESSAGE_STACK_CONTAINER_DESTRUCTOR_SYMBOL,
        ),
        (0x4C, IMAGE_REL_I386_DIR32, "__except_list"),
    )
    if len(destructor.relocations) != len(relocation_specs):
        raise ValueError(
            f"{prefix} rejects missing, duplicate, or extra body relocations"
        )
    expected_definition_mask: set[int] = set()
    for relocation, (offset, relocation_type, symbol_spec) in zip(
        destructor.relocations,
        relocation_specs,
    ):
        symbol_matches = (
            symbol_spec.fullmatch(relocation.symbol_name) is not None
            if isinstance(symbol_spec, re.Pattern)
            else relocation.symbol_name == symbol_spec
        )
        if (
            relocation.offset != offset
            or relocation.type != relocation_type
            or not symbol_matches
            or struct.unpack_from("<I", destructor.data, offset)[0] != 0
        ):
            raise ValueError(
                f"{prefix} rejects wrong body relocation offset, type, "
                "target, or addend"
            )
        expected_definition_mask.update(range(offset, offset + 4))
    if {
        index
        for index, masked in enumerate(destructor.relocation_mask)
        if masked
    } != expected_definition_mask:
        raise ValueError(f"{prefix} requires an exact body relocation mask")
    if (
        destructor.data[0x1D:0x1F]
        != bytes((0x6A, _cc_catalog.HUD_UI_MESSAGE_STACK_ELEMENT_COUNT))
        or destructor.data[0x1F:0x22]
        != bytes((0x8D, 0x46, _cc_catalog.HUD_UI_MESSAGE_STACK_ELEMENT_OFFSET))
        or destructor.data[0x22] != 0x68
        or struct.unpack_from("<I", destructor.data, 0x23)[0]
        != _cc_catalog.HUD_UI_MESSAGE_STACK_ELEMENT_STRIDE
    ):
        raise ValueError(
            f"{prefix} requires four exact HudUiPanel elements at this+0x10 "
            "with stride 0x2a4"
        )
    if (
        indexes.by_address.get(
            _cc_catalog.HUD_UI_MESSAGE_STACK_PANEL_DESTRUCTOR_ADDRESS
        )
        != _cc_catalog.HUD_UI_MESSAGE_STACK_PANEL_DESTRUCTOR_IDENTITY
        or indexes.by_address.get(
            _cc_catalog.HUD_UI_MESSAGE_STACK_CONTAINER_DESTRUCTOR_ADDRESS
        )
        != _cc_catalog.HUD_UI_MESSAGE_STACK_CONTAINER_DESTRUCTOR_IDENTITY
        or indexes.by_address.get(_cc_catalog.HUD_UI_MESSAGE_STACK_FRAME_HANDLER_ADDRESS)
        not in indexes.provider_ids
    ):
        raise ValueError(
            f"{prefix} requires the exact panel, container, and frame-handler "
            "identities"
        )

    retail_body = _cc_cfg._hexdump_bytes(bridge.hexdump(target_address, target_size))
    if (
        len(retail_body) != target_size
        or struct.unpack_from("<I", retail_body, 0x19)[0]
        != address_value(_cc_catalog.HUD_UI_MESSAGE_STACK_PANEL_DESTRUCTOR_ADDRESS)
        or any(
            candidate_byte != retail_byte
            for index, (candidate_byte, retail_byte) in enumerate(
                zip(destructor.data, retail_body)
            )
            if not destructor.relocation_mask[index]
        )
    ):
        raise ValueError(
            f"{prefix} requires exact relocation-normalized retail body bytes"
        )

    runtime_associations = tuple(
        section
        for section in destructor.associated_sections
        if section.name.startswith(".text")
        or section.name.startswith(".xdata")
    )
    if (
        len(runtime_associations) != 2
        or sorted(section.name for section in runtime_associations)
        != [".text$x", ".xdata$x"]
        or any(
            not section.section_is_comdat
            or section.comdat_selection != 5
            or section.association_section_index <= 0
            or section.section_external_symbols
            for section in runtime_associations
        )
    ):
        raise ValueError(
            f"{prefix} requires exact associative .text$x/.xdata$x topology"
        )
    funclet = next(
        section for section in runtime_associations if section.name == ".text$x"
    )
    xdata = next(
        section for section in runtime_associations if section.name == ".xdata$x"
    )
    if (
        funclet.association_section_index != xdata.association_section_index
        or funclet.section_size != 0x12
        or len(funclet.data) != 0x12
        or xdata.section_size != 0x28
        or len(xdata.data) != 0x28
        or funclet.data
        != bytes.fromhex(
            "8b 4d f0 e9 00 00 00 00 b8 00 00 00 00 "
            "e9 00 00 00 00"
        )
        or xdata.data
        != bytes.fromhex(
            "20 05 93 19 01 00 00 00 00 00 00 00 "
            "00 00 00 00 00 00 00 00 00 00 00 00 "
            "00 00 00 00 00 00 00 00 ff ff ff ff 00 00 00 00"
        )
    ):
        raise ValueError(
            f"{prefix} requires exact associative funclet/xdata body shape"
        )

    association_specs = (
        (
            funclet,
            (
                (
                    0x04,
                    IMAGE_REL_I386_REL32,
                    _cc_catalog.HUD_UI_MESSAGE_STACK_CONTAINER_DESTRUCTOR_SYMBOL,
                ),
                (0x09, IMAGE_REL_I386_DIR32, re.compile(r"\$T[0-9]+")),
                (
                    0x0E,
                    IMAGE_REL_I386_REL32,
                    _cc_catalog.HUD_UI_MESSAGE_STACK_FRAME_HANDLER_SYMBOL,
                ),
            ),
        ),
        (
            xdata,
            (
                (0x08, IMAGE_REL_I386_DIR32, re.compile(r"\$T[0-9]+")),
                (0x24, IMAGE_REL_I386_DIR32, re.compile(r"\$L[0-9]+")),
            ),
        ),
    )
    for section, specs in association_specs:
        expected_mask: set[int] = set()
        if len(section.relocations) != len(specs):
            raise ValueError(
                f"{prefix} rejects missing, duplicate, or extra "
                f"{section.name} relocations"
            )
        for relocation, (offset, relocation_type, symbol_spec) in zip(
            section.relocations,
            specs,
        ):
            symbol_matches = (
                symbol_spec.fullmatch(relocation.symbol_name) is not None
                if isinstance(symbol_spec, re.Pattern)
                else relocation.symbol_name == symbol_spec
            )
            if (
                relocation.offset != offset
                or relocation.type != relocation_type
                or not symbol_matches
                or struct.unpack_from("<I", section.data, offset)[0] != 0
            ):
                raise ValueError(
                    f"{prefix} rejects wrong {section.name} relocation "
                    "offset, type, target, or addend"
                )
            expected_mask.update(range(offset, offset + 4))
        if {
            index
            for index, masked in enumerate(section.relocation_mask)
            if masked
        } != expected_mask:
            raise ValueError(
                f"{prefix} requires an exact {section.name} relocation mask"
            )
    funclet_labels = {
        symbol.value: symbol.name
        for symbol in funclet.symbols
        if symbol.storage_class == 6
        and symbol.type == 0
        and re.fullmatch(r"\$L[0-9]+", symbol.name)
    }
    xdata_labels = {
        symbol.value: symbol.name
        for symbol in xdata.symbols
        if symbol.storage_class == 3
        and symbol.type == 0
        and re.fullmatch(r"\$T[0-9]+", symbol.name)
    }
    if (
        set(funclet_labels) != {0, 8}
        or set(xdata_labels) != {0, 0x20}
        or destructor.relocations[1].symbol_name != funclet_labels[8]
        or funclet.relocations[1].symbol_name != xdata_labels[0]
        or xdata.relocations[0].symbol_name != xdata_labels[0x20]
        or xdata.relocations[1].symbol_name != funclet_labels[0]
    ):
        raise ValueError(
            f"{prefix} requires exact main/funclet/xdata local-symbol linkage"
        )

    funclet_address = (
        struct.unpack_from("<I", retail_body, 0x09)[0] - 8
    )
    retail_funclet = _cc_cfg._hexdump_bytes(
        bridge.hexdump(hex(funclet_address), funclet.section_size)
    )
    if len(retail_funclet) != funclet.section_size:
        raise ValueError(f"{prefix} retail funclet extent is incomplete")
    xdata_address = struct.unpack_from("<I", retail_funclet, 0x09)[0]
    retail_xdata = _cc_cfg._hexdump_bytes(
        bridge.hexdump(hex(xdata_address), xdata.section_size)
    )
    if (
        len(retail_xdata) != xdata.section_size
        or struct.unpack_from("<I", retail_xdata, 0x08)[0]
        != xdata_address + 0x20
        or struct.unpack_from("<I", retail_xdata, 0x24)[0]
        != funclet_address
        or any(
            candidate_byte != retail_byte
            for index, (candidate_byte, retail_byte) in enumerate(
                zip(funclet.data, retail_funclet)
            )
            if not funclet.relocation_mask[index]
        )
        or any(
            candidate_byte != retail_byte
            for index, (candidate_byte, retail_byte) in enumerate(
                zip(xdata.data, retail_xdata)
            )
            if not xdata.relocation_mask[index]
        )
    ):
        raise ValueError(
            f"{prefix} requires exact relocation-normalized retail "
            "funclet/xdata topology"
        )
    funclet_container_target = (
        funclet_address
        + 0x08
        + struct.unpack_from("<i", retail_funclet, 0x04)[0]
    )
    funclet_handler_target = (
        funclet_address
        + 0x12
        + struct.unpack_from("<i", retail_funclet, 0x0E)[0]
    )
    if (
        funclet_container_target
        != address_value(_cc_catalog.HUD_UI_MESSAGE_STACK_CONTAINER_DESTRUCTOR_ADDRESS)
        or funclet_handler_target
        != address_value(_cc_catalog.HUD_UI_MESSAGE_STACK_FRAME_HANDLER_ADDRESS)
    ):
        raise ValueError(
            f"{prefix} requires the exact container unwind tail and frame "
            "handler"
        )

    retail_contract = _cc_extraction.extract_invocation_contract(
        _cc_listing.parse_assembly(bridge.assembly(target_address), source="bn"),
        source="bn",
        caller_identity=target_identity,
        caller_start=target_address,
        caller_end_exclusive=target_end,
        # This is the separately reviewed compiler-generated destructor body,
        # not an instruction subset of the ShutdownResources caller whose
        # package is active while candidate bridges are resolved.  Keep the
        # extractor's foreign-row rejection intact and explicitly start a
        # child-body extraction context only after the exact lifecycle
        # identity, extent, retail bytes, COFF body, and EH topology above have
        # all been proved.  Caller-local IAT transfer facts cannot cross this
        # physical-body boundary.
        indexes=_cc_identity._without_caller_scoped_retail_proofs(indexes),
        bridge_names=bridge_names,
    )
    if (
        len(retail_contract) != 2
        or retail_contract[0].get("ordinal") != 0
        or retail_contract[0].get("form") != "call"
        or retail_contract[0].get("dispatch") != "direct"
        or retail_contract[0].get("identity_kind") != "provider"
        or retail_contract[0].get("target_identity")
        not in indexes.provider_ids
        or retail_contract[0].get("cleanup_bytes") is not None
        or retail_contract[1]
        != {
            "ordinal": 1,
            "form": "call",
            "dispatch": "direct",
            "identity_kind": "direct",
            "target_identity": (
                _cc_catalog.HUD_UI_MESSAGE_STACK_CONTAINER_DESTRUCTOR_IDENTITY
            ),
            "storage_identity": "",
            "slot_displacement": None,
            "cleanup_bytes": None,
        }
    ):
        raise ValueError(
            f"{prefix} requires exact EH-vector then container retail calls"
        )
    candidate_contract = _cc_extraction.extract_invocation_contract(
        destructor.instructions,
        source="cod",
        caller_identity=target_identity,
        caller_start=target_address,
        caller_end_exclusive=target_end,
        indexes=indexes,
        bridge_names=bridge_names,
        compiler_generated_bridges={
            _cc_catalog.MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL: str(
                retail_contract[0]["target_identity"]
            ),
            _cc_catalog.HUD_UI_MESSAGE_STACK_CONTAINER_DESTRUCTOR_SYMBOL: (
                _cc_catalog.HUD_UI_MESSAGE_STACK_CONTAINER_DESTRUCTOR_IDENTITY
            ),
        },
        local_control_flow_indices=destructor.local_control_flow_indices,
        local_control_flow_targets=destructor.local_control_flow_targets,
    )
    if (
        len(candidate_contract) != 2
        or _cc_comparison.compare_call_contracts(retail_contract, candidate_contract)[
            "passed"
        ]
        is not True
    ):
        raise ValueError(
            f"{prefix} requires exact relocation-normalized candidate/retail "
            "call topology"
        )
    return True


def _prove_player_hud_ui_element_generated_destructor_bridge(
    candidate_name: str,
    target_identity: str,
    *,
    document: ProgressDocument,
    candidate: CandidateAssembly,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    invocation_row: Mapping[str, Any],
) -> bool:
    """Prove two exact Player inline HudUiElement vftable-reset destructors."""

    if candidate_name != "??1HudUiElement@@UAE@XZ":
        return False
    profiles = {
        (
            "symbol:recoil:function:0x41eb20",
            "0x41eb20",
            "0x41eb30",
        ): (
            "?ResetUnderwaterFxPass3UiSingleton@Player@@YAXXZ",
            "_g_Player_UnderwaterFxPass3Ui",
        ),
        (
            "symbol:recoil:function:0x41eb80",
            "0x41eb80",
            "0x41eb90",
        ): (
            "?ResetProjectileCameraFxPass3UiSingleton@Player@@YAXXZ",
            "_g_Player_State7FxPass3Ui",
        ),
    }
    profile = profiles.get(
        (
            caller_identity,
            normalize_address(caller_start),
            normalize_address(caller_end_exclusive),
        )
    )
    if profile is None:
        return False
    prefix = "Player HudUiElement generated-destructor bridge"
    caller_symbol, storage_symbol = profile
    target_symbol_id = "recoil:function:0x4b47a0"
    expected_target = f"symbol:{target_symbol_id}"
    target = document.collection("symbols").get(target_symbol_id)
    if (
        target_identity != expected_target
        or indexes.by_address.get("0x4b47a0") != expected_target
        or expected_target in indexes.provider_ids
        or not isinstance(target, Mapping)
        or _cc_identity._symbol_identity(target_symbol_id, target) != expected_target
        or target.get("binary") != "recoil"
        or target.get("kind") != "function"
        or target.get("address") != "0x4b47a0"
        or target.get("end_exclusive") != "0x4b47b0"
        or target.get("size") != 0x10
        or target.get("extent_state") != "known"
        or target.get("pipeline_class") != "authored-lifecycle"
        or target.get("authored_order_role")
        != "compiler-generated-implicit-cleanup"
        or target.get("ownership_state") != "primary-owned"
        or target.get("navigation_name")
        != "HudUiElement::ResetCommonFTable"
        or target.get("verification_target_ids")
        != [
            "recoil:vc5-target:hud_ui_element_destructor",
        ]
        or invocation_row
        != {
            "ordinal": 0,
            "form": "tail",
            "dispatch": "direct",
            "identity_kind": "direct",
            "target_identity": (
                "compiler-candidate-destructor:" + candidate_name
            ),
            "storage_identity": "",
            "slot_displacement": None,
            "cleanup_bytes": None,
        }
    ):
        raise ValueError(
            f"{prefix} rejects immutable target or invocation authority drift"
        )

    caller = candidate.caller_definition
    definition = candidate.complete_destructor_definitions.get(candidate_name)
    caller_relocations = (
        sorted(caller.relocations, key=lambda row: row.offset)
        if caller is not None else []
    )
    destructor_relocation = (
        definition.relocations[0]
        if definition is not None and len(definition.relocations) == 1
        else None
    )
    associated = (
        definition.associated_sections[0]
        if definition is not None
        and len(definition.associated_sections) == 1
        else None
    )
    associated_relocation = (
        associated.relocations[0]
        if associated is not None and len(associated.relocations) == 1
        else None
    )
    if (
        caller is None
        or caller.symbol != caller_symbol
        or caller.data
        != b"\xb9\x00\x00\x00\x00\xe9\x00\x00\x00\x00"
        + b"\x90" * 6
        or caller.relocation_mask
        != (False, True, True, True, True, False, True, True, True, True)
        + (False,) * 6
        or caller.undefined_external_functions.count(candidate_name) != 0
        or caller.defined_external_functions.count(candidate_name) != 1
        or len(caller_relocations) != 2
        or caller_relocations[0].offset != 1
        or caller_relocations[0].type != IMAGE_REL_I386_DIR32
        or caller_relocations[0].symbol_name != storage_symbol
        or caller_relocations[1].offset != 6
        or caller_relocations[1].type != IMAGE_REL_I386_REL32
        or caller_relocations[1].symbol_name != candidate_name
        or caller_relocations[0].symbol_index
        == caller_relocations[1].symbol_index
        or definition is None
        or definition.symbol != candidate_name
        or not definition.section_is_comdat
        or definition.comdat_selection != 2
        or definition.section_external_functions != (candidate_name,)
        or definition.section_size != 0x10
        or definition.data
        != b"\xc7\x01\x00\x00\x00\x00\xc3" + b"\x90" * 9
        or definition.relocation_mask
        != (False, False, True, True, True, True, False) + (False,) * 9
        or definition.local_control_flow_indices != frozenset()
        or bool(definition.local_control_flow_targets)
        or len(definition.instructions) != 2
        or _cc_cfg._instruction_mnemonic(definition.instructions[0]) != "mov"
        or _cc_cfg._instruction_operand(definition.instructions[0]).strip()
        != "dword [ecx], OFFSET FLAT:??_7HudUiElement@@6B@"
        or tuple(value.lower() for value in definition.instructions[0].bytes)
        != ("c7", "01", "00", "00", "00", "00")
        or not _cc_cfg._exact_return_terminates(definition.instructions[1])
        or destructor_relocation is None
        or destructor_relocation.offset != 2
        or destructor_relocation.type != IMAGE_REL_I386_DIR32
        or destructor_relocation.symbol_name != "??_7HudUiElement@@6B@"
        or associated is None
        or associated.name != ".debug$F"
        or associated.data
        != b"\x00\x00\x00\x00\x07" + b"\x00" * 11
        or associated.relocation_mask
        != (True, True, True, True) + (False,) * 12
        or associated.section_size != 0x10
        or not associated.section_is_comdat
        or associated.comdat_selection != 5
        or associated.section_external_symbols != ()
        or associated.symbols != ()
        or associated_relocation is None
        or associated_relocation.offset != 0
        or associated_relocation.type != 0x07
        or associated_relocation.symbol_name != candidate_name
    ):
        raise ValueError(
            f"{prefix} requires the exact caller and SELECT_ANY vftable-reset "
            "COD/COFF packages"
        )
    return True
