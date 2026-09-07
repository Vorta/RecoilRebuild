"""Recoil call-contract recoil hud timers fonts evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        IdentityIndexes,
        ReviewedMemberVptrStorageBridge,
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
from _recoil.lib.binja import BinaryNinjaBridge
from _recoil.lib.progress import ProgressDocument, address_value, normalize_address


def _hud_timer_floor_retail_iat_indexes(
    retail_instructions: Sequence[Instruction],
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_data_rows: Sequence[Any],
    bridge: BinaryNinjaBridge,
    reference: Path = _cc_catalog.DEFAULT_REFERENCE,
) -> IdentityIndexes:
    """Resolve the exact retail ``floor`` IAT slot for one reviewed caller.

    This is deliberately a per-body identity bridge, not a tracker/provider
    registration.  Expected truth comes only from the immutable retail PE, the
    live retail BN data/bytes, and the caller identity already present in the
    tracker.  Candidate symbols and candidate bytes are not consulted here.
    """

    if (
        caller_identity != _cc_catalog.HUD_TIMER_FLOOR_CALLER_IDENTITY
        or normalize_address(caller_start) != _cc_catalog.HUD_TIMER_FLOOR_CALLER_START
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_TIMER_FLOOR_CALLER_END_EXCLUSIVE
    ):
        return indexes

    caller = document.collection("symbols").get(
        _cc_catalog.HUD_TIMER_FLOOR_CALLER_SYMBOL_ID
    )
    if (
        not isinstance(caller, Mapping)
        or caller.get("binary") != "recoil"
        or caller.get("kind") != "function"
        or caller.get("pipeline_class") != "authored"
        or caller.get("ownership_state") != "primary-owned"
        or caller.get("address") != _cc_catalog.HUD_TIMER_FLOOR_CALLER_START
        or caller.get("end_exclusive")
        != _cc_catalog.HUD_TIMER_FLOOR_CALLER_END_EXCLUSIVE
        or caller.get("extent_state") != "known"
        or caller.get("size") != 0xA0
        or caller.get("navigation_name") != _cc_catalog.HUD_TIMER_FLOOR_CALLER_NAME
        or caller.get("output_section_id") != "recoil:section:.text"
        or caller.get("physical_block_id")
        != _cc_catalog.HUD_TIMER_FLOOR_PHYSICAL_BLOCK_ID
        or _cc_catalog.HUD_TIMER_FLOOR_TARGET_ID
        not in caller.get("verification_target_ids", ())
        or indexes.by_address.get(_cc_catalog.HUD_TIMER_FLOOR_CALLER_START)
        != _cc_catalog.HUD_TIMER_FLOOR_CALLER_IDENTITY
        or _cc_catalog.HUD_TIMER_FLOOR_CALLER_IDENTITY in indexes.provider_ids
    ):
        raise ValueError(
            "HUD timer floor IAT bridge requires its exact current tracker "
            "authored caller identity"
        )

    retail_import, _directory_context = retail_import_target(
        reference=reference,
        address=_cc_catalog.HUD_TIMER_FLOOR_IAT_ADDRESS,
        dll=_cc_catalog.HUD_TIMER_FLOOR_IMPORT_DLL,
        import_name=_cc_catalog.HUD_TIMER_FLOOR_IMPORT_NAME,
    )
    if (
        retail_import.address != _cc_catalog.HUD_TIMER_FLOOR_IAT_ADDRESS
        or retail_import.dll != _cc_catalog.HUD_TIMER_FLOOR_IMPORT_DLL
        or retail_import.import_name != _cc_catalog.HUD_TIMER_FLOOR_IMPORT_NAME
        or retail_import.import_ordinal is not None
    ):
        raise ValueError(
            "HUD timer floor IAT bridge immutable retail import tuple drifted"
        )

    iat_rows = [
        row
        for row in bridge_data_rows
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_TIMER_FLOOR_IAT_ADDRESS
    ]
    provider_rows = [
        row
        for row in bridge_data_rows
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_TIMER_FLOOR_PROVIDER_ADDRESS
    ]
    if (
        len(iat_rows) != 1
        or getattr(iat_rows[0], "name", "")
        != _cc_catalog.HUD_TIMER_FLOOR_IMPORT_NAME
        or getattr(iat_rows[0], "raw_name", "")
        != _cc_catalog.HUD_TIMER_FLOOR_IMPORT_NAME
        or getattr(iat_rows[0], "type_text", "")
        != _cc_catalog.HUD_TIMER_FLOOR_IAT_TYPE
        or getattr(iat_rows[0], "size", 0) != 4
    ):
        raise ValueError(
            "HUD timer floor IAT bridge requires one exact live retail typed "
            "IAT data row"
        )
    if (
        len(provider_rows) != 1
        or getattr(provider_rows[0], "name", "")
        != _cc_catalog.HUD_TIMER_FLOOR_IMPORT_NAME
        or getattr(provider_rows[0], "raw_name", "")
        != _cc_catalog.HUD_TIMER_FLOOR_IMPORT_NAME
        or getattr(provider_rows[0], "type_text", "")
        != _cc_catalog.HUD_TIMER_FLOOR_PROVIDER_TYPE
        or getattr(provider_rows[0], "size", -1) != 0
    ):
        raise ValueError(
            "HUD timer floor IAT bridge requires one exact live retail bound "
            "provider row"
        )

    iat_bytes = _cc_cfg._hexdump_bytes(
        bridge.hexdump(_cc_catalog.HUD_TIMER_FLOOR_IAT_ADDRESS, 4)
    )
    if (
        len(iat_bytes) != 4
        or normalize_address(struct.unpack("<I", iat_bytes)[0])
        != _cc_catalog.HUD_TIMER_FLOOR_PROVIDER_ADDRESS
    ):
        raise ValueError(
            "HUD timer floor IAT bridge retail slot bytes do not bind the "
            "exact provider row"
        )

    calls_through_slot = [
        instruction
        for instruction in retail_instructions
        if _cc_cfg._instruction_mnemonic(instruction) == "call"
        and _cc_targets._exact_ff15_absolute_address(instruction)
        == _cc_catalog.HUD_TIMER_FLOOR_IAT_ADDRESS
    ]
    calls_named_floor = [
        instruction
        for instruction in retail_instructions
        if _cc_cfg._instruction_mnemonic(instruction) == "call"
        and _cc_cfg._instruction_operand(instruction).strip().lower()
        == f"dword [{_cc_catalog.HUD_TIMER_FLOOR_IMPORT_NAME}]"
    ]
    if (
        calls_through_slot != calls_named_floor
        or tuple(
            _cc_cfg._source_instruction_address(instruction)
            for instruction in calls_through_slot
        )
        != _cc_catalog.HUD_TIMER_FLOOR_RETAIL_CALL_ADDRESSES
    ):
        raise ValueError(
            "HUD timer floor IAT bridge requires exactly the three reviewed "
            "retail FF15 floor calls"
        )

    storage_by_address = dict(indexes.storage_by_address)
    storage_by_name = dict(indexes.storage_by_name)
    for mapping, key in (
        (storage_by_address, _cc_catalog.HUD_TIMER_FLOOR_IAT_ADDRESS),
        (storage_by_name, _cc_catalog.HUD_TIMER_FLOOR_IMPORT_NAME),
        (storage_by_name, _cc_catalog.HUD_TIMER_FLOOR_CANDIDATE_IMPORT_SYMBOL),
    ):
        prior = mapping.get(key)
        if prior is not None and prior != _cc_catalog.HUD_TIMER_FLOOR_IAT_IDENTITY:
            raise ValueError(
                "HUD timer floor IAT bridge conflicts with an existing storage "
                f"identity for {key!r}"
            )
        mapping[key] = _cc_catalog.HUD_TIMER_FLOOR_IAT_IDENTITY
    return replace(
        indexes,
        storage_by_address=storage_by_address,
        storage_by_name=storage_by_name,
    )


def _hud_ui_mgr_percent_font_candidate_vptr_storage_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedMemberVptrStorageBridge]:
    """Prove only the current percent-text-panel vptr load and slot call.

    Retail's SetFont call remains an independently extracted direct call.  The
    returned candidate-only bridge supplies storage provenance, not equivalence
    between the candidate virtual dispatch and retail direct dispatch.
    """
    from _recoil.call_contract.records import ReviewedMemberVptrStorageBridge
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_CALLER_START:
        return {}
    if (
        caller_identity != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_CALLER_END_EXCLUSIVE
    ):
        raise ValueError(
            "HUD percent-font vptr bridge requires the exact reviewed "
            "InitHudLayouts caller and extent"
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
            "HUD percent-font vptr bridge requires the exact registered HUD "
            "target, caller definition, and unique hud.cpp contribution"
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
            "HUD percent-font vptr bridge requires the exact authored "
            "InitHudLayouts hud.cpp contribution identity"
        )

    if (
        indexes.by_address.get(_cc_catalog.HUD_UI_MGR_PERCENT_PANEL_NEW_ADDRESS)
        != _cc_catalog.HUD_UI_MGR_PERCENT_PANEL_NEW_IDENTITY
        or [
            address
            for address, identity in indexes.by_address.items()
            if identity == _cc_catalog.HUD_UI_MGR_PERCENT_PANEL_NEW_IDENTITY
        ]
        != [_cc_catalog.HUD_UI_MGR_PERCENT_PANEL_NEW_ADDRESS]
        or indexes.by_address.get(_cc_catalog.HUD_UI_MGR_PERCENT_PANEL_FONT_ADDRESS)
        != _cc_catalog.HUD_UI_MGR_PERCENT_PANEL_FONT_IDENTITY
        or [
            address
            for address, identity in indexes.by_address.items()
            if identity == _cc_catalog.HUD_UI_MGR_PERCENT_PANEL_FONT_IDENTITY
        ]
        != [_cc_catalog.HUD_UI_MGR_PERCENT_PANEL_FONT_ADDRESS]
        or indexes.by_candidate_name.get(
            _cc_catalog.HUD_UI_MGR_PERCENT_PANEL_FONT_SYMBOL
        )
        != _cc_catalog.HUD_UI_MGR_PERCENT_PANEL_FONT_IDENTITY
        or _cc_catalog.HUD_UI_MGR_PERCENT_PANEL_FONT_IDENTITY in indexes.provider_ids
    ):
        raise ValueError(
            "HUD percent-font vptr bridge rejects missing, aliased, "
            "provider, or conflicting new/SetFont identities"
        )
    expected_set_font = [
        row
        for row in expected
        if (
            row.get("target_identity")
            == _cc_catalog.HUD_UI_MGR_PERCENT_PANEL_FONT_IDENTITY
            and row.get("ordinal")
            == _cc_catalog.HUD_UI_MGR_PERCENT_PANEL_RETAIL_FONT_ORDINAL
        )
    ]
    if (
        len(expected_set_font) != 1
        or any(
            expected_set_font[0].get(key) != value
            for key, value in {
                "form": "call",
                "dispatch": "direct",
                "identity_kind": "direct",
                "storage_identity": "",
                "slot_displacement": None,
                "cleanup_bytes": None,
            }.items()
        )
    ):
        raise ValueError(
            "HUD percent-font vptr bridge requires one exact retail direct "
            f"HudUiPanel::SetFont contract; got {expected_set_font!r}"
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
    instruction_by_offset = {
        address - start: instruction
        for instruction, address in zip(instructions, addresses)
        if address is not None and counts.get(address) == 1
    }
    index_by_address = {
        address: index
        for index, address in enumerate(addresses)
        if address is not None and counts.get(address) == 1
    }
    all_functions = (
        definition.undefined_external_functions
        + definition.defined_external_functions
    )
    all_data = (
        definition.undefined_external_data + definition.defined_external_data
    )

    def exact_row(
        offset: int,
        body: bytes,
        pattern: str,
    ) -> bool:
        instruction = instruction_by_offset.get(offset)
        return bool(
            instruction is not None
            and counts.get(start + offset) == 1
            and bytes(int(value, 16) for value in instruction.bytes)
            == body
            and re.fullmatch(
                pattern,
                instruction.raw_text.strip(),
                flags=re.IGNORECASE,
            )
            is not None
            and definition.data[offset : offset + len(body)] == body
        )

    def exact_relocation(
        offset: int,
        symbol: str,
        relocation_type: int,
        references: Sequence[str],
    ) -> bool:
        relocations = [
            relocation
            for relocation in definition.relocations
            if relocation.offset == offset
        ]
        return bool(
            len(relocations) == 1
            and relocations[0].symbol_name == symbol
            and relocations[0].type == relocation_type
            and offset + 4 <= len(definition.data)
            and struct.unpack_from("<I", definition.data, offset)[0] == 0
            and all(
                definition.relocation_mask[index]
                for index in range(offset, offset + 4)
            )
            and references.count(symbol) == 1
        )

    direct_site_rows = {
        -0x4F: (
            b"\xe8\x00\x00\x00\x00",
            r"call\s+\?\?2@YAPAXI@Z",
        ),
        -0x4A: (b"\x8b\xf8", r"mov\s+edi\s*,\s*eax"),
        -0x48: (b"\x83\xc4\x04", r"add\s+esp\s*,\s*4"),
        -0x45: (b"\x3b\xfb", r"cmp\s+edi\s*,\s*ebx"),
        -0x43: (b"\x74\x60", r"je\s+\$L[0-9]+"),
        -0x41: (
            b"\x8d\x4f\x1c",
            r"lea\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[edi\+28\]",
        ),
        -0x3E: (b"\x3b\xcb", r"cmp\s+ecx\s*,\s*ebx"),
        -0x3C: (b"\x74\x06", r"je\s+\$L[0-9]+"),
        -0x3A: (b"\x53", r"push\s+ebx"),
        -0x39: (
            b"\xe8\x00\x00\x00\x00",
            r"call\s+\?\?0HudUiWidget@@QAE@I@Z",
        ),
        -0x34: (
            b"\x8d\xb7\xd8\x00\x00\x00",
            r"lea\s+esi\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[edi\+216\]",
        ),
        -0x2E: (b"\x3b\xf3", r"cmp\s+esi\s*,\s*ebx"),
        -0x2C: (b"\x74\x0a", r"je\s+\$L[0-9]+"),
        -0x2A: (b"\x53", r"push\s+ebx"),
        -0x29: (b"\x53", r"push\s+ebx"),
        -0x28: (b"\x53", r"push\s+ebx"),
        -0x27: (b"\x8b\xce", r"mov\s+ecx\s*,\s*esi"),
        -0x25: (
            b"\xe8\x00\x00\x00\x00",
            r"call\s+\?\?0HudUiPanel@@QAE@PBDHH@Z",
        ),
        -0x20: (
            b"\x68\x40\xbf\x20\x00",
            r"push\s+2146112",
        ),
        -0x1B: (b"\x8b\xce", r"mov\s+ecx\s*,\s*esi"),
        -0x19: (
            b"\xe8\x00\x00\x00\x00",
            r"call\s+\?SetTextColor@HudUiPanel@@QAEII@Z",
        ),
        -0x14: (b"\x8b\xce", r"mov\s+ecx\s*,\s*esi"),
        -0x12: (b"\x6a\x02", r"push\s+2"),
        -0x10: (b"\x53", r"push\s+ebx"),
        -0x0F: (b"\x53", r"push\s+ebx"),
        -0x0E: (b"\x6a\x06", r"push\s+6"),
        -0x0C: (b"\x68\xf4\x01\x00\x00", r"push\s+500"),
        -0x07: (b"\x6a\x0a", r"push\s+10"),
        -0x05: (
            b"\x68\x00\x00\x00\x00",
            r"push\s+OFFSET\s+FLAT:\?g_HudFontName_Arial@@3PADA",
        ),
        0x00: (
            b"\xe8\x00\x00\x00\x00",
            rf"call\s+{re.escape(_cc_catalog.HUD_UI_MGR_PERCENT_PANEL_FONT_SYMBOL)}",
        ),
        0x05: (b"\x8b\xce", r"mov\s+ecx\s*,\s*esi"),
        0x07: (b"\x55", r"push\s+ebp"),
        0x08: (b"\x55", r"push\s+ebp"),
        0x09: (b"\x6a\x01", r"push\s+1"),
        0x0B: (
            b"\xe8\x00\x00\x00\x00",
            r"call\s+\?SetShadow@HudUiPanel@@QAEIIHH@Z",
        ),
    }
    direct_call_offsets = [
        offset
        for offset in instruction_by_offset
        if exact_row(
            offset,
            b"\xe8\x00\x00\x00\x00",
            rf"call\s+{re.escape(_cc_catalog.HUD_UI_MGR_PERCENT_PANEL_FONT_SYMBOL)}",
        )
    ]
    direct_structural_offsets = [
        offset
        for offset in direct_call_offsets
        if all(
            exact_row(offset + relative, body, pattern)
            for relative, (body, pattern) in direct_site_rows.items()
        )
    ]
    legacy_site_rows = {
        0x00: (
            b"\x8b\x06",
            r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[esi\]",
        ),
        0x02: (b"\x8b\xce", r"mov\s+ecx\s*,\s*esi"),
        0x04: (b"\x6a\x02", r"push\s+2"),
        0x06: (b"\x55", r"push\s+ebp"),
        0x07: (b"\x55", r"push\s+ebp"),
        0x08: (b"\x6a\x06", r"push\s+6"),
        0x0A: (b"\x68\xf4\x01\x00\x00", r"push\s+500"),
        0x0F: (b"\x6a\x0a", r"push\s+10"),
        0x11: (
            b"\x68\x00\x00\x00\x00",
            r"push\s+OFFSET\s+FLAT:\?g_HudFontName_Arial@@3PADA",
        ),
        0x16: (
            b"\xff\x90\x80\x00\x00\x00",
            r"call\s+(?:dword\s+(?:ptr\s+)?)?\[eax\+128\]",
        ),
    }
    legacy_load_offsets = [
        offset
        for offset in instruction_by_offset
        if all(
            exact_row(offset + relative, body, pattern)
            for relative, (body, pattern) in legacy_site_rows.items()
        )
    ]
    if (
        len(direct_structural_offsets) > 1
        or len(legacy_load_offsets) > 1
        or bool(direct_structural_offsets) == bool(legacy_load_offsets)
    ):
        raise ValueError(
            "HUD percent-font bridge requires exactly one unambiguous "
            "embedded direct SetFont structure or legacy virtual "
            "slot-0x80 structure"
        )

    if direct_structural_offsets:
        call_offset = direct_structural_offsets[0]
        direct_relocation_specs = (
            (
                -0x4E,
                _cc_catalog.HUD_UI_MGR_PERCENT_PANEL_NEW_SYMBOL,
                IMAGE_REL_I386_REL32,
            ),
            (
                -0x38,
                _cc_catalog.HUD_UI_MGR_PERCENT_PANEL_WIDGET_CONSTRUCTOR_SYMBOL,
                IMAGE_REL_I386_REL32,
            ),
            (
                -0x24,
                _cc_catalog.HUD_UI_MGR_PERCENT_PANEL_CONSTRUCTOR_SYMBOL,
                IMAGE_REL_I386_REL32,
            ),
            (
                -0x18,
                _cc_catalog.HUD_UI_MGR_PERCENT_PANEL_TEXT_COLOR_SYMBOL,
                IMAGE_REL_I386_REL32,
            ),
            (
                -0x04,
                _cc_catalog.HUD_UI_MGR_PERCENT_PANEL_FONT_NAME_SYMBOL,
                IMAGE_REL_I386_DIR32,
            ),
            (
                0x01,
                _cc_catalog.HUD_UI_MGR_PERCENT_PANEL_FONT_SYMBOL,
                IMAGE_REL_I386_REL32,
            ),
            (
                0x0C,
                _cc_catalog.HUD_UI_MGR_PERCENT_PANEL_SHADOW_SYMBOL,
                IMAGE_REL_I386_REL32,
            ),
        )
        if any(
            not exact_relocation(
                call_offset + relative,
                symbol,
                relocation_type,
                (
                    all_data
                    if relocation_type == IMAGE_REL_I386_DIR32
                    else all_functions
                ),
            )
            for relative, symbol, relocation_type
            in direct_relocation_specs
        ):
            raise ValueError(
                "HUD percent-font direct bridge requires the exact bounded "
                "construction, SetFont, font-name, and use relocations"
            )
        expected_bounded_relocations = {
            (call_offset + relative, symbol, relocation_type)
            for relative, symbol, relocation_type
            in direct_relocation_specs
        }
        actual_bounded_relocations = {
            (
                relocation.offset,
                relocation.symbol_name,
                relocation.type,
            )
            for relocation in definition.relocations
            if call_offset - 0x4F
            <= relocation.offset
            < call_offset + 0x10
        }
        if actual_bounded_relocations != expected_bounded_relocations:
            raise ValueError(
                "HUD percent-font direct bridge rejects extra, duplicate, "
                "or wrong bounded structural relocations"
            )
        for branch_relative, target_relative in {
            -0x43: 0x1F,
            -0x3C: -0x34,
            -0x2C: -0x20,
        }.items():
            branch_index = index_by_address[
                start + call_offset + branch_relative
            ]
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
                or addresses[branch[1]]
                != start + call_offset + target_relative
            ):
                raise ValueError(
                    "HUD percent-font direct bridge requires the exact "
                    "bounded construction null-check CFG"
                )
        receiver_index = index_by_address[start + call_offset - 0x14]
        call_index = index_by_address[start + call_offset]
        ebx_writes = [
            index
            for index in range(receiver_index)
            if _cc_cfg._instruction_may_clobber_register(
                instructions[index],
                "ebx",
            )
        ]
        zero_index = ebx_writes[-1] if ebx_writes else None
        if (
            zero_index is None
            or bytes(
                int(value, 16) for value in instructions[zero_index].bytes
            )
            != b"\x33\xdb"
            or re.fullmatch(
                r"xor\s+ebx\s*,\s*ebx",
                instructions[zero_index].raw_text.strip(),
                flags=re.IGNORECASE,
            )
            is None
            or any(
                _cc_cfg._instruction_may_clobber_register(
                    instructions[index],
                    "ebx",
                )
                for index in range(zero_index + 1, receiver_index)
            )
            or any(
                _cc_cfg._instruction_mnemonic(instructions[index]).startswith("j")
                or _cc_cfg._instruction_may_clobber_register(
                    instructions[index],
                    "esi",
                )
                or _cc_cfg._instruction_may_clobber_register(
                    instructions[index],
                    "ecx",
                )
                for index in range(receiver_index + 1, call_index)
            )
            or _cc_cfg._cleanup_after(instructions, call_index) is not None
        ):
            raise ValueError(
                "HUD percent-font direct bridge rejects an unproven zero "
                "argument, receiver clobber, CFG, or caller cleanup"
            )
        return {}

    load_offset = legacy_load_offsets[0]
    legacy_rows = {
        -0x3B: (
            b"\xe8\x00\x00\x00\x00",
            r"call\s+\?\?2@YAPAXI@Z",
        ),
        -0x36: (b"\x8b\xf8", r"mov\s+edi\s*,\s*eax"),
        -0x31: (b"\x3b\xfd", r"cmp\s+edi\s*,\s*ebp"),
        -0x2F: (b"\x74\x65", r"je\s+\$L[0-9]+"),
        -0x2D: (
            b"\x8d\x4f\x1c",
            r"lea\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[edi\+28\]",
        ),
        -0x2A: (b"\x3b\xcd", r"cmp\s+ecx\s*,\s*ebp"),
        -0x28: (b"\x74\x06", r"je\s+\$L[0-9]+"),
        -0x26: (b"\x55", r"push\s+ebp"),
        -0x25: (
            b"\xe8\x00\x00\x00\x00",
            r"call\s+\?\?0HudUiWidget@@QAE@I@Z",
        ),
        -0x20: (
            b"\x8d\xb7\xd8\x00\x00\x00",
            r"lea\s+esi\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[edi\+216\]",
        ),
        -0x1A: (b"\x3b\xf5", r"cmp\s+esi\s*,\s*ebp"),
        -0x18: (b"\x74\x0a", r"je\s+\$L[0-9]+"),
        -0x16: (b"\x55", r"push\s+ebp"),
        -0x15: (b"\x55", r"push\s+ebp"),
        -0x14: (b"\x55", r"push\s+ebp"),
        -0x13: (b"\x8b\xce", r"mov\s+ecx\s*,\s*esi"),
        -0x11: (
            b"\xe8\x00\x00\x00\x00",
            r"call\s+\?\?0HudUiPanel@@QAE@PBDHH@Z",
        ),
        -0x0C: (b"\x68\x40\xbf\x20\x00", r"push\s+2146112"),
        -0x07: (b"\x8b\xce", r"mov\s+ecx\s*,\s*esi"),
        -0x05: (
            b"\xe8\x00\x00\x00\x00",
            r"call\s+\?SetTextColor@HudUiPanel@@QAEII@Z",
        ),
        **legacy_site_rows,
        0x1C: (b"\x8b\xce", r"mov\s+ecx\s*,\s*esi"),
        0x1E: (b"\x6a\xff", r"push\s+-1"),
        0x20: (b"\x6a\xff", r"push\s+-1"),
        0x22: (b"\x6a\x01", r"push\s+1"),
        0x24: (
            b"\xe8\x00\x00\x00\x00",
            r"call\s+\?SetShadow@HudUiPanel@@QAEIIHH@Z",
        ),
    }
    if not all(
        exact_row(load_offset + relative, body, pattern)
        for relative, (body, pattern) in legacy_rows.items()
    ):
        raise ValueError(
            "HUD percent-font legacy bridge requires the exact structural "
            "construction, receiver, virtual-call, and shadow topology"
        )
    relocation_specs = (
        (-0x3A, _cc_catalog.HUD_UI_MGR_PERCENT_PANEL_NEW_SYMBOL, IMAGE_REL_I386_REL32),
        (
            -0x24,
            _cc_catalog.HUD_UI_MGR_PERCENT_PANEL_WIDGET_CONSTRUCTOR_SYMBOL,
            IMAGE_REL_I386_REL32,
        ),
        (
            -0x10,
            _cc_catalog.HUD_UI_MGR_PERCENT_PANEL_CONSTRUCTOR_SYMBOL,
            IMAGE_REL_I386_REL32,
        ),
        (
            -0x04,
            _cc_catalog.HUD_UI_MGR_PERCENT_PANEL_TEXT_COLOR_SYMBOL,
            IMAGE_REL_I386_REL32,
        ),
        (
            0x12,
            _cc_catalog.HUD_UI_MGR_PERCENT_PANEL_FONT_NAME_SYMBOL,
            IMAGE_REL_I386_DIR32,
        ),
        (
            0x25,
            _cc_catalog.HUD_UI_MGR_PERCENT_PANEL_SHADOW_SYMBOL,
            IMAGE_REL_I386_REL32,
        ),
    )
    for relative, symbol, relocation_type in relocation_specs:
        references = (
            all_data
            if relocation_type == IMAGE_REL_I386_DIR32
            else all_functions
        )
        if not exact_relocation(
            load_offset + relative,
            symbol,
            relocation_type,
            references,
        ):
            raise ValueError(
                "HUD percent-font legacy bridge requires one exact "
                f"relocation-backed reference to {symbol!r}"
            )

    for branch_relative, target_relative in {
        -0x2F: 0x38,
        -0x28: -0x20,
        -0x18: -0x0C,
    }.items():
        branch_index = index_by_address[
            start + load_offset + branch_relative
        ]
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
            or addresses[branch[1]]
            != start + load_offset + target_relative
        ):
            raise ValueError(
                "HUD percent-font legacy bridge requires the exact reviewed "
                "null-check CFG"
            )
    load_index = index_by_address[start + load_offset]
    receiver_index = index_by_address[start + load_offset + 0x02]
    call_offset = load_offset + 0x16
    call_index = index_by_address[start + call_offset]
    if (
        any(definition.relocation_mask[call_offset : call_offset + 6])
        or any(
            relocation.offset < call_offset + 6
            and call_offset < relocation.offset + 4
            for relocation in definition.relocations
        )
        or _cc_cfg._cleanup_after(instructions, call_index) is not None
        or any(
            _cc_cfg._instruction_mnemonic(instructions[index]).startswith("j")
            or _cc_cfg._instruction_may_clobber_register(
                instructions[index],
                "eax",
            )
            or _cc_cfg._instruction_may_clobber_register(
                instructions[index],
                "esi",
            )
            for index in range(load_index + 1, call_index)
        )
        or any(
            _cc_cfg._instruction_may_clobber_register(
                instructions[index],
                "ecx",
            )
            for index in range(receiver_index + 1, call_index)
        )
    ):
        raise ValueError(
            "HUD percent-font legacy bridge rejects relocation-backed call "
            "bytes, CFG, cleanup, or EAX/ECX/ESI clobbers"
        )
    receiver_provenance = (
        "address(call-result("
        f"{_cc_catalog.HUD_UI_MGR_PERCENT_PANEL_NEW_IDENTITY}"
        ")+0xd8)"
    )
    return {
        normalize_address(load_offset): ReviewedMemberVptrStorageBridge(
            register="eax",
            source_register="esi",
            source_provenance=receiver_provenance,
            receiver_register="ecx",
            receiver_provenance=receiver_provenance,
            storage_identity=_cc_catalog.HUD_UI_MGR_PERCENT_PANEL_STORAGE_IDENTITY,
            slot_displacement=_cc_catalog.HUD_UI_MGR_PERCENT_PANEL_VPTR_SLOT_DISPLACEMENT,
            call_address=normalize_address(call_offset),
        )
    }
