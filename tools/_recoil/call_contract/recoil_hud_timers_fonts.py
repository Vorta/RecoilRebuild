"""Recoil call-contract recoil hud timers fonts evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        IdentityIndexes,
    )

import struct
from dataclasses import replace
from pathlib import Path
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import (
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
