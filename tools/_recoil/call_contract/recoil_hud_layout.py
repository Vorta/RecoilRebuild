"""Recoil call-contract recoil hud layout evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import extraction as _cc_extraction
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract import instructions as _cc_instructions
from _recoil.call_contract import receiver_candidate as _cc_receiver_candidate
from _recoil.call_contract import receiver_storage as _cc_receiver_storage
from _recoil.call_contract import recoil_hud_widgets as _cc_recoil_hud_widgets
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
    Instruction,
)
from _recoil.lib.authored_icf import (
    exact_required_target_membership,
    exact_selected_target_membership,
)
from _recoil.lib.pe import parse_pe_headers, rva_to_offset
from _recoil.lib.progress import ProgressDocument, address_value, normalize_address
from _recoil.lib.tooling import REPO_ROOT
from _recoil.lib.windows_identity import StableReadHandle


def _hud_layout_hw_update_objective_dirty_rect_register_storage_bridges(
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    reference: Path = _cc_catalog.DEFAULT_REFERENCE,
) -> tuple[dict[str, str], dict[str, str]]:
    """Prove only UpdateObjectiveDirtyRect's GetCenterX/GetCenterY pair."""
    from _recoil.call_contract.records import StorageContainer
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_CALLER_START:
        return {}, {}

    symbols = document.collection("symbols")
    caller_symbol_id = (
        _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_CALLER_IDENTITY.removeprefix(
            "symbol:"
        )
    )
    caller_row = symbols.get(caller_symbol_id)
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
    target_rows: dict[str, list[tuple[Any, Any]]] = {
        _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_CALLER_START: [],
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
    caller_addresses = sorted(
        address
        for address, identity in indexes.by_address.items()
        if identity == caller_identity
    )
    caller_names = [
        (name, identity)
        for name, identity in indexes.by_candidate_name.items()
        if name.casefold()
        == _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_CALLER_SYMBOL.casefold()
    ]
    if (
        caller_identity
        != _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_CALLER_END_EXCLUSIVE
        or caller_addresses
        != [_cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_CALLER_START]
        or caller_names
        not in (
            [],
            [
                (
                    _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_CALLER_SYMBOL,
                    caller_identity,
                )
            ],
        )
        or caller_identity in indexes.provider_ids
        or not isinstance(caller_row, Mapping)
        or caller_row.get("binary") != "recoil"
        or caller_row.get("kind") != "function"
        or caller_row.get("pipeline_class") != "authored"
        or caller_row.get("ownership_state") != "primary-owned"
        or caller_row.get("address") != normalized_start
        or caller_row.get("end_exclusive")
        != _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_CALLER_END_EXCLUSIVE
        or caller_row.get("extent_state") != "known"
        or caller_row.get("size") != 0x90
        or caller_row.get("navigation_name")
        != "HudLayoutHW::UpdateObjectiveDirtyRect"
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
        != _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
        or caller is None
        or caller.symbol
        != _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_CALLER_SYMBOL
        or len(caller.data) not in {0x90, 0xA0}
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
        or any(len(rows) != 1 for rows in target_rows.values())
    ):
        raise ValueError(
            "HUD UpdateObjectiveDirtyRect center bridge requires the exact "
            "authored caller, source edge, extent, symbol, and current HUD "
            "target authority"
        )

    expected_target_rows = {
        _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_CALLER_START: (
            "",
            r"\?UpdateObjectiveDirtyRect@HudLayoutHW@@.*",
            "HudLayoutHW::UpdateObjectiveDirtyRect",
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
                "HUD UpdateObjectiveDirtyRect center bridge requires exact "
                "authored hud.cpp caller/GetCenterX/GetCenterY target rows"
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
        row = symbols.get(identity.removeprefix("symbol:"))
        row_trace = (
            row.get("source_traceability")
            if isinstance(row, Mapping)
            else None
        )
        row_edges = (
            row_trace.get("source_edges")
            if isinstance(row_trace, Mapping)
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
            not isinstance(row, Mapping)
            or row.get("binary") != "recoil"
            or row.get("kind") != "function"
            or row.get("pipeline_class") != "authored"
            or row.get("ownership_state") != "primary-owned"
            or row.get("extent_state") != "known"
            or row.get("address") != address
            or row.get("size") != 0x40
            or row.get("navigation_name") != navigation_name
            or not isinstance(row_trace, Mapping)
            or row_trace.get("state") != "resolved"
            or row_trace.get("reason_code") not in {None, ""}
            or not isinstance(row_edges, list)
            or len(row_edges) != 1
            or not isinstance(row_edges[0], Mapping)
            or row_edges[0].get("relation") != "defines"
            or row_edges[0].get("anchor_id") != anchor_id
            or row_edges[0].get("emission_context")
            != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
            or matching_addresses != [address]
            or matching_names != [(candidate_name, identity)]
            or identity in indexes.provider_ids
        ):
            raise ValueError(
                "HUD UpdateObjectiveDirtyRect center bridge requires unique "
                f"typed authored {navigation_name} authority"
            )

    storages = document.collection("storage_contributions")
    owners = document.collection("owners")
    targets = document.collection("verification_targets")
    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    aggregate_symbol = symbols.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID)
    aggregate_storage = storages.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_STORAGE_ID)
    aggregate_target = targets.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_ID)
    aggregate_registration = (
        aggregate_target.get("registration")
        if isinstance(aggregate_target, Mapping)
        else None
    )
    aggregate_owner = owners.get(_cc_catalog.HUD_UI_MGR_OWNER_ID)
    aggregate_gates = (
        aggregate_owner.get("gates")
        if isinstance(aggregate_owner, Mapping)
        else None
    )
    aggregate_start = address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
    field_address = normalize_address(
        aggregate_start
        + _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_WIDGET_DISPLACEMENT
    )
    exact_containers = [
        row
        for row in indexes.storage_containers
        if row
        == StorageContainer(
            aggregate_start,
            aggregate_start + _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE,
            aggregate_identity,
        )
    ]
    if (
        not isinstance(aggregate_symbol, Mapping)
        or aggregate_symbol.get("binary") != "recoil"
        or aggregate_symbol.get("kind") != "data"
        or aggregate_symbol.get("disposition") != "authored"
        or aggregate_symbol.get("address") != _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
        or aggregate_symbol.get("navigation_name")
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_NAME
        or aggregate_symbol.get("storage_contribution_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_STORAGE_ID]
        or aggregate_symbol.get("verification_target_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_ID]
        or not isinstance(aggregate_storage, Mapping)
        or aggregate_storage.get("binary") != "recoil"
        or aggregate_storage.get("kind") != "data-symbol"
        or aggregate_storage.get("overlap") != "none"
        or aggregate_storage.get("owner_ids") != [_cc_catalog.HUD_UI_MGR_OWNER_ID]
        or aggregate_storage.get("symbol_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID]
        or not isinstance(aggregate_storage.get("reference"), Mapping)
        or aggregate_storage["reference"].get("address")
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
        or not isinstance(aggregate_target, Mapping)
        or aggregate_target.get("binary") != "recoil"
        or aggregate_target.get("kind") != "vc5"
        or aggregate_target.get("symbol_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID]
        or not isinstance(aggregate_registration, Mapping)
        or aggregate_registration.get("data_addresses")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS]
        or not isinstance(aggregate_owner, Mapping)
        or aggregate_owner.get("provider_state") == "accepted"
        or not isinstance(aggregate_gates, Mapping)
        or any(
            aggregate_gates.get(gate) != "accepted"
            for gate in ("boundary", "source", "data", "owner_linkage")
        )
        or indexes.storage_by_address.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
        != aggregate_identity
        or indexes.storage_by_address.get(field_address, "") != ""
        or len(exact_containers) != 1
        or aggregate_identity in indexes.provider_ids
        or field_address
        != _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_WIDGET_ADDRESS
        or (
            _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_WIDGET_DISPLACEMENT + 4
            > _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
        )
    ):
        raise ValueError(
            "HUD UpdateObjectiveDirtyRect center bridge requires the exact "
            "reviewed g_HudUiMgr aggregate, owner gates, container, and "
            "bounded uncatalogued +0x6ac widget authority"
        )

    retail_body = bytes.fromhex(
        "a1 b8 65 4e 00 83 ec 10 85 c0 56 57 8b f9 74 06 "
        "0f bf 70 04 eb 02 33 f6 a1 7c 65 4e 00 b9 7c 65 4e 00 "
        "ff 50 64 8b 0d 78 65 4e 00 8b 15 7c 65 4e 00 03 c6 "
        "89 4c 24 10 b9 7c 65 4e 00 89 44 24 08 ff 52 68 "
        "8b 0d b8 65 4e 00 89 44 24 0c 85 c9 74 06 0f bf 49 06 "
        "eb 02 33 c9 03 c1 8d 8f b4 01 00 00 89 44 24 14 "
        "8d 44 24 08 50 e8 71 0b 0a 00 b9 f0 62 4e 00 "
        "e8 57 0e 0a 00 b9 f0 62 4e 00 e8 cd c0 ff ff "
        "5f 5e 83 c4 10 c3 90 90 90 90 90 90 90"
    )
    try:
        with StableReadHandle(reference) as stable_reference:
            reference_data = stable_reference.read()
        reference_headers = parse_pe_headers(reference_data)
    except (OSError, ValueError) as exc:
        raise ValueError(
            "HUD UpdateObjectiveDirtyRect center bridge cannot read the "
            "immutable retail PE extent"
        ) from exc
    reference_start_rva = (
        address_value(normalized_start) - reference_headers.image_base
    )
    reference_end_rva = (
        address_value(_cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_CALLER_END_EXCLUSIVE)
        - reference_headers.image_base
    )
    reference_start_offset = rva_to_offset(
        reference_start_rva,
        reference_headers.sections,
    )
    reference_last_offset = rva_to_offset(
        reference_end_rva - 1,
        reference_headers.sections,
    )
    if (
        reference_headers.image_base != 0x400000
        or reference_start_offset is None
        or reference_last_offset is None
        or reference_last_offset
        != reference_start_offset + len(retail_body) - 1
        or reference_data[
            reference_start_offset : reference_start_offset + len(retail_body)
        ]
        != retail_body
    ):
        raise ValueError(
            "HUD UpdateObjectiveDirtyRect center bridge rejects immutable "
            "support/Recoil.exe code or trailing-padding extent drift"
        )

    retail_code_size = (
        address_value(
            _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_RETAIL_CODE_END_EXCLUSIVE
        )
        - address_value(normalized_start)
    )
    retail_code_body = retail_body[:retail_code_size]
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
        for address, instruction in zip(retail_addresses, retail_instructions)
        if address is not None and retail_counts.get(address) == 1
    }
    retail_index_by_address = {
        address: index
        for index, address in enumerate(retail_addresses)
        if address is not None and retail_counts.get(address) == 1
    }
    try:
        observed_retail_body = b"".join(
            bytes(int(value, 16) for value in instruction.bytes)
            for instruction in retail_instructions
        )
    except (TypeError, ValueError) as exc:
        raise ValueError(
            "HUD UpdateObjectiveDirtyRect center bridge requires exact "
            "retail instruction bytes"
        ) from exc
    retail_call_x = address_value(
        _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_RETAIL_CALL_X
    )
    retail_call_y = address_value(
        _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_RETAIL_CALL_Y
    )
    retail_pair_addresses = (
        address_value(normalized_start) + 0x18,
        address_value(normalized_start) + 0x1D,
        retail_call_x,
        address_value(normalized_start) + 0x25,
        address_value(normalized_start) + 0x2B,
        address_value(normalized_start) + 0x31,
        address_value(normalized_start) + 0x33,
        address_value(normalized_start) + 0x37,
        address_value(normalized_start) + 0x3C,
        retail_call_y,
    )
    retail_pair = tuple(
        retail_by_address.get(address) for address in retail_pair_addresses
    )
    retail_invocations = [
        address
        for address, instruction in zip(retail_addresses, retail_instructions)
        if address is not None
        and _cc_cfg._instruction_mnemonic(instruction) == "call"
    ]
    retail_cleanup_x = _cc_cfg._cleanup_after(
        retail_instructions,
        retail_index_by_address[retail_call_x],
    )
    retail_cleanup_y = _cc_cfg._cleanup_after(
        retail_instructions,
        retail_index_by_address[retail_call_y],
    )
    if (
        observed_retail_body != retail_code_body
        or any(instruction is None for instruction in retail_pair)
        or retail_invocations[:2] != [retail_call_x, retail_call_y]
        or not retail_instructions
        or retail_addresses[-1]
        != address_value(
            _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_RETAIL_CODE_END_EXCLUSIVE
        )
        - 1
        or _cc_cfg._instruction_mnemonic(retail_instructions[-1]) != "retn"
        or tuple(retail_instructions[-1].bytes) != ("c3",)
        or retail_cleanup_x is not None
        or retail_cleanup_y is not None
    ):
        raise ValueError(
            "HUD UpdateObjectiveDirtyRect center bridge rejects retail body, "
            "pair order, slot unit, terminal topology, or cleanup drift; "
            f"observed_body_len={len(observed_retail_body):#x}, "
            f"expected_code_len={len(retail_code_body):#x}, "
            f"first_calls={tuple(normalize_address(row) for row in retail_invocations[:2])!r}, "
            f"last_address={normalize_address(retail_addresses[-1]) if retail_addresses else None!r}, "
            f"last_mnemonic={_cc_cfg._instruction_mnemonic(retail_instructions[-1]) if retail_instructions else None!r}, "
            f"last_bytes={tuple(retail_instructions[-1].bytes) if retail_instructions else None!r}, "
            f"cleanup=({retail_cleanup_x!r}, {retail_cleanup_y!r})"
        )

    legacy_candidate_body = bytes.fromhex(
        "83 ec 10 53 55 56 8b 35 e8 06 00 00 85 f6 57 8b "
        "e9 74 06 0f bf 5e 04 eb 02 33 db a1 ac 06 00 00 "
        "b9 ac 06 00 00 ff 50 64 8b 15 ac 06 00 00 b9 ac 06 00 00 "
        "8b f8 ff 52 68 85 f6 74 06 0f bf 4e 06 eb 02 33 c9 "
        "8b 15 a8 06 00 00 89 44 24 14 03 c8 8d 44 24 10 "
        "03 fb 89 4c 24 1c 50 8d 8d b4 01 00 00 89 7c 24 14 "
        "89 54 24 1c e8 00 00 00 00 8b 15 20 04 00 00 "
        "b9 20 04 00 00 ff 52 20 a1 20 04 00 00 b9 20 04 00 00 "
        "ff 50 04 5f 5e 5d 5b 83 c4 10 c3 "
        "90 90 90 90 90 90 90 90 90 90 90 90 90 90 90"
    )
    legacy_relocation_specs = (
        (0x08, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x6E8),
        (0x1C, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x6AC),
        (0x21, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x6AC),
        (0x2A, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x6AC),
        (0x2F, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x6AC),
        (0x46, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x6A8),
        (0x6A, IMAGE_REL_I386_REL32, _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_INVALIDATE_SYMBOL, 0),
        (0x70, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x420),
        (0x75, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x420),
        (0x7D, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x420),
        (0x82, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x420),
    )
    direct_candidate_body = bytes.fromhex(
        "83 ec 10 53 55 56 8b 35 e8 06 00 00 85 f6 57 8b "
        "e9 74 06 0f bf 5e 04 eb 02 33 db a1 ac 06 00 00 "
        "b9 ac 06 00 00 ff 50 64 8b 15 ac 06 00 00 b9 ac 06 00 00 "
        "8b f8 ff 52 68 85 f6 74 06 0f bf 4e 06 eb 02 33 c9 "
        "8b 15 a8 06 00 00 89 44 24 14 03 c8 8d 44 24 10 "
        "03 fb 89 4c 24 1c 50 8d 8d b4 01 00 00 89 7c 24 14 "
        "89 54 24 1c e8 00 00 00 00 b9 20 04 00 00 "
        "e8 00 00 00 00 8b 15 20 04 00 00 b9 20 04 00 00 "
        "ff 52 04 5f 5e 5d 5b 83 c4 10 c3 90 90"
    )
    direct_relocation_specs = (
        (0x08, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x6E8),
        (0x1C, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x6AC),
        (0x21, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x6AC),
        (0x2A, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x6AC),
        (0x2F, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x6AC),
        (0x46, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x6A8),
        (0x6A, IMAGE_REL_I386_REL32, _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_INVALIDATE_SYMBOL, 0),
        (0x6F, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x420),
        (
            0x74,
            IMAGE_REL_I386_REL32,
            _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_INVALIDATE_SYMBOL,
            0,
        ),
        (0x7A, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x420),
        (0x7F, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x420),
    )
    dual_direct_candidate_body = bytes.fromhex(
        "83 ec 10 53 55 56 8b 35 e8 06 00 00 85 f6 57 8b "
        "e9 74 06 0f bf 5e 04 eb 02 33 db a1 ac 06 00 00 "
        "b9 ac 06 00 00 ff 50 64 8b 15 ac 06 00 00 b9 ac 06 00 00 "
        "8b f8 ff 52 68 85 f6 74 06 0f bf 4e 06 eb 02 33 c9 "
        "8b 15 a8 06 00 00 89 44 24 14 03 c8 8d 44 24 10 "
        "03 fb 89 4c 24 1c 50 8d 8d b4 01 00 00 89 7c 24 14 "
        "89 54 24 1c e8 00 00 00 00 b9 20 04 00 00 "
        "e8 00 00 00 00 b9 20 04 00 00 e8 00 00 00 00 "
        "5f 5e 5d 5b 83 c4 10 c3 90 90 90 90 90 90"
    )
    current_dual_direct_candidate_body = bytes.fromhex(
        "83 ec 10 53 55 56 8b 35 e8 06 00 00 85 f6 57 8b "
        "e9 74 06 0f bf 5e 04 eb 02 33 db a1 ac 06 00 00 "
        "b9 ac 06 00 00 ff 50 64 8b 15 ac 06 00 00 b9 ac 06 00 00 "
        "8b f8 ff 52 68 85 f6 74 06 0f bf 4e 06 eb 02 33 c9 "
        "8b 15 a8 06 00 00 89 44 24 14 03 c8 8d 44 24 10 "
        "03 df 89 4c 24 1c 50 8d 8d b4 01 00 00 89 5c 24 14 "
        "89 54 24 1c e8 00 00 00 00 b9 20 04 00 00 "
        "e8 00 00 00 00 b9 20 04 00 00 e8 00 00 00 00 "
        "5f 5e 5d 5b 83 c4 10 c3 90 90 90 90 90 90"
    )
    dual_direct_relocation_specs = (
        (0x08, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x6E8),
        (0x1C, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x6AC),
        (0x21, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x6AC),
        (0x2A, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x6AC),
        (0x2F, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x6AC),
        (0x46, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x6A8),
        (0x6A, IMAGE_REL_I386_REL32, _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_INVALIDATE_SYMBOL, 0),
        (0x6F, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x420),
        (
            0x74,
            IMAGE_REL_I386_REL32,
            _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_INVALIDATE_SYMBOL,
            0,
        ),
        (0x79, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, 0x420),
        (
            0x7E,
            IMAGE_REL_I386_REL32,
            _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_DRAW_SYMBOL,
            0,
        ),
    )
    candidate_shapes = {
        legacy_candidate_body: (
            "legacy-indirect-invalidate",
            legacy_relocation_specs,
            0x90,
        ),
        direct_candidate_body: (
            "direct-hud-ui-element-invalidate",
            direct_relocation_specs,
            0x8D,
        ),
        dual_direct_candidate_body: (
            "direct-hud-ui-element-invalidate-and-triplet-panel-draw",
            dual_direct_relocation_specs,
            0x89,
        ),
        current_dual_direct_candidate_body: (
            "direct-hud-ui-element-invalidate-and-triplet-panel-draw",
            dual_direct_relocation_specs,
            0x89,
        ),
    }
    candidate_shape = candidate_shapes.get(caller.data)
    if candidate_shape is None:
        raise ValueError(
            "HUD UpdateObjectiveDirtyRect center bridge rejects candidate "
            "body or terminal-padding extent outside the four complete "
            "reviewed candidate shapes"
        )
    (
        candidate_shape_name,
        relocation_specs,
        expected_return_offset,
    ) = candidate_shape
    if candidate_shape_name == "legacy-indirect-invalidate":
        expected_invocation_rows = (
            (0x25, ("ff", "50", "64"), "dword [eax+100]"),
            (0x35, ("ff", "52", "68"), "dword [edx+104]"),
            (
                0x69,
                ("e8", "00", "00", "00", "00"),
                _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_INVALIDATE_SYMBOL,
            ),
            (0x79, ("ff", "52", "20"), "dword [edx+32]"),
            (0x86, ("ff", "50", "04"), "dword [eax+4]"),
        )
    elif candidate_shape_name == "direct-hud-ui-element-invalidate":
        expected_invocation_rows = (
            (0x25, ("ff", "50", "64"), "dword [eax+100]"),
            (0x35, ("ff", "52", "68"), "dword [edx+104]"),
            (
                0x69,
                ("e8", "00", "00", "00", "00"),
                _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_INVALIDATE_SYMBOL,
            ),
            (
                0x73,
                ("e8", "00", "00", "00", "00"),
                _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_INVALIDATE_SYMBOL,
            ),
            (0x83, ("ff", "52", "04"), "dword [edx+4]"),
        )
    else:
        expected_invocation_rows = (
            (0x25, ("ff", "50", "64"), "dword [eax+100]"),
            (0x35, ("ff", "52", "68"), "dword [edx+104]"),
            (
                0x69,
                ("e8", "00", "00", "00", "00"),
                _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_INVALIDATE_SYMBOL,
            ),
            (
                0x73,
                ("e8", "00", "00", "00", "00"),
                _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_INVALIDATE_SYMBOL,
            ),
            (
                0x7D,
                ("e8", "00", "00", "00", "00"),
                _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_DRAW_SYMBOL,
            ),
        )
    expected_invocation_offsets = tuple(
        offset for offset, _bytes, _operand in expected_invocation_rows
    )
    observed_relocations = tuple(
        (
            relocation.offset,
            relocation.type,
            relocation.symbol_name,
            (
                struct.unpack_from("<I", caller.data, relocation.offset)[0]
                if 0 <= relocation.offset <= len(caller.data) - 4
                else None
            ),
        )
        for relocation in caller.relocations
    )
    expected_mask = {
        index
        for offset, _type, _symbol, _addend in relocation_specs
        for index in range(offset, offset + 4)
    }
    observed_mask = {
        index
        for index, masked in enumerate(caller.relocation_mask)
        if masked
    }
    if (
        observed_relocations != relocation_specs
        or observed_mask != expected_mask
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in caller.defined_external_data
        or caller.undefined_external_functions.count(
            _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_INVALIDATE_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_INVALIDATE_SYMBOL
        in caller.defined_external_functions
        or (
            candidate_shape_name != "legacy-indirect-invalidate"
            and caller.undefined_external_functions.count(
                _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_INVALIDATE_SYMBOL
            )
            != 1
        )
        or (
            candidate_shape_name != "legacy-indirect-invalidate"
            and _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_INVALIDATE_SYMBOL
            in caller.defined_external_functions
        )
        or (
            candidate_shape_name == "legacy-indirect-invalidate"
            and _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_INVALIDATE_SYMBOL
            in (
                caller.undefined_external_functions
                + caller.defined_external_functions
            )
        )
        or (
            candidate_shape_name
            == "direct-hud-ui-element-invalidate-and-triplet-panel-draw"
            and caller.defined_external_functions.count(
                _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_DRAW_SYMBOL
            )
            != 1
        )
        or (
            candidate_shape_name
            == "direct-hud-ui-element-invalidate-and-triplet-panel-draw"
            and _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_DRAW_SYMBOL
            in caller.undefined_external_functions
        )
        or (
            candidate_shape_name
            != "direct-hud-ui-element-invalidate-and-triplet-panel-draw"
            and _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_DRAW_SYMBOL
            in (
                caller.undefined_external_functions
                + caller.defined_external_functions
            )
        )
    ):
        raise ValueError(
            "HUD UpdateObjectiveDirtyRect center bridge rejects candidate "
            "body, terminal padding, complete COFF relocation/mask table, "
            "addend, target, or symbol-class drift"
        )

    candidate_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    offset_counts: dict[int, int] = {}
    for offset in candidate_offsets:
        if offset is not None:
            offset_counts[offset] = offset_counts.get(offset, 0) + 1
    candidate_by_offset = {
        offset: instruction
        for offset, instruction in zip(candidate_offsets, candidate.instructions)
        if offset is not None and offset_counts.get(offset) == 1
    }
    candidate_index_by_offset = {
        offset: index
        for index, offset in enumerate(candidate_offsets)
        if offset is not None and offset_counts.get(offset) == 1
    }
    pair_offsets = (0x1B, 0x20, 0x25, 0x28, 0x2E, 0x33, 0x35)
    candidate_pair = tuple(
        candidate_by_offset.get(offset) for offset in pair_offsets
    )
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    ordinal_by_index = {
        instruction_index: ordinal
        for ordinal, instruction_index in enumerate(invocation_indices)
    }
    candidate_call_x_index = candidate_index_by_offset.get(
        _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_CANDIDATE_CALL_X
    )
    candidate_call_y_index = candidate_index_by_offset.get(
        _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_CANDIDATE_CALL_Y
    )
    observed_invocation_offsets = tuple(
        candidate_offsets[index] for index in invocation_indices
    )
    invocation_rows_are_exact = all(
        (
            candidate_by_offset.get(offset) is not None
            and _cc_cfg._instruction_mnemonic(candidate_by_offset[offset]) == "call"
            and tuple(candidate_by_offset[offset].bytes) == instruction_bytes
            and _cc_cfg._instruction_operand(candidate_by_offset[offset]) == operand
        )
        for offset, instruction_bytes, operand in expected_invocation_rows
    )
    candidate_return_index = candidate_index_by_offset.get(
        expected_return_offset
    )
    if (
        any(instruction is None for instruction in candidate_pair)
        or caller.data[0x1B:0x38]
        != bytes.fromhex(
            "a1 ac 06 00 00 b9 ac 06 00 00 ff 50 64 "
            "8b 15 ac 06 00 00 b9 ac 06 00 00 8b f8 ff 52 68"
        )
        or candidate_call_x_index is None
        or candidate_call_y_index is None
        or ordinal_by_index.get(candidate_call_x_index) != 0
        or ordinal_by_index.get(candidate_call_y_index) != 1
        or candidate_call_x_index in candidate.local_control_flow_indices
        or candidate_call_y_index in candidate.local_control_flow_indices
        or _cc_cfg._cleanup_after(candidate.instructions, candidate_call_x_index)
        is not None
        or _cc_cfg._cleanup_after(candidate.instructions, candidate_call_y_index)
        is not None
        or observed_invocation_offsets != expected_invocation_offsets
        or not invocation_rows_are_exact
        or any(
            index in candidate.local_control_flow_indices
            for index in invocation_indices
        )
        or any(
            _cc_cfg._cleanup_after(candidate.instructions, index) is not None
            for index in invocation_indices
        )
        or candidate_return_index is None
        or candidate_return_index != len(candidate.instructions) - 1
        or _cc_cfg._instruction_mnemonic(
            candidate.instructions[candidate_return_index]
        )
        not in {"ret", "retn"}
        or tuple(candidate.instructions[candidate_return_index].bytes)
        != ("c3",)
    ):
        raise ValueError(
            "HUD UpdateObjectiveDirtyRect center bridge rejects candidate "
            "receiver, result-preservation, pair order, slot, cleanup, or "
            "local-control-flow topology drift"
        )

    if candidate_shape_name != "legacy-indirect-invalidate":
        direct_target_id = (
            _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_INVALIDATE_IDENTITY
            .removeprefix("symbol:")
        )
        direct_target = symbols.get(direct_target_id)
        direct_trace = (
            direct_target.get("source_traceability")
            if isinstance(direct_target, Mapping)
            else None
        )
        direct_edges = (
            direct_trace.get("source_edges")
            if isinstance(direct_trace, Mapping)
            else None
        )
        if (
            not isinstance(direct_target, Mapping)
            or direct_target.get("binary") != "recoil"
            or direct_target.get("kind") != "function"
            or direct_target.get("pipeline_class") != "authored"
            or direct_target.get("ownership_state") != "primary-owned"
            or direct_target.get("address")
            != _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_INVALIDATE_ADDRESS
            or direct_target.get("end_exclusive") != "0x4b4190"
            or direct_target.get("extent_state") != "known"
            or direct_target.get("size") != 0x10
            or direct_target.get("navigation_name")
            != "HudUiElement::Invalidate"
            or direct_target.get("output_section_id")
            != "recoil:section:.text"
            or direct_target.get("physical_block_id")
            != "recoil:block:0x4b3ce0"
            or tuple(direct_target.get("verification_target_ids", ()))
            != (
                "recoil:vc5-target:hud_ui_element_invalidate",
                "recoil:vc5-target:zui_4b3ce0_4bffe0_authored_order",
            )
            or not isinstance(direct_trace, Mapping)
            or direct_trace.get("state") != "resolved"
            or direct_trace.get("reason_code") not in {None, ""}
            or not isinstance(direct_edges, list)
            or len(direct_edges) != 1
            or not isinstance(direct_edges[0], Mapping)
            or direct_edges[0].get("relation") != "defines"
            or direct_edges[0].get("anchor_id")
            != (
                "recoil:anchor:gamezrecoil-zui-zui-widgets-"
                "huduielement-invalidate"
            )
            or direct_edges[0].get("emission_context")
            != {"translation_unit": "src/GameZRecoil/zUI/zui_widgets.cpp"}
            or indexes.by_address.get(
                _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_INVALIDATE_ADDRESS
            )
            != _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_INVALIDATE_IDENTITY
            or indexes.by_candidate_name.get(
                _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_INVALIDATE_SYMBOL
            )
            != _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_INVALIDATE_IDENTITY
            or _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_INVALIDATE_IDENTITY
            in indexes.provider_ids
        ):
            raise ValueError(
                "HUD UpdateObjectiveDirtyRect center bridge requires the "
                "exact authored HudUiElement::Invalidate source, extent, "
                "candidate name, and target identity authority"
            )

    if (
        candidate_shape_name
        == "direct-hud-ui-element-invalidate-and-triplet-panel-draw"
    ):
        draw_contribution_rows = [
            (contribution, row)
            for contribution in getattr(
                target,
                "translation_unit_function_order",
                (),
            )
            for row in getattr(contribution, "functions", ())
            if normalize_address(str(getattr(row, "address", "")))
            == _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_DRAW_ADDRESS
        ]
        draw_target_id = (
            _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_DRAW_IDENTITY
            .removeprefix("symbol:")
        )
        draw_target = symbols.get(draw_target_id)
        draw_trace = (
            draw_target.get("source_traceability")
            if isinstance(draw_target, Mapping)
            else None
        )
        draw_edges = (
            draw_trace.get("source_edges")
            if isinstance(draw_trace, Mapping)
            else None
        )
        if (
            not isinstance(draw_target, Mapping)
            or draw_target.get("binary") != "recoil"
            or draw_target.get("kind") != "function"
            or draw_target.get("pipeline_class") != "authored"
            or draw_target.get("ownership_state") != "primary-owned"
            or draw_target.get("address")
            != _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_DRAW_ADDRESS
            or draw_target.get("end_exclusive") != "0x40f460"
            or draw_target.get("extent_state") != "known"
            or draw_target.get("size") != 0x60
            or draw_target.get("navigation_name")
            != "HudUiTripletPanel::Draw"
            or draw_target.get("output_section_id")
            != "recoil:section:.text"
            or draw_target.get("physical_block_id")
            != "recoil:block:0x404ca0"
            or not exact_required_target_membership(
                draw_target.get("verification_target_ids", ()),
                (
                    "recoil:vc5-target:hud_404ca0_415ab0_authored_order",
                    "recoil:vc5-target:hud_ui_triplet_panel_draw",
                ),
            )
            or not isinstance(draw_trace, Mapping)
            or draw_trace.get("state") != "resolved"
            or draw_trace.get("reason_code") not in {None, ""}
            or not isinstance(draw_edges, list)
            or len(draw_edges) != 1
            or not isinstance(draw_edges[0], Mapping)
            or draw_edges[0].get("relation") != "defines"
            or draw_edges[0].get("anchor_id")
            != "recoil:anchor:battlesport.hud.huduitripletpanel-draw"
            or draw_edges[0].get("emission_context")
            != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
            or len(draw_contribution_rows) != 1
            or indexes.by_address.get(
                _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_DRAW_ADDRESS
            )
            != _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_DRAW_IDENTITY
            or _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_DRAW_IDENTITY
            in indexes.provider_ids
        ):
            raise ValueError(
                "HUD UpdateObjectiveDirtyRect center bridge requires the "
                "exact authored HudUiTripletPanel::Draw source, target "
                "registration, and identity authority"
            )
        draw_contribution, draw_row = draw_contribution_rows[0]
        if (
            getattr(draw_contribution, "source_from", "")
            != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
            or getattr(draw_contribution, "order_scope", "") != "authored"
            or getattr(draw_row, "symbol", "") != ""
            or getattr(draw_row, "symbol_regex", None)
            != r"\?Draw@HudUiTripletPanel@@.*"
            or getattr(draw_row, "name", "") != "HudUiTripletPanel::Draw"
            or getattr(draw_row, "pipeline_class", "") != "authored"
            or getattr(draw_row, "authored_order_role", "")
            != "authored-body"
            or not bool(getattr(draw_row, "required_presence", False))
            or not bool(getattr(draw_row, "full_order_gate", False))
        ):
            raise ValueError(
                "HUD UpdateObjectiveDirtyRect center bridge requires the "
                "exact authored HudUiTripletPanel::Draw source, target "
                "registration, and identity authority"
            )

    retail_bridges = {
        _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_WIDGET_ADDRESS:
            _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_WIDGET_STORAGE_IDENTITY,
    }
    candidate_bridges = {
        _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL: aggregate_identity,
    }
    required_pair = [
        {
            "ordinal": 0,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity":
                _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_WIDGET_STORAGE_IDENTITY,
            "slot_displacement": 0x64,
            "cleanup_bytes": None,
        },
        {
            "ordinal": 1,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity":
                _cc_catalog.HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_WIDGET_STORAGE_IDENTITY,
            "slot_displacement": 0x68,
            "cleanup_bytes": None,
        },
    ]
    retail_contract = _cc_extraction.extract_invocation_contract(
        tuple(instruction for instruction in retail_pair if instruction),
        source="bn",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        reviewed_register_storage_bridges=retail_bridges,
    )
    candidate_contract = _cc_extraction.extract_invocation_contract(
        tuple(instruction for instruction in candidate_pair if instruction),
        source="cod",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        reviewed_register_storage_bridges=candidate_bridges,
    )
    if retail_contract != required_pair or candidate_contract != required_pair:
        raise ValueError(
            "HUD UpdateObjectiveDirtyRect center bridge cannot derive exact "
            "retail/candidate +0x6ac GetCenterX/GetCenterY equality"
        )
    return retail_bridges, candidate_bridges


def _hud_ui_message_rebuild_weapon_layout_vptr_bridges(
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
) -> tuple[
    dict[str, ReviewedLoopVptrStorageBridge],
    dict[str, ReviewedLoopVptrStorageBridge],
]:
    """Prove RebuildWeaponLayout's independently sourced call contracts."""
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_CALLER_START:
        return {}, {}

    symbols = document.collection("symbols")
    storages = document.collection("storage_contributions")
    owners = document.collection("owners")
    targets = document.collection("verification_targets")
    caller_row = symbols.get(
        _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_CALLER_IDENTITY.removeprefix("symbol:")
    )
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
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_CALLER_START
    ]
    tracker_target = targets.get(
        _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_ORDER_TARGET_ID
    )
    registration = (
        tracker_target.get("registration")
        if isinstance(tracker_target, Mapping)
        else None
    )
    if (
        caller_identity != _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(normalized_start) != caller_identity
        or caller_identity in indexes.provider_ids
        or not isinstance(caller_row, Mapping)
        or caller_row.get("binary") != "recoil"
        or caller_row.get("kind") != "function"
        or caller_row.get("pipeline_class") != "authored"
        or caller_row.get("ownership_state") != "primary-owned"
        or caller_row.get("address") != normalized_start
        or caller_row.get("end_exclusive")
        != _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_CALLER_END_EXCLUSIVE
        or caller_row.get("extent_state") != "known"
        or caller_row.get("size") != 0x110
        or caller_row.get("navigation_name")
        != "HudUiMessage::RebuildWeaponLayout"
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
        != _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
        or caller is None
        or caller.symbol != _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_CALLER_SYMBOL
        or len(caller.data) != 0x120
        or len(caller.relocation_mask) != len(caller.data)
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
            "HUD message RebuildWeaponLayout bridge requires the exact "
            "authored caller, source anchor, extent, object symbol, and "
            "governed target authority"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "")
        != _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_CALLER_SYMBOL
        or getattr(contribution_row, "symbol_regex", None) is not None
        or getattr(contribution_row, "name", "")
        != "HudUiMessage::RebuildWeaponLayout"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or getattr(contribution_row, "required_presence", None) is not True
        or getattr(contribution_row, "full_order_gate", None) is not True
    ):
        raise ValueError(
            "HUD message RebuildWeaponLayout bridge rejects exact hud.cpp "
            "target contribution-row authority drift"
        )

    layout_symbol = symbols.get(_cc_catalog.HUD_LAYOUT_HW_DATA_ID)
    layout_storage = storages.get(_cc_catalog.HUD_LAYOUT_HW_STORAGE_ID)
    layout_owner = owners.get(_cc_catalog.HUD_LAYOUT_HW_OWNER_ID)
    layout_reference = (
        layout_storage.get("reference")
        if isinstance(layout_storage, Mapping)
        else None
    )
    layout_relationships = [
        (str(owner_id), relationship)
        for owner_id, row in owners.items()
        if isinstance(row, Mapping)
        for relationship in row.get("relationships", ())
        if isinstance(relationship, Mapping)
        and relationship.get("kind") == "primary-data"
        and relationship.get("symbol_id") == _cc_catalog.HUD_LAYOUT_HW_DATA_ID
    ]
    layout_containers = [
        row
        for row in indexes.storage_containers
        if row.identity == _cc_catalog.HUD_LAYOUT_HW_STORAGE_IDENTITY
    ]
    if (
        not isinstance(layout_symbol, Mapping)
        or layout_symbol.get("binary") != "recoil"
        or layout_symbol.get("kind") != "data"
        or layout_symbol.get("disposition") != "authored"
        or layout_symbol.get("address") != _cc_catalog.HUD_LAYOUT_HW_ADDRESS
        or layout_symbol.get("navigation_name") != "g_HudLayoutHW"
        or layout_symbol.get("output_section_id") != "recoil:section:.data"
        or layout_symbol.get("extent_state") != "unknown"
        or layout_symbol.get("size") is not None
        or layout_symbol.get("end_exclusive") is not None
        or layout_symbol.get("storage_contribution_ids")
        != [_cc_catalog.HUD_LAYOUT_HW_STORAGE_ID]
        or not isinstance(layout_storage, Mapping)
        or layout_storage.get("binary") != "recoil"
        or layout_storage.get("kind") != "data-symbol"
        or layout_storage.get("output_section_id") != "recoil:section:.data"
        or layout_storage.get("overlap") != "none"
        or layout_storage.get("owner_ids") != [_cc_catalog.HUD_LAYOUT_HW_OWNER_ID]
        or layout_storage.get("parent_contribution_id") is not None
        or layout_storage.get("symbol_ids") != [_cc_catalog.HUD_LAYOUT_HW_DATA_ID]
        or not isinstance(layout_reference, Mapping)
        or layout_reference.get("address") != _cc_catalog.HUD_LAYOUT_HW_ADDRESS
        or layout_reference.get("extent_state") != "unknown"
        or not isinstance(layout_owner, Mapping)
        or layout_owner.get("binary") != "recoil"
        or layout_owner.get("kind") != "class"
        or layout_owner.get("lifecycle_state") != "active"
        or layout_owner.get("provider_state") == "accepted"
        or layout_relationships
        != [
            (
                _cc_catalog.HUD_LAYOUT_HW_OWNER_ID,
                {
                    "address": _cc_catalog.HUD_LAYOUT_HW_ADDRESS,
                    "kind": "primary-data",
                    "name": "g_HudLayoutHW",
                    "symbol_id": _cc_catalog.HUD_LAYOUT_HW_DATA_ID,
                },
            )
        ]
        or indexes.storage_by_address.get(_cc_catalog.HUD_LAYOUT_HW_ADDRESS)
        != _cc_catalog.HUD_LAYOUT_HW_STORAGE_IDENTITY
        or _cc_catalog.HUD_LAYOUT_HW_STORAGE_IDENTITY in indexes.provider_ids
        or len(layout_containers) > 1
        or (
            layout_containers
            and (
                layout_containers[0].start != address_value(_cc_catalog.HUD_LAYOUT_HW_ADDRESS)
                or layout_containers[0].start
                + _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_LAYOUT_DISPLACEMENT
                + 4
                > layout_containers[0].end_exclusive
            )
        )
    ):
        raise ValueError(
            "HUD message RebuildWeaponLayout bridge requires the unique "
            "reviewed g_HudLayoutHW storage identity and primary linkage"
        )

    manager_symbol = symbols.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID)
    manager_containers = [
        row
        for row in indexes.storage_containers
        if row.identity == f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    ]
    if (
        not isinstance(manager_symbol, Mapping)
        or manager_symbol.get("binary") != "recoil"
        or manager_symbol.get("kind") != "data"
        or manager_symbol.get("disposition") != "authored"
        or manager_symbol.get("address") != _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
        or manager_symbol.get("navigation_name") != _cc_catalog.HUD_UI_MGR_AGGREGATE_NAME
        or manager_symbol.get("output_section_id") != "recoil:section:.data"
        or manager_symbol.get("storage_contribution_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_STORAGE_ID]
        or manager_symbol.get("verification_target_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_ID]
        or indexes.storage_by_address.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
        != f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
        or len(manager_containers) != 1
        or manager_containers[0].start
        != address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
        or manager_containers[0].end_exclusive
        != address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
        + _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
        or _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_ORIGIN_X_DISPLACEMENT + 4
        > _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
    ):
        raise ValueError(
            "HUD message RebuildWeaponLayout bridge requires the reviewed "
            "manager-origin extent"
        )

    def encoded(instruction: Instruction) -> bytes:
        return bytes(int(value, 16) for value in instruction.bytes)

    retail_addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(normalized_start),
    )
    retail_expected_address = address_value(normalized_start)
    retail_contiguous = True
    for instruction, address in zip(retail_instructions, retail_addresses):
        if address != retail_expected_address:
            retail_contiguous = False
            break
        retail_expected_address += len(encoded(instruction))
    retail_calls = tuple(
        address
        for instruction, address in zip(retail_instructions, retail_addresses)
        if address is not None and _cc_cfg._instruction_mnemonic(instruction) == "call"
    )
    if (
        not retail_contiguous
        or retail_expected_address != 0x41417C
        or b"".join(encoded(row) for row in retail_instructions)
        != _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_RETAIL_CODE
        or retail_calls != _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_RETAIL_CALL_ORDER
        or _cc_cfg._instruction_mnemonic(retail_instructions[-1]) not in {"ret", "retn"}
    ):
        raise ValueError(
            "HUD message RebuildWeaponLayout bridge requires the exact "
            "immutable-retail body, seven-call population/order, and tail"
        )

    candidate_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    candidate_expected_offset = 0
    candidate_contiguous = True
    for instruction, offset in zip(candidate.instructions, candidate_offsets):
        if offset != candidate_expected_offset:
            candidate_contiguous = False
            break
        candidate_expected_offset += len(encoded(instruction))
    candidate_invocations = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    candidate_calls = tuple(
        candidate_offsets[index] for index in candidate_invocations
    )
    if (
        not candidate_contiguous
        or candidate_expected_offset != 0x114
        or b"".join(encoded(row) for row in candidate.instructions)
        != _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_CANDIDATE_BODY[:0x114]
        or caller.data != _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_CANDIDATE_BODY
        or candidate_calls != _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_CANDIDATE_CALL_ORDER
        or candidate.local_control_flow_indices
        or candidate.local_control_flow_targets
        or _cc_cfg._instruction_mnemonic(candidate.instructions[-1]) not in {"ret", "retn"}
    ):
        raise ValueError(
            "HUD message RebuildWeaponLayout bridge requires the exact current "
            "candidate body, seven-call population/order, CFG, padding, and tail"
        )

    expected_relocations = {
        0x04: (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL,
            _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_LAYOUT_DISPLACEMENT,
        ),
        0x0F: (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL,
            _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_LAYOUT_DISPLACEMENT,
        ),
        0x18: (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL,
            _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_LAYOUT_DISPLACEMENT,
        ),
        0x1D: (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL,
            _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_LAYOUT_DISPLACEMENT,
        ),
        0x29: (
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_ORIGIN_X_DISPLACEMENT,
        ),
    }
    expected_mask = {
        position
        for offset in expected_relocations
        for position in range(offset, offset + 4)
    }
    if (
        len(caller.relocations) != len(expected_relocations)
        or {row.offset for row in caller.relocations}
        != set(expected_relocations)
        or any(
            (
                row.type,
                row.symbol_name,
                struct.unpack_from("<I", caller.data, row.offset)[0],
            )
            != expected_relocations[row.offset]
            for row in caller.relocations
        )
        or {
            index
            for index, masked in enumerate(caller.relocation_mask)
            if masked
        }
        != expected_mask
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL
        )
        != 1
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or (
            caller.defined_external_data
            + caller.defined_external_functions
            + caller.undefined_external_functions
        ).count(_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL)
        != 0
        or (
            caller.defined_external_data
            + caller.defined_external_functions
            + caller.undefined_external_functions
        ).count(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
        != 0
    ):
        raise ValueError(
            "HUD message RebuildWeaponLayout bridge rejects candidate "
            "g_HudLayoutHW/g_HudUiMgr COFF relocation "
            "identity/type/addend/mask/provenance drift"
        )

    retail_specs = (
        (0x414083, "eax", _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_LAYOUT_STORAGE_IDENTITY, 0x64),
        (0x414093, "edx", _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_LAYOUT_STORAGE_IDENTITY, 0x68),
        (0x4140DD, "eax", _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_THIS_VPTR_IDENTITY, 0x0C),
        (0x4140EB, "eax", _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_THIS_VPTR_IDENTITY, 0x18),
        (0x414134, "edx", _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_PANEL_VPTR_IDENTITY, 0x0C),
        (0x414142, "eax", _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_PANEL_VPTR_IDENTITY, 0x18),
        (0x414171, "edx", _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_WIDGET_VPTR_IDENTITY, 0x0C),
    )
    candidate_specs = (
        (0x13, "eax", _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_LAYOUT_STORAGE_IDENTITY, 0x64),
        (0x23, "edx", _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_LAYOUT_STORAGE_IDENTITY, 0x68),
        (0x6F, "edx", _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_THIS_VPTR_IDENTITY, 0x0C),
        (0x7D, "edx", _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_THIS_VPTR_IDENTITY, 0x18),
        (0xC8, "eax", _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_PANEL_VPTR_IDENTITY, 0x0C),
        (0xD6, "edx", _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_PANEL_VPTR_IDENTITY, 0x18),
        (0x109, "edx", _cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_WIDGET_VPTR_IDENTITY, 0x0C),
    )
    retail_bridges = {
        normalize_address(hex(address)): ReviewedLoopVptrStorageBridge(
            register=register,
            storage_identity=storage_identity,
            slot_displacement=slot,
            assembly_source="bn",
        )
        for address, register, storage_identity, slot in retail_specs
    }
    candidate_bridges = {
        normalize_address(hex(offset)): ReviewedLoopVptrStorageBridge(
            register=register,
            storage_identity=storage_identity,
            slot_displacement=slot,
            assembly_source="cod",
        )
        for offset, register, storage_identity, slot in candidate_specs
    }

    def indirect_contract(
        specs: Sequence[tuple[int, str, str, int]],
    ) -> list[dict[str, object]]:
        return [
            {
                "ordinal": ordinal,
                "form": "call",
                "dispatch": "indirect",
                "identity_kind": "virtual-slot",
                "target_identity": "",
                "storage_identity": storage_identity,
                "slot_displacement": slot,
                "cleanup_bytes": None,
            }
            for ordinal, (_address, _register, storage_identity, slot)
            in enumerate(specs)
        ]

    expected_retail_contract = indirect_contract(retail_specs)
    expected_candidate_contract = indirect_contract(candidate_specs)
    retail_contract = _cc_extraction.extract_invocation_contract(
        retail_instructions,
        source="bn",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge_names=bridge_names,
        reviewed_loop_vptr_storage_bridges=retail_bridges,
    )
    candidate_contract = _cc_extraction.extract_invocation_contract(
        candidate.instructions,
        source="cod",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        reviewed_loop_vptr_storage_bridges=candidate_bridges,
    )
    if (
        retail_contract != expected_retail_contract
        or candidate_contract != expected_candidate_contract
        or any(row.get("cleanup_bytes") is not None for row in retail_contract)
        or any(row.get("cleanup_bytes") is not None for row in candidate_contract)
    ):
        raise ValueError(
            "HUD message RebuildWeaponLayout bridge cannot independently "
            "derive the complete retail and exact-current candidate contracts"
        )
    return retail_bridges, candidate_bridges


def _require_exact_retail_storage_layout(
    *,
    address: str,
    size: int,
    section_name: str,
    zero_filled: bool = False,
) -> None:
    """Fail closed unless one retail range has the reviewed PE layout."""

    try:
        with StableReadHandle(_cc_catalog.DEFAULT_REFERENCE) as stable_reference:
            reference_data = stable_reference.read()
        headers = parse_pe_headers(
            reference_data,
            source=str(_cc_catalog.DEFAULT_REFERENCE),
        )
    except (OSError, ValueError) as exc:
        raise ValueError(
            "reviewed retail storage layout cannot read the immutable PE"
        ) from exc
    start_rva = address_value(address) - headers.image_base
    end_rva = start_rva + size
    matches = [
        section
        for section in headers.sections
        if section.name == section_name
        and section.virtual_address <= start_rva
        and end_rva
        <= section.virtual_address + section.virtual_size
    ]
    if len(matches) != 1:
        raise ValueError(
            "reviewed retail storage lacks its exact section and extent"
        )
    section = matches[0]
    raw_end_rva = section.virtual_address + section.raw_size
    if zero_filled:
        if start_rva < raw_end_rva:
            raise ValueError(
                "reviewed retail storage is no longer initially zero-filled"
            )
    elif end_rva > raw_end_rva:
        raise ValueError(
            "reviewed retail storage is no longer file-backed"
        )


def _hud_ui_mgr_viewport_layout_candidate_vptr_bridges(
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
    dict[str, ReviewedVptrStorageBridge],
]:
    """Bridge only OnViewportChanged's exact current-layout activation call."""
    from _recoil.call_contract.records import (
        ReviewedStaticStorageReferenceBridge,
        ReviewedVptrStorageBridge,
        StorageContainer,
    )
    caller_start = normalize_address(caller_start)
    if caller_start != _cc_catalog.HUD_UI_MGR_VIEWPORT_CALLER_START:
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
        if name.casefold() == _cc_catalog.HUD_UI_MGR_VIEWPORT_CALLER_SYMBOL.casefold()
    ]
    if (
        caller_identity != _cc_catalog.HUD_UI_MGR_VIEWPORT_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_VIEWPORT_CALLER_END_EXCLUSIVE
        or caller_addresses != [_cc_catalog.HUD_UI_MGR_VIEWPORT_CALLER_START]
        or caller_identity in indexes.provider_ids
        or caller_name_rows
        not in (
            [],
            [
                (
                    _cc_catalog.HUD_UI_MGR_VIEWPORT_CALLER_SYMBOL,
                    _cc_catalog.HUD_UI_MGR_VIEWPORT_CALLER_IDENTITY,
                )
            ],
        )
        or caller is None
        or caller.symbol != _cc_catalog.HUD_UI_MGR_VIEWPORT_CALLER_SYMBOL
        or len(caller.data)
        != address_value(_cc_catalog.HUD_UI_MGR_VIEWPORT_CALLER_END_EXCLUSIVE)
        - address_value(_cc_catalog.HUD_UI_MGR_VIEWPORT_CALLER_START)
        or len(caller.relocation_mask) != len(caller.data)
    ):
        raise ValueError(
            "HUD viewport layout-vptr bridge requires the exact reviewed "
            "authored caller identity, extent, symbol, and candidate body"
        )

    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_VIEWPORT_CALLER_START
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
            "HUD viewport layout-vptr bridge requires one exact current HUD "
            "source authority"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != r"\?OnViewportChanged@HudUiMgr@@.*"
        or re.fullmatch(
            r"\?OnViewportChanged@HudUiMgr@@.*",
            caller.symbol,
        )
        is None
        or getattr(contribution_row, "name", "")
        != "HudUiMgr::OnViewportChanged"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD viewport layout-vptr bridge requires the exact authored "
            "hud.cpp contribution row"
        )

    caller_symbol_id = caller_identity.removeprefix("symbol:")
    caller_symbol = document.collection("symbols").get(caller_symbol_id)
    trace = (
        caller_symbol.get("source_traceability")
        if isinstance(caller_symbol, Mapping)
        else None
    )
    source_edges = trace.get("source_edges") if isinstance(trace, Mapping) else None
    if (
        not isinstance(caller_symbol, Mapping)
        or _cc_identity._symbol_identity(caller_symbol_id, caller_symbol)
        != caller_identity
        or caller_symbol.get("binary") != "recoil"
        or caller_symbol.get("kind") != "function"
        or caller_symbol.get("pipeline_class") != "authored"
        or caller_symbol.get("extent_state") != "known"
        or normalize_address(str(caller_symbol.get("address", "")))
        != _cc_catalog.HUD_UI_MGR_VIEWPORT_CALLER_START
        or normalize_address(str(caller_symbol.get("end_exclusive", "")))
        != _cc_catalog.HUD_UI_MGR_VIEWPORT_CALLER_END_EXCLUSIVE
        or caller_symbol.get("size") != len(caller.data)
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
        != _cc_catalog.HUD_UI_MGR_VIEWPORT_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
    ):
        raise ValueError(
            "HUD viewport layout-vptr bridge requires one exact unaliased "
            "reviewed caller and resolved source edge"
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
    ):
        raise ValueError(
            "HUD viewport layout-vptr bridge requires the exact reviewed HUD "
            "aggregate symbol, storage, and target authority"
        )
    aggregate_containers = [
        row
        for row in indexes.storage_containers
        if row.identity == f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    ]
    if (
        indexes.storage_by_address.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
        != f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
        or aggregate_containers
        != [
            StorageContainer(
                address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS),
                address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
                + _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE,
                f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}",
            )
        ]
    ):
        raise ValueError(
            "HUD viewport layout-vptr bridge requires one exact aggregate "
            "storage container and positive extent"
        )

    expected_row = {
        "ordinal": _cc_catalog.HUD_UI_MGR_VIEWPORT_LAYOUT_CALL_ORDINAL,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": _cc_catalog.HUD_UI_MGR_VIEWPORT_LAYOUT_STORAGE_IDENTITY,
        "slot_displacement": _cc_catalog.HUD_UI_MGR_VIEWPORT_LAYOUT_SLOT_DISPLACEMENT,
        "cleanup_bytes": None,
    }
    if (
        len(expected) <= _cc_catalog.HUD_UI_MGR_VIEWPORT_LAYOUT_CALL_ORDINAL
        or expected[_cc_catalog.HUD_UI_MGR_VIEWPORT_LAYOUT_CALL_ORDINAL]
        != expected_row
        or sum(
            row.get("storage_identity")
            == _cc_catalog.HUD_UI_MGR_VIEWPORT_LAYOUT_STORAGE_IDENTITY
            for row in expected
        )
        != 1
    ):
        raise ValueError(
            "HUD viewport layout-vptr bridge requires the exact ordinal-1 "
            "retail virtual-slot contract"
        )

    exact_body = bytes.fromhex(
        "8b 0d 18 00 00 00 85 c9 74 05 8b 01 ff 50 18"
    )
    load_offset = _cc_catalog.HUD_UI_MGR_VIEWPORT_LAYOUT_LOAD_OFFSET
    call_offset = _cc_catalog.HUD_UI_MGR_VIEWPORT_LAYOUT_CALL_OFFSET
    if (
        caller.data[load_offset : call_offset + 3] != exact_body
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in caller.defined_external_data
    ):
        raise ValueError(
            "HUD viewport layout-vptr bridge requires the exact bounded "
            "aggregate-load/null-guard/vptr/call object body"
        )
    bounded_relocations = tuple(
        row
        for row in caller.relocations
        if load_offset <= row.offset < call_offset + 3
    )
    relocation = bounded_relocations[0] if len(bounded_relocations) == 1 else None
    if (
        relocation is None
        or relocation.offset
        != _cc_catalog.HUD_UI_MGR_VIEWPORT_LAYOUT_RELOCATION_OFFSET
        or relocation.type != IMAGE_REL_I386_DIR32
        or relocation.symbol_name
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        or struct.unpack_from("<I", caller.data, relocation.offset)[0]
        != _cc_catalog.HUD_UI_MGR_VIEWPORT_LAYOUT_DISPLACEMENT
    ):
        raise ValueError(
            "HUD viewport layout-vptr bridge rejects missing, duplicate, "
            "extra, or malformed bounded relocation"
        )
    expected_mask = set(
        range(
            _cc_catalog.HUD_UI_MGR_VIEWPORT_LAYOUT_RELOCATION_OFFSET,
            _cc_catalog.HUD_UI_MGR_VIEWPORT_LAYOUT_RELOCATION_OFFSET + 4,
        )
    )
    if {
        index
        for index in range(load_offset, call_offset + 3)
        if caller.relocation_mask[index]
    } != expected_mask:
        raise ValueError(
            "HUD viewport layout-vptr bridge requires an exact bounded "
            "relocation mask"
        )

    matching_loads: list[int] = []
    matching_calls: list[int] = []
    for index, instruction in enumerate(candidate.instructions):
        if (
            _cc_cfg._instruction_mnemonic(instruction) == "mov"
            and instruction.bytes == ("8b", "0d", "18", "00", "00", "00")
            and _cc_targets._exact_memory_expression(
                _cc_cfg._instruction_operand(instruction).split(",", 1)[-1]
            )
            in {
                (
                    f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
                    f"+{_cc_catalog.HUD_UI_MGR_VIEWPORT_LAYOUT_DISPLACEMENT}"
                ),
                (
                    f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
                    f"+0x{_cc_catalog.HUD_UI_MGR_VIEWPORT_LAYOUT_DISPLACEMENT:x}"
                ),
            }
        ):
            matching_loads.append(index)
        if (
            _cc_cfg._instruction_mnemonic(instruction) == "call"
            and instruction.bytes == ("ff", "50", "18")
            and _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(instruction))
            in {"eax+24", "eax+0x18"}
        ):
            matching_calls.append(index)
    if (
        len(matching_loads) != 1
        or len(matching_calls) != 1
        or matching_calls[0] != matching_loads[0] + 4
    ):
        raise ValueError(
            "HUD viewport layout-vptr bridge requires one unique contiguous "
            "candidate EAX-vptr load chain"
        )
    load_index = matching_loads[0]
    call_index = matching_calls[0]
    sequence = candidate.instructions[load_index : call_index + 1]
    if (
        tuple(instruction.bytes for instruction in sequence)
        != (
            ("8b", "0d", "18", "00", "00", "00"),
            ("85", "c9"),
            ("74", "05"),
            ("8b", "01"),
            ("ff", "50", "18"),
        )
        or tuple(
            _cc_cfg._instruction_mnemonic(instruction) for instruction in sequence
        )
        != ("mov", "test", "je", "mov", "call")
        or _cc_cfg._source_instruction_address(sequence[-1])
        != hex(_cc_catalog.HUD_UI_MGR_VIEWPORT_LAYOUT_CALL_OFFSET)
        or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
        or call_index in candidate.local_control_flow_indices
    ):
        raise ValueError(
            "HUD viewport layout-vptr bridge rejects reordered, aliased, "
            "neighboring, or malformed EAX-indirect instructions"
        )
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
        ordinal_by_index.get(call_index)
        != _cc_catalog.HUD_UI_MGR_VIEWPORT_LAYOUT_CALL_ORDINAL
    ):
        raise ValueError(
            "HUD viewport layout-vptr bridge requires the exact candidate "
            "call ordinal and opcode offset"
        )

    static_bridge = ReviewedStaticStorageReferenceBridge(
        aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
        displacement=_cc_catalog.HUD_UI_MGR_VIEWPORT_LAYOUT_DISPLACEMENT,
        access_width=4,
        storage_identity=f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}",
    )
    return (
        {
            (
                f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
                f"+{_cc_catalog.HUD_UI_MGR_VIEWPORT_LAYOUT_DISPLACEMENT}"
            ): static_bridge
        },
        {
            normalize_address(
                hex(_cc_catalog.HUD_UI_MGR_VIEWPORT_LAYOUT_CALL_OFFSET)
            ): ReviewedVptrStorageBridge(
                register="eax",
                provenance=(
                    f"load(storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID})"
                ),
                storage_identity=_cc_catalog.HUD_UI_MGR_VIEWPORT_LAYOUT_STORAGE_IDENTITY,
                slot_displacement=(
                    _cc_catalog.HUD_UI_MGR_VIEWPORT_LAYOUT_SLOT_DISPLACEMENT
                ),
                identity_kind="virtual-slot",
            )
        },
    )


def _hud_ui_mgr_enable_current_layout_candidate_bridges(
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
    """Bridge only EnableHud's current-layout virtual Enable invocation."""
    from _recoil.call_contract.records import (
        ReviewedMemberVptrStorageBridge,
        ReviewedStaticStorageReferenceBridge,
    )
    caller_start = normalize_address(caller_start)
    if caller_start != _cc_catalog.HUD_UI_MGR_ENABLE_CALLER_START:
        return {}, {}

    # Preserve the complete caller/source/aggregate authority and the exact
    # ordinal-0 prefix proved by WSI-20260729-002.  This bridge adds only the
    # immediately following current-layout field/vptr/call chain.
    _cc_recoil_hud_widgets._hud_ui_mgr_enable_hud_candidate_vptr_bridges(
        expected,
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    caller = candidate.caller_definition
    if caller is None:
        raise ValueError(
            "HUD EnableHud current-layout bridge requires the reviewed "
            "candidate caller body"
        )

    expected_row = {
        "ordinal": _cc_catalog.HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_CALL_ORDINAL,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": (
            _cc_catalog.HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_STORAGE_IDENTITY
        ),
        "slot_displacement": (
            _cc_catalog.HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_SLOT_DISPLACEMENT
        ),
        "cleanup_bytes": None,
    }
    if (
        len(expected) <= _cc_catalog.HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_CALL_ORDINAL
        or expected[_cc_catalog.HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_CALL_ORDINAL]
        != expected_row
        or sum(
            row.get("storage_identity")
            == _cc_catalog.HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_STORAGE_IDENTITY
            and row.get("slot_displacement")
            == _cc_catalog.HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_SLOT_DISPLACEMENT
            for row in expected
        )
        != 1
    ):
        raise ValueError(
            "HUD EnableHud current-layout bridge requires the exact immutable "
            "ordinal-1 retail virtual-slot contract"
        )

    reference_offset = _cc_catalog.HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_REFERENCE_OFFSET
    relocation_offset = _cc_catalog.HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_RELOCATION_OFFSET
    vptr_load_offset = _cc_catalog.HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_VPTR_LOAD_OFFSET
    call_offset = _cc_catalog.HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_CALL_OFFSET
    bounded_end = call_offset + 3
    exact_body = bytes.fromhex(
        "8b 0d 18 00 00 00 8b 11 ff 52 10"
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
            "HUD EnableHud current-layout bridge requires the exact bounded "
            "field-load/vptr-load/call object body"
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
        != _cc_catalog.HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_DISPLACEMENT
    ):
        raise ValueError(
            "HUD EnableHud current-layout bridge rejects a missing, duplicate, "
            "extra, or malformed field DIR32 relocation"
        )
    expected_mask = set(range(relocation_offset, relocation_offset + 4))
    if {
        offset
        for offset in range(reference_offset, bounded_end)
        if caller.relocation_mask[offset]
    } != expected_mask:
        raise ValueError(
            "HUD EnableHud current-layout bridge requires the exact bounded "
            "field relocation mask"
        )

    instruction_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    index_by_offset = {
        offset: index
        for index, offset in enumerate(instruction_offsets)
        if offset is not None
    }
    if (
        set(
            (
                reference_offset,
                vptr_load_offset,
                call_offset,
            )
        )
        - set(index_by_offset)
    ):
        raise ValueError(
            "HUD EnableHud current-layout bridge requires exact candidate "
            "instruction offsets"
        )
    reference_index = index_by_offset[reference_offset]
    vptr_load_index = index_by_offset[vptr_load_offset]
    call_index = index_by_offset[call_offset]
    if (
        (reference_index, vptr_load_index, call_index)
        != (
            reference_index,
            reference_index + 1,
            reference_index + 2,
        )
    ):
        raise ValueError(
            "HUD EnableHud current-layout bridge rejects a missing, duplicate, "
            "or noncontiguous candidate sequence"
        )
    sequence = candidate.instructions[reference_index : call_index + 1]
    expected_expression = (
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
        f"+{_cc_catalog.HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_DISPLACEMENT}"
    )
    expected_hex_expression = (
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
        f"+0x{_cc_catalog.HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_DISPLACEMENT:x}"
    )
    if (
        tuple(instruction.bytes for instruction in sequence)
        != (
            ("8b", "0d", "18", "00", "00", "00"),
            ("8b", "11"),
            ("ff", "52", "10"),
        )
        or tuple(
            _cc_cfg._instruction_mnemonic(instruction)
            for instruction in sequence
        )
        != ("mov", "mov", "call")
        or _cc_cfg._instruction_operand(sequence[0]).split(",", 1)[0]
        .strip()
        .lower()
        != "ecx"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(sequence[0]).split(",", 1)[-1]
        )
        not in {expected_expression, expected_hex_expression}
        or _cc_cfg._instruction_operand(sequence[1]).split(",", 1)[0]
        .strip()
        .lower()
        != "edx"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(sequence[1]).split(",", 1)[-1]
        )
        != "ecx"
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(sequence[2]))
        not in {"edx+16", "edx+0x10"}
    ):
        raise ValueError(
            "HUD EnableHud current-layout bridge rejects field, register, "
            "receiver, slot, indexed, aliased, or instruction-byte drift"
        )

    matching_references = [
        index
        for index, instruction in enumerate(candidate.instructions)
        if (
            _cc_cfg._instruction_mnemonic(instruction) == "mov"
            and instruction.bytes
            == ("8b", "0d", "18", "00", "00", "00")
            and _cc_cfg._instruction_operand(instruction).split(",", 1)[0]
            .strip()
            .lower()
            == "ecx"
            and _cc_targets._exact_memory_expression(
                _cc_cfg._instruction_operand(instruction).split(",", 1)[-1]
            )
            in {expected_expression, expected_hex_expression}
        )
    ]
    matching_vptr_loads = [
        index
        for index, instruction in enumerate(candidate.instructions)
        if (
            _cc_cfg._instruction_mnemonic(instruction) == "mov"
            and instruction.bytes == ("8b", "11")
            and _cc_cfg._instruction_operand(instruction).split(",", 1)[0]
            .strip()
            .lower()
            == "edx"
            and _cc_targets._exact_memory_expression(
                _cc_cfg._instruction_operand(instruction).split(",", 1)[-1]
            )
            == "ecx"
        )
    ]
    matching_calls = [
        index
        for index, instruction in enumerate(candidate.instructions)
        if (
            _cc_cfg._instruction_mnemonic(instruction) == "call"
            and instruction.bytes == ("ff", "52", "10")
            and _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(instruction))
            in {"edx+16", "edx+0x10"}
        )
    ]
    if (
        matching_references != [reference_index]
        or matching_vptr_loads != [vptr_load_index]
        or matching_calls != [call_index]
    ):
        raise ValueError(
            "HUD EnableHud current-layout bridge requires one unique exact "
            "ECX field / EDX vptr / slot-0x10 call chain"
        )

    bounded_indices = frozenset(
        range(reference_index, call_index + 1)
    )
    if (
        bool(candidate.local_control_flow_indices & bounded_indices)
        or any(
            target in bounded_indices
            for targets in candidate.local_control_flow_targets.values()
            for target in targets
        )
        or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
    ):
        raise ValueError(
            "HUD EnableHud current-layout bridge rejects cleanup or "
            "control-flow ambiguity in the reviewed chain"
        )
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    if (
        len(invocation_indices)
        <= _cc_catalog.HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_CALL_ORDINAL
        or invocation_indices[
            _cc_catalog.HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_CALL_ORDINAL
        ]
        != call_index
    ):
        raise ValueError(
            "HUD EnableHud current-layout bridge requires the exact candidate "
            "ordinal-1 call"
        )
    next_invocation = (
        invocation_indices[2]
        if len(invocation_indices) > 2
        else len(candidate.instructions)
    )
    for instruction in candidate.instructions[call_index + 1 : next_invocation]:
        operand = _cc_cfg._instruction_operand(instruction).lower()
        if re.search(r"\beax\b", operand):
            destination = (
                operand.split(",", 1)[0].strip()
                if "," in operand
                else ""
            )
            if destination != "eax":
                raise ValueError(
                    "HUD EnableHud current-layout bridge rejects unexpected "
                    "call-result consumption"
                )
        if _cc_cfg._instruction_may_clobber_register(instruction, "eax"):
            break

    static_bridge = ReviewedStaticStorageReferenceBridge(
        aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
        displacement=_cc_catalog.HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_DISPLACEMENT,
        access_width=4,
        storage_identity=f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}",
    )
    member_bridge = ReviewedMemberVptrStorageBridge(
        register="edx",
        source_register="ecx",
        source_provenance=f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}",
        receiver_register="ecx",
        receiver_provenance=f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}",
        storage_identity=_cc_catalog.HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_STORAGE_IDENTITY,
        slot_displacement=_cc_catalog.HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_SLOT_DISPLACEMENT,
        call_address=normalize_address(hex(call_offset)),
    )
    return (
        {expected_expression: static_bridge},
        {normalize_address(hex(vptr_load_offset)): member_bridge},
    )


def _hud_ui_mgr_disable_current_layout_candidate_bridges(
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
    """Bridge only DisableHud's current-layout virtual Disable invocation."""
    from _recoil.call_contract.records import (
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedMemberVptrStorageBridge,
        ReviewedStaticStorageReferenceBridge,
        ReviewedVptrStorageBridge,
    )
    caller_start = normalize_address(caller_start)
    if caller_start != _cc_catalog.HUD_UI_MGR_DISABLE_CALLER_START:
        return {}, {}

    (
        set_enabled_absolute_bridges,
        set_enabled_vptr_bridges,
    ) = _cc_recoil_hud_widgets._hud_ui_mgr_disable_set_enabled_candidate_vptr_bridges(
        expected,
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    exact_set_enabled_absolute_bridge = {
        normalize_address(
            hex(_cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_LOAD_OFFSET)
        ): ReviewedAbsoluteStorageLoadBridge(
            register="eax",
            aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            displacement=0,
            access_width=4,
            storage_identity=(
                _cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_STORAGE_IDENTITY
            ),
        )
    }
    exact_set_enabled_vptr_bridge = {
        normalize_address(
            hex(_cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_CALL_OFFSET)
        ): ReviewedVptrStorageBridge(
            register="eax",
            provenance=_cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_LOAD_PROVENANCE,
            storage_identity=(
                _cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_STORAGE_IDENTITY
            ),
            slot_displacement=(
                _cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_SLOT_DISPLACEMENT
            ),
            identity_kind="callback",
        )
    }
    if (
        set_enabled_absolute_bridges
        != exact_set_enabled_absolute_bridge
        or set_enabled_vptr_bridges != exact_set_enabled_vptr_bridge
    ):
        raise ValueError(
            "HUD DisableHud current-layout bridge requires WSI-006's exact "
            "caller/source/aggregate/container/callback authority"
        )

    expected_row = {
        "ordinal": _cc_catalog.HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_CALL_ORDINAL,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": (
            _cc_catalog.HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_STORAGE_IDENTITY
        ),
        "slot_displacement": (
            _cc_catalog.HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_SLOT_DISPLACEMENT
        ),
        "cleanup_bytes": None,
    }
    if (
        len(expected) <= _cc_catalog.HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_CALL_ORDINAL
        or expected[_cc_catalog.HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_CALL_ORDINAL]
        != expected_row
        or sum(row == expected_row for row in expected) != 1
    ):
        raise ValueError(
            "HUD DisableHud current-layout bridge requires the exact "
            "immutable ordinal-4 retail virtual-slot contract"
        )

    caller = candidate.caller_definition
    reference_offset = _cc_catalog.HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_REFERENCE_OFFSET
    relocation_offset = _cc_catalog.HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_RELOCATION_OFFSET
    vptr_load_offset = _cc_catalog.HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_VPTR_LOAD_OFFSET
    call_offset = _cc_catalog.HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_CALL_OFFSET
    bounded_end = call_offset + 3
    exact_body = bytes.fromhex(
        "8b 0d 18 00 00 00 8b 11 ff 52 14"
    )
    if (
        caller is None
        or caller.data[reference_offset:bounded_end] != exact_body
    ):
        raise ValueError(
            "HUD DisableHud current-layout bridge requires the exact bounded "
            "field-load/vptr-load/call object body"
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
        != _cc_catalog.HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_DISPLACEMENT
    ):
        raise ValueError(
            "HUD DisableHud current-layout bridge rejects a missing, "
            "duplicate, extra, or malformed field DIR32 relocation"
        )
    expected_mask = set(range(relocation_offset, relocation_offset + 4))
    if {
        offset
        for offset in range(reference_offset, bounded_end)
        if caller.relocation_mask[offset]
    } != expected_mask:
        raise ValueError(
            "HUD DisableHud current-layout bridge requires the exact bounded "
            "field relocation mask"
        )

    instruction_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    index_by_offset = {
        offset: index
        for index, offset in enumerate(instruction_offsets)
        if offset is not None
    }
    sequence_offsets = (
        reference_offset,
        vptr_load_offset,
        call_offset,
    )
    if set(sequence_offsets) - set(index_by_offset):
        raise ValueError(
            "HUD DisableHud current-layout bridge requires exact candidate "
            "instruction offsets"
        )
    sequence_indices = tuple(
        index_by_offset[offset] for offset in sequence_offsets
    )
    if sequence_indices != tuple(
        range(sequence_indices[0], sequence_indices[0] + 3)
    ):
        raise ValueError(
            "HUD DisableHud current-layout bridge rejects a missing, "
            "duplicate, or noncontiguous candidate chain"
        )
    sequence = tuple(
        candidate.instructions[index] for index in sequence_indices
    )
    expected_expression = (
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
        f"+{_cc_catalog.HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_DISPLACEMENT}"
    )
    expected_hex_expression = (
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
        f"+0x{_cc_catalog.HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_DISPLACEMENT:x}"
    )
    if (
        tuple(instruction.bytes for instruction in sequence)
        != (
            ("8b", "0d", "18", "00", "00", "00"),
            ("8b", "11"),
            ("ff", "52", "14"),
        )
        or tuple(
            _cc_cfg._instruction_mnemonic(instruction)
            for instruction in sequence
        )
        != ("mov", "mov", "call")
        or _cc_cfg._instruction_operand(sequence[0]).split(",", 1)[0]
        .strip()
        .lower()
        != "ecx"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(sequence[0]).split(",", 1)[-1]
        )
        not in {expected_expression, expected_hex_expression}
        or _cc_cfg._instruction_operand(sequence[1]).split(",", 1)[0]
        .strip()
        .lower()
        != "edx"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(sequence[1]).split(",", 1)[-1]
        )
        != "ecx"
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(sequence[2]))
        not in {"edx+20", "edx+0x14"}
    ):
        raise ValueError(
            "HUD DisableHud current-layout bridge rejects field/symbol/"
            "addend/register/receiver/slot/indexed/aliased/byte drift"
        )

    matching_chain_starts = [
        index
        for index in range(len(candidate.instructions) - 2)
        if tuple(
            instruction.bytes
            for instruction in candidate.instructions[index : index + 3]
        )
        == tuple(instruction.bytes for instruction in sequence)
    ]
    if matching_chain_starts != [sequence_indices[0]]:
        raise ValueError(
            "HUD DisableHud current-layout bridge requires one unique exact "
            "ECX field / EDX vptr / slot-0x14 call chain"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    call_index = sequence_indices[2]
    if (
        len(invocation_indices)
        <= _cc_catalog.HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_CALL_ORDINAL
        or invocation_indices[
            _cc_catalog.HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_CALL_ORDINAL
        ]
        != call_index
    ):
        raise ValueError(
            "HUD DisableHud current-layout bridge requires the exact "
            "candidate ordinal-4 call"
        )
    previous_call_index = invocation_indices[
        _cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_CALL_ORDINAL
    ]
    ecx_definitions = [
        index
        for index in range(previous_call_index + 1, call_index)
        if _cc_cfg._instruction_may_clobber_register(
            candidate.instructions[index],
            "ecx",
        )
    ]
    edx_definitions = [
        index
        for index in range(previous_call_index + 1, call_index)
        if _cc_cfg._instruction_may_clobber_register(
            candidate.instructions[index],
            "edx",
        )
    ]
    if (
        ecx_definitions != [sequence_indices[0]]
        or edx_definitions != [sequence_indices[1]]
    ):
        raise ValueError(
            "HUD DisableHud current-layout bridge requires unique ECX/EDX "
            "reaching definitions and exact receiver provenance"
        )

    bounded_indices = frozenset(sequence_indices)
    if (
        bool(candidate.local_control_flow_indices & bounded_indices)
        or any(
            target in bounded_indices
            for targets in candidate.local_control_flow_targets.values()
            for target in targets
        )
        or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
    ):
        raise ValueError(
            "HUD DisableHud current-layout bridge rejects cleanup or "
            "control-flow ambiguity in the reviewed chain"
        )
    next_invocation = (
        invocation_indices[
            _cc_catalog.HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_CALL_ORDINAL + 1
        ]
        if len(invocation_indices)
        > _cc_catalog.HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_CALL_ORDINAL + 1
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
                    "HUD DisableHud current-layout bridge rejects unexpected "
                    "call-result consumption"
                )
        if _cc_cfg._instruction_may_clobber_register(instruction, "eax"):
            break

    static_bridge = ReviewedStaticStorageReferenceBridge(
        aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
        displacement=_cc_catalog.HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_DISPLACEMENT,
        access_width=4,
        storage_identity=f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}",
    )
    member_bridge = ReviewedMemberVptrStorageBridge(
        register="edx",
        source_register="ecx",
        source_provenance=f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}",
        receiver_register="ecx",
        receiver_provenance=f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}",
        storage_identity=_cc_catalog.HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_STORAGE_IDENTITY,
        slot_displacement=(
            _cc_catalog.HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_SLOT_DISPLACEMENT
        ),
        call_address=normalize_address(hex(call_offset)),
    )
    return (
        {expected_expression: static_bridge},
        {normalize_address(hex(vptr_load_offset)): member_bridge},
    )


def _hud_ui_mgr_trigger_current_layout_on_activated_bridges(
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
    dict[str, ReviewedVptrStorageBridge],
]:
    """Prove TriggerCurrentLayoutOnActivated's one guarded tail call."""
    from _recoil.call_contract.records import (
        ReviewedStaticStorageReferenceBridge,
        ReviewedVptrStorageBridge,
        StorageContainer,
    )
    caller_start = normalize_address(caller_start)
    if caller_start != _cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALLER_START:
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
        == _cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALLER_SYMBOL.casefold()
    ]
    if (
        caller_identity != _cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALLER_END_EXCLUSIVE
        or caller_addresses
        != [_cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALLER_START]
        or caller_identity in indexes.provider_ids
        or caller_name_rows
        not in (
            [],
            [
                (
                    _cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALLER_SYMBOL,
                    _cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALLER_IDENTITY,
                )
            ],
        )
        or caller is None
        or caller.symbol != _cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALLER_SYMBOL
        or len(caller.data) != 0x10
        or len(caller.relocation_mask) != len(caller.data)
    ):
        raise ValueError(
            "HUD TriggerCurrentLayoutOnActivated bridge requires the exact "
            "reviewed authored caller identity, extent, symbol, and body"
        )

    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALLER_START
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
            "HUD TriggerCurrentLayoutOnActivated bridge requires one exact "
            "current HUD source authority"
        )
    contribution, contribution_row = contribution_rows[0]
    caller_symbol_pattern = (
        r"\?TriggerCurrentLayoutOnActivated@HudUiMgr@@.*"
    )
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != caller_symbol_pattern
        or re.fullmatch(caller_symbol_pattern, caller.symbol) is None
        or getattr(contribution_row, "name", "")
        != "HudUiMgr::TriggerCurrentLayoutOnActivated"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD TriggerCurrentLayoutOnActivated bridge requires the exact "
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
        or caller_symbol.get("ownership_state") != "primary-owned"
        or caller_symbol.get("extent_state") != "known"
        or normalize_address(str(caller_symbol.get("address", "")))
        != _cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALLER_START
        or normalize_address(str(caller_symbol.get("end_exclusive", "")))
        != _cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALLER_END_EXCLUSIVE
        or caller_symbol.get("size") != 0x10
        or caller_symbol.get("navigation_name")
        != "HudUiMgr::TriggerCurrentLayoutOnActivated"
        or caller_symbol.get("output_section_id") != "recoil:section:.text"
        or caller_symbol.get("physical_block_id") != "recoil:block:0x404ca0"
        or not exact_required_target_membership(
            caller_symbol.get("verification_target_ids", ()),
            _cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_VERIFICATION_TARGET_IDS,
        )
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
        != _cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
    ):
        raise ValueError(
            "HUD TriggerCurrentLayoutOnActivated bridge requires one exact "
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
    aggregate_containers = [
        row
        for row in indexes.storage_containers
        if row.identity == aggregate_identity
    ]
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
        or aggregate_containers
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
            "HUD TriggerCurrentLayoutOnActivated bridge requires the exact "
            "HUD aggregate symbol, storage, target, and container authority"
        )

    expected_row = {
        "ordinal": _cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALL_ORDINAL,
        "form": "tail",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": _cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_STORAGE_IDENTITY,
        "slot_displacement": (
            _cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_SLOT_DISPLACEMENT
        ),
        "cleanup_bytes": None,
    }
    if len(expected) != 1 or expected[0] != expected_row:
        raise ValueError(
            "HUD TriggerCurrentLayoutOnActivated bridge requires the exact "
            "immutable ordinal-0 retail tail virtual-slot contract"
        )

    retail_addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(caller_start),
    )
    retail_bytes = tuple(
        tuple(value.lower() for value in instruction.bytes)
        for instruction in retail_instructions
    )
    retail_mnemonics = tuple(
        _cc_cfg._instruction_mnemonic(instruction)
        for instruction in retail_instructions
    )
    if (
        retail_addresses
        != (0x413630, 0x413636, 0x413638, 0x41363A, 0x41363C, 0x41363F)
        or retail_bytes
        != (
            ("8b", "0d", "e8", "5e", "4e", "00"),
            ("85", "c9"),
            ("74", "05"),
            ("8b", "01"),
            ("ff", "60", "18"),
            ("c3",),
        )
        or retail_mnemonics
        != ("mov", "test", "je", "mov", "jmp", "retn")
        or _cc_cfg._instruction_operand(retail_instructions[0]).split(",", 1)[0]
        .strip()
        .lower()
        != "ecx"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(retail_instructions[0]).split(",", 1)[-1]
        )
        != "0x4e5ee8"
        or _cc_cfg._instruction_operand(retail_instructions[1])
        .replace(" ", "")
        .lower()
        != "ecx,ecx"
        or _cc_cfg._instruction_operand(retail_instructions[2]).strip().lower()
        != "0x41363f"
        or _cc_cfg._instruction_operand(retail_instructions[3]).split(",", 1)[0]
        .strip()
        .lower()
        != "eax"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(retail_instructions[3]).split(",", 1)[-1]
        )
        != "ecx"
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(retail_instructions[4]))
        not in {"eax+24", "eax+0x18"}
        or _cc_cfg._instruction_operand(retail_instructions[5]).strip() != ""
    ):
        raise ValueError(
            "HUD TriggerCurrentLayoutOnActivated bridge rejects immutable "
            "retail storage, null-guard, receiver, slot, return, or topology "
            "drift"
        )

    exact_candidate_body = bytes.fromhex(
        "8b 0d 18 00 00 00 85 c9 74 05 8b 01 ff 60 18 c3"
    )
    relocation = caller.relocations[0] if len(caller.relocations) == 1 else None
    if (
        caller.data != exact_candidate_body
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or (
            caller.defined_external_data
            + caller.undefined_external_functions
            + caller.defined_external_functions
        ).count(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
        != 0
        or caller.defined_external_functions.count(
            _cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALLER_SYMBOL
        )
        != 1
        or (
            caller.undefined_external_functions
            + caller.undefined_external_data
            + caller.defined_external_data
        ).count(_cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALLER_SYMBOL)
        != 0
    ):
        raise ValueError(
            "HUD TriggerCurrentLayoutOnActivated bridge requires the exact "
            "complete candidate null-guard/tail-call object body and external "
            "population"
        )
    if (
        relocation is None
        or relocation.offset
        != _cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_RELOCATION_OFFSET
        or relocation.type != IMAGE_REL_I386_DIR32
        or relocation.symbol_name != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        or struct.unpack_from("<I", caller.data, relocation.offset)[0]
        != _cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_DISPLACEMENT
        or {
            index
            for index, masked in enumerate(caller.relocation_mask)
            if masked
        }
        != set(
            range(
                _cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_RELOCATION_OFFSET,
                _cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_RELOCATION_OFFSET + 4,
            )
        )
    ):
        raise ValueError(
            "HUD TriggerCurrentLayoutOnActivated bridge rejects a missing, "
            "duplicate, extra, malformed, or incorrectly masked current-"
            "layout DIR32 relocation"
        )

    candidate_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    candidate_bytes = tuple(
        tuple(value.lower() for value in instruction.bytes)
        for instruction in candidate.instructions
    )
    candidate_mnemonics = tuple(
        _cc_cfg._instruction_mnemonic(instruction)
        for instruction in candidate.instructions
    )
    expected_expression = (
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
        f"+{_cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_DISPLACEMENT}"
    )
    expected_hex_expression = (
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
        f"+0x{_cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_DISPLACEMENT:x}"
    )
    if (
        candidate_offsets
        not in {
            (None, 0x06, 0x08, 0x0A, 0x0C, 0x0F),
            (0x00, 0x06, 0x08, 0x0A, 0x0C, 0x0F),
        }
        or candidate_bytes
        != (
            ("8b", "0d", "18", "00", "00", "00"),
            ("85", "c9"),
            ("74", "05"),
            ("8b", "01"),
            ("ff", "60", "18"),
            ("c3",),
        )
        or candidate_mnemonics
        != ("mov", "test", "je", "mov", "jmp", "ret")
        or _cc_cfg._instruction_operand(candidate.instructions[0]).split(",", 1)[0]
        .strip()
        .lower()
        != "ecx"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(candidate.instructions[0]).split(",", 1)[-1]
        )
        not in {expected_expression, expected_hex_expression}
        or _cc_cfg._instruction_operand(candidate.instructions[1])
        .replace(" ", "")
        .lower()
        != "ecx,ecx"
        or _cc_cfg._instruction_operand(candidate.instructions[3]).split(",", 1)[0]
        .strip()
        .lower()
        != "eax"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(candidate.instructions[3]).split(",", 1)[-1]
        )
        != "ecx"
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(candidate.instructions[4]))
        not in {"eax+24", "eax+0x18"}
        or _cc_cfg._instruction_operand(candidate.instructions[5]).strip() not in {
            "",
            "0",
        }
    ):
        raise ValueError(
            "HUD TriggerCurrentLayoutOnActivated bridge rejects candidate "
            "storage, null-guard, vptr, receiver, slot, return, or instruction "
            "order drift"
        )

    invocation_indices = tuple(
        index
        for index, instruction in enumerate(candidate.instructions)
        if _cc_cfg._instruction_mnemonic(instruction) == "call"
        or (
            _cc_cfg._instruction_mnemonic(instruction) == "jmp"
            and index not in candidate.local_control_flow_indices
        )
    )
    ecx_definitions = [
        index
        for index in range(4)
        if _cc_cfg._instruction_may_clobber_register(
            candidate.instructions[index], "ecx"
        )
    ]
    eax_definitions = [
        index
        for index in range(4)
        if _cc_cfg._instruction_may_clobber_register(
            candidate.instructions[index], "eax"
        )
    ]
    if (
        invocation_indices != (4,)
        or ecx_definitions != [0]
        or eax_definitions != [3]
        or candidate.local_control_flow_indices != frozenset()
        or dict(candidate.local_control_flow_targets) != {}
        or _cc_cfg._cleanup_after(candidate.instructions, 4) is not None
    ):
        raise ValueError(
            "HUD TriggerCurrentLayoutOnActivated bridge rejects candidate "
            "tail-call ordinal, reaching definition, cleanup, null-branch, "
            "alternate-entry, or return topology drift"
        )

    static_bridge = ReviewedStaticStorageReferenceBridge(
        aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
        displacement=_cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_DISPLACEMENT,
        access_width=4,
        storage_identity=aggregate_identity,
    )
    vptr_bridge = ReviewedVptrStorageBridge(
        register="eax",
        provenance=f"load({aggregate_identity})",
        storage_identity=_cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_STORAGE_IDENTITY,
        slot_displacement=_cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_SLOT_DISPLACEMENT,
        identity_kind="virtual-slot",
    )
    return (
        {expected_expression: static_bridge},
        {
            normalize_address(
                hex(_cc_catalog.HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALL_OFFSET)
            ): vptr_bridge
        },
    )


def _hud_ui_mgr_switch_active_dialog_current_layout_bridges(
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
    dict[str, ReviewedVptrStorageBridge],
]:
    """Prove SwitchActiveDialog's guarded current-layout SetActive(0)."""
    from _recoil.call_contract.records import (
        ReviewedStaticStorageReferenceBridge,
        ReviewedVptrStorageBridge,
        StorageContainer,
    )
    caller_start = normalize_address(caller_start)
    if caller_start != _cc_catalog.HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_CALLER_START:
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
        == _cc_catalog.HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_CALLER_SYMBOL.casefold()
    ]
    if (
        caller_identity != _cc_catalog.HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_CALLER_END_EXCLUSIVE
        or caller_addresses != [_cc_catalog.HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_CALLER_START]
        or caller_identity in indexes.provider_ids
        or caller_name_rows
        not in (
            [],
            [
                (
                    _cc_catalog.HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_CALLER_SYMBOL,
                    _cc_catalog.HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_CALLER_IDENTITY,
                )
            ],
        )
        or caller is None
        or caller.symbol != _cc_catalog.HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_CALLER_SYMBOL
        or len(caller.data) != 0x50
        or len(caller.relocation_mask) != len(caller.data)
    ):
        raise ValueError(
            "HUD SwitchActiveDialog current-layout bridge requires the exact "
            "reviewed authored caller identity, extent, symbol, and body"
        )

    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_CALLER_START
    ]
    if (
        target is None
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
    ):
        raise ValueError(
            "HUD SwitchActiveDialog current-layout bridge requires one exact "
            "current HUD source authority"
        )
    contribution, contribution_row = contribution_rows[0]
    caller_symbol_pattern = r"\?SwitchActiveDialog@HudUiMgr@@.*"
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != caller_symbol_pattern
        or re.fullmatch(caller_symbol_pattern, caller.symbol) is None
        or getattr(contribution_row, "name", "")
        != "HudUiMgr::SwitchActiveDialog"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD SwitchActiveDialog current-layout bridge requires the exact "
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
        or caller_symbol.get("ownership_state") != "primary-owned"
        or caller_symbol.get("extent_state") != "known"
        or normalize_address(str(caller_symbol.get("address", "")))
        != _cc_catalog.HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_CALLER_START
        or normalize_address(str(caller_symbol.get("end_exclusive", "")))
        != _cc_catalog.HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_CALLER_END_EXCLUSIVE
        or caller_symbol.get("size") != 0x50
        or caller_symbol.get("navigation_name")
        != "HudUiMgr::SwitchActiveDialog"
        or caller_symbol.get("output_section_id") != "recoil:section:.text"
        or caller_symbol.get("physical_block_id") != "recoil:block:0x404ca0"
        or not exact_required_target_membership(
            caller_symbol.get("verification_target_ids", ()),
            _cc_catalog.HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_VERIFICATION_TARGET_IDS,
        )
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
        != _cc_catalog.HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
    ):
        raise ValueError(
            "HUD SwitchActiveDialog current-layout bridge requires one exact "
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
    aggregate_containers = [
        row
        for row in indexes.storage_containers
        if row.identity == aggregate_identity
    ]
    if (
        not isinstance(aggregate_symbol, Mapping)
        or aggregate_symbol.get("binary") != "recoil"
        or aggregate_symbol.get("kind") != "data"
        or normalize_address(str(aggregate_symbol.get("address", "")))
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
        or aggregate_symbol.get("disposition") != "authored"
        or aggregate_symbol.get("navigation_name") != _cc_catalog.HUD_UI_MGR_AGGREGATE_NAME
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
        or aggregate_containers
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
            "HUD SwitchActiveDialog current-layout bridge requires the exact "
            "HUD aggregate symbol, storage, target, and container authority"
        )

    expected_rows = [
        {
            "ordinal": 0,
            "form": "call",
            "dispatch": "direct",
            "identity_kind": "direct",
            "target_identity": "symbol:recoil:function:0x410ed0",
            "storage_identity": "",
            "slot_displacement": None,
            "cleanup_bytes": None,
        },
        {
            "ordinal": _cc_catalog.HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_CALL_ORDINAL,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": _cc_catalog.HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_STORAGE_IDENTITY,
            "slot_displacement": _cc_catalog.HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_SLOT_DISPLACEMENT,
            "cleanup_bytes": None,
        },
        {
            "ordinal": 2,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": "load(this)",
            "slot_displacement": 0x08,
            "cleanup_bytes": None,
        },
        {
            "ordinal": 3,
            "form": "call",
            "dispatch": "direct",
            "identity_kind": "direct",
            "target_identity": "symbol:recoil:function:0x410e90",
            "storage_identity": "",
            "slot_displacement": None,
            "cleanup_bytes": None,
        },
    ]
    if list(expected) != expected_rows:
        raise ValueError(
            "HUD SwitchActiveDialog current-layout bridge requires the exact "
            "immutable four-call retail contract"
        )

    retail_addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(caller_start),
    )
    retail_bytes = tuple(
        tuple(value.lower() for value in instruction.bytes)
        for instruction in retail_instructions
    )
    if (
        retail_addresses
        != (
            0x413660, 0x413665, 0x413666, 0x413667, 0x413669,
            0x41366B, 0x41366D, 0x41366F, 0x413674, 0x413676,
            0x413680, 0x413686, 0x413688, 0x41368A, 0x41368C,
            0x41368E, 0x413691, 0x413693, 0x413695, 0x413697,
            0x41369A, 0x41369C, 0x4136A2, 0x4136A4, 0x4136A9,
            0x4136AA, 0x4136AB,
        )
        or retail_bytes
        != (
            ("a1", "d4", "5e", "4e", "00"),
            ("56",), ("57",), ("8b", "f1"), ("85", "c0"),
            ("8b", "f8"), ("74", "07"),
            ("e8", "5c", "d8", "ff", "ff"), ("eb", "0a"),
            ("c7", "05", "bc", "61", "4e", "00", "02", "00", "00", "00"),
            ("8b", "0d", "e8", "5e", "4e", "00"),
            ("85", "c9"), ("74", "07"), ("8b", "01"),
            ("6a", "00"), ("ff", "50", "08"), ("8b", "16"),
            ("6a", "01"), ("8b", "ce"), ("ff", "52", "08"),
            ("85", "ff"), ("89", "35", "e8", "5e", "4e", "00"),
            ("74", "05"), ("e8", "e7", "d7", "ff", "ff"),
            ("5f",), ("5e",), ("c3",),
        )
        or tuple(_cc_cfg._instruction_mnemonic(row) for row in retail_instructions)
        != (
            "mov", "push", "push", "mov", "test", "mov", "je",
            "call", "jmp", "mov", "mov", "test", "je", "mov",
            "push", "call", "mov", "push", "mov", "call", "test",
            "mov", "je", "call", "pop", "pop", "retn",
        )
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(retail_instructions[10]).split(",", 1)[-1]
        )
        != "0x4e5ee8"
        or _cc_cfg._instruction_operand(retail_instructions[11]).replace(" ", "").lower()
        != "ecx,ecx"
        or _cc_cfg._instruction_operand(retail_instructions[12]).strip().lower()
        != "0x413691"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(retail_instructions[13]).split(",", 1)[-1]
        )
        != "ecx"
        or _cc_cfg._instruction_operand(retail_instructions[14]).strip().lower()
        not in {"0", "0x0"}
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(retail_instructions[15]))
        not in {"eax+8", "eax+0x8"}
    ):
        raise ValueError(
            "HUD SwitchActiveDialog current-layout bridge rejects immutable "
            "retail storage, null guard, SetActive(0), receiver, slot, call "
            "form/order, cleanup, or surrounding topology drift"
        )

    exact_candidate_body = bytes.fromhex(
        "56 57 8b 3d 04 00 00 00 8b f1 85 ff 74 07 e8 00 00 00 00 "
        "eb 0a c7 05 ec 02 00 00 02 00 00 00 8b 0d 18 00 00 00 "
        "85 c9 74 07 8b 01 6a 00 ff 50 08 8b 16 6a 01 8b ce ff 52 "
        "08 85 ff 89 35 18 00 00 00 74 05 e8 00 00 00 00 5f 5e "
        "c3 90 90 90 90 90"
    )
    current_layout_relocations = [
        relocation
        for relocation in caller.relocations
        if relocation.symbol_name == _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        and relocation.offset == _cc_catalog.HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_RELOCATION_OFFSET
    ]
    if (
        caller.data != exact_candidate_body
        or tuple(
            (row.offset, row.type, row.symbol_name)
            for row in caller.relocations
        )
        != (
            (0x04, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL),
            (0x0F, IMAGE_REL_I386_REL32, _cc_catalog.HUD_UI_MGR_DISABLE_CALLER_SYMBOL),
            (0x17, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL),
            (0x21, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL),
            (0x3D, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL),
            (0x44, IMAGE_REL_I386_REL32, _cc_catalog.HUD_UI_MGR_ENABLE_CALLER_SYMBOL),
        )
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or (
            caller.defined_external_data
            + caller.undefined_external_functions
            + caller.defined_external_functions
        ).count(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
        != 0
        or caller.defined_external_functions.count(
            _cc_catalog.HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_CALLER_SYMBOL
        )
        != 1
        or caller.defined_external_functions.count(_cc_catalog.HUD_UI_MGR_DISABLE_CALLER_SYMBOL)
        != 1
        or caller.defined_external_functions.count(_cc_catalog.HUD_UI_MGR_ENABLE_CALLER_SYMBOL)
        != 1
        or (
            caller.undefined_external_functions
            + caller.undefined_external_data
            + caller.defined_external_data
        ).count(_cc_catalog.HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_CALLER_SYMBOL)
        != 0
        or len(current_layout_relocations) != 1
    ):
        raise ValueError(
            "HUD SwitchActiveDialog current-layout bridge requires the exact "
            "complete candidate body, external population, and unique current-"
            "layout relocation"
        )
    relocation = current_layout_relocations[0]
    expected_mask = {
        index
        for row in caller.relocations
        for index in range(row.offset, row.offset + 4)
    }
    if (
        relocation.type != IMAGE_REL_I386_DIR32
        or struct.unpack_from("<I", caller.data, relocation.offset)[0]
        != _cc_catalog.HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_DISPLACEMENT
        or {
            index
            for index, masked in enumerate(caller.relocation_mask)
            if masked
        }
        != expected_mask
    ):
        raise ValueError(
            "HUD SwitchActiveDialog current-layout bridge rejects a malformed "
            "current-layout DIR32 relocation or relocation mask"
        )

    candidate_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    candidate_bytes = tuple(
        tuple(value.lower() for value in instruction.bytes)
        for instruction in candidate.instructions
    )
    expected_expression = (
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
        f"+{_cc_catalog.HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_DISPLACEMENT}"
    )
    if (
        candidate_offsets
        != (
            0x00, 0x01, 0x02, 0x08, 0x0A, 0x0C, 0x0E, 0x13,
            0x15, 0x1F, 0x25, 0x27, 0x29, 0x2B, 0x2D, 0x30,
            0x32, 0x34, 0x36, 0x39, 0x3B, 0x41, 0x43, 0x48,
            0x49, 0x4A,
        )
        or candidate_bytes
        != (
            ("56",), ("57",), ("8b", "3d", "04", "00", "00", "00"),
            ("8b", "f1"), ("85", "ff"), ("74", "07"),
            ("e8", "00", "00", "00", "00"), ("eb", "0a"),
            ("c7", "05", "ec", "02", "00", "00", "02", "00", "00", "00"),
            ("8b", "0d", "18", "00", "00", "00"), ("85", "c9"),
            ("74", "07"), ("8b", "01"), ("6a", "00"),
            ("ff", "50", "08"), ("8b", "16"), ("6a", "01"),
            ("8b", "ce"), ("ff", "52", "08"), ("85", "ff"),
            ("89", "35", "18", "00", "00", "00"), ("74", "05"),
            ("e8", "00", "00", "00", "00"), ("5f",), ("5e",),
            ("c3",),
        )
        or tuple(_cc_cfg._instruction_mnemonic(row) for row in candidate.instructions)
        != (
            "push", "push", "mov", "mov", "test", "je", "call", "jmp",
            "mov", "mov", "test", "je", "mov", "push", "call", "mov",
            "push", "mov", "call", "test", "mov", "je", "call", "pop",
            "pop", "ret",
        )
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(candidate.instructions[9]).split(",", 1)[-1]
        )
        not in {expected_expression, f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}+0x18"}
        or _cc_cfg._instruction_operand(candidate.instructions[10]).replace(" ", "").lower()
        != "ecx,ecx"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(candidate.instructions[12]).split(",", 1)[-1]
        )
        != "ecx"
        or _cc_cfg._instruction_operand(candidate.instructions[13]).strip().lower()
        != "0"
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(candidate.instructions[14]))
        not in {"eax+8", "eax+0x8"}
    ):
        raise ValueError(
            "HUD SwitchActiveDialog current-layout bridge rejects candidate "
            "storage, null guard, SetActive(0), receiver, slot, call order, "
            "or complete surrounding instruction topology drift"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    ecx_definitions = [
        index
        for index in range(9, 14)
        if _cc_cfg._instruction_may_clobber_register(candidate.instructions[index], "ecx")
    ]
    eax_definitions = [
        index
        for index in range(9, 14)
        if _cc_cfg._instruction_may_clobber_register(candidate.instructions[index], "eax")
    ]
    if (
        invocation_indices != (6, 14, 18, 22)
        or ecx_definitions != [9]
        or eax_definitions != [12]
        or candidate.local_control_flow_indices != frozenset()
        or dict(candidate.local_control_flow_targets) != {}
        or _cc_cfg._cleanup_after(candidate.instructions, 14) is not None
    ):
        raise ValueError(
            "HUD SwitchActiveDialog current-layout bridge rejects candidate "
            "ordinal, reaching definition, cleanup, conditional-null-guard, "
            "alternate-entry, or surrounding call topology drift"
        )

    static_bridge = ReviewedStaticStorageReferenceBridge(
        aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
        displacement=_cc_catalog.HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_DISPLACEMENT,
        access_width=4,
        storage_identity=aggregate_identity,
    )
    vptr_bridge = ReviewedVptrStorageBridge(
        register="eax",
        provenance=f"load({aggregate_identity})",
        storage_identity=_cc_catalog.HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_STORAGE_IDENTITY,
        slot_displacement=_cc_catalog.HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_SLOT_DISPLACEMENT,
        identity_kind="virtual-slot",
    )
    return (
        {expected_expression: static_bridge},
        {
            normalize_address(
                hex(_cc_catalog.HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_CALL_OFFSET)
            ): vptr_bridge
        },
    )


def _hud_layout_apply_text_label_authority(
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> str | None:
    """Require the reviewed ApplyTextLabel caller, source, and target rows."""
    caller_start = normalize_address(caller_start)
    if caller_start != _cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_CALLER_START:
        return None
    caller = candidate.caller_definition
    caller_addresses = sorted(
        address
        for address, identity in indexes.by_address.items()
        if identity == caller_identity
    )
    if (
        caller_identity != _cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_CALLER_END_EXCLUSIVE
        or caller_addresses != [_cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_CALLER_START]
        or caller_identity in indexes.provider_ids
        or caller is None
        or caller.symbol != _cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_CALLER_SYMBOL
        or len(caller.data) != 0x80
        or len(caller.relocation_mask) != len(caller.data)
    ):
        raise ValueError(
            "HUD ApplyTextLabel bridge requires the exact reviewed authored "
            "caller identity, extent, symbol, and 0x80-byte candidate body"
        )

    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_CALLER_START
    ]
    if (
        target is None
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
    ):
        raise ValueError(
            "HUD ApplyTextLabel bridge requires one exact current hud.cpp "
            "translation-unit authority"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "")
        != _cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_CALLER_SYMBOL
        or getattr(contribution_row, "symbol_regex", None) is not None
        or getattr(contribution_row, "name", "")
        != "HudUiLayoutNode::ApplyTextLabel"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD ApplyTextLabel bridge requires its exact authored hud.cpp "
            "contribution row"
        )

    symbols = document.collection("symbols")
    targets = document.collection("verification_targets")
    caller_symbol_id = caller_identity.removeprefix("symbol:")
    caller_symbol = symbols.get(caller_symbol_id)
    trace = (
        caller_symbol.get("source_traceability")
        if isinstance(caller_symbol, Mapping)
        else None
    )
    if (
        not isinstance(caller_symbol, Mapping)
        or _cc_identity._symbol_identity(caller_symbol_id, caller_symbol) != caller_identity
        or caller_symbol.get("binary") != "recoil"
        or caller_symbol.get("kind") != "function"
        or caller_symbol.get("pipeline_class") != "authored"
        or caller_symbol.get("ownership_state") != "primary-owned"
        or caller_symbol.get("extent_state") != "known"
        or normalize_address(str(caller_symbol.get("address", "")))
        != _cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_CALLER_START
        or normalize_address(str(caller_symbol.get("end_exclusive", "")))
        != _cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_CALLER_END_EXCLUSIVE
        or caller_symbol.get("size") != 0x80
        or caller_symbol.get("navigation_name")
        != "HudUiLayoutNode::ApplyTextLabel"
        or caller_symbol.get("output_section_id") != "recoil:section:.text"
        or caller_symbol.get("physical_block_id") != "recoil:block:0x404ca0"
        or not exact_required_target_membership(
            caller_symbol.get("verification_target_ids", ()),
            _cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_VERIFICATION_TARGET_IDS,
        )
        or caller_symbol.get("logical_identity_key") not in {None, ""}
        or caller_symbol.get("icf_fold_status") not in {None, ""}
        or bool(caller_symbol.get("logical_aliases"))
        or not isinstance(trace, Mapping)
        or trace.get("state") != "resolved"
        or trace.get("reason_code") not in {None, ""}
        or trace.get("source_edges")
        != [
            {
                "anchor_id": _cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_CALLER_ANCHOR_ID,
                "emission_context": {
                    "translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
                },
                "evidence_ids": [],
                "relation": "defines",
            }
        ]
    ):
        raise ValueError(
            "HUD ApplyTextLabel bridge requires one exact unaliased reviewed "
            "caller and resolved source-anchor edge"
        )

    vc5_target = targets.get(
        _cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_VERIFICATION_TARGET_IDS[0]
    )
    vc5_registration = (
        vc5_target.get("registration")
        if isinstance(vc5_target, Mapping)
        else None
    )
    if (
        not isinstance(vc5_target, Mapping)
        or vc5_target.get("binary") != "recoil"
        or vc5_target.get("kind") != "vc5"
        or vc5_target.get("name") != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_TARGET_NAME
        or vc5_target.get("unresolved_addresses") not in (None, [])
        or tuple(vc5_target.get("registered_addresses", ())).count(
            _cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_CALLER_START
        )
        != 1
        or not isinstance(vc5_registration, Mapping)
        or vc5_registration.get("binary") != "recoil"
        or vc5_registration.get("name")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_TARGET_NAME
        or vc5_registration.get("manifest_path")
        != "tools/vc5_verify_targets/hud_404ca0_415ab0_authored_order.json"
        or vc5_registration.get("source_from")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or not bool(
            vc5_registration.get("check_translation_unit_function_order")
        )
    ):
        raise ValueError(
            "HUD ApplyTextLabel bridge requires its exact reviewed VC5 target "
            "authority"
        )
    return _cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_RECEIVER_STORAGE_IDENTITY


def _hud_layout_apply_text_label_retail_vptr_bridges(
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedLoopVptrStorageBridge]:
    """Prove ApplyTextLabel's immutable-retail receiver and branch unit."""
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge
    receiver_identity = _hud_layout_apply_text_label_authority(
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    if receiver_identity is None:
        return {}
    exact_body = bytes.fromhex(
        "8b 01 56 83 f8 04 8b f2 74 06 33 c0 5e c2 0c 00 "
        "8b 49 04 8b 54 24 08 57 53 8b 41 14 8b 79 0c 8b "
        "49 1c 03 c2 8b 54 24 14 03 ca 8b 54 24 18 85 d2 "
        "74 09 8b 1a 03 c3 8b 5a 04 03 cb 8b 16 51 50 8b "
        "ce ff 52 0c 85 ff 74 15 8b 06 57 56 ff 50 74 83 "
        "c4 08 b8 01 00 00 00 5b 5f 5e c2 0c 00 8b 0e 68 "
        "e0 5c 4e 00 56 ff 51 74 83 c4 08 b8 01 00 00 00 "
        "5b 5f 5e c2 0c 00 90 90 90 90 90 90 90 90 90 90"
    )
    expected_offsets = (
        0x00, 0x02, 0x03, 0x06, 0x08, 0x0A, 0x0C, 0x0D,
        0x10, 0x13, 0x17, 0x18, 0x19, 0x1C, 0x1F, 0x22,
        0x24, 0x28, 0x2A, 0x2E, 0x30, 0x32, 0x34, 0x36,
        0x39, 0x3B, 0x3D, 0x3E, 0x3F, 0x41, 0x44, 0x46,
        0x48, 0x4A, 0x4B, 0x4C, 0x4F, 0x52, 0x57, 0x58,
        0x59, 0x5A, 0x5D, 0x5F, 0x64, 0x65, 0x68, 0x6B,
        0x70, 0x71, 0x72, 0x73,
    )
    addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(caller_start),
    )
    expected_addresses = tuple(
        address_value(caller_start) + offset for offset in expected_offsets
    )
    encoded = bytes(
        int(value, 16)
        for instruction in retail_instructions
        for value in instruction.bytes
    )
    by_address = dict(zip(addresses, retail_instructions))

    def short_target(address: int) -> int | None:
        instruction = by_address.get(address)
        body = (
            bytes(int(value, 16) for value in instruction.bytes)
            if instruction is not None
            else b""
        )
        if len(body) != 2 or not 0x70 <= body[0] <= 0x7F:
            return None
        return address + 2 + struct.unpack("<b", body[1:2])[0]

    calls = tuple(
        (
            address,
            _cc_targets._memory_slot(_cc_cfg._instruction_operand(by_address[address]))[0],
            _cc_targets._memory_slot(_cc_cfg._instruction_operand(by_address[address]))[1],
            _cc_cfg._cleanup_after(
                retail_instructions,
                addresses.index(address),
            ),
        )
        for address, _register, _slot, _cleanup
        in _cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_RETAIL_CALLS
    )
    invocation_addresses = tuple(
        address
        for address, instruction in zip(addresses, retail_instructions)
        if _cc_cfg._instruction_mnemonic(instruction) == "call"
    )
    if (
        addresses != expected_addresses
        or encoded != exact_body[:-10]
        or invocation_addresses
        != tuple(row[0] for row in _cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_RETAIL_CALLS)
        or calls
        != tuple(
            (address, f"{register}+0x{slot:x}", slot, cleanup)
            for address, register, slot, cleanup
            in _cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_RETAIL_CALLS
        )
        or short_target(0x413998) != 0x4139A0
        or short_target(0x4139C0) != 0x4139CB
        or short_target(0x4139D6) != 0x4139ED
        or tuple(
            address
            for address in addresses
            if bytes(
                int(value, 16) for value in by_address[address].bytes
            )
            == b"\xc2\x0c\x00"
        )
        != (0x41399D, 0x4139EA, 0x413A03)
        or _cc_cfg._instruction_operand(by_address[0x413996]).strip().lower()
        != "esi, edx"
        or _cc_cfg._instruction_operand(by_address[0x4139CB]).strip().lower()
        not in {"edx, dword [esi]", "edx, dword ptr [esi]"}
        or _cc_cfg._instruction_operand(by_address[0x4139D8]).strip().lower()
        not in {"eax, dword [esi]", "eax, dword ptr [esi]"}
        or _cc_cfg._instruction_operand(by_address[0x4139ED]).strip().lower()
        not in {"ecx, dword [esi]", "ecx, dword ptr [esi]"}
    ):
        raise ValueError(
            "HUD ApplyTextLabel bridge rejects immutable retail body, EDX "
            "receiver/vptr provenance, three-call population/order/form/"
            "slot/cleanup, branch targets, return topology, empty literal, "
            "or exact extent"
        )
    return {
        normalize_address(hex(address)): ReviewedLoopVptrStorageBridge(
            register=register,
            storage_identity=receiver_identity,
            slot_displacement=slot,
            assembly_source="bn",
        )
        for address, register, slot, _cleanup
        in _cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_RETAIL_CALLS
    }


def _hud_layout_apply_text_label_candidate_vptr_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedLoopVptrStorageBridge]:
    """Prove the current ApplyTextLabel object and branch-local callsites."""
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge
    receiver_identity = _hud_layout_apply_text_label_authority(
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    if receiver_identity is None:
        return {}
    expected_rows = [
        {
            "ordinal": ordinal,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": receiver_identity,
            "slot_displacement": slot,
            "cleanup_bytes": cleanup,
        }
        for ordinal, (_offset, _register, slot, cleanup)
        in enumerate(_cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_CANDIDATE_CALLS)
    ]
    if list(expected) != expected_rows:
        raise ValueError(
            "HUD ApplyTextLabel bridge requires the exact immutable-retail "
            "three-call contract"
        )
    caller = candidate.caller_definition
    assert caller is not None
    exact_body = bytes.fromhex(
        "8b 01 56 83 f8 04 8b f2 74 06 33 c0 5e c2 0c 00 "
        "8b 49 04 8b 54 24 08 57 53 8b 41 14 8b 79 0c 8b "
        "49 1c 03 c2 8b 54 24 14 03 ca 8b 54 24 18 85 d2 "
        "74 09 8b 1a 03 c3 8b 5a 04 03 cb 8b 16 51 50 8b "
        "ce ff 52 0c 85 ff 74 15 8b 06 57 56 ff 50 74 83 "
        "c4 08 b8 01 00 00 00 5b 5f 5e c2 0c 00 8b 0e 68 "
        "00 00 00 00 56 ff 51 74 83 c4 08 b8 01 00 00 00 "
        "5b 5f 5e c2 0c 00 90 90 90 90 90 90 90 90 90 90"
    )
    expected_relocations = (
        (
            0x60,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_EMPTY_LITERAL_SYMBOL,
            0,
        ),
    )
    observed_relocations = tuple(
        (
            row.offset,
            row.type,
            row.symbol_name,
            (
                struct.unpack_from("<I", caller.data, row.offset)[0]
                if row.offset + 4 <= len(caller.data)
                else None
            ),
        )
        for row in caller.relocations
    )
    expected_mask = set(range(0x60, 0x64))
    external_population = (
        caller.defined_external_functions.count(
            _cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_CALLER_SYMBOL
        ),
        caller.undefined_external_functions.count(
            _cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_CALLER_SYMBOL
        ),
        caller.defined_external_data.count(
            _cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_EMPTY_LITERAL_SYMBOL
        ),
        caller.undefined_external_data.count(
            _cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_EMPTY_LITERAL_SYMBOL
        ),
    )
    if (
        caller.data != exact_body
        or observed_relocations != expected_relocations
        or {
            index
            for index, masked in enumerate(caller.relocation_mask)
            if masked
        }
        != expected_mask
        or external_population != (1, 0, 1, 0)
    ):
        raise ValueError(
            "HUD ApplyTextLabel bridge requires the exact complete candidate "
            "body, empty-literal COFF relocation/addend/order, relocation "
            "mask, ten-byte padding, and relevant external population"
        )

    expected_offsets = (
        0x00, 0x02, 0x03, 0x06, 0x08, 0x0A, 0x0C, 0x0D,
        0x10, 0x13, 0x17, 0x18, 0x19, 0x1C, 0x1F, 0x22,
        0x24, 0x28, 0x2A, 0x2E, 0x30, 0x32, 0x34, 0x36,
        0x39, 0x3B, 0x3D, 0x3E, 0x3F, 0x41, 0x44, 0x46,
        0x48, 0x4A, 0x4B, 0x4C, 0x4F, 0x52, 0x57, 0x58,
        0x59, 0x5A, 0x5D, 0x5F, 0x64, 0x65, 0x68, 0x6B,
        0x70, 0x71, 0x72, 0x73,
    )
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    by_offset = dict(zip(offsets, candidate.instructions))
    encoded = bytes(
        int(value, 16)
        for instruction in candidate.instructions
        for value in instruction.bytes
    )

    def short_target(offset: int) -> int | None:
        instruction = by_offset.get(offset)
        body = (
            bytes(int(value, 16) for value in instruction.bytes)
            if instruction is not None
            else b""
        )
        if len(body) != 2 or not 0x70 <= body[0] <= 0x7F:
            return None
        return offset + 2 + struct.unpack("<b", body[1:2])[0]

    calls = tuple(
        (
            offset,
            _cc_targets._memory_slot(_cc_cfg._instruction_operand(by_offset[offset]))[0],
            _cc_targets._memory_slot(_cc_cfg._instruction_operand(by_offset[offset]))[1],
            _cc_cfg._cleanup_after(candidate.instructions, offsets.index(offset)),
        )
        for offset, _register, _slot, _cleanup
        in _cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_CANDIDATE_CALLS
    )
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    if (
        offsets != expected_offsets
        or encoded != exact_body[:-10]
        or invocation_indices != (29, 35, 45)
        or tuple(offsets[index] for index in invocation_indices)
        != tuple(row[0] for row in _cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_CANDIDATE_CALLS)
        or calls
        != tuple(
            (offset, f"{register}+{slot}", slot, cleanup)
            for offset, register, slot, cleanup
            in _cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_CANDIDATE_CALLS
        )
        or short_target(0x08) != 0x10
        or short_target(0x30) != 0x3B
        or short_target(0x46) != 0x5D
        or tuple(
            offset
            for offset in offsets
            if bytes(
                int(value, 16) for value in by_offset[offset].bytes
            )
            == b"\xc2\x0c\x00"
        )
        != (0x0D, 0x5A, 0x73)
        or _cc_cfg._instruction_operand(by_offset[0x06]).strip().lower()
        != "esi, edx"
        or _cc_cfg._instruction_operand(by_offset[0x3B]).strip().lower()
        not in {"edx, dword [esi]", "edx, dword ptr [esi]"}
        or _cc_cfg._instruction_operand(by_offset[0x48]).strip().lower()
        not in {"eax, dword [esi]", "eax, dword ptr [esi]"}
        or _cc_cfg._instruction_operand(by_offset[0x5D]).strip().lower()
        not in {"ecx, dword [esi]", "ecx, dword ptr [esi]"}
        or candidate.local_control_flow_indices != frozenset()
        or dict(candidate.local_control_flow_targets) != {}
    ):
        raise ValueError(
            "HUD ApplyTextLabel bridge rejects candidate EDX receiver/vptr "
            "provenance, three-call population/order/form/slot/cleanup, "
            "branch targets, return topology, or complete instruction extent"
        )
    return {
        normalize_address(hex(offset)): ReviewedLoopVptrStorageBridge(
            register=register,
            storage_identity=receiver_identity,
            slot_displacement=slot,
            assembly_source="cod",
        )
        for offset, register, slot, _cleanup
        in _cc_catalog.HUD_LAYOUT_APPLY_TEXT_LABEL_CANDIDATE_CALLS
    }


def _hud_layout_hw_absolute_storage_load_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedAbsoluteStorageLoadBridge]:
    """Bridge one exact VC5 absolute global load to retail provenance.

    The key is the load instruction offset, not the repeated symbol expression,
    so no sibling reference to the aggregate inherits this candidate-only
    call-contract fact.
    """
    from _recoil.call_contract.records import ReviewedAbsoluteStorageLoadBridge
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_LAYOUT_HW_VPTR_CALLER_START:
        return {}
    caller_addresses = sorted(
        address
        for address, identity in indexes.by_address.items()
        if identity == caller_identity
    )
    if (
        caller_identity != _cc_catalog.HUD_LAYOUT_HW_VPTR_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_LAYOUT_HW_VPTR_CALLER_END_EXCLUSIVE
        or caller_addresses != [_cc_catalog.HUD_LAYOUT_HW_VPTR_CALLER_START]
        or caller_identity in indexes.provider_ids
    ):
        raise ValueError(
            "HUD-layout-HW absolute-load bridge requires the exact reviewed "
            "authored 0x40f2e0 caller identity and extent"
        )

    symbols = document.collection("symbols")
    storage_rows = document.collection("storage_contributions")
    owners = document.collection("owners")
    symbol = symbols.get(_cc_catalog.HUD_LAYOUT_HW_DATA_ID)
    storage = storage_rows.get(_cc_catalog.HUD_LAYOUT_HW_STORAGE_ID)
    owner = owners.get(_cc_catalog.HUD_LAYOUT_HW_OWNER_ID)
    reference = storage.get("reference") if isinstance(storage, Mapping) else None
    gates = owner.get("gates") if isinstance(owner, Mapping) else None
    rows_at_address = [
        str(entity_id)
        for entity_id, row in symbols.items()
        if isinstance(row, Mapping)
        and isinstance(row.get("address", row.get("start")), str)
        and normalize_address(str(row.get("address", row.get("start"))))
        == _cc_catalog.HUD_LAYOUT_HW_ADDRESS
    ]
    storage_at_address = [
        str(entity_id)
        for entity_id, row in storage_rows.items()
        if isinstance(row, Mapping)
        and isinstance(row.get("reference"), Mapping)
        and isinstance(row["reference"].get("address"), str)
        and normalize_address(str(row["reference"]["address"]))
        == _cc_catalog.HUD_LAYOUT_HW_ADDRESS
    ]
    primary_relationships = [
        (str(owner_id), relationship)
        for owner_id, candidate_owner in owners.items()
        if isinstance(candidate_owner, Mapping)
        for relationship in candidate_owner.get("relationships", [])
        if isinstance(relationship, Mapping)
        and relationship.get("kind") == "primary-data"
        and relationship.get("symbol_id") == _cc_catalog.HUD_LAYOUT_HW_DATA_ID
    ]
    if (
        rows_at_address != [_cc_catalog.HUD_LAYOUT_HW_DATA_ID]
        or storage_at_address != [_cc_catalog.HUD_LAYOUT_HW_STORAGE_ID]
        or not isinstance(symbol, Mapping)
        or symbol.get("binary") != "recoil"
        or symbol.get("kind") != "data"
        or symbol.get("disposition") != "authored"
        or symbol.get("navigation_name") != "g_HudLayoutHW"
        or symbol.get("output_section_id") != "recoil:section:.data"
        or symbol.get("extent_state") != "unknown"
        or symbol.get("size") is not None
        or symbol.get("end_exclusive") is not None
        or symbol.get("storage_contribution_ids")
        != [_cc_catalog.HUD_LAYOUT_HW_STORAGE_ID]
        or symbol.get("logical_aliases")
        or symbol.get("relocation_target_binding")
        or not isinstance(storage, Mapping)
        or storage.get("binary") != "recoil"
        or storage.get("kind") != "data-symbol"
        or storage.get("output_section_id") != "recoil:section:.data"
        or storage.get("overlap") != "none"
        or storage.get("owner_ids") != [_cc_catalog.HUD_LAYOUT_HW_OWNER_ID]
        or storage.get("parent_contribution_id") is not None
        or storage.get("symbol_ids") != [_cc_catalog.HUD_LAYOUT_HW_DATA_ID]
        or storage.get("logical_aliases")
        or not isinstance(reference, Mapping)
        or normalize_address(str(reference.get("address", "")))
        != _cc_catalog.HUD_LAYOUT_HW_ADDRESS
        or reference.get("extent_state") != "unknown"
        or reference.get("size") is not None
        or reference.get("end_exclusive") is not None
        or not isinstance(owner, Mapping)
        or owner.get("binary") != "recoil"
        or owner.get("kind") != "class"
        or owner.get("lifecycle_state") != "active"
        or owner.get("provider_state") == "accepted"
        or not isinstance(gates, Mapping)
        or any(
            gates.get(gate) != "accepted"
            for gate in ("boundary", "source", "data", "owner_linkage")
        )
        or primary_relationships
        != [
            (
                _cc_catalog.HUD_LAYOUT_HW_OWNER_ID,
                {
                    "address": _cc_catalog.HUD_LAYOUT_HW_ADDRESS,
                    "kind": "primary-data",
                    "name": "g_HudLayoutHW",
                    "symbol_id": _cc_catalog.HUD_LAYOUT_HW_DATA_ID,
                },
            )
        ]
    ):
        raise ValueError(
            "HUD-layout-HW absolute-load bridge requires unique authored "
            "tracker symbol, storage contribution, and primary owner linkage"
        )

    matching_storage_addresses = sorted(
        address
        for address, identity in indexes.storage_by_address.items()
        if identity == _cc_catalog.HUD_LAYOUT_HW_STORAGE_IDENTITY
    )
    matching_containers = [
        row
        for row in indexes.storage_containers
        if row.identity == _cc_catalog.HUD_LAYOUT_HW_STORAGE_IDENTITY
    ]
    if (
        indexes.storage_by_address.get(_cc_catalog.HUD_LAYOUT_HW_ADDRESS)
        != _cc_catalog.HUD_LAYOUT_HW_STORAGE_IDENTITY
        or matching_storage_addresses != [_cc_catalog.HUD_LAYOUT_HW_ADDRESS]
        or _cc_catalog.HUD_LAYOUT_HW_STORAGE_IDENTITY in indexes.provider_ids
        or len(matching_containers) > 1
        or (
            matching_containers
            and (
                matching_containers[0].start
                != address_value(_cc_catalog.HUD_LAYOUT_HW_ADDRESS)
                or matching_containers[0].start
                + _cc_catalog.HUD_LAYOUT_HW_DISPLACEMENT
                + _cc_catalog.HUD_LAYOUT_HW_ACCESS_WIDTH
                > matching_containers[0].end_exclusive
            )
        )
    ):
        raise ValueError(
            "HUD-layout-HW absolute-load bridge requires one exact "
            "non-provider storage identity and no conflicting retail extent"
        )

    desired_storage = (
        f"load({_cc_catalog.HUD_LAYOUT_HW_STORAGE_IDENTITY}"
        f"+0x{_cc_catalog.HUD_LAYOUT_HW_DISPLACEMENT:x})"
    )
    expected_matches = [
        row
        for row in expected
        if row.get("form") == "call"
        and row.get("dispatch") == "indirect"
        and row.get("identity_kind") == "virtual-slot"
        and row.get("target_identity") == ""
        and row.get("storage_identity") == desired_storage
        and row.get("slot_displacement")
        == _cc_catalog.HUD_LAYOUT_HW_SLOT_DISPLACEMENT
        and row.get("cleanup_bytes") is None
    ]
    if len(expected_matches) != 1:
        raise ValueError(
            "HUD-layout-HW absolute-load bridge requires one exact "
            "retail-derived load(storage+0x1b4) virtual-slot contract"
        )
    expected_edx_matches = [
        row
        for row in expected
        if row.get("form") == "call"
        and row.get("dispatch") == "indirect"
        and row.get("identity_kind") == "virtual-slot"
        and row.get("target_identity") == ""
        and row.get("storage_identity") == desired_storage
        and row.get("slot_displacement")
        == _cc_catalog.HUD_LAYOUT_HW_EDX_SLOT_DISPLACEMENT
        and row.get("cleanup_bytes") is None
    ]
    if len(expected_edx_matches) != 1:
        raise ValueError(
            "HUD-layout-HW absolute-load bridge requires one exact "
            "retail-derived EDX load(storage+0x1b4) slot-0x68 contract"
        )

    definition = candidate.caller_definition
    if (
        definition is None
        or definition.symbol != _cc_catalog.HUD_LAYOUT_HW_VPTR_CALLER_SYMBOL
        or len(definition.data) != len(definition.relocation_mask)
        or len(definition.data) < _cc_catalog.HUD_LAYOUT_HW_EDX_CALL_OFFSET + 3
    ):
        raise ValueError(
            "HUD-layout-HW absolute-load bridge requires the exact current "
            "caller object contribution"
        )
    instruction_offsets = _cc_cfg._instruction_runtime_addresses(
        candidate.instructions,
        source="cod",
        caller_start=0,
    )
    load_rows = [
        instruction
        for instruction, offset in zip(
            candidate.instructions, instruction_offsets
        )
        if offset == _cc_catalog.HUD_LAYOUT_HW_LOAD_OFFSET
    ]
    call_rows = [
        instruction
        for instruction, offset in zip(
            candidate.instructions, instruction_offsets
        )
        if offset == _cc_catalog.HUD_LAYOUT_HW_CALL_OFFSET
    ]
    edx_load_rows = [
        instruction
        for instruction, offset in zip(
            candidate.instructions, instruction_offsets
        )
        if offset == _cc_catalog.HUD_LAYOUT_HW_EDX_LOAD_OFFSET
    ]
    edx_call_rows = [
        instruction
        for instruction, offset in zip(
            candidate.instructions, instruction_offsets
        )
        if offset == _cc_catalog.HUD_LAYOUT_HW_EDX_CALL_OFFSET
    ]
    if (
        len(load_rows) != 1
        or len(call_rows) != 1
        or len(edx_load_rows) != 1
        or len(edx_call_rows) != 1
    ):
        raise ValueError(
            "HUD-layout-HW absolute-load bridge requires unique EAX +0x13/"
            "+0x1f and EDX +0x22/+0x31 load/call instructions"
        )
    load = load_rows[0]
    call = call_rows[0]
    edx_load = edx_load_rows[0]
    edx_call = edx_call_rows[0]
    load_match = re.fullmatch(
        r"mov\s+eax\s*,\s*(?P<source>.+)",
        load.raw_text.strip(),
        flags=re.IGNORECASE,
    )
    load_expression = (
        _cc_targets._exact_memory_expression(load_match.group("source"))
        if load_match is not None
        else ""
    )
    edx_load_match = re.fullmatch(
        r"mov\s+edx\s*,\s*(?P<source>.+)",
        edx_load.raw_text.strip(),
        flags=re.IGNORECASE,
    )
    edx_load_expression = (
        _cc_targets._exact_memory_expression(edx_load_match.group("source"))
        if edx_load_match is not None
        else ""
    )
    if (
        _cc_cfg._instruction_mnemonic(load) != "mov"
        or tuple(value.lower() for value in load.bytes)
        != ("a1", "b4", "01", "00", "00")
        or load_expression
        not in {
            f"{_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL}+436",
            f"{_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL}+0x1b4",
        }
        or _cc_cfg._instruction_mnemonic(call) != "call"
        or tuple(value.lower() for value in call.bytes)
        != ("ff", "50", "64")
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(call))
        not in {"eax+100", "eax+0x64"}
        or _cc_cfg._instruction_mnemonic(edx_load) != "mov"
        or tuple(value.lower() for value in edx_load.bytes)
        != ("8b", "15", "b4", "01", "00", "00")
        or edx_load_expression
        not in {
            f"{_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL}+436",
            f"{_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL}+0x1b4",
        }
        or _cc_cfg._instruction_mnemonic(edx_call) != "call"
        or tuple(value.lower() for value in edx_call.bytes)
        != ("ff", "52", "68")
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(edx_call))
        not in {"edx+104", "edx+0x68"}
    ):
        raise ValueError(
            "HUD-layout-HW absolute-load bridge requires exact VC5 A1/FF5064 "
            "EAX and 8B15/FF5268 EDX aggregate+0x1b4 load/call pairs"
        )

    load_end = _cc_catalog.HUD_LAYOUT_HW_LOAD_OFFSET + 5
    call_end = _cc_catalog.HUD_LAYOUT_HW_CALL_OFFSET + 3
    relocation_end = _cc_catalog.HUD_LAYOUT_HW_RELOCATION_OFFSET + 4
    edx_load_end = _cc_catalog.HUD_LAYOUT_HW_EDX_LOAD_OFFSET + 6
    edx_call_end = _cc_catalog.HUD_LAYOUT_HW_EDX_CALL_OFFSET + 3
    edx_relocation_end = _cc_catalog.HUD_LAYOUT_HW_EDX_RELOCATION_OFFSET + 4
    load_relocations = [
        relocation
        for relocation in definition.relocations
        if relocation.offset == _cc_catalog.HUD_LAYOUT_HW_RELOCATION_OFFSET
    ]
    edx_load_relocations = [
        relocation
        for relocation in definition.relocations
        if relocation.offset == _cc_catalog.HUD_LAYOUT_HW_EDX_RELOCATION_OFFSET
    ]
    aggregate_references = [
        relocation
        for relocation in definition.relocations
        if relocation.symbol_name.casefold()
        == _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL.casefold()
    ]
    undefined_rows = [
        name
        for name in definition.undefined_external_data
        if name.casefold() == _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL.casefold()
    ]
    defined_rows = [
        name
        for name in definition.defined_external_data
        if name.casefold() == _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL.casefold()
    ]
    if (
        definition.data[_cc_catalog.HUD_LAYOUT_HW_LOAD_OFFSET:load_end]
        != b"\xa1\xb4\x01\x00\x00"
        or definition.data[_cc_catalog.HUD_LAYOUT_HW_CALL_OFFSET:call_end]
        != b"\xff\x50\x64"
        or definition.data[
            _cc_catalog.HUD_LAYOUT_HW_EDX_LOAD_OFFSET:edx_load_end
        ]
        != b"\x8b\x15\xb4\x01\x00\x00"
        or definition.data[
            _cc_catalog.HUD_LAYOUT_HW_EDX_CALL_OFFSET:edx_call_end
        ]
        != b"\xff\x52\x68"
        or definition.relocation_mask[_cc_catalog.HUD_LAYOUT_HW_LOAD_OFFSET]
        or not all(
            definition.relocation_mask[index]
            for index in range(
                _cc_catalog.HUD_LAYOUT_HW_RELOCATION_OFFSET, relocation_end
            )
        )
        or any(
            definition.relocation_mask[index]
            for index in range(_cc_catalog.HUD_LAYOUT_HW_CALL_OFFSET, call_end)
        )
        or any(
            definition.relocation_mask[index]
            for index in range(
                _cc_catalog.HUD_LAYOUT_HW_EDX_LOAD_OFFSET,
                _cc_catalog.HUD_LAYOUT_HW_EDX_RELOCATION_OFFSET,
            )
        )
        or not all(
            definition.relocation_mask[index]
            for index in range(
                _cc_catalog.HUD_LAYOUT_HW_EDX_RELOCATION_OFFSET,
                edx_relocation_end,
            )
        )
        or any(
            definition.relocation_mask[index]
            for index in range(
                _cc_catalog.HUD_LAYOUT_HW_EDX_CALL_OFFSET,
                edx_call_end,
            )
        )
        or len(load_relocations) != 1
        or load_relocations[0].type != IMAGE_REL_I386_DIR32
        or load_relocations[0].symbol_name
        != _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL
        or struct.unpack_from(
            "<I",
            definition.data,
            _cc_catalog.HUD_LAYOUT_HW_RELOCATION_OFFSET,
        )[0]
        != _cc_catalog.HUD_LAYOUT_HW_DISPLACEMENT
        or len(edx_load_relocations) != 1
        or edx_load_relocations[0].type != IMAGE_REL_I386_DIR32
        or edx_load_relocations[0].symbol_name
        != _cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL
        or struct.unpack_from(
            "<I",
            definition.data,
            _cc_catalog.HUD_LAYOUT_HW_EDX_RELOCATION_OFFSET,
        )[0]
        != _cc_catalog.HUD_LAYOUT_HW_DISPLACEMENT
        or not aggregate_references
        or load_relocations[0] not in aggregate_references
        or edx_load_relocations[0] not in aggregate_references
        or undefined_rows != [_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL]
        or defined_rows
    ):
        raise ValueError(
            "HUD-layout-HW absolute-load bridge requires one exact fully "
            "masked DIR32 aggregate relocation/addend and undefined data symbol"
        )

    return {
        normalize_address(hex(_cc_catalog.HUD_LAYOUT_HW_LOAD_OFFSET)): ReviewedAbsoluteStorageLoadBridge(
            register="eax",
            aggregate_symbol=_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL,
            displacement=_cc_catalog.HUD_LAYOUT_HW_DISPLACEMENT,
            access_width=_cc_catalog.HUD_LAYOUT_HW_ACCESS_WIDTH,
            storage_identity=_cc_catalog.HUD_LAYOUT_HW_STORAGE_IDENTITY,
        ),
        normalize_address(hex(_cc_catalog.HUD_LAYOUT_HW_EDX_LOAD_OFFSET)): ReviewedAbsoluteStorageLoadBridge(
            register="edx",
            aggregate_symbol=_cc_catalog.HUD_LAYOUT_HW_AGGREGATE_SYMBOL,
            displacement=_cc_catalog.HUD_LAYOUT_HW_DISPLACEMENT,
            access_width=_cc_catalog.HUD_LAYOUT_HW_ACCESS_WIDTH,
            storage_identity=_cc_catalog.HUD_LAYOUT_HW_STORAGE_IDENTITY,
        ),
    }


def _hud_layout_member_vptr_storage_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedMemberVptrStorageBridge]:
    """Bridge one exact member-derived vptr load and receiver topology."""
    from _recoil.call_contract.records import ReviewedMemberVptrStorageBridge
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_LAYOUT_HW_VPTR_CALLER_START:
        return {}
    caller_addresses = sorted(
        address
        for address, identity in indexes.by_address.items()
        if identity == caller_identity
    )
    if (
        caller_identity != _cc_catalog.HUD_LAYOUT_HW_VPTR_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_LAYOUT_HW_VPTR_CALLER_END_EXCLUSIVE
        or caller_addresses != [_cc_catalog.HUD_LAYOUT_HW_VPTR_CALLER_START]
        or caller_identity in indexes.provider_ids
    ):
        raise ValueError(
            "HUD-layout member-vptr bridge requires the exact reviewed "
            "authored 0x40f2e0 caller identity and extent"
        )

    receiver_provenance = (
        f"address(this+0x{_cc_catalog.HUD_LAYOUT_MEMBER_RECEIVER_OFFSET:x})"
    )
    storage_identity = (
        f"load(this+0x{_cc_catalog.HUD_LAYOUT_MEMBER_RECEIVER_OFFSET:x})"
    )
    expected_rows = [
        row
        for row in expected
        if row.get("ordinal") == _cc_catalog.HUD_LAYOUT_MEMBER_EXPECTED_ORDINAL
        and row.get("form") == "call"
        and row.get("dispatch") == "indirect"
        and row.get("identity_kind") == "virtual-slot"
        and row.get("target_identity") == ""
        and row.get("storage_identity") == storage_identity
        and row.get("slot_displacement")
        == _cc_catalog.HUD_LAYOUT_MEMBER_VPTR_SLOT_DISPLACEMENT
        and row.get("cleanup_bytes") is None
    ]
    if len(expected_rows) != 1:
        raise ValueError(
            "HUD-layout member-vptr bridge requires the exact independently "
            "retail-derived ordinal-6 member storage/slot contract"
        )

    definition = candidate.caller_definition
    if (
        definition is None
        or definition.symbol != _cc_catalog.HUD_LAYOUT_HW_VPTR_CALLER_SYMBOL
        or len(definition.data) != len(definition.relocation_mask)
        or len(definition.data) < _cc_catalog.HUD_LAYOUT_MEMBER_VPTR_CALL_OFFSET + 3
    ):
        raise ValueError(
            "HUD-layout member-vptr bridge requires the exact current caller "
            "object contribution"
        )

    instruction_offsets = _cc_cfg._instruction_runtime_addresses(
        candidate.instructions,
        source="cod",
        caller_start=0,
    )
    instruction_by_offset: dict[int, Instruction] = {}
    duplicate_offsets: set[int] = set()
    for instruction, offset in zip(
        candidate.instructions,
        instruction_offsets,
    ):
        if offset is None:
            continue
        if offset in instruction_by_offset:
            duplicate_offsets.add(offset)
        instruction_by_offset[offset] = instruction
    required_offsets = {
        _cc_catalog.HUD_LAYOUT_MEMBER_THIS_MOVE_OFFSET,
        _cc_catalog.HUD_LAYOUT_MEMBER_LEA_OFFSET,
        _cc_catalog.HUD_LAYOUT_MEMBER_DIRECT_CALL_OFFSET,
        _cc_catalog.HUD_LAYOUT_MEMBER_VPTR_LOAD_OFFSET,
        _cc_catalog.HUD_LAYOUT_MEMBER_RECEIVER_MOVE_OFFSET,
        _cc_catalog.HUD_LAYOUT_MEMBER_VPTR_CALL_OFFSET,
    }
    if (
        duplicate_offsets & required_offsets
        or not required_offsets.issubset(instruction_by_offset)
    ):
        raise ValueError(
            "HUD-layout member-vptr bridge requires unique instructions at "
            "the reviewed receiver, load, and call offsets"
        )

    approved_this_register_rows = {
        "edi": (
            ("8b", "f9"),
            r"mov\s+edi\s*,\s*ecx",
            ("8d", "9f", "b4", "01", "00", "00"),
            r"lea\s+ebx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[edi(?:\+436|\+0x1b4)\]",
        ),
        "esi": (
            ("8b", "f1"),
            r"mov\s+esi\s*,\s*ecx",
            ("8d", "9e", "b4", "01", "00", "00"),
            r"lea\s+ebx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[esi(?:\+436|\+0x1b4)\]",
        ),
    }
    this_instruction = instruction_by_offset[
        _cc_catalog.HUD_LAYOUT_MEMBER_THIS_MOVE_OFFSET
    ]
    this_register_rows = [
        (register, move_body, lea_body, lea_pattern)
        for register, (
            move_body,
            move_pattern,
            lea_body,
            lea_pattern,
        ) in approved_this_register_rows.items()
        if (
            tuple(
                value.lower() for value in this_instruction.bytes
            )
            == move_body
            and re.fullmatch(
                move_pattern,
                this_instruction.raw_text.strip(),
                flags=re.IGNORECASE,
            )
            is not None
        )
    ]
    if len(this_register_rows) != 1:
        raise ValueError(
            "HUD-layout member-vptr bridge requires one exact reviewed "
            "EDI-or-ESI MOV reg,ECX binding at +0xf"
        )
    this_register, this_move_body, this_lea_body, this_lea_pattern = (
        this_register_rows[0]
    )

    expected_instruction_rows = {
        _cc_catalog.HUD_LAYOUT_MEMBER_THIS_MOVE_OFFSET: (
            this_move_body,
            rf"mov\s+{this_register}\s*,\s*ecx",
        ),
        _cc_catalog.HUD_LAYOUT_MEMBER_LEA_OFFSET: (
            this_lea_body,
            this_lea_pattern,
        ),
        _cc_catalog.HUD_LAYOUT_MEMBER_DIRECT_CALL_OFFSET: (
            ("e8", "00", "00", "00", "00"),
            rf"call\s+{re.escape(_cc_catalog.HUD_LAYOUT_MEMBER_APPLY_IMAGE_SYMBOL)}"
            r"(?:\s*;.*)?",
        ),
        _cc_catalog.HUD_LAYOUT_MEMBER_VPTR_LOAD_OFFSET: (
            ("8b", "13"),
            r"mov\s+edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[ebx\]",
        ),
        _cc_catalog.HUD_LAYOUT_MEMBER_RECEIVER_MOVE_OFFSET: (
            ("8b", "cb"),
            r"mov\s+ecx\s*,\s*ebx",
        ),
        _cc_catalog.HUD_LAYOUT_MEMBER_VPTR_CALL_OFFSET: (
            ("ff", "52", "68"),
            r"call\s+(?:dword\s+(?:ptr\s+)?)?"
            r"\[edx(?:\+104|\+0x68)\]",
        ),
    }
    for offset, (body, text_pattern) in expected_instruction_rows.items():
        instruction = instruction_by_offset[offset]
        if (
            tuple(value.lower() for value in instruction.bytes) != body
            or re.fullmatch(
                text_pattern,
                instruction.raw_text.strip(),
                flags=re.IGNORECASE,
            )
            is None
        ):
            raise ValueError(
                "HUD-layout member-vptr bridge requires exact candidate "
                f"instruction bytes and operands at +0x{offset:x}"
            )

    exact_bodies = {
        _cc_catalog.HUD_LAYOUT_MEMBER_THIS_MOVE_OFFSET: bytes.fromhex(
            " ".join(this_move_body)
        ),
        _cc_catalog.HUD_LAYOUT_MEMBER_LEA_OFFSET: bytes.fromhex(
            " ".join(this_lea_body)
        ),
        _cc_catalog.HUD_LAYOUT_MEMBER_VPTR_LOAD_OFFSET: b"\x8b\x13",
        _cc_catalog.HUD_LAYOUT_MEMBER_RECEIVER_MOVE_OFFSET: b"\x8b\xcb",
        _cc_catalog.HUD_LAYOUT_MEMBER_VPTR_CALL_OFFSET: b"\xff\x52\x68",
    }
    for offset, body in exact_bodies.items():
        end = offset + len(body)
        if (
            definition.data[offset:end] != body
            or any(definition.relocation_mask[offset:end])
            or any(
                relocation.offset < end
                and relocation.offset + 4 > offset
                for relocation in definition.relocations
            )
        ):
            raise ValueError(
                "HUD-layout member-vptr bridge requires exact relocation-free "
                f"caller bytes at +0x{offset:x}"
            )

    direct_call_relocations = [
        relocation
        for relocation in definition.relocations
        if relocation.offset
        == _cc_catalog.HUD_LAYOUT_MEMBER_DIRECT_CALL_RELOCATION_OFFSET
    ]
    undefined_rows = [
        name
        for name in definition.undefined_external_functions
        if name.casefold()
        == _cc_catalog.HUD_LAYOUT_MEMBER_APPLY_IMAGE_SYMBOL.casefold()
    ]
    defined_rows = [
        name
        for name in definition.defined_external_functions
        if name.casefold()
        == _cc_catalog.HUD_LAYOUT_MEMBER_APPLY_IMAGE_SYMBOL.casefold()
    ]
    direct_call_end = _cc_catalog.HUD_LAYOUT_MEMBER_DIRECT_CALL_OFFSET + 5
    if (
        definition.data[
            _cc_catalog.HUD_LAYOUT_MEMBER_DIRECT_CALL_OFFSET:direct_call_end
        ]
        != b"\xe8\x00\x00\x00\x00"
        or definition.relocation_mask[
            _cc_catalog.HUD_LAYOUT_MEMBER_DIRECT_CALL_OFFSET
        ]
        or not all(
            definition.relocation_mask[index]
            for index in range(
                _cc_catalog.HUD_LAYOUT_MEMBER_DIRECT_CALL_RELOCATION_OFFSET,
                direct_call_end,
            )
        )
        or len(direct_call_relocations) != 1
        or direct_call_relocations[0].type != IMAGE_REL_I386_REL32
        or direct_call_relocations[0].symbol_name
        != _cc_catalog.HUD_LAYOUT_MEMBER_APPLY_IMAGE_SYMBOL
        or struct.unpack_from(
            "<I",
            definition.data,
            _cc_catalog.HUD_LAYOUT_MEMBER_DIRECT_CALL_RELOCATION_OFFSET,
        )[0]
        != 0
        or undefined_rows + defined_rows
        != [_cc_catalog.HUD_LAYOUT_MEMBER_APPLY_IMAGE_SYMBOL]
    ):
        raise ValueError(
            "HUD-layout member-vptr bridge requires one exact zero-addend "
            "REL32 ApplyImageWidget call between receiver formation and load"
        )
    direct_identity = indexes.by_candidate_name.get(
        _cc_catalog.HUD_LAYOUT_MEMBER_APPLY_IMAGE_SYMBOL,
        "",
    )
    if not direct_identity or direct_identity in indexes.provider_ids:
        raise ValueError(
            "HUD-layout member-vptr bridge requires the exact authored "
            "ApplyImageWidget candidate identity"
        )

    ordered_indices = {
        offset: index
        for index, offset in enumerate(instruction_offsets)
        if offset in required_offsets
    }
    first_index = ordered_indices[_cc_catalog.HUD_LAYOUT_MEMBER_THIS_MOVE_OFFSET]
    call_index = ordered_indices[_cc_catalog.HUD_LAYOUT_MEMBER_VPTR_CALL_OFFSET]
    if (
        any(
            _cc_cfg._instruction_mnemonic(instruction).startswith("j")
            or _cc_cfg._instruction_mnemonic(instruction)
            in {"loop", "loope", "loopne", "loopnz", "loopz"}
            for instruction in candidate.instructions[
                first_index:call_index + 1
            ]
        )
        or any(
            first_index <= index <= call_index
            for index in candidate.local_control_flow_indices
        )
        or any(
            first_index <= target <= call_index
            for targets in candidate.local_control_flow_targets.values()
            for target in targets
        )
    ):
        raise ValueError(
            "HUD-layout member-vptr bridge rejects alternate control-flow "
            "paths through the reviewed topology"
        )

    registers = _cc_cfg._empty_register_state()
    registers.update({"ecx": "this", "esp": "stack", "ebp": "frame"})
    for index, instruction in enumerate(candidate.instructions[:call_index]):
        offset = instruction_offsets[index]
        if _cc_cfg._instruction_mnemonic(instruction) == "call":
            for volatile in ("eax", "ecx", "edx"):
                registers[volatile] = ""
            continue
        if offset == _cc_catalog.HUD_LAYOUT_MEMBER_VPTR_LOAD_OFFSET:
            if registers.get("ebx") != receiver_provenance:
                raise ValueError(
                    "HUD-layout member-vptr bridge requires one reaching EBX "
                    "receiver definition from the exact +0x90 LEA"
                )
            registers["edx"] = storage_identity
            continue
        _cc_receiver_storage._update_register_state(
            instruction,
            registers,
            assembly_source="cod",
            indexes=indexes,
            reviewed_register_storage_bridges={},
        )
        if (
            offset is not None
            and _cc_catalog.HUD_LAYOUT_MEMBER_THIS_MOVE_OFFSET
            <= offset
            <= _cc_catalog.HUD_LAYOUT_MEMBER_LEA_OFFSET
            and registers.get(this_register) != "this"
        ):
            raise ValueError(
                "HUD-layout member-vptr bridge requires the exact "
                f"{this_register.upper()} definition from +0xf to retain "
                "this through the +0x90 receiver LEA"
            )
        if (
            offset == _cc_catalog.HUD_LAYOUT_MEMBER_LEA_OFFSET
            and registers.get("ebx") != receiver_provenance
        ):
            raise ValueError(
                "HUD-layout member-vptr bridge requires the exact bound "
                f"{this_register.upper()} receiver at +0x90"
            )
    if (
        registers.get("ebx") != receiver_provenance
        or registers.get("edx") != storage_identity
        or registers.get("ecx") != receiver_provenance
    ):
        raise ValueError(
            "HUD-layout member-vptr bridge requires one exact reaching vptr "
            "load and matching ECX/EBX receiver at +0xa8"
        )

    return {
        normalize_address(hex(_cc_catalog.HUD_LAYOUT_MEMBER_VPTR_LOAD_OFFSET)):
        ReviewedMemberVptrStorageBridge(
            register="edx",
            source_register="ebx",
            source_provenance=receiver_provenance,
            receiver_register="ecx",
            receiver_provenance=receiver_provenance,
            storage_identity=storage_identity,
            slot_displacement=_cc_catalog.HUD_LAYOUT_MEMBER_VPTR_SLOT_DISPLACEMENT,
            call_address=normalize_address(
                hex(_cc_catalog.HUD_LAYOUT_MEMBER_VPTR_CALL_OFFSET)
            ),
        )
    }


def _hud_ui_mgr_layout_array_loop_vptr_storage_bridges(
    instructions: Sequence[Instruction],
    *,
    source: str,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    candidate: CandidateAssembly | None = None,
) -> dict[str, ReviewedLoopVptrStorageBridge]:
    """Prove the exact 23-element HudUiPanelSimple initialization loop.

    Retail and candidate encodings are intentionally validated by distinct
    fixed rows.  This bridge does not recognize arbitrary induction loops.
    """
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_CALLER_START:
        return {}
    if (
        caller_identity != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_CALLER_END_EXCLUSIVE
        or source not in {"bn", "cod"}
        or (source == "bn") != (candidate is None)
    ):
        raise ValueError(
            "HUD layout-array loop bridge requires the exact reviewed "
            "0x40f4c0 caller, extent, and assembly side"
        )

    start = address_value(normalized_start)
    end = address_value(caller_end_exclusive)
    addresses = _cc_cfg._instruction_runtime_addresses(
        instructions,
        source=source,
        caller_start=start,
    )
    address_counts: dict[int, int] = {}
    for address in addresses:
        if address is not None:
            address_counts[address] = address_counts.get(address, 0) + 1
    instruction_by_address = {
        address: instruction
        for instruction, address in zip(instructions, addresses)
        if address is not None and address_counts.get(address) == 1
    }
    index_by_address = {
        address: index
        for index, address in enumerate(addresses)
        if address is not None and address_counts.get(address) == 1
    }

    candidate_variant = ""
    candidate_ordered_keys: tuple[str, ...] = ()
    first_vptr_register = "eax"
    second_vptr_register = "edx"
    x_register = ""
    if source == "bn":
        fixed = {
            "base": 0x40F53E,
            "seed": 0x40F564,
            "array_constructor_target": 0x40F55F,
            "array_count_argument": 0x40F567,
            "array_stride_argument": 0x40F569,
            "array_constructor_call": 0x40F574,
            "count": 0x40F589,
            "header": 0x40F58E,
            "load_first": 0x40F58E,
            "receiver_first": 0x40F593,
            "call_first": 0x40F595,
            "load_second": 0x40F5A0,
            "receiver_second": 0x40F5A2,
            "call_second": 0x40F5A6,
            "stride": 0x40F5A9,
            "decrement": 0x40F5B2,
            "backedge": 0x40F5B3,
        }
        expected_rows = {
            fixed["base"]: (
                ("8b", "f8"),
                r"mov\s+edi\s*,\s*eax",
            ),
            fixed["seed"]: (
                ("8d", "77", "20"),
                r"lea\s+esi\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
                r"\[edi(?:\+32|\+0x20)\]",
            ),
            fixed["array_constructor_target"]: (
                ("68", "b0", "fa", "40", "00"),
                r"push\s+(?:0x)?40fab0",
            ),
            fixed["array_count_argument"]: (
                ("6a", "17"),
                r"push\s+(?:23|0x17)",
            ),
            fixed["array_stride_argument"]: (
                ("68", "a4", "02", "00", "00"),
                r"push\s+(?:676|0x2a4)",
            ),
            fixed["array_constructor_call"]: (
                ("e8", "87", "6a", "0b", "00"),
                r"call\s+MSVC_EH_ArrayConstructor(?:\s*;.*)?",
            ),
            fixed["count"]: (
                ("bd", "17", "00", "00", "00"),
                r"mov\s+ebp\s*,\s*(?:23|0x17)",
            ),
            fixed["load_first"]: (
                ("8b", "06"),
                r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[esi\]",
            ),
            fixed["receiver_first"]: (
                ("8b", "ce"),
                r"mov\s+ecx\s*,\s*esi",
            ),
            fixed["call_first"]: (
                ("ff", "50", "0c"),
                r"call\s+(?:dword\s+(?:ptr\s+)?)?"
                r"\[eax(?:\+12|\+0xc)\]",
            ),
            fixed["load_second"]: (
                ("8b", "16"),
                r"mov\s+edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[esi\]",
            ),
            fixed["receiver_second"]: (
                ("8b", "ce"),
                r"mov\s+ecx\s*,\s*esi",
            ),
            fixed["call_second"]: (
                ("ff", "52", "60"),
                r"call\s+(?:dword\s+(?:ptr\s+)?)?"
                r"\[edx(?:\+96|\+0x60)\]",
            ),
            fixed["stride"]: (
                ("81", "c6", "a4", "02", "00", "00"),
                r"add\s+esi\s*,\s*(?:676|0x2a4)",
            ),
            fixed["decrement"]: (
                ("4d",),
                r"dec\s+ebp",
            ),
            fixed["backedge"]: (
                ("75", "d9"),
                r"jne\s+(?:0x)?40f58e",
            ),
        }
        bridge_addresses = {
            fixed["call_first"]: ReviewedLoopVptrStorageBridge(
                register="eax",
                storage_identity=(
                    _cc_catalog.HUD_UI_MGR_LAYOUT_ARRAY_STORAGE_IDENTITY
                ),
                slot_displacement=0x0C,
                assembly_source="bn",
            ),
            fixed["call_second"]: ReviewedLoopVptrStorageBridge(
                register="edx",
                storage_identity=(
                    _cc_catalog.HUD_UI_MGR_LAYOUT_ARRAY_STORAGE_IDENTITY
                ),
                slot_displacement=0x60,
                assembly_source="bn",
            ),
        }
        base_register = "edi"
    else:
        destructor_pattern = (
            rf"push\s+(?:OFFSET\s+FLAT:)?"
            rf"{re.escape(_cc_catalog.HUD_UI_MGR_PANEL_SIMPLE_DESTRUCTOR_SYMBOL)}"
            r"(?:\s*;.*)?"
        )
        anchor_addresses = [
            address
            for address, instruction in instruction_by_address.items()
            if (
                tuple(value.lower() for value in instruction.bytes)
                == ("68", "00", "00", "00", "00")
                and re.fullmatch(
                    destructor_pattern,
                    instruction.raw_text.strip(),
                    flags=re.IGNORECASE,
                )
                is not None
            )
        ]
        if len(anchor_addresses) != 1:
            raise ValueError(
                "HUD layout-array loop bridge requires one unique structural "
                "candidate destructor anchor"
            )
        anchor = anchor_addresses[0]
        current_fixed = {
            "base": anchor - 0x1C,
            "null_compare": anchor - 0x13,
            "null_branch": anchor - 0x09,
            "base_receiver": anchor - 0x07,
            "base_constructor_call": anchor - 0x05,
            "array_destructor_target": anchor,
            "array_constructor_target": anchor + 0x05,
            "seed": anchor + 0x0A,
            "array_count_argument": anchor + 0x0D,
            "array_stride_argument": anchor + 0x0F,
            "array_base_push": anchor + 0x14,
            "array_eh_state": anchor + 0x15,
            "array_constructor_call": anchor + 0x1A,
            "vtable_store": anchor + 0x24,
            "x_seed": anchor + 0x2A,
            "count": anchor + 0x2F,
            "header": anchor + 0x34,
            "load_first": anchor + 0x34,
            "arg_y": anchor + 0x36,
            "arg_x": anchor + 0x37,
            "receiver_first": anchor + 0x39,
            "call_first": anchor + 0x3B,
            "add_child_receiver": anchor + 0x3E,
            "add_child_argument": anchor + 0x40,
            "add_child_call": anchor + 0x41,
            "load_second": anchor + 0x46,
            "receiver_second": anchor + 0x48,
            "arg_second": anchor + 0x4A,
            "call_second": anchor + 0x4C,
            "x_increment": anchor + 0x4F,
            "stride": anchor + 0x52,
            "decrement": anchor + 0x58,
            "backedge": anchor + 0x59,
            "post_load": anchor + 0x5B,
            "post_argument": anchor + 0x5D,
            "post_receiver": anchor + 0x5F,
            "post_call": anchor + 0x61,
            "zero_register": anchor + 0x64,
            "minus_one_register": anchor + 0x66,
            "tail_branch": anchor + 0x69,
            "null_base": anchor + 0x6B,
            "tail_join": anchor + 0x6D,
        }
        current_rows = {
            current_fixed["base"]: (
                ("8b", "f8"),
                r"mov\s+edi\s*,\s*eax",
            ),
            current_fixed["null_compare"]: (
                ("3b", "fd"),
                r"cmp\s+edi\s*,\s*ebp",
            ),
            current_fixed["null_branch"]: (
                ("74", "72"),
                r"je\s+(?:(?:short|near)\s+)?\$L[0-9A-Za-z_]+",
            ),
            current_fixed["base_receiver"]: (
                ("8b", "cf"),
                r"mov\s+ecx\s*,\s*edi",
            ),
            current_fixed["base_constructor_call"]: (
                ("e8", "00", "00", "00", "00"),
                r"call\s+\?\?0HudUiContainer@@QAE@XZ(?:\s*;.*)?",
            ),
            current_fixed["array_destructor_target"]: (
                ("68", "00", "00", "00", "00"),
                destructor_pattern,
            ),
            current_fixed["array_constructor_target"]: (
                ("68", "00", "00", "00", "00"),
                rf"push\s+(?:OFFSET\s+FLAT:)?"
                rf"{re.escape(
                    _cc_catalog.HUD_UI_MGR_PANEL_SIMPLE_DEFAULT_CONSTRUCTOR_CLOSURE_SYMBOL
                )}"
                r"(?:\s*;.*)?",
            ),
            current_fixed["seed"]: (
                ("8d", "77", "20"),
                r"lea\s+esi\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
                r"\[edi(?:\+32|\+0x20)\]",
            ),
            current_fixed["array_count_argument"]: (
                ("6a", "17"),
                r"push\s+(?:23|0x17)",
            ),
            current_fixed["array_stride_argument"]: (
                ("68", "a4", "02", "00", "00"),
                r"push\s+(?:676|0x2a4)",
            ),
            current_fixed["array_base_push"]: (("56",), r"push\s+esi"),
            current_fixed["array_eh_state"]: (
                ("c6", "44", "24", "3c", "02"),
                r"mov\s+byte\s+__\$EHRec\$\[esp\+72\]\s*,\s*"
                r"(?:2|0x2)",
            ),
            current_fixed["array_constructor_call"]: (
                ("e8", "00", "00", "00", "00"),
                rf"call\s+{re.escape(
                    _cc_catalog.HUD_UI_MGR_LAYOUT_ARRAY_EH_CONSTRUCTOR_SYMBOL
                )}(?:\s*;.*)?",
            ),
            current_fixed["vtable_store"]: (
                ("c7", "07", "00", "00", "00", "00"),
                r"mov\s+(?:dword\s+(?:ptr\s+)?)?\[edi\]\s*,\s*"
                r"(?:OFFSET\s+FLAT:)?\?\?_7HudUiStringMenu@@6B@"
                r"(?:\s*;.*)?",
            ),
            current_fixed["x_seed"]: (
                ("bb", "5f", "00", "00", "00"),
                r"mov\s+ebx\s*,\s*(?:95|0x5f)",
            ),
            current_fixed["count"]: (
                ("bd", "17", "00", "00", "00"),
                r"mov\s+ebp\s*,\s*(?:23|0x17)",
            ),
            current_fixed["load_first"]: (
                ("8b", "06"),
                r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[esi\]",
            ),
            current_fixed["arg_y"]: (("53",), r"push\s+ebx"),
            current_fixed["arg_x"]: (("6a", "05"), r"push\s+5"),
            current_fixed["receiver_first"]: (
                ("8b", "ce"),
                r"mov\s+ecx\s*,\s*esi",
            ),
            current_fixed["call_first"]: (
                ("ff", "50", "0c"),
                r"call\s+(?:dword\s+(?:ptr\s+)?)?"
                r"\[eax(?:\+12|\+0xc)\]",
            ),
            current_fixed["add_child_receiver"]: (
                ("8b", "cf"),
                r"mov\s+ecx\s*,\s*edi",
            ),
            current_fixed["add_child_argument"]: (("56",), r"push\s+esi"),
            current_fixed["add_child_call"]: (
                ("e8", "00", "00", "00", "00"),
                rf"call\s+{re.escape(_cc_catalog.HUD_SHIELD_LAYOUT_ADD_CHILD_SYMBOL)}"
                r"(?:\s*;.*)?",
            ),
            current_fixed["load_second"]: (
                ("8b", "16"),
                r"mov\s+edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[esi\]",
            ),
            current_fixed["receiver_second"]: (
                ("8b", "ce"),
                r"mov\s+ecx\s*,\s*esi",
            ),
            current_fixed["arg_second"]: (("6a", "01"), r"push\s+1"),
            current_fixed["call_second"]: (
                ("ff", "52", "60"),
                r"call\s+(?:dword\s+(?:ptr\s+)?)?"
                r"\[edx(?:\+96|\+0x60)\]",
            ),
            current_fixed["x_increment"]: (
                ("83", "c3", "0f"),
                r"add\s+ebx\s*,\s*(?:15|0xf)",
            ),
            current_fixed["stride"]: (
                ("81", "c6", "a4", "02", "00", "00"),
                r"add\s+esi\s*,\s*(?:676|0x2a4)",
            ),
            current_fixed["decrement"]: (("4d",), r"dec\s+ebp"),
            current_fixed["backedge"]: (
                ("75", "d9"),
                r"jne\s+(?:(?:short|near)\s+)?\$L[0-9A-Za-z_]+",
            ),
            current_fixed["post_load"]: (
                ("8b", "07"),
                r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[edi\]",
            ),
            current_fixed["post_argument"]: (("6a", "01"), r"push\s+1"),
            current_fixed["post_receiver"]: (
                ("8b", "cf"),
                r"mov\s+ecx\s*,\s*edi",
            ),
            current_fixed["post_call"]: (
                ("ff", "50", "04"),
                r"call\s+(?:dword\s+(?:ptr\s+)?)?"
                r"\[eax(?:\+4|\+0x4)\]",
            ),
            current_fixed["zero_register"]: (
                ("33", "ed"),
                r"xor\s+ebp\s*,\s*ebp",
            ),
            current_fixed["minus_one_register"]: (
                ("83", "cb", "ff"),
                r"or\s+ebx\s*,\s*-1",
            ),
            current_fixed["tail_branch"]: (
                ("eb", "02"),
                r"jmp\s+(?:(?:short|near)\s+)?\$L[0-9A-Za-z_]+",
            ),
            current_fixed["null_base"]: (
                ("33", "ff"),
                r"xor\s+edi\s*,\s*edi",
            ),
        }

        def row_matches(
            address: int,
            body: tuple[str, ...],
            pattern: str,
        ) -> bool:
            return bool(
                address_counts.get(address) == 1
                and address in instruction_by_address
                and tuple(
                    value.lower()
                    for value in instruction_by_address[address].bytes
                )
                == body
                and re.fullmatch(
                    pattern,
                    instruction_by_address[address].raw_text.strip(),
                    flags=re.IGNORECASE,
                )
                is not None
            )

        def rows_match(
            rows: Mapping[int, tuple[tuple[str, ...], str]],
        ) -> bool:
            return all(
                row_matches(address, body, pattern)
                for address, (body, pattern) in rows.items()
            )

        if not rows_match(current_rows):
            mismatches = [normalize_address(address)
                for address, (encoding, pattern) in current_rows.items()
                if not row_matches(address, encoding, pattern)]
            raise ValueError("HUD native layout loop has changed instruction rows: " + str(mismatches))
        candidate_variant, fixed, expected_rows = "current", current_fixed, current_rows
        if candidate_variant == "current":
            bridge_addresses = {
                fixed["call_first"] - start:
                ReviewedLoopVptrStorageBridge(
                    register="eax",
                    storage_identity=(
                        _cc_catalog.HUD_UI_MGR_LAYOUT_ARRAY_STORAGE_IDENTITY
                    ),
                    slot_displacement=0x0C,
                    assembly_source="cod",
                ),
                fixed["call_second"] - start:
                ReviewedLoopVptrStorageBridge(
                    register="edx",
                    storage_identity=(
                        _cc_catalog.HUD_UI_MGR_LAYOUT_ARRAY_STORAGE_IDENTITY
                    ),
                    slot_displacement=0x60,
                    assembly_source="cod",
                ),
            }
            base_register = "edi"
            first_vptr_register = "eax"
            second_vptr_register = "edx"
            x_register = "ebx"
            candidate_ordered_keys = (
                "base", "null_compare", "null_branch", "base_receiver",
                "base_constructor_call", "array_destructor_target",
                "array_constructor_target", "seed",
                "array_count_argument", "array_stride_argument",
                "array_base_push", "array_eh_state",
                "array_constructor_call", "vtable_store", "x_seed",
                "count", "header", "load_first", "arg_y", "arg_x",
                "receiver_first", "call_first", "add_child_receiver",
                "add_child_argument", "add_child_call", "load_second",
                "receiver_second", "arg_second", "call_second",
                "x_increment", "stride", "decrement", "backedge",
                "post_load", "post_argument", "post_receiver",
                "post_call", "zero_register", "minus_one_register",
                "tail_branch", "null_base",
            )

    required_addresses = set(expected_rows)
    if (
        not required_addresses.issubset(instruction_by_address)
        or any(
            address_counts.get(address) != 1
            for address in required_addresses
        )
    ):
        raise ValueError(
            "HUD layout-array loop bridge requires one unique instruction "
            "at every reviewed topology address"
        )
    for address, (body, text_pattern) in expected_rows.items():
        instruction = instruction_by_address[address]
        if (
            tuple(value.lower() for value in instruction.bytes) != body
            or re.fullmatch(
                text_pattern,
                instruction.raw_text.strip(),
                flags=re.IGNORECASE,
            )
            is None
        ):
            raise ValueError(
                "HUD layout-array loop bridge requires exact instruction "
                f"bytes and operands at {normalize_address(address)}"
            )

    ordered_keys = (
        candidate_ordered_keys
        if source == "cod"
        else (
            "base",
            "seed",
            "count",
            "header",
            "load_first",
            "receiver_first",
            "call_first",
            "load_second",
            "receiver_second",
            "call_second",
            "stride",
            "decrement",
            "backedge",
        )
    )
    if any(
        fixed[left] > fixed[right]
        for left, right in zip(ordered_keys, ordered_keys[1:])
    ):
        raise ValueError(
            "HUD layout-array loop bridge requires the exact reviewed "
            "instruction order"
        )

    base_index = index_by_address[fixed["base"]]
    seed_index = index_by_address[fixed["seed"]]
    header_index = index_by_address[fixed["header"]]
    load_first_index = index_by_address[fixed["load_first"]]
    receiver_first_index = index_by_address[fixed["receiver_first"]]
    call_first_index = index_by_address[fixed["call_first"]]
    load_second_index = index_by_address[fixed["load_second"]]
    receiver_second_index = index_by_address[fixed["receiver_second"]]
    call_second_index = index_by_address[fixed["call_second"]]
    stride_index = index_by_address[fixed["stride"]]
    decrement_index = index_by_address[fixed["decrement"]]
    backedge_index = index_by_address[fixed["backedge"]]


    branch = _cc_cfg._exact_local_direct_branch(
        instruction_by_address[fixed["backedge"]],
        instruction_index=backedge_index,
        instruction_addresses=addresses,
        instruction_index_by_address=index_by_address,
        source=source,
        caller_start=start,
        caller_end=end,
    )
    if branch != ("conditional", header_index):
        raise ValueError(
            "HUD layout-array loop bridge requires the exact reviewed "
            "backedge to the loop header"
        )
    if source == "cod" and candidate_variant == "current":
        null_branch_index = index_by_address[fixed["null_branch"]]
        null_branch = _cc_cfg._exact_local_direct_branch(
            instruction_by_address[fixed["null_branch"]],
            instruction_index=null_branch_index,
            instruction_addresses=addresses,
            instruction_index_by_address=index_by_address,
            source=source,
            caller_start=start,
            caller_end=end,
        )
        tail_branch_index = index_by_address[fixed["tail_branch"]]
        tail_branch = _cc_cfg._exact_local_direct_branch(
            instruction_by_address[fixed["tail_branch"]],
            instruction_index=tail_branch_index,
            instruction_addresses=addresses,
            instruction_index_by_address=index_by_address,
            source=source,
            caller_start=start,
            caller_end=end,
        )
        if (
            null_branch
            != ("conditional", index_by_address[fixed["null_base"]])
            or tail_branch
            != ("unconditional", index_by_address[fixed["tail_join"]])
        ):
            raise ValueError(
                "HUD layout-array loop bridge requires the exact current "
                "allocation-null and post-loop join CFG"
            )
    for index in range(header_index, backedge_index + 1):
        instruction = instructions[index]
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        if (
            index != backedge_index
            and (
                mnemonic.startswith("j")
                or mnemonic
                in {"loop", "loope", "loopne", "loopnz", "loopz"}
            )
        ):
            raise ValueError(
                "HUD layout-array loop bridge rejects alternate CFG inside "
                "the reviewed loop"
            )
        if _cc_instructions.may_clobber_register(instruction, "esi") and index != stride_index:
            raise ValueError(
                "HUD layout-array loop bridge rejects cursor register "
                "clobber or alias drift"
            )
        if _cc_instructions.may_clobber_register(instruction, "ebp") and index != decrement_index:
            raise ValueError(
                "HUD layout-array loop bridge rejects induction-count "
                "clobber or alias drift"
            )
        if (
            x_register
            and _cc_instructions.may_clobber_register(instruction, x_register)
            and index != index_by_address[fixed["x_increment"]]
        ):
            raise ValueError(
                "HUD layout-array loop bridge rejects X induction-register "
                "clobber or alias drift"
            )
    for begin, finish, register in (
        (
            load_first_index + 1,
            call_first_index,
            first_vptr_register,
        ),
        (receiver_first_index + 1, call_first_index, "ecx"),
        (
            load_second_index + 1,
            call_second_index,
            second_vptr_register,
        ),
        (receiver_second_index + 1, call_second_index, "ecx"),
    ):
        if any(
            _cc_instructions.may_clobber_register(instructions[index], register)
            for index in range(begin, finish)
        ):
            raise ValueError(
                "HUD layout-array loop bridge rejects receiver or vptr "
                f"clobber of {register}"
            )
    if any(
        _cc_instructions.may_clobber_register(instructions[index], base_register)
        and not (
            source == "cod"
            and index == index_by_address[fixed["null_base"]]
        )
        for index in range(base_index + 1, seed_index)
    ):
        raise ValueError(
            "HUD layout-array loop bridge rejects base-register clobber "
            "before the exact element seed"
        )

    if candidate is not None:
        definition = candidate.caller_definition
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
            == _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_CALLER_START
        ]
        if (
            definition is None
            or definition.symbol != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_CALLER_SYMBOL
            or len(definition.data) != len(definition.relocation_mask)
            or len(definition.data) <= fixed["backedge"] - start + 1
            or not _cc_receiver_candidate._candidate_listing_matches_coff(
                instructions, addresses=addresses, caller_start=start,
                definition=definition)
        ):
            raise ValueError(
                "HUD layout-array loop bridge requires the exact candidate "
                "caller definition and relocation mask"
            )
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
                "HUD layout-array loop bridge requires the exact registered "
                "HUD target and unique hud.cpp contribution"
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
                "HUD layout-array loop bridge requires the exact authored "
                "InitHudLayouts hud.cpp contribution identity"
            )

        relocation_specs = (
            *(
                (
                    (
                        fixed["base_constructor_call"] - start,
                        "??0HudUiContainer@@QAE@XZ",
                        IMAGE_REL_I386_REL32,
                        b"\xe8\x00\x00\x00\x00",
                    ),
                )
                if candidate_variant == "current"
                else ()
            ),
            (
                fixed["array_destructor_target"] - start,
                _cc_catalog.HUD_UI_MGR_PANEL_SIMPLE_DESTRUCTOR_SYMBOL,
                IMAGE_REL_I386_DIR32,
                b"\x68\x00\x00\x00\x00",
            ),
            (
                fixed["array_constructor_target"] - start,
                _cc_catalog.HUD_UI_MGR_PANEL_SIMPLE_DEFAULT_CONSTRUCTOR_CLOSURE_SYMBOL,
                IMAGE_REL_I386_DIR32,
                b"\x68\x00\x00\x00\x00",
            ),
            (
                fixed["array_constructor_call"] - start,
                _cc_catalog.HUD_UI_MGR_LAYOUT_ARRAY_EH_CONSTRUCTOR_SYMBOL,
                IMAGE_REL_I386_REL32,
                b"\xe8\x00\x00\x00\x00",
            ),
            (
                fixed["add_child_call"] - start,
                _cc_catalog.HUD_SHIELD_LAYOUT_ADD_CHILD_SYMBOL,
                IMAGE_REL_I386_REL32,
                b"\xe8\x00\x00\x00\x00",
            ),
        )
        all_external_functions = (
            definition.undefined_external_functions
            + definition.defined_external_functions
        )
        all_external_data = (
            definition.undefined_external_data
            + definition.defined_external_data
        )
        for instruction_offset, symbol, relocation_type, body in (
            relocation_specs
        ):
            relocation_offset = instruction_offset + 1
            instruction_end = instruction_offset + len(body)
            relocations = [
                relocation
                for relocation in definition.relocations
                if relocation.offset == relocation_offset
            ]
            if (
                definition.data[instruction_offset:instruction_end] != body
                or definition.relocation_mask[instruction_offset]
                or not all(
                    definition.relocation_mask[index]
                    for index in range(relocation_offset, instruction_end)
                )
                or len(relocations) != 1
                or relocations[0].type != relocation_type
                or relocations[0].symbol_name != symbol
                or struct.unpack_from(
                    "<I", definition.data, relocation_offset
                )[0]
                != 0
                or all_external_functions.count(symbol) != 1
            ):
                raise ValueError(
                    "HUD layout-array loop bridge requires one exact "
                    f"relocation-backed {symbol!r} reference"
                )
        if candidate_variant == "current":
            vtable_symbol = "??_7HudUiStringMenu@@6B@"
            vtable_instruction_offset = fixed["vtable_store"] - start
            vtable_relocation_offset = vtable_instruction_offset + 2
            vtable_relocations = [
                relocation
                for relocation in definition.relocations
                if relocation.offset == vtable_relocation_offset
            ]
            if (
                definition.data[
                    vtable_instruction_offset :
                    vtable_instruction_offset + 6
                ]
                != b"\xc7\x07\x00\x00\x00\x00"
                or any(
                    definition.relocation_mask[
                        vtable_instruction_offset :
                        vtable_relocation_offset
                    ]
                )
                or not all(
                    definition.relocation_mask[index]
                    for index in range(
                        vtable_relocation_offset,
                        vtable_relocation_offset + 4,
                    )
                )
                or len(vtable_relocations) != 1
                or vtable_relocations[0].type != IMAGE_REL_I386_DIR32
                or vtable_relocations[0].symbol_name != vtable_symbol
                or struct.unpack_from(
                    "<I",
                    definition.data,
                    vtable_relocation_offset,
                )[0]
                != 0
                or all_external_data.count(vtable_symbol) != 1
            ):
                raise ValueError(
                    "HUD layout-array loop bridge requires one exact current "
                    "HudUiStringMenu vtable-store relocation"
                )

        unique_construction_symbols = {
            _cc_catalog.HUD_UI_MGR_PANEL_SIMPLE_DESTRUCTOR_SYMBOL,
            _cc_catalog.HUD_UI_MGR_PANEL_SIMPLE_DEFAULT_CONSTRUCTOR_CLOSURE_SYMBOL,
            _cc_catalog.HUD_UI_MGR_LAYOUT_ARRAY_EH_CONSTRUCTOR_SYMBOL,
        }
        array_topology_end = fixed["null_base"] - start
        direct_constructor_relocations = [
            relocation
            for relocation in definition.relocations
            if relocation.symbol_name.startswith(
                _cc_catalog.HUD_UI_MGR_PANEL_SIMPLE_CONSTRUCTOR_PREFIX
            )
        ]
        for relocation in direct_constructor_relocations:
            instruction_offset = relocation.offset - 1
            instruction_address = start + instruction_offset
            instruction = instruction_by_address.get(instruction_address)
            same_offset = [
                row
                for row in definition.relocations
                if row.offset == relocation.offset
            ]
            if (
                instruction_offset <= array_topology_end
                or relocation.type != IMAGE_REL_I386_REL32
                or len(same_offset) != 1
                or instruction is None
                or _cc_cfg._instruction_mnemonic(instruction) != "call"
                or _cc_cfg._instruction_operand(instruction)
                != relocation.symbol_name
                or bytes(
                    int(value, 16) for value in instruction.bytes
                )
                != b"\xe8\x00\x00\x00\x00"
                or definition.data[
                    instruction_offset : instruction_offset + 5
                ]
                != b"\xe8\x00\x00\x00\x00"
                or definition.relocation_mask[instruction_offset]
                or not all(
                    definition.relocation_mask[index]
                    for index in range(
                        relocation.offset,
                        relocation.offset + 4,
                    )
                )
                or struct.unpack_from(
                    "<I", definition.data, relocation.offset
                )[0]
                != 0
                or all_external_functions.count(
                    relocation.symbol_name
                )
                != 1
            ):
                raise ValueError(
                    "HUD layout-array loop bridge rejects a direct "
                    "HudUiPanelSimple constructor inside the reviewed array "
                    "topology or without one independent later exact call"
                )
        if (
            any(
                relocation.symbol_name in unique_construction_symbols
                and (
                    relocation.offset
                    != next(
                        spec[0] + 1
                        for spec in relocation_specs
                        if spec[1] == relocation.symbol_name
                    )
                    or relocation.type
                    != next(
                        spec[2]
                        for spec in relocation_specs
                        if spec[1] == relocation.symbol_name
                    )
                )
                for relocation in definition.relocations
            )
        ):
            raise ValueError(
                "HUD layout-array loop bridge rejects duplicate construction "
                "or duplicate/wrong reviewed helper and callback references"
            )

    return {
        normalize_address(hex(address)): bridge
        for address, bridge in bridge_addresses.items()
    }
