"""Recoil call-contract recoil hud fonts evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import identity as _cc_identity
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
    Instruction,
)
from _recoil.lib.progress import ProgressDocument, address_value, normalize_address
from _recoil.lib.tooling import REPO_ROOT


def _hud_ui_mgr_ensure_font_loop_retail_vptr_bridges(
    instructions: Sequence[Instruction],
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedLoopVptrStorageBridge]:
    """Bind only EnsureHudLoaded's exact retail string-menu font loop."""
    from _recoil.call_contract.records import (
        ReviewedLoopVptrStorageBridge,
        StorageContainer,
    )
    caller_start = normalize_address(caller_start)
    if caller_start != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START:
        return {}
    if (
        caller_identity != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_END_EXCLUSIVE
        or sorted(
            address
            for address, identity in indexes.by_address.items()
            if identity == caller_identity
        )
        != [_cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START]
        or caller_identity in indexes.provider_ids
    ):
        raise ValueError(
            "HUD EnsureHudLoaded font-loop bridge requires the exact "
            "reviewed authored caller identity and extent"
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
            "HUD EnsureHudLoaded font-loop bridge requires one exact "
            "unaliased authored caller and resolved source edge"
        )

    target_id = "recoil:vc5-target:hud_404ca0_415ab0_authored_order"
    target = document.collection("verification_targets").get(target_id)
    registration = (
        target.get("registration") if isinstance(target, Mapping) else None
    )
    contribution_rows: list[
        tuple[Mapping[str, Any], Mapping[str, Any]]
    ] = []
    if isinstance(registration, Mapping):
        for contribution in registration.get(
            "translation_unit_function_order",
            [],
        ):
            if not isinstance(contribution, Mapping):
                continue
            for row in contribution.get("functions", []):
                if (
                    isinstance(row, Mapping)
                    and normalize_address(str(row.get("address", "")))
                    == _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START
                ):
                    contribution_rows.append((contribution, row))
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
        or len(contribution_rows) != 1
    ):
        raise ValueError(
            "HUD EnsureHudLoaded font-loop bridge requires one exact "
            "current HUD verification-target authority"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        contribution.get("source_from")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or contribution.get("order_scope") != "authored"
        or contribution_row.get("symbol") != ""
        or contribution_row.get("symbol_regex")
        != r"\?EnsureHudLoaded@HudUiMgr@@.*"
        or contribution_row.get("name") != "HudUiMgr::EnsureHudLoaded"
        or contribution_row.get("pipeline_class") != "authored"
        or contribution_row.get("authored_order_role") != "authored-body"
        or contribution_row.get("required_presence") is not True
        or contribution_row.get("full_order_gate") is not True
        or target_id not in caller_symbol.get(
            "verification_target_ids",
            [],
        )
    ):
        raise ValueError(
            "HUD EnsureHudLoaded font-loop bridge requires the exact "
            "authored hud.cpp contribution row"
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
        or address_value(_cc_catalog.HUD_UI_MGR_ENSURE_MENU_POINTER_ADDRESS)
        != address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
        + _cc_catalog.HUD_UI_MGR_ENSURE_MENU_POINTER_DISPLACEMENT
    ):
        raise ValueError(
            "HUD EnsureHudLoaded font-loop bridge requires the exact "
            "reviewed HUD aggregate storage container and +0xee0 field"
        )

    start = address_value(_cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START)
    instruction_addresses = _cc_cfg._instruction_runtime_addresses(
        instructions,
        source="bn",
        caller_start=start,
    )
    address_counts: dict[int, int] = {}
    for address in instruction_addresses:
        if address is not None:
            address_counts[address] = address_counts.get(address, 0) + 1
    instruction_by_address = {
        address: instruction
        for address, instruction in zip(instruction_addresses, instructions)
        if address is not None and address_counts.get(address) == 1
    }
    index_by_address = {
        address: index
        for index, address in enumerate(instruction_addresses)
        if address is not None and address_counts.get(address) == 1
    }
    fixed_rows = {
        0x410255: ("mov", ("8b", "35", "b0", "6d", "4e", "00")),
        0x41025B: ("mov", ("bb", "17", "00", "00", "00")),
        0x410260: ("lea", ("8d", "56", "1c")),
        0x410263: ("lea", ("8d", "46", "18")),
        0x410266: ("lea", ("8d", "4e", "10")),
        0x410269: ("lea", ("8d", "7e", "14")),
        0x41026C: ("mov", ("89", "55", "ec")),
        0x41026F: ("mov", ("89", "45", "f8")),
        0x410272: ("mov", ("89", "4d", "fc")),
        0x410275: ("add", ("83", "c6", "20")),
        0x410278: ("mov", ("8b", "07")),
        0x41027A: ("mov", ("8b", "4d", "ec")),
        0x41027D: ("push", ("6a", "02")),
        0x41027F: ("push", ("6a", "00")),
        0x410281: ("push", ("6a", "00")),
        0x410283: ("push", ("50",)),
        0x410284: ("mov", ("8b", "01")),
        0x410286: ("mov", ("8b", "4d", "f8")),
        0x410289: ("mov", ("8b", "16")),
        0x41028B: ("push", ("50",)),
        0x41028C: ("mov", ("8b", "01")),
        0x41028E: ("mov", ("8b", "4d", "fc")),
        0x410291: ("push", ("50",)),
        0x410292: ("mov", ("8b", "01")),
        0x410294: ("mov", ("8b", "ce")),
        0x410296: ("push", ("50",)),
        0x410297: (
            "call",
            ("ff", "92", "80", "00", "00", "00"),
        ),
        0x41029D: (
            "add",
            ("81", "c6", "a4", "02", "00", "00"),
        ),
        0x4102A3: ("dec", ("4b",)),
        0x4102A4: ("jne", ("75", "d2")),
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
            "HUD EnsureHudLoaded font-loop bridge requires the exact "
            "retail +0xee0/0x17/+0x20/0x2a4 EDX-vptr loop body"
        )
    fixed_indices = [index_by_address[address] for address in fixed_rows]
    if fixed_indices != sorted(fixed_indices) or len(set(fixed_indices)) != len(
        fixed_indices
    ):
        raise ValueError(
            "HUD EnsureHudLoaded font-loop bridge rejects reordered or "
            "aliased retail instructions"
        )

    call_address = address_value(_cc_catalog.HUD_UI_MGR_ENSURE_FONT_LOOP_CALL_ADDRESS)
    call_index = index_by_address[call_address]
    bounded_calls = [
        address
        for address, instruction in zip(
            instruction_addresses,
            instructions,
        )
        if (
            address is not None
            and 0x410255 <= address <= 0x4102A4
            and _cc_cfg._instruction_mnemonic(instruction) == "call"
        )
    ]
    if (
        bounded_calls != [call_address]
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(instructions[call_index])
        )
        not in {"edx+128", "edx+0x80"}
        or sum(
            _cc_cfg._instruction_mnemonic(instruction) == "call"
            for instruction in instructions[:call_index]
        )
        != _cc_catalog.HUD_UI_MGR_ENSURE_FONT_LOOP_CALL_ORDINAL
        or any(
            _cc_cfg._instruction_mnemonic(instruction) == "jmp"
            for instruction in instructions[:call_index]
        )
        or _cc_cfg._cleanup_after(instructions, call_index) is not None
    ):
        raise ValueError(
            "HUD EnsureHudLoaded font-loop bridge requires the exact "
            "ordinal-13 EDX slot-0x80 callee-cleanup invocation"
        )

    return {
        _cc_catalog.HUD_UI_MGR_ENSURE_FONT_LOOP_CALL_ADDRESS:
        ReviewedLoopVptrStorageBridge(
            register="edx",
            storage_identity=(
                _cc_catalog.HUD_UI_MGR_ENSURE_FONT_LOOP_STORAGE_IDENTITY
            ),
            slot_displacement=(
                _cc_catalog.HUD_UI_MGR_ENSURE_FONT_LOOP_SLOT_DISPLACEMENT
            ),
            assembly_source="bn",
        )
    }


def _hud_ui_mgr_ensure_font_loop_candidate_vptr_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedLoopVptrStorageBridge]:
    """Bind only EnsureHudLoaded's exact candidate string-menu font loop."""
    from _recoil.call_contract.records import (
        ReviewedLoopVptrStorageBridge,
        StorageContainer,
    )
    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START:
        return {}
    if (
        caller_identity != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_END_EXCLUSIVE
        or sorted(
            address
            for address, identity in indexes.by_address.items()
            if identity == caller_identity
        )
        != [_cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START]
        or caller_identity in indexes.provider_ids
    ):
        raise ValueError(
            "HUD EnsureHudLoaded font-loop candidate bridge requires the "
            "exact reviewed authored caller identity and extent"
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
    target_id = "recoil:vc5-target:hud_404ca0_415ab0_authored_order"
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
        or target_id not in caller_symbol.get("verification_target_ids", [])
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
            "HUD EnsureHudLoaded font-loop candidate bridge requires one "
            "exact unaliased authored caller and resolved source edge"
        )

    expected_rows = [
        row
        for row in expected
        if row.get("ordinal") == _cc_catalog.HUD_UI_MGR_ENSURE_FONT_LOOP_CALL_ORDINAL
    ]
    if (
        len(expected_rows) != 1
        or any(
            expected_rows[0].get(key) != value
            for key, value in {
                "form": "call",
                "dispatch": "indirect",
                "identity_kind": "virtual-slot",
                "target_identity": "",
                "storage_identity": (
                    _cc_catalog.HUD_UI_MGR_ENSURE_FONT_LOOP_STORAGE_IDENTITY
                ),
                "slot_displacement": (
                    _cc_catalog.HUD_UI_MGR_ENSURE_FONT_LOOP_SLOT_DISPLACEMENT
                ),
                "cleanup_bytes": None,
            }.items()
        )
    ):
        raise ValueError(
            "HUD EnsureHudLoaded font-loop candidate bridge requires one-to-"
            "one agreement with the exact reviewed retail ordinal-13 storage "
            "identity"
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
            "HUD EnsureHudLoaded font-loop candidate bridge requires the "
            "nonempty candidate caller and registered HUD target"
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
            "HUD EnsureHudLoaded font-loop candidate bridge requires the "
            "exact authored EnsureHudLoaded hud.cpp contribution identity"
        )

    aggregate_identity = f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
    if (
        indexes.storage_by_address.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
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
        or definition.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in definition.defined_external_data
    ):
        raise ValueError(
            "HUD EnsureHudLoaded font-loop candidate bridge requires the "
            "exact typed HUD aggregate storage identity"
        )

    start = 0
    end = address_value(caller_end_exclusive)
    fixed = {
        "manager_font_load": (
            start + _cc_catalog.HUD_UI_MGR_ENSURE_FONT_LOOP_CANDIDATE_LOAD_OFFSET
        ),
        "font_seed": start + 0x13A,
        "font_receiver": start + 0x13D,
        "font_argument": start + 0x13F,
        "read_rect": start + 0x141,
        "induction_seed": start + 0x146,
        "header": (
            start + _cc_catalog.HUD_UI_MGR_ENSURE_FONT_LOOP_CANDIDATE_LOOP_OFFSET
        ),
        "manager_loop_load": (
            start + _cc_catalog.HUD_UI_MGR_ENSURE_FONT_LOOP_CANDIDATE_LOOP_OFFSET
        ),
        "push_two": start + 0x14E,
        "push_zero_first": start + 0x150,
        "push_zero_second": start + 0x151,
        "receiver_seed": start + 0x152,
        "width_load": start + 0x156,
        "width_push": start + 0x159,
        "weight_load": start + 0x15A,
        "vptr_load": start + 0x15D,
        "weight_push": start + 0x15F,
        "height_load": start + 0x160,
        "height_push": start + 0x163,
        "face_load": start + 0x164,
        "face_push": start + 0x166,
        "call": start + _cc_catalog.HUD_UI_MGR_ENSURE_FONT_LOOP_CANDIDATE_CALL_OFFSET,
        "stride": start + 0x16D,
        "bound": start + 0x173,
        "backedge": start + 0x179,
    }
    baseline_fixed = fixed
    baseline_expected_instruction_rows = {
        baseline_fixed["manager_font_load"]: (
            ("8b", "0d", "e0", "0e", "00", "00"),
            rf"mov\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            rf"{re.escape(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)}"
            r"\+(?:3808|0xee0)",
        ),
        baseline_fixed["font_seed"]: (
            ("8d", "71", "10"),
            r"lea\s+esi\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[ecx(?:\+16|\+0x10)\]",
        ),
        baseline_fixed["font_receiver"]: (("8b", "c8"), r"mov\s+ecx\s*,\s*eax"),
        baseline_fixed["font_argument"]: (("8b", "d6"), r"mov\s+edx\s*,\s*esi"),
        baseline_fixed["read_rect"]: (
            ("e8", "00", "00", "00", "00"),
            rf"call\s+{re.escape(
                _cc_catalog.HUD_UI_MGR_ENSURE_FONT_LOOP_READ_RECT_SYMBOL
            )}(?:\s*;.*)?",
        ),
        baseline_fixed["induction_seed"]: (("33", "ff"), r"xor\s+edi\s*,\s*edi"),
        baseline_fixed["manager_loop_load"]: (
            ("8b", "15", "e0", "0e", "00", "00"),
            rf"mov\s+edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            rf"{re.escape(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)}"
            r"\+(?:3808|0xee0)",
        ),
        baseline_fixed["push_two"]: (("6a", "02"), r"push\s+(?:2|0x2)"),
        baseline_fixed["push_zero_first"]: (("53",), r"push\s+ebx"),
        baseline_fixed["push_zero_second"]: (("53",), r"push\s+ebx"),
        baseline_fixed["receiver_seed"]: (
            ("8d", "4c", "3a", "20"),
            r"lea\s+ecx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[edx\+edi(?:\+32|\+0x20)\]",
        ),
        baseline_fixed["width_load"]: (
            ("8b", "56", "0c"),
            r"mov\s+edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[esi(?:\+12|\+0xc)\]",
        ),
        baseline_fixed["width_push"]: (("52",), r"push\s+edx"),
        baseline_fixed["weight_load"]: (
            ("8b", "56", "08"),
            r"mov\s+edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[esi(?:\+8|\+0x8)\]",
        ),
        baseline_fixed["vptr_load"]: (
            ("8b", "01"),
            r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[ecx\]",
        ),
        baseline_fixed["weight_push"]: (("52",), r"push\s+edx"),
        baseline_fixed["height_load"]: (
            ("8b", "56", "04"),
            r"mov\s+edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
            r"\[esi(?:\+4|\+0x4)\]",
        ),
        baseline_fixed["height_push"]: (("52",), r"push\s+edx"),
        baseline_fixed["face_load"]: (
            ("8b", "16"),
            r"mov\s+edx\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[esi\]",
        ),
        baseline_fixed["face_push"]: (("52",), r"push\s+edx"),
        baseline_fixed["call"]: (
            ("ff", "90", "80", "00", "00", "00"),
            r"call\s+(?:dword\s+(?:ptr\s+)?)?"
            r"\[eax(?:\+128|\+0x80)\]",
        ),
        baseline_fixed["stride"]: (
            ("81", "c7", "a4", "02", "00", "00"),
            r"add\s+edi\s*,\s*(?:676|0x2a4)",
        ),
        baseline_fixed["bound"]: (
            ("81", "ff", "bc", "3c", "00", "00"),
            r"cmp\s+edi\s*,\s*(?:15548|0x3cbc)",
        ),
        baseline_fixed["backedge"]: (
            ("7c", "cd"),
            r"jl\s+(?:(?:short|near)\s+)?\$L[0-9A-Za-z_]+",
        ),
    }
    (
        unit_shift,
        addresses,
        instruction_by_address,
        index_by_address,
    ) = _cc_callable_identity._locate_shifted_candidate_instruction_unit(
        candidate,
        baseline_expected_instruction_rows,
        label="HUD EnsureHudLoaded font-loop candidate bridge",
    )
    fixed = {
        key: address + unit_shift
        for key, address in baseline_fixed.items()
    }
    expected_instruction_rows = {
        address + unit_shift: row
        for address, row in baseline_expected_instruction_rows.items()
    }
    for address, (body, pattern) in expected_instruction_rows.items():
        instruction = instruction_by_address[address]
        instruction_offset = address - start
        encoded = bytes(int(value, 16) for value in instruction.bytes)
        if (
            tuple(value.lower() for value in instruction.bytes) != body
            or re.fullmatch(
                pattern,
                instruction.raw_text.strip(),
                flags=re.IGNORECASE,
            )
            is None
            or definition.data[
                instruction_offset : instruction_offset + len(encoded)
            ]
            != encoded
        ):
            raise ValueError(
                "HUD EnsureHudLoaded font-loop candidate bridge requires "
                "exact instruction bytes and operands at "
                f"+{hex(instruction_offset)}"
            )
    ordered_addresses = [
        address for key, address in fixed.items() if key != "header"
    ]
    ordered_indices = [index_by_address[address] for address in ordered_addresses]
    if (
        ordered_addresses != sorted(ordered_addresses)
        or ordered_indices != sorted(ordered_indices)
        or len(set(ordered_indices)) != len(ordered_indices)
    ):
        raise ValueError(
            "HUD EnsureHudLoaded font-loop candidate bridge rejects reordered "
            "or aliased instructions"
        )

    expected_relocations = {
        _cc_catalog.HUD_UI_MGR_ENSURE_FONT_LOOP_CANDIDATE_LOAD_OFFSET + unit_shift + 2: (
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_ENSURE_MENU_POINTER_DISPLACEMENT,
        ),
        _cc_catalog.HUD_UI_MGR_ENSURE_FONT_LOOP_CANDIDATE_LOOP_OFFSET + unit_shift + 2: (
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_ENSURE_MENU_POINTER_DISPLACEMENT,
        ),
        0x142 + unit_shift: (
            _cc_catalog.HUD_UI_MGR_ENSURE_FONT_LOOP_READ_RECT_SYMBOL,
            IMAGE_REL_I386_REL32,
            0,
        ),
    }
    for relocation_offset, (symbol, relocation_type, addend) in (
        expected_relocations.items()
    ):
        relocation_rows = [
            row
            for row in definition.relocations
            if row.offset == relocation_offset
        ]
        if (
            len(relocation_rows) != 1
            or relocation_rows[0].symbol_name != symbol
            or relocation_rows[0].type != relocation_type
            or struct.unpack_from(
                "<I", definition.data, relocation_offset
            )[0]
            != addend
            or not all(
                definition.relocation_mask[index]
                for index in range(relocation_offset, relocation_offset + 4)
            )
        ):
            raise ValueError(
                "HUD EnsureHudLoaded font-loop candidate bridge requires "
                f"one exact relocation-backed {symbol!r} reference at "
                f"+{hex(relocation_offset)}"
            )
    bounded_aggregate_relocations = [
        row
        for row in definition.relocations
        if (
            _cc_catalog.HUD_UI_MGR_ENSURE_FONT_LOOP_CANDIDATE_LOAD_OFFSET + unit_shift
            <= row.offset
            < 0x17B
            and row.symbol_name == _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
    ]
    if [
        row.offset for row in bounded_aggregate_relocations
    ] != [
        _cc_catalog.HUD_UI_MGR_ENSURE_FONT_LOOP_CANDIDATE_LOAD_OFFSET + unit_shift + 2,
        _cc_catalog.HUD_UI_MGR_ENSURE_FONT_LOOP_CANDIDATE_LOOP_OFFSET + unit_shift + 2,
    ]:
        raise ValueError(
            "HUD EnsureHudLoaded font-loop candidate bridge rejects missing, "
            "extra, or reordered aggregate-field relocations"
        )
    if (
        definition.undefined_external_functions
        + definition.defined_external_functions
    ).count(_cc_catalog.HUD_UI_MGR_ENSURE_FONT_LOOP_READ_RECT_SYMBOL) != 1:
        raise ValueError(
            "HUD EnsureHudLoaded font-loop candidate bridge requires the "
            "exact candidate ReadRect function identity"
        )

    call_index = index_by_address[fixed["call"]]
    header_index = index_by_address[fixed["header"]]
    backedge_index = index_by_address[fixed["backedge"]]
    bounded_calls = [
        address
        for address, instruction in zip(addresses, candidate.instructions)
        if (
            address is not None
            and fixed["manager_font_load"] <= address <= fixed["backedge"]
            and _cc_cfg._instruction_mnemonic(instruction) == "call"
        )
    ]
    if (
        bounded_calls != [fixed["read_rect"], fixed["call"]]
        or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
        or any(
            definition.relocation_mask[
                _cc_catalog.HUD_UI_MGR_ENSURE_FONT_LOOP_CANDIDATE_CALL_OFFSET + unit_shift :
                _cc_catalog.HUD_UI_MGR_ENSURE_FONT_LOOP_CANDIDATE_CALL_OFFSET + unit_shift + 6
            ]
        )
    ):
        raise ValueError(
            "HUD EnsureHudLoaded font-loop candidate bridge requires the "
            "exact ordinal-13 EAX slot-0x80 callee-cleanup invocation"
        )
    backedge = _cc_cfg._exact_local_direct_branch(
        instruction_by_address[fixed["backedge"]],
        instruction_index=backedge_index,
        instruction_addresses=addresses,
        instruction_index_by_address=index_by_address,
        source="cod",
        caller_start=start,
        caller_end=end,
    )
    if backedge != ("conditional", header_index):
        raise ValueError(
            "HUD EnsureHudLoaded font-loop candidate bridge requires the "
            "exact +0x179 backedge to the +0x148 loop header"
        )


    for index in range(header_index, backedge_index + 1):
        instruction = candidate.instructions[index]
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
                "HUD EnsureHudLoaded font-loop candidate bridge rejects "
                "alternate CFG inside the reviewed loop"
            )
        if (
            _cc_instructions.may_clobber_register(instruction, "edi")
            and index != index_by_address[fixed["stride"]]
        ):
            raise ValueError(
                "HUD EnsureHudLoaded font-loop candidate bridge rejects "
                "induction-register clobber or alias drift"
            )
    vptr_index = index_by_address[fixed["vptr_load"]]
    receiver_index = index_by_address[fixed["receiver_seed"]]
    if (
        any(
            _cc_instructions.may_clobber_register(candidate.instructions[index], "eax")
            for index in range(vptr_index + 1, call_index)
        )
        or any(
            _cc_instructions.may_clobber_register(candidate.instructions[index], "ecx")
            for index in range(receiver_index + 1, call_index)
        )
    ):
        raise ValueError(
            "HUD EnsureHudLoaded font-loop candidate bridge rejects EAX-vptr "
            "or ECX-receiver clobber"
        )

    return {
        normalize_address(
            hex(_cc_catalog.HUD_UI_MGR_ENSURE_FONT_LOOP_CANDIDATE_CALL_OFFSET + unit_shift)
        ): ReviewedLoopVptrStorageBridge(
            register="eax",
            storage_identity=_cc_catalog.HUD_UI_MGR_ENSURE_FONT_LOOP_STORAGE_IDENTITY,
            slot_displacement=_cc_catalog.HUD_UI_MGR_ENSURE_FONT_LOOP_SLOT_DISPLACEMENT,
            assembly_source="cod",
        )
    }
