"""Recoil call-contract recoil hud shield evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        IdentityIndexes,
        ReviewedStaticStorageReferenceBridge,
    )

import re
import struct
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import IMAGE_REL_I386_DIR32, Instruction
from _recoil.commands.vc5_verify import load_manifest
from _recoil.lib.progress import (
    ProgressDocument,
    ProgressError,
    address_value,
    normalize_address,
)
from _recoil.lib.tooling import REPO_ROOT


def _hud_shield_layout_affine_storage_bridges(
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
    dict[str, str],
    dict[str, ReviewedStaticStorageReferenceBridge],
]:
    """Bridge one reviewed aggregate-interior pointer on both call-contract sides.

    Retail has no independently catalogued leaf at 0x4e6dac.  The identity is
    therefore an affine pointer field within the synchronized g_HudUiMgr
    aggregate, never a fabricated tracker symbol.  Every predicate is scoped
    to the exact 0x40eb00 caller and validates retail and candidate evidence
    independently before publishing either side of the bridge.
    """
    from _recoil.call_contract.records import ReviewedStaticStorageReferenceBridge
    if normalize_address(caller_start) != _cc_catalog.HUD_SHIELD_LAYOUT_CALLER_START:
        return {}, {}

    caller = candidate.caller_definition
    caller_addresses = [
        address
        for address, identity in indexes.by_address.items()
        if identity == caller_identity
    ]
    if (
        caller_identity != _cc_catalog.HUD_SHIELD_LAYOUT_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_SHIELD_LAYOUT_CALLER_END_EXCLUSIVE
        or caller_addresses != [_cc_catalog.HUD_SHIELD_LAYOUT_CALLER_START]
        or caller_identity in indexes.provider_ids
        or caller is None
        or caller.symbol != _cc_catalog.HUD_SHIELD_LAYOUT_CALLER_SYMBOL
        or not caller.data
        or len(caller.data) != len(caller.relocation_mask)
    ):
        raise ValueError(
            "HUD shield-layout affine-storage bridge requires the exact "
            "authored 0x40eb00 caller identity, extent, symbol, and object body"
        )

    target = document.collection("verification_targets").get(
        _cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_ID
    )
    registration = (
        target.get("registration")
        if isinstance(target, Mapping)
        else None
    )
    registrations: list[str] = []
    for target_id, row in document.collection(
        "verification_targets"
    ).items():
        candidate_registration = (
            row.get("registration") if isinstance(row, Mapping) else None
        )
        if not isinstance(candidate_registration, Mapping):
            continue
        try:
            registered_addresses = {
                normalize_address(str(raw_address))
                for raw_address in candidate_registration.get(
                    "data_addresses", []
                )
                if isinstance(raw_address, str)
            }
        except ProgressError:
            continue
        if _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS in registered_addresses:
            registrations.append(str(target_id))
    if (
        not isinstance(target, Mapping)
        or target.get("binary") != "recoil"
        or target.get("kind") != "vc5"
        or target.get("name") != "hud_ui_mgr_data"
        or target.get("symbol_ids") != [_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID]
        or target.get("unresolved_addresses") not in (None, [])
        or not isinstance(registration, Mapping)
        or registration.get("binary") != "recoil"
        or registration.get("name") != "hud_ui_mgr_data"
        or registration.get("manifest_path")
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_MANIFEST
        or registration.get("data_addresses")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS]
        or registrations != [_cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_ID]
    ):
        raise ValueError(
            "HUD shield-layout affine-storage bridge requires one exact "
            "registered aggregate target without registry collisions"
        )

    from _recoil.lib.verification_targets import vc5_target_registration

    manifest_path = (
        REPO_ROOT / _cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_MANIFEST
    ).resolve()
    manifest_root = (REPO_ROOT / "tools" / "vc5_verify_targets").resolve()
    try:
        manifest_path.relative_to(manifest_root)
        current_id, current_target = vc5_target_registration(manifest_path)
        manifest = load_manifest(manifest_path, enforce_source_policy=False)
    except (OSError, ProgressError, ValueError) as exc:
        raise ValueError(
            "HUD shield-layout affine-storage bridge cannot read the "
            "synchronized aggregate target"
        ) from exc
    current_registration = current_target.get("registration")
    manifest_rows = tuple(getattr(manifest, "data_symbols", ()))
    if (
        current_id != _cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_ID
        or current_target.get("binary") != "recoil"
        or current_target.get("kind") != "vc5"
        or current_target.get("name") != "hud_ui_mgr_data"
        or not isinstance(current_registration, Mapping)
        or current_registration.get("manifest_path")
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_MANIFEST
        or current_registration.get("data_addresses")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS]
        or len(manifest_rows) != 1
        or getattr(manifest_rows[0], "address", None)
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
        or getattr(manifest_rows[0], "symbol", None)
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        or getattr(manifest_rows[0], "symbol_regex", None) is not None
        or getattr(manifest_rows[0], "name", None)
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_NAME
        or getattr(manifest_rows[0], "bn_name", None)
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_NAME
        or getattr(manifest_rows[0], "byte_length", None)
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
    ):
        raise ValueError(
            "HUD shield-layout affine-storage bridge requires the synchronized "
            "exact decorated aggregate symbol and 0x7844 extent"
        )

    symbols = document.collection("symbols")
    storages = document.collection("storage_contributions")
    owners = document.collection("owners")
    symbol_rows = [
        (str(symbol_id), row)
        for symbol_id, row in symbols.items()
        if isinstance(row, Mapping)
        and isinstance(row.get("address", row.get("start")), str)
        and normalize_address(
            str(row.get("address", row.get("start")))
        )
        == _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
    ]
    storage_rows = [
        (str(storage_id), row)
        for storage_id, row in storages.items()
        if isinstance(row, Mapping)
        and isinstance(row.get("reference"), Mapping)
        and isinstance(row["reference"].get("address"), str)
        and normalize_address(str(row["reference"]["address"]))
        == _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
    ]
    if (
        len(symbol_rows) != 1
        or symbol_rows[0][0] != _cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID
        or len(storage_rows) != 1
        or storage_rows[0][0] != _cc_catalog.HUD_UI_MGR_AGGREGATE_STORAGE_ID
    ):
        raise ValueError(
            "HUD shield-layout affine-storage bridge requires unique aggregate "
            "symbol/storage authority"
        )
    aggregate_symbol = symbol_rows[0][1]
    aggregate_storage = storage_rows[0][1]
    reference = aggregate_storage.get("reference")
    if (
        aggregate_symbol.get("binary") != "recoil"
        or aggregate_symbol.get("kind") != "data"
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
        or aggregate_symbol.get("size") is not None
        or aggregate_symbol.get("end_exclusive") is not None
        or aggregate_symbol.get("logical_aliases")
        or aggregate_symbol.get("relocation_target_binding")
        or aggregate_storage.get("binary") != "recoil"
        or aggregate_storage.get("kind") != "data-symbol"
        or aggregate_storage.get("output_section_id")
        != "recoil:section:.data"
        or aggregate_storage.get("overlap") != "none"
        or aggregate_storage.get("owner_ids") != [_cc_catalog.HUD_UI_MGR_OWNER_ID]
        or aggregate_storage.get("parent_contribution_id") is not None
        or aggregate_storage.get("symbol_ids")
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID]
        or aggregate_storage.get("logical_aliases")
        or not isinstance(reference, Mapping)
        or reference.get("extent_state") != "unknown"
        or reference.get("size") is not None
        or reference.get("end_exclusive") is not None
    ):
        raise ValueError(
            "HUD shield-layout affine-storage bridge requires exact authored "
            "aggregate classification and storage linkage"
        )

    owner = owners.get(_cc_catalog.HUD_UI_MGR_OWNER_ID)
    gates = owner.get("gates") if isinstance(owner, Mapping) else None
    relationships = [
        (str(owner_id), relationship)
        for owner_id, row in owners.items()
        if isinstance(row, Mapping)
        for relationship in row.get("relationships", [])
        if isinstance(relationship, Mapping)
        and relationship.get("kind") == "primary-data"
        and relationship.get("symbol_id")
        == _cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID
    ]
    if (
        not isinstance(owner, Mapping)
        or owner.get("binary") != "recoil"
        or owner.get("kind") != "data-owner"
        or owner.get("provider_state") == "accepted"
        or not isinstance(gates, Mapping)
        or any(
            gates.get(gate) != "accepted"
            for gate in ("boundary", "source", "data", "owner_linkage")
        )
        or len(relationships) != 1
        or relationships[0][0] != _cc_catalog.HUD_UI_MGR_OWNER_ID
        or relationships[0][1].get("address")
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
        or relationships[0][1].get("name")
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_NAME
    ):
        raise ValueError(
            "HUD shield-layout affine-storage bridge requires one accepted "
            "non-provider aggregate owner relationship"
        )

    aggregate_start = address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
    pointer_start = address_value(_cc_catalog.HUD_SHIELD_LAYOUT_POINTER_ADDRESS)
    if (
        aggregate_start + _cc_catalog.HUD_SHIELD_LAYOUT_POINTER_DISPLACEMENT
        != pointer_start
        or _cc_catalog.HUD_SHIELD_LAYOUT_POINTER_DISPLACEMENT
        + _cc_catalog.HUD_SHIELD_LAYOUT_POINTER_ACCESS_WIDTH
        > _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
        or _cc_catalog.HUD_SHIELD_LAYOUT_POINTER_ADDRESS in indexes.storage_by_address
        or _cc_catalog.HUD_SHIELD_LAYOUT_POINTER_IDENTITY in indexes.provider_ids
    ):
        raise ValueError(
            "HUD shield-layout pointer is not one uncatalogued bounded "
            "aggregate-interior 4-byte field"
        )

    decorated_folded = _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL.casefold()
    callable_collisions = [
        name
        for name in indexes.by_candidate_name
        if name.casefold() == decorated_folded
    ]
    storage_collisions = [
        (name, identity)
        for name, identity in indexes.storage_by_name.items()
        if name.casefold() == decorated_folded
        and identity
        not in {"", f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"}
    ]
    retail_name_addresses = {
        normalize_address(str(getattr(row, "address", "")))
        for name, rows in bridge_names.items()
        if name.casefold() == decorated_folded
        for row in (
            list(rows)
            if isinstance(rows, (list, tuple, set, frozenset))
            else [rows]
        )
    }
    if (
        callable_collisions
        or storage_collisions
        or (
            retail_name_addresses
            and retail_name_addresses != {_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS}
        )
    ):
        raise ValueError(
            "HUD shield-layout affine-storage bridge has a callable/storage/"
            "provider/registry identity collision"
        )

    retail_offsets = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(_cc_catalog.HUD_SHIELD_LAYOUT_CALLER_START),
    )
    retail_by_address = {
        normalize_address(hex(offset)): instruction
        for instruction, offset in zip(retail_instructions, retail_offsets)
        if offset is not None
    }
    absolute_references = {
        address: instruction
        for address, instruction in retail_by_address.items()
        if _cc_catalog.HUD_SHIELD_LAYOUT_POINTER_ADDRESS[2:]
        in instruction.raw_text.casefold()
    }
    if set(absolute_references) != set(_cc_catalog.HUD_SHIELD_LAYOUT_RETAIL_LOADS):
        raise ValueError(
            "HUD shield-layout affine-storage bridge requires the exact "
            "fourteen immutable retail pointer-load sites"
        )
    for address, expected_bytes in _cc_catalog.HUD_SHIELD_LAYOUT_RETAIL_LOADS.items():
        instruction = absolute_references[address]
        if (
            _cc_cfg._instruction_mnemonic(instruction) != "mov"
            or tuple(item.lower() for item in instruction.bytes)
            != expected_bytes
        ):
            raise ValueError(
                "HUD shield-layout affine-storage bridge has a mismatched "
                f"retail pointer load at {address}"
            )
    primary_chain = {
        "0x40eb41": ("8b", "0d", "ac", "6d", "4e", "00"),
        "0x40eb4b": ("83", "c1", "1c"),
        "0x40eb4e": ("8b", "11"),
        "0x40eb50": ("ff", "52", "68"),
    }
    if any(
        address not in retail_by_address
        or tuple(
            item.lower() for item in retail_by_address[address].bytes
        )
        != expected_bytes
        for address, expected_bytes in primary_chain.items()
    ):
        raise ValueError(
            "HUD shield-layout affine-storage bridge requires the exact retail "
            "load/add/vptr/slot-0x68 provenance chain"
        )

    expected_expression = (
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
        f"+{_cc_catalog.HUD_SHIELD_LAYOUT_POINTER_DISPLACEMENT}"
    )
    candidate_offsets = _cc_cfg._instruction_runtime_addresses(
        candidate.instructions,
        source="cod",
        caller_start=0,
    )
    candidate_references = [
        (index, instruction, offset)
        for index, (instruction, offset) in enumerate(
            zip(candidate.instructions, candidate_offsets)
        )
        if "g_huduimgr" in instruction.raw_text.casefold()
    ]
    expected_loads = {
        (
            "edx",
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}+804",
        ): (b"\x8b\x15", 2, 0x324),
        (
            "edi",
            expected_expression,
        ): (b"\x8b\x3d", 2, _cc_catalog.HUD_SHIELD_LAYOUT_POINTER_DISPLACEMENT),
        (
            "eax",
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}+800",
        ): (b"\xa1", 1, 0x320),
    }
    derived_relocation_sites: dict[int, int] = {}
    matched_loads: dict[
        tuple[str, str],
        tuple[Instruction, int],
    ] = {}
    address_load_count = 0
    for index, instruction, offset in candidate_references:
        if offset is None:
            raise ValueError(
                "HUD shield-layout affine-storage bridge cannot derive a "
                "candidate aggregate relocation from an unresolved COD offset"
            )
        try:
            encoded = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError) as exc:
            raise ValueError(
                "HUD shield-layout affine-storage bridge requires exact "
                "candidate aggregate instruction bytes"
            ) from exc
        load_match = re.fullmatch(
            r"mov\s+(?P<register>edx|edi|eax)\s*,\s*(?P<source>.+)",
            instruction.raw_text.strip(),
            flags=re.IGNORECASE,
        )
        if load_match is not None:
            key = (
                load_match.group("register").lower(),
                _cc_targets._exact_memory_expression(load_match.group("source")),
            )
            specification = expected_loads.get(key)
            if (
                specification is None
                or key in matched_loads
                or not encoded.startswith(specification[0])
                or len(encoded) != specification[1] + 4
            ):
                raise ValueError(
                    "HUD shield-layout affine-storage bridge has a duplicate "
                    "or mismatched candidate aggregate load"
                )
            immediate_offset = specification[1]
            relocation_offset = offset + immediate_offset
            matched_loads[key] = (instruction, offset)
            derived_relocation_sites[relocation_offset] = specification[2]
            continue
        address_match = re.fullmatch(
            r"mov\s+ecx\s*,\s*OFFSET\s+FLAT:"
            r"(?P<symbol>\?g_HudUiMgr@@3THudUiMgrDataStorage@@A)",
            instruction.raw_text.strip(),
            flags=re.IGNORECASE,
        )
        if (
            address_match is None
            or address_match.group("symbol")
            != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
            or encoded != b"\xb9\x00\x00\x00\x00"
            or index + 1 >= len(candidate.instructions)
            or not re.fullmatch(
                r"call\s+"
                + re.escape(_cc_catalog.HUD_SHIELD_LAYOUT_ADD_CHILD_SYMBOL),
                candidate.instructions[index + 1].raw_text.strip(),
                flags=re.IGNORECASE,
            )
            or tuple(
                item.lower()
                for item in candidate.instructions[index + 1].bytes[:1]
            )
            != ("e8",)
        ):
            raise ValueError(
                "HUD shield-layout affine-storage bridge has an unmatched "
                "candidate aggregate address load"
            )
        address_load_count += 1
        derived_relocation_sites[offset + 1] = 0
    pointer_instruction = matched_loads.get(
        ("edi", expected_expression)
    )
    if (
        len(candidate_references) != 6
        or set(matched_loads) != set(expected_loads)
        or address_load_count != 3
        or len(derived_relocation_sites) != 6
        or pointer_instruction is None
    ):
        raise ValueError(
            "HUD shield-layout affine-storage bridge requires exactly three "
            "precise aggregate loads and three AddChild address loads"
        )
    expected_relocations = [
        relocation
        for relocation in caller.relocations
        if relocation.symbol_name.casefold() == decorated_folded
    ]
    undefined_aggregate_symbols = [
        name
        for name in caller.undefined_external_data
        if name.casefold() == decorated_folded
    ]
    defined_aggregate_symbols = [
        name
        for name in caller.defined_external_data
        if name.casefold() == decorated_folded
    ]
    actual_relocations = {
        relocation.offset: relocation
        for relocation in expected_relocations
    }
    if (
        len(expected_relocations) != 6
        or len(actual_relocations) != len(expected_relocations)
        or set(actual_relocations)
        != set(derived_relocation_sites)
        or undefined_aggregate_symbols
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL]
        or defined_aggregate_symbols
    ):
        raise ValueError(
            "HUD shield-layout affine-storage bridge requires the exact "
            "candidate pointer expression, body, and aggregate relocation set"
        )
    for offset, addend in derived_relocation_sites.items():
        relocation = actual_relocations[offset]
        owning_references = [
            (instruction, instruction_offset)
            for _index, instruction, instruction_offset
            in candidate_references
            if (
                instruction_offset is not None
                and instruction_offset <= offset
                < instruction_offset + len(instruction.bytes)
            )
        ]
        if len(owning_references) != 1:
            raise ValueError(
                "HUD shield-layout affine-storage bridge cannot uniquely "
                f"associate candidate relocation caller+0x{offset:x}"
            )
        instruction, instruction_offset = owning_references[0]
        instruction_end = instruction_offset + len(instruction.bytes)
        if (
            relocation.symbol_name != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
            or relocation.type != IMAGE_REL_I386_DIR32
            or offset + 4 > len(caller.data)
            or struct.unpack_from("<I", caller.data, offset)[0] != addend
            or not all(
                caller.relocation_mask[index]
                for index in range(offset, offset + 4)
            )
            or caller.data[instruction_offset:instruction_end]
            != bytes(int(item, 16) for item in instruction.bytes)
            or any(
                caller.relocation_mask[index]
                for index in range(instruction_offset, offset)
            )
        ):
            raise ValueError(
                "HUD shield-layout affine-storage bridge has a mismatched "
                f"candidate DIR32 aggregate relocation at caller+0x{offset:x}"
            )

    return (
        {
            _cc_catalog.HUD_SHIELD_LAYOUT_POINTER_ADDRESS: (
                _cc_catalog.HUD_SHIELD_LAYOUT_POINTER_IDENTITY
            )
        },
        {
            expected_expression: ReviewedStaticStorageReferenceBridge(
                aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                displacement=_cc_catalog.HUD_SHIELD_LAYOUT_POINTER_DISPLACEMENT,
                access_width=_cc_catalog.HUD_SHIELD_LAYOUT_POINTER_ACCESS_WIDTH,
                storage_identity=_cc_catalog.HUD_SHIELD_LAYOUT_POINTER_IDENTITY,
            )
        },
    )
