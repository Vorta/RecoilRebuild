"""Recoil call-contract recoil hud sensor track evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import instructions as _cc_instructions
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        IdentityIndexes,
        ReviewedLoopVptrStorageBridge,
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


def _hud_ui_mgr_sensor_track_counter_slot_vptr_bridges(
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedLoopVptrStorageBridge],
    dict[str, ReviewedLoopVptrStorageBridge],
]:
    """Prove PlaceTrackCounterWidget's first weapon-slot SetPos call."""
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_CALLER_START:
        return {}, {}

    caller_symbol_id = (
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_CALLER_IDENTITY.removeprefix(
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
        == _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_CALLER_START
    ]
    if (
        caller_identity
        != _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(normalized_start) != caller_identity
        or caller_identity in indexes.provider_ids
        or not isinstance(caller_row, Mapping)
        or caller_row.get("binary") != "recoil"
        or caller_row.get("kind") != "function"
        or caller_row.get("pipeline_class") != "authored"
        or caller_row.get("ownership_state") != "primary-owned"
        or caller_row.get("address") != normalized_start
        or caller_row.get("end_exclusive")
        != _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_CALLER_END_EXCLUSIVE
        or caller_row.get("extent_state") != "known"
        or caller_row.get("size") != 0x250
        or caller_row.get("navigation_name")
        != "HudUiMgrSensor::PlaceTrackCounterWidget"
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
        != _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
        or caller is None
        or caller.symbol
        != _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_CALLER_SYMBOL
        or len(caller.data) < (
            _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_CANDIDATE_CALL_OFFSET + 3
        )
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
            "HUD track-counter slot bridge requires the exact authored "
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
        != r"\?PlaceTrackCounterWidget@HudUiMgrSensor@@.*"
        or getattr(contribution_row, "name", "")
        != "HudUiMgrSensor::PlaceTrackCounterWidget"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD track-counter slot bridge requires the exact authored "
            "hud.cpp contribution row"
        )

    symbols = document.collection("symbols")
    storages = document.collection("storage_contributions")
    aggregate_symbol = symbols.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID)
    aggregate_storage = storages.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_STORAGE_ID)
    aggregate_target = document.collection("verification_targets").get(
        _cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_ID
    )
    aggregate_registration = (
        aggregate_target.get("registration")
        if isinstance(aggregate_target, Mapping)
        else None
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
    array_start = (
        aggregate_start
        + _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_ARRAY_DISPLACEMENT
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
        or aggregate_storage.get("symbol_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID]
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
        or indexes.storage_by_address.get(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
        )
        != aggregate_identity
        or len(exact_containers) != 1
        or aggregate_identity in indexes.provider_ids
        or normalize_address(array_start) != "0x4e6dcc"
        or (
            _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_ARRAY_DISPLACEMENT
            + _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_CAPACITY
            * _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_ARRAY_STRIDE
            > _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
        )
        or indexes.storage_by_address.get(
            normalize_address(array_start),
            "",
        )
        != ""
    ):
        raise ValueError(
            "HUD track-counter slot bridge requires the exact reviewed "
            "aggregate symbol, storage, target, and bounded uncatalogued "
            "+0xefc weapon-slot array authority"
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
        0x412073: ("mov", b"\xa1\xc8\x6d\x4e\x00"),
        0x412079: ("cmp", b"\x83\xf8\x20"),
        0x41207C: ("mov", b"\x8b\xda"),
        0x412095: ("mov", b"\x8b\xf0"),
        0x412098: ("shl", b"\xc1\xe6\x03"),
        0x41209B: ("sub", b"\x2b\xf0"),
        0x41209F: ("shl", b"\xc1\xe6\x06"),
        0x4120A2: ("add", b"\x81\xc6\xcc\x6d\x4e\x00"),
        0x4120A8: ("inc", b"\x40"),
        0x4120A9: ("mov", b"\xa3\xc8\x6d\x4e\x00"),
        0x4120C3: ("mov", b"\x8b\x2e"),
        0x4120C8: ("test", b"\x85\xc0"),
        0x4120CA: ("je", b"\x74\x0e"),
        0x4120CE: ("call", b"\xe8\xd3\x3f\x0b\x00"),
        0x4120D8: ("jmp", b"\xeb\x08"),
        0x4120DA: ("call", b"\xe8\xc7\x3f\x0b\x00"),
        0x4120E2: ("call", b"\xe8\xbf\x3f\x0b\x00"),
        0x4120E7: ("push", b"\x50"),
        0x4120E8: ("mov", b"\x8b\xce"),
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_RETAIL_CALL: (
            "call",
            b"\xff\x55\x0c",
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
            "HUD track-counter slot bridge requires the exact retail "
            "counter/index/stride/branch/vptr/receiver/slot call unit"
        )
    retail_call_index = retail_instructions.index(
        retail_by_address[_cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_RETAIL_CALL]
    )
    retail_vptr_index = retail_instructions.index(
        retail_by_address[0x4120C3]
    )
    if (
        _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(retail_instructions[retail_call_index])
        )
        not in {"ebp+12", "ebp+0xc"}
        or _cc_cfg._instruction_operand(
            retail_by_address[0x4120E8]
        ).replace(" ", "").lower()
        != "ecx,esi"
        or any(
            _cc_cfg._instruction_may_clobber_register(
                retail_instructions[index],
                register,
            )
            for index in range(retail_vptr_index + 1, retail_call_index)
            for register in ("ebp", "esi")
        )
        or _cc_cfg._cleanup_after(retail_instructions, retail_call_index)
        is not None
        or sum(
            1
            for instruction in retail_instructions[: retail_call_index + 1]
            if (
                _cc_cfg._instruction_mnemonic(instruction) == "call"
                and _cc_targets._exact_memory_expression(
                    _cc_cfg._instruction_operand(instruction)
                )
                in {"ebp+12", "ebp+0xc"}
            )
        )
        != 1
    ):
        raise ValueError(
            "HUD track-counter slot bridge rejects retail EBP/ESI reaching "
            "definition, receiver, virtual slot, cleanup, or uniqueness drift"
        )

    aggregate_relocations = [
        relocation
        for relocation in caller.relocations
        if relocation.symbol_name
        == _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
    ]
    array_relocations = [
        relocation
        for relocation in aggregate_relocations
        if (
            relocation.type == IMAGE_REL_I386_DIR32
            and relocation.offset + 4 <= len(caller.data)
            and struct.unpack_from("<I", caller.data, relocation.offset)[0]
            == _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_ARRAY_DISPLACEMENT
        )
    ]
    if (
        len(array_relocations) != 1
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in caller.defined_external_data
    ):
        raise ValueError(
            "HUD track-counter slot bridge requires one exact candidate "
            "aggregate+0xefc DIR32 relocation and undefined aggregate symbol"
        )
    array_relocation = array_relocations[0]
    candidate_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    offset_counts: dict[int, int] = {}
    for offset in candidate_offsets:
        if offset is not None:
            offset_counts[offset] = offset_counts.get(offset, 0) + 1
    index_by_offset = {
        offset: index
        for index, offset in enumerate(candidate_offsets)
        if offset is not None and offset_counts.get(offset) == 1
    }

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
            instruction_body = encoded_body(index)
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
            offset + len(instruction_body) <= len(caller.data)
            and caller.data[
                offset : offset + len(instruction_body)
            ]
            == instruction_body
            and {
                position
                for position in range(
                    offset,
                    offset + len(instruction_body),
                )
                if caller.relocation_mask[position]
            }
            == allowed_mask
        )

    frame_allocation_rows = [
        index
        for index, offset in enumerate(candidate_offsets)
        if (
            offset is not None
            and _cc_cfg._instruction_mnemonic(candidate.instructions[index])
            == "sub"
            and _cc_cfg._instruction_operand(
                candidate.instructions[index]
            ).split(",", 1)[0].strip().lower()
            == "esp"
            and len(encoded_body(index)) == 3
            and encoded_body(index)[:2] == b"\x83\xec"
            and encoded_body(index)[2] != 0
            and candidate_body_matches(index)
        )
    ]
    if (
        len(frame_allocation_rows) != 1
        or candidate_offsets[frame_allocation_rows[0]] != 0
    ):
        raise ValueError(
            "HUD track-counter slot bridge requires one unique nonzero "
            "entry sub-ESP frame allocation"
        )
    frame_size = encoded_body(frame_allocation_rows[0])[2]

    relocation_owners = [
        index
        for index, offset in enumerate(candidate_offsets)
        if (
            offset is not None
            and offset
            <= array_relocation.offset
            < offset + len(candidate.instructions[index].bytes)
        )
    ]
    if len(relocation_owners) != 1:
        raise ValueError(
            "HUD track-counter slot bridge requires one unique instruction "
            "owner for the aggregate+0xefc relocation"
        )
    array_add_index = relocation_owners[0]
    array_add_offset = candidate_offsets[array_add_index]
    assert array_add_offset is not None
    array_add_operands = [
        item.strip()
        for item in _cc_cfg._instruction_operand(
            candidate.instructions[array_add_index]
        ).split(",", 1)
    ]
    array_expressions = {
        (
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
            f"+{_cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_ARRAY_DISPLACEMENT}"
        ),
        (
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
            f"+0x{_cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_ARRAY_DISPLACEMENT:x}"
        ),
    }
    array_source_match = (
        re.fullmatch(
            r"OFFSET\s+FLAT:(?P<expression>.+)",
            array_add_operands[1],
            flags=re.IGNORECASE,
        )
        if len(array_add_operands) == 2
        else None
    )
    array_source_expression = (
        _cc_targets._exact_memory_expression(array_source_match.group("expression"))
        if array_source_match is not None
        else ""
    )
    if (
        _cc_cfg._instruction_mnemonic(
            candidate.instructions[array_add_index]
        )
        != "add"
        or len(array_add_operands) != 2
        or array_add_operands[0].lower() != "esi"
        or array_source_expression not in array_expressions
        or encoded_body(array_add_index)
        != b"\x81\xc6\xfc\x0e\x00\x00"
        or array_relocation.offset != array_add_offset + 2
        or not candidate_body_matches(
            array_add_index,
            (array_relocation,),
        )
    ):
        raise ValueError(
            "HUD track-counter slot bridge requires the exact structurally "
            "owned and masked aggregate+0xefc array-base add"
        )

    count_relocations = [
        relocation
        for relocation in aggregate_relocations
        if (
            relocation.type == IMAGE_REL_I386_DIR32
            and relocation.offset + 4 <= len(caller.data)
            and struct.unpack_from("<I", caller.data, relocation.offset)[0]
            == _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_COUNT_DISPLACEMENT
        )
    ]
    if len(count_relocations) != 2:
        raise ValueError(
            "HUD track-counter slot bridge requires exact count load/store "
            "aggregate+0xef8 relocations"
        )
    count_owner_rows: list[tuple[int, CoffRelocation]] = []
    for relocation in count_relocations:
        owners = [
            index
            for index, offset in enumerate(candidate_offsets)
            if (
                offset is not None
                and offset
                <= relocation.offset
                < offset + len(candidate.instructions[index].bytes)
            )
        ]
        if len(owners) != 1:
            raise ValueError(
                "HUD track-counter slot bridge requires unique count "
                "relocation instruction owners"
            )
        count_owner_rows.append((owners[0], relocation))
    count_load_rows = [
        (index, relocation)
        for index, relocation in count_owner_rows
        if (
            _cc_cfg._instruction_mnemonic(candidate.instructions[index]) == "mov"
            and encoded_body(index) == b"\xa1\xf8\x0e\x00\x00"
            and _cc_cfg._instruction_operand(
                candidate.instructions[index]
            ).split(",", 1)[0].strip().lower()
            == "eax"
            and candidate_body_matches(index, (relocation,))
        )
    ]
    count_store_rows = [
        (index, relocation)
        for index, relocation in count_owner_rows
        if (
            _cc_cfg._instruction_mnemonic(candidate.instructions[index]) == "mov"
            and encoded_body(index) == b"\xa3\xf8\x0e\x00\x00"
            and _cc_cfg._instruction_operand(
                candidate.instructions[index]
            ).split(",", 1)[-1].strip().lower()
            == "eax"
            and candidate_body_matches(index, (relocation,))
        )
    ]
    if len(count_load_rows) != 1 or len(count_store_rows) != 1:
        raise ValueError(
            "HUD track-counter slot bridge requires one exact relocated "
            "target-marker count load and store"
        )
    count_load_index = count_load_rows[0][0]
    count_store_index = count_store_rows[0][0]

    def exact_indices(
        mnemonic: str,
        normalized_operand: str,
        instruction_body: bytes,
        *,
        start: int,
        end: int,
    ) -> list[int]:
        return [
            index
            for index in range(start, end)
            if (
                _cc_cfg._instruction_mnemonic(candidate.instructions[index])
                == mnemonic
                and _cc_cfg._instruction_operand(
                    candidate.instructions[index]
                ).replace(" ", "").lower()
                == normalized_operand
                and encoded_body(index) == instruction_body
                and candidate_body_matches(index)
            )
        ]

    capacity_rows = exact_indices(
        "cmp",
        "eax,32",
        b"\x83\xf8\x20",
        start=count_load_index + 1,
        end=array_add_index,
    )
    seed_rows = exact_indices(
        "mov",
        "esi,eax",
        b"\x8b\xf0",
        start=count_load_index + 1,
        end=array_add_index,
    )
    if len(capacity_rows) != 1 or len(seed_rows) != 1:
        raise ValueError(
            "HUD track-counter slot bridge requires one exact capacity-32 "
            "guard and ESI index seed"
        )
    capacity_index = capacity_rows[0]
    seed_index = seed_rows[0]
    success_entry_rows = [
        index
        for index in range(capacity_index + 1, seed_index)
        if (
            _cc_cfg._instruction_mnemonic(candidate.instructions[index]) == "jl"
            and encoded_body(index) == b"\x7c\x07"
            and candidate_body_matches(index)
        )
    ]
    success_path_topology = (
        (0x00, "jl", b"\x7c\x07"),
        (0x02, "xor", b"\x33\xc0"),
        (0x04, "pop", b"\x5b"),
        (0x05, "add", bytes((0x83, 0xC4, frame_size))),
        (0x08, "ret", b"\xc3"),
        (0x09, "push", b"\x57"),
        (0x0A, "push", b"\x56"),
        (0x0B, "mov", b"\x8b\xf0"),
    )
    success_entry_offset = (
        candidate_offsets[success_entry_rows[0]]
        if len(success_entry_rows) == 1
        else None
    )
    connected_cleanup_rows = [
        index
        for index in range(capacity_index + 1, seed_index)
        if (
            _cc_cfg._instruction_mnemonic(candidate.instructions[index]) == "add"
            and _cc_cfg._instruction_operand(
                candidate.instructions[index]
            ).split(",", 1)[0].strip().lower()
            == "esp"
            and len(encoded_body(index)) == 3
            and encoded_body(index)[:2] == b"\x83\xc4"
            and candidate_body_matches(index)
        )
    ]
    if (
        len(success_entry_rows) != 1
        or len(connected_cleanup_rows) != 1
        or encoded_body(connected_cleanup_rows[0])[2] != frame_size
        or success_entry_rows[0] + len(success_path_topology) - 1
        != seed_index
        or any(
            success_entry_offset is None
            or candidate_offsets[success_entry_rows[0] + delta]
            != success_entry_offset + relative_offset
            or _cc_cfg._instruction_mnemonic(
                candidate.instructions[success_entry_rows[0] + delta]
            )
            != mnemonic
            or encoded_body(success_entry_rows[0] + delta)
            != instruction_body
            or not candidate_body_matches(success_entry_rows[0] + delta)
            for delta, (
                relative_offset,
                mnemonic,
                instruction_body,
            ) in enumerate(
                success_path_topology
            )
        )
        or success_entry_offset is None
    ):
        raise ValueError(
            "HUD track-counter slot bridge requires the exact capacity "
            "guard/early-return/success-path EAX-to-ESI seed topology"
        )
    affine_specs = (
        ("shl", "esi,3", b"\xc1\xe6\x03"),
        ("sub", "esi,eax", b"\x2b\xf0"),
        ("shl", "esi,6", b"\xc1\xe6\x06"),
    )
    affine_rows = [
        exact_indices(
            mnemonic,
            operand,
            instruction_body,
            start=seed_index + 1,
            end=array_add_index,
        )
        for mnemonic, operand, instruction_body in affine_specs
    ]
    if (
        not (
            count_load_index
            < capacity_index
            < seed_index
            < array_add_index
            < count_store_index
        )
        or any(len(rows) != 1 for rows in affine_rows)
        or not (
            seed_index
            < affine_rows[0][0]
            < affine_rows[1][0]
            < affine_rows[2][0]
            < array_add_index
        )
        or [
            index
            for index in range(seed_index + 1, array_add_index + 1)
            if _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index],
                "esi",
            )
        ]
        != [
            affine_rows[0][0],
            affine_rows[1][0],
            affine_rows[2][0],
            array_add_index,
        ]
    ):
        raise ValueError(
            "HUD track-counter slot bridge requires the unique ordered "
            "7*64=0x1c0 ESI affine element computation"
        )
    increment_rows = exact_indices(
        "inc",
        "eax",
        b"\x40",
        start=array_add_index + 1,
        end=count_store_index,
    )
    if len(increment_rows) != 1:
        raise ValueError(
            "HUD track-counter slot bridge requires one exact count increment "
            "before the relocated aggregate store"
        )

    candidate_chains: list[tuple[int, int, int, str]] = []
    for call_index, call_instruction in enumerate(candidate.instructions):
        if (
            call_index <= array_add_index
            or _cc_cfg._instruction_mnemonic(call_instruction) != "call"
        ):
            continue
        expression = _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(call_instruction)
        )
        call_match = re.fullmatch(
            r"(?P<register>eax|ebx|ecx|edx|esi|edi|ebp)"
            r"\+(?:12|0xc)",
            expression,
        )
        if call_match is None:
            continue
        vptr_register = call_match.group("register")
        vptr_rows = [
            index
            for index in range(array_add_index + 1, call_index)
            if (
                _cc_cfg._instruction_mnemonic(candidate.instructions[index])
                == "mov"
                and _cc_cfg._instruction_operand(
                    candidate.instructions[index]
                ).replace(" ", "").lower()
                in {
                    f"{vptr_register},dword[esi]",
                    f"{vptr_register},dwordptr[esi]",
                }
                and candidate_body_matches(index)
                and not any(
                    _cc_cfg._instruction_may_clobber_register(
                        candidate.instructions[between],
                        vptr_register,
                    )
                    for between in range(index + 1, call_index)
                )
            )
        ]
        if len(vptr_rows) != 1:
            continue
        vptr_index = vptr_rows[0]
        receiver_rows = exact_indices(
            "mov",
            "ecx,esi",
            b"\x8b\xce",
            start=vptr_index + 1,
            end=call_index,
        )
        argument_rows = [
            index
            for index in range(vptr_index + 1, call_index)
            if _cc_cfg._instruction_mnemonic(candidate.instructions[index])
            == "push"
        ]
        if (
            len(receiver_rows) != 1
            or len(argument_rows) != 2
            or any(
                _cc_cfg._instruction_may_clobber_register(
                    candidate.instructions[index],
                    "esi",
                )
                for index in range(array_add_index + 1, call_index)
            )
            or not candidate_body_matches(call_index)
            or encoded_body(call_index)
            != (
                b"\xff"
                + bytes(
                    [
                        {
                            "eax": 0x50,
                            "ebx": 0x53,
                            "ecx": 0x51,
                            "edx": 0x52,
                            "esi": 0x56,
                            "edi": 0x57,
                            "ebp": 0x55,
                        }[vptr_register],
                        0x0C,
                    ]
                )
            )
            or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
            or call_index in candidate.local_control_flow_indices
            or any(
                vptr_index <= target_index <= call_index
                for target_indices
                in candidate.local_control_flow_targets.values()
                for target_index in target_indices
            )
        ):
            continue
        candidate_chains.append(
            (
                vptr_index,
                receiver_rows[0],
                call_index,
                vptr_register,
            )
        )
    if len(candidate_chains) != 1:
        raise ValueError(
            "HUD track-counter slot bridge requires one unique connected "
            "candidate ESI-element/vptr/two-argument/slot-0x0c chain before "
            "the switch cluster"
        )
    (
        candidate_vptr_index,
        _candidate_receiver_index,
        candidate_call_index,
        candidate_vptr_register,
    ) = candidate_chains[0]
    candidate_call_offset = candidate_offsets[candidate_call_index]
    assert candidate_call_offset is not None
    if any(
        _cc_cfg._instruction_may_clobber_register(
            candidate.instructions[index],
            "esi",
        )
        for index in range(array_add_index + 1, candidate_vptr_index)
    ):
        raise ValueError(
            "HUD track-counter slot bridge rejects disconnected candidate "
            "ESI array-element provenance before the vptr load"
        )

    storage_identity = _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_STORAGE_IDENTITY
    return (
        {
            normalize_address(
                _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_RETAIL_CALL
            ): ReviewedLoopVptrStorageBridge(
                register="ebp",
                storage_identity=storage_identity,
                slot_displacement=(
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_SLOT_DISPLACEMENT
                ),
                assembly_source="bn",
            )
        },
        {
            normalize_address(
                hex(candidate_call_offset)
            ): ReviewedLoopVptrStorageBridge(
                register=candidate_vptr_register,
                storage_identity=storage_identity,
                slot_displacement=(
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_SLOT_DISPLACEMENT
                ),
                assembly_source="cod",
            )
        },
    )


def _hud_ui_mgr_sensor_track_counter_switch_vptr_bridges(
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedLoopVptrStorageBridge],
    dict[str, ReviewedLoopVptrStorageBridge],
]:
    """Prove only PlaceTrackCounterWidget's connected switch call cluster."""
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_CALLER_START:
        return {}, {}

    base_retail, base_candidate = (
        _hud_ui_mgr_sensor_track_counter_slot_vptr_bridges(
            retail_instructions,
            candidate,
            document=document,
            caller_identity=caller_identity,
            caller_start=normalized_start,
            caller_end_exclusive=caller_end_exclusive,
            indexes=indexes,
        )
    )
    if (
        base_retail
        != {
            "0x4120ea": ReviewedLoopVptrStorageBridge(
                register="ebp",
                storage_identity=(
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_STORAGE_IDENTITY
                ),
                slot_displacement=0x0C,
                assembly_source="bn",
            )
        }
        or len(base_candidate) != 1
        or next(iter(base_candidate.values()))
        != ReviewedLoopVptrStorageBridge(
            register=next(iter(base_candidate.values())).register,
            storage_identity=(
                _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_STORAGE_IDENTITY
            ),
            slot_displacement=0x0C,
            assembly_source="cod",
        )
    ):
        raise ValueError(
            "HUD track-counter switch bridge requires WSI-010's exact "
            "structural caller/source/aggregate/array/COFF authority"
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
    retail_index_by_address = {
        address: index
        for index, address in enumerate(retail_addresses)
        if address is not None and retail_counts.get(address) == 1
    }
    candidate_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    caller = candidate.caller_definition
    assert caller is not None

    def body(instruction: Instruction) -> bytes:
        return bytes(int(value, 16) for value in instruction.bytes)

    def require_retail_row(
        address: int,
        mnemonic: str,
        expected_body: bytes,
    ) -> Instruction:
        instruction = retail_by_address.get(address)
        if (
            instruction is None
            or _cc_cfg._instruction_mnemonic(instruction) != mnemonic
            or body(instruction) != expected_body
        ):
            raise ValueError(
                "HUD track-counter switch bridge requires exact retail "
                f"instruction {normalize_address(address)}"
            )
        return instruction

    require_retail_row(
        0x4120ED,
        "cmp",
        b"\x83\xfb\x08",
    )
    require_retail_row(
        0x4120F0,
        "ja",
        b"\x0f\x87\x81\x01\x00\x00",
    )
    require_retail_row(
        0x4120F6,
        "jmp",
        b"\xff\x24\x9d\x94\x22\x41\x00",
    )
    for address, mnemonic, expected_body in (
        (0x412105, "jmp", b"\xe9\x6d\x01\x00\x00"),
        (0x41216B, "jmp", b"\xe9\x07\x01\x00\x00"),
        (0x4121D8, "jmp", b"\xe9\x95\x00\x00\x00"),
        (0x41221E, "jmp", b"\xeb\x52"),
    ):
        require_retail_row(address, mnemonic, expected_body)

    def candidate_body_matches(
        index: int,
        allowed_relocations: Sequence[CoffRelocation] = (),
    ) -> bool:
        offset = candidate_offsets[index]
        if offset is None:
            return False
        try:
            instruction_body = body(candidate.instructions[index])
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
            offset + len(instruction_body) <= len(caller.data)
            and caller.data[
                offset : offset + len(instruction_body)
            ]
            == instruction_body
            and {
                position
                for position in range(
                    offset,
                    offset + len(instruction_body),
                )
                if caller.relocation_mask[position]
            }
            == allowed_mask
        )

    if (
        len(candidate.local_control_flow_indices) != 1
        or set(candidate.local_control_flow_targets)
        != set(candidate.local_control_flow_indices)
    ):
        raise ValueError(
            "HUD track-counter switch bridge requires one unique candidate "
            "nine-entry local switch dispatch and no additional dispatch"
        )
    dispatch_index = next(iter(candidate.local_control_flow_indices))
    dispatch_offset = candidate_offsets[dispatch_index]
    dispatch_targets = candidate.local_control_flow_targets.get(
        dispatch_index,
        (),
    )
    if (
        dispatch_offset is None
        or len(dispatch_targets) != 9
        or any(
            target < 0 or target >= len(candidate.instructions)
            for target in dispatch_targets
        )
    ):
        raise ValueError(
            "HUD track-counter switch bridge requires one complete candidate "
            "nine-entry switch target map"
        )
    (
        case_0_index,
        case_1_index,
        case_2_index,
        default_index,
        case_4_index,
        default_5_index,
        default_6_index,
        default_7_index,
        case_8_index,
    ) = dispatch_targets
    if (
        default_5_index != default_index
        or default_6_index != default_index
        or default_7_index != default_index
        or len(set(dispatch_targets)) != 6
        or not (
            dispatch_index
            < case_0_index
            < case_1_index
            < case_2_index
            < case_4_index
            < case_8_index
            < default_index
        )
    ):
        raise ValueError(
            "HUD track-counter switch bridge requires exact case "
            "0/1/2/default/4/default/default/default/8 reachability"
        )

    base_candidate_offset = int(next(iter(base_candidate)), 16)
    base_call_rows = [
        index
        for index, offset in enumerate(candidate_offsets)
        if offset == base_candidate_offset
    ]
    if len(base_call_rows) != 1:
        raise ValueError(
            "HUD track-counter switch bridge requires the unique WSI-010 "
            "base call instruction"
        )
    base_call_index = base_call_rows[0]
    dispatch_instruction = candidate.instructions[dispatch_index]
    dispatch_body = body(dispatch_instruction)
    register_by_code = {
        0: "eax",
        1: "ecx",
        2: "edx",
        3: "ebx",
        5: "ebp",
        6: "esi",
        7: "edi",
    }
    dispatch_register = (
        register_by_code.get((dispatch_body[2] >> 3) & 7)
        if (
            len(dispatch_body) == 7
            and dispatch_body[:2] == b"\xff\x24"
            and dispatch_body[2] & 0xC7 == 0x85
            and dispatch_body[3:] == b"\0\0\0\0"
        )
        else None
    )
    dispatch_relocations = [
        relocation
        for relocation in caller.relocations
        if (
            dispatch_offset + 3
            <= relocation.offset
            < dispatch_offset + len(dispatch_body)
        )
    ]
    if (
        dispatch_register is None
        or _cc_cfg._instruction_mnemonic(dispatch_instruction) != "jmp"
        or len(dispatch_relocations) != 1
        or dispatch_relocations[0].offset != dispatch_offset + 3
        or dispatch_relocations[0].type != IMAGE_REL_I386_DIR32
        or not re.fullmatch(
            r"\$L[0-9A-Za-z_]+",
            dispatch_relocations[0].symbol_name,
        )
        or struct.unpack_from(
            "<I",
            caller.data,
            dispatch_relocations[0].offset,
        )[0]
        != 0
        or not candidate_body_matches(
            dispatch_index,
            dispatch_relocations,
        )
    ):
        raise ValueError(
            "HUD track-counter switch bridge requires one structurally owned "
            "and masked candidate DIR32 scaled-index jump-table relocation"
        )

    register_codes = {
        "eax": 0,
        "ecx": 1,
        "edx": 2,
        "ebx": 3,
        "ebp": 5,
        "esi": 6,
        "edi": 7,
    }
    cmp_body = bytes(
        (0x83, 0xF8 + register_codes[dispatch_register], 0x08)
    )
    cmp_rows = [
        index
        for index in range(base_call_index + 1, dispatch_index)
        if (
            _cc_cfg._instruction_mnemonic(candidate.instructions[index]) == "cmp"
            and _cc_cfg._instruction_operand(
                candidate.instructions[index]
            ).replace(" ", "").lower()
            == f"{dispatch_register},8"
            and body(candidate.instructions[index]) == cmp_body
            and candidate_body_matches(index)
        )
    ]
    default_offset = candidate_offsets[default_index]
    ja_rows = [
        index
        for index in range(base_call_index + 1, dispatch_index)
        if (
            _cc_cfg._instruction_mnemonic(candidate.instructions[index]) == "ja"
            and len(body(candidate.instructions[index])) == 6
            and body(candidate.instructions[index])[:2] == b"\x0f\x87"
            and candidate_body_matches(index)
            and candidate_offsets[index] is not None
            and default_offset is not None
            and candidate_offsets[index]
            + 6
            + struct.unpack(
                "<i",
                body(candidate.instructions[index])[2:],
            )[0]
            == default_offset
        )
    ]
    if (
        len(cmp_rows) != 1
        or len(ja_rows) != 1
        or not (
            base_call_index
            < cmp_rows[0]
            < ja_rows[0]
            < dispatch_index
        )
        or any(
            _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index],
                dispatch_register,
            )
            for index in range(cmp_rows[0] + 1, dispatch_index)
        )
    ):
        raise ValueError(
            "HUD track-counter switch bridge requires one connected "
            "candidate index<=8 guard and default edge into the dispatch"
        )

    def direct_jump_target(index: int) -> int | None:
        instruction_body = body(candidate.instructions[index])
        offset = candidate_offsets[index]
        if (
            offset is None
            or _cc_cfg._instruction_mnemonic(candidate.instructions[index]) != "jmp"
            or not candidate_body_matches(index)
        ):
            return None
        if len(instruction_body) == 2 and instruction_body[0] == 0xEB:
            return (
                offset
                + 2
                + struct.unpack("<b", instruction_body[1:])[0]
            )
        if len(instruction_body) == 5 and instruction_body[0] == 0xE9:
            return (
                offset
                + 5
                + struct.unpack("<i", instruction_body[1:])[0]
            )
        return None

    case_tail_rows: list[int] = []
    case_tail_targets: list[int] = []
    for start_index, end_index in (
        (case_0_index, case_1_index),
        (case_1_index, case_2_index),
        (case_2_index, case_4_index),
        (case_4_index, case_8_index),
    ):
        tails = [
            (index, direct_jump_target(index))
            for index in range(start_index, end_index)
            if direct_jump_target(index) is not None
        ]
        if len(tails) != 1:
            raise ValueError(
                "HUD track-counter switch bridge requires one exact "
                "case-0/1/2/4 direct tail edge"
            )
        assert tails[0][1] is not None
        case_tail_rows.append(tails[0][0])
        case_tail_targets.append(tails[0][1])
    (
        case_0_tail_target,
        case_1_tail_target,
        case_2_tail_target,
        case_4_tail_target,
    ) = case_tail_targets
    (
        _case_0_tail_index,
        _case_1_tail_index,
        case_2_tail_index,
        _case_4_tail_index,
    ) = case_tail_rows
    candidate_index_by_unique_offset = {
        offset: index
        for index, offset in enumerate(candidate_offsets)
        if (
            offset is not None
            and candidate_offsets.count(offset) == 1
        )
    }
    shared_case_tail_index = (
        candidate_index_by_unique_offset.get(case_2_tail_target)
        if case_2_tail_target != default_offset
        else None
    )
    if (
        case_0_tail_target != default_offset
        or case_1_tail_target != default_offset
        or (
            not (
                case_2_tail_target == default_offset
                and case_4_tail_target == default_offset
            )
            and not (
                case_2_tail_target == case_4_tail_target
                and case_2_tail_target != default_offset
                and shared_case_tail_index is not None
                and (
                    case_8_index
                    <= shared_case_tail_index
                    < default_index
                )
            )
        )
    ):
        raise ValueError(
            "HUD track-counter switch bridge requires exact case-0/1 "
            "default tails and case-2/case-4 per-case or equal case-8 "
            "shared tails"
        )

    slot_storage = _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_STORAGE_IDENTITY
    widget_storage = (
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_WIDGET_STORAGE_IDENTITY
    )
    retail_specs = (
        # call, vptr load, receiver, register, slot, storage, load, receiver
        (0x412114, 0x41210A, 0x412112, "eax", 0x60,
         widget_storage, b"\x8b\x46\x48", b"\x8b\xcf"),
        (0x412129, 0x412125, 0x412127, "edx", 0x68,
         slot_storage, b"\x8b\x16", b"\x8b\xce"),
        (0x412168, 0x412161, 0x412166, "edx", 0x0C,
         widget_storage, b"\x8b\x17", b"\x8b\xcf"),
        (0x41217A, 0x412170, 0x412178, "eax", 0x60,
         widget_storage, b"\x8b\x46\x48", b"\x8b\xcf"),
        (0x41218F, 0x41218B, 0x41218D, "edx", 0x68,
         slot_storage, b"\x8b\x16", b"\x8b\xce"),
        (0x4121C7, 0x4121C0, 0x4121C5, "edx", 0x64,
         slot_storage, b"\x8b\x16", b"\x8b\xce"),
        (0x4121E7, 0x4121DD, 0x4121E5, "eax", 0x60,
         widget_storage, b"\x8b\x46\x48", b"\x8b\xcf"),
        (0x412204, 0x4121F8, 0x412202, "edx", 0x68,
         slot_storage, b"\x8b\x16", b"\x8b\xce"),
        (0x41220D, 0x41220B, 0x412208, "eax", 0x64,
         slot_storage, b"\x8b\x06", b"\x8b\xce"),
        (0x41222A, 0x412220, 0x412228, "edx", 0x60,
         widget_storage, b"\x8b\x56\x48", b"\x8b\xcf"),
        (0x41223E, 0x41223A, 0x41223C, "edx", 0x64,
         slot_storage, b"\x8b\x16", b"\x8b\xce"),
        (0x412247, 0x412243, 0x412245, "eax", 0x68,
         slot_storage, b"\x8b\x06", b"\x8b\xce"),
        (0x412274, 0x41225D, 0x412272, "ebp", 0x0C,
         widget_storage, b"\x8b\x2f", b"\x8b\xcf"),
    )
    retail_bridges: dict[str, ReviewedLoopVptrStorageBridge] = {}
    for (
        call_address,
        vptr_address,
        receiver_address,
        register,
        slot,
        storage,
        vptr_body,
        receiver_body,
    ) in retail_specs:
        vptr = require_retail_row(vptr_address, "mov", vptr_body)
        receiver = require_retail_row(
            receiver_address,
            "mov",
            receiver_body,
        )
        call_body = (
            b"\xff"
            + bytes(
                [
                    {
                        "eax": 0x50,
                        "edx": 0x52,
                        "ebp": 0x55,
                    }[register],
                    slot,
                ]
            )
        )
        call = require_retail_row(call_address, "call", call_body)
        call_index = retail_index_by_address[call_address]
        vptr_index = retail_index_by_address[vptr_address]
        if (
            _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(call))
            not in {
                f"{register}+{slot}",
                f"{register}+0x{slot:x}",
            }
            or _cc_cfg._instruction_operand(receiver).replace(" ", "").lower()
            not in {"ecx,esi", "ecx,edi"}
            or any(
                _cc_cfg._instruction_may_clobber_register(
                    retail_instructions[index],
                    register,
                )
                for index in range(vptr_index + 1, call_index)
            )
            or _cc_cfg._cleanup_after(retail_instructions, call_index) is not None
        ):
            raise ValueError(
                "HUD track-counter switch bridge rejects retail vptr/"
                "receiver/slot/reaching-definition/cleanup drift at "
                f"{normalize_address(call_address)}"
            )
        retail_bridges[normalize_address(call_address)] = (
            ReviewedLoopVptrStorageBridge(
                register=register,
                storage_identity=storage,
                slot_displacement=slot,
                assembly_source="bn",
            )
        )

    # The retail case-2/4/8 SetPos tail has three independently exact
    # counter-widget vptr definitions which converge on the one 0x412274 call.
    for vptr_address, tail_address, tail_body in (
        (0x4121C2, 0x4121D8, b"\xe9\x95\x00\x00\x00"),
        (0x412200, 0x41221E, b"\xeb\x52"),
        (0x41225D, 0x412274, b"\xff\x55\x0c"),
    ):
        require_retail_row(vptr_address, "mov", b"\x8b\x2f")
        tail_mnemonic = "call" if tail_address == 0x412274 else "jmp"
        require_retail_row(tail_address, tail_mnemonic, tail_body)
        start_index = retail_index_by_address[vptr_address]
        end_index = retail_index_by_address[tail_address]
        if any(
            _cc_cfg._instruction_may_clobber_register(
                retail_instructions[index],
                register,
            )
            for index in range(start_index + 1, end_index)
            for register in ("ebp", "edi")
        ):
            raise ValueError(
                "HUD track-counter switch bridge rejects a retail shared "
                "SetPos EBP/EDI reaching path"
            )

    if any(
        _cc_cfg._instruction_may_clobber_register(
            candidate.instructions[index],
            "esi",
        )
        for index in range(base_call_index + 1, default_index)
    ):
        raise ValueError(
            "HUD track-counter switch bridge rejects candidate weapon-slot "
            "ESI provenance drift through the switch cluster"
        )

    case_2_specs = (
        ("indirect", widget_storage, 0x60),
        ("direct", "", 0),
        ("indirect", slot_storage, 0x68),
        ("indirect", slot_storage, 0x64),
    )
    if shared_case_tail_index is None:
        case_2_specs = (
            *case_2_specs,
            ("indirect", widget_storage, 0x0C),
        )
    candidate_case_specs = (
        (
            "case-0",
            case_0_index,
            case_1_index,
            (),
        ),
        (
            "case-1",
            case_1_index,
            case_2_index,
            (
                ("indirect", widget_storage, 0x60),
                ("direct", "", 0),
                ("indirect", slot_storage, 0x68),
                ("indirect", widget_storage, 0x0C),
            ),
        ),
        (
            "case-2",
            case_2_index,
            case_4_index,
            case_2_specs,
        ),
        (
            "case-4",
            case_4_index,
            case_8_index,
            (
                ("indirect", widget_storage, 0x60),
                ("direct", "", 0),
                ("indirect", slot_storage, 0x68),
                ("indirect", slot_storage, 0x64),
                *(
                    ()
                    if shared_case_tail_index is not None
                    else (("indirect", widget_storage, 0x0C),)
                ),
            ),
        ),
        (
            "case-8",
            case_8_index,
            default_index,
            (
                ("indirect", widget_storage, 0x60),
                ("direct", "", 0),
                ("indirect", slot_storage, 0x64),
                ("indirect", slot_storage, 0x68),
                ("indirect", widget_storage, 0x0C),
            ),
        ),
    )
    call_modrm_by_register = {
        "eax": 0x50,
        "ecx": 0x51,
        "edx": 0x52,
        "ebx": 0x53,
        "ebp": 0x55,
        "esi": 0x56,
        "edi": 0x57,
    }

    def candidate_indirect_call_bridge(
        call_index: int,
        *,
        case_start_index: int,
        expected_storage: str,
        expected_slot: int,
    ) -> ReviewedLoopVptrStorageBridge:
        instruction = candidate.instructions[call_index]
        expression = _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(instruction)
        )
        call_match = re.fullmatch(
            r"(?P<register>eax|ebx|ecx|edx|esi|edi|ebp)"
            r"\+(?P<slot>12|0xc|96|0x60|100|0x64|104|0x68)",
            expression,
        )
        if call_match is None:
            raise ValueError(
                "HUD track-counter switch bridge requires an exact "
                "candidate virtual slot call"
            )
        vptr_register = call_match.group("register")
        slot = int(call_match.group("slot"), 0)
        expected_body = bytes(
            (
                0xFF,
                call_modrm_by_register[vptr_register],
                slot,
            )
        )
        if (
            slot != expected_slot
            or body(instruction) != expected_body
            or not candidate_body_matches(call_index)
            or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
            or call_index in candidate.local_control_flow_indices
        ):
            raise ValueError(
                "HUD track-counter switch bridge rejects candidate virtual "
                "slot, body, cleanup, or dispatch drift"
            )

        vptr_rows = [
            index
            for index in range(case_start_index, call_index)
            if (
                _cc_cfg._instruction_mnemonic(candidate.instructions[index])
                == "mov"
                and _cc_cfg._instruction_operand(
                    candidate.instructions[index]
                ).split(",", 1)[0].strip().lower()
                == vptr_register
                and candidate_body_matches(index)
                and not any(
                    _cc_cfg._instruction_may_clobber_register(
                        candidate.instructions[between],
                        vptr_register,
                    )
                    for between in range(index + 1, call_index)
                )
            )
        ]
        receiver_rows = [
            index
            for index in range(case_start_index, call_index)
            if (
                _cc_cfg._instruction_mnemonic(candidate.instructions[index])
                == "mov"
                and _cc_cfg._instruction_operand(
                    candidate.instructions[index]
                ).split(",", 1)[0].strip().lower()
                == "ecx"
                and candidate_body_matches(index)
                and not any(
                    _cc_cfg._instruction_may_clobber_register(
                        candidate.instructions[between],
                        "ecx",
                    )
                    for between in range(index + 1, call_index)
                )
            )
        ]
        if len(vptr_rows) != 1 or len(receiver_rows) != 1:
            raise ValueError(
                "HUD track-counter switch bridge requires unique connected "
                "candidate vptr and receiver reaching definitions"
            )
        vptr_operands = _cc_cfg._instruction_operand(
            candidate.instructions[vptr_rows[0]]
        ).split(",", 1)
        receiver_operands = _cc_cfg._instruction_operand(
            candidate.instructions[receiver_rows[0]]
        ).split(",", 1)
        if len(vptr_operands) != 2 or len(receiver_operands) != 2:
            raise ValueError(
                "HUD track-counter switch bridge requires exact candidate "
                "vptr and receiver operands"
            )
        vptr_source = _cc_targets._exact_memory_expression(vptr_operands[1])
        receiver_register = receiver_operands[1].strip().lower()
        if expected_storage == slot_storage:
            storage_valid = (
                receiver_register == "esi"
                and vptr_source == "esi"
            )
        else:
            widget_lea_rows = [
                index
                for index in range(base_call_index + 1, call_index)
                if (
                    _cc_cfg._instruction_mnemonic(candidate.instructions[index])
                    == "lea"
                    and _cc_cfg._instruction_operand(
                        candidate.instructions[index]
                    ).split(",", 1)[0].strip().lower()
                    == receiver_register
                    and len(
                        _cc_cfg._instruction_operand(
                            candidate.instructions[index]
                        ).split(",", 1)
                    )
                    == 2
                    and _cc_targets._exact_memory_expression(
                        _cc_cfg._instruction_operand(
                            candidate.instructions[index]
                        ).split(",", 1)[1]
                    )
                    in {"esi+72", "esi+0x48"}
                    and candidate_body_matches(index)
                    and not any(
                        _cc_cfg._instruction_may_clobber_register(
                            candidate.instructions[between],
                            receiver_register,
                        )
                        for between in range(index + 1, call_index)
                    )
                )
            ]
            storage_valid = (
                receiver_register != "esi"
                and len(widget_lea_rows) == 1
                and vptr_source
                in {
                    receiver_register,
                    "esi+72",
                    "esi+0x48",
                }
            )
        if not storage_valid:
            raise ValueError(
                "HUD track-counter switch bridge rejects candidate "
                "weapon-slot versus embedded +0x48 widget storage drift"
            )
        return ReviewedLoopVptrStorageBridge(
            register=vptr_register,
            storage_identity=expected_storage,
            slot_displacement=expected_slot,
            assembly_source="cod",
        )

    candidate_bridges: dict[str, ReviewedLoopVptrStorageBridge] = {}
    candidate_direct_symbols: list[str] = []
    structurally_reviewed_call_indices: list[int] = []
    for (
        case_name,
        case_start_index,
        case_end_index,
        case_specs,
    ) in candidate_case_specs:
        call_indices = [
            index
            for index in range(case_start_index, case_end_index)
            if _cc_cfg._instruction_mnemonic(candidate.instructions[index]) == "call"
        ]
        if len(call_indices) != len(case_specs):
            raise ValueError(
                "HUD track-counter switch bridge rejects physical call "
                f"population/order drift in candidate {case_name}"
            )
        structurally_reviewed_call_indices.extend(call_indices)
        for call_index, (form, storage, slot) in zip(
            call_indices,
            case_specs,
        ):
            call_offset = candidate_offsets[call_index]
            assert call_offset is not None
            if form == "direct":
                instruction_body = body(candidate.instructions[call_index])
                direct_relocations = [
                    relocation
                    for relocation in caller.relocations
                    if (
                        call_offset
                        <= relocation.offset
                        < call_offset + len(instruction_body)
                    )
                ]
                if (
                    len(instruction_body) != 5
                    or instruction_body != b"\xe8\0\0\0\0"
                    or len(direct_relocations) != 1
                    or direct_relocations[0].offset != call_offset + 1
                    or direct_relocations[0].type != IMAGE_REL_I386_REL32
                    or struct.unpack_from(
                        "<I",
                        caller.data,
                        direct_relocations[0].offset,
                    )[0]
                    != 0
                    or not candidate_body_matches(
                        call_index,
                        direct_relocations,
                    )
                    or _cc_cfg._cleanup_after(
                        candidate.instructions,
                        call_index,
                    )
                    is not None
                ):
                    raise ValueError(
                        "HUD track-counter switch bridge requires one exact "
                        f"REL32 direct image call in candidate {case_name}"
                    )
                candidate_direct_symbols.append(
                    direct_relocations[0].symbol_name
                )
                continue
            bridge = candidate_indirect_call_bridge(
                call_index,
                case_start_index=case_start_index,
                expected_storage=storage,
                expected_slot=slot,
            )
            candidate_bridges[normalize_address(hex(call_offset))] = bridge

    if shared_case_tail_index is not None:
        case_8_call_indices = [
            index
            for index in range(case_8_index, default_index)
            if _cc_cfg._instruction_mnemonic(candidate.instructions[index]) == "call"
        ]
        shared_call_index = case_8_call_indices[-1]
        shared_call_offset = candidate_offsets[shared_call_index]
        assert shared_call_offset is not None
        shared_bridge = candidate_bridges.get(
            normalize_address(hex(shared_call_offset))
        )
        shared_call_match = re.fullmatch(
            r"(?P<register>eax|ebx|ecx|edx|esi|edi|ebp)"
            r"\+(?:12|0xc)",
            _cc_targets._exact_memory_expression(
                _cc_cfg._instruction_operand(
                    candidate.instructions[shared_call_index]
                )
            ),
        )
        shared_receiver_operands = _cc_cfg._instruction_operand(
            candidate.instructions[shared_case_tail_index]
        ).split(",", 1)
        shared_vptr_register = (
            shared_call_match.group("register")
            if shared_call_match is not None
            else ""
        )
        shared_receiver_register = (
            shared_receiver_operands[1].strip().lower()
            if len(shared_receiver_operands) == 2
            else ""
        )
        if (
            shared_bridge
            != ReviewedLoopVptrStorageBridge(
                register=shared_vptr_register,
                storage_identity=widget_storage,
                slot_displacement=0x0C,
                assembly_source="cod",
            )
            or shared_case_tail_index + 1 != shared_call_index
            or _cc_cfg._instruction_mnemonic(
                candidate.instructions[shared_case_tail_index]
            )
            != "mov"
            or len(shared_receiver_operands) != 2
            or shared_receiver_operands[0].strip().lower() != "ecx"
        ):
            raise ValueError(
                "HUD track-counter switch bridge requires the exact "
                "case-2/case-4/case-8 shared counter-widget SetPos suffix"
            )

        for (
            path_name,
            path_start_index,
            path_end_index,
        ) in (
            ("case-2", case_2_index, case_2_tail_index),
            ("case-4", case_4_index, _case_4_tail_index),
            ("case-8", case_8_index, shared_call_index),
        ):
            path_vptr_rows = [
                index
                for index in range(path_start_index, path_end_index)
                if (
                    _cc_cfg._instruction_mnemonic(candidate.instructions[index])
                    == "mov"
                    and _cc_cfg._instruction_operand(
                        candidate.instructions[index]
                    ).split(",", 1)[0].strip().lower()
                    == shared_vptr_register
                    and candidate_body_matches(index)
                    and not any(
                        _cc_cfg._instruction_may_clobber_register(
                            candidate.instructions[between],
                            shared_vptr_register,
                        )
                        for between in range(index + 1, path_end_index)
                    )
                )
            ]
            path_widget_lea_rows = [
                index
                for index in range(path_start_index, path_end_index)
                if (
                    _cc_cfg._instruction_mnemonic(candidate.instructions[index])
                    == "lea"
                    and _cc_cfg._instruction_operand(
                        candidate.instructions[index]
                    ).split(",", 1)[0].strip().lower()
                    == shared_receiver_register
                    and len(
                        _cc_cfg._instruction_operand(
                            candidate.instructions[index]
                        ).split(",", 1)
                    )
                    == 2
                    and _cc_targets._exact_memory_expression(
                        _cc_cfg._instruction_operand(
                            candidate.instructions[index]
                        ).split(",", 1)[1]
                    )
                    in {"esi+72", "esi+0x48"}
                    and candidate_body_matches(index)
                    and not any(
                        _cc_cfg._instruction_may_clobber_register(
                            candidate.instructions[between],
                            shared_receiver_register,
                        )
                        for between in range(index + 1, path_end_index)
                    )
                )
            ]
            path_vptr_source = (
                _cc_targets._exact_memory_expression(
                    _cc_cfg._instruction_operand(
                        candidate.instructions[path_vptr_rows[0]]
                    ).split(",", 1)[1]
                )
                if (
                    len(path_vptr_rows) == 1
                    and len(
                        _cc_cfg._instruction_operand(
                            candidate.instructions[path_vptr_rows[0]]
                        ).split(",", 1)
                    )
                    == 2
                )
                else ""
            )
            if (
                len(path_vptr_rows) != 1
                or len(path_widget_lea_rows) != 1
                or path_vptr_source != shared_receiver_register
                or sum(
                    1
                    for index in range(
                        path_vptr_rows[0] + 1,
                        path_end_index,
                    )
                    if _cc_cfg._instruction_mnemonic(
                        candidate.instructions[index]
                    )
                    == "push"
                )
                != 2
            ):
                raise ValueError(
                    "HUD track-counter switch bridge requires exact shared "
                    f"vptr/receiver/two-argument provenance on {path_name}"
                )

    if (
        len(set(candidate_direct_symbols)) != 1
        or caller.undefined_external_functions.count(
            candidate_direct_symbols[0]
        )
        != 1
        or candidate_direct_symbols[0] in caller.defined_external_functions
    ):
        raise ValueError(
            "HUD track-counter switch bridge requires one consistent "
            "undefined direct image-call identity across cases 1/2/4/8"
        )

    retail_call_addresses = tuple(
        address
        for instruction, address in zip(
            retail_instructions,
            retail_addresses,
        )
        if (
            address is not None
            and 0x4120F6 < address <= 0x412274
            and _cc_cfg._instruction_mnemonic(instruction) == "call"
        )
    )
    if retail_call_addresses != (
        0x412114, 0x412120, 0x412129, 0x412168,
        0x41217A, 0x412186, 0x41218F, 0x4121C7,
        0x4121E7, 0x4121F3, 0x412204, 0x41220D,
        0x41222A, 0x412235, 0x41223E, 0x412247,
        0x412274,
    ) or structurally_reviewed_call_indices != [
        index
        for index in range(case_0_index, default_index)
        if _cc_cfg._instruction_mnemonic(candidate.instructions[index]) == "call"
    ]:
        raise ValueError(
            "HUD track-counter switch bridge rejects physical call count, "
            "order, branch-tail merge, or direct/indirect population drift"
        )

    return retail_bridges, candidate_bridges


def _hud_ui_mgr_sensor_track_marker_loop_candidate_vptr_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedLoopVptrStorageBridge]:
    """Prove PlaceTrackMarker's two loop-cursor center virtual calls."""
    from _recoil.call_contract.records import (
        ReviewedLoopVptrStorageBridge,
        StorageContainer,
    )
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_CALLER_START:
        return {}

    normalized_end = normalize_address(caller_end_exclusive)
    caller_symbol_id = (
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_CALLER_IDENTITY.removeprefix(
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
    caller_addresses = sorted(
        address
        for address, identity in indexes.by_address.items()
        if identity == caller_identity
    )
    if (
        caller_identity
        != _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_CALLER_IDENTITY
        or normalized_end
        != _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_CALLER_END_EXCLUSIVE
        or caller_addresses
        != [_cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_CALLER_START]
        or caller_identity in indexes.provider_ids
        or not isinstance(caller_row, Mapping)
        or caller_row.get("binary") != "recoil"
        or caller_row.get("kind") != "function"
        or caller_row.get("pipeline_class") != "authored"
        or caller_row.get("ownership_state") != "primary-owned"
        or normalize_address(str(caller_row.get("address", "")))
        != _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_CALLER_START
        or normalize_address(
            str(caller_row.get("end_exclusive", ""))
        )
        != _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_CALLER_END_EXCLUSIVE
        or caller_row.get("extent_state") != "known"
        or caller_row.get("size") != 0x1F0
        or caller_row.get("navigation_name")
        != "HudUiMgrSensor::PlaceTrackMarker"
        or caller_row.get("output_section_id")
        != "recoil:section:.text"
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
        != _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
        or caller is None
        or caller.symbol
        != _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_CALLER_SYMBOL
        or len(caller.data) != 0x1D0
        or len(caller.relocation_mask) != len(caller.data)
    ):
        raise ValueError(
            "HUD track-marker loop bridge requires the exact unaliased "
            "authored caller, extent, source edge, and candidate symbol"
        )

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
        == _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_CALLER_START
    ]
    if (
        target is None
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
            "HUD track-marker loop bridge requires one exact current HUD "
            "target authority"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != r"\?PlaceTrackMarker@HudUiMgrSensor@@.*"
        or re.fullmatch(
            r"\?PlaceTrackMarker@HudUiMgrSensor@@.*",
            caller.symbol,
        )
        is None
        or getattr(contribution_row, "name", "")
        != "HudUiMgrSensor::PlaceTrackMarker"
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
            "HUD track-marker loop bridge requires the exact authored "
            "hud.cpp contribution row"
        )

    symbols = document.collection("symbols")
    storages = document.collection("storage_contributions")
    aggregate_symbol = symbols.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID)
    aggregate_storage = storages.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_STORAGE_ID)
    aggregate_target = document.collection("verification_targets").get(
        _cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_ID
    )
    aggregate_registration = (
        aggregate_target.get("registration")
        if isinstance(aggregate_target, Mapping)
        else None
    )
    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    aggregate_start = address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
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
        or aggregate_storage.get("symbol_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID]
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
        or indexes.storage_by_address.get(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
        )
        != aggregate_identity
        or [
            row
            for row in indexes.storage_containers
            if row.identity == aggregate_identity
        ]
        != [
            StorageContainer(
                aggregate_start,
                aggregate_start + _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE,
                aggregate_identity,
            )
        ]
    ):
        raise ValueError(
            "HUD track-marker loop bridge requires the exact reviewed "
            "aggregate symbol, storage, target, and positive extent"
        )

    expected_prefix = tuple(expected[:2])
    retail_storage_identity = (
        str(expected_prefix[0].get("storage_identity", ""))
        if len(expected_prefix) == 2
        else ""
    )
    if (
        len(expected) < 2
        or not retail_storage_identity
        or any(
            row
            != {
                "ordinal": ordinal,
                "form": "call",
                "dispatch": "indirect",
                "identity_kind": "virtual-slot",
                "target_identity": "",
                "storage_identity": retail_storage_identity,
                "slot_displacement": slot,
                "cleanup_bytes": None,
            }
            for row, (ordinal, _, _, _, slot) in zip(
                expected_prefix,
                _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_CALL_SPECS,
            )
        )
    ):
        raise ValueError(
            "HUD track-marker loop bridge requires exact immutable retail "
            "ordinal-0/1 slot-0x64/0x68 contracts"
        )

    aggregate = _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
    base_relocations = (
        (0x04, _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_COUNT_DISPLACEMENT),
        (0x18, _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_ARRAY_DISPLACEMENT),
        (0x1D, _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_COUNTER_ARRAY_DISPLACEMENT),
    )
    if (
        caller.undefined_external_data.count(aggregate) != 1
        or aggregate in caller.defined_external_data
    ):
        raise ValueError(
            "HUD track-marker loop bridge requires one undefined aggregate "
            "data authority"
        )
    for relocation_offset, addend in base_relocations:
        rows = [
            row
            for row in caller.relocations
            if row.offset == relocation_offset
        ]
        if (
            len(rows) != 1
            or rows[0].type != IMAGE_REL_I386_DIR32
            or rows[0].symbol_name != aggregate
            or struct.unpack_from(
                "<I",
                caller.data,
                relocation_offset,
            )[0]
            != addend
            or not all(
                caller.relocation_mask[index]
                for index in range(
                    relocation_offset,
                    relocation_offset + 4,
                )
            )
        ):
            raise ValueError(
                "HUD track-marker loop bridge requires exact relocation-"
                f"backed aggregate addend 0x{addend:x} at "
                f"+0x{relocation_offset:x}"
            )
    bounded_base_relocations = [
        (row.offset, row.symbol_name, row.type)
        for row in caller.relocations
        if 0x03 <= row.offset < 0x21
    ]
    if bounded_base_relocations != [
        (offset, aggregate, IMAGE_REL_I386_DIR32)
        for offset, _ in base_relocations
    ] or {
        offset
        for offset in range(0x03, 0x21)
        if caller.relocation_mask[offset]
    } != {
        index
        for offset, _ in base_relocations
        for index in range(offset, offset + 4)
    }:
        raise ValueError(
            "HUD track-marker loop bridge rejects missing, extra, "
            "reordered, or ambiguously masked base relocations"
        )

    instruction_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(
        candidate
    )
    address_rows: dict[int, list[int]] = {}
    for index, offset in enumerate(instruction_offsets):
        if offset is not None:
            address_rows.setdefault(offset, []).append(index)
    fixed_offsets = (
        0x03,
        0x0A,
        0x0C,
        0x0F,
        0x12,
        0x16,
        0x1C,
        0x25,
        0x40,
        0x46,
        0x9A,
        0x9C,
        0x9E,
        0xAB,
        0xAD,
        0xAF,
        0xD8,
        0xDE,
        0xE0,
        0xEB,
    )
    if any(len(address_rows.get(offset, ())) != 1 for offset in fixed_offsets):
        raise ValueError(
            "HUD track-marker loop bridge requires unique exact candidate "
            "callsites, cursor chain, loop header, induction, and CFG rows"
        )
    index_by_offset = {
        offset: address_rows[offset][0] for offset in fixed_offsets
    }

    def require_instruction(
        offset: int,
        mnemonic: str,
        operand_pattern: str,
    ) -> Instruction:
        instruction = candidate.instructions[index_by_offset[offset]]
        encoded = bytes(int(value, 16) for value in instruction.bytes)
        if (
            _cc_cfg._instruction_mnemonic(instruction) != mnemonic
            or re.fullmatch(
                operand_pattern,
                _cc_cfg._instruction_operand(instruction).strip(),
                flags=re.IGNORECASE,
            )
            is None
            or caller.data[offset : offset + len(encoded)] != encoded
        ):
            raise ValueError(
                "HUD track-marker loop bridge rejects byte, operand, "
                f"callsite, cursor, receiver, stride, or bound drift at "
                f"+0x{offset:x}"
            )
        return instruction

    require_instruction(
        0x03,
        "mov",
        rf"eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
        rf"{re.escape(aggregate)}\+(?:3832|0xef8)",
    )
    require_instruction(0x0A, "mov", r"ebp\s*,\s*eax")
    require_instruction(0x0C, "shl", r"ebp\s*,\s*(?:3|0x3)")
    require_instruction(0x0F, "sub", r"ebp\s*,\s*eax")
    require_instruction(0x12, "shl", r"ebp\s*,\s*(?:6|0x6)")
    require_instruction(
        0x16,
        "add",
        rf"ebp\s*,\s*(?:offset\s+flat:)?"
        rf"{re.escape(aggregate)}\+(?:3836|0xefc)",
    )
    require_instruction(
        0x1C,
        "mov",
        rf"esi\s*,\s*(?:offset\s+flat:)?"
        rf"{re.escape(aggregate)}\+(?:3836|0xefc)",
    )
    require_instruction(0x25, "cmp", r"ebp\s*,\s*esi")
    require_instruction(
        0xD8,
        "add",
        r"esi\s*,\s*(?:448|0x1c0)",
    )
    require_instruction(0xDE, "cmp", r"esi\s*,\s*ebp")

    start = 0
    end = len(caller.data)
    backedge_index = index_by_offset[0xE0]
    exit_index = index_by_offset[0x40]
    if (
        _cc_cfg._exact_local_direct_branch(
            candidate.instructions[backedge_index],
            instruction_index=backedge_index,
            instruction_addresses=instruction_offsets,
            instruction_index_by_address={
                offset: rows[0]
                for offset, rows in address_rows.items()
                if len(rows) == 1
            },
            source="cod",
            caller_start=start,
            caller_end=end,
        )
        != ("conditional", index_by_offset[0x46])
        or _cc_cfg._exact_local_direct_branch(
            candidate.instructions[exit_index],
            instruction_index=exit_index,
            instruction_addresses=instruction_offsets,
            instruction_index_by_address={
                offset: rows[0]
                for offset, rows in address_rows.items()
                if len(rows) == 1
            },
            source="cod",
            caller_start=start,
            caller_end=end,
        )
        != ("conditional", index_by_offset[0xEB])
    ):
        raise ValueError(
            "HUD track-marker loop bridge requires the exact entry exit "
            "and +0xe0-to-+0x46 induction-loop CFG"
        )


    header_index = index_by_offset[0x46]
    if any(
        _cc_instructions.may_clobber_register(candidate.instructions[index], "esi")
        and index != index_by_offset[0xD8]
        for index in range(header_index, backedge_index + 1)
    ) or any(
        _cc_instructions.may_clobber_register(candidate.instructions[index], "ebp")
        for index in range(header_index, backedge_index + 1)
    ):
        raise ValueError(
            "HUD track-marker loop bridge rejects loop-cursor, induction, "
            "or bound-register clobber and alias drift"
        )

    bounded_calls = [
        offset
        for offset, rows in address_rows.items()
        if (
            0x46 <= offset <= 0xE0
            and len(rows) == 1
            and _cc_cfg._instruction_mnemonic(
                candidate.instructions[rows[0]]
            )
            == "call"
        )
    ]
    if sorted(bounded_calls) != [0x9E, 0xAF]:
        raise ValueError(
            "HUD track-marker loop bridge requires exactly the two ordered "
            "physical virtual calls in the reviewed loop"
        )

    bridges: dict[str, ReviewedLoopVptrStorageBridge] = {}
    for _, vptr_offset, receiver_offset, call_offset, slot in (
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_CALL_SPECS
    ):
        vptr = require_instruction(
            vptr_offset,
            "mov",
            r"(?:eax|edx)\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[esi\]",
        )
        receiver = require_instruction(
            receiver_offset,
            "mov",
            r"ecx\s*,\s*esi",
        )
        del receiver
        vptr_operands = _cc_cfg._instruction_operand(vptr).split(",", 1)
        vptr_register = (
            vptr_operands[0].strip().lower()
            if len(vptr_operands) == 2
            else ""
        )
        call = require_instruction(
            call_offset,
            "call",
            rf"(?:dword\s+(?:ptr\s+)?)?\[{vptr_register}"
            rf"\+(?:{slot}|0x{slot:x})\]",
        )
        vptr_index = index_by_offset[vptr_offset]
        receiver_index = index_by_offset[receiver_offset]
        call_index = index_by_offset[call_offset]
        if (
            (vptr_index, receiver_index, call_index)
            != (vptr_index, vptr_index + 1, vptr_index + 2)
            or any(
                any(
                    caller.relocation_mask[
                        offset : offset + len(
                            bytes(
                                int(value, 16)
                                for value in instruction.bytes
                            )
                        )
                    ]
                )
                for offset, instruction in (
                    (vptr_offset, vptr),
                    (receiver_offset, candidate.instructions[receiver_index]),
                    (call_offset, call),
                )
            )
            or _cc_cfg._cleanup_after(
                candidate.instructions,
                call_index,
            )
            is not None
        ):
            raise ValueError(
                "HUD track-marker loop bridge requires an immediate "
                "[ESI]-vptr, ECX receiver, callee-cleanup virtual call "
                f"chain at +0x{call_offset:x}"
            )
        bridges[normalize_address(hex(call_offset))] = (
            ReviewedLoopVptrStorageBridge(
                register=vptr_register,
                storage_identity=retail_storage_identity,
                slot_displacement=slot,
                assembly_source="cod",
            )
        )
    return bridges


def _hud_ui_mgr_sensor_track_marker_exit_candidate_vptr_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedLoopVptrStorageBridge]:
    """Bind PlaceTrackMarker's tracked-slot exit virtual-call prefix."""
    from _recoil.call_contract.records import ReviewedExactIndirectStorageBridge
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_CALLER_START:
        return {}

    # Reuse the already-reviewed caller, source, aggregate, loop, and initial
    # call-order authority.  This bridge adds only the post-loop receiver path.
    _hud_ui_mgr_sensor_track_marker_loop_candidate_vptr_bridges(
        expected,
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )

    retail_row = (
        expected[_cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_EXIT_RETAIL_ORDINAL]
        if len(expected)
        > _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_EXIT_RETAIL_ORDINAL
        else None
    )
    if retail_row != {
        "ordinal": _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_EXIT_RETAIL_ORDINAL,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": (
            _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_EXIT_STORAGE_IDENTITY
        ),
        "slot_displacement": _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_EXIT_SLOT,
        "cleanup_bytes": None,
    }:
        raise ValueError(
            "HUD track-marker exit bridge requires the exact immutable "
            "retail ordinal-3 tracked-slot virtual contract"
        )
    selected_retail_row = (
        expected[_cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SELECTED_RETAIL_ORDINAL]
        if len(expected)
        > _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SELECTED_RETAIL_ORDINAL
        else None
    )
    if selected_retail_row != {
        "ordinal": _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SELECTED_RETAIL_ORDINAL,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": (
            _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_EXIT_STORAGE_IDENTITY
        ),
        "slot_displacement": _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SELECTED_SLOT,
        "cleanup_bytes": None,
    }:
        raise ValueError(
            "HUD track-marker exit bridge requires the exact immutable "
            "retail ordinal-4 selected-slot GetCenterX contract"
        )
    setpos_retail_row = (
        expected[_cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_RETAIL_ORDINAL]
        if len(expected)
        > _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_RETAIL_ORDINAL
        else None
    )
    if setpos_retail_row != {
        "ordinal": _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_RETAIL_ORDINAL,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": (
            _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_STORAGE_IDENTITY
        ),
        "slot_displacement": _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_SLOT,
        "cleanup_bytes": None,
    }:
        raise ValueError(
            "HUD track-marker exit bridge requires the exact immutable "
            "retail ordinal-5 tracked-widget SetPos contract"
        )
    setvisible_retail_row = (
        expected[_cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_RETAIL_ORDINAL]
        if len(expected)
        > _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_RETAIL_ORDINAL
        else None
    )
    if setvisible_retail_row != {
        "ordinal": _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_RETAIL_ORDINAL,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": (
            _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_STORAGE_IDENTITY
        ),
        "slot_displacement": _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_SLOT,
        "cleanup_bytes": None,
    }:
        raise ValueError(
            "HUD track-marker exit bridge requires the exact immutable "
            "retail ordinal-6 tracked-widget SetVisible contract"
        )

    caller = candidate.caller_definition
    if caller is None:
        raise ValueError(
            "HUD track-marker exit bridge requires the exact candidate "
            "caller definition"
        )
    aggregate = _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
    receiver_relocations = [
        row
        for row in caller.relocations
        if (
            _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_EXIT_LOAD_OFFSET
            < row.offset
            < 0xEB
        )
    ]
    receiver_relocation_offset = (
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_EXIT_LOAD_OFFSET + 1
    )
    if (
        len(receiver_relocations) != 1
        or receiver_relocations[0].offset != receiver_relocation_offset
        or receiver_relocations[0].type != IMAGE_REL_I386_DIR32
        or receiver_relocations[0].symbol_name != aggregate
        or struct.unpack_from(
            "<I",
            caller.data,
            receiver_relocation_offset,
        )[0]
        != 0xC20
        or not all(
            caller.relocation_mask[index]
            for index in range(
                receiver_relocation_offset,
                receiver_relocation_offset + 4,
            )
        )
        or {
            offset
            for offset in range(
                _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_EXIT_LOAD_OFFSET,
                0xEB,
            )
            if caller.relocation_mask[offset]
        }
        != set(
            range(
                receiver_relocation_offset,
                receiver_relocation_offset + 4,
            )
        )
    ):
        raise ValueError(
            "HUD track-marker exit bridge requires one exact DIR32 aggregate "
            "member +0xc20 receiver load"
        )

    instruction_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    address_rows: dict[int, list[int]] = {}
    for index, offset in enumerate(instruction_offsets):
        if offset is not None:
            address_rows.setdefault(offset, []).append(index)
    fixed_offsets = (
        0xE6,
        0xEB,
        0xF0,
        0xF6,
        0xFA,
        0x100,
        0x102,
        0x108,
        0x10A,
        0x110,
        0x116,
        0x118,
        0x119,
        0x11F,
        0x121,
        0x126,
        0x128,
        0x12E,
        0x130,
        0x133,
        0x135,
        0x13E,
        0x144,
        0x147,
        0x149,
        0x14D,
        0x14E,
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_FIRST_ARGUMENT_OFFSET,
        0x151,
        0x153,
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_VPTR_OFFSET,
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_SECOND_ARGUMENT_OFFSET,
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_RECEIVER_OFFSET,
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_CALL_OFFSET,
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_VPTR_OFFSET,
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_RECEIVER_OFFSET,
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_ARGUMENT_OFFSET,
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_CALL_OFFSET,
    )
    if any(len(address_rows.get(offset, ())) != 1 for offset in fixed_offsets):
        raise ValueError(
            "HUD track-marker exit bridge requires unique exact tracked-slot "
            "load, guard, receiver, vptr, call, and successor rows"
        )
    index_by_offset = {
        offset: address_rows[offset][0] for offset in fixed_offsets
    }

    def require_instruction(
        offset: int,
        mnemonic: str,
        operand_pattern: str,
    ) -> Instruction:
        instruction = candidate.instructions[index_by_offset[offset]]
        encoded = bytes(int(value, 16) for value in instruction.bytes)
        if (
            _cc_cfg._instruction_mnemonic(instruction) != mnemonic
            or re.fullmatch(
                operand_pattern,
                _cc_cfg._instruction_operand(instruction).strip(),
                flags=re.IGNORECASE,
            )
            is None
            or caller.data[offset : offset + len(encoded)] != encoded
        ):
            raise ValueError(
                "HUD track-marker exit bridge rejects byte, operand, global, "
                f"member, register, slot, or topology drift at +0x{offset:x}"
            )
        return instruction

    require_instruction(
        0xE6,
        "mov",
        rf"eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
        rf"{re.escape(aggregate)}\+(?:3104|0xc20)",
    )
    require_instruction(
        0xEB,
        "cmp",
        r"(?:dword\s+)?(?:"
        r"_markerMode\$\[esp\+32\]"
        r"|\[esp\+24\]"
        r")\s*,\s*1",
    )
    require_instruction(0xF0, "jne", r".+")
    require_instruction(
        0xF6,
        "mov",
        r"ecx\s*,\s*(?:dword\s+)?(?:"
        r"_nearestDistSq\$\[esp\+32\]"
        r"|\[esp\+20\]"
        r")",
    )
    require_instruction(
        0xFA,
        "mov",
        rf"edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
        rf"{re.escape(aggregate)}\+(?:18172|0x46fc)",
    )
    require_instruction(0x100, "cmp", r"ecx\s*,\s*edx")
    require_instruction(0x102, "jae", r".+")
    require_instruction(0x108, "test", r"eax\s*,\s*eax")
    require_instruction(0x10A, "je", r".+")
    require_instruction(
        0x110,
        "mov",
        rf"edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
        rf"{re.escape(aggregate)}\+(?:3812|0xee4)",
    )
    require_instruction(0x116, "mov", r"esi\s*,\s*eax")
    require_instruction(0x118, "push", r"edx")
    require_instruction(
        0x119,
        "lea",
        r"edi\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[esi\+(?:260|0x104)\]",
    )
    require_instruction(0x11F, "mov", r"ecx\s*,\s*edi")
    direct_call = require_instruction(0x121, "call", r".+")
    vptr = require_instruction(
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_EXIT_VPTR_OFFSET,
        "mov",
        r"eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[esi\]",
    )
    require_instruction(
        0x128,
        "mov",
        r"ebp\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[esi\+(?:320|0x140)\]",
    )
    require_instruction(0x12E, "mov", r"ecx\s*,\s*esi")
    virtual_call = require_instruction(
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_EXIT_CALL_OFFSET,
        "call",
        r"(?:dword\s+(?:ptr\s+)?)?\[eax\+(?:104|0x68)\]",
    )
    require_instruction(0x133, "mov", r"ebx\s*,\s*eax")
    selected_receiver = require_instruction(
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SELECTED_RECEIVER_OFFSET,
        "mov",
        r"ecx\s*,\s*esi",
    )
    selected_vptr = require_instruction(
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SELECTED_VPTR_OFFSET,
        "mov",
        r"edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[esi\]",
    )
    selected_virtual_call = require_instruction(
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SELECTED_CALL_OFFSET,
        "call",
        r"(?:dword\s+(?:ptr\s+)?)?\[edx\+(?:100|0x64)\]",
    )
    require_instruction(0x147, "mov", r"ecx\s*,\s*eax")
    require_instruction(
        0x149,
        "movsx",
        r"eax\s*,\s*(?:word\s+(?:ptr\s+)?)?\[ebp\+(?:4|0x4)\]",
    )
    require_instruction(0x14D, "cdq", r"")
    require_instruction(0x14E, "sub", r"eax\s*,\s*edx")
    first_setpos_argument = require_instruction(
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_FIRST_ARGUMENT_OFFSET,
        "push",
        r"ebx",
    )
    require_instruction(0x151, "sar", r"eax\s*,\s*(?:1|0x1)")
    require_instruction(0x153, "sub", r"ecx\s*,\s*eax")
    setpos_vptr = require_instruction(
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_VPTR_OFFSET,
        "mov",
        r"eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[edi\]",
    )
    second_setpos_argument = require_instruction(
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_SECOND_ARGUMENT_OFFSET,
        "push",
        r"ecx",
    )
    setpos_receiver = require_instruction(
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_RECEIVER_OFFSET,
        "mov",
        r"ecx\s*,\s*edi",
    )
    setpos_call = require_instruction(
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_CALL_OFFSET,
        "call",
        r"(?:dword\s+(?:ptr\s+)?)?\[eax\+(?:12|0xc)\]",
    )
    setvisible_vptr = require_instruction(
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_VPTR_OFFSET,
        "mov",
        r"edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[edi\]",
    )
    setvisible_receiver = require_instruction(
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_RECEIVER_OFFSET,
        "mov",
        r"ecx\s*,\s*edi",
    )
    setvisible_argument = require_instruction(
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_ARGUMENT_OFFSET,
        "push",
        r"(?:1|0x1)",
    )
    setvisible_call = require_instruction(
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_CALL_OFFSET,
        "call",
        r"(?:dword\s+(?:ptr\s+)?)?\[edx\+(?:96|0x60)\]",
    )

    instruction_index_by_address = {
        offset: rows[0]
        for offset, rows in address_rows.items()
        if len(rows) == 1
    }
    for branch_offset in (0xF0, 0x102, 0x10A):
        branch_index = index_by_offset[branch_offset]
        if (
            _cc_cfg._exact_local_direct_branch(
                candidate.instructions[branch_index],
                instruction_index=branch_index,
                instruction_addresses=instruction_offsets,
                instruction_index_by_address=instruction_index_by_address,
                source="cod",
                caller_start=0,
                caller_end=len(caller.data),
            )
            != ("conditional", instruction_index_by_address.get(0x1BC))
        ):
            raise ValueError(
                "HUD track-marker exit bridge requires the exact three-guard "
                "CFG to the common +0x1bc exit"
            )

    call_offsets = [
        offset
        for offset, rows in address_rows.items()
        if (
            offset <= _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_CALL_OFFSET
            and len(rows) == 1
            and _cc_cfg._instruction_mnemonic(candidate.instructions[rows[0]])
            == "call"
        )
    ]
    direct_call_bytes = bytes(int(value, 16) for value in direct_call.bytes)
    virtual_call_bytes = bytes(
        int(value, 16) for value in virtual_call.bytes
    )
    selected_virtual_call_bytes = bytes(
        int(value, 16) for value in selected_virtual_call.bytes
    )
    setpos_call_bytes = bytes(
        int(value, 16) for value in setpos_call.bytes
    )
    setvisible_call_bytes = bytes(
        int(value, 16) for value in setvisible_call.bytes
    )
    setpos_prefix_offsets = (
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_FIRST_ARGUMENT_OFFSET,
        0x151,
        0x153,
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_VPTR_OFFSET,
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_SECOND_ARGUMENT_OFFSET,
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_RECEIVER_OFFSET,
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_CALL_OFFSET,
    )
    setpos_prefix_indices = tuple(
        index_by_offset[offset] for offset in setpos_prefix_offsets
    )
    setvisible_prefix_offsets = (
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_VPTR_OFFSET,
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_RECEIVER_OFFSET,
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_ARGUMENT_OFFSET,
        _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_CALL_OFFSET,
    )
    setvisible_prefix_indices = tuple(
        index_by_offset[offset] for offset in setvisible_prefix_offsets
    )
    if (
        sorted(call_offsets)
        != [0x9E, 0xAF, 0x121, 0x130, 0x144, 0x15A, 0x163]
        or not direct_call_bytes
        or direct_call_bytes[0] != 0xE8
        or virtual_call_bytes != b"\xff\x50\x68"
        or selected_virtual_call_bytes != b"\xff\x52\x64"
        or setpos_call_bytes != b"\xff\x50\x0c"
        or setvisible_call_bytes != b"\xff\x52\x60"
        or setpos_prefix_indices
        != tuple(
            range(
                setpos_prefix_indices[0],
                setpos_prefix_indices[0] + len(setpos_prefix_indices),
            )
        )
        or setvisible_prefix_indices
        != tuple(
            range(
                setvisible_prefix_indices[0],
                setvisible_prefix_indices[0]
                + len(setvisible_prefix_indices),
            )
        )
        or any(
            any(
                caller.relocation_mask[
                    offset : offset
                    + len(
                        bytes(
                            int(value, 16)
                            for value in instruction.bytes
                        )
                    )
                ]
            )
            for offset, instruction in (
                (0x116, candidate.instructions[index_by_offset[0x116]]),
                (0x126, vptr),
                (0x12E, candidate.instructions[index_by_offset[0x12E]]),
                (0x130, virtual_call),
                (
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SELECTED_RECEIVER_OFFSET,
                    selected_receiver,
                ),
                (
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SELECTED_VPTR_OFFSET,
                    selected_vptr,
                ),
                (
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SELECTED_CALL_OFFSET,
                    selected_virtual_call,
                ),
                (
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_FIRST_ARGUMENT_OFFSET,
                    first_setpos_argument,
                ),
                (
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_VPTR_OFFSET,
                    setpos_vptr,
                ),
                (
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_SECOND_ARGUMENT_OFFSET,
                    second_setpos_argument,
                ),
                (
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_RECEIVER_OFFSET,
                    setpos_receiver,
                ),
                (
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_CALL_OFFSET,
                    setpos_call,
                ),
                (
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_VPTR_OFFSET,
                    setvisible_vptr,
                ),
                (
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_RECEIVER_OFFSET,
                    setvisible_receiver,
                ),
                (
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_ARGUMENT_OFFSET,
                    setvisible_argument,
                ),
                (
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_CALL_OFFSET,
                    setvisible_call,
                ),
            )
        )
        or _cc_cfg._cleanup_after(
            candidate.instructions,
            index_by_offset[0x130],
        )
        is not None
        or _cc_cfg._cleanup_after(
            candidate.instructions,
            index_by_offset[
                _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SELECTED_CALL_OFFSET
            ],
        )
        is not None
        or _cc_cfg._cleanup_after(
            candidate.instructions,
            index_by_offset[
                _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_CALL_OFFSET
            ],
        )
        is not None
        or _cc_cfg._cleanup_after(
            candidate.instructions,
            index_by_offset[
                _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_CALL_OFFSET
            ],
        )
        is not None
    ):
        raise ValueError(
            "HUD track-marker exit bridge rejects call count/order/form, "
            "receiver-chain relocation, SetPos argument order, virtual "
            "dispatch, or cleanup drift"
        )

    if (
        any(
            _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index],
                "eax",
            )
            for index in range(
                index_by_offset[0xE6] + 1,
                index_by_offset[0x116],
            )
        )
        or any(
            _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index],
                "esi",
            )
            for index in range(
                index_by_offset[0x116] + 1,
                index_by_offset[
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SELECTED_CALL_OFFSET
                ],
            )
        )
        or any(
            _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index],
                "eax",
            )
            for index in range(
                index_by_offset[0x126] + 1,
                index_by_offset[0x130],
            )
        )
        or any(
            _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index],
                "ecx",
            )
            for index in range(
                index_by_offset[
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SELECTED_RECEIVER_OFFSET
                ]
                + 1,
                index_by_offset[
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SELECTED_CALL_OFFSET
                ],
            )
        )
        or any(
            _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index],
                "edx",
            )
            for index in range(
                index_by_offset[
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SELECTED_VPTR_OFFSET
                ]
                + 1,
                index_by_offset[
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SELECTED_CALL_OFFSET
                ],
            )
        )
        or any(
            _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index],
                "edi",
            )
            for index in range(
                index_by_offset[0x119] + 1,
                index_by_offset[
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_CALL_OFFSET
                ],
            )
        )
        or any(
            _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index],
                "eax",
            )
            for index in range(
                index_by_offset[
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_VPTR_OFFSET
                ]
                + 1,
                index_by_offset[
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_CALL_OFFSET
                ],
            )
        )
        or any(
            _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index],
                "ecx",
            )
            for index in range(
                index_by_offset[
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_RECEIVER_OFFSET
                ]
                + 1,
                index_by_offset[
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_CALL_OFFSET
                ],
            )
        )
        or any(
            _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index],
                "edx",
            )
            for index in range(
                index_by_offset[
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_VPTR_OFFSET
                ]
                + 1,
                index_by_offset[
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_CALL_OFFSET
                ],
            )
        )
        or any(
            _cc_cfg._instruction_may_clobber_register(
                candidate.instructions[index],
                "ecx",
            )
            for index in range(
                index_by_offset[
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_RECEIVER_OFFSET
                ]
                + 1,
                index_by_offset[
                    _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_CALL_OFFSET
                ],
            )
        )
    ):
        raise ValueError(
            "HUD track-marker exit bridge rejects tracked-slot, selected "
            "member EDI/ECX receiver, or EAX/EDX vptr reaching-definition "
            "clobber"
        )

    return {
        normalize_address(
            hex(_cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_EXIT_CALL_OFFSET)
        ): ReviewedExactIndirectStorageBridge(
            register="eax",
            storage_identity=(
                _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_EXIT_STORAGE_IDENTITY
            ),
            slot_displacement=_cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_EXIT_SLOT,
            assembly_source="cod",
        ),
        normalize_address(
            hex(_cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SELECTED_CALL_OFFSET)
        ): ReviewedExactIndirectStorageBridge(
            register="edx",
            storage_identity=(
                _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_EXIT_STORAGE_IDENTITY
            ),
            slot_displacement=_cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SELECTED_SLOT,
            assembly_source="cod",
        ),
        normalize_address(
            hex(_cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_CALL_OFFSET)
        ): ReviewedExactIndirectStorageBridge(
            register="eax",
            storage_identity=(
                _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_STORAGE_IDENTITY
            ),
            slot_displacement=_cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_SLOT,
            assembly_source="cod",
        ),
        normalize_address(
            hex(_cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_CALL_OFFSET)
        ): ReviewedExactIndirectStorageBridge(
            register="edx",
            storage_identity=(
                _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_STORAGE_IDENTITY
            ),
            slot_displacement=(
                _cc_catalog.HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_SLOT
            ),
            assembly_source="cod",
        ),
    }
