"""Recoil call-contract recoil world evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import candidate as _cc_candidate
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import errors as _cc_errors
from _recoil.call_contract import source as _cc_source

if TYPE_CHECKING:
    from _recoil.call_contract.records import CandidateAssembly, IdentityIndexes

import struct
from pathlib import Path
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import IMAGE_REL_I386_DIR32, IMAGE_REL_I386_REL32
from _recoil.commands.vc5_build import DEFAULT_MANIFEST as DEFAULT_FINAL_BUILD_MANIFEST
from _recoil.commands.vc5_build import load_config as load_final_build_config
from _recoil.commands.vc5_verify import load_manifest
from _recoil.lib.cpp_definition_closure import (
    CallableKey,
    decode_vc5_zero_argument_callable_identity,
)
from _recoil.lib.progress import ProgressDocument, ProgressError, normalize_address
from _recoil.lib.repository_paths import load_repository_path_inventory
from _recoil.lib.tooling import REPO_ROOT


def _zdeclient_qsand_callback_authority_provenance(
    document: ProgressDocument,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    target: Any,
    expected: Sequence[Mapping[str, Any]],
) -> dict[str, Any]:
    """Compose the exact reviewed QSand callback authority package."""

    manifest_payload: Any | None = None
    current_callback_target: Mapping[str, Any] | None = None
    current_callback_target_id = ""
    manifest_error = ""
    from _recoil.lib.verification_targets import vc5_target_registration
    try:
        current_callback_target_id, current = vc5_target_registration(
            _cc_catalog.ZDECLIENT_QSAND_CALLBACK_TARGET_MANIFEST_PATH
        )
        current_callback_target = current
        manifest_payload = load_manifest(
            _cc_catalog.ZDECLIENT_QSAND_CALLBACK_TARGET_MANIFEST_PATH,
            enforce_source_policy=False,
        )
    except (OSError, ProgressError, ValueError) as exc:
        manifest_error = str(exc)

    caller_symbol_id = _cc_catalog.ZDECLIENT_QSAND_CALLER_IDENTITY.removeprefix("symbol:")
    target_registration = document.collection("verification_targets").get(
        _cc_catalog.ZDECLIENT_QSAND_TARGET_ID
    )
    target_registration_body = (
        target_registration.get("registration")
        if isinstance(target_registration, Mapping)
        else None
    )
    caller_row = document.collection("symbols").get(caller_symbol_id)
    callback_row = document.collection("symbols").get(
        _cc_catalog.ZDECLIENT_QSAND_CALLBACK_SYMBOL_ID
    )
    storage_row = document.collection("storage_contributions").get(
        _cc_catalog.ZDECLIENT_QSAND_CALLBACK_STORAGE_ID
    )
    callback_target = document.collection("verification_targets").get(
        _cc_catalog.ZDECLIENT_QSAND_CALLBACK_TARGET_ID
    )
    callback_registration = (
        callback_target.get("registration")
        if isinstance(callback_target, Mapping)
        else None
    )
    owner = document.collection("owners").get(
        _cc_catalog.ZDECLIENT_QSAND_CALLBACK_OWNER_ID
    )
    owner_gates = owner.get("gates") if isinstance(owner, Mapping) else None
    address_metadata = (
        owner.get("address_metadata", {}).get(_cc_catalog.ZDECLIENT_QSAND_CALLBACK_ADDRESS)
        if isinstance(owner, Mapping)
        and isinstance(owner.get("address_metadata"), Mapping)
        else None
    )
    caller_edges = (
        caller_row.get("source_traceability", {}).get("source_edges", [])
        if isinstance(caller_row, Mapping)
        and isinstance(caller_row.get("source_traceability"), Mapping)
        else []
    )
    callback_edges = (
        callback_row.get("source_traceability", {}).get("source_edges", [])
        if isinstance(callback_row, Mapping)
        and isinstance(callback_row.get("source_traceability"), Mapping)
        else []
    )
    manifest_rows = tuple(getattr(manifest_payload, "data_symbols", ()))
    exact_manifest_rows = [
        row
        for row in manifest_rows
        if getattr(row, "address", None) == _cc_catalog.ZDECLIENT_QSAND_CALLBACK_ADDRESS
    ]
    selected = None
    selected_error = ""
    try:
        selected = _cc_candidate._target_function(target, _cc_catalog.ZDECLIENT_QSAND_CALLER_ADDRESS)
    except (RuntimeError, ValueError) as exc:
        selected_error = str(exc)

    checks = {
        "caller": bool(
            caller_identity == _cc_catalog.ZDECLIENT_QSAND_CALLER_IDENTITY
            and normalize_address(caller_start) == _cc_catalog.ZDECLIENT_QSAND_CALLER_ADDRESS
            and normalize_address(caller_end_exclusive)
            == _cc_catalog.ZDECLIENT_QSAND_CALLER_END_EXCLUSIVE
            and isinstance(caller_row, Mapping)
            and caller_row.get("binary") == "recoil"
            and caller_row.get("kind") == "function"
            and caller_row.get("pipeline_class") == "authored"
            and caller_row.get("address") == _cc_catalog.ZDECLIENT_QSAND_CALLER_ADDRESS
            and caller_row.get("end_exclusive")
            == _cc_catalog.ZDECLIENT_QSAND_CALLER_END_EXCLUSIVE
            and caller_row.get("extent_state") == "known"
            and caller_row.get("size") == 0x120
            and caller_row.get("output_section_id") == "recoil:section:.text"
            and caller_row.get("physical_block_id") == "recoil:block:0x455ea0"
            and caller_row.get("source_traceability", {}).get("state")
            == "resolved"
            and caller_edges
            == [
                {
                    "anchor_id": _cc_catalog.ZDECLIENT_QSAND_CALLER_ANCHOR_ID,
                    "emission_context": {
                        "translation_unit": _cc_catalog.ZDECLIENT_QSAND_SOURCE_PATH
                    },
                    "evidence_ids": [],
                    "relation": "defines",
                }
            ]
        ),
        "data": bool(
            isinstance(callback_row, Mapping)
            and callback_row.get("binary") == "recoil"
            and callback_row.get("kind") == "data"
            and callback_row.get("disposition") == "authored"
            and callback_row.get("address") == _cc_catalog.ZDECLIENT_QSAND_CALLBACK_ADDRESS
            and callback_row.get("end_exclusive") == "0x539de8"
            and callback_row.get("extent_state") == "known"
            and callback_row.get("size") == 4
            and callback_row.get("output_section_id") == "recoil:section:.data"
            and callback_row.get("storage_contribution_ids")
            == [_cc_catalog.ZDECLIENT_QSAND_CALLBACK_STORAGE_ID]
            and callback_row.get("verification_target_ids")
            == [_cc_catalog.ZDECLIENT_QSAND_CALLBACK_TARGET_ID]
            and callback_row.get("source_traceability", {}).get("state")
            == "resolved"
            and callback_edges
            == [
                {
                    "anchor_id": _cc_catalog.ZDECLIENT_QSAND_CALLBACK_ANCHOR_ID,
                    "emission_context": {
                        "translation_unit": _cc_catalog.ZDECLIENT_QSAND_CALLBACK_SOURCE_PATH
                    },
                    "evidence_ids": [],
                    "relation": "defines",
                }
            ]
        ),
        "storage": bool(
            isinstance(storage_row, Mapping)
            and storage_row.get("binary") == "recoil"
            and storage_row.get("kind") == "data-symbol"
            and storage_row.get("output_section_id") == "recoil:section:.data"
            and storage_row.get("overlap") == "none"
            and storage_row.get("owner_ids")
            == [_cc_catalog.ZDECLIENT_QSAND_CALLBACK_OWNER_ID]
            and storage_row.get("symbol_ids")
            == [_cc_catalog.ZDECLIENT_QSAND_CALLBACK_SYMBOL_ID]
            and storage_row.get("reference", {}).get("address")
            == _cc_catalog.ZDECLIENT_QSAND_CALLBACK_ADDRESS
            and storage_row.get("reference", {}).get("extent_state")
            in {"unknown", "known"}
            and indexes.storage_by_address.get(_cc_catalog.ZDECLIENT_QSAND_CALLBACK_ADDRESS)
            == _cc_catalog.ZDECLIENT_QSAND_CALLBACK_IDENTITY
        ),
        "owner": bool(
            isinstance(owner, Mapping)
            and owner.get("binary") == "recoil"
            and owner.get("kind") == "data-owner"
            and owner.get("lifecycle_state") == "active"
            and owner.get("source_paths")
            == [_cc_catalog.ZDECLIENT_QSAND_CALLBACK_SOURCE_PATH, _cc_catalog.ZDECLIENT_QSAND_CALLBACK_HEADER_PATH]
            and isinstance(owner_gates, Mapping)
            and owner_gates.get("boundary") == "accepted"
            and owner_gates.get("source") == "accepted"
            and owner_gates.get("data") == "accepted"
            and owner_gates.get("owner_linkage") == "accepted"
            and address_metadata
            == {
                "group": "data.effects_weapons",
                "name": _cc_catalog.ZDECLIENT_QSAND_CALLBACK_NAME,
                "source_path": _cc_catalog.ZDECLIENT_QSAND_CALLBACK_SOURCE_PATH,
                "target": "zdeclient_net_relay_callback_globals",
            }
        ),
        "caller_target": bool(
            selected is not None
            and getattr(target, "name", "") == _cc_catalog.ZDECLIENT_QSAND_TARGET_NAME
            and getattr(target, "target_binary", "") == "recoil"
            and Path(str(getattr(target, "manifest_path", ""))).resolve()
            == _cc_catalog.ZDECLIENT_QSAND_TARGET_MANIFEST.resolve()
            and getattr(target, "source_from", "") == _cc_catalog.ZDECLIENT_QSAND_SOURCE_PATH
            and bool(getattr(target, "check_translation_unit_function_order", False))
            and getattr(selected, "symbol", "") == _cc_catalog.ZDECLIENT_QSAND_CALLER_SYMBOL
            and getattr(selected, "pipeline_class", "") == "authored"
            and getattr(selected, "authored_order_role", "") == "authored-body"
            and bool(getattr(selected, "required_presence", False))
            and isinstance(target_registration, Mapping)
            and target_registration.get("binary") == "recoil"
            and target_registration.get("kind") == "vc5"
            and target_registration.get("name") == _cc_catalog.ZDECLIENT_QSAND_TARGET_NAME
            and isinstance(target_registration_body, Mapping)
            and target_registration_body.get("manifest_path")
            == _cc_catalog.ZDECLIENT_QSAND_TARGET_MANIFEST.relative_to(REPO_ROOT).as_posix()
            and target_registration_body.get("source_from") == _cc_catalog.ZDECLIENT_QSAND_SOURCE_PATH
            and target_registration_body.get("function_order_scope") == "authored"
            and target_registration_body.get("function_addresses", []).count(
                _cc_catalog.ZDECLIENT_QSAND_CALLER_ADDRESS
            ) == 1
        ),
        "callback_target": bool(
            isinstance(callback_target, Mapping)
            and callback_target.get("binary") == "recoil"
            and callback_target.get("kind") == "vc5"
            and callback_target.get("name") == "zdeclient_net_relay_callback_globals"
            and callback_target.get("symbol_ids")
            == [_cc_catalog.ZDECLIENT_QSAND_CALLBACK_SYMBOL_ID, "recoil:data:0x539de8"]
            and callback_target.get("unresolved_addresses") in (None, [])
            and isinstance(callback_registration, Mapping)
            and callback_registration.get("manifest_path")
            == _cc_catalog.ZDECLIENT_QSAND_CALLBACK_TARGET_MANIFEST
            and callback_registration.get("source_from")
            == _cc_catalog.ZDECLIENT_QSAND_CALLBACK_SOURCE_PATH
            and callback_registration.get("data_addresses")
            == [_cc_catalog.ZDECLIENT_QSAND_CALLBACK_ADDRESS, "0x539de8"]
            and current_callback_target_id == _cc_catalog.ZDECLIENT_QSAND_CALLBACK_TARGET_ID
            and isinstance(current_callback_target, Mapping)
            and current_callback_target.get("binary") == callback_target.get("binary")
            and current_callback_target.get("kind") == callback_target.get("kind")
            and current_callback_target.get("name") == callback_target.get("name")
            and isinstance(current_callback_target.get("registration"), Mapping)
            and current_callback_target["registration"].get("manifest_path")
            == callback_registration.get("manifest_path")
            and current_callback_target["registration"].get("source_from")
            == callback_registration.get("source_from")
            and current_callback_target["registration"].get("data_addresses")
            == callback_registration.get("data_addresses")
        ),
        "manifest": bool(
            manifest_error == ""
            and getattr(manifest_payload, "name", "")
            == "zdeclient_net_relay_callback_globals"
            and getattr(manifest_payload, "source_from", "")
            == _cc_catalog.ZDECLIENT_QSAND_CALLBACK_SOURCE_PATH
            and tuple(getattr(manifest_payload, "source_files", ()))
            == (_cc_catalog.ZDECLIENT_QSAND_CALLBACK_SOURCE_PATH,)
            and len(exact_manifest_rows) == 1
            and getattr(exact_manifest_rows[0], "symbol", None)
            == _cc_catalog.ZDECLIENT_QSAND_CALLBACK_DECORATED_SYMBOL
            and getattr(exact_manifest_rows[0], "symbol_regex", None) is None
            and getattr(exact_manifest_rows[0], "name", None)
            == _cc_catalog.ZDECLIENT_QSAND_CALLBACK_NAME
            and getattr(exact_manifest_rows[0], "bn_name", None)
            == _cc_catalog.ZDECLIENT_QSAND_CALLBACK_NAME
            and getattr(exact_manifest_rows[0], "byte_length", None) == 4
        ),
        "retail_contract": sum(
            row.get("form") == "call"
            and row.get("dispatch") == "indirect"
            and row.get("identity_kind") == "direct"
            and row.get("target_identity")
            == _cc_catalog.ZDECLIENT_QSAND_CALLBACK_RETAIL_TARGET_IDENTITY
            and row.get("storage_identity")
            == _cc_catalog.ZDECLIENT_QSAND_CALLBACK_IDENTITY
            and row.get("slot_displacement") is None
            and row.get("cleanup_bytes") is None
            for row in expected
        ) == 1,
    }
    return {
        "kind": "call-contract-callback-authority-dependency",
        "contract_version": 1,
        "candidate_expected_truth": False,
        "caller_symbol_id": caller_symbol_id,
        "caller_address": _cc_catalog.ZDECLIENT_QSAND_CALLER_ADDRESS,
        "caller_end_exclusive": _cc_catalog.ZDECLIENT_QSAND_CALLER_END_EXCLUSIVE,
        "caller_target_id": _cc_catalog.ZDECLIENT_QSAND_TARGET_ID,
        "authority_entity_ids": [
            _cc_catalog.ZDECLIENT_QSAND_CALLBACK_SYMBOL_ID,
            _cc_catalog.ZDECLIENT_QSAND_CALLBACK_STORAGE_ID,
            _cc_catalog.ZDECLIENT_QSAND_CALLBACK_OWNER_ID,
            _cc_catalog.ZDECLIENT_QSAND_CALLBACK_TARGET_ID,
        ],
        "authority_paths": [
            _cc_catalog.ZDECLIENT_QSAND_CALLBACK_SOURCE_PATH,
            _cc_catalog.ZDECLIENT_QSAND_CALLBACK_HEADER_PATH,
            _cc_catalog.ZDECLIENT_QSAND_CALLBACK_TARGET_MANIFEST,
        ],
        "checks": checks,
        "missing_authority_gates": sorted(
            key for key, value in checks.items() if value is not True
        ),
        "manifest_error": manifest_error,
        "selected_caller_error": selected_error,
        "complete": all(checks.values()),
        "repair_route": "governed-authority-or-tool-maintenance",
        "source_repairable": False,
    }


def _zdeclient_qsand_callback_candidate_register_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, str]:
    """Bind the exact QSand callback load to its reviewed retail storage.

    The tracker intentionally carries no general candidate spelling for this
    data row.  Publish the one decorated C++ name only when the caller, order
    manifest, callback data/storage rows, data-target registration, owner
    metadata, and candidate COFF use all agree exactly.
    """

    if (
        caller_identity != _cc_catalog.ZDECLIENT_QSAND_CALLER_IDENTITY
        or normalize_address(caller_start) != _cc_catalog.ZDECLIENT_QSAND_CALLER_ADDRESS
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.ZDECLIENT_QSAND_CALLER_END_EXCLUSIVE
    ):
        return {}
    caller = candidate.caller_definition
    if caller is None or _cc_catalog.ZDECLIENT_QSAND_CALLBACK_DECORATED_SYMBOL not in (
        caller.undefined_external_data
        + caller.defined_external_data
        + caller.undefined_external_functions
        + caller.defined_external_functions
    ):
        return {}

    authority = _zdeclient_qsand_callback_authority_provenance(
        document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        target=candidate.target,
        expected=expected,
    )
    if authority["complete"] is not True:
        raise _cc_errors.CandidateCallbackAuthorityError(
            provenance=authority,
            message=(
                "zDEClient QSand callback bridge requires exact synchronized "
                "caller, data/storage, owner, and manifest authority; "
                "missing authority gates: "
                + ", ".join(authority["missing_authority_gates"])
            ),
        )

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    offset_rows = {
        offset: instruction
        for instruction, offset in zip(candidate.instructions, offsets)
        if offset is not None
    }
    relocation_rows = [
        row
        for row in caller.relocations
        if row.symbol_name == _cc_catalog.ZDECLIENT_QSAND_CALLBACK_DECORATED_SYMBOL
    ]
    if (
        caller.symbol != _cc_catalog.ZDECLIENT_QSAND_CALLER_SYMBOL
        or caller.undefined_external_data.count(
            _cc_catalog.ZDECLIENT_QSAND_CALLBACK_DECORATED_SYMBOL
        )
        != 1
        or (
            caller.defined_external_data
            + caller.undefined_external_functions
            + caller.defined_external_functions
        ).count(_cc_catalog.ZDECLIENT_QSAND_CALLBACK_DECORATED_SYMBOL)
        != 0
        or len(relocation_rows) != 1
        or relocation_rows[0].offset != 0x02
        or relocation_rows[0].type != IMAGE_REL_I386_DIR32
        or struct.unpack_from("<I", caller.data, 0x02)[0] != 0
        or not all(caller.relocation_mask[index] for index in range(0x02, 0x06))
        or caller.data[0x01:0x06] != bytes.fromhex("a1 00 00 00 00")
        or caller.data[0x0E:0x10] != bytes.fromhex("ff d0")
        or 0x01 not in offset_rows
        or _cc_cfg._instruction_mnemonic(offset_rows[0x01]) != "mov"
        or _cc_catalog.ZDECLIENT_QSAND_CALLBACK_DECORATED_SYMBOL
        not in _cc_cfg._instruction_operand(offset_rows[0x01])
        or 0x0E not in offset_rows
        or _cc_cfg._instruction_mnemonic(offset_rows[0x0E]) != "call"
        or _cc_cfg._instruction_operand(offset_rows[0x0E]).strip().casefold() != "eax"
    ):
        raise ValueError(
            "zDEClient QSand callback bridge requires the exact offset-1 A1 "
            "DIR32 decorated load and offset-0xe FF D0 call"
        )
    return {
        _cc_catalog.ZDECLIENT_QSAND_CALLBACK_DECORATED_SYMBOL: (
            _cc_catalog.ZDECLIENT_QSAND_CALLBACK_IDENTITY
        )
    }


def _zdeclient_crater_callback_authority_provenance(
    document: ProgressDocument,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    target: Any,
    expected: Sequence[Mapping[str, Any]],
) -> dict[str, Any]:
    """Compose the exact reviewed Crater callback authority package."""

    manifest_payload: Any | None = None
    current_callback_target: Mapping[str, Any] | None = None
    current_callback_target_id = ""
    manifest_error = ""
    from _recoil.lib.verification_targets import vc5_target_registration
    try:
        current_callback_target_id, current = vc5_target_registration(
            _cc_catalog.ZDECLIENT_QSAND_CALLBACK_TARGET_MANIFEST_PATH
        )
        current_callback_target = current
        manifest_payload = load_manifest(
            _cc_catalog.ZDECLIENT_QSAND_CALLBACK_TARGET_MANIFEST_PATH,
            enforce_source_policy=False,
        )
    except (OSError, ProgressError, ValueError) as exc:
        manifest_error = str(exc)

    caller_symbol_id = _cc_catalog.ZDECLIENT_CRATER_CALLER_IDENTITY.removeprefix("symbol:")
    target_registration = document.collection("verification_targets").get(
        _cc_catalog.ZDECLIENT_CRATER_TARGET_ID
    )
    target_registration_body = (
        target_registration.get("registration")
        if isinstance(target_registration, Mapping)
        else None
    )
    caller_row = document.collection("symbols").get(caller_symbol_id)
    callback_row = document.collection("symbols").get(
        _cc_catalog.ZDECLIENT_CRATER_CALLBACK_SYMBOL_ID
    )
    storage_row = document.collection("storage_contributions").get(
        _cc_catalog.ZDECLIENT_CRATER_CALLBACK_STORAGE_ID
    )
    callback_target = document.collection("verification_targets").get(
        _cc_catalog.ZDECLIENT_QSAND_CALLBACK_TARGET_ID
    )
    callback_registration = (
        callback_target.get("registration")
        if isinstance(callback_target, Mapping)
        else None
    )
    owner = document.collection("owners").get(
        _cc_catalog.ZDECLIENT_QSAND_CALLBACK_OWNER_ID
    )
    owner_gates = owner.get("gates") if isinstance(owner, Mapping) else None
    address_metadata = (
        owner.get("address_metadata", {}).get(
            _cc_catalog.ZDECLIENT_CRATER_CALLBACK_ADDRESS
        )
        if isinstance(owner, Mapping)
        and isinstance(owner.get("address_metadata"), Mapping)
        else None
    )
    caller_edges = (
        caller_row.get("source_traceability", {}).get("source_edges", [])
        if isinstance(caller_row, Mapping)
        and isinstance(caller_row.get("source_traceability"), Mapping)
        else []
    )
    callback_edges = (
        callback_row.get("source_traceability", {}).get("source_edges", [])
        if isinstance(callback_row, Mapping)
        and isinstance(callback_row.get("source_traceability"), Mapping)
        else []
    )
    exact_manifest_rows = [
        row
        for row in tuple(getattr(manifest_payload, "data_symbols", ()))
        if getattr(row, "address", None) == _cc_catalog.ZDECLIENT_CRATER_CALLBACK_ADDRESS
    ]
    selected = None
    selected_error = ""
    try:
        selected = _cc_candidate._target_function(target, _cc_catalog.ZDECLIENT_CRATER_CALLER_ADDRESS)
    except (RuntimeError, ValueError) as exc:
        selected_error = str(exc)

    checks = {
        "caller": bool(
            caller_identity == _cc_catalog.ZDECLIENT_CRATER_CALLER_IDENTITY
            and normalize_address(caller_start)
            == _cc_catalog.ZDECLIENT_CRATER_CALLER_ADDRESS
            and normalize_address(caller_end_exclusive)
            == _cc_catalog.ZDECLIENT_CRATER_CALLER_END_EXCLUSIVE
            and isinstance(caller_row, Mapping)
            and caller_row.get("binary") == "recoil"
            and caller_row.get("kind") == "function"
            and caller_row.get("pipeline_class") == "authored"
            and caller_row.get("address") == _cc_catalog.ZDECLIENT_CRATER_CALLER_ADDRESS
            and caller_row.get("end_exclusive")
            == _cc_catalog.ZDECLIENT_CRATER_CALLER_END_EXCLUSIVE
            and caller_row.get("extent_state") == "known"
            and caller_row.get("size") == 0x30
            and caller_row.get("output_section_id") == "recoil:section:.text"
            and caller_row.get("physical_block_id") == "recoil:block:0x456ad0"
            and caller_row.get("source_traceability", {}).get("state")
            == "resolved"
            and caller_edges
            == [{
                "anchor_id": _cc_catalog.ZDECLIENT_CRATER_CALLER_ANCHOR_ID,
                "emission_context": {
                    "translation_unit": _cc_catalog.ZDECLIENT_CRATER_SOURCE_PATH
                },
                "evidence_ids": [],
                "relation": "defines",
            }]
        ),
        "data": bool(
            isinstance(callback_row, Mapping)
            and callback_row.get("binary") == "recoil"
            and callback_row.get("kind") == "data"
            and callback_row.get("disposition") == "authored"
            and callback_row.get("address")
            == _cc_catalog.ZDECLIENT_CRATER_CALLBACK_ADDRESS
            and callback_row.get("end_exclusive") == "0x539dec"
            and callback_row.get("extent_state") == "known"
            and callback_row.get("size") == 4
            and callback_row.get("output_section_id") == "recoil:section:.data"
            and callback_row.get("storage_contribution_ids")
            == [_cc_catalog.ZDECLIENT_CRATER_CALLBACK_STORAGE_ID]
            and callback_row.get("verification_target_ids")
            == [_cc_catalog.ZDECLIENT_QSAND_CALLBACK_TARGET_ID]
            and callback_row.get("source_traceability", {}).get("state")
            == "resolved"
            and callback_edges
            == [{
                "anchor_id": _cc_catalog.ZDECLIENT_CRATER_CALLBACK_ANCHOR_ID,
                "emission_context": {
                    "translation_unit": _cc_catalog.ZDECLIENT_QSAND_CALLBACK_SOURCE_PATH
                },
                "evidence_ids": [],
                "relation": "defines",
            }]
        ),
        "storage": bool(
            isinstance(storage_row, Mapping)
            and storage_row.get("binary") == "recoil"
            and storage_row.get("kind") == "data-symbol"
            and storage_row.get("output_section_id") == "recoil:section:.data"
            and storage_row.get("overlap") == "none"
            and storage_row.get("owner_ids")
            == [_cc_catalog.ZDECLIENT_QSAND_CALLBACK_OWNER_ID]
            and storage_row.get("symbol_ids")
            == [_cc_catalog.ZDECLIENT_CRATER_CALLBACK_SYMBOL_ID]
            and storage_row.get("reference", {}).get("address")
            == _cc_catalog.ZDECLIENT_CRATER_CALLBACK_ADDRESS
            and storage_row.get("reference", {}).get("extent_state")
            in {"unknown", "known"}
            and indexes.storage_by_address.get(
                _cc_catalog.ZDECLIENT_CRATER_CALLBACK_ADDRESS
            ) == _cc_catalog.ZDECLIENT_CRATER_CALLBACK_IDENTITY
        ),
        "owner": bool(
            isinstance(owner, Mapping)
            and owner.get("binary") == "recoil"
            and owner.get("kind") == "data-owner"
            and owner.get("lifecycle_state") == "active"
            and owner.get("source_paths")
            == [
                _cc_catalog.ZDECLIENT_QSAND_CALLBACK_SOURCE_PATH,
                _cc_catalog.ZDECLIENT_QSAND_CALLBACK_HEADER_PATH,
            ]
            and isinstance(owner_gates, Mapping)
            and owner_gates.get("boundary") == "accepted"
            and owner_gates.get("source") == "accepted"
            and owner_gates.get("data") == "accepted"
            and owner_gates.get("owner_linkage") == "accepted"
            and address_metadata == {
                "group": "data.effects_weapons",
                "name": _cc_catalog.ZDECLIENT_CRATER_CALLBACK_NAME,
                "source_path": _cc_catalog.ZDECLIENT_QSAND_CALLBACK_SOURCE_PATH,
                "target": "zdeclient_net_relay_callback_globals",
            }
        ),
        "caller_target": bool(
            selected is not None
            and getattr(target, "name", "") == _cc_catalog.ZDECLIENT_CRATER_TARGET_NAME
            and getattr(target, "target_binary", "") == "recoil"
            and Path(str(getattr(target, "manifest_path", ""))).resolve()
            == _cc_catalog.ZDECLIENT_CRATER_TARGET_MANIFEST.resolve()
            and getattr(target, "source_from", "")
            == _cc_catalog.ZDECLIENT_CRATER_SOURCE_PATH
            and bool(getattr(target, "check_translation_unit_function_order", False))
            and getattr(selected, "symbol", "") == _cc_catalog.ZDECLIENT_CRATER_CALLER_SYMBOL
            and getattr(selected, "pipeline_class", "") == "authored"
            and getattr(selected, "authored_order_role", "") == "authored-body"
            and bool(getattr(selected, "required_presence", False))
            and isinstance(target_registration, Mapping)
            and target_registration.get("binary") == "recoil"
            and target_registration.get("kind") == "vc5"
            and target_registration.get("name") == _cc_catalog.ZDECLIENT_CRATER_TARGET_NAME
            and isinstance(target_registration_body, Mapping)
            and target_registration_body.get("manifest_path")
            == _cc_catalog.ZDECLIENT_CRATER_TARGET_MANIFEST.relative_to(REPO_ROOT).as_posix()
            and target_registration_body.get("source_from")
            == _cc_catalog.ZDECLIENT_CRATER_SOURCE_PATH
            and target_registration_body.get("function_order_scope") == "authored"
            and target_registration_body.get("function_addresses", []).count(
                _cc_catalog.ZDECLIENT_CRATER_CALLER_ADDRESS
            ) == 1
        ),
        "callback_target": bool(
            isinstance(callback_target, Mapping)
            and callback_target.get("binary") == "recoil"
            and callback_target.get("kind") == "vc5"
            and callback_target.get("name")
            == "zdeclient_net_relay_callback_globals"
            and callback_target.get("symbol_ids")
            == [
                _cc_catalog.ZDECLIENT_QSAND_CALLBACK_SYMBOL_ID,
                _cc_catalog.ZDECLIENT_CRATER_CALLBACK_SYMBOL_ID,
            ]
            and callback_target.get("unresolved_addresses") in (None, [])
            and isinstance(callback_registration, Mapping)
            and callback_registration.get("manifest_path")
            == _cc_catalog.ZDECLIENT_QSAND_CALLBACK_TARGET_MANIFEST
            and callback_registration.get("source_from")
            == _cc_catalog.ZDECLIENT_QSAND_CALLBACK_SOURCE_PATH
            and callback_registration.get("data_addresses")
            == [
                _cc_catalog.ZDECLIENT_QSAND_CALLBACK_ADDRESS,
                _cc_catalog.ZDECLIENT_CRATER_CALLBACK_ADDRESS,
            ]
            and current_callback_target_id == _cc_catalog.ZDECLIENT_QSAND_CALLBACK_TARGET_ID
            and isinstance(current_callback_target, Mapping)
            and current_callback_target.get("binary")
            == callback_target.get("binary")
            and current_callback_target.get("kind")
            == callback_target.get("kind")
            and current_callback_target.get("name")
            == callback_target.get("name")
            and isinstance(current_callback_target.get("registration"), Mapping)
            and current_callback_target["registration"].get("manifest_path")
            == callback_registration.get("manifest_path")
            and current_callback_target["registration"].get("source_from")
            == callback_registration.get("source_from")
            and current_callback_target["registration"].get("data_addresses")
            == callback_registration.get("data_addresses")
        ),
        "manifest": bool(
            manifest_error == ""
            and getattr(manifest_payload, "name", "")
            == "zdeclient_net_relay_callback_globals"
            and getattr(manifest_payload, "source_from", "")
            == _cc_catalog.ZDECLIENT_QSAND_CALLBACK_SOURCE_PATH
            and tuple(getattr(manifest_payload, "source_files", ()))
            == (_cc_catalog.ZDECLIENT_QSAND_CALLBACK_SOURCE_PATH,)
            and len(exact_manifest_rows) == 1
            and getattr(exact_manifest_rows[0], "symbol", None)
            == _cc_catalog.ZDECLIENT_CRATER_CALLBACK_DECORATED_SYMBOL
            and getattr(exact_manifest_rows[0], "symbol_regex", None) is None
            and getattr(exact_manifest_rows[0], "name", None)
            == _cc_catalog.ZDECLIENT_CRATER_CALLBACK_NAME
            and getattr(exact_manifest_rows[0], "bn_name", None)
            == _cc_catalog.ZDECLIENT_CRATER_CALLBACK_NAME
            and getattr(exact_manifest_rows[0], "byte_length", None) == 4
        ),
        "retail_contract": sum(
            row.get("form") == "call"
            and row.get("dispatch") == "indirect"
            and row.get("identity_kind") == "direct"
            and row.get("target_identity")
            == _cc_catalog.ZDECLIENT_CRATER_CALLBACK_RETAIL_TARGET_IDENTITY
            and row.get("storage_identity")
            == _cc_catalog.ZDECLIENT_CRATER_CALLBACK_IDENTITY
            and row.get("slot_displacement") is None
            and row.get("cleanup_bytes") is None
            for row in expected
        ) == 1,
    }
    return {
        "kind": "call-contract-callback-authority-dependency",
        "contract_version": 1,
        "candidate_expected_truth": False,
        "caller_symbol_id": caller_symbol_id,
        "caller_address": _cc_catalog.ZDECLIENT_CRATER_CALLER_ADDRESS,
        "caller_end_exclusive": _cc_catalog.ZDECLIENT_CRATER_CALLER_END_EXCLUSIVE,
        "caller_target_id": _cc_catalog.ZDECLIENT_CRATER_TARGET_ID,
        "authority_entity_ids": [
            _cc_catalog.ZDECLIENT_CRATER_CALLBACK_SYMBOL_ID,
            _cc_catalog.ZDECLIENT_CRATER_CALLBACK_STORAGE_ID,
            _cc_catalog.ZDECLIENT_QSAND_CALLBACK_OWNER_ID,
            _cc_catalog.ZDECLIENT_QSAND_CALLBACK_TARGET_ID,
        ],
        "authority_paths": [
            _cc_catalog.ZDECLIENT_QSAND_CALLBACK_SOURCE_PATH,
            _cc_catalog.ZDECLIENT_QSAND_CALLBACK_HEADER_PATH,
            _cc_catalog.ZDECLIENT_QSAND_CALLBACK_TARGET_MANIFEST,
        ],
        "checks": checks,
        "missing_authority_gates": sorted(
            key for key, value in checks.items() if value is not True
        ),
        "manifest_error": manifest_error,
        "selected_caller_error": selected_error,
        "complete": all(checks.values()),
        "repair_route": "governed-authority-or-tool-maintenance",
        "source_repairable": False,
    }


def _zdeclient_crater_callback_candidate_register_bridge(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, str]:
    """Bind only the exact Crater callback load to reviewed storage."""

    if (
        caller_identity != _cc_catalog.ZDECLIENT_CRATER_CALLER_IDENTITY
        or normalize_address(caller_start) != _cc_catalog.ZDECLIENT_CRATER_CALLER_ADDRESS
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.ZDECLIENT_CRATER_CALLER_END_EXCLUSIVE
    ):
        return {}
    caller = candidate.caller_definition
    if caller is None or _cc_catalog.ZDECLIENT_CRATER_CALLBACK_DECORATED_SYMBOL not in (
        caller.undefined_external_data
        + caller.defined_external_data
        + caller.undefined_external_functions
        + caller.defined_external_functions
    ):
        return {}

    authority = _zdeclient_crater_callback_authority_provenance(
        document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        target=candidate.target,
        expected=expected,
    )
    if authority["complete"] is not True:
        raise _cc_errors.CandidateCallbackAuthorityError(
            provenance=authority,
            message=(
                "zDEClient Crater callback bridge requires exact synchronized "
                "caller, data/storage, owner, and manifest authority; "
                "missing authority gates: "
                + ", ".join(authority["missing_authority_gates"])
            ),
        )

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    offset_rows = {
        offset: instruction
        for instruction, offset in zip(candidate.instructions, offsets)
        if offset is not None
    }
    relocation_rows = [
        row
        for row in caller.relocations
        if row.symbol_name == _cc_catalog.ZDECLIENT_CRATER_CALLBACK_DECORATED_SYMBOL
    ]
    if (
        caller.symbol != _cc_catalog.ZDECLIENT_CRATER_CALLER_SYMBOL
        or caller.undefined_external_data.count(
            _cc_catalog.ZDECLIENT_CRATER_CALLBACK_DECORATED_SYMBOL
        ) != 1
        or (
            caller.defined_external_data
            + caller.undefined_external_functions
            + caller.defined_external_functions
        ).count(_cc_catalog.ZDECLIENT_CRATER_CALLBACK_DECORATED_SYMBOL) != 0
        or len(relocation_rows) != 1
        or relocation_rows[0].offset != 0x01
        or relocation_rows[0].type != IMAGE_REL_I386_DIR32
        or struct.unpack_from("<I", caller.data, 0x01)[0] != 0
        or not all(caller.relocation_mask[index] for index in range(0x01, 0x05))
        or caller.data[0x00:0x05] != bytes.fromhex("a1 00 00 00 00")
        or caller.data[0x0C:0x0E] != bytes.fromhex("ff d0")
        or 0x00 not in offset_rows
        or _cc_cfg._instruction_mnemonic(offset_rows[0x00]) != "mov"
        or _cc_catalog.ZDECLIENT_CRATER_CALLBACK_DECORATED_SYMBOL
        not in _cc_cfg._instruction_operand(offset_rows[0x00])
        or 0x0C not in offset_rows
        or _cc_cfg._instruction_mnemonic(offset_rows[0x0C]) != "call"
        or _cc_cfg._instruction_operand(offset_rows[0x0C]).strip().casefold() != "eax"
    ):
        raise ValueError(
            "zDEClient Crater callback bridge requires the exact offset-0 A1 "
            "DIR32 decorated load and offset-0xc FF D0 call"
        )
    return {
        _cc_catalog.ZDECLIENT_CRATER_CALLBACK_DECORATED_SYMBOL: (
            _cc_catalog.ZDECLIENT_CRATER_CALLBACK_IDENTITY
        )
    }


def _cls_util_exact_registered_target(
    document: ProgressDocument,
    *,
    target_id: str,
    manifest_path: Path,
    target_name: str,
    source_path: str,
    address: str,
    symbol: str,
    name: str,
    selected_target: Any | None = None,
) -> Any:
    """Require one synchronized exact-symbol VC5 contribution row."""

    target_row = document.collection("verification_targets").get(target_id)
    if not isinstance(target_row, Mapping):
        raise ValueError(
            f"cls_util identity closure lacks target {target_id!r}"
        )
    from _recoil.lib.verification_targets import vc5_target_registration

    try:
        current_id, current_row = vc5_target_registration(manifest_path)
    except (OSError, ValueError) as exc:
        raise ValueError(
            "cls_util identity closure cannot synchronize registered "
            f"target {target_id!r}: {exc}"
        ) from exc
    if (
        current_id != target_id
        or target_row != current_row
        or target_row.get("binary") != "recoil"
        or target_row.get("kind") != "vc5"
        or target_row.get("name") != target_name
    ):
        raise ValueError(
            "cls_util identity closure requires an exact synchronized "
            f"registered target {target_id!r}"
        )

    target = selected_target
    if target is None:
        target = _cc_source._call_contract_cached_manifest(document, manifest_path)
    contribution_rows = [
        (entry, row)
        for entry in getattr(target, "translation_unit_function_order", ())
        for row in getattr(entry, "functions", ())
        if normalize_address(str(getattr(row, "address", ""))) == address
    ]
    if (
        getattr(target, "name", "") != target_name
        or getattr(target, "target_binary", "") != "recoil"
        or Path(str(getattr(target, "manifest_path", ""))).resolve()
        != manifest_path.resolve()
        or not bool(
            getattr(target, "check_translation_unit_function_order", False)
        )
        or tuple(getattr(target, "source_files", ()))
        not in {(), (source_path,)}
        or len(contribution_rows) != 1
    ):
        raise ValueError(
            "cls_util identity closure requires the exact loaded target, "
            f"source listing, and unique contribution for {address}"
        )
    contribution, contribution_row = contribution_rows[0]
    if (
        getattr(contribution, "source_from", "") != source_path
        or getattr(contribution, "order_scope", "") != "authored"
        or getattr(contribution_row, "symbol", "") != symbol
        or getattr(contribution_row, "symbol_regex", None) is not None
        or getattr(contribution_row, "name", "") != name
        or getattr(contribution_row, "pipeline_class", "") != "authored"
        or getattr(contribution_row, "authored_order_role", "")
        != "authored-body"
        or not bool(getattr(contribution_row, "required_presence", False))
        or not bool(getattr(contribution_row, "full_order_gate", False))
    ):
        raise ValueError(
            "cls_util identity closure requires one exact authored "
            f"registered contribution for {address}"
        )
    return target


def _cls_util_resolved_tracker_symbol(
    document: ProgressDocument,
    *,
    symbol_id: str,
    address: str,
    end_exclusive: str,
    size: int,
    name: str,
    block_id: str,
    source_path: str,
    anchor_id: str,
    target_id: str,
) -> None:
    """Require the exact distinct symbol, source edge, and physical block."""

    symbol = document.collection("symbols").get(symbol_id)
    block = document.collection("physical_blocks").get(block_id)
    source_traceability = (
        symbol.get("source_traceability")
        if isinstance(symbol, Mapping)
        else None
    )
    source_edges = (
        source_traceability.get("source_edges")
        if isinstance(source_traceability, Mapping)
        else None
    )
    if (
        not isinstance(symbol, Mapping)
        or symbol.get("binary") != "recoil"
        or symbol.get("kind") != "function"
        or symbol.get("pipeline_class") != "authored"
        or symbol.get("authored_order_role") != "authored-body"
        or symbol.get("address") != address
        or symbol.get("end_exclusive") != end_exclusive
        or symbol.get("extent_state") != "known"
        or symbol.get("size") != size
        or symbol.get("navigation_name") != name
        or symbol.get("output_section_id") != "recoil:section:.text"
        or symbol.get("ownership_state") != "primary-owned"
        or symbol.get("physical_block_id") != block_id
        or not isinstance(symbol.get("verification_target_ids"), list)
        or symbol["verification_target_ids"].count(target_id) != 1
        or not isinstance(source_traceability, Mapping)
        or source_traceability.get("state") != "resolved"
        or source_traceability.get("reason_code") is not None
        or source_edges
        != [
            {
                "anchor_id": anchor_id,
                "emission_context": {"translation_unit": source_path},
                "evidence_ids": [],
                "relation": "defines",
            }
        ]
    ):
        raise ValueError(
            "cls_util identity closure requires exact authored tracker "
            f"symbol/source provenance for {symbol_id}"
        )
    accepted_order_facts = (
        block.get("accepted_order_facts")
        if isinstance(block, Mapping)
        else None
    )
    if (
        not isinstance(block, Mapping)
        or block.get("binary") != "recoil"
        or block.get("row_kind") != "physical-source-block"
        or block.get("contribution_kind") != "authored"
        or block.get("source_path") != source_path
        or block.get("agent_source_path") != source_path
        or block.get("original_source_path") != source_path
        or not isinstance(block.get("contribution_ids"), list)
        or block["contribution_ids"].count(symbol_id) != 1
        or not isinstance(accepted_order_facts, Mapping)
        or accepted_order_facts.get("target_id") != target_id
        or accepted_order_facts.get("phase") != "authored-function-order"
        or accepted_order_facts.get("validation_mode") != "live"
        or not isinstance(accepted_order_facts.get("matched_identities"), list)
        or accepted_order_facts["matched_identities"].count(symbol_id) != 1
    ):
        raise ValueError(
            "cls_util identity closure requires exact authored physical-"
            f"block provenance for {symbol_id}"
        )


def _zclass_freeall_source_provenance(
    document: ProgressDocument,
) -> None:
    """Require the unique current header declaration and two TU definitions."""

    repository_inventory = load_repository_path_inventory(REPO_ROOT)
    root = repository_inventory.repository_root
    config = load_final_build_config(DEFAULT_FINAL_BUILD_MANIFEST)
    contexts: dict[str, tuple[str, ...]] = {}
    configured_sources, _stale_diagnostics = (
        _cc_source._call_contract_final_build_source_map(
            config,
            inventory=repository_inventory,
        )
    )
    for rows in configured_sources.values():
        if len(rows) != 1:
            raise ValueError(
                "zClass FreeAll identity bridge final-build source is duplicated"
            )
        relative, resolved = rows[0]
        contexts[relative] = _cc_source._call_contract_vc5_preprocessor_defines(
            config, resolved
        )
    if (
        list(contexts).count(_cc_catalog.ZCLASS_FREEALL_CALLER_SOURCE_PATH) != 1
        or list(contexts).count(_cc_catalog.ZCLASS_FREEALL_TARGET_SOURCE_PATH) != 1
    ):
        raise ValueError(
            "zClass FreeAll identity bridge requires unique final-build caller "
            "and definition translation units"
        )
    resolver = _cc_source._call_contract_callable_inventory_resolver(
        document, root=root
    )
    caller_context = contexts[_cc_catalog.ZCLASS_FREEALL_CALLER_SOURCE_PATH]
    target_context = contexts[_cc_catalog.ZCLASS_FREEALL_TARGET_SOURCE_PATH]
    freeall_key = CallableKey(
        qualified_name=_cc_catalog.ZCLASS_FREEALL_TARGET_NAME,
        parameter_shapes=(),
    )
    shutdown_key = CallableKey(
        qualified_name=_cc_catalog.ZCLASS_FREEALL_CALLER_NAME,
        parameter_shapes=(),
    )
    header_inventories = tuple(
        resolver(_cc_catalog.ZCLASS_FREEALL_HEADER_PATH, context)
        for context in sorted({caller_context, target_context})
    )
    caller_inventory = resolver(
        _cc_catalog.ZCLASS_FREEALL_CALLER_SOURCE_PATH, caller_context
    )
    target_inventory = resolver(
        _cc_catalog.ZCLASS_FREEALL_TARGET_SOURCE_PATH, target_context
    )
    if (
        not header_inventories
        or any(
            freeall_key not in inventory.declarations
            or freeall_key in inventory.definitions
            or shutdown_key not in inventory.declarations
            for inventory in header_inventories
        )
        or freeall_key not in target_inventory.definitions
        or freeall_key in target_inventory.declarations
        or shutdown_key in target_inventory.definitions
        or freeall_key in caller_inventory.declarations
        or freeall_key in caller_inventory.definitions
        or shutdown_key not in caller_inventory.definitions
    ):
        raise ValueError(
            "zClass FreeAll identity bridge requires its unique qualified "
            "header declaration, List.c definition, and cls_util.c caller"
        )


def _zclass_freeall_candidate_identity_bridge(
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
    """Bind one exact candidate-only /Gr FreeAll spelling to retail identity.

    Retail expected truth is already present in ``expected``.  The candidate
    spelling, COD listing, and COFF relocation are accepted only as provenance
    for resolving that candidate observation to the distinct governed target.
    """

    if normalize_address(caller_start) != _cc_catalog.ZCLASS_FREEALL_CALLER_ADDRESS:
        return {}
    if (
        caller_identity != _cc_catalog.ZCLASS_FREEALL_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.ZCLASS_FREEALL_CALLER_END_EXCLUSIVE
    ):
        raise ValueError(
            "zClass FreeAll identity bridge requires its exact reviewed caller"
        )

    mentions = [
        instruction
        for instruction in candidate.instructions
        if _cc_catalog.ZCLASS_FREEALL_CANDIDATE_SYMBOL.casefold()
        in _cc_cfg._instruction_operand(instruction).casefold()
    ]
    if not mentions:
        return {}
    if (
        len(mentions) != 1
        or _cc_cfg._instruction_mnemonic(mentions[0]) != "call"
        or _cc_cfg._instruction_operand(mentions[0]).strip()
        != _cc_catalog.ZCLASS_FREEALL_CANDIDATE_SYMBOL
    ):
        raise ValueError(
            "zClass FreeAll identity bridge accepts one exact direct CALL; "
            "aliases, case drift, duplicates, and indirect forms are forbidden"
        )

    caller_decoded = decode_vc5_zero_argument_callable_identity(
        _cc_catalog.ZCLASS_FREEALL_CANDIDATE_SYMBOL
    )
    target_decoded = decode_vc5_zero_argument_callable_identity(
        _cc_catalog.ZCLASS_FREEALL_REGISTERED_SYMBOL
    )
    expected_key = CallableKey(
        qualified_name=_cc_catalog.ZCLASS_FREEALL_TARGET_NAME,
        parameter_shapes=(),
    )
    if (
        caller_decoded is None
        or target_decoded is None
        or caller_decoded.callable_key != expected_key
        or target_decoded.callable_key != expected_key
        or caller_decoded.calling_convention != "__fastcall"
        or target_decoded.calling_convention != "__cdecl"
        or caller_decoded.return_shape != "void"
        or target_decoded.return_shape != "void"
    ):
        raise ValueError(
            "zClass FreeAll identity bridge requires the exact qualified "
            "zero-argument void candidate/registered ABI-name pair"
        )

    _cls_util_exact_registered_target(
        document,
        target_id=_cc_catalog.ZCLASS_FREEALL_CALLER_TARGET_ID,
        manifest_path=_cc_catalog.ZCLASS_FREEALL_CALLER_TARGET_MANIFEST,
        target_name=_cc_catalog.ZCLASS_FREEALL_CALLER_TARGET_NAME,
        source_path=_cc_catalog.ZCLASS_FREEALL_CALLER_SOURCE_PATH,
        address=_cc_catalog.ZCLASS_FREEALL_CALLER_ADDRESS,
        symbol=_cc_catalog.ZCLASS_FREEALL_CALLER_SYMBOL,
        name=_cc_catalog.ZCLASS_FREEALL_CALLER_NAME,
        selected_target=candidate.target,
    )
    _cls_util_exact_registered_target(
        document,
        target_id=_cc_catalog.ZCLASS_FREEALL_TARGET_ID,
        manifest_path=_cc_catalog.ZCLASS_FREEALL_TARGET_MANIFEST,
        target_name=_cc_catalog.ZCLASS_FREEALL_TARGET_ID.removeprefix("recoil:vc5-target:"),
        source_path=_cc_catalog.ZCLASS_FREEALL_TARGET_SOURCE_PATH,
        address=_cc_catalog.ZCLASS_FREEALL_TARGET_ADDRESS,
        symbol=_cc_catalog.ZCLASS_FREEALL_REGISTERED_SYMBOL,
        name=_cc_catalog.ZCLASS_FREEALL_TARGET_NAME,
    )
    _cls_util_resolved_tracker_symbol(
        document,
        symbol_id=_cc_catalog.ZCLASS_FREEALL_CALLER_SYMBOL_ID,
        address=_cc_catalog.ZCLASS_FREEALL_CALLER_ADDRESS,
        end_exclusive=_cc_catalog.ZCLASS_FREEALL_CALLER_END_EXCLUSIVE,
        size=len(_cc_catalog.ZCLASS_FREEALL_CALLER_BODY),
        name=_cc_catalog.ZCLASS_FREEALL_CALLER_NAME,
        block_id=_cc_catalog.ZCLASS_FREEALL_CALLER_BLOCK_ID,
        source_path=_cc_catalog.ZCLASS_FREEALL_CALLER_SOURCE_PATH,
        anchor_id=_cc_catalog.ZCLASS_FREEALL_CALLER_ANCHOR_ID,
        target_id=_cc_catalog.ZCLASS_FREEALL_CALLER_TARGET_ID,
    )
    _cls_util_resolved_tracker_symbol(
        document,
        symbol_id=_cc_catalog.ZCLASS_FREEALL_TARGET_SYMBOL_ID,
        address=_cc_catalog.ZCLASS_FREEALL_TARGET_ADDRESS,
        end_exclusive=_cc_catalog.ZCLASS_FREEALL_TARGET_END_EXCLUSIVE,
        size=0x30,
        name=_cc_catalog.ZCLASS_FREEALL_TARGET_NAME,
        block_id=_cc_catalog.ZCLASS_FREEALL_TARGET_BLOCK_ID,
        source_path=_cc_catalog.ZCLASS_FREEALL_TARGET_SOURCE_PATH,
        anchor_id=_cc_catalog.ZCLASS_FREEALL_TARGET_ANCHOR_ID,
        target_id=_cc_catalog.ZCLASS_FREEALL_TARGET_ID,
    )
    if (
        indexes.by_address.get(_cc_catalog.ZCLASS_FREEALL_CALLER_ADDRESS)
        != _cc_catalog.ZCLASS_FREEALL_CALLER_IDENTITY
        or indexes.by_address.get(_cc_catalog.ZCLASS_FREEALL_TARGET_ADDRESS)
        != _cc_catalog.ZCLASS_FREEALL_TARGET_IDENTITY
        or [
            address
            for address, identity in indexes.by_address.items()
            if identity == _cc_catalog.ZCLASS_FREEALL_TARGET_IDENTITY
        ]
        != [_cc_catalog.ZCLASS_FREEALL_TARGET_ADDRESS]
        or _cc_catalog.ZCLASS_FREEALL_TARGET_IDENTITY in indexes.provider_ids
        or _cc_catalog.ZCLASS_FREEALL_CANDIDATE_SYMBOL in indexes.by_candidate_name
        or _cc_catalog.ZCLASS_FREEALL_CANDIDATE_SYMBOL in indexes.storage_by_name
        or _cc_catalog.ZCLASS_FREEALL_CANDIDATE_SYMBOL in bridge_names
    ):
        raise ValueError(
            "zClass FreeAll identity bridge requires collision-free unique "
            "authored caller and target identities"
        )
    _zclass_freeall_source_provenance(document)

    exact_expected = {
        "ordinal": _cc_catalog.ZCLASS_FREEALL_CALL_ORDINAL,
        "form": "call",
        "dispatch": "direct",
        "identity_kind": "direct",
        "target_identity": _cc_catalog.ZCLASS_FREEALL_TARGET_IDENTITY,
        "storage_identity": "",
        "slot_displacement": None,
        "cleanup_bytes": None,
    }
    expected_rows = [
        row
        for row in expected
        if row.get("target_identity") == _cc_catalog.ZCLASS_FREEALL_TARGET_IDENTITY
    ]
    if expected_rows != [exact_expected]:
        raise ValueError(
            "zClass FreeAll identity bridge requires one exact ordinal-1 "
            "retail authored direct-call contract"
        )

    definition = candidate.caller_definition
    relocation_rows = (
        tuple(
            (row.offset, row.type, row.symbol_name)
            for row in definition.relocations
        )
        if definition is not None
        else ()
    )
    expected_mask = tuple(
        any(offset <= index < offset + 4 for offset, _kind, _name in (
            _cc_catalog.ZCLASS_FREEALL_CALLER_RELOCATIONS
        ))
        for index in range(len(_cc_catalog.ZCLASS_FREEALL_CALLER_BODY))
    )
    if (
        definition is None
        or definition.symbol != _cc_catalog.ZCLASS_FREEALL_CALLER_SYMBOL
        or definition.data != _cc_catalog.ZCLASS_FREEALL_CALLER_BODY
        or definition.section_index
        != _cc_catalog.ZCLASS_FREEALL_CALLER_COFF_SECTION_INDEX
        or definition.section_start != 0
        or definition.section_end != len(_cc_catalog.ZCLASS_FREEALL_CALLER_BODY)
        or relocation_rows != _cc_catalog.ZCLASS_FREEALL_CALLER_RELOCATIONS
        or definition.relocation_mask != expected_mask
        or definition.undefined_external_functions.count(
            _cc_catalog.ZCLASS_FREEALL_CANDIDATE_SYMBOL
        )
        != 1
        or _cc_catalog.ZCLASS_FREEALL_CANDIDATE_SYMBOL
        in (
            definition.defined_external_functions
            + definition.undefined_external_data
            + definition.defined_external_data
        )
        or _cc_catalog.ZCLASS_FREEALL_REGISTERED_SYMBOL
        in (
            definition.undefined_external_functions
            + definition.defined_external_functions
            + definition.undefined_external_data
            + definition.defined_external_data
        )
    ):
        raise ValueError(
            "zClass FreeAll identity bridge requires the exact complete caller "
            "COFF body, relocation partition, and unique undefined symbol"
        )

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    encoded = bytearray()
    expected_offsets: list[int] = []
    for instruction in candidate.instructions:
        expected_offsets.append(len(encoded))
        try:
            encoded.extend(int(value, 16) for value in instruction.bytes)
        except ValueError as exc:
            raise ValueError(
                "zClass FreeAll identity bridge COD contains non-byte tokens"
            ) from exc
    call_index = candidate.instructions.index(mentions[0])
    if (
        tuple(expected_offsets) != offsets
        or bytes(encoded) != _cc_catalog.ZCLASS_FREEALL_CALLER_BODY[: len(encoded)]
        or len(encoded) != 0x59
        or _cc_catalog.ZCLASS_FREEALL_CALLER_BODY[len(encoded) :] != b"\x90" * 7
        or offsets[call_index] != _cc_catalog.ZCLASS_FREEALL_CALL_OFFSET
        or tuple(mentions[0].bytes) != ("e8", "00", "00", "00", "00")
        or candidate.local_control_flow_indices
        or candidate.local_control_flow_targets
    ):
        raise ValueError(
            "zClass FreeAll identity bridge requires one complete contiguous "
            "COD procedure with the exact offset-0xb direct call"
        )
    references = tuple(
        relocation
        for relocation in definition.relocations
        if relocation.symbol_name == _cc_catalog.ZCLASS_FREEALL_CANDIDATE_SYMBOL
    )
    if (
        len(references) != 1
        or references[0].offset != _cc_catalog.ZCLASS_FREEALL_RELOCATION_OFFSET
        or references[0].type != IMAGE_REL_I386_REL32
        or struct.unpack_from(
            "<I", definition.data, _cc_catalog.ZCLASS_FREEALL_RELOCATION_OFFSET
        )[0]
        != 0
        or not all(
            definition.relocation_mask[index]
            for index in range(
                _cc_catalog.ZCLASS_FREEALL_RELOCATION_OFFSET,
                _cc_catalog.ZCLASS_FREEALL_RELOCATION_OFFSET + 4,
            )
        )
    ):
        raise ValueError(
            "zClass FreeAll identity bridge requires one zero-addend fully "
            "masked E8 REL32 candidate relocation"
        )
    return {
        _cc_catalog.ZCLASS_FREEALL_CANDIDATE_SYMBOL: _cc_catalog.ZCLASS_FREEALL_TARGET_IDENTITY
    }


def _zclass_reset_zbd_tracker_source_provenance(
    document: ProgressDocument,
) -> None:
    """Require the reviewed no-literal cls_zbd prefix without upgrading it."""

    symbol = document.collection("symbols").get(
        _cc_catalog.ZCLASS_RESET_ZBD_TARGET_SYMBOL_ID
    )
    block = document.collection("physical_blocks").get(
        _cc_catalog.ZCLASS_RESET_ZBD_TARGET_BLOCK_ID
    )
    trace = (
        symbol.get("source_traceability")
        if isinstance(symbol, Mapping)
        else None
    )
    placement_exceptions = (
        symbol.get("placement_exceptions")
        if isinstance(symbol, Mapping)
        else None
    )
    if (
        not isinstance(symbol, Mapping)
        or symbol.get("binary") != "recoil"
        or symbol.get("kind") != "function"
        or symbol.get("pipeline_class") != "authored"
        or symbol.get("authored_order_role") != "authored-body"
        or symbol.get("address") != _cc_catalog.ZCLASS_RESET_ZBD_TARGET_ADDRESS
        or symbol.get("end_exclusive")
        != _cc_catalog.ZCLASS_RESET_ZBD_TARGET_END_EXCLUSIVE
        or symbol.get("extent_state") != "known"
        or symbol.get("size") != 0x10
        or symbol.get("navigation_name") != _cc_catalog.ZCLASS_RESET_ZBD_TARGET_NAME
        or symbol.get("output_section_id") != "recoil:section:.text"
        or symbol.get("ownership_state") != "primary-owned"
        or symbol.get("physical_block_id")
        != _cc_catalog.ZCLASS_RESET_ZBD_TARGET_BLOCK_ID
        or not isinstance(symbol.get("verification_target_ids"), list)
        or symbol["verification_target_ids"].count(
            _cc_catalog.ZCLASS_RESET_ZBD_TARGET_ID
        )
        != 1
        or trace
        != {
            "reason_code": "detached-or-unsupported-source-topology",
            "source_edges": [],
            "state": "unresolved",
        }
        or placement_exceptions
        != [{
            "address": _cc_catalog.ZCLASS_RESET_ZBD_TARGET_ADDRESS,
            "classification": "mapped no-literal cls_zbd.c prefix",
            "name": _cc_catalog.ZCLASS_RESET_ZBD_TARGET_NAME,
            "physical_block": _cc_catalog.ZCLASS_RESET_ZBD_TARGET_SOURCE_PATH,
            "summary": (
                "One-function ZBD-path reset prefix immediately before "
                "literal-backed cls_zbd.c helpers; no separate cls_path.c "
                "source file is proven"
            ),
        }]
    ):
        raise ValueError(
            "cls_util ResetCurrentZbdPath bridge requires its exact authored "
            "tracker extent and reviewed unresolved placement boundary"
        )
    facts = (
        block.get("accepted_order_facts")
        if isinstance(block, Mapping)
        else None
    )
    if (
        not isinstance(block, Mapping)
        or block.get("binary") != "recoil"
        or block.get("row_kind") != "physical-source-block"
        or block.get("contribution_kind") != "authored"
        or block.get("start") != _cc_catalog.ZCLASS_RESET_ZBD_TARGET_ADDRESS
        or block.get("end_exclusive") != "0x4558f0"
        or block.get("source_path") != _cc_catalog.ZCLASS_RESET_ZBD_TARGET_SOURCE_PATH
        or block.get("agent_source_path")
        != _cc_catalog.ZCLASS_RESET_ZBD_TARGET_SOURCE_PATH
        or block.get("original_source_path")
        != _cc_catalog.ZCLASS_RESET_ZBD_TARGET_SOURCE_PATH
        or not isinstance(block.get("contribution_ids"), list)
        or block["contribution_ids"].count(
            _cc_catalog.ZCLASS_RESET_ZBD_TARGET_SYMBOL_ID
        )
        != 1
        or not isinstance(facts, Mapping)
        or facts.get("target_id") != _cc_catalog.ZCLASS_RESET_ZBD_TARGET_ID
        or facts.get("phase") != "authored-function-order"
        or facts.get("validation_mode") != "live"
        or not isinstance(facts.get("matched_identities"), list)
        or facts["matched_identities"].count(
            _cc_catalog.ZCLASS_RESET_ZBD_TARGET_SYMBOL_ID
        )
        != 1
    ):
        raise ValueError(
            "cls_util ResetCurrentZbdPath bridge requires the exact accepted "
            "cls_zbd physical-block contribution"
        )


def _zclass_reset_zbd_source_provenance(
    document: ProgressDocument,
) -> None:
    """Prove the declaration/definition/caller split under VC5 contexts."""

    repository_inventory = load_repository_path_inventory(REPO_ROOT)
    root = repository_inventory.repository_root
    config = load_final_build_config(DEFAULT_FINAL_BUILD_MANIFEST)
    contexts: dict[str, tuple[str, ...]] = {}
    configured_sources, _stale_diagnostics = (
        _cc_source._call_contract_final_build_source_map(
            config,
            inventory=repository_inventory,
        )
    )
    for rows in configured_sources.values():
        if len(rows) != 1:
            raise ValueError(
                "cls_util ResetCurrentZbdPath bridge final-build source is duplicated"
            )
        relative, resolved = rows[0]
        if relative in contexts:
            raise ValueError(
                "cls_util ResetCurrentZbdPath bridge final-build source is "
                f"duplicated: {relative}"
            )
        contexts[relative] = _cc_source._call_contract_vc5_preprocessor_defines(
            config, resolved
        )
    if (
        _cc_catalog.ZCLASS_FREEALL_CALLER_SOURCE_PATH not in contexts
        or _cc_catalog.ZCLASS_RESET_ZBD_TARGET_SOURCE_PATH not in contexts
    ):
        raise ValueError(
            "cls_util ResetCurrentZbdPath bridge requires unique final-build "
            "caller and definition translation units"
        )
    resolver = _cc_source._call_contract_callable_inventory_resolver(
        document, root=root
    )
    caller_context = contexts[_cc_catalog.ZCLASS_FREEALL_CALLER_SOURCE_PATH]
    target_context = contexts[_cc_catalog.ZCLASS_RESET_ZBD_TARGET_SOURCE_PATH]
    reset_key = CallableKey(
        qualified_name=_cc_catalog.ZCLASS_RESET_ZBD_TARGET_NAME,
        parameter_shapes=(),
    )
    shutdown_key = CallableKey(
        qualified_name=_cc_catalog.ZCLASS_FREEALL_CALLER_NAME,
        parameter_shapes=(),
    )
    header_inventories = tuple(
        resolver(_cc_catalog.ZCLASS_FREEALL_HEADER_PATH, context)
        for context in sorted({caller_context, target_context})
    )
    caller_inventory = resolver(
        _cc_catalog.ZCLASS_FREEALL_CALLER_SOURCE_PATH, caller_context
    )
    target_inventory = resolver(
        _cc_catalog.ZCLASS_RESET_ZBD_TARGET_SOURCE_PATH, target_context
    )
    if (
        not header_inventories
        or any(
            reset_key not in inventory.declarations
            or reset_key in inventory.definitions
            or shutdown_key not in inventory.declarations
            for inventory in header_inventories
        )
        or reset_key not in target_inventory.definitions
        or reset_key in target_inventory.declarations
        or shutdown_key in target_inventory.definitions
        or reset_key in caller_inventory.declarations
        or reset_key in caller_inventory.definitions
        or shutdown_key not in caller_inventory.definitions
    ):
        raise ValueError(
            "cls_util ResetCurrentZbdPath bridge requires its unique qualified "
            "header declaration, cls_zbd.c definition, and cls_util.c caller"
        )


def _zclass_reset_zbd_candidate_identity_bridge(
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
    """Bind one exact candidate-only ResetCurrentZbdPath spelling."""

    if normalize_address(caller_start) != _cc_catalog.ZCLASS_FREEALL_CALLER_ADDRESS:
        return {}
    if (
        caller_identity != _cc_catalog.ZCLASS_FREEALL_CALLER_IDENTITY
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.ZCLASS_FREEALL_CALLER_END_EXCLUSIVE
    ):
        raise ValueError(
            "cls_util ResetCurrentZbdPath bridge requires its exact reviewed "
            "ShutdownCore caller"
        )
    mentions = [
        instruction
        for instruction in candidate.instructions
        if _cc_catalog.ZCLASS_RESET_ZBD_CANDIDATE_SYMBOL.casefold()
        in _cc_cfg._instruction_operand(instruction).casefold()
    ]
    if not mentions:
        return {}
    if (
        len(mentions) != 1
        or _cc_cfg._instruction_mnemonic(mentions[0]) != "call"
        or _cc_cfg._instruction_operand(mentions[0]).strip()
        != _cc_catalog.ZCLASS_RESET_ZBD_CANDIDATE_SYMBOL
    ):
        raise ValueError(
            "cls_util ResetCurrentZbdPath bridge accepts one exact direct CALL; "
            "aliases, case drift, duplicates, and indirect forms are forbidden"
        )

    _cls_util_exact_registered_target(
        document,
        target_id=_cc_catalog.ZCLASS_FREEALL_CALLER_TARGET_ID,
        manifest_path=_cc_catalog.ZCLASS_FREEALL_CALLER_TARGET_MANIFEST,
        target_name=_cc_catalog.ZCLASS_FREEALL_CALLER_TARGET_NAME,
        source_path=_cc_catalog.ZCLASS_FREEALL_CALLER_SOURCE_PATH,
        address=_cc_catalog.ZCLASS_FREEALL_CALLER_ADDRESS,
        symbol=_cc_catalog.ZCLASS_FREEALL_CALLER_SYMBOL,
        name=_cc_catalog.ZCLASS_FREEALL_CALLER_NAME,
        selected_target=candidate.target,
    )
    _cls_util_exact_registered_target(
        document,
        target_id=_cc_catalog.ZCLASS_RESET_ZBD_TARGET_ID,
        manifest_path=_cc_catalog.ZCLASS_RESET_ZBD_TARGET_MANIFEST,
        target_name=_cc_catalog.ZCLASS_RESET_ZBD_TARGET_ID.removeprefix(
            "recoil:vc5-target:"
        ),
        source_path=_cc_catalog.ZCLASS_RESET_ZBD_TARGET_SOURCE_PATH,
        address=_cc_catalog.ZCLASS_RESET_ZBD_TARGET_ADDRESS,
        symbol=_cc_catalog.ZCLASS_RESET_ZBD_REGISTERED_SYMBOL,
        name=_cc_catalog.ZCLASS_RESET_ZBD_TARGET_NAME,
    )
    _cls_util_resolved_tracker_symbol(
        document,
        symbol_id=_cc_catalog.ZCLASS_FREEALL_CALLER_SYMBOL_ID,
        address=_cc_catalog.ZCLASS_FREEALL_CALLER_ADDRESS,
        end_exclusive=_cc_catalog.ZCLASS_FREEALL_CALLER_END_EXCLUSIVE,
        size=len(_cc_catalog.ZCLASS_FREEALL_CALLER_BODY),
        name=_cc_catalog.ZCLASS_FREEALL_CALLER_NAME,
        block_id=_cc_catalog.ZCLASS_FREEALL_CALLER_BLOCK_ID,
        source_path=_cc_catalog.ZCLASS_FREEALL_CALLER_SOURCE_PATH,
        anchor_id=_cc_catalog.ZCLASS_FREEALL_CALLER_ANCHOR_ID,
        target_id=_cc_catalog.ZCLASS_FREEALL_CALLER_TARGET_ID,
    )
    _zclass_reset_zbd_tracker_source_provenance(document)
    if (
        indexes.by_address.get(_cc_catalog.ZCLASS_FREEALL_CALLER_ADDRESS)
        != _cc_catalog.ZCLASS_FREEALL_CALLER_IDENTITY
        or indexes.by_address.get(_cc_catalog.ZCLASS_RESET_ZBD_TARGET_ADDRESS)
        != _cc_catalog.ZCLASS_RESET_ZBD_TARGET_IDENTITY
        or [
            address
            for address, identity in indexes.by_address.items()
            if identity == _cc_catalog.ZCLASS_RESET_ZBD_TARGET_IDENTITY
        ]
        != [_cc_catalog.ZCLASS_RESET_ZBD_TARGET_ADDRESS]
        or _cc_catalog.ZCLASS_RESET_ZBD_TARGET_IDENTITY in indexes.provider_ids
        or _cc_catalog.ZCLASS_RESET_ZBD_CANDIDATE_SYMBOL in indexes.by_candidate_name
        or _cc_catalog.ZCLASS_RESET_ZBD_CANDIDATE_SYMBOL in indexes.storage_by_name
        or _cc_catalog.ZCLASS_RESET_ZBD_CANDIDATE_SYMBOL in bridge_names
    ):
        raise ValueError(
            "cls_util ResetCurrentZbdPath bridge requires collision-free unique "
            "authored caller and target identities"
        )
    _zclass_reset_zbd_source_provenance(document)

    exact_expected = {
        "ordinal": _cc_catalog.ZCLASS_RESET_ZBD_CALL_ORDINAL,
        "form": "call",
        "dispatch": "direct",
        "identity_kind": "direct",
        "target_identity": _cc_catalog.ZCLASS_RESET_ZBD_TARGET_IDENTITY,
        "storage_identity": "",
        "slot_displacement": None,
        "cleanup_bytes": None,
    }
    expected_rows = [
        row
        for row in expected
        if row.get("target_identity") == _cc_catalog.ZCLASS_RESET_ZBD_TARGET_IDENTITY
    ]
    if expected_rows != [exact_expected]:
        raise ValueError(
            "cls_util ResetCurrentZbdPath bridge requires one exact ordinal-3 "
            "retail authored direct-call contract"
        )

    definition = candidate.caller_definition
    relocation_rows = (
        tuple(
            (row.offset, row.type, row.symbol_name)
            for row in definition.relocations
        )
        if definition is not None
        else ()
    )
    expected_mask = tuple(
        any(
            offset <= index < offset + 4
            for offset, _kind, _name in _cc_catalog.ZCLASS_FREEALL_CALLER_RELOCATIONS
        )
        for index in range(len(_cc_catalog.ZCLASS_FREEALL_CALLER_BODY))
    )
    if (
        definition is None
        or definition.symbol != _cc_catalog.ZCLASS_FREEALL_CALLER_SYMBOL
        or definition.data != _cc_catalog.ZCLASS_FREEALL_CALLER_BODY
        or definition.section_index
        != _cc_catalog.ZCLASS_FREEALL_CALLER_COFF_SECTION_INDEX
        or definition.section_start != 0
        or definition.section_end != len(_cc_catalog.ZCLASS_FREEALL_CALLER_BODY)
        or relocation_rows != _cc_catalog.ZCLASS_FREEALL_CALLER_RELOCATIONS
        or definition.relocation_mask != expected_mask
        or definition.undefined_external_functions.count(
            _cc_catalog.ZCLASS_RESET_ZBD_CANDIDATE_SYMBOL
        )
        != 1
        or _cc_catalog.ZCLASS_RESET_ZBD_CANDIDATE_SYMBOL
        in (
            definition.defined_external_functions
            + definition.undefined_external_data
            + definition.defined_external_data
        )
        or _cc_catalog.ZCLASS_RESET_ZBD_REGISTERED_SYMBOL
        in (
            definition.undefined_external_functions
            + definition.defined_external_functions
            + definition.undefined_external_data
            + definition.defined_external_data
        )
    ):
        raise ValueError(
            "cls_util ResetCurrentZbdPath bridge requires the exact complete "
            "caller COFF body, relocation partition, and undefined symbol"
        )

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    encoded = bytearray()
    expected_offsets: list[int] = []
    for instruction in candidate.instructions:
        expected_offsets.append(len(encoded))
        try:
            encoded.extend(int(value, 16) for value in instruction.bytes)
        except ValueError as exc:
            raise ValueError(
                "cls_util ResetCurrentZbdPath bridge COD contains non-byte "
                "tokens"
            ) from exc
    call_index = candidate.instructions.index(mentions[0])
    if (
        tuple(expected_offsets) != offsets
        or bytes(encoded) != _cc_catalog.ZCLASS_FREEALL_CALLER_BODY[: len(encoded)]
        or len(encoded) != 0x59
        or _cc_catalog.ZCLASS_FREEALL_CALLER_BODY[len(encoded) :] != b"\x90" * 7
        or offsets[call_index] != _cc_catalog.ZCLASS_RESET_ZBD_CALL_OFFSET
        or tuple(mentions[0].bytes) != ("e8", "00", "00", "00", "00")
        or candidate.local_control_flow_indices
        or candidate.local_control_flow_targets
    ):
        raise ValueError(
            "cls_util ResetCurrentZbdPath bridge requires one complete "
            "contiguous COD procedure with the exact offset-0x4a direct call"
        )
    references = tuple(
        relocation
        for relocation in definition.relocations
        if relocation.symbol_name == _cc_catalog.ZCLASS_RESET_ZBD_CANDIDATE_SYMBOL
    )
    if (
        len(references) != 1
        or references[0].offset != _cc_catalog.ZCLASS_RESET_ZBD_RELOCATION_OFFSET
        or references[0].type != IMAGE_REL_I386_REL32
        or struct.unpack_from(
            "<I", definition.data, _cc_catalog.ZCLASS_RESET_ZBD_RELOCATION_OFFSET
        )[0]
        != 0
        or not all(
            definition.relocation_mask[index]
            for index in range(
                _cc_catalog.ZCLASS_RESET_ZBD_RELOCATION_OFFSET,
                _cc_catalog.ZCLASS_RESET_ZBD_RELOCATION_OFFSET + 4,
            )
        )
    ):
        raise ValueError(
            "cls_util ResetCurrentZbdPath bridge requires one zero-addend fully "
            "masked E8 REL32 candidate relocation"
        )
    return {
        _cc_catalog.ZCLASS_RESET_ZBD_CANDIDATE_SYMBOL: _cc_catalog.ZCLASS_RESET_ZBD_TARGET_IDENTITY
    }


def _cls_util_candidate_direct_authored_identity_closure(
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
    """Return the finite independently proven cls_util direct-name set."""

    result: dict[str, str] = {}
    for resolver in (
        _zclass_freeall_candidate_identity_bridge,
        _zclass_reset_zbd_candidate_identity_bridge,
    ):
        additions = resolver(
            expected,
            candidate,
            document=document,
            caller_identity=caller_identity,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            indexes=indexes,
            bridge_names=bridge_names,
        )
        result.update(additions)
    return result
