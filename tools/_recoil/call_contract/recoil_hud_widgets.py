"""Recoil call-contract recoil hud widgets evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import extraction as _cc_extraction
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        IdentityIndexes,
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedExactIndirectStorageBridge,
        ReviewedLoopVptrStorageBridge,
        ReviewedStaticStorageReferenceBridge,
        ReviewedVptrStorageBridge,
        StructuralPhysicalProviderSupplier,
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
from _recoil.commands.vc5_verify import load_manifest
from _recoil.lib.authored_icf import exact_required_target_membership
from _recoil.lib.binja import BinaryNinjaBridge
from _recoil.lib.progress import (
    ProgressDocument,
    ProgressError,
    address_value,
    normalize_address,
)
from _recoil.lib.tooling import REPO_ROOT


def _hud_cmd_binding_ptr_vector_erase_provider_supplier(
    document: ProgressDocument,
    *,
    by_address: Mapping[str, str],
    by_candidate_name: Mapping[str, str],
    provider_ids: frozenset[str],
    bridge: BinaryNinjaBridge | None,
    fact_transcript: list[dict[str, Any]] | None = None,
) -> StructuralPhysicalProviderSupplier | None:
    """Derive one exact provider from tracker plus same-invocation retail bytes."""
    from _recoil.call_contract.records import StructuralPhysicalProviderSupplier

    physical_rows: list[tuple[str, Mapping[str, Any]]] = []
    for symbol_id, symbol in document.collection("symbols").items():
        if not isinstance(symbol, Mapping):
            continue
        raw_address = symbol.get("address", symbol.get("start"))
        if not isinstance(raw_address, str):
            continue
        try:
            address = normalize_address(raw_address)
        except ProgressError:
            continue
        if address == _cc_catalog.HUD_CMD_BINDING_PTR_VECTOR_ERASE_ADDRESS:
            physical_rows.append((str(symbol_id), symbol))
    expected_symbol_id = "recoil:function:0x4ba4d0"
    if (
        len(physical_rows) != 1
        or physical_rows[0][0] != expected_symbol_id
    ):
        return None
    symbol = physical_rows[0][1]
    # The ordinary provider index validates the complete owner, evidence and
    # canonical-header registration before calling this physical-body rule.
    # Registration adds an object symbol and changes the row kind; it must
    # not make an independently identical pointer-vector COMDAT disappear.
    registered = (symbol.get("kind") == "provider-function"
        and isinstance(symbol.get("object_symbol"), str)
        and bool(symbol["object_symbol"])
        and by_candidate_name.get(symbol["object_symbol"])
            == _cc_catalog.HUD_CMD_BINDING_PTR_VECTOR_ERASE_IDENTITY)
    logical_aliases = symbol.get("logical_aliases")
    icf_group = symbol.get("icf_address_group")
    if (
        symbol.get("binary") != "recoil"
        or (symbol.get("kind") != "function" and not registered)
        or symbol.get("address")
        != _cc_catalog.HUD_CMD_BINDING_PTR_VECTOR_ERASE_ADDRESS
        or symbol.get("end_exclusive")
        != _cc_catalog.HUD_CMD_BINDING_PTR_VECTOR_ERASE_END_EXCLUSIVE
        or symbol.get("extent_state") != "known"
        or symbol.get("size") != 0x40
        or symbol.get("pipeline_class") != "non-authored"
        or symbol.get("authored_order_role") != "non-authored"
        or symbol.get("logical_identity_key") not in {None, ""}
        or symbol.get("icf_fold_status") not in {None, ""}
        or (symbol.get("object_symbol") not in {None, ""} and not registered)
        or logical_aliases not in (None, {})
        or icf_group not in (None, {})
        or by_address.get(_cc_catalog.HUD_CMD_BINDING_PTR_VECTOR_ERASE_ADDRESS)
        != _cc_catalog.HUD_CMD_BINDING_PTR_VECTOR_ERASE_IDENTITY
        or _cc_catalog.HUD_CMD_BINDING_PTR_VECTOR_ERASE_IDENTITY not in provider_ids
    ):
        return None
    if (
        address_value(_cc_catalog.HUD_CMD_BINDING_PTR_VECTOR_ERASE_END_EXCLUSIVE)
        - address_value(_cc_catalog.HUD_CMD_BINDING_PTR_VECTOR_ERASE_ADDRESS)
        != 0x40
    ):
        return None
    retail_body = _cc_identity._direct_bn_retail_bytes(
        bridge,
        address=_cc_catalog.HUD_CMD_BINDING_PTR_VECTOR_ERASE_ADDRESS,
        length=len(_cc_catalog.HUD_CMD_BINDING_PTR_VECTOR_ERASE_RETAIL_BODY),
    )
    if retail_body != _cc_catalog.HUD_CMD_BINDING_PTR_VECTOR_ERASE_RETAIL_BODY:
        return None
    if fact_transcript is not None:
        fact_transcript.append({
            "fact": "provider-pointer-vector-erase-body",
            "address": _cc_catalog.HUD_CMD_BINDING_PTR_VECTOR_ERASE_ADDRESS,
            "requested_length": len(_cc_catalog.HUD_CMD_BINDING_PTR_VECTOR_ERASE_RETAIL_BODY),
            "direct_bytes": list(retail_body),
            "parsed_cleanup_bytes": 8,
            "provider_identity": _cc_catalog.HUD_CMD_BINDING_PTR_VECTOR_ERASE_IDENTITY,
            "provider_role": "msvc-stl-vector-erase-range",
        })
    return StructuralPhysicalProviderSupplier(
        candidate_name=_cc_catalog.HUD_CMD_BINDING_PTR_VECTOR_ERASE_SYMBOL,
        address=_cc_catalog.HUD_CMD_BINDING_PTR_VECTOR_ERASE_ADDRESS,
        identity=_cc_catalog.HUD_CMD_BINDING_PTR_VECTOR_ERASE_IDENTITY,
        stack_cleanup_bytes=8,
    )


def _hud_ui_mgr_stats_list_static_storage_reference_bridges(
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    candidate_storage_bridges: Mapping[str, str],
    compiler_generated_bridges: Mapping[str, str],
) -> dict[str, ReviewedStaticStorageReferenceBridge]:
    """Resolve one exact aggregate-plus-field candidate storage reference.

    This is deliberately separate from registered decorated-data publication:
    it grants one candidate-side, call-contract-only static load after the
    synchronized aggregate manifest, tracker aggregate/leaf ownership, and
    current caller-object COD/COFF evidence all converge.  Lifecycle, byte,
    tier, and final-layout state are intentionally outside this predicate.
    """
    from _recoil.call_contract.records import ReviewedStaticStorageReferenceBridge
    def exact_mov_source(instruction: Instruction) -> str:
        match = re.fullmatch(
            r"mov\s+[a-z]{2,3}\s*,\s*(?P<source>.+)",
            instruction.raw_text.strip(),
            flags=re.IGNORECASE,
        )
        return (
            _cc_targets._exact_memory_expression(match.group("source"))
            if match is not None
            else ""
        )

    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_SCOREBOARD_DISPATCH_SET_SCALE_CALLER_START:
        return {}

    caller = candidate.caller_definition
    caller_addresses = [
        address
        for address, identity in indexes.by_address.items()
        if identity == caller_identity
    ]
    if (
        caller_identity
        != _cc_catalog.HUD_SCOREBOARD_DISPATCH_SET_SCALE_CALLER_IDENTITY
        or normalized_start
        != _cc_catalog.HUD_SCOREBOARD_DISPATCH_SET_SCALE_CALLER_START
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_SCOREBOARD_DISPATCH_SET_SCALE_CALLER_END_EXCLUSIVE
        or caller_addresses
        != [_cc_catalog.HUD_SCOREBOARD_DISPATCH_SET_SCALE_CALLER_START]
        or caller_identity in indexes.provider_ids
        or caller is None
        or caller.symbol
        != _cc_catalog.HUD_SCOREBOARD_DISPATCH_SET_SCALE_CALLER_SYMBOL
        or len(caller.data) != len(caller.relocation_mask)
    ):
        raise ValueError(
            "HUD stats-list static-storage bridge requires the exact reviewed "
            "authored 0x40eae0 caller identity, extent, symbol, and object body"
        )

    target = document.collection("verification_targets").get(
        _cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_ID
    )
    registration = (
        target.get("registration")
        if isinstance(target, Mapping)
        else None
    )
    aggregate_registrations: list[str] = []
    for target_id, candidate_target in document.collection(
        "verification_targets"
    ).items():
        if not isinstance(candidate_target, Mapping):
            continue
        candidate_registration = candidate_target.get("registration")
        if not isinstance(candidate_registration, Mapping):
            continue
        matching_addresses = []
        for raw_address in candidate_registration.get("data_addresses", []):
            if not isinstance(raw_address, str):
                continue
            try:
                if (
                    normalize_address(raw_address)
                    == _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
                ):
                    matching_addresses.append(raw_address)
            except ProgressError:
                continue
        if matching_addresses:
            aggregate_registrations.append(str(target_id))
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
        or aggregate_registrations
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_ID]
    ):
        raise ValueError(
            "HUD stats-list static-storage bridge requires one exact "
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
        manifest = load_manifest(
            manifest_path,
            enforce_source_policy=False,
        )
    except (OSError, ProgressError, ValueError) as exc:
        raise ValueError(
            "HUD stats-list static-storage bridge cannot read the synchronized "
            "aggregate target"
        ) from exc
    current_registration = current_target.get("registration")
    aggregate_rows = tuple(getattr(manifest, "data_symbols", ()))
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
        or len(aggregate_rows) != 1
        or getattr(aggregate_rows[0], "address", None)
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS
        or getattr(aggregate_rows[0], "symbol", None)
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        or getattr(aggregate_rows[0], "symbol_regex", None) is not None
        or getattr(aggregate_rows[0], "name", None)
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_NAME
        or getattr(aggregate_rows[0], "bn_name", None)
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_NAME
        or getattr(aggregate_rows[0], "byte_length", None)
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
    ):
        raise ValueError(
            "HUD stats-list static-storage bridge requires the synchronized "
            "exact decorated aggregate symbol and positive 0x7844 extent"
        )

    symbols = document.collection("symbols")
    storage_rows = document.collection("storage_contributions")
    owners = document.collection("owners")

    def rows_at_address(
        rows: Mapping[str, Any],
        address: str,
        *,
        storage: bool,
    ) -> list[tuple[str, Mapping[str, Any]]]:
        matches: list[tuple[str, Mapping[str, Any]]] = []
        for entity_id, row in rows.items():
            if not isinstance(row, Mapping):
                continue
            reference = row.get("reference")
            raw_address = (
                reference.get("address")
                if storage and isinstance(reference, Mapping)
                else row.get("address", row.get("start"))
            )
            if not isinstance(raw_address, str):
                continue
            try:
                if normalize_address(raw_address) == address:
                    matches.append((str(entity_id), row))
            except ProgressError:
                continue
        return matches

    aggregate_symbol_rows = rows_at_address(
        symbols,
        _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS,
        storage=False,
    )
    leaf_symbol_rows = rows_at_address(
        symbols,
        _cc_catalog.HUD_UI_MGR_STATS_LIST_ADDRESS,
        storage=False,
    )
    aggregate_storage_rows = rows_at_address(
        storage_rows,
        _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS,
        storage=True,
    )
    leaf_storage_rows = rows_at_address(
        storage_rows,
        _cc_catalog.HUD_UI_MGR_STATS_LIST_ADDRESS,
        storage=True,
    )
    if (
        len(aggregate_symbol_rows) != 1
        or aggregate_symbol_rows[0][0]
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID
        or len(leaf_symbol_rows) != 1
        or leaf_symbol_rows[0][0] != _cc_catalog.HUD_UI_MGR_STATS_LIST_SYMBOL_ID
        or len(aggregate_storage_rows) != 1
        or aggregate_storage_rows[0][0]
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_STORAGE_ID
        or len(leaf_storage_rows) != 1
        or leaf_storage_rows[0][0] != _cc_catalog.HUD_UI_MGR_STATS_LIST_STORAGE_ID
    ):
        raise ValueError(
            "HUD stats-list static-storage bridge requires unique aggregate "
            "and leaf symbol/storage rows without data/provider collisions"
        )

    aggregate_symbol = aggregate_symbol_rows[0][1]
    leaf_symbol = leaf_symbol_rows[0][1]
    aggregate_storage = aggregate_storage_rows[0][1]
    leaf_storage = leaf_storage_rows[0][1]
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
        or leaf_symbol.get("binary") != "recoil"
        or leaf_symbol.get("kind") != "data"
        or leaf_symbol.get("disposition") != "authored"
        or leaf_symbol.get("navigation_name")
        != _cc_catalog.HUD_UI_MGR_STATS_LIST_NAME
        or leaf_symbol.get("ownership_state") != "primary-owned"
        or leaf_symbol.get("output_section_id") != "recoil:section:.data"
        or leaf_symbol.get("storage_contribution_ids")
        != [_cc_catalog.HUD_UI_MGR_STATS_LIST_STORAGE_ID]
        or not isinstance(
            leaf_symbol.get("verification_target_ids"),
            list,
        )
        or not leaf_symbol.get("verification_target_ids")
        or len(leaf_symbol["verification_target_ids"])
        != len(set(leaf_symbol["verification_target_ids"]))
        or leaf_symbol.get("extent_state") != "unknown"
        or leaf_symbol.get("size") is not None
        or leaf_symbol.get("end_exclusive") is not None
        or leaf_symbol.get("logical_aliases")
        or leaf_symbol.get("relocation_target_binding")
    ):
        raise ValueError(
            "HUD stats-list static-storage bridge requires exact authored "
            "aggregate and canonical primary-owned leaf classification"
        )

    for storage_id, storage, symbol_id, address in (
        (
            _cc_catalog.HUD_UI_MGR_AGGREGATE_STORAGE_ID,
            aggregate_storage,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS,
        ),
        (
            _cc_catalog.HUD_UI_MGR_STATS_LIST_STORAGE_ID,
            leaf_storage,
            _cc_catalog.HUD_UI_MGR_STATS_LIST_SYMBOL_ID,
            _cc_catalog.HUD_UI_MGR_STATS_LIST_ADDRESS,
        ),
    ):
        reference = storage.get("reference")
        if (
            storage.get("binary") != "recoil"
            or storage.get("kind") != "data-symbol"
            or storage.get("output_section_id")
            != "recoil:section:.data"
            or storage.get("overlap") != "none"
            or storage.get("owner_ids") != [_cc_catalog.HUD_UI_MGR_OWNER_ID]
            or storage.get("parent_contribution_id") is not None
            or storage.get("symbol_ids") != [symbol_id]
            or storage.get("logical_aliases")
            or not isinstance(reference, Mapping)
            or normalize_address(str(reference.get("address", "")))
            != address
            or reference.get("extent_state") != "unknown"
            or reference.get("size") is not None
            or reference.get("end_exclusive") is not None
        ):
            raise ValueError(
                "HUD stats-list static-storage bridge requires exact authored "
                f"storage linkage for {storage_id}"
            )

    owner = owners.get(_cc_catalog.HUD_UI_MGR_OWNER_ID)
    gates = owner.get("gates") if isinstance(owner, Mapping) else None
    primary_relationships = [
        (str(owner_id), relationship)
        for owner_id, candidate_owner in owners.items()
        if isinstance(candidate_owner, Mapping)
        for relationship in candidate_owner.get("relationships", [])
        if isinstance(relationship, Mapping)
        and relationship.get("kind") == "primary-data"
        and relationship.get("symbol_id")
        in {
            _cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID,
            _cc_catalog.HUD_UI_MGR_STATS_LIST_SYMBOL_ID,
        }
    ]
    expected_relationships = {
        (
            _cc_catalog.HUD_UI_MGR_OWNER_ID,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_NAME,
        ),
        (
            _cc_catalog.HUD_UI_MGR_OWNER_ID,
            _cc_catalog.HUD_UI_MGR_STATS_LIST_SYMBOL_ID,
            _cc_catalog.HUD_UI_MGR_STATS_LIST_ADDRESS,
            _cc_catalog.HUD_UI_MGR_STATS_LIST_NAME,
        ),
    }
    actual_relationships = {
        (
            owner_id,
            str(relationship.get("symbol_id", "")),
            normalize_address(str(relationship.get("address", ""))),
            str(relationship.get("name", "")),
        )
        for owner_id, relationship in primary_relationships
    }
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
        or len(primary_relationships) != 2
        or actual_relationships != expected_relationships
    ):
        raise ValueError(
            "HUD stats-list static-storage bridge requires one identical "
            "non-provider owner with accepted boundary/source/data/linkage gates"
        )

    aggregate_start = address_value(_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS)
    leaf_start = address_value(_cc_catalog.HUD_UI_MGR_STATS_LIST_ADDRESS)
    if (
        aggregate_start + _cc_catalog.HUD_UI_MGR_STATS_LIST_DISPLACEMENT
        != leaf_start
        or _cc_catalog.HUD_UI_MGR_STATS_LIST_DISPLACEMENT < 0
        or _cc_catalog.HUD_UI_MGR_STATS_LIST_DISPLACEMENT
        + _cc_catalog.HUD_UI_MGR_STATS_LIST_ACCESS_WIDTH
        > _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
    ):
        raise ValueError(
            "HUD stats-list static-storage bridge leaf is not the exact "
            "bounded 4-byte field at aggregate delta 0x7610"
        )

    decorated_folded = _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL.casefold()
    ordinary_callable_names = [
        name
        for name in indexes.by_candidate_name
        if name.casefold() == decorated_folded
    ]
    ordinary_storage_names = [
        (name, identity)
        for name, identity in indexes.storage_by_name.items()
        if name.casefold() == decorated_folded
    ]
    permitted_storage_names = {
        (),
        ((_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL, ""),),
        (
            (
                _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}",
            ),
        ),
    }
    candidate_bridge_names = [
        name
        for name in (
            set(candidate_storage_bridges)
            | set(compiler_generated_bridges)
        )
        if name.casefold() == decorated_folded
    ]
    retail_name_addresses: set[str] = set()
    for name, rows in bridge_names.items():
        if name.casefold() != decorated_folded:
            continue
        values = (
            list(rows)
            if isinstance(rows, (list, tuple, set, frozenset))
            else [rows]
        )
        for row in values:
            try:
                retail_name_addresses.add(
                    normalize_address(str(getattr(row, "address", "")))
                )
            except ProgressError:
                retail_name_addresses.add("")
    if (
        ordinary_callable_names
        or tuple(ordinary_storage_names) not in permitted_storage_names
        or candidate_bridge_names
        or (
            retail_name_addresses
            and retail_name_addresses != {_cc_catalog.HUD_UI_MGR_AGGREGATE_ADDRESS}
        )
        or _cc_catalog.HUD_UI_MGR_STATS_LIST_IDENTITY in indexes.provider_ids
    ):
        raise ValueError(
            "HUD stats-list static-storage bridge has a data/provider/IAT/"
            "alias/registry identity collision"
        )

    expected_decimal_expression = (
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
        f"+{_cc_catalog.HUD_UI_MGR_STATS_LIST_DISPLACEMENT}"
    )
    expected_hex_expression = (
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
        f"+0x{_cc_catalog.HUD_UI_MGR_STATS_LIST_DISPLACEMENT:x}"
    )
    instruction_offsets = _cc_cfg._instruction_runtime_addresses(
        candidate.instructions,
        source="cod",
        caller_start=0,
    )
    references = [
        (index, instruction, offset)
        for index, (instruction, offset) in enumerate(
            zip(candidate.instructions, instruction_offsets)
        )
        if "g_huduimgr" in instruction.raw_text.casefold()
    ]
    exact_references = [
        row
        for row in references
        if exact_mov_source(row[1])
        in {expected_decimal_expression, expected_hex_expression}
    ]
    if len(references) != 1 or len(exact_references) != 1:
        raise ValueError(
            "HUD stats-list static-storage bridge requires one exact "
            "case-sensitive candidate aggregate field reference"
        )
    instruction_index, instruction, instruction_offset = exact_references[0]
    if (
        instruction_index != 0
        or instruction_offset
        not in {None, _cc_catalog.HUD_UI_MGR_STATS_LIST_REFERENCE_OFFSET}
        or _cc_cfg._instruction_mnemonic(instruction) != "mov"
        or not re.match(
            r"^mov\s+ecx\s*,",
            instruction.raw_text.strip(),
            flags=re.IGNORECASE,
        )
        or tuple(item.lower() for item in instruction.bytes)
        != ("8b", "0d", "10", "76", "00", "00")
        or caller.data[:6] != b"\x8b\x0d\x10\x76\x00\x00"
    ):
        raise ValueError(
            "HUD stats-list static-storage bridge requires exact COD/COFF "
            "MOV ECX, moffs32 bytes at caller offset zero"
        )

    folded_relocations = [
        relocation
        for relocation in caller.relocations
        if relocation.symbol_name.casefold() == decorated_folded
    ]
    relocation_rows = [
        relocation
        for relocation in caller.relocations
        if relocation.offset == _cc_catalog.HUD_UI_MGR_STATS_LIST_RELOCATION_OFFSET
    ]
    undefined_folded = [
        name
        for name in caller.undefined_external_data
        if name.casefold() == decorated_folded
    ]
    defined_folded = [
        name
        for name in caller.defined_external_data
        if name.casefold() == decorated_folded
    ]
    relocation_end = _cc_catalog.HUD_UI_MGR_STATS_LIST_RELOCATION_OFFSET + 4
    if (
        len(folded_relocations) != 1
        or relocation_rows != folded_relocations
        or folded_relocations[0].symbol_name
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        or folded_relocations[0].type != IMAGE_REL_I386_DIR32
        or undefined_folded
        != [_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL]
        or defined_folded
        or relocation_end > len(caller.data)
        or relocation_end > len(caller.relocation_mask)
        or struct.unpack_from(
            "<I",
            caller.data,
            _cc_catalog.HUD_UI_MGR_STATS_LIST_RELOCATION_OFFSET,
        )[0]
        != _cc_catalog.HUD_UI_MGR_STATS_LIST_DISPLACEMENT
        or not all(
            caller.relocation_mask[index]
            for index in range(
                _cc_catalog.HUD_UI_MGR_STATS_LIST_RELOCATION_OFFSET,
                relocation_end,
            )
        )
    ):
        raise ValueError(
            "HUD stats-list static-storage bridge requires one exact undefined "
            "DIR32 relocation with addend 0x7610 at caller+2"
        )

    expression = exact_mov_source(instruction)
    return {
        expression: ReviewedStaticStorageReferenceBridge(
            aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            displacement=_cc_catalog.HUD_UI_MGR_STATS_LIST_DISPLACEMENT,
            access_width=_cc_catalog.HUD_UI_MGR_STATS_LIST_ACCESS_WIDTH,
            storage_identity=_cc_catalog.HUD_UI_MGR_STATS_LIST_IDENTITY,
        )
    }


def _hud_loading_checkpoint_stdio_iat_indexes(
    retail_instructions: Sequence[Instruction],
    *,
    candidate: CandidateAssembly,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_data_rows: Sequence[Any],
    bridge: BinaryNinjaBridge,
    reference: Path = _cc_catalog.DEFAULT_REFERENCE,
) -> IdentityIndexes:
    """Publish only this caller's immutable-retail ``puts``/``fflush`` IATs.

    This is a comparison-scoped identity bridge, not a provider registration.
    The two identities come from the immutable retail import directory, exact
    live retail data/provider rows and slot bytes, and the already-reviewed
    authored caller/TU/target authority.  Candidate COD/COFF proves only the
    independently compiled call form and symbols; it never supplies expected
    retail identity.
    """

    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_LOADING_CHECKPOINT_CALLER_START:
        return indexes

    caller_row = document.collection("symbols").get(
        _cc_catalog.HUD_LOADING_CHECKPOINT_CALLER_IDENTITY.removeprefix("symbol:")
    )
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
        == _cc_catalog.HUD_LOADING_CHECKPOINT_CALLER_START
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
        caller_identity != _cc_catalog.HUD_LOADING_CHECKPOINT_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_LOADING_CHECKPOINT_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(normalized_start) != caller_identity
        or caller_identity in indexes.provider_ids
        or not isinstance(caller_row, Mapping)
        or caller_row.get("binary") != "recoil"
        or caller_row.get("kind") != "function"
        or caller_row.get("pipeline_class") != "authored"
        or caller_row.get("ownership_state") != "primary-owned"
        or caller_row.get("address") != normalized_start
        or caller_row.get("end_exclusive")
        != _cc_catalog.HUD_LOADING_CHECKPOINT_CALLER_END_EXCLUSIVE
        or caller_row.get("extent_state") != "known"
        or caller_row.get("size") != 0x90
        or caller_row.get("navigation_name")
        != "HudUiLoadingCheckpoint::AdvanceAndLog"
        or caller_row.get("output_section_id") != "recoil:section:.text"
        or caller_row.get("physical_block_id") != "recoil:block:0x404ca0"
        or not exact_required_target_membership(
            caller_row.get("verification_target_ids", ()),
            (_cc_catalog.HUD_UI_MESSAGE_REBUILD_WEAPON_ORDER_TARGET_ID, _cc_catalog.HUD_LOADING_CHECKPOINT_TRIGGER_TARGET_ID),
        )
        or not isinstance(trace, Mapping)
        or trace.get("state") != "resolved"
        or trace.get("reason_code") not in {None, ""}
        or not isinstance(source_edges, list)
        or len(source_edges) != 1
        or not isinstance(source_edges[0], Mapping)
        or source_edges[0].get("relation") != "defines"
        or source_edges[0].get("anchor_id")
        != _cc_catalog.HUD_LOADING_CHECKPOINT_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
        or definition is None
        or definition.symbol != _cc_catalog.HUD_LOADING_CHECKPOINT_CALLER_SYMBOL
        or len(definition.data) != 0x80
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
            "HUD loading-checkpoint stdio IAT bridge requires the exact "
            "authored caller, source anchor/TU, extent, object symbol, and "
            "governed target authority"
        )

    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "")
        != _cc_catalog.HUD_LOADING_CHECKPOINT_CALLER_SYMBOL
        or getattr(contribution_row, "symbol_regex", None) is not None
        or getattr(contribution_row, "name", "")
        != "HudUiLoadingCheckpoint::AdvanceAndLog"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or getattr(contribution_row, "required_presence", None) is not True
        or getattr(contribution_row, "full_order_gate", None) is not True
    ):
        raise ValueError(
            "HUD loading-checkpoint stdio IAT bridge rejects exact hud.cpp "
            "target contribution-row authority drift"
        )

    retail_addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(normalized_start),
    )
    retail_invocations = tuple(
        address
        for instruction, address in zip(retail_instructions, retail_addresses)
        if address is not None and _cc_cfg._instruction_mnemonic(instruction) == "call"
    )
    candidate_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    candidate_invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    candidate_invocations = tuple(
        candidate_offsets[index] for index in candidate_invocation_indices
    )
    if (
        retail_invocations != _cc_catalog.HUD_LOADING_CHECKPOINT_RETAIL_CALL_ORDER
        or candidate_invocations
        != _cc_catalog.HUD_LOADING_CHECKPOINT_CANDIDATE_CALL_ORDER
    ):
        raise ValueError(
            "HUD loading-checkpoint stdio IAT bridge rejects complete retail/"
            "candidate five-call population, order, or call form drift"
        )

    retail_by_address = {
        normalize_address(hex(address)): (index, instruction)
        for index, (instruction, address) in enumerate(
            zip(retail_instructions, retail_addresses)
        )
        if address is not None
    }
    candidate_by_offset = {
        offset: (index, candidate.instructions[index])
        for index, offset in enumerate(candidate_offsets)
        if offset is not None
    }
    storage_by_address = dict(indexes.storage_by_address)
    storage_by_name = dict(indexes.storage_by_name)
    for spec in _cc_catalog.HUD_LOADING_CHECKPOINT_STDIO_IAT_SPECS:
        import_name = str(spec["import_name"])
        candidate_symbol = str(spec["candidate_symbol"])
        iat_address = str(spec["iat_address"])
        provider_address = str(spec["provider_address"])
        iat_identity = f"iat:{import_name}"
        retail_import, _directory_context = retail_import_target(
            reference=reference,
            address=iat_address,
            dll="MSVCRT.dll",
            import_name=import_name,
        )
        if (
            retail_import.address != iat_address
            or retail_import.dll != "MSVCRT.dll"
            or retail_import.import_name != import_name
            or retail_import.import_ordinal is not None
        ):
            raise ValueError(
                "HUD loading-checkpoint stdio IAT bridge immutable retail "
                f"{import_name} import tuple drifted"
            )

        iat_rows = [
            row
            for row in bridge_data_rows
            if normalize_address(str(getattr(row, "address", "")))
            == iat_address
        ]
        provider_rows = [
            row
            for row in bridge_data_rows
            if normalize_address(str(getattr(row, "address", "")))
            == provider_address
        ]
        if (
            len(iat_rows) != 1
            or getattr(iat_rows[0], "name", "") != import_name
            or getattr(iat_rows[0], "raw_name", "") != import_name
            or getattr(iat_rows[0], "type_text", "") != spec["iat_type"]
            or getattr(iat_rows[0], "size", 0) != 4
        ):
            raise ValueError(
                "HUD loading-checkpoint stdio IAT bridge requires one exact "
                f"live retail typed {import_name} IAT row"
            )
        if (
            len(provider_rows) != 1
            or getattr(provider_rows[0], "name", "") != import_name
            or getattr(provider_rows[0], "raw_name", "") != import_name
            or getattr(provider_rows[0], "type_text", "")
            != spec["provider_type"]
            or getattr(provider_rows[0], "size", -1) != 0
        ):
            raise ValueError(
                "HUD loading-checkpoint stdio IAT bridge requires one exact "
                f"live retail bound {import_name} provider row"
            )
        slot_bytes = _cc_cfg._hexdump_bytes(bridge.hexdump(iat_address, 4))
        if (
            len(slot_bytes) != 4
            or normalize_address(struct.unpack("<I", slot_bytes)[0])
            != provider_address
        ):
            raise ValueError(
                "HUD loading-checkpoint stdio IAT bridge retail slot bytes "
                f"do not bind the exact {import_name} provider row"
            )

        retail_call_address = str(spec["retail_call_address"])
        candidate_call_offset = int(spec["candidate_call_offset"])
        retail_call_row = retail_by_address.get(retail_call_address)
        candidate_call_row = candidate_by_offset.get(candidate_call_offset)
        if retail_call_row is None or candidate_call_row is None:
            raise ValueError(
                "HUD loading-checkpoint stdio IAT bridge lacks an exact "
                f"{import_name} callsite"
            )
        retail_call_index, retail_call = retail_call_row
        candidate_call_index, candidate_call = candidate_call_row
        if (
            _cc_cfg._instruction_mnemonic(retail_call) != "call"
            or _cc_cfg._instruction_operand(retail_call).strip().lower()
            != f"dword [{import_name}]"
            or _cc_targets._exact_ff15_absolute_address(retail_call) != iat_address
            or _cc_cfg._cleanup_after(retail_instructions, retail_call_index) != 4
            or _cc_cfg._instruction_mnemonic(candidate_call) != "call"
            or _cc_cfg._instruction_operand(candidate_call).strip().lower()
            != f"dword {candidate_symbol.lower()}"
            or tuple(value.lower() for value in candidate_call.bytes)
            != ("ff", "15", "00", "00", "00", "00")
            or _cc_cfg._cleanup_after(candidate.instructions, candidate_call_index) != 4
            or retail_invocations[int(spec["ordinal"])]
            != address_value(retail_call_address)
            or candidate_invocations[int(spec["ordinal"])]
            != candidate_call_offset
        ):
            raise ValueError(
                "HUD loading-checkpoint stdio IAT bridge rejects exact "
                f"{import_name} ordinal, FF15 form, storage, target, or "
                "caller cleanup +4"
            )

        references = tuple(
            relocation
            for relocation in definition.relocations
            if relocation.symbol_name == candidate_symbol
        )
        relocation_offset = int(spec["candidate_relocation_offset"])
        if (
            definition.undefined_external_functions.count(candidate_symbol)
            != 1
            or len(references) != 1
            or references[0].type != IMAGE_REL_I386_DIR32
            or references[0].offset != relocation_offset
            or relocation_offset < 2
            or relocation_offset + 4 > len(definition.data)
            or definition.data[relocation_offset - 2 : relocation_offset]
            != b"\xff\x15"
            or struct.unpack_from("<I", definition.data, relocation_offset)[0]
            != 0
            or not all(
                definition.relocation_mask[index]
                for index in range(relocation_offset, relocation_offset + 4)
            )
            or (
                definition.defined_external_functions
                + definition.undefined_external_data
                + definition.defined_external_data
            ).count(candidate_symbol)
            != 0
        ):
            raise ValueError(
                "HUD loading-checkpoint stdio IAT bridge requires one exact "
                f"undefined external {candidate_symbol} FF15 DIR32 relocation"
            )

        for mapping, key in (
            (storage_by_address, iat_address),
            (storage_by_name, import_name),
            (storage_by_name, candidate_symbol),
        ):
            prior = mapping.get(key)
            if prior is not None and prior != iat_identity:
                raise ValueError(
                    "HUD loading-checkpoint stdio IAT bridge conflicts with "
                    f"an existing storage identity for {key!r}"
                )
            mapping[key] = iat_identity

    return replace(
        indexes,
        storage_by_address=storage_by_address,
        storage_by_name=storage_by_name,
    )


def _hud_ui_mp_exit_update_candidate_indirect_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedExactIndirectStorageBridge]:
    """Recover only Update's exact relocation-backed slot-0 stack call.

    VC5 places the conditional stack update behind a local branch whose COD
    label is intentionally not candidate truth.  This bridge is therefore
    published only for the complete current caller/object contribution and
    exact reviewed global-storage authority.  It supplies candidate receiver
    provenance at one call site; retail plus tracker identities continue to
    own the expected invocation contract.
    """
    from _recoil.call_contract.records import ReviewedExactIndirectStorageBridge

    normalized_start = normalize_address(caller_start)
    if normalized_start != _cc_catalog.HUD_UI_MP_EXIT_UPDATE_CALLER_START:
        return {}

    expected_row = {
        "ordinal": _cc_catalog.HUD_UI_MP_EXIT_UPDATE_CALL_ORDINAL,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": (
            f"load(storage:{_cc_catalog.HUD_UI_MP_EXIT_UPDATE_STACK_SYMBOL_ID})"
        ),
        "slot_displacement": 0,
        "cleanup_bytes": None,
    }
    if (
        len(expected) <= _cc_catalog.HUD_UI_MP_EXIT_UPDATE_CALL_ORDINAL
        or dict(expected[_cc_catalog.HUD_UI_MP_EXIT_UPDATE_CALL_ORDINAL]) != expected_row
    ):
        raise ValueError(
            "HUD MP-exit Update bridge requires immutable retail ordinal-5 "
            "slot-0 top-message-stack truth"
        )

    symbols = document.collection("symbols")
    storage_rows = document.collection("storage_contributions")
    targets = document.collection("verification_targets")
    caller_symbol_id = _cc_catalog.HUD_UI_MP_EXIT_UPDATE_CALLER_IDENTITY.removeprefix(
        "symbol:"
    )
    caller_row = symbols.get(caller_symbol_id)
    caller_target = targets.get(_cc_catalog.HUD_UI_MP_EXIT_UPDATE_TARGET_ID)
    caller_registration = (
        caller_target.get("registration")
        if isinstance(caller_target, Mapping)
        else None
    )
    definition = candidate.caller_definition
    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MP_EXIT_UPDATE_CALLER_START
    ]
    if (
        caller_identity != _cc_catalog.HUD_UI_MP_EXIT_UPDATE_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MP_EXIT_UPDATE_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(normalized_start) != caller_identity
        or indexes.by_candidate_name.get(_cc_catalog.HUD_UI_MP_EXIT_UPDATE_CALLER_SYMBOL)
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
        != _cc_catalog.HUD_UI_MP_EXIT_UPDATE_CALLER_START
        or normalize_address(str(caller_row.get("end_exclusive", "")))
        != _cc_catalog.HUD_UI_MP_EXIT_UPDATE_CALLER_END_EXCLUSIVE
        or caller_row.get("size") != len(_cc_catalog.HUD_UI_MP_EXIT_UPDATE_CANDIDATE_BODY)
        or caller_row.get("navigation_name") != "HudUiMpExitDialog::Update"
        or caller_row.get("output_section_id") != "recoil:section:.text"
        or caller_row.get("physical_block_id") != "recoil:block:0x417350"
        or caller_row.get("verification_target_ids")
        != [
            "recoil:vc5-target:hud_ui_mp_exit_dialog_table_cluster",
            _cc_catalog.HUD_UI_MP_EXIT_UPDATE_TARGET_ID,
        ]
        or not isinstance(caller_target, Mapping)
        or caller_target.get("binary") != "recoil"
        or caller_target.get("kind") != "vc5"
        or caller_target.get("name") != _cc_catalog.HUD_UI_MP_EXIT_UPDATE_TARGET_NAME
        or tuple(caller_target.get("registered_addresses", ())).count(
            _cc_catalog.HUD_UI_MP_EXIT_UPDATE_CALLER_START
        )
        != 1
        or not isinstance(caller_registration, Mapping)
        or caller_registration.get("binary") != "recoil"
        or caller_registration.get("name") != _cc_catalog.HUD_UI_MP_EXIT_UPDATE_TARGET_NAME
        or caller_registration.get("manifest_path")
        != str(
            _cc_catalog.HUD_UI_MP_EXIT_UPDATE_TARGET_MANIFEST.relative_to(REPO_ROOT)
        ).replace("\\", "/")
        or caller_registration.get("source_from")
        != _cc_catalog.HUD_UI_MP_EXIT_UPDATE_SOURCE_PATH
        or caller_registration.get("check_translation_unit_function_order")
        is not True
        or caller_registration.get("function_order_scope") != "authored"
        or definition is None
        or definition.symbol != _cc_catalog.HUD_UI_MP_EXIT_UPDATE_CALLER_SYMBOL
        or target is None
        or getattr(target, "name", "") != _cc_catalog.HUD_UI_MP_EXIT_UPDATE_TARGET_NAME
        or getattr(target, "target_binary", "") != "recoil"
        or Path(str(getattr(target, "manifest_path", ""))).resolve()
        != _cc_catalog.HUD_UI_MP_EXIT_UPDATE_TARGET_MANIFEST.resolve()
        or not bool(
            getattr(target, "check_translation_unit_function_order", False)
        )
        or len(contribution_rows) != 1
    ):
        raise ValueError(
            "HUD MP-exit Update bridge requires the exact authored caller, "
            "mission.cpp contribution, extent, and governed target authority"
        )

    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MP_EXIT_UPDATE_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "")
        != _cc_catalog.HUD_UI_MP_EXIT_UPDATE_CALLER_SYMBOL
        or getattr(contribution_row, "symbol_regex", None) is not None
        or getattr(contribution_row, "name", "")
        != "HudUiMpExitDialog::Update"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or getattr(contribution_row, "required_presence", None) is not True
        or getattr(contribution_row, "full_order_gate", None) is not True
    ):
        raise ValueError(
            "HUD MP-exit Update bridge rejects stale mission.cpp "
            "contribution-row authority"
        )

    stack_symbol = symbols.get(_cc_catalog.HUD_UI_MP_EXIT_UPDATE_STACK_SYMBOL_ID)
    stack_storage = storage_rows.get(_cc_catalog.HUD_UI_MP_EXIT_UPDATE_STACK_STORAGE_ID)
    stack_target = targets.get(_cc_catalog.HUD_UI_MP_EXIT_UPDATE_STACK_TARGET_ID)
    stack_registration = (
        stack_target.get("registration")
        if isinstance(stack_target, Mapping)
        else None
    )
    stack_trace = (
        stack_symbol.get("source_traceability")
        if isinstance(stack_symbol, Mapping)
        else None
    )
    stack_identity = f"storage:{_cc_catalog.HUD_UI_MP_EXIT_UPDATE_STACK_SYMBOL_ID}"
    if (
        not isinstance(stack_symbol, Mapping)
        or _cc_identity._symbol_identity(
            _cc_catalog.HUD_UI_MP_EXIT_UPDATE_STACK_SYMBOL_ID, stack_symbol
        )
        != f"symbol:{_cc_catalog.HUD_UI_MP_EXIT_UPDATE_STACK_SYMBOL_ID}"
        or stack_symbol.get("binary") != "recoil"
        or stack_symbol.get("kind") != "data"
        or stack_symbol.get("disposition") != "authored"
        or stack_symbol.get("extent_state") != "unknown"
        or normalize_address(str(stack_symbol.get("address", "")))
        != _cc_catalog.HUD_UI_MP_EXIT_UPDATE_STACK_ADDRESS
        or stack_symbol.get("navigation_name")
        != _cc_catalog.HUD_UI_MP_EXIT_UPDATE_STACK_NAME
        or stack_symbol.get("output_section_id") != "recoil:section:.data"
        or stack_symbol.get("storage_contribution_ids")
        != [_cc_catalog.HUD_UI_MP_EXIT_UPDATE_STACK_STORAGE_ID]
        or stack_symbol.get("verification_target_ids")
        != [_cc_catalog.HUD_UI_MP_EXIT_UPDATE_STACK_TARGET_ID]
        or not isinstance(stack_trace, Mapping)
        or stack_trace.get("state") != "resolved"
        or stack_trace.get("reason_code") not in {None, ""}
        or stack_trace.get("source_edges")
        != [
            {
                "anchor_id": (
                    "recoil:anchor:gamezrecoil-zui-zui-widgets-"
                    "g-huduitopmessagestack"
                ),
                "emission_context": {
                    "translation_unit": "src/GameZRecoil/zUI/zui_widgets.cpp"
                },
                "evidence_ids": [],
                "relation": "defines",
            }
        ]
        or not isinstance(stack_storage, Mapping)
        or stack_storage.get("binary") != "recoil"
        or stack_storage.get("kind") != "data-symbol"
        or stack_storage.get("output_section_id") != "recoil:section:.data"
        or stack_storage.get("overlap") != "none"
        or stack_storage.get("parent_contribution_id") is not None
        or stack_storage.get("owner_ids")
        != ["recoil:owner:hud_ui.hud_ui_top_message_stack_global"]
        or stack_storage.get("symbol_ids")
        != [_cc_catalog.HUD_UI_MP_EXIT_UPDATE_STACK_SYMBOL_ID]
        or stack_storage.get("reference")
        != {
            "address": _cc_catalog.HUD_UI_MP_EXIT_UPDATE_STACK_ADDRESS,
            "evidence_ids": [],
            "extent_state": "unknown",
        }
        or not isinstance(stack_target, Mapping)
        or stack_target.get("binary") != "recoil"
        or stack_target.get("kind") != "vc5"
        or stack_target.get("name") != "hud_ui_top_message_stack_global"
        or stack_target.get("symbol_ids")
        != [_cc_catalog.HUD_UI_MP_EXIT_UPDATE_STACK_SYMBOL_ID]
        or stack_target.get("unresolved_addresses") != []
        or not isinstance(stack_registration, Mapping)
        or stack_registration.get("manifest_path")
        != "tools/vc5_verify_targets/hud_ui_top_message_stack_global.json"
        or stack_registration.get("source_from")
        != "src/GameZRecoil/zUI/zui.cpp"
        or stack_registration.get("data_addresses")
        != [_cc_catalog.HUD_UI_MP_EXIT_UPDATE_STACK_ADDRESS]
        or indexes.storage_by_address.get(_cc_catalog.HUD_UI_MP_EXIT_UPDATE_STACK_ADDRESS)
        != stack_identity
        or indexes.storage_by_name.get(_cc_catalog.HUD_UI_MP_EXIT_UPDATE_STACK_NAME)
        != stack_identity
        or stack_identity in indexes.provider_ids
    ):
        raise ValueError(
            "HUD MP-exit Update bridge requires exact reviewed top-message-"
            "stack symbol, storage, source, and target authority"
        )

    assert definition is not None
    local_constant_sentinel = object()
    expected_relocations = (
        (0x1B, IMAGE_REL_I386_DIR32, local_constant_sentinel),
        (0x42, IMAGE_REL_I386_REL32, "?SetScaleAndRebuild@HudScoreboard@@YGXM@Z"),
        (0x47, IMAGE_REL_I386_REL32, "?RunPostprocessOnPrimaryBuffer@zVideo@@YAHXZ"),
        (0x5A, IMAGE_REL_I386_REL32, "?BlitToActiveTarget@zVid_Image@@YIXPAUzVidImagePartial@@HHHPAUzVidRect32@@@Z"),
        (0x66, IMAGE_REL_I386_REL32, "?UpdateAll@HudUiBackgroundContainer@@UAEXM@Z"),
        (0x76, IMAGE_REL_I386_REL32, "?DispatchSetScale@HudScoreboard@@YGXM@Z"),
        (0x7E, IMAGE_REL_I386_DIR32, _cc_catalog.HUD_UI_MP_EXIT_UPDATE_STACK_OBJECT_SYMBOL),
        (0x83, IMAGE_REL_I386_DIR32, "_g_Time_UnscaledDeltaTimeSec"),
        (0x8D, IMAGE_REL_I386_REL32, "?DispatchUnlockPrimarySurfaceState@zVideo@@YAHXZ"),
        (0x92, IMAGE_REL_I386_REL32, "?GetWindowSection@zOpt@@YIPAUzOpt_ViewRectSection@@XZ"),
        (0x99, IMAGE_REL_I386_REL32, "?GetWindowSection@zOpt@@YIPAUzOpt_ViewRectSection@@XZ"),
        (0xA6, IMAGE_REL_I386_REL32, "?AdjustSurfacesIfEnabled@zVideo@@YIHPAUzVidRect32@@0HH@Z"),
    )
    first_relocation = (
        definition.relocations[0] if definition.relocations else None
    )
    normalize_first_local_constant = (
        first_relocation is not None
        and first_relocation.offset == 0x1B
        and first_relocation.type == IMAGE_REL_I386_DIR32
        and re.fullmatch(r"\$T[0-9]+", first_relocation.symbol_name)
        is not None
        and definition.data == _cc_catalog.HUD_UI_MP_EXIT_UPDATE_CANDIDATE_BODY
        and len(definition.relocation_mask) == len(definition.data)
        and struct.unpack_from("<I", definition.data, 0x1B)[0] == 0
        and all(definition.relocation_mask[0x1B:0x1F])
    )
    observed_relocations = tuple(
        (
            row.offset,
            row.type,
            (
                local_constant_sentinel
                if index == 0
                and normalize_first_local_constant
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
        definition.data != _cc_catalog.HUD_UI_MP_EXIT_UPDATE_CANDIDATE_BODY
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
            "HUD MP-exit Update bridge rejects stale candidate body, complete "
            "COFF relocation population, addend, or relocation mask"
        )

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    by_offset = {
        offset: instruction
        for offset, instruction in zip(offsets, candidate.instructions)
        if offset is not None
    }
    exact_unit = {
        0x7C: ("mov", _cc_catalog.HUD_UI_MP_EXIT_UPDATE_STACK_OBJECT_SYMBOL, b"\x8b\x0d\0\0\0\0"),
        0x82: ("mov", "_g_Time_UnscaledDeltaTimeSec", b"\xa1\0\0\0\0"),
        0x87: ("push", "eax", b"\x50"),
        0x88: ("mov", "dword [ecx]", b"\x8b\x11"),
        0x8A: ("call", "dword [edx]", b"\xff\x12"),
    }
    if len(by_offset) != len(candidate.instructions):
        raise ValueError(
            "HUD MP-exit Update bridge requires unique complete COD offsets"
        )
    for offset, (mnemonic, operand_fragment, body) in exact_unit.items():
        instruction = by_offset.get(offset)
        try:
            encoded = bytes(int(value, 16) for value in instruction.bytes)
        except (AttributeError, TypeError, ValueError) as exc:
            raise ValueError(
                "HUD MP-exit Update bridge requires exact candidate stack-call bytes"
            ) from exc
        if (
            _cc_cfg._instruction_mnemonic(instruction) != mnemonic
            or operand_fragment.casefold()
            not in _cc_cfg._instruction_operand(instruction).casefold()
            or encoded != body
        ):
            raise ValueError(
                "HUD MP-exit Update bridge rejects stale global/time/receiver/"
                f"vptr/call topology at +0x{offset:x}"
            )

    return {
        normalize_address(_cc_catalog.HUD_UI_MP_EXIT_UPDATE_CALL_OFFSET): (
            ReviewedExactIndirectStorageBridge(
                register="edx",
                storage_identity=expected_row["storage_identity"],
                slot_displacement=0,
                assembly_source="cod",
            )
        )
    }


def _hud_ui_mgr_enable_hud_candidate_vptr_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedAbsoluteStorageLoadBridge],
    dict[str, ReviewedVptrStorageBridge],
]:
    """Bridge only EnableHud's aggregate-base SetEnabled invocation."""
    from _recoil.call_contract.records import (
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedVptrStorageBridge,
        StorageContainer,
    )
    caller_start = normalize_address(caller_start)
    if caller_start != _cc_catalog.HUD_UI_MGR_ENABLE_CALLER_START:
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
        if name.casefold() == _cc_catalog.HUD_UI_MGR_ENABLE_CALLER_SYMBOL.casefold()
    ]
    if (
        caller_identity != _cc_catalog.HUD_UI_MGR_ENABLE_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_ENABLE_CALLER_END_EXCLUSIVE
        or caller_addresses != [_cc_catalog.HUD_UI_MGR_ENABLE_CALLER_START]
        or caller_identity in indexes.provider_ids
        or caller_name_rows
        not in (
            [],
            [
                (
                    _cc_catalog.HUD_UI_MGR_ENABLE_CALLER_SYMBOL,
                    _cc_catalog.HUD_UI_MGR_ENABLE_CALLER_IDENTITY,
                )
            ],
        )
        or caller is None
        or caller.symbol != _cc_catalog.HUD_UI_MGR_ENABLE_CALLER_SYMBOL
        or len(caller.data)
        != address_value(_cc_catalog.HUD_UI_MGR_ENABLE_CALLER_END_EXCLUSIVE)
        - address_value(_cc_catalog.HUD_UI_MGR_ENABLE_CALLER_START)
        or len(caller.relocation_mask) != len(caller.data)
    ):
        raise ValueError(
            "HUD EnableHud aggregate-base vptr bridge requires the exact "
            "reviewed authored caller identity, extent, symbol, and body"
        )

    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_ENABLE_CALLER_START
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
            "HUD EnableHud aggregate-base vptr bridge requires one exact "
            "current HUD source authority"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != r"\?EnableHud@HudUiMgr@@.*"
        or re.fullmatch(
            r"\?EnableHud@HudUiMgr@@.*",
            caller.symbol,
        )
        is None
        or getattr(contribution_row, "name", "")
        != "HudUiMgr::EnableHud"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD EnableHud aggregate-base vptr bridge requires the exact "
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
        or caller_symbol.get("extent_state") != "known"
        or normalize_address(str(caller_symbol.get("address", "")))
        != _cc_catalog.HUD_UI_MGR_ENABLE_CALLER_START
        or normalize_address(str(caller_symbol.get("end_exclusive", "")))
        != _cc_catalog.HUD_UI_MGR_ENABLE_CALLER_END_EXCLUSIVE
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
        != _cc_catalog.HUD_UI_MGR_ENABLE_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
    ):
        raise ValueError(
            "HUD EnableHud aggregate-base vptr bridge requires one exact "
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
            "HUD EnableHud aggregate-base vptr bridge requires the exact "
            "reviewed aggregate symbol, storage, and target authority"
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
            "HUD EnableHud aggregate-base vptr bridge requires one exact "
            "aggregate storage container and positive extent"
        )

    expected_row = {
        "ordinal": _cc_catalog.HUD_UI_MGR_ENABLE_CALL_ORDINAL,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "callback",
        "target_identity": "",
        "storage_identity": _cc_catalog.HUD_UI_MGR_ENABLE_STORAGE_IDENTITY,
        "slot_displacement": _cc_catalog.HUD_UI_MGR_ENABLE_SLOT_DISPLACEMENT,
        "cleanup_bytes": None,
    }
    if (
        len(expected) <= _cc_catalog.HUD_UI_MGR_ENABLE_CALL_ORDINAL
        or expected[_cc_catalog.HUD_UI_MGR_ENABLE_CALL_ORDINAL] != expected_row
        or sum(
            row.get("storage_identity")
            == _cc_catalog.HUD_UI_MGR_ENABLE_STORAGE_IDENTITY
            and row.get("slot_displacement")
            == _cc_catalog.HUD_UI_MGR_ENABLE_SLOT_DISPLACEMENT
            for row in expected
        )
        != 1
    ):
        raise ValueError(
            "HUD EnableHud aggregate-base vptr bridge requires the exact "
            "ordinal-0 retail indirect-storage contract"
        )
    exact_body = bytes.fromhex(
        "a1 00 00 00 00 56 8b 35 04 00 00 00 "
        "6a 01 b9 00 00 00 00 ff 50 04"
    )
    bounded_end = _cc_catalog.HUD_UI_MGR_ENABLE_CALL_OFFSET + 3
    if (
        caller.data[_cc_catalog.HUD_UI_MGR_ENABLE_LOAD_OFFSET:bounded_end] != exact_body
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in caller.defined_external_data
    ):
        raise ValueError(
            "HUD EnableHud aggregate-base vptr bridge requires the exact "
            "bounded load/argument/receiver/vptr/call object body"
        )
    relocation_specs = {
        0x01: 0,
        0x08: 4,
        0x0F: 0,
    }
    bounded_relocations = tuple(
        row for row in caller.relocations if 0 <= row.offset < bounded_end
    )
    if (
        len(bounded_relocations) != len(relocation_specs)
        or {row.offset for row in bounded_relocations}
        != set(relocation_specs)
        or any(
            row.type != IMAGE_REL_I386_DIR32
            or row.symbol_name != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
            or struct.unpack_from("<I", caller.data, row.offset)[0]
            != relocation_specs[row.offset]
            for row in bounded_relocations
        )
    ):
        raise ValueError(
            "HUD EnableHud aggregate-base vptr bridge rejects missing, "
            "duplicate, extra, or malformed bounded DIR32 relocation"
        )
    expected_mask = {
        offset
        for relocation_offset in relocation_specs
        for offset in range(relocation_offset, relocation_offset + 4)
    }
    if {
        offset
        for offset in range(bounded_end)
        if caller.relocation_mask[offset]
    } != expected_mask:
        raise ValueError(
            "HUD EnableHud aggregate-base vptr bridge requires the exact "
            "bounded relocation mask"
        )

    instruction_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    exact_offsets = (0x00, 0x05, 0x06, 0x0C, 0x0E, 0x13)
    exact_bytes = (
        ("a1", "00", "00", "00", "00"),
        ("56",),
        ("8b", "35", "04", "00", "00", "00"),
        ("6a", "01"),
        ("b9", "00", "00", "00", "00"),
        ("ff", "50", "04"),
    )
    if (
        len(candidate.instructions) < len(exact_offsets)
        or tuple(instruction_offsets[:6]) != exact_offsets
        or tuple(instruction.bytes for instruction in candidate.instructions[:6])
        != exact_bytes
        or tuple(
            _cc_cfg._instruction_mnemonic(instruction)
            for instruction in candidate.instructions[:6]
        )
        != ("mov", "push", "mov", "push", "mov", "call")
        or _cc_cfg._instruction_operand(candidate.instructions[0]).split(",", 1)[0]
        .strip()
        .lower()
        != "eax"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(candidate.instructions[0]).split(",", 1)[-1]
        )
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        or _cc_cfg._instruction_operand(candidate.instructions[1]).strip().lower()
        != "esi"
        or _cc_cfg._instruction_operand(candidate.instructions[2]).split(",", 1)[0]
        .strip()
        .lower()
        != "esi"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(candidate.instructions[2]).split(",", 1)[-1]
        )
        not in {
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}+4",
            f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}+0x4",
        }
        or _cc_cfg._instruction_operand(candidate.instructions[3]).strip().lower()
        not in {"1", "01", "0x1"}
        or re.fullmatch(
            r"ecx\s*,\s*OFFSET\s+FLAT:"
            + re.escape(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL),
            _cc_cfg._instruction_operand(candidate.instructions[4]).strip(),
            flags=re.IGNORECASE,
        )
        is None
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(candidate.instructions[5])
        )
        not in {"eax+4", "eax+0x4"}
    ):
        raise ValueError(
            "HUD EnableHud aggregate-base vptr bridge rejects instruction "
            "offset, byte, register, argument, receiver, or slot drift"
        )
    matching_loads = [
        index
        for index, instruction in enumerate(candidate.instructions)
        if (
            _cc_cfg._instruction_mnemonic(instruction) == "mov"
            and instruction.bytes == exact_bytes[0]
            and _cc_targets._exact_memory_expression(
                _cc_cfg._instruction_operand(instruction).split(",", 1)[-1]
            )
            == _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
    ]
    matching_calls = [
        index
        for index, instruction in enumerate(candidate.instructions)
        if (
            _cc_cfg._instruction_mnemonic(instruction) == "call"
            and instruction.bytes == exact_bytes[-1]
            and _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(instruction))
            in {"eax+4", "eax+0x4"}
        )
    ]
    if matching_loads != [0] or matching_calls != [5]:
        raise ValueError(
            "HUD EnableHud aggregate-base vptr bridge requires one unique "
            "exact EAX load and slot-0x4 call"
        )
    bounded_indices = frozenset(range(6))
    if (
        bool(candidate.local_control_flow_indices & bounded_indices)
        or any(
            target in bounded_indices
            for targets in candidate.local_control_flow_targets.values()
            for target in targets
        )
        or _cc_cfg._cleanup_after(candidate.instructions, 5) is not None
    ):
        raise ValueError(
            "HUD EnableHud aggregate-base vptr bridge rejects cleanup or "
            "control-flow ambiguity in the reviewed call chain"
        )
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    if not invocation_indices or invocation_indices[0] != 5:
        raise ValueError(
            "HUD EnableHud aggregate-base vptr bridge requires the exact "
            "candidate ordinal-0 call"
        )
    next_invocation = (
        invocation_indices[1]
        if len(invocation_indices) > 1
        else len(candidate.instructions)
    )
    for instruction in candidate.instructions[6:next_invocation]:
        operand = _cc_cfg._instruction_operand(instruction).lower()
        if re.search(r"\beax\b", operand):
            destination = (
                operand.split(",", 1)[0].strip()
                if "," in operand
                else ""
            )
            if destination != "eax":
                raise ValueError(
                    "HUD EnableHud aggregate-base vptr bridge rejects "
                    "unexpected call-result consumption"
                )
        if _cc_cfg._instruction_may_clobber_register(instruction, "eax"):
            break

    return (
        {
            normalize_address(hex(_cc_catalog.HUD_UI_MGR_ENABLE_LOAD_OFFSET)): (
                ReviewedAbsoluteStorageLoadBridge(
                    register="eax",
                    aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                    displacement=0,
                    access_width=4,
                    storage_identity=(
                        f"storage:{_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID}"
                    ),
                )
            )
        },
        {
            normalize_address(hex(_cc_catalog.HUD_UI_MGR_ENABLE_CALL_OFFSET)): (
                ReviewedVptrStorageBridge(
                    register="eax",
                    provenance=_cc_catalog.HUD_UI_MGR_ENABLE_LOAD_PROVENANCE,
                    storage_identity=_cc_catalog.HUD_UI_MGR_ENABLE_STORAGE_IDENTITY,
                    slot_displacement=_cc_catalog.HUD_UI_MGR_ENABLE_SLOT_DISPLACEMENT,
                    identity_kind="callback",
                )
            )
        },
    )


def _hud_ui_mgr_disable_track_marker_loop_vptr_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedLoopVptrStorageBridge]:
    """Bridge only DisableHud's first weapon-slot SetVisible invocation."""
    from _recoil.call_contract.records import (
        ReviewedLoopVptrStorageBridge,
        StorageContainer,
    )
    caller_start = normalize_address(caller_start)
    if caller_start != _cc_catalog.HUD_UI_MGR_DISABLE_CALLER_START:
        return {}

    caller = candidate.caller_definition
    caller_addresses = sorted(
        address
        for address, identity in indexes.by_address.items()
        if identity == caller_identity
    )
    caller_name_rows = [
        (name, identity)
        for name, identity in indexes.by_candidate_name.items()
        if name.casefold() == _cc_catalog.HUD_UI_MGR_DISABLE_CALLER_SYMBOL.casefold()
    ]
    if (
        caller_identity != _cc_catalog.HUD_UI_MGR_DISABLE_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_DISABLE_CALLER_END_EXCLUSIVE
        or caller_addresses != [_cc_catalog.HUD_UI_MGR_DISABLE_CALLER_START]
        or caller_identity in indexes.provider_ids
        or caller_name_rows
        not in (
            [],
            [
                (
                    _cc_catalog.HUD_UI_MGR_DISABLE_CALLER_SYMBOL,
                    _cc_catalog.HUD_UI_MGR_DISABLE_CALLER_IDENTITY,
                )
            ],
        )
        or caller is None
        or caller.symbol != _cc_catalog.HUD_UI_MGR_DISABLE_CALLER_SYMBOL
        or len(caller.data)
        != address_value(_cc_catalog.HUD_UI_MGR_DISABLE_CALLER_END_EXCLUSIVE)
        - address_value(_cc_catalog.HUD_UI_MGR_DISABLE_CALLER_START)
        or len(caller.relocation_mask) != len(caller.data)
    ):
        raise ValueError(
            "HUD DisableHud track-marker loop bridge requires the exact "
            "reviewed authored caller identity, extent, symbol, and body"
        )

    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_DISABLE_CALLER_START
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
            "HUD DisableHud track-marker loop bridge requires one exact "
            "current HUD source authority"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != ""
        or getattr(contribution_row, "symbol_regex", None)
        != r"\?DisableHud@HudUiMgr@@.*"
        or re.fullmatch(
            r"\?DisableHud@HudUiMgr@@.*",
            caller.symbol,
        )
        is None
        or getattr(contribution_row, "name", "")
        != "HudUiMgr::DisableHud"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD DisableHud track-marker loop bridge requires the exact "
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
        or caller_symbol.get("extent_state") != "known"
        or normalize_address(str(caller_symbol.get("address", "")))
        != _cc_catalog.HUD_UI_MGR_DISABLE_CALLER_START
        or normalize_address(str(caller_symbol.get("end_exclusive", "")))
        != _cc_catalog.HUD_UI_MGR_DISABLE_CALLER_END_EXCLUSIVE
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
        != _cc_catalog.HUD_UI_MGR_DISABLE_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
    ):
        raise ValueError(
            "HUD DisableHud track-marker loop bridge requires one exact "
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
            "HUD DisableHud track-marker loop bridge requires the exact "
            "reviewed aggregate symbol, storage, and target authority"
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
            "HUD DisableHud track-marker loop bridge requires one exact "
            "aggregate storage container and positive extent"
        )

    expected_row = {
        "ordinal": _cc_catalog.HUD_UI_MGR_DISABLE_TRACK_MARKER_CALL_ORDINAL,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": (
            _cc_catalog.HUD_UI_MGR_DISABLE_TRACK_MARKER_STORAGE_IDENTITY
        ),
        "slot_displacement": (
            _cc_catalog.HUD_UI_MGR_DISABLE_TRACK_MARKER_SLOT_DISPLACEMENT
        ),
        "cleanup_bytes": None,
    }
    if (
        len(expected) <= _cc_catalog.HUD_UI_MGR_DISABLE_TRACK_MARKER_CALL_ORDINAL
        or expected[_cc_catalog.HUD_UI_MGR_DISABLE_TRACK_MARKER_CALL_ORDINAL]
        != expected_row
        or sum(row == expected_row for row in expected) != 1
    ):
        raise ValueError(
            "HUD DisableHud track-marker loop bridge requires the exact "
            "immutable ordinal-1 retail virtual-slot contract"
        )

    base_offset = _cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_OFFSET
    call_offset = _cc_catalog.HUD_UI_MGR_DISABLE_TRACK_MARKER_CALL_OFFSET
    bounded_end = call_offset + 3
    exact_chain = bytes.fromhex(
        "be 44 0f 00 00 "
        "8b 86 bc 00 00 00 "
        "8d 8e bc 00 00 00 "
        "6a 00 ff 50 60"
    )
    if (
        caller.data[base_offset:bounded_end] != exact_chain
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        in caller.defined_external_data
    ):
        raise ValueError(
            "HUD DisableHud track-marker loop bridge requires the exact "
            "bounded loop-base/member/receiver/argument/call object body"
        )
    base_relocations = tuple(
        row
        for row in caller.relocations
        if base_offset <= row.offset < bounded_end
    )
    base_relocation = (
        base_relocations[0] if len(base_relocations) == 1 else None
    )
    if (
        base_relocation is None
        or base_relocation.offset
        != _cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_RELOCATION_OFFSET
        or base_relocation.type != IMAGE_REL_I386_DIR32
        or base_relocation.symbol_name
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        or struct.unpack_from(
            "<I",
            caller.data,
            base_relocation.offset,
        )[0]
        != _cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_DISPLACEMENT
    ):
        raise ValueError(
            "HUD DisableHud track-marker loop bridge rejects a missing, "
            "duplicate, extra, or malformed base DIR32 relocation"
        )
    expected_base_mask = set(
        range(
            _cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_RELOCATION_OFFSET,
            _cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_RELOCATION_OFFSET + 4,
        )
    )
    if {
        offset
        for offset in range(base_offset, bounded_end)
        if caller.relocation_mask[offset]
    } != expected_base_mask:
        raise ValueError(
            "HUD DisableHud track-marker loop bridge requires the exact "
            "bounded base relocation mask"
        )

    loop_tail_start = 0x23
    loop_tail_end = 0x3A
    exact_loop_tail = bytes.fromhex(
        "8b 16 8b ce 6a 00 ff 52 60 "
        "81 c6 c0 01 00 00 "
        "81 fe 44 47 00 00 7c d8"
    )
    if caller.data[loop_tail_start:loop_tail_end] != exact_loop_tail:
        raise ValueError(
            "HUD DisableHud track-marker loop bridge requires the exact "
            "second-slot/stride/bound/backedge object body"
        )
    bound_relocations = tuple(
        row
        for row in caller.relocations
        if loop_tail_start <= row.offset < loop_tail_end
    )
    bound_relocation = (
        bound_relocations[0] if len(bound_relocations) == 1 else None
    )
    if (
        bound_relocation is None
        or bound_relocation.offset != 0x34
        or bound_relocation.type != IMAGE_REL_I386_DIR32
        or bound_relocation.symbol_name
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        or struct.unpack_from("<I", caller.data, 0x34)[0] != 0x4744
        or {
            offset
            for offset in range(loop_tail_start, loop_tail_end)
            if caller.relocation_mask[offset]
        }
        != set(range(0x34, 0x38))
    ):
        raise ValueError(
            "HUD DisableHud track-marker loop bridge requires the exact "
            "aggregate loop-bound DIR32 relocation and mask"
        )

    instruction_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    index_by_offset = {
        offset: index
        for index, offset in enumerate(instruction_offsets)
        if offset is not None
    }
    chain_offsets = (0x0D, 0x12, 0x18, 0x1E, 0x20)
    tail_offsets = (0x23, 0x25, 0x27, 0x29, 0x2C, 0x32, 0x38)
    if set((*chain_offsets, *tail_offsets)) - set(index_by_offset):
        raise ValueError(
            "HUD DisableHud track-marker loop bridge requires exact candidate "
            "chain and loop-tail instruction offsets"
        )
    chain_indices = tuple(index_by_offset[offset] for offset in chain_offsets)
    if chain_indices != tuple(range(chain_indices[0], chain_indices[0] + 5)):
        raise ValueError(
            "HUD DisableHud track-marker loop bridge rejects a missing, "
            "duplicate, or noncontiguous candidate chain"
        )
    sequence = tuple(candidate.instructions[index] for index in chain_indices)
    base_expression = (
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
        f"+{_cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_DISPLACEMENT}"
    )
    base_hex_expression = (
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
        f"+0x{_cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_DISPLACEMENT:x}"
    )
    if (
        tuple(instruction.bytes for instruction in sequence)
        != (
            ("be", "44", "0f", "00", "00"),
            ("8b", "86", "bc", "00", "00", "00"),
            ("8d", "8e", "bc", "00", "00", "00"),
            ("6a", "00"),
            ("ff", "50", "60"),
        )
        or tuple(
            _cc_cfg._instruction_mnemonic(instruction)
            for instruction in sequence
        )
        != ("mov", "mov", "lea", "push", "call")
        or _cc_cfg._instruction_operand(sequence[0]).split(",", 1)[0]
        .strip()
        .lower()
        != "esi"
        or re.fullmatch(
            r"OFFSET\s+FLAT:(?P<expression>.+)",
            _cc_cfg._instruction_operand(sequence[0]).split(",", 1)[-1].strip(),
            flags=re.IGNORECASE,
        )
        is None
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(sequence[0]).split(":", 1)[-1]
        )
        not in {base_expression, base_hex_expression}
        or _cc_cfg._instruction_operand(sequence[1]).split(",", 1)[0]
        .strip()
        .lower()
        != "eax"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(sequence[1]).split(",", 1)[-1]
        )
        not in {"esi+188", "esi+0xbc"}
        or _cc_cfg._instruction_operand(sequence[2]).split(",", 1)[0]
        .strip()
        .lower()
        != "ecx"
        or _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(sequence[2]).split(",", 1)[-1]
        )
        not in {"esi+188", "esi+0xbc"}
        or _cc_cfg._instruction_operand(sequence[3]).strip().lower()
        not in {"0", "00", "0x0"}
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(sequence[4]))
        not in {"eax+96", "eax+0x60"}
    ):
        raise ValueError(
            "HUD DisableHud track-marker loop bridge rejects base/member/"
            "register/receiver/argument/slot/indexed/aliased/byte drift"
        )

    matching_bases = [
        index
        for index, instruction in enumerate(candidate.instructions)
        if (
            _cc_cfg._instruction_mnemonic(instruction) == "mov"
            and instruction.bytes == sequence[0].bytes
            and _cc_cfg._instruction_operand(instruction).split(",", 1)[0]
            .strip()
            .lower()
            == "esi"
            and _cc_targets._exact_memory_expression(
                _cc_cfg._instruction_operand(instruction).split(":", 1)[-1]
            )
            in {base_expression, base_hex_expression}
        )
    ]
    matching_members = [
        index
        for index, instruction in enumerate(candidate.instructions)
        if (
            _cc_cfg._instruction_mnemonic(instruction) == "mov"
            and instruction.bytes == sequence[1].bytes
            and _cc_cfg._instruction_operand(instruction).split(",", 1)[0]
            .strip()
            .lower()
            == "eax"
            and _cc_targets._exact_memory_expression(
                _cc_cfg._instruction_operand(instruction).split(",", 1)[-1]
            )
            in {"esi+188", "esi+0xbc"}
        )
    ]
    matching_receivers = [
        index
        for index, instruction in enumerate(candidate.instructions)
        if (
            _cc_cfg._instruction_mnemonic(instruction) == "lea"
            and instruction.bytes == sequence[2].bytes
            and _cc_cfg._instruction_operand(instruction).split(",", 1)[0]
            .strip()
            .lower()
            == "ecx"
            and _cc_targets._exact_memory_expression(
                _cc_cfg._instruction_operand(instruction).split(",", 1)[-1]
            )
            in {"esi+188", "esi+0xbc"}
        )
    ]
    matching_calls = [
        index
        for index, instruction in enumerate(candidate.instructions)
        if (
            _cc_cfg._instruction_mnemonic(instruction) == "call"
            and instruction.bytes == sequence[4].bytes
            and _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(instruction))
            in {"eax+96", "eax+0x60"}
        )
    ]
    matching_chain_starts = [
        index
        for index in range(len(candidate.instructions) - 4)
        if tuple(
            instruction.bytes
            for instruction in candidate.instructions[index : index + 5]
        )
        == tuple(instruction.bytes for instruction in sequence)
    ]
    if (
        matching_bases != [chain_indices[0]]
        or matching_members != [chain_indices[1]]
        or matching_receivers != [chain_indices[2]]
        or chain_indices[4] not in matching_calls
        or matching_chain_starts != [chain_indices[0]]
    ):
        raise ValueError(
            "HUD DisableHud track-marker loop bridge requires one unique "
            "exact ESI/EAX/ECX/slot-0x60 chain"
        )

    tail_indices = tuple(index_by_offset[offset] for offset in tail_offsets)
    if tail_indices != tuple(range(tail_indices[0], tail_indices[0] + 7)):
        raise ValueError(
            "HUD DisableHud track-marker loop bridge rejects a noncontiguous "
            "loop tail"
        )
    tail = tuple(candidate.instructions[index] for index in tail_indices)
    if (
        tuple(instruction.bytes for instruction in tail)
        != (
            ("8b", "16"),
            ("8b", "ce"),
            ("6a", "00"),
            ("ff", "52", "60"),
            ("81", "c6", "c0", "01", "00", "00"),
            ("81", "fe", "44", "47", "00", "00"),
            ("7c", "d8"),
        )
        or tuple(
            _cc_cfg._instruction_mnemonic(instruction) for instruction in tail
        )
        != ("mov", "mov", "push", "call", "add", "cmp", "jl")
        or 0x3A + struct.unpack("b", bytes([0xD8]))[0] != 0x12
        or any(
            _cc_cfg._instruction_mnemonic(instruction).startswith("j")
            for instruction in candidate.instructions[
                chain_indices[0] : tail_indices[-1]
            ]
        )
        or bool(
            candidate.local_control_flow_indices
            & frozenset(
                range(chain_indices[0], tail_indices[-1] + 1)
            )
        )
        or any(
            chain_indices[0] <= target <= tail_indices[-1]
            for targets in candidate.local_control_flow_targets.values()
            for target in targets
        )
    ):
        raise ValueError(
            "HUD DisableHud track-marker loop bridge rejects loop stride, "
            "bound, backedge, alternate-entry, or CFG ambiguity"
        )

    call_index = chain_indices[4]
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    if (
        len(invocation_indices)
        <= _cc_catalog.HUD_UI_MGR_DISABLE_TRACK_MARKER_CALL_ORDINAL
        or invocation_indices[
            _cc_catalog.HUD_UI_MGR_DISABLE_TRACK_MARKER_CALL_ORDINAL
        ]
        != call_index
        or _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None
    ):
        raise ValueError(
            "HUD DisableHud track-marker loop bridge requires the exact "
            "ordinal-1 call with no caller cleanup"
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
                    "HUD DisableHud track-marker loop bridge rejects "
                    "unexpected call-result consumption"
                )
        if _cc_cfg._instruction_may_clobber_register(instruction, "eax"):
            break

    return {
        normalize_address(hex(call_offset)): ReviewedLoopVptrStorageBridge(
            register="eax",
            storage_identity=(
                _cc_catalog.HUD_UI_MGR_DISABLE_TRACK_MARKER_STORAGE_IDENTITY
            ),
            slot_displacement=(
                _cc_catalog.HUD_UI_MGR_DISABLE_TRACK_MARKER_SLOT_DISPLACEMENT
            ),
            assembly_source="cod",
        )
    }


def _hud_ui_mgr_disable_slot_base_loop_vptr_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedLoopVptrStorageBridge]:
    """Bridge only DisableHud's second weapon-slot SetVisible invocation."""
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge
    caller_start = normalize_address(caller_start)
    if caller_start != _cc_catalog.HUD_UI_MGR_DISABLE_CALLER_START:
        return {}

    track_marker_bridges = (
        _hud_ui_mgr_disable_track_marker_loop_vptr_bridge(
            expected,
            candidate,
            document=document,
            caller_identity=caller_identity,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            indexes=indexes,
        )
    )
    exact_track_marker_bridge = {
        normalize_address(
            hex(_cc_catalog.HUD_UI_MGR_DISABLE_TRACK_MARKER_CALL_OFFSET)
        ): ReviewedLoopVptrStorageBridge(
            register="eax",
            storage_identity=(
                _cc_catalog.HUD_UI_MGR_DISABLE_TRACK_MARKER_STORAGE_IDENTITY
            ),
            slot_displacement=(
                _cc_catalog.HUD_UI_MGR_DISABLE_TRACK_MARKER_SLOT_DISPLACEMENT
            ),
            assembly_source="cod",
        )
    }
    if track_marker_bridges != exact_track_marker_bridge:
        raise ValueError(
            "HUD DisableHud slot-base loop bridge requires WSI-004's exact "
            "caller/source/aggregate/COFF/base/loop/CFG authority"
        )

    expected_row = {
        "ordinal": _cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_CALL_ORDINAL,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "virtual-slot",
        "target_identity": "",
        "storage_identity": _cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_STORAGE_IDENTITY,
        "slot_displacement": (
            _cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_SLOT_DISPLACEMENT
        ),
        "cleanup_bytes": None,
    }
    if (
        len(expected) <= _cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_CALL_ORDINAL
        or expected[_cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_CALL_ORDINAL]
        != expected_row
        or sum(row == expected_row for row in expected) != 1
    ):
        raise ValueError(
            "HUD DisableHud slot-base loop bridge requires the exact "
            "immutable ordinal-2 retail virtual-slot contract"
        )

    caller = candidate.caller_definition
    chain_start = _cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_VPTR_LOAD_OFFSET
    call_offset = _cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_CALL_OFFSET
    chain_end = call_offset + 3
    exact_chain = bytes.fromhex("8b 16 8b ce 6a 00 ff 52 60")
    if (
        caller is None
        or caller.data[chain_start:chain_end] != exact_chain
        or any(
            chain_start <= relocation.offset < chain_end
            for relocation in caller.relocations
        )
        or any(caller.relocation_mask[chain_start:chain_end])
    ):
        raise ValueError(
            "HUD DisableHud slot-base loop bridge requires the exact "
            "relocation-free second-slot object body and mask"
        )

    instruction_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    index_by_offset = {
        offset: index
        for index, offset in enumerate(instruction_offsets)
        if offset is not None
    }
    chain_offsets = (
        _cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_VPTR_LOAD_OFFSET,
        _cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_RECEIVER_OFFSET,
        _cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_ARGUMENT_OFFSET,
        _cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_CALL_OFFSET,
    )
    if set(chain_offsets) - set(index_by_offset):
        raise ValueError(
            "HUD DisableHud slot-base loop bridge requires exact candidate "
            "second-slot instruction offsets"
        )
    chain_indices = tuple(index_by_offset[offset] for offset in chain_offsets)
    if chain_indices != tuple(range(chain_indices[0], chain_indices[0] + 4)):
        raise ValueError(
            "HUD DisableHud slot-base loop bridge rejects a missing, "
            "duplicate, or noncontiguous second-slot chain"
        )
    sequence = tuple(candidate.instructions[index] for index in chain_indices)
    vptr_operands = [
        item.strip().lower()
        for item in _cc_cfg._instruction_operand(sequence[0]).split(",", 1)
    ]
    receiver_operands = [
        item.strip().lower()
        for item in _cc_cfg._instruction_operand(sequence[1]).split(",", 1)
    ]
    if (
        tuple(instruction.bytes for instruction in sequence)
        != (
            ("8b", "16"),
            ("8b", "ce"),
            ("6a", "00"),
            ("ff", "52", "60"),
        )
        or tuple(
            _cc_cfg._instruction_mnemonic(instruction)
            for instruction in sequence
        )
        != ("mov", "mov", "push", "call")
        or vptr_operands != ["edx", "dword [esi]"]
        or _cc_targets._exact_memory_expression(vptr_operands[1]) != "esi"
        or receiver_operands != ["ecx", "esi"]
        or _cc_cfg._instruction_operand(sequence[2]).strip().lower()
        not in {"0", "00", "0x0"}
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(sequence[3]))
        not in {"edx+96", "edx+0x60"}
    ):
        raise ValueError(
            "HUD DisableHud slot-base loop bridge rejects base/register/"
            "receiver/argument/slot/indexed/aliased/byte drift"
        )

    matching_chain_starts = [
        index
        for index in range(len(candidate.instructions) - 3)
        if tuple(
            instruction.bytes
            for instruction in candidate.instructions[index : index + 4]
        )
        == tuple(instruction.bytes for instruction in sequence)
    ]
    if matching_chain_starts != [chain_indices[0]]:
        raise ValueError(
            "HUD DisableHud slot-base loop bridge requires one unique exact "
            "EDX/ECX/false/slot-0x60 chain"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    call_index = chain_indices[3]
    if (
        len(invocation_indices) <= _cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_CALL_ORDINAL
        or invocation_indices[_cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_CALL_ORDINAL]
        != call_index
    ):
        raise ValueError(
            "HUD DisableHud slot-base loop bridge requires the exact "
            "ordinal-2 call"
        )
    previous_call_index = invocation_indices[
        _cc_catalog.HUD_UI_MGR_DISABLE_TRACK_MARKER_CALL_ORDINAL
    ]
    edx_definitions = [
        index
        for index in range(previous_call_index + 1, call_index)
        if _cc_cfg._instruction_may_clobber_register(
            candidate.instructions[index],
            "edx",
        )
    ]
    ecx_definitions = [
        index
        for index in range(previous_call_index + 1, call_index)
        if _cc_cfg._instruction_may_clobber_register(
            candidate.instructions[index],
            "ecx",
        )
    ]
    base_index = index_by_offset[_cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_OFFSET]
    esi_clobbers = [
        index
        for index in range(base_index + 1, call_index)
        if _cc_cfg._instruction_may_clobber_register(
            candidate.instructions[index],
            "esi",
        )
    ]
    if (
        edx_definitions != [chain_indices[0]]
        or ecx_definitions != [chain_indices[1]]
        or esi_clobbers
    ):
        raise ValueError(
            "HUD DisableHud slot-base loop bridge requires unique EDX/ECX "
            "reaching definitions and unbroken ESI loop-base provenance"
        )

    if _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None:
        raise ValueError(
            "HUD DisableHud slot-base loop bridge requires no caller cleanup"
        )
    loop_tail_index = index_by_offset[0x38]
    for instruction in candidate.instructions[
        call_index + 1 : loop_tail_index + 1
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
                    "HUD DisableHud slot-base loop bridge rejects unexpected "
                    "call-result consumption"
                )
        if _cc_cfg._instruction_may_clobber_register(instruction, "eax"):
            break

    return {
        normalize_address(hex(call_offset)): ReviewedLoopVptrStorageBridge(
            register="edx",
            storage_identity=_cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_STORAGE_IDENTITY,
            slot_displacement=(
                _cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_SLOT_DISPLACEMENT
            ),
            assembly_source="cod",
        )
    }


def _hud_ui_mgr_disable_set_enabled_candidate_vptr_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[
    dict[str, ReviewedAbsoluteStorageLoadBridge],
    dict[str, ReviewedVptrStorageBridge],
]:
    """Bridge only DisableHud's delayed aggregate SetEnabled(0) callback."""
    from _recoil.call_contract.records import (
        ReviewedAbsoluteStorageLoadBridge,
        ReviewedLoopVptrStorageBridge,
        ReviewedVptrStorageBridge,
    )
    caller_start = normalize_address(caller_start)
    if caller_start != _cc_catalog.HUD_UI_MGR_DISABLE_CALLER_START:
        return {}, {}

    slot_base_bridges = _hud_ui_mgr_disable_slot_base_loop_vptr_bridge(
        expected,
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    exact_slot_base_bridge = {
        normalize_address(
            hex(_cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_CALL_OFFSET)
        ): ReviewedLoopVptrStorageBridge(
            register="edx",
            storage_identity=_cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_STORAGE_IDENTITY,
            slot_displacement=(
                _cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_SLOT_DISPLACEMENT
            ),
            assembly_source="cod",
        )
    }
    if slot_base_bridges != exact_slot_base_bridge:
        raise ValueError(
            "HUD DisableHud SetEnabled bridge requires WSI-004/005's exact "
            "caller/source/aggregate/container/COFF/loop/CFG authority"
        )

    expected_row = {
        "ordinal": _cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_CALL_ORDINAL,
        "form": "call",
        "dispatch": "indirect",
        "identity_kind": "callback",
        "target_identity": "",
        "storage_identity": (
            _cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_STORAGE_IDENTITY
        ),
        "slot_displacement": (
            _cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_SLOT_DISPLACEMENT
        ),
        "cleanup_bytes": None,
    }
    if (
        len(expected) <= _cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_CALL_ORDINAL
        or expected[_cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_CALL_ORDINAL]
        != expected_row
        or sum(row == expected_row for row in expected) != 1
    ):
        raise ValueError(
            "HUD DisableHud SetEnabled bridge requires the exact immutable "
            "ordinal-3 retail callback contract"
        )

    caller = candidate.caller_definition
    sequence_start = _cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_LOAD_OFFSET
    call_offset = _cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_CALL_OFFSET
    sequence_end = call_offset + 3
    exact_body = bytes.fromhex(
        "a1 00 00 00 00 "
        "6a 00 "
        "b9 00 00 00 00 "
        "c7 05 f8 0e 00 00 00 00 00 00 "
        "c7 05 00 47 00 00 00 00 00 00 "
        "ff 50 04"
    )
    if (
        caller is None
        or caller.data[sequence_start:sequence_end] != exact_body
    ):
        raise ValueError(
            "HUD DisableHud SetEnabled bridge requires the exact bounded "
            "load/argument/receiver/two-store/call object body"
        )

    relocation_specs = {
        0x3B: 0,
        0x42: 0,
        0x48: 0xEF8,
        0x52: 0x4700,
    }
    bounded_relocations = tuple(
        row
        for row in caller.relocations
        if sequence_start <= row.offset < sequence_end
    )
    if (
        len(bounded_relocations) != len(relocation_specs)
        or {row.offset for row in bounded_relocations}
        != set(relocation_specs)
        or any(
            row.type != IMAGE_REL_I386_DIR32
            or row.symbol_name != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
            or struct.unpack_from("<I", caller.data, row.offset)[0]
            != relocation_specs[row.offset]
            for row in bounded_relocations
        )
    ):
        raise ValueError(
            "HUD DisableHud SetEnabled bridge rejects a missing, duplicate, "
            "extra, or malformed load/receiver/store DIR32 relocation"
        )
    expected_mask = {
        offset
        for relocation_offset in relocation_specs
        for offset in range(relocation_offset, relocation_offset + 4)
    }
    if {
        offset
        for offset in range(sequence_start, sequence_end)
        if caller.relocation_mask[offset]
    } != expected_mask:
        raise ValueError(
            "HUD DisableHud SetEnabled bridge requires the exact bounded "
            "four-DIR32 relocation mask"
        )

    instruction_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    index_by_offset = {
        offset: index
        for index, offset in enumerate(instruction_offsets)
        if offset is not None
    }
    sequence_offsets = (
        _cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_LOAD_OFFSET,
        _cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_ARGUMENT_OFFSET,
        _cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_RECEIVER_OFFSET,
        _cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_FIRST_STORE_OFFSET,
        _cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_SECOND_STORE_OFFSET,
        _cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_CALL_OFFSET,
    )
    if set(sequence_offsets) - set(index_by_offset):
        raise ValueError(
            "HUD DisableHud SetEnabled bridge requires exact delayed-call "
            "instruction offsets"
        )
    sequence_indices = tuple(
        index_by_offset[offset] for offset in sequence_offsets
    )
    if sequence_indices != tuple(
        range(sequence_indices[0], sequence_indices[0] + 6)
    ):
        raise ValueError(
            "HUD DisableHud SetEnabled bridge rejects a missing, duplicate, "
            "extra-stack, or noncontiguous delayed-call sequence"
        )
    sequence = tuple(
        candidate.instructions[index] for index in sequence_indices
    )
    load_operands = [
        item.strip().lower()
        for item in _cc_cfg._instruction_operand(sequence[0]).split(",", 1)
    ]
    store_operands = [
        [
            item.strip().lower()
            for item in _cc_cfg._instruction_operand(instruction).split(",", 1)
        ]
        for instruction in sequence[3:5]
    ]
    store_expressions = [
        _cc_targets._exact_memory_expression(operands[0])
        if len(operands) == 2
        else ""
        for operands in store_operands
    ]
    first_store_expressions = {
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}+3832".casefold(),
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}+0xef8".casefold(),
    }
    second_store_expressions = {
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}+18176".casefold(),
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}+0x4700".casefold(),
    }
    if (
        tuple(instruction.bytes for instruction in sequence)
        != (
            ("a1", "00", "00", "00", "00"),
            ("6a", "00"),
            ("b9", "00", "00", "00", "00"),
            ("c7", "05", "f8", "0e", "00", "00", "00", "00", "00", "00"),
            ("c7", "05", "00", "47", "00", "00", "00", "00", "00", "00"),
            ("ff", "50", "04"),
        )
        or tuple(
            _cc_cfg._instruction_mnemonic(instruction)
            for instruction in sequence
        )
        != ("mov", "push", "mov", "mov", "mov", "call")
        or len(load_operands) != 2
        or load_operands[0] != "eax"
        or _cc_targets._exact_memory_expression(load_operands[1]).casefold()
        != _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL.casefold()
        or _cc_cfg._instruction_operand(sequence[1]).strip().lower()
        not in {"0", "00", "0x0"}
        or re.fullmatch(
            r"ecx\s*,\s*OFFSET\s+FLAT:"
            + re.escape(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL),
            _cc_cfg._instruction_operand(sequence[2]).strip(),
            flags=re.IGNORECASE,
        )
        is None
        or store_expressions[0] not in first_store_expressions
        or store_expressions[1] not in second_store_expressions
        or store_expressions[0] == store_expressions[1]
        or any(
            len(operands) != 2
            or operands[1] not in {"0", "00", "0x0"}
            for operands in store_operands
        )
        or _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(sequence[5]))
        not in {"eax+4", "eax+0x4"}
    ):
        raise ValueError(
            "HUD DisableHud SetEnabled bridge rejects symbol/addend/register/"
            "receiver/argument/slot/store/byte drift"
        )

    matching_sequence_starts = [
        index
        for index in range(len(candidate.instructions) - 5)
        if tuple(
            instruction.bytes
            for instruction in candidate.instructions[index : index + 6]
        )
        == tuple(instruction.bytes for instruction in sequence)
    ]
    if matching_sequence_starts != [sequence_indices[0]]:
        raise ValueError(
            "HUD DisableHud SetEnabled bridge requires one unique exact "
            "delayed aggregate callback sequence"
        )

    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    call_index = sequence_indices[5]
    if (
        len(invocation_indices) <= _cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_CALL_ORDINAL
        or invocation_indices[
            _cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_CALL_ORDINAL
        ]
        != call_index
    ):
        raise ValueError(
            "HUD DisableHud SetEnabled bridge requires the exact candidate "
            "ordinal-3 callback"
        )
    previous_call_index = invocation_indices[
        _cc_catalog.HUD_UI_MGR_DISABLE_SLOT_BASE_CALL_ORDINAL
    ]
    eax_definitions = [
        index
        for index in range(previous_call_index + 1, call_index)
        if _cc_cfg._instruction_may_clobber_register(
            candidate.instructions[index],
            "eax",
        )
    ]
    ecx_definitions = [
        index
        for index in range(previous_call_index + 1, call_index)
        if _cc_cfg._instruction_may_clobber_register(
            candidate.instructions[index],
            "ecx",
        )
    ]
    false_arguments = [
        index
        for index in range(previous_call_index + 1, call_index)
        if _cc_cfg._instruction_mnemonic(candidate.instructions[index]) == "push"
    ]
    memory_store_indices = [
        index
        for index in range(previous_call_index + 1, call_index)
        if (
            _cc_cfg._instruction_mnemonic(candidate.instructions[index]) == "mov"
            and _cc_cfg._instruction_operand(candidate.instructions[index])
            .split(",", 1)[0]
            .strip()
            .lower()
            not in {
                "eax",
                "ebx",
                "ecx",
                "edx",
                "esi",
                "edi",
                "ebp",
                "esp",
            }
        )
    ]
    if (
        eax_definitions != [sequence_indices[0]]
        or ecx_definitions != [sequence_indices[2]]
        or false_arguments != [sequence_indices[1]]
        or memory_store_indices != list(sequence_indices[3:5])
        or tuple(sequence_indices[3:5])
        != tuple(range(sequence_indices[2] + 1, call_index))
    ):
        raise ValueError(
            "HUD DisableHud SetEnabled bridge requires unique EAX/ECX "
            "definitions, one false argument, and only two independent "
            "intervening stores"
        )

    bounded_indices = frozenset(sequence_indices)
    if (
        bool(candidate.local_control_flow_indices & bounded_indices)
        or any(
            target in bounded_indices
            for targets in candidate.local_control_flow_targets.values()
            for target in targets
        )
        or any(
            _cc_cfg._instruction_mnemonic(instruction).startswith("j")
            for instruction in sequence
        )
    ):
        raise ValueError(
            "HUD DisableHud SetEnabled bridge rejects control-flow ambiguity "
            "in the delayed call sequence"
        )
    if _cc_cfg._cleanup_after(candidate.instructions, call_index) is not None:
        raise ValueError(
            "HUD DisableHud SetEnabled bridge requires no caller cleanup"
        )
    next_invocation = (
        invocation_indices[
            _cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_CALL_ORDINAL + 1
        ]
        if len(invocation_indices)
        > _cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_CALL_ORDINAL + 1
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
                    "HUD DisableHud SetEnabled bridge rejects unexpected "
                    "call-result consumption"
                )
        if _cc_cfg._instruction_may_clobber_register(instruction, "eax"):
            break

    return (
        {
            normalize_address(hex(sequence_start)): (
                ReviewedAbsoluteStorageLoadBridge(
                    register="eax",
                    aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
                    displacement=0,
                    access_width=4,
                    storage_identity=(
                        _cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_STORAGE_IDENTITY
                    ),
                )
            )
        },
        {
            normalize_address(hex(call_offset)): ReviewedVptrStorageBridge(
                register="eax",
                provenance=(
                    _cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_LOAD_PROVENANCE
                ),
                storage_identity=(
                    _cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_STORAGE_IDENTITY
                ),
                slot_displacement=(
                    _cc_catalog.HUD_UI_MGR_DISABLE_SET_ENABLED_SLOT_DISPLACEMENT
                ),
                identity_kind="callback",
            )
        },
    )


def _hud_ui_aux_overlay_apply_text_line_authority(
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> str | None:
    """Fail closed on ApplyTextLineOp's exact tracker and source authority."""
    from _recoil.call_contract.records import StorageContainer
    caller_start = normalize_address(caller_start)
    if caller_start != _cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_CALLER_START:
        return None
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
        == _cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_CALLER_SYMBOL.casefold()
    ]
    if (
        caller_identity != _cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_CALLER_END_EXCLUSIVE
        or caller_addresses
        != [_cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_CALLER_START]
        or caller_identity in indexes.provider_ids
        or caller_name_rows
        not in (
            [],
            [
                (
                    _cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_CALLER_SYMBOL,
                    _cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_CALLER_IDENTITY,
                )
            ],
        )
        or caller is None
        or caller.symbol != _cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_CALLER_SYMBOL
        or len(caller.data) != 0x80
        or len(caller.relocation_mask) != len(caller.data)
    ):
        raise ValueError(
            "HUD ApplyTextLineOp bridge requires the exact reviewed authored "
            "caller identity, extent, symbol, and complete candidate body"
        )

    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_CALLER_START
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
            "HUD ApplyTextLineOp bridge requires one exact current HUD source "
            "authority"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "")
        != _cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_CALLER_SYMBOL
        or getattr(contribution_row, "symbol_regex", None) is not None
        or getattr(contribution_row, "name", "")
        != "HudUiAuxOverlay::UpdateTextLine"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD ApplyTextLineOp bridge requires its exact authored hud.cpp "
            "contribution row"
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
        or _cc_identity._symbol_identity(caller_symbol_id, caller_symbol) != caller_identity
        or caller_symbol.get("binary") != "recoil"
        or caller_symbol.get("kind") != "function"
        or caller_symbol.get("pipeline_class") != "authored"
        or caller_symbol.get("ownership_state") != "primary-owned"
        or caller_symbol.get("extent_state") != "known"
        or normalize_address(str(caller_symbol.get("address", "")))
        != _cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_CALLER_START
        or normalize_address(str(caller_symbol.get("end_exclusive", "")))
        != _cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_CALLER_END_EXCLUSIVE
        or caller_symbol.get("size") != 0xE0
        or caller_symbol.get("navigation_name")
        != "HudUiAuxOverlay::UpdateTextLine"
        or caller_symbol.get("output_section_id") != "recoil:section:.text"
        or caller_symbol.get("physical_block_id") != "recoil:block:0x404ca0"
        or not exact_required_target_membership(
            caller_symbol.get("verification_target_ids", ()),
            _cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_VERIFICATION_TARGET_IDS,
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
        != _cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_CALLER_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH}
    ):
        raise ValueError(
            "HUD ApplyTextLineOp bridge requires one exact unaliased reviewed "
            "caller and resolved source edge"
        )

    verification_targets = document.collection("verification_targets")
    symbols = document.collection("symbols")
    storage_rows = document.collection("storage_contributions")
    aggregate_symbol = symbols.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_SYMBOL_ID)
    aggregate_storage = storage_rows.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_STORAGE_ID)
    aggregate_target = verification_targets.get(_cc_catalog.HUD_UI_MGR_AGGREGATE_TARGET_ID)
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
        or _cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_DISPLACEMENT + 4
        > _cc_catalog.HUD_UI_MGR_AGGREGATE_SIZE
    ):
        raise ValueError(
            "HUD ApplyTextLineOp bridge requires the exact HUD aggregate "
            "string-menu field, storage, target, and container authority"
        )
    return aggregate_identity


def _hud_ui_aux_overlay_apply_text_line_retail_vptr_bridges(
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedLoopVptrStorageBridge]:
    """Prove retail's six EDX-indexed string-menu callsites independently."""
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge
    aggregate_identity = _hud_ui_aux_overlay_apply_text_line_authority(
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    if aggregate_identity is None:
        return {}
    retail_body = bytes.fromhex(
        "83 f9 01 56 75 41 8b c2 8b 0d b0 6d 4e 00 c1 e0 03 2b c2 "
        "8d 04 40 8d 34 c2 c1 e6 02 8b 54 31 20 8d 44 31 20 8b 4c "
        "24 08 51 50 ff 52 74 8b 15 b0 6d 4e 00 83 c4 08 8b 44 32 "
        "20 8d 4c 32 20 6a 01 ff 50 60 5e c2 04 00 85 c9 75 23 8b "
        "c2 6a 00 c1 e0 03 2b c2 8d 0c 40 a1 b0 6d 4e 00 8d 14 ca "
        "8d 4c 90 20 8b 54 90 20 ff 52 60 5e c2 04 00 83 f9 02 75 "
        "61 8b 4c 24 08 80 39 00 8b c2 74 38 c1 e0 03 2b c2 51 8d "
        "04 40 8d 34 c2 8b 15 b0 6d 4e 00 c1 e6 02 8d 44 32 20 50 "
        "8b 10 ff 52 74 a1 b0 6d 4e 00 83 c4 08 8b 54 30 20 8d 4c "
        "30 20 6a 01 ff 52 60 5e c2 04 00 c1 e0 03 2b c2 6a 00 8d "
        "04 40 8d 0c c2 8b 15 b0 6d 4e 00 8b 44 8a 20 8d 4c 8a 20 "
        "ff 50 60 5e c2 04 00"
    )
    addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(caller_start),
    )
    encoded = bytes(
        int(value, 16)
        for instruction in retail_instructions
        for value in instruction.bytes
    )
    calls = []
    for index, instruction in enumerate(retail_instructions):
        if _cc_cfg._instruction_mnemonic(instruction) != "call":
            continue
        expression = _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(instruction))
        match = re.fullmatch(r"([a-z]+)\+(0x[0-9a-f]+|[0-9]+)", expression)
        if match is None:
            raise ValueError(
                "HUD ApplyTextLineOp retail bridge rejects a non-exact call "
                "operand"
            )
        calls.append(
            (
                addresses[index],
                match.group(1),
                int(match.group(2), 0),
                _cc_cfg._cleanup_after(retail_instructions, index),
            )
        )
    branch_targets = tuple(
        (addresses[index], _cc_cfg._instruction_operand(instruction).strip().lower())
        for index, instruction in enumerate(retail_instructions)
        if _cc_cfg._instruction_mnemonic(instruction) in {"je", "jne"}
    )
    absolute_loads = tuple(
        addresses[index]
        for index, instruction in enumerate(retail_instructions)
        if _cc_cfg._instruction_mnemonic(instruction) == "mov"
        and _cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_RETAIL_STORAGE_ADDRESS
        in _cc_cfg._instruction_operand(instruction).replace(" ", "").lower()
    )
    if (
        not addresses
        or addresses[0] != 0x4137F0
        or encoded != retail_body
        or tuple(calls) != _cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_RETAIL_CALLS
        or branch_targets
        != (
            (0x4137F4, "0x413837"),
            (0x413839, "0x41385e"),
            (0x413861, "0x4138c4"),
            (0x41386C, "0x4138a6"),
        )
        or absolute_loads
        != (0x4137F8, 0x41381D, 0x413847, 0x41387A, 0x41388D, 0x4138B3)
    ):
        raise ValueError(
            "HUD ApplyTextLineOp bridge rejects immutable retail EDX-index "
            "algebra, string-menu storage, receiver/vptr definitions, call "
            "population/order/form/slots/cleanup, or branch topology drift"
        )
    return {
        normalize_address(hex(call_address)): ReviewedLoopVptrStorageBridge(
            register=register,
            storage_identity=_cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_STORAGE_IDENTITY,
            slot_displacement=slot,
            assembly_source="bn",
        )
        for call_address, register, slot, _ in calls
    }


def _hud_ui_aux_overlay_apply_text_line_candidate_bridges(
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
    dict[str, ReviewedLoopVptrStorageBridge],
]:
    """Prove candidate storage independently and bridge its six exact calls."""
    from _recoil.call_contract.records import (
        ReviewedLoopVptrStorageBridge,
        ReviewedStaticStorageReferenceBridge,
    )
    aggregate_identity = _hud_ui_aux_overlay_apply_text_line_authority(
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    if aggregate_identity is None:
        return {}, {}
    expected_rows = [
        {
            "ordinal": ordinal,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": _cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_STORAGE_IDENTITY,
            "slot_displacement": slot,
            "cleanup_bytes": cleanup,
        }
        for ordinal, (_, _, slot, cleanup) in enumerate(
            _cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_RETAIL_CALLS
        )
    ]
    if list(expected) != expected_rows:
        raise ValueError(
            "HUD ApplyTextLineOp bridge requires the exact immutable six-call "
            "retail contract"
        )
    caller = candidate.caller_definition
    assert caller is not None
    exact_body = bytes.fromhex(
        "8b c2 56 c1 e0 03 2b c2 83 f9 01 8d 04 40 8d 14 c2 a1 e0 "
        "0e 00 00 8d 74 90 20 75 1b 8b 54 24 08 8b 0e 52 56 ff 51 "
        "74 8b 06 83 c4 08 8b ce 6a 01 ff 50 60 5e c2 04 00 85 c9 "
        "75 0c 8b 16 51 8b ce ff 52 60 5e c2 04 00 83 f9 02 75 29 "
        "8b 44 24 08 80 38 00 74 17 8b 0e 50 56 ff 51 74 8b 16 83 "
        "c4 08 8b ce 6a 01 ff 52 60 5e c2 04 00 8b 06 6a 00 8b ce "
        "ff 50 60 5e c2 04 00 90 90 90 90 90 90 90"
    )
    expected_relocations = (
        (
            _cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_RELOCATION_OFFSET,
            IMAGE_REL_I386_DIR32,
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
            _cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_DISPLACEMENT,
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
    expected_mask = set(
        range(
            _cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_RELOCATION_OFFSET,
            _cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_RELOCATION_OFFSET + 4,
        )
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
        or caller.undefined_external_data.count(
            _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL
        )
        != 1
        or caller.defined_external_functions.count(
            _cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_CALLER_SYMBOL
        )
        != 1
        or (
            caller.defined_external_data
            + caller.undefined_external_functions
            + caller.defined_external_functions
        ).count(_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL)
        != 0
    ):
        raise ValueError(
            "HUD ApplyTextLineOp bridge requires the exact complete candidate "
            "body, aggregate COFF row/addend, relocation mask, padding, and "
            "external population"
        )

    decoded_body = bytes(
        int(value, 16)
        for instruction in candidate.instructions
        for value in instruction.bytes
    )
    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    calls = []
    for index, instruction in enumerate(candidate.instructions):
        if _cc_cfg._instruction_mnemonic(instruction) != "call":
            continue
        expression = _cc_targets._exact_memory_expression(_cc_cfg._instruction_operand(instruction))
        match = re.fullmatch(r"([a-z]+)\+(0x[0-9a-f]+|[0-9]+)", expression)
        if match is None:
            raise ValueError(
                "HUD ApplyTextLineOp candidate bridge rejects a non-exact "
                "call operand"
            )
        calls.append(
            (
                offsets[index],
                match.group(1),
                int(match.group(2), 0),
                _cc_cfg._cleanup_after(candidate.instructions, index),
            )
        )
    aggregate_expression = (
        f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}"
        f"+{_cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_DISPLACEMENT}"
    )
    aggregate_operands = tuple(
        _cc_targets._exact_memory_expression(
            _cc_cfg._instruction_operand(instruction).split(",", 1)[-1]
        )
        for instruction in candidate.instructions
        if _cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL.casefold()
        in _cc_cfg._instruction_operand(instruction).casefold()
    )
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    if (
        decoded_body != exact_body[:-7]
        or tuple(calls) != _cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_CANDIDATE_CALLS
        or aggregate_operands
        not in (
            (aggregate_expression,),
            (f"{_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL}+0xee0",),
        )
        or invocation_indices != (14, 19, 27, 38, 43, 49)
        or candidate.local_control_flow_indices != frozenset()
        or dict(candidate.local_control_flow_targets) != {}
    ):
        raise ValueError(
            "HUD ApplyTextLineOp bridge rejects candidate EDX-index algebra, "
            "string-menu storage, receiver/vptr definitions, call population/"
            "order/form/slots/cleanup, branch topology, or alternate entry"
        )
    static_bridge = ReviewedStaticStorageReferenceBridge(
        aggregate_symbol=_cc_catalog.HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL,
        displacement=_cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_DISPLACEMENT,
        access_width=4,
        storage_identity=aggregate_identity,
    )
    return (
        {aggregate_expression: static_bridge},
        {
            normalize_address(hex(call_offset)): ReviewedLoopVptrStorageBridge(
                register=register,
                storage_identity=(
                    _cc_catalog.HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_STORAGE_IDENTITY
                ),
                slot_displacement=slot,
                assembly_source="cod",
            )
            for call_offset, register, slot, _ in calls
        },
    )


def _hud_ui_mgr_enable_stacks_authority(
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[tuple[str, str, str], ...] | None:
    """Require the exact caller, Clear target, and two stack data owners."""
    caller_start = normalize_address(caller_start)
    if caller_start != _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CALLER_START:
        return None
    caller = candidate.caller_definition
    caller_addresses = sorted(
        address
        for address, identity in indexes.by_address.items()
        if identity == caller_identity
    )
    caller_name_rows = [
        (name, identity)
        for name, identity in indexes.by_candidate_name.items()
        if name.casefold() == _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CALLER_SYMBOL.casefold()
    ]
    if (
        caller_identity != _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CALLER_END_EXCLUSIVE
        or caller_addresses != [_cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CALLER_START]
        or caller_identity in indexes.provider_ids
        or caller_name_rows
        not in (
            [],
            [
                (
                    _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CALLER_SYMBOL,
                    _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CALLER_IDENTITY,
                )
            ],
        )
        or caller is None
        or caller.symbol != _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CALLER_SYMBOL
        or len(caller.data) != 0x40
        or len(caller.relocation_mask) != len(caller.data)
    ):
        raise ValueError(
            "HUD EnableTopAndChatStacks bridge requires the exact reviewed "
            "authored caller identity, extent, symbol, and candidate body"
        )

    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CALLER_START
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
            "HUD EnableTopAndChatStacks bridge requires one exact current "
            "HUD source authority"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") not in {None, ""}
        or getattr(contribution_row, "symbol_regex", None)
        != _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CALLER_SYMBOL_REGEX
        or getattr(contribution_row, "name", "")
        != "HudUiMgr::EnableTopAndChatStacks"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD EnableTopAndChatStacks bridge requires its exact authored "
            "hud.cpp contribution row"
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
        != _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CALLER_START
        or normalize_address(str(caller_symbol.get("end_exclusive", "")))
        != _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CALLER_END_EXCLUSIVE
        or caller_symbol.get("size") != 0x40
        or caller_symbol.get("navigation_name")
        != "HudUiMgr::EnableTopAndChatStacks"
        or caller_symbol.get("output_section_id") != "recoil:section:.text"
        or caller_symbol.get("physical_block_id") != "recoil:block:0x404ca0"
        or not exact_required_target_membership(
            caller_symbol.get("verification_target_ids", ()),
            _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_VERIFICATION_TARGET_IDS,
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
                "anchor_id": _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CALLER_ANCHOR_ID,
                "emission_context": {
                    "translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
                },
                "evidence_ids": [],
                "relation": "defines",
            }
        ]
    ):
        raise ValueError(
            "HUD EnableTopAndChatStacks bridge requires one exact unaliased "
            "reviewed caller and resolved source edge"
        )

    byte_target = targets.get(
        _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_VERIFICATION_TARGET_IDS[1]
    )
    byte_registration = (
        byte_target.get("registration")
        if isinstance(byte_target, Mapping)
        else None
    )
    if (
        not isinstance(byte_target, Mapping)
        or byte_target.get("binary") != "recoil"
        or byte_target.get("kind") != "vc5"
        or byte_target.get("name") != "hud_ui_text_stack_show_enable"
        or not _cc_targets._registered_function_population_matches(
            byte_target, ("recoil:function:0x4138d0", caller_symbol_id), symbols
        )
        or not isinstance(byte_registration, Mapping)
        or byte_registration.get("manifest_path")
        != "tools/vc5_verify_targets/hud_ui_text_stack_show_enable.json"
        or byte_registration.get("source_from")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or byte_registration.get("function_addresses")
        != ["0x4138d0", _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CALLER_START]
    ):
        raise ValueError(
            "HUD EnableTopAndChatStacks bridge requires its exact focused "
            "VC5 target authority"
        )

    clear_symbol_id = _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_IDENTITY.removeprefix(
        "symbol:"
    )
    clear_symbol = symbols.get(clear_symbol_id)
    clear_trace = (
        clear_symbol.get("source_traceability")
        if isinstance(clear_symbol, Mapping)
        else None
    )
    if (
        indexes.by_address.get(_cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_ADDRESS)
        != _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_IDENTITY
        or indexes.by_candidate_name.get(_cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_SYMBOL)
        not in {None, _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_IDENTITY}
        or _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_IDENTITY in indexes.provider_ids
        or not isinstance(clear_symbol, Mapping)
        or _cc_identity._symbol_identity(clear_symbol_id, clear_symbol)
        != _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_IDENTITY
        or clear_symbol.get("binary") != "recoil"
        or clear_symbol.get("kind") != "function"
        or clear_symbol.get("pipeline_class") != "authored"
        or clear_symbol.get("ownership_state") != "primary-owned"
        or clear_symbol.get("extent_state") != "known"
        or normalize_address(str(clear_symbol.get("address", "")))
        != _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_ADDRESS
        or normalize_address(str(clear_symbol.get("end_exclusive", "")))
        != "0x4bd2d0"
        or clear_symbol.get("size") != 0x30
        or clear_symbol.get("navigation_name") != "HudUiTextStack4::Clear"
        or not isinstance(clear_trace, Mapping)
        or clear_trace.get("state") != "resolved"
        or clear_trace.get("reason_code") not in {None, ""}
        or clear_trace.get("source_edges")
        != [
            {
                "anchor_id": (
                    "recoil:anchor:gamezrecoil-zui-zui-"
                    "huduitextstack4-clear"
                ),
                "emission_context": {
                    "translation_unit": "src/GameZRecoil/zUI/zui.cpp"
                },
                "evidence_ids": [],
                "relation": "defines",
            }
        ]
    ):
        raise ValueError(
            "HUD EnableTopAndChatStacks bridge requires the exact authored "
            "HudUiTextStack4::Clear target authority"
        )

    storage_rows = document.collection("storage_contributions")
    stack_authorities: list[tuple[str, str, str]] = []
    for spec in _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STACK_CALLS:
        (
            label,
            symbol_id,
            storage_id,
            address,
            navigation_name,
            _object_symbol,
            target_id,
            owner_id,
            anchor_id,
            *_offsets,
        ) = spec
        symbol = symbols.get(symbol_id)
        storage = storage_rows.get(storage_id)
        target_row = targets.get(target_id)
        registration = (
            target_row.get("registration")
            if isinstance(target_row, Mapping)
            else None
        )
        symbol_trace = (
            symbol.get("source_traceability")
            if isinstance(symbol, Mapping)
            else None
        )
        storage_identity = f"storage:{symbol_id}"
        if (
            not isinstance(symbol, Mapping)
            or _cc_identity._symbol_identity(symbol_id, symbol) != f"symbol:{symbol_id}"
            or symbol.get("binary") != "recoil"
            or symbol.get("kind") != "data"
            or symbol.get("disposition") != "authored"
            or symbol.get("extent_state") != "unknown"
            or normalize_address(str(symbol.get("address", ""))) != address
            or symbol.get("navigation_name") != navigation_name
            or symbol.get("output_section_id") != "recoil:section:.data"
            or symbol.get("storage_contribution_ids") != [storage_id]
            or symbol.get("verification_target_ids") != [target_id]
            or not isinstance(symbol_trace, Mapping)
            or symbol_trace.get("state") != "resolved"
            or symbol_trace.get("reason_code") not in {None, ""}
            or symbol_trace.get("source_edges")
            != [
                {
                    "anchor_id": anchor_id,
                    "emission_context": {
                        "translation_unit": (
                            "src/GameZRecoil/zUI/zui_widgets.cpp"
                        )
                    },
                    "evidence_ids": [],
                    "relation": "defines",
                }
            ]
            or not isinstance(storage, Mapping)
            or storage.get("binary") != "recoil"
            or storage.get("kind") != "data-symbol"
            or storage.get("output_section_id") != "recoil:section:.data"
            or storage.get("overlap") != "none"
            or storage.get("parent_contribution_id") is not None
            or storage.get("owner_ids") != [owner_id]
            or storage.get("symbol_ids") != [symbol_id]
            or storage.get("reference")
            != {
                "address": address,
                "evidence_ids": [],
                "extent_state": "unknown",
            }
            or not isinstance(target_row, Mapping)
            or target_row.get("binary") != "recoil"
            or target_row.get("kind") != "vc5"
            or target_row.get("name")
            != target_id.removeprefix("recoil:vc5-target:")
            or target_row.get("symbol_ids") != [symbol_id]
            or target_row.get("unresolved_addresses") != []
            or not isinstance(registration, Mapping)
            or registration.get("manifest_path")
            != (
                "tools/vc5_verify_targets/"
                f"{target_id.removeprefix('recoil:vc5-target:')}.json"
            )
            or registration.get("source_from")
            != "src/GameZRecoil/zUI/zui.cpp"
            or registration.get("data_addresses") != [address]
            or indexes.storage_by_address.get(address) != storage_identity
            or indexes.storage_by_name.get(navigation_name)
            != storage_identity
        ):
            raise ValueError(
                "HUD EnableTopAndChatStacks bridge requires the exact typed "
                f"{label} stack symbol/storage/source/target authority"
            )
        stack_authorities.append(
            (symbol_id, address, f"load({storage_identity})")
        )
    return tuple(stack_authorities)


def _hud_ui_mgr_enable_stacks_retail_vptr_bridges(
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedLoopVptrStorageBridge]:
    """Validate retail's complete top-then-chat call unit independently."""
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge
    stack_authorities = _hud_ui_mgr_enable_stacks_authority(
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    if stack_authorities is None:
        return {}
    exact_body = bytes.fromhex(
        "8b 0d 24 bd 56 00 e8 85 99 0a 00 8b 0d 24 bd 56 00 "
        "6a 01 8b 01 ff 50 04 8b 0d 20 bd 56 00 e8 6d 99 0a "
        "00 8b 0d 20 bd 56 00 6a 01 8b 11 ff 52 04 c3"
    )
    addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(caller_start),
    )
    encoded = bytes(
        int(value, 16)
        for instruction in retail_instructions
        for value in instruction.bytes
    )
    call_rows = tuple(
        (
            addresses[index],
            _cc_cfg._instruction_operand(instruction).strip(),
            _cc_cfg._cleanup_after(retail_instructions, index),
        )
        for index, instruction in enumerate(retail_instructions)
        if _cc_cfg._instruction_mnemonic(instruction) == "call"
    )
    memory_sources = tuple(
        (
            addresses[index],
            _cc_targets._exact_memory_expression(
                _cc_cfg._instruction_operand(instruction).split(",", 1)[-1]
            ),
        )
        for index, instruction in enumerate(retail_instructions)
        if _cc_cfg._instruction_mnemonic(instruction) == "mov"
    )
    if (
        addresses
        != (
            0x413910, 0x413916, 0x41391B, 0x413921, 0x413923,
            0x413925, 0x413928, 0x41392E, 0x413933, 0x413939,
            0x41393B, 0x41393D, 0x413940,
        )
        or encoded != exact_body
        or tuple(_cc_cfg._instruction_mnemonic(row) for row in retail_instructions)
        != (
            "mov", "call", "mov", "push", "mov", "call", "mov",
            "call", "mov", "push", "mov", "call", "retn",
        )
        or call_rows
        != (
            (0x413916, "HudUiTextStack4::Clear", None),
            (0x413925, "dword [eax+0x4]", None),
            (0x41392E, "HudUiTextStack4::Clear", None),
            (0x41393D, "dword [edx+0x4]", None),
        )
        or memory_sources
        != (
            (0x413910, "0x56bd24"),
            (0x41391B, "0x56bd24"),
            (0x413923, "ecx"),
            (0x413928, "0x56bd20"),
            (0x413933, "0x56bd20"),
            (0x41393B, "ecx"),
        )
        or _cc_cfg._instruction_operand(retail_instructions[3]).strip().lower()
        not in {"1", "0x1"}
        or _cc_cfg._instruction_operand(retail_instructions[9]).strip().lower()
        not in {"1", "0x1"}
        or _cc_cfg._instruction_operand(retail_instructions[12]).strip() not in {"", "0"}
    ):
        raise ValueError(
            "HUD EnableTopAndChatStacks bridge rejects immutable retail "
            "top/chat storage, receiver/vptr reaching definitions, Clear/"
            "SetEnabled call population/order/form/slot/cleanup, return, or "
            "complete topology drift"
        )
    storage_by_symbol = {
        symbol_id: storage_identity
        for symbol_id, _address, storage_identity in stack_authorities
    }
    return {
        normalize_address(hex(call_address)): ReviewedLoopVptrStorageBridge(
            register=register,
            storage_identity=storage_by_symbol[symbol_id],
            slot_displacement=0x04,
            assembly_source="bn",
        )
        for call_address, register, symbol_id
        in _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_RETAIL_VIRTUAL_CALLS
    }


def _hud_ui_mgr_enable_stacks_candidate_vptr_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedLoopVptrStorageBridge]:
    """Validate candidate independently and bridge its EAX/EDX vptr loads."""
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge
    stack_authorities = _hud_ui_mgr_enable_stacks_authority(
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    if stack_authorities is None:
        return {}
    storage_by_symbol = {
        symbol_id: storage_identity
        for symbol_id, _address, storage_identity in stack_authorities
    }
    expected_rows = [
        {
            "ordinal": 0,
            "form": "call",
            "dispatch": "direct",
            "identity_kind": "direct",
            "target_identity": _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_IDENTITY,
            "storage_identity": "",
            "slot_displacement": None,
            "cleanup_bytes": None,
        },
        {
            "ordinal": 1,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": storage_by_symbol["recoil:data:0x56bd24"],
            "slot_displacement": 0x04,
            "cleanup_bytes": None,
        },
        {
            "ordinal": 2,
            "form": "call",
            "dispatch": "direct",
            "identity_kind": "direct",
            "target_identity": _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_IDENTITY,
            "storage_identity": "",
            "slot_displacement": None,
            "cleanup_bytes": None,
        },
        {
            "ordinal": 3,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": storage_by_symbol["recoil:data:0x56bd20"],
            "slot_displacement": 0x04,
            "cleanup_bytes": None,
        },
    ]
    if list(expected) != expected_rows:
        raise ValueError(
            "HUD EnableTopAndChatStacks bridge requires the exact immutable "
            "four-call retail contract"
        )
    caller = candidate.caller_definition
    assert caller is not None
    exact_body = bytes.fromhex(
        "8b 0d 00 00 00 00 e8 00 00 00 00 8b 0d 00 00 00 00 "
        "6a 01 8b 01 ff 50 04 8b 0d 00 00 00 00 e8 00 00 00 "
        "00 8b 0d 00 00 00 00 6a 01 8b 11 ff 52 04 c3 90 90 "
        "90 90 90 90 90 90 90 90 90 90 90 90 90"
    )
    top_symbol = _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STACK_CALLS[0][5]
    chat_symbol = _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STACK_CALLS[1][5]
    expected_relocations = (
        (0x02, IMAGE_REL_I386_DIR32, top_symbol, 0),
        (0x07, IMAGE_REL_I386_REL32, _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_SYMBOL, 0),
        (0x0D, IMAGE_REL_I386_DIR32, top_symbol, 0),
        (0x1A, IMAGE_REL_I386_DIR32, chat_symbol, 0),
        (0x1F, IMAGE_REL_I386_REL32, _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_SYMBOL, 0),
        (0x25, IMAGE_REL_I386_DIR32, chat_symbol, 0),
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
    expected_mask = {
        index
        for offset, _kind, _symbol, _addend in expected_relocations
        for index in range(offset, offset + 4)
    }
    if (
        caller.data != exact_body
        or observed_relocations != expected_relocations
        or {
            index
            for index, masked in enumerate(caller.relocation_mask)
            if masked
        }
        != expected_mask
        or caller.undefined_external_functions.count(
            _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_SYMBOL
        )
        != 1
        or caller.undefined_external_data.count(top_symbol) != 1
        or caller.undefined_external_data.count(chat_symbol) != 1
        or caller.defined_external_functions.count(
            _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CALLER_SYMBOL
        )
        != 1
        or any(
            symbol in caller.defined_external_data
            for symbol in (top_symbol, chat_symbol)
        )
        or _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_SYMBOL
        in caller.defined_external_functions
    ):
        raise ValueError(
            "HUD EnableTopAndChatStacks bridge requires the exact complete "
            "candidate body, six COFF rows/addends/order, relocation mask, "
            "padding, and relevant external population"
        )

    offsets = list(_cc_callable_identity._candidate_complete_instruction_offsets(candidate))
    if (
        offsets
        and offsets[0] is None
        and len(offsets) > 1
        and offsets[1] == len(candidate.instructions[0].bytes)
    ):
        offsets[0] = 0
    encoded = bytes(
        int(value, 16)
        for instruction in candidate.instructions
        for value in instruction.bytes
    )
    call_rows = tuple(
        (
            offsets[index],
            _cc_cfg._instruction_operand(instruction).strip(),
            _cc_cfg._cleanup_after(candidate.instructions, index),
        )
        for index, instruction in enumerate(candidate.instructions)
        if _cc_cfg._instruction_mnemonic(instruction) == "call"
    )
    memory_sources = tuple(
        (
            offsets[index],
            _cc_targets._exact_memory_expression(
                _cc_cfg._instruction_operand(instruction).split(",", 1)[-1]
            ),
        )
        for index, instruction in enumerate(candidate.instructions)
        if _cc_cfg._instruction_mnemonic(instruction) == "mov"
    )
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    if (
        tuple(offsets)
        != (0x00, 0x06, 0x0B, 0x11, 0x13, 0x15, 0x18,
            0x1E, 0x23, 0x29, 0x2B, 0x2D, 0x30)
        or encoded != exact_body[:-15]
        or tuple(_cc_cfg._instruction_mnemonic(row) for row in candidate.instructions)
        != (
            "mov", "call", "mov", "push", "mov", "call", "mov",
            "call", "mov", "push", "mov", "call", "ret",
        )
        or call_rows
        != (
            (0x06, _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_SYMBOL, None),
            (0x15, "dword [eax+4]", None),
            (0x1E, _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_SYMBOL, None),
            (0x2D, "dword [edx+4]", None),
        )
        or memory_sources
        != (
            (0x00, top_symbol),
            (0x0B, top_symbol),
            (0x13, "ecx"),
            (0x18, chat_symbol),
            (0x23, chat_symbol),
            (0x2B, "ecx"),
        )
        or _cc_cfg._instruction_operand(candidate.instructions[3]).strip().lower()
        not in {"1", "0x1"}
        or _cc_cfg._instruction_operand(candidate.instructions[9]).strip().lower()
        not in {"1", "0x1"}
        or _cc_cfg._instruction_operand(candidate.instructions[12]).strip() not in {"", "0"}
        or invocation_indices != (1, 5, 7, 11)
        or candidate.local_control_flow_indices != frozenset()
        or dict(candidate.local_control_flow_targets) != {}
    ):
        raise ValueError(
            "HUD EnableTopAndChatStacks bridge rejects candidate top/chat "
            "storage, receiver/vptr reaching definitions, Clear/SetEnabled "
            "call population/order/form/slot/cleanup, return, padding, or "
            "complete topology drift"
        )
    return {
        normalize_address(hex(call_offset)): ReviewedLoopVptrStorageBridge(
            register=register,
            storage_identity=storage_by_symbol[symbol_id],
            slot_displacement=0x04,
            assembly_source="cod",
        )
        for call_offset, register, symbol_id
        in _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CANDIDATE_VIRTUAL_CALLS
    }


def _hud_ui_mgr_disable_stacks_authority(
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> tuple[tuple[str, str, str], ...] | None:
    """Require DisableTopAndChatStacks and its independently typed inputs."""
    caller_start = normalize_address(caller_start)
    if caller_start != _cc_catalog.HUD_UI_MGR_DISABLE_STACKS_CALLER_START:
        return None
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
        == _cc_catalog.HUD_UI_MGR_DISABLE_STACKS_CALLER_SYMBOL.casefold()
    ]
    if (
        caller_identity != _cc_catalog.HUD_UI_MGR_DISABLE_STACKS_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.HUD_UI_MGR_DISABLE_STACKS_CALLER_END_EXCLUSIVE
        or caller_addresses != [_cc_catalog.HUD_UI_MGR_DISABLE_STACKS_CALLER_START]
        or caller_identity in indexes.provider_ids
        or caller_name_rows
        not in (
            [],
            [
                (
                    _cc_catalog.HUD_UI_MGR_DISABLE_STACKS_CALLER_SYMBOL,
                    _cc_catalog.HUD_UI_MGR_DISABLE_STACKS_CALLER_IDENTITY,
                )
            ],
        )
        or caller is None
        or caller.symbol != _cc_catalog.HUD_UI_MGR_DISABLE_STACKS_CALLER_SYMBOL
        or len(caller.data) != 0x40
        or len(caller.relocation_mask) != len(caller.data)
    ):
        raise ValueError(
            "HUD DisableTopAndChatStacks bridge requires the exact reviewed "
            "authored caller identity, extent, symbol, and candidate body"
        )

    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_DISABLE_STACKS_CALLER_START
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
            "HUD DisableTopAndChatStacks bridge requires one exact current "
            "HUD source authority"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "")
        != _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") not in {None, ""}
        or getattr(contribution_row, "symbol_regex", None)
        != _cc_catalog.HUD_UI_MGR_DISABLE_STACKS_CALLER_SYMBOL_REGEX
        or getattr(contribution_row, "name", "")
        != "HudUiMgr::DisableTopAndChatStacks"
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "HUD DisableTopAndChatStacks bridge requires its exact authored "
            "hud.cpp contribution row"
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
        != _cc_catalog.HUD_UI_MGR_DISABLE_STACKS_CALLER_START
        or normalize_address(str(caller_symbol.get("end_exclusive", "")))
        != _cc_catalog.HUD_UI_MGR_DISABLE_STACKS_CALLER_END_EXCLUSIVE
        or caller_symbol.get("size") != 0x40
        or caller_symbol.get("navigation_name")
        != "HudUiMgr::DisableTopAndChatStacks"
        or caller_symbol.get("output_section_id") != "recoil:section:.text"
        or caller_symbol.get("physical_block_id") != "recoil:block:0x404ca0"
        or not exact_required_target_membership(
            caller_symbol.get("verification_target_ids", ()),
            _cc_catalog.HUD_UI_MGR_DISABLE_STACKS_VERIFICATION_TARGET_IDS,
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
                "anchor_id": _cc_catalog.HUD_UI_MGR_DISABLE_STACKS_CALLER_ANCHOR_ID,
                "emission_context": {
                    "translation_unit": _cc_catalog.HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH
                },
                "evidence_ids": [],
                "relation": "defines",
            }
        ]
    ):
        raise ValueError(
            "HUD DisableTopAndChatStacks bridge requires one exact unaliased "
            "reviewed caller and resolved source edge"
        )

    clear_symbol_id = _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_IDENTITY.removeprefix(
        "symbol:"
    )
    clear_symbol = symbols.get(clear_symbol_id)
    clear_trace = (
        clear_symbol.get("source_traceability")
        if isinstance(clear_symbol, Mapping)
        else None
    )
    if (
        indexes.by_address.get(_cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_ADDRESS)
        != _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_IDENTITY
        or indexes.by_candidate_name.get(_cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_SYMBOL)
        not in {None, _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_IDENTITY}
        or _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_IDENTITY in indexes.provider_ids
        or not isinstance(clear_symbol, Mapping)
        or _cc_identity._symbol_identity(clear_symbol_id, clear_symbol)
        != _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_IDENTITY
        or clear_symbol.get("binary") != "recoil"
        or clear_symbol.get("kind") != "function"
        or clear_symbol.get("pipeline_class") != "authored"
        or clear_symbol.get("ownership_state") != "primary-owned"
        or clear_symbol.get("extent_state") != "known"
        or normalize_address(str(clear_symbol.get("address", "")))
        != _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_ADDRESS
        or normalize_address(str(clear_symbol.get("end_exclusive", "")))
        != "0x4bd2d0"
        or clear_symbol.get("size") != 0x30
        or clear_symbol.get("navigation_name") != "HudUiTextStack4::Clear"
        or not isinstance(clear_trace, Mapping)
        or clear_trace.get("state") != "resolved"
        or clear_trace.get("reason_code") not in {None, ""}
        or clear_trace.get("source_edges")
        != [
            {
                "anchor_id": (
                    "recoil:anchor:gamezrecoil-zui-zui-"
                    "huduitextstack4-clear"
                ),
                "emission_context": {
                    "translation_unit": "src/GameZRecoil/zUI/zui.cpp"
                },
                "evidence_ids": [],
                "relation": "defines",
            }
        ]
    ):
        raise ValueError(
            "HUD DisableTopAndChatStacks bridge requires the exact authored "
            "HudUiTextStack4::Clear target authority"
        )

    storage_rows = document.collection("storage_contributions")
    stack_authorities: list[tuple[str, str, str]] = []
    for spec in _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STACK_CALLS:
        (
            label,
            symbol_id,
            storage_id,
            address,
            navigation_name,
            _object_symbol,
            target_id,
            owner_id,
            anchor_id,
            *_offsets,
        ) = spec
        symbol = symbols.get(symbol_id)
        storage = storage_rows.get(storage_id)
        target_row = targets.get(target_id)
        registration = (
            target_row.get("registration")
            if isinstance(target_row, Mapping)
            else None
        )
        symbol_trace = (
            symbol.get("source_traceability")
            if isinstance(symbol, Mapping)
            else None
        )
        storage_identity = f"storage:{symbol_id}"
        if (
            not isinstance(symbol, Mapping)
            or _cc_identity._symbol_identity(symbol_id, symbol) != f"symbol:{symbol_id}"
            or symbol.get("binary") != "recoil"
            or symbol.get("kind") != "data"
            or symbol.get("disposition") != "authored"
            or symbol.get("extent_state") != "unknown"
            or normalize_address(str(symbol.get("address", ""))) != address
            or symbol.get("navigation_name") != navigation_name
            or symbol.get("output_section_id") != "recoil:section:.data"
            or symbol.get("storage_contribution_ids") != [storage_id]
            or symbol.get("verification_target_ids") != [target_id]
            or not isinstance(symbol_trace, Mapping)
            or symbol_trace.get("state") != "resolved"
            or symbol_trace.get("reason_code") not in {None, ""}
            or symbol_trace.get("source_edges")
            != [
                {
                    "anchor_id": anchor_id,
                    "emission_context": {
                        "translation_unit": (
                            "src/GameZRecoil/zUI/zui_widgets.cpp"
                        )
                    },
                    "evidence_ids": [],
                    "relation": "defines",
                }
            ]
            or not isinstance(storage, Mapping)
            or storage.get("binary") != "recoil"
            or storage.get("kind") != "data-symbol"
            or storage.get("output_section_id") != "recoil:section:.data"
            or storage.get("overlap") != "none"
            or storage.get("parent_contribution_id") is not None
            or storage.get("owner_ids") != [owner_id]
            or storage.get("symbol_ids") != [symbol_id]
            or storage.get("reference")
            != {
                "address": address,
                "evidence_ids": [],
                "extent_state": "unknown",
            }
            or not isinstance(target_row, Mapping)
            or target_row.get("binary") != "recoil"
            or target_row.get("kind") != "vc5"
            or target_row.get("name")
            != target_id.removeprefix("recoil:vc5-target:")
            or target_row.get("symbol_ids") != [symbol_id]
            or target_row.get("unresolved_addresses") != []
            or not isinstance(registration, Mapping)
            or registration.get("manifest_path")
            != (
                "tools/vc5_verify_targets/"
                f"{target_id.removeprefix('recoil:vc5-target:')}.json"
            )
            or registration.get("source_from")
            != "src/GameZRecoil/zUI/zui.cpp"
            or registration.get("data_addresses") != [address]
            or indexes.storage_by_address.get(address) != storage_identity
            or indexes.storage_by_name.get(navigation_name)
            != storage_identity
        ):
            raise ValueError(
                "HUD DisableTopAndChatStacks bridge requires the exact typed "
                f"{label} stack symbol/storage/source/target authority"
            )
        stack_authorities.append(
            (symbol_id, address, f"load({storage_identity})")
        )
    return tuple(stack_authorities)


def _hud_ui_mgr_disable_stacks_retail_vptr_bridges(
    retail_instructions: Sequence[Instruction],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedLoopVptrStorageBridge]:
    """Validate retail's independent clear-and-hide top/chat unit."""
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge
    stack_authorities = _hud_ui_mgr_disable_stacks_authority(
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    if stack_authorities is None:
        return {}
    exact_body = bytes.fromhex(
        "8b 0d 24 bd 56 00 e8 45 99 0a 00 8b 0d 24 bd 56 00 "
        "6a 00 8b 01 ff 50 04 8b 0d 20 bd 56 00 e8 2d 99 0a "
        "00 8b 0d 20 bd 56 00 6a 00 8b 11 ff 52 04 c3"
    )
    addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(caller_start),
    )
    encoded = bytes(
        int(value, 16)
        for instruction in retail_instructions
        for value in instruction.bytes
    )
    call_rows = tuple(
        (
            addresses[index],
            _cc_cfg._instruction_operand(instruction).strip(),
            _cc_cfg._cleanup_after(retail_instructions, index),
        )
        for index, instruction in enumerate(retail_instructions)
        if _cc_cfg._instruction_mnemonic(instruction) == "call"
    )
    memory_sources = tuple(
        (
            addresses[index],
            _cc_targets._exact_memory_expression(
                _cc_cfg._instruction_operand(instruction).split(",", 1)[-1]
            ),
        )
        for index, instruction in enumerate(retail_instructions)
        if _cc_cfg._instruction_mnemonic(instruction) == "mov"
    )
    if (
        addresses
        != (
            0x413950, 0x413956, 0x41395B, 0x413961, 0x413963,
            0x413965, 0x413968, 0x41396E, 0x413973, 0x413979,
            0x41397B, 0x41397D, 0x413980,
        )
        or encoded != exact_body
        or tuple(_cc_cfg._instruction_mnemonic(row) for row in retail_instructions)
        != (
            "mov", "call", "mov", "push", "mov", "call", "mov",
            "call", "mov", "push", "mov", "call", "retn",
        )
        or call_rows
        != (
            (0x413956, "HudUiTextStack4::Clear", None),
            (0x413965, "dword [eax+0x4]", None),
            (0x41396E, "HudUiTextStack4::Clear", None),
            (0x41397D, "dword [edx+0x4]", None),
        )
        or memory_sources
        != (
            (0x413950, "0x56bd24"),
            (0x41395B, "0x56bd24"),
            (0x413963, "ecx"),
            (0x413968, "0x56bd20"),
            (0x413973, "0x56bd20"),
            (0x41397B, "ecx"),
        )
        or _cc_cfg._instruction_operand(retail_instructions[3]).strip().lower()
        not in {"0", "0x0"}
        or _cc_cfg._instruction_operand(retail_instructions[9]).strip().lower()
        not in {"0", "0x0"}
        or _cc_cfg._instruction_operand(retail_instructions[12]).strip() not in {"", "0"}
    ):
        raise ValueError(
            "HUD DisableTopAndChatStacks bridge rejects immutable retail "
            "top/chat storage, receiver/vptr reaching definitions, Clear/"
            "SetEnabled call population/order/form/slot/cleanup, false "
            "arguments, return, or complete topology drift"
        )
    storage_by_symbol = {
        symbol_id: storage_identity
        for symbol_id, _address, storage_identity in stack_authorities
    }
    return {
        normalize_address(hex(call_address)): ReviewedLoopVptrStorageBridge(
            register=register,
            storage_identity=storage_by_symbol[symbol_id],
            slot_displacement=0x04,
            assembly_source="bn",
        )
        for call_address, register, symbol_id
        in _cc_catalog.HUD_UI_MGR_DISABLE_STACKS_RETAIL_VIRTUAL_CALLS
    }


def _hud_ui_mgr_disable_stacks_candidate_vptr_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, ReviewedLoopVptrStorageBridge]:
    """Validate candidate's separate clear-and-hide four-call unit."""
    from _recoil.call_contract.records import ReviewedLoopVptrStorageBridge
    stack_authorities = _hud_ui_mgr_disable_stacks_authority(
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
    )
    if stack_authorities is None:
        return {}
    storage_by_symbol = {
        symbol_id: storage_identity
        for symbol_id, _address, storage_identity in stack_authorities
    }
    expected_rows = [
        {
            "ordinal": 0,
            "form": "call",
            "dispatch": "direct",
            "identity_kind": "direct",
            "target_identity": _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_IDENTITY,
            "storage_identity": "",
            "slot_displacement": None,
            "cleanup_bytes": None,
        },
        {
            "ordinal": 1,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": storage_by_symbol["recoil:data:0x56bd24"],
            "slot_displacement": 0x04,
            "cleanup_bytes": None,
        },
        {
            "ordinal": 2,
            "form": "call",
            "dispatch": "direct",
            "identity_kind": "direct",
            "target_identity": _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_IDENTITY,
            "storage_identity": "",
            "slot_displacement": None,
            "cleanup_bytes": None,
        },
        {
            "ordinal": 3,
            "form": "call",
            "dispatch": "indirect",
            "identity_kind": "virtual-slot",
            "target_identity": "",
            "storage_identity": storage_by_symbol["recoil:data:0x56bd20"],
            "slot_displacement": 0x04,
            "cleanup_bytes": None,
        },
    ]
    if list(expected) != expected_rows:
        raise ValueError(
            "HUD DisableTopAndChatStacks bridge requires the exact immutable "
            "four-call retail contract"
        )
    caller = candidate.caller_definition
    assert caller is not None
    exact_body = bytes.fromhex(
        "8b 0d 00 00 00 00 e8 00 00 00 00 8b 0d 00 00 00 00 "
        "6a 00 8b 01 ff 50 04 8b 0d 00 00 00 00 e8 00 00 00 "
        "00 8b 0d 00 00 00 00 6a 00 8b 11 ff 52 04 c3 90 90 "
        "90 90 90 90 90 90 90 90 90 90 90 90 90"
    )
    top_symbol = _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STACK_CALLS[0][5]
    chat_symbol = _cc_catalog.HUD_UI_MGR_UPDATE_FRAME_STACK_CALLS[1][5]
    expected_relocations = (
        (0x02, IMAGE_REL_I386_DIR32, top_symbol, 0),
        (0x07, IMAGE_REL_I386_REL32, _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_SYMBOL, 0),
        (0x0D, IMAGE_REL_I386_DIR32, top_symbol, 0),
        (0x1A, IMAGE_REL_I386_DIR32, chat_symbol, 0),
        (0x1F, IMAGE_REL_I386_REL32, _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_SYMBOL, 0),
        (0x25, IMAGE_REL_I386_DIR32, chat_symbol, 0),
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
    expected_mask = {
        index
        for offset, _kind, _symbol, _addend in expected_relocations
        for index in range(offset, offset + 4)
    }
    if (
        caller.data != exact_body
        or observed_relocations != expected_relocations
        or {
            index
            for index, masked in enumerate(caller.relocation_mask)
            if masked
        }
        != expected_mask
        or caller.undefined_external_functions.count(
            _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_SYMBOL
        )
        != 1
        or caller.undefined_external_data.count(top_symbol) != 1
        or caller.undefined_external_data.count(chat_symbol) != 1
        or caller.defined_external_functions.count(
            _cc_catalog.HUD_UI_MGR_DISABLE_STACKS_CALLER_SYMBOL
        )
        != 1
        or any(
            symbol in caller.defined_external_data
            for symbol in (top_symbol, chat_symbol)
        )
        or _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_SYMBOL
        in caller.defined_external_functions
    ):
        raise ValueError(
            "HUD DisableTopAndChatStacks bridge requires the exact complete "
            "candidate body, six COFF rows/addends/order, relocation mask, "
            "padding, and relevant external population"
        )

    offsets = list(_cc_callable_identity._candidate_complete_instruction_offsets(candidate))
    if (
        offsets
        and offsets[0] is None
        and len(offsets) > 1
        and offsets[1] == len(candidate.instructions[0].bytes)
    ):
        offsets[0] = 0
    encoded = bytes(
        int(value, 16)
        for instruction in candidate.instructions
        for value in instruction.bytes
    )
    call_rows = tuple(
        (
            offsets[index],
            _cc_cfg._instruction_operand(instruction).strip(),
            _cc_cfg._cleanup_after(candidate.instructions, index),
        )
        for index, instruction in enumerate(candidate.instructions)
        if _cc_cfg._instruction_mnemonic(instruction) == "call"
    )
    memory_sources = tuple(
        (
            offsets[index],
            _cc_targets._exact_memory_expression(
                _cc_cfg._instruction_operand(instruction).split(",", 1)[-1]
            ),
        )
        for index, instruction in enumerate(candidate.instructions)
        if _cc_cfg._instruction_mnemonic(instruction) == "mov"
    )
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    if (
        tuple(offsets)
        != (0x00, 0x06, 0x0B, 0x11, 0x13, 0x15, 0x18,
            0x1E, 0x23, 0x29, 0x2B, 0x2D, 0x30)
        or encoded != exact_body[:-15]
        or tuple(_cc_cfg._instruction_mnemonic(row) for row in candidate.instructions)
        != (
            "mov", "call", "mov", "push", "mov", "call", "mov",
            "call", "mov", "push", "mov", "call", "ret",
        )
        or call_rows
        != (
            (0x06, _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_SYMBOL, None),
            (0x15, "dword [eax+4]", None),
            (0x1E, _cc_catalog.HUD_UI_MGR_ENABLE_STACKS_CLEAR_SYMBOL, None),
            (0x2D, "dword [edx+4]", None),
        )
        or memory_sources
        != (
            (0x00, top_symbol),
            (0x0B, top_symbol),
            (0x13, "ecx"),
            (0x18, chat_symbol),
            (0x23, chat_symbol),
            (0x2B, "ecx"),
        )
        or _cc_cfg._instruction_operand(candidate.instructions[3]).strip().lower()
        not in {"0", "0x0"}
        or _cc_cfg._instruction_operand(candidate.instructions[9]).strip().lower()
        not in {"0", "0x0"}
        or _cc_cfg._instruction_operand(candidate.instructions[12]).strip() not in {"", "0"}
        or invocation_indices != (1, 5, 7, 11)
        or candidate.local_control_flow_indices != frozenset()
        or dict(candidate.local_control_flow_targets) != {}
    ):
        raise ValueError(
            "HUD DisableTopAndChatStacks bridge rejects candidate top/chat "
            "storage, receiver/vptr reaching definitions, Clear/SetEnabled "
            "call population/order/form/slot/cleanup, false arguments, "
            "return, padding, or complete topology drift"
        )
    return {
        normalize_address(hex(call_offset)): ReviewedLoopVptrStorageBridge(
            register=register,
            storage_identity=storage_by_symbol[symbol_id],
            slot_displacement=0x04,
            assembly_source="cod",
        )
        for call_offset, register, symbol_id
        in _cc_catalog.HUD_UI_MGR_DISABLE_STACKS_CANDIDATE_VIRTUAL_CALLS
    }


def _hud_ui_mgr_zrd_payload_tu_local_candidate_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    compiler_generated_bridges: Mapping[str, str],
) -> dict[str, str]:
    """Expose EnsureHudLoaded's exact TU-local payload helper calls.

    Retail inlines the helper semantics.  The reviewed caller identity is only
    a candidate comparison sentinel: all seven direct calls remain in the
    candidate contract so ordinary comparison can report them as extra.
    """
    matching_instructions: list[tuple[int, Instruction]] = []
    matching_symbols: set[str] = set()
    for index, instruction in enumerate(candidate.instructions):
        if _cc_cfg._instruction_mnemonic(instruction) not in {"call", "jmp"}:
            continue
        operand = _cc_cfg._instruction_operand(instruction).strip()
        if _cc_catalog.HUD_UI_MGR_ZRD_PAYLOAD_TU_LOCAL_COD_RE.fullmatch(operand):
            matching_instructions.append((index, instruction))
            matching_symbols.add(operand)
            continue
        if "HudUiZrdPayload" in operand:
            raise ValueError(
                "HUD EnsureHudLoaded ZRD-payload TU-local bridge accepts only "
                "the exact helper name, 00_hud.cpp TU marker, decimal "
                "discriminator, and fastcall function signature"
            )
    if not matching_instructions:
        return {}
    if (
        len(matching_instructions)
        != _cc_catalog.HUD_UI_MGR_ZRD_PAYLOAD_CALL_COUNT
        or len(matching_symbols) != 1
    ):
        raise ValueError(
            "HUD EnsureHudLoaded ZRD-payload TU-local bridge requires exactly "
            "seven calls to one candidate helper decoration"
        )
    cod_symbol = next(iter(matching_symbols))
    coff_symbol = cod_symbol.replace(
        "?_tu_order\\",
        "?%_tu_order\\",
        1,
    )
    cod_match = _cc_catalog.HUD_UI_MGR_ZRD_PAYLOAD_TU_LOCAL_COD_RE.fullmatch(cod_symbol)
    coff_match = _cc_catalog.HUD_UI_MGR_ZRD_PAYLOAD_TU_LOCAL_COFF_RE.fullmatch(coff_symbol)
    if (
        cod_match is None
        or coff_match is None
        or cod_match.group("discriminator")
        != coff_match.group("discriminator")
    ):
        raise ValueError(
            "HUD EnsureHudLoaded ZRD-payload TU-local bridge requires one "
            "consistent COD/COFF decimal discriminator"
        )

    definition = candidate.caller_definition
    normalized_start = normalize_address(caller_start)
    normalized_end = normalize_address(caller_end_exclusive)
    if (
        caller_identity != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_IDENTITY
        or normalized_start != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START
        or normalized_end != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_END_EXCLUSIVE
        or sorted(
            address
            for address, identity in indexes.by_address.items()
            if identity == caller_identity
        )
        != [_cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START]
        or caller_identity in indexes.provider_ids
        or definition is None
        or definition.symbol != _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_SYMBOL
        or not definition.data
        or len(definition.relocation_mask) != len(definition.data)
        or any(
            row.get("target_identity") == caller_identity
            for row in expected
        )
    ):
        raise ValueError(
            "HUD EnsureHudLoaded ZRD-payload TU-local bridge requires the "
            "exact reviewed caller, extent, candidate symbol, and no retail "
            "comparison-sentinel target"
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
            "HUD EnsureHudLoaded ZRD-payload TU-local bridge requires one "
            "exact unaliased authored caller and resolved source edge"
        )

    target = candidate.target
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.HUD_UI_MGR_ENSURE_CALLER_START
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
            "HUD EnsureHudLoaded ZRD-payload TU-local bridge requires the "
            "exact registered HUD target and unique hud.cpp contribution"
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
            "HUD EnsureHudLoaded ZRD-payload TU-local bridge requires the "
            "exact authored EnsureHudLoaded hud.cpp contribution identity"
        )

    collision_names = {cod_symbol, coff_symbol}
    if (
        any(name in indexes.by_candidate_name for name in collision_names)
        or any(name in bridge_names for name in collision_names)
        or any(name in indexes.storage_by_name for name in collision_names)
        or any(
            name in compiler_generated_bridges for name in collision_names
        )
    ):
        raise ValueError(
            "HUD EnsureHudLoaded ZRD-payload TU-local bridge has a conflicting "
            "ordinary candidate, retail, storage, or compiler-generated "
            "identity"
        )

    folded_names = {name.casefold() for name in collision_names}
    defined_case_matches = [
        name
        for name in definition.defined_external_functions
        if name.casefold() in folded_names
    ]
    undefined_case_matches = [
        name
        for name in definition.undefined_external_functions
        if name.casefold() in folded_names
    ]
    helper_definitions = candidate.tu_local_function_definitions
    payload_helper_definitions = [
        name
        for name in helper_definitions
        if _cc_catalog.HUD_UI_MGR_ZRD_PAYLOAD_TU_LOCAL_COFF_RE.fullmatch(name)
        is not None
    ]
    if (
        defined_case_matches != [coff_symbol]
        or undefined_case_matches
        or payload_helper_definitions != [coff_symbol]
    ):
        raise ValueError(
            "HUD EnsureHudLoaded ZRD-payload TU-local bridge requires exactly "
            "one same-object defined helper and no undefined, duplicate, or "
            "case-folded alias"
        )
    helper_definition = helper_definitions[coff_symbol]
    if (
        helper_definition.symbol != coff_symbol
        or helper_definition.data
        != _cc_catalog.HUD_UI_MGR_ZRD_PAYLOAD_HELPER_BODY
        or helper_definition.section_size
        != len(_cc_catalog.HUD_UI_MGR_ZRD_PAYLOAD_HELPER_BODY)
        or helper_definition.relocations
        or len(helper_definition.relocation_mask)
        != len(_cc_catalog.HUD_UI_MGR_ZRD_PAYLOAD_HELPER_BODY)
        or any(helper_definition.relocation_mask)
        or helper_definition.section_external_functions != (coff_symbol,)
    ):
        raise ValueError(
            "HUD EnsureHudLoaded ZRD-payload TU-local bridge requires the "
            "exact 16-byte relocation-free unaliased helper definition"
        )

    complete_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    instruction_offsets: list[int] = []
    for instruction_index, instruction in matching_instructions:
        instruction_offset = complete_offsets[instruction_index]
        if (
            _cc_cfg._instruction_mnemonic(instruction) != "call"
            or tuple(value.lower() for value in instruction.bytes)
            != ("e8", "00", "00", "00", "00")
            or instruction_offset is None
            or _cc_cfg._cleanup_after(candidate.instructions, instruction_index)
            is not None
        ):
            raise ValueError(
                "HUD EnsureHudLoaded ZRD-payload TU-local bridge requires "
                "exact structurally located direct E8 callee-cleanup calls"
            )
        instruction_offsets.append(instruction_offset)
    if (
        len(set(instruction_offsets)) != len(instruction_offsets)
        or tuple(instruction_offsets) != tuple(sorted(instruction_offsets))
    ):
        raise ValueError(
            "HUD EnsureHudLoaded ZRD-payload TU-local bridge requires the "
            "unique ordered seven-call candidate topology"
        )

    helper_casefold_names = {
        cod_symbol.casefold(),
        coff_symbol.casefold(),
    }
    references = tuple(
        sorted(
            (
                relocation
                for relocation in definition.relocations
                if relocation.symbol_name.casefold()
                in helper_casefold_names
            ),
            key=lambda relocation: relocation.offset,
        )
    )
    expected_relocation_offsets = tuple(
        offset + 1 for offset in instruction_offsets
    )
    if (
        tuple(reference.offset for reference in references)
        != expected_relocation_offsets
        or any(reference.symbol_name != coff_symbol for reference in references)
    ):
        raise ValueError(
            "HUD EnsureHudLoaded ZRD-payload TU-local bridge requires one "
            "exact ordered COFF relocation for each reviewed candidate call"
        )
    for reference in references:
        instruction_offset = reference.offset - 1
        field_end = reference.offset + 4
        relocations_at_offset = [
            relocation
            for relocation in definition.relocations
            if relocation.offset == reference.offset
        ]
        if (
            reference.type != IMAGE_REL_I386_REL32
            or len(relocations_at_offset) != 1
            or instruction_offset < 0
            or field_end > len(definition.data)
            or field_end > len(definition.relocation_mask)
            or definition.data[instruction_offset:reference.offset]
            != b"\xe8"
            or definition.relocation_mask[instruction_offset]
            or struct.unpack_from("<I", definition.data, reference.offset)[0]
            != 0
            or not all(
                definition.relocation_mask[index]
                for index in range(reference.offset, field_end)
            )
        ):
            raise ValueError(
                "HUD EnsureHudLoaded ZRD-payload TU-local bridge requires "
                "exact zero-addend fully masked E8 REL32 relocations"
            )

    return {cod_symbol: caller_identity}


def _hud_text_input_constructor_alias_candidate_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    compiler_generated_bridges: Mapping[str, str],
) -> dict[str, str]:
    """Map one exact source wrapper spelling to its reviewed retail constructor.

    The current ``HudUiMgrData`` constructor invokes the source-level
    ``HudUiTextInput::Constructor(int)`` wrapper.  VC5 decorates that wrapper
    differently from the ordinary C++ constructor symbol which the reviewed
    retail identity uses.  This bridge is deliberately caller-scoped: it
    accepts only the exact class/signature, exact call and relocation offsets,
    and the exact retail contract row at the reviewed direct call site.
    """
    matching_instructions: list[Instruction] = []
    for instruction in candidate.instructions:
        if _cc_cfg._instruction_mnemonic(instruction) not in {"call", "jmp"}:
            continue
        operand = _cc_cfg._instruction_operand(instruction).strip()
        if operand == _cc_catalog.HUD_TEXT_INPUT_CONSTRUCTOR_ALIAS_SYMBOL:
            matching_instructions.append(instruction)
            continue
        if (
            "Constructor@HudUiTextInput" in operand
            or "HudUiTextInput@@QAEPAU1@" in operand
        ):
            raise ValueError(
                "HUD text-input constructor alias bridge accepts only the exact "
                "HudUiTextInput::Constructor(int) decorated signature"
            )
    if not matching_instructions:
        return {}
    if len(matching_instructions) != 1:
        raise ValueError(
            "HUD text-input constructor alias bridge requires exactly one "
            "candidate invocation"
        )

    definition = candidate.caller_definition
    normalized_start = normalize_address(caller_start)
    normalized_end = normalize_address(caller_end_exclusive)
    if (
        caller_identity != _cc_catalog.HUD_TEXT_INPUT_CONSTRUCTOR_ALIAS_CALLER_IDENTITY
        or normalized_start != _cc_catalog.HUD_TEXT_INPUT_CONSTRUCTOR_ALIAS_CALLER_START
        or normalized_end
        != _cc_catalog.HUD_TEXT_INPUT_CONSTRUCTOR_ALIAS_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(normalized_start) != caller_identity
        or caller_identity in indexes.provider_ids
        or definition is None
        or definition.symbol != _cc_catalog.HUD_TEXT_INPUT_CONSTRUCTOR_ALIAS_CALLER_SYMBOL
    ):
        raise ValueError(
            "HUD text-input constructor alias bridge requires the exact reviewed "
            "0x40d7e0 caller identity, extent, and VC5 constructor symbol"
        )
    if (
        indexes.by_address.get(_cc_catalog.HUD_TEXT_INPUT_CONSTRUCTOR_TARGET_ADDRESS)
        != _cc_catalog.HUD_TEXT_INPUT_CONSTRUCTOR_TARGET_IDENTITY
        or indexes.by_candidate_name.get(
            _cc_catalog.HUD_TEXT_INPUT_CONSTRUCTOR_TARGET_SYMBOL
        )
        != _cc_catalog.HUD_TEXT_INPUT_CONSTRUCTOR_TARGET_IDENTITY
        or _cc_catalog.HUD_TEXT_INPUT_CONSTRUCTOR_TARGET_IDENTITY in indexes.provider_ids
    ):
        raise ValueError(
            "HUD text-input constructor alias bridge requires the exact reviewed "
            "0x4b42f0 authored constructor identity and ordinary VC5 symbol"
        )
    if (
        _cc_catalog.HUD_TEXT_INPUT_CONSTRUCTOR_ALIAS_SYMBOL in indexes.by_candidate_name
        or _cc_catalog.HUD_TEXT_INPUT_CONSTRUCTOR_ALIAS_SYMBOL in indexes.storage_by_name
        or _cc_catalog.HUD_TEXT_INPUT_CONSTRUCTOR_ALIAS_SYMBOL in bridge_names
        or _cc_catalog.HUD_TEXT_INPUT_CONSTRUCTOR_ALIAS_SYMBOL
        in compiler_generated_bridges
    ):
        raise ValueError(
            "HUD text-input constructor alias bridge has a conflicting ordinary "
            "candidate, retail, storage, or compiler bridge identity"
        )

    undefined_case_matches = [
        name
        for name in definition.undefined_external_functions
        if name.casefold()
        == _cc_catalog.HUD_TEXT_INPUT_CONSTRUCTOR_ALIAS_SYMBOL.casefold()
    ]
    defined_case_matches = [
        name
        for name in definition.defined_external_functions
        if name.casefold()
        == _cc_catalog.HUD_TEXT_INPUT_CONSTRUCTOR_ALIAS_SYMBOL.casefold()
    ]
    if (
        undefined_case_matches
        != [_cc_catalog.HUD_TEXT_INPUT_CONSTRUCTOR_ALIAS_SYMBOL]
        or defined_case_matches
    ):
        raise ValueError(
            "HUD text-input constructor alias bridge requires exactly one "
            "undefined external function with the exact wrapper signature and "
            "no same-object or case-folded definition"
        )

    instruction = matching_instructions[0]
    raw_offset = _cc_cfg._source_instruction_address(instruction)
    if (
        _cc_cfg._instruction_mnemonic(instruction) != "call"
        or tuple(instruction.bytes[:1]) != ("e8",)
        or not raw_offset
    ):
        raise ValueError(
            "HUD text-input constructor alias bridge requires one exact "
            "offset-bearing direct E8 candidate call"
        )
    call_offset = address_value(raw_offset)

    references = tuple(
        relocation
        for relocation in definition.relocations
        if relocation.symbol_name
        == _cc_catalog.HUD_TEXT_INPUT_CONSTRUCTOR_ALIAS_SYMBOL
    )
    relocation_offset = call_offset + 1
    if (
        len(references) != 1
        or references[0].offset != relocation_offset
        or references[0].type != IMAGE_REL_I386_REL32
        or relocation_offset + 4 > len(definition.data)
        or relocation_offset + 4 > len(definition.relocation_mask)
        or definition.data[
            call_offset:relocation_offset
        ]
        != b"\xe8"
        or struct.unpack_from(
            "<I",
            definition.data,
            relocation_offset,
        )[0]
        != 0
        or not all(
            definition.relocation_mask[index]
            for index in range(relocation_offset, relocation_offset + 4)
        )
    ):
        raise ValueError(
            "HUD text-input constructor alias bridge requires one exact "
            "zero-addend fully masked E8 REL32 relocation at the reviewed "
            "candidate call"
        )

    expected_rows = [
        row
        for row in expected
        if row.get("target_identity")
        == _cc_catalog.HUD_TEXT_INPUT_CONSTRUCTOR_TARGET_IDENTITY
    ]
    if (
        len(expected_rows) != 1
        or expected_rows[0].get("ordinal")
        != _cc_catalog.HUD_TEXT_INPUT_CONSTRUCTOR_RETAIL_ORDINAL
        or expected_rows[0].get("form") != "call"
        or expected_rows[0].get("dispatch") != "direct"
        or expected_rows[0].get("identity_kind") != "direct"
        or expected_rows[0].get("storage_identity") != ""
        or expected_rows[0].get("slot_displacement") is not None
        or expected_rows[0].get("cleanup_bytes") is not None
    ):
        raise ValueError(
            "HUD text-input constructor alias bridge requires the exact reviewed "
            "ordinal-8 direct retail call contract for 0x4b42f0"
        )

    provisional_bridges = dict(compiler_generated_bridges)
    provisional_bridges[
        _cc_catalog.HUD_TEXT_INPUT_CONSTRUCTOR_ALIAS_SYMBOL
    ] = _cc_catalog.HUD_TEXT_INPUT_CONSTRUCTOR_TARGET_IDENTITY
    provisional = _cc_extraction.extract_invocation_contract(
        candidate.instructions,
        source="cod",
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge_names=bridge_names,
        compiler_generated_bridges=provisional_bridges,
        local_control_flow_indices=candidate.local_control_flow_indices,
        local_control_flow_targets=candidate.local_control_flow_targets,
    )
    candidate_rows = [
        row
        for row in provisional
        if row.get("target_identity")
        == _cc_catalog.HUD_TEXT_INPUT_CONSTRUCTOR_TARGET_IDENTITY
    ]
    if (
        len(candidate_rows) != 1
        or candidate_rows[0].get("form") != "call"
        or candidate_rows[0].get("dispatch") != "direct"
        or candidate_rows[0].get("identity_kind") != "direct"
        or candidate_rows[0].get("storage_identity") != ""
        or candidate_rows[0].get("slot_displacement") is not None
        or candidate_rows[0].get("cleanup_bytes") is not None
    ):
        raise ValueError(
            "HUD text-input constructor alias bridge does not produce one exact "
            "direct candidate constructor call"
        )
    return {
        _cc_catalog.HUD_TEXT_INPUT_CONSTRUCTOR_ALIAS_SYMBOL: (
            _cc_catalog.HUD_TEXT_INPUT_CONSTRUCTOR_TARGET_IDENTITY
        )
    }
