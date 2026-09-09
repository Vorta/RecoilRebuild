"""Recoil call-contract identity evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import callbacks as _cc_callbacks
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import errors as _cc_errors
from _recoil.call_contract import iat as _cc_iat
from _recoil.call_contract import instructions as _cc_instructions
from _recoil.call_contract import listing as _cc_listing
from _recoil.call_contract import logical_identity as _cc_logical_identity
from _recoil.call_contract import proofs as _cc_proofs
from _recoil.call_contract import receiver_candidate as _cc_receiver_candidate
from _recoil.call_contract import receiver_fields as _cc_receiver_fields
from _recoil.call_contract import receiver_instructions as _cc_receiver_instructions
from _recoil.call_contract import receiver_proofs as _cc_receiver_proofs
from _recoil.call_contract import receiver_storage as _cc_receiver_storage
from _recoil.call_contract import recoil_callbacks as _cc_recoil_callbacks
from _recoil.call_contract import recoil_hud_layout as _cc_recoil_hud_layout
from _recoil.call_contract import recoil_hud_widgets as _cc_recoil_hud_widgets
from _recoil.call_contract import recoil_input as _cc_recoil_input
from _recoil.call_contract import recoil_lifecycle as _cc_recoil_lifecycle
from _recoil.call_contract import recoil_network as _cc_recoil_network
from _recoil.call_contract import recoil_weapons as _cc_recoil_weapons
from _recoil.call_contract import retail_imports as _cc_retail_imports
from _recoil.call_contract import reviewed_dispatch as _cc_reviewed_dispatch
from _recoil.call_contract import source as _cc_source
from _recoil.call_contract import storage_identity as _cc_storage_identity
from _recoil.call_contract import targets as _cc_targets

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        CandidateExactIatRegisterLoadProof,
        CurrentIatStoragePackage,
        IdentityIndexes,
        ProviderOrdinalImportThunk,
        RegisteredPointerVectorDestroyProviderSupplier,
        RegisteredVectorAssignmentProviderSupplier,
        RetailCallerScopedProofPackage,
        ReviewedExactIndirectStorageBridge,
        ReviewedLoopVptrStorageBridge,
        ReviewedRegisterCallStorageBridge,
        ReviewedRetailProvenanceAdapters,
    )


import json
import os
import re
import struct
from dataclasses import replace
from pathlib import Path, PureWindowsPath
from types import SimpleNamespace
from typing import Any, Callable, Iterable, Mapping, Sequence

from _recoil.commands.asm_verify import Instruction
from _recoil.commands.provider_target_mutation import _retail_import_targets
from _recoil.commands.relocation_expectations import (
    normalize_relocation_target_binding,
    relocation_target_row_context,
)
from _recoil.commands.vc5_build import DEFAULT_MANIFEST as DEFAULT_FINAL_BUILD_MANIFEST
from _recoil.commands.vc5_build import alias_object_path, make_coff_alias_command
from _recoil.commands.vc5_build import build_paths as final_build_paths
from _recoil.commands.vc5_build import load_config as load_final_build_config
from _recoil.commands.vc5_build import run_command as run_final_build_command
from _recoil.commands.vc5_verify import load_manifest
from _recoil.lib.authored_icf import (
    AUTHORED_ICF_GROUP_MODEL,
    authored_icf_vtable_selector_index,
    require_valid_authored_icf_groups,
)
from _recoil.lib.binja import BinaryNinjaBridge, BridgeError
from _recoil.lib.coff_alias import (
    CoffAliasSource,
    resolve_llvm_ml,
    validate_alias_object,
    validate_alias_source,
)
from _recoil.lib.pe import parse_pe_headers, rva_to_offset
from _recoil.lib.progress import (
    AUTHORED_ORDER_DIMENSIONS,
    ProgressDocument,
    ProgressError,
    address_value,
    is_current_accepted_state,
    normalize_address,
)
from _recoil.lib.repository_paths import (
    RepositoryPathError,
    load_repository_path_inventory,
    resolve_repository_file,
)
from _recoil.lib.tooling import REPO_ROOT
from _recoil.lib.windows_identity import StableReadHandle


def resolve_validated_coff_alias_identity_bridges(
    specs: Sequence[CoffAliasSource],
    *,
    validation_by_source: Mapping[str, Mapping[str, object]],
    indexes: IdentityIndexes,
) -> dict[str, str]:
    """Map exact validated weak aliases to existing candidate identities.

    This is a relocation-name bridge only.  It grants no general symbol
    equivalence, provider classification, source model, tier, order, byte, or
    acceptance state.
    """
    result: dict[str, str] = {}
    target_by_alias: dict[str, str] = {}
    for spec in specs:
        report = validation_by_source.get(str(spec.source.resolve()))
        if (
            not isinstance(report, Mapping)
            or report.get("validated") is not True
            or report.get("policy_class") != "coff-weak-alias-only"
        ):
            raise ValueError(
                f"COFF alias source has no exact validated zero-section object: {spec.source}"
            )
        reported_rows = report.get("weak_aliases")
        exact_rows = [
            {"alias": item.alias, "target": item.target, "characteristics": 3}
            for item in spec.aliases
        ]
        if reported_rows != exact_rows:
            raise ValueError(
                f"COFF alias validation report does not match exact manifest aliases: {spec.source}"
            )
        for item in spec.aliases:
            prior_target = target_by_alias.get(item.alias)
            if prior_target is not None and prior_target != item.target:
                raise ValueError(
                    f"ambiguous COFF weak alias {item.alias!r}: "
                    f"{prior_target!r} versus {item.target!r}"
                )
            target_by_alias[item.alias] = item.target

    for alias, target in target_by_alias.items():
        if alias in indexes.by_candidate_name:
            raise ValueError(
                f"COFF weak alias {alias!r} conflicts with an ordinary candidate identity"
            )
        identity = indexes.by_candidate_name.get(target, "")
        if not identity:
            raise ValueError(
                f"COFF weak alias target {target!r} has no exact current candidate identity"
            )
        result[alias] = identity
    return result


def _without_caller_scoped_retail_proofs(
    indexes: IdentityIndexes,
) -> IdentityIndexes:
    """Start extraction for an independently proved physical child body.

    Caller-local register-IAT and proof-package facts are meaningful only for
    the exact instruction objects (or exact coordinate-rebound subset) of the
    physical caller that produced them.  A bridge which has independently
    proved another body's identity, extent, and bytes must not leak those facts
    into that child extraction.
    """

    return replace(
        indexes,
        reviewed_iat_register_joins=frozenset(),
        reviewed_retail_iat_load_proofs={},
        active_retail_caller_proof_package=None,
    )


def _symbol_identity(symbol_id: str, symbol: Mapping[str, Any]) -> str:
    if (
        symbol.get("kind") in {"provider-function", "compiler-function"}
        or symbol.get("pipeline_class") == "non-authored"
    ):
        return f"provider:{symbol_id}"
    return f"symbol:{symbol_id}"


def _decorated_coff_name(name: str) -> bool:
    if (
        not name
        or any(character.isspace() for character in name)
        or any(character in ",]" for character in name)
    ):
        return False
    if name.startswith("?"):
        return len(name) > 1
    if name.startswith("__imp_"):
        return len(name) > len("__imp_")
    if name[0] not in {"_", "@"} or len(name) < 2:
        return False
    first = name[1]
    if not (("A" <= first <= "Z") or ("a" <= first <= "z")):
        return False
    return all(
        ("A" <= character <= "Z")
        or ("a" <= character <= "z")
        or ("0" <= character <= "9")
        or character in "_@$?"
        for character in name[2:]
    )


def _finite_literal_symbol_regex_alternatives(
    symbol_regex: Any,
) -> tuple[str, ...] | None:
    """Enumerate one deliberately tiny regex language without executing regex."""
    if (
        not isinstance(symbol_regex, str)
        or len(symbol_regex) > _cc_catalog.FINITE_SYMBOL_REGEX_MAX_ENCODED_LENGTH
        or not symbol_regex.startswith("^(?:")
        or not symbol_regex.endswith(")$")
    ):
        return None
    encoded = symbol_regex[4:-2]
    alternatives: list[str] = []
    decoded: list[str] = []
    index = 0
    while index < len(encoded):
        character = encoded[index]
        if character == "|":
            name = "".join(decoded)
            if not name:
                return None
            alternatives.append(name)
            if len(alternatives) > _cc_catalog.FINITE_SYMBOL_REGEX_MAX_ALTERNATIVES:
                return None
            decoded = []
            index += 1
            continue
        if character == "\\":
            index += 1
            if index >= len(encoded):
                return None
            escaped = encoded[index]
            if escaped not in _cc_catalog.FINITE_SYMBOL_REGEX_LITERAL_METACHARACTERS:
                return None
            decoded.append(escaped)
        elif character in _cc_catalog.FINITE_SYMBOL_REGEX_LITERAL_METACHARACTERS:
            return None
        else:
            decoded.append(character)
        if len(decoded) > _cc_catalog.FINITE_SYMBOL_REGEX_MAX_DECODED_NAME_LENGTH:
            return None
        index += 1
    name = "".join(decoded)
    if not name:
        return None
    alternatives.append(name)
    if (
        len(alternatives) < 2
        or len(alternatives) > _cc_catalog.FINITE_SYMBOL_REGEX_MAX_ALTERNATIVES
        or len(set(alternatives)) != len(alternatives)
        or any(not _decorated_coff_name(item) for item in alternatives)
    ):
        return None
    return tuple(alternatives)


def _mapping_target_function_rows(
    target: Mapping[str, Any],
) -> tuple[Mapping[str, Any], ...]:
    return tuple(
        row for _view, row in _mapping_target_function_rows_with_views(target)
    )


def _mapping_target_function_rows_with_views(
    target: Mapping[str, Any],
) -> tuple[tuple[str, Mapping[str, Any]], ...]:
    registration = target.get("registration")
    if not isinstance(registration, Mapping):
        return ()
    rows: list[tuple[str, Mapping[str, Any]]] = [
        ("functions", row)
        for row in registration.get("functions", [])
        if isinstance(row, Mapping)
    ]
    for collection_name in (
        "translation_unit_function_order",
        "linked_function_intervals",
    ):
        for entry in registration.get(collection_name, []):
            if not isinstance(entry, Mapping):
                continue
            rows.extend(
                (collection_name, row)
                for row in entry.get("functions", [])
                if isinstance(row, Mapping)
            )
    return tuple(rows)


def _synchronized_finite_symbol_regex_target(
    target_id: str,
    target: Mapping[str, Any],
    *,
    address: str,
    row: Mapping[str, Any],
) -> None:
    registration = target.get("registration")
    if not isinstance(registration, Mapping):
        raise ValueError(
            f"finite symbol-regex authority target {target_id!r} has no registration"
        )
    manifest_value = registration.get("manifest_path")
    if not isinstance(manifest_value, str) or not manifest_value:
        raise ValueError(
            f"finite symbol-regex authority target {target_id!r} has no manifest path"
        )
    manifest_path = Path(manifest_value)
    if not manifest_path.is_absolute():
        manifest_path = REPO_ROOT / manifest_path
    try:
        manifest_path = manifest_path.resolve()
        manifest_path.relative_to(
            (REPO_ROOT / "tools" / "vc5_verify_targets").resolve()
        )
    except ValueError as exc:
        raise ValueError(
            f"finite symbol-regex authority target {target_id!r} manifest is outside "
            "tools/vc5_verify_targets"
        ) from exc
    if not manifest_path.is_file():
        raise ValueError(
            f"finite symbol-regex authority target {target_id!r} manifest is missing"
        )
    from _recoil.lib.verification_targets import vc5_target_registration

    try:
        current_id, current = vc5_target_registration(manifest_path)
    except (OSError, ValueError) as exc:
        raise ValueError(
            f"finite symbol-regex authority target {target_id!r} cannot be synchronized: {exc}"
        ) from exc
    current_registration = current.get("registration")
    stable_target_identity = (
        current_id == target_id
        and target.get("binary") == current.get("binary") == "recoil"
        and target.get("kind") == current.get("kind") == "vc5"
        and target.get("name") == current.get("name")
        and isinstance(current_registration, Mapping)
        and current_registration.get("manifest_path") == manifest_value
    )
    if not stable_target_identity:
        raise ValueError(
            f"finite symbol-regex authority target {target_id!r} is stale or conflicting"
        )
    current_registered = current.get("registered_addresses")
    if (
        not isinstance(current_registered, list)
        or sum(
            1
            for item in current_registered
            if isinstance(item, str)
            and normalize_address(item) == address
        )
        != 1
    ):
        raise ValueError(
            f"finite symbol-regex authority target {target_id!r} current "
            f"registration has missing or duplicate address {address}"
        )
    current_rows = [
        (view, current_row)
        for view, current_row in _mapping_target_function_rows_with_views(current)
        if isinstance(current_row.get("address"), str)
        and normalize_address(current_row["address"]) == address
    ]
    synchronized_fields = (
        "address",
        "pipeline_class",
        "authored_order_role",
        "authored_order_gate",
        "authored_relative_order_gate",
        "required_presence",
        "full_order_gate",
        "logical_identity_key",
        "icf_fold_status",
    )
    authority_rows = [
        current_row
        for _view, current_row in current_rows
        if any(
            isinstance(current_row.get(key), str)
            and bool(current_row.get(key))
            for key in ("symbol", "symbol_regex", "name")
        )
    ]
    if (
        not current_rows
        or len({view for view, _row in current_rows}) != len(current_rows)
        or any(
            tuple(current_row.get(field) for field in synchronized_fields)
            != tuple(row.get(field) for field in synchronized_fields)
            for _view, current_row in current_rows
        )
        or not authority_rows
        or any(dict(current_row) != dict(row) for current_row in authority_rows)
    ):
        raise ValueError(
            f"finite symbol-regex authority target {target_id!r} exact row at "
            f"{address} is stale, duplicated, or conflicting"
        )


def _index_finite_symbol_regex_names(
    document: ProgressDocument,
    *,
    by_address: Mapping[str, str],
    by_candidate_name: dict[str, str],
) -> None:
    symbols = document.collection("symbols")
    blocks = document.collection("physical_blocks")
    evidence = document.collection("evidence")
    symbol_rows_by_address: dict[
        str, list[tuple[str, Mapping[str, Any]]]
    ] = {}
    for symbol_id, symbol in symbols.items():
        if (
            not isinstance(symbol, Mapping)
            or symbol.get("kind")
            not in {"function", "provider-function", "compiler-function"}
        ):
            continue
        raw_address = symbol.get("address", symbol.get("start"))
        if not isinstance(raw_address, str):
            continue
        try:
            address = normalize_address(raw_address)
        except ProgressError:
            continue
        symbol_rows_by_address.setdefault(address, []).append(
            (str(symbol_id), symbol)
        )

    finite_names: dict[str, tuple[str, str, str]] = {}
    for target_id, target in document.collection("verification_targets").items():
        if not isinstance(target, Mapping):
            continue
        finite_rows: list[
            tuple[str, Mapping[str, Any], tuple[str, ...]]
        ] = []
        for view, row in _mapping_target_function_rows_with_views(target):
            alternatives = _finite_literal_symbol_regex_alternatives(
                row.get("symbol_regex")
            )
            if alternatives is not None:
                finite_rows.append((view, row, alternatives))
        if not finite_rows:
            continue

        seen_row_addresses: set[str] = set()
        for _view, row, alternatives in finite_rows:
            raw_address = row.get("address")
            if not isinstance(raw_address, str):
                continue
            try:
                address = normalize_address(raw_address)
            except ProgressError:
                continue
            if (
                row.get("required_presence") is not True
                or row.get("pipeline_class")
                not in {"authored", "authored-lifecycle"}
                or row.get("authored_order_role")
                not in {"authored-body", "authored-lifecycle-body"}
                or row.get("authored_order_gate") is not True
                or row.get("authored_relative_order_gate") is not True
                or row.get("logical_identity_key") not in {None, ""}
                or row.get("icf_fold_status") not in {None, ""}
            ):
                # Only physical authored gating rows can grant finite-name
                # authority. Inventory, provider, and logical-alias rows are
                # deliberately invisible to this candidate index.
                continue
            same_address_rows: list[
                tuple[str, Mapping[str, Any]]
            ] = []
            for (
                candidate_view,
                candidate_row,
                _candidate_alternatives,
            ) in finite_rows:
                candidate_raw_address = candidate_row.get("address")
                if not isinstance(candidate_raw_address, str):
                    continue
                try:
                    candidate_address = normalize_address(
                        candidate_raw_address
                    )
                except ProgressError:
                    continue
                if candidate_address == address:
                    same_address_rows.append(
                        (candidate_view, candidate_row)
                    )
            if (
                len(
                    {
                        candidate_view
                        for candidate_view, _candidate_row in same_address_rows
                    }
                )
                != len(same_address_rows)
            ):
                raise ValueError(
                    f"finite symbol-regex authority target {target_id!r} has "
                    f"duplicate rows for {address}"
                )
            if normalize_address(address) != "0x4b9850" and any(
                dict(candidate_row) != dict(row)
                for _candidate_view, candidate_row in same_address_rows
            ):
                raise ValueError(
                    f"finite symbol-regex authority target {target_id!r} has "
                    f"conflicting rows for {address}"
                )
            if address in seen_row_addresses:
                continue
            seen_row_addresses.add(address)
            physical_rows = symbol_rows_by_address.get(address, [])
            if len(physical_rows) != 1:
                raise ValueError(
                    f"finite symbol-regex row {target_id}:{address} has ambiguous "
                    "physical symbol population"
                )
            symbol_id, symbol = physical_rows[0]
            identity = by_address.get(address, "")
            if identity != f"symbol:{symbol_id}":
                raise ValueError(
                    f"finite symbol-regex row {target_id}:{address} has unresolved "
                    "or aliased physical identity"
                )
            block_id = str(symbol.get("physical_block_id") or "")
            block = blocks.get(block_id)
            facts = (
                block.get("accepted_order_facts")
                if isinstance(block, Mapping)
                else None
            )
            if (
                not isinstance(facts, Mapping)
                or facts.get("target_id") != target_id
            ):
                # Historical or non-accepted target rows are not authority.
                continue
            registered = target.get("registered_addresses")
            if (
                not isinstance(registered, list)
                or sum(
                    1
                    for item in registered
                    if isinstance(item, str)
                    and normalize_address(item) == address
                )
                != 1
            ):
                raise ValueError(
                    f"finite symbol-regex authority target {target_id!r} has "
                    f"missing or duplicate registered address {address}"
                )
            _synchronized_finite_symbol_regex_target(
                str(target_id),
                target,
                address=address,
                row=row,
            )
            if (
                facts.get("phase") != "authored-function-order"
                or facts.get("validation_mode") != "live"
                or not isinstance(facts.get("covered_block_ids"), list)
                or facts["covered_block_ids"].count(block_id) != 1
                or not isinstance(facts.get("matched_identities"), list)
                or facts["matched_identities"].count(symbol_id) != 1
            ):
                raise ValueError(
                    f"finite symbol-regex row {target_id}:{address} lacks exact "
                    "accepted authored-order block facts"
                )
            authored = block.get("order", {}).get("authored", {})
            if not isinstance(authored, Mapping):
                raise ValueError(
                    f"finite symbol-regex row {target_id}:{address} lacks authored-order state"
                )
            evidence_ids: set[str] = set()
            for dimension in AUTHORED_ORDER_DIMENSIONS:
                state = authored.get(dimension)
                if not is_current_accepted_state(state):
                    raise ValueError(
                        f"finite symbol-regex row {target_id}:{address} has stale "
                        f"authored-order state {dimension}"
                    )
                raw_evidence_ids = state.get("evidence_ids")
                if (
                    not isinstance(raw_evidence_ids, list)
                    or not raw_evidence_ids
                    or any(
                        not isinstance(item, str) or not item
                        for item in raw_evidence_ids
                    )
                ):
                    raise ValueError(
                        f"finite symbol-regex row {target_id}:{address} lacks "
                        f"accepted evidence for {dimension}"
                    )
                evidence_ids.update(raw_evidence_ids)
            for evidence_id in evidence_ids:
                evidence_row = evidence.get(evidence_id)
                provenance = (
                    evidence_row.get("provenance")
                    if isinstance(evidence_row, Mapping)
                    else None
                )
                if (
                    not is_current_accepted_state(evidence_row)
                    or not isinstance(provenance, Mapping)
                    or provenance.get("phase") != "authored-function-order"
                    or provenance.get("target_id") != target_id
                    or not isinstance(provenance.get("covered_block_ids"), list)
                    or provenance["covered_block_ids"].count(block_id) != 1
                    or not isinstance(provenance.get("expected_sequence"), list)
                    or provenance["expected_sequence"].count(symbol_id) != 1
                    or not isinstance(provenance.get("candidate_sequence"), list)
                    or provenance["candidate_sequence"].count(symbol_id) != 1
                ):
                    raise ValueError(
                        f"finite symbol-regex row {target_id}:{address} has stale "
                        f"or mismatched accepted evidence {evidence_id!r}"
                    )
            exact_symbol = row.get("symbol")
            if (
                isinstance(exact_symbol, str)
                and exact_symbol
                and exact_symbol not in alternatives
            ):
                raise ValueError(
                    f"finite symbol-regex row {target_id}:{address} exact symbol "
                    "disagrees with its alternatives"
                )
            for name in alternatives:
                previous_finite = finite_names.get(name)
                authority = (identity, str(target_id), address)
                if previous_finite is not None:
                    raise ValueError(
                        f"finite symbol-regex candidate name {name!r} has duplicate "
                        f"or ambiguous authority {previous_finite!r} and {authority!r}"
                    )
                finite_names[name] = authority
                prior = by_candidate_name.get(name)
                if prior is not None and prior != identity:
                    raise ValueError(
                        f"finite symbol-regex candidate name {name!r} conflicts "
                        f"with exact identity {prior!r}"
                    )
                if prior is None:
                    by_candidate_name[name] = identity


def _registered_vector_assignment_provider_supplier(
    document: ProgressDocument,
    *,
    target_id: str,
    target: Mapping[str, Any],
    row: Mapping[str, Any],
    by_address: Mapping[str, str],
    provider_ids: frozenset[str],
) -> RegisteredVectorAssignmentProviderSupplier | None:
    """Prove one registered provider row and its exact tracker identity."""
    from _recoil.call_contract.records import RegisteredVectorAssignmentProviderSupplier
    registration = target.get("registration")
    manifest_value = (
        registration.get("manifest_path")
        if isinstance(registration, Mapping)
        else None
    )
    if (
        target.get("binary") != "recoil"
        or target.get("kind") != "vc5"
        or not isinstance(registration, Mapping)
        or registration.get("binary") != "recoil"
        or registration.get("name") != target.get("name")
        or not isinstance(manifest_value, str)
        or not manifest_value
    ):
        return None
    manifest_path = Path(manifest_value)
    if not manifest_path.is_absolute():
        manifest_path = REPO_ROOT / manifest_path
    manifest_root = (REPO_ROOT / "tools" / "vc5_verify_targets").resolve()
    try:
        manifest_path = manifest_path.resolve()
        manifest_path.relative_to(manifest_root)
    except (OSError, ValueError):
        return None
    if not manifest_path.is_file():
        return None

    raw_address = row.get("address")
    if not isinstance(raw_address, str):
        return None
    try:
        address = normalize_address(raw_address)
    except ProgressError:
        return None
    if (
        row.get("name") != _cc_catalog.HUD_PANEL_LAYOUT_COPY_ASSIGN_PROVIDER_NAME
        or row.get("symbol") not in {None, ""}
        or row.get("symbol_regex")
        != _cc_catalog.HUD_PANEL_LAYOUT_COPY_ASSIGN_PROVIDER_REGEX
        or row.get("pipeline_class") != "unresolved"
        or row.get("authored_order_role") != "unresolved"
        or row.get("authored_order_gate") is not False
        or row.get("authored_relative_order_gate") is not False
        or row.get("required_presence") is not True
        or row.get("full_order_gate") is not True
        or row.get("logical_identity_key") not in {None, ""}
        or row.get("icf_fold_status") not in {None, ""}
    ):
        return None

    physical_rows: list[tuple[str, Mapping[str, Any]]] = []
    for symbol_id, symbol in document.collection("symbols").items():
        if not isinstance(symbol, Mapping):
            continue
        raw_symbol_address = symbol.get("address", symbol.get("start"))
        if not isinstance(raw_symbol_address, str):
            continue
        try:
            if normalize_address(raw_symbol_address) == address:
                physical_rows.append((str(symbol_id), symbol))
        except ProgressError:
            continue
    if len(physical_rows) != 1:
        return None
    symbol_id, symbol = physical_rows[0]
    raw_end = symbol.get("end_exclusive")
    raw_size = symbol.get("size")
    logical_aliases = symbol.get("logical_aliases")
    icf_group = symbol.get("icf_address_group")
    identity = f"provider:{symbol_id}"
    if not isinstance(raw_end, str):
        return None
    try:
        extent_size = (
            address_value(normalize_address(raw_end))
            - address_value(address)
        )
    except ProgressError:
        return None
    if (
        symbol.get("binary") != "recoil"
        or symbol.get("kind") != "function"
        or symbol.get("pipeline_class") != "non-authored"
        or symbol.get("authored_order_role") != "non-authored"
        or symbol.get("extent_state") != "known"
        or isinstance(raw_size, bool)
        or not isinstance(raw_size, int)
        or raw_size <= 0
        or extent_size != raw_size
        or symbol.get("logical_identity_key") not in {None, ""}
        or symbol.get("icf_fold_status") not in {None, ""}
        or isinstance(logical_aliases, Mapping)
        and bool(logical_aliases)
        or isinstance(icf_group, Mapping)
        and bool(icf_group)
        or by_address.get(address) != identity
        or identity not in provider_ids
    ):
        return None
    return RegisteredVectorAssignmentProviderSupplier(
        candidate_name=_cc_catalog.HUD_PANEL_LAYOUT_VECTOR_ASSIGNMENT_SYMBOL,
        address=address,
        identity=identity,
        target_id=target_id,
    )


def _index_registered_vector_assignment_provider_name(
    document: ProgressDocument,
    *,
    by_address: Mapping[str, str],
    by_candidate_name: dict[str, str],
    provider_ids: frozenset[str],
) -> None:
    """Publish the one exact vector-assignment name from one provider row."""
    suppliers: list[RegisteredVectorAssignmentProviderSupplier] = []
    matching_row_count = 0
    ambiguous = False
    for target_id, target in document.collection(
        "verification_targets"
    ).items():
        if not isinstance(target, Mapping):
            continue
        registration = target.get("registration")
        if not isinstance(registration, Mapping):
            continue
        for row in _mapping_target_function_rows(target):
            if (
                row.get("name")
                != _cc_catalog.HUD_PANEL_LAYOUT_COPY_ASSIGN_PROVIDER_NAME
                and row.get("symbol_regex")
                != _cc_catalog.HUD_PANEL_LAYOUT_COPY_ASSIGN_PROVIDER_REGEX
            ):
                continue
            matching_row_count += 1
            supplier = _registered_vector_assignment_provider_supplier(
                document,
                target_id=str(target_id),
                target=target,
                row=row,
                by_address=by_address,
                provider_ids=provider_ids,
            )
            if supplier is None:
                ambiguous = True
                continue
            suppliers.append(supplier)

    identities = {
        (supplier.identity, supplier.address, supplier.target_id)
        for supplier in suppliers
    }
    prior = by_candidate_name.get(
        _cc_catalog.HUD_PANEL_LAYOUT_VECTOR_ASSIGNMENT_SYMBOL
    )
    if (
        ambiguous
        or matching_row_count != 1
        or len(identities) != 1
        or prior not in {None, suppliers[0].identity if suppliers else ""}
    ):
        by_candidate_name[_cc_catalog.HUD_PANEL_LAYOUT_VECTOR_ASSIGNMENT_SYMBOL] = ""
        return
    by_candidate_name[_cc_catalog.HUD_PANEL_LAYOUT_VECTOR_ASSIGNMENT_SYMBOL] = (
        suppliers[0].identity
    )


def _direct_bn_retail_bytes(
    bridge: BinaryNinjaBridge | None,
    *,
    address: str,
    length: int,
) -> bytes | None:
    """Read exact same-invocation retail bytes from the authenticated BN view."""

    if bridge is None or length <= 0:
        return None
    try:
        text = bridge.hexdump(address, length)
    except (BridgeError, OSError, RuntimeError, ValueError):
        return None
    result = bytearray()
    expected_address = address_value(normalize_address(address))
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line:
            continue
        match = re.fullmatch(
            r"(?:0x)?(?P<address>[0-9a-fA-F]{1,16})\s*:?[ ]+"
            r"(?P<rendered>.+)",
            line,
        )
        if match is None:
            return None
        row_address = int(match.group("address"), 16)
        if row_address != expected_address + len(result):
            return None
        rendered = match.group("rendered")
        if rendered.endswith(":") and re.search(r"\s", rendered) is None:
            # The governed bridge prefixes a hexdump with an address-qualified
            # function label. It carries no bytes and must identify the next
            # unread address.
            continue
        byte_column = re.split(r"\s{2,}", rendered, maxsplit=1)[0]
        tokens = byte_column.split()
        if not tokens or any(
            re.fullmatch(r"[0-9a-fA-F]{2}", token) is None
            for token in tokens
        ):
            return None
        if len(result) + len(tokens) > length:
            return None
        result.extend(int(token, 16) for token in tokens)
    return bytes(result) if len(result) == length else None


def _registered_pointer_vector_destroy_provider_suppliers(
    document: ProgressDocument,
    *,
    by_address: Mapping[str, str],
    provider_ids: frozenset[str],
    bridge: BinaryNinjaBridge | None,
    fact_transcript: list[dict[str, Any]] | None = None,
) -> tuple[RegisteredPointerVectorDestroyProviderSupplier, ...]:
    """Derive reviewed pointer-vector no-op destroy providers without candidates.

    Authority comes from one exact tracker function, its synchronized VC5
    catalog row, and direct bytes from this invocation's authenticated retail
    BN session. Candidate object names never participate in supplier discovery.
    """
    from _recoil.call_contract.records import (
        RegisteredPointerVectorDestroyProviderSupplier,
    )
    from _recoil.lib.verification_targets import vc5_target_registration

    target_rows = document.collection("verification_targets")
    manifest_root = (REPO_ROOT / "tools" / "vc5_verify_targets").resolve()
    suppliers: list[RegisteredPointerVectorDestroyProviderSupplier] = []
    for symbol_id, symbol in document.collection("symbols").items():
        if not isinstance(symbol, Mapping):
            continue
        navigation_name = symbol.get("navigation_name")
        if (
            not isinstance(navigation_name, str)
            or _cc_catalog.MSVC_POINTER_VECTOR_DESTROY_NAVIGATION_RE.fullmatch(
                navigation_name
            )
            is None
        ):
            continue
        raw_address = symbol.get("address", symbol.get("start"))
        raw_end = symbol.get("end_exclusive")
        raw_size = symbol.get("size")
        trace = symbol.get("source_traceability")
        verification_target_ids = symbol.get("verification_target_ids")
        if (
            not isinstance(raw_address, str)
            or not isinstance(raw_end, str)
            or isinstance(raw_size, bool)
            or not isinstance(raw_size, int)
            or raw_size <= 0
            or not isinstance(trace, Mapping)
            or trace.get("state") != "not-applicable"
            or trace.get("reason_code") != "provider-boundary"
            or not isinstance(verification_target_ids, list)
            or symbol.get("binary") != "recoil"
            or symbol.get("kind") not in {"function", "provider-function"}
            or symbol.get("pipeline_class") != "non-authored"
            or symbol.get("authored_order_role")
            != "compiler-generated-icf-representative"
            or symbol.get("extent_state") != "known"
            or symbol.get("logical_identity_key") not in {None, ""}
            or symbol.get("icf_fold_status") not in {None, ""}
        ):
            continue
        try:
            address = normalize_address(raw_address)
            end_exclusive = normalize_address(raw_end)
        except ProgressError:
            continue
        if address_value(end_exclusive) - address_value(address) != raw_size:
            continue
        identity = f"provider:{symbol_id}"
        if (
            by_address.get(address) != identity
            or identity not in provider_ids
        ):
            continue

        for target_id in verification_target_ids:
            target = target_rows.get(str(target_id))
            if not isinstance(target, Mapping):
                continue
            symbol_ids = target.get("symbol_ids")
            unresolved_addresses = target.get("unresolved_addresses")
            if (
                target.get("binary") != "recoil"
                or target.get("kind") != "vc5"
                or not isinstance(symbol_ids, list)
                or symbol_ids.count(str(symbol_id)) != 1
                or unresolved_addresses not in (None, [])
            ):
                continue
            registration = target.get("registration")
            manifest_value = (
                registration.get("manifest_path")
                if isinstance(registration, Mapping)
                else None
            )
            if (
                not isinstance(registration, Mapping)
                or registration.get("binary") != "recoil"
                or registration.get("name") != target.get("name")
                or not isinstance(manifest_value, str)
                or not manifest_value
            ):
                continue
            manifest_path = Path(manifest_value)
            if not manifest_path.is_absolute():
                manifest_path = REPO_ROOT / manifest_path
            try:
                manifest_path = manifest_path.resolve()
                manifest_path.relative_to(manifest_root)
            except (OSError, ValueError):
                continue
            if not manifest_path.is_file():
                continue
            try:
                current_id, current = vc5_target_registration(manifest_path)
            except (OSError, ProgressError, ValueError):
                continue
            current_registration = current.get("registration")
            if (
                current_id != target_id
                or not (
                    current.get("binary")
                    == target.get("binary")
                    == "recoil"
                )
                or not (
                    current.get("kind")
                    == target.get("kind")
                    == "vc5"
                )
                or current.get("name") != target.get("name")
                or not isinstance(current_registration, Mapping)
                or current_registration.get("manifest_path") != manifest_value
            ):
                continue
            current_registered_addresses = current.get(
                "registered_addresses"
            )
            stored_function_addresses = registration.get(
                "function_addresses"
            )
            current_function_addresses = current_registration.get(
                "function_addresses"
            )
            if (
                not isinstance(current_registered_addresses, list)
                or current_registered_addresses.count(address) != 1
                or not isinstance(stored_function_addresses, list)
                or stored_function_addresses.count(address) != 1
                or not isinstance(current_function_addresses, list)
                or current_function_addresses.count(address) != 1
            ):
                continue
            try:
                stored_rows = [
                    row
                    for row in _mapping_target_function_rows(target)
                    if isinstance(row.get("address"), str)
                    and normalize_address(str(row["address"])) == address
                ]
                current_rows = [
                    row
                    for row in _mapping_target_function_rows(current)
                    if isinstance(row.get("address"), str)
                    and normalize_address(str(row["address"])) == address
                ]
            except ProgressError:
                continue
            if (
                len(stored_rows) != 1
                or len(current_rows) != 1
                or dict(stored_rows[0]) != dict(current_rows[0])
            ):
                continue

            try:
                manifest = load_manifest(
                    manifest_path,
                    enforce_source_policy=False,
                )
            except (OSError, ValueError):
                continue
            manifest_rows = [
                row
                for row in manifest.functions
                if normalize_address(row.address) == address
            ]
            if len(manifest_rows) != 1:
                continue
            manifest_row = manifest_rows[0]
            catalog_symbol = manifest_row.symbol
            if (
                not isinstance(catalog_symbol, str)
                or not _decorated_coff_name(catalog_symbol)
                or manifest_row.symbol_regex is not None
                or _cc_catalog.MSVC_POINTER_VECTOR_DESTROY_PROVIDER_RE.fullmatch(
                    catalog_symbol
                )
                is None
            ):
                continue

            retail_body = _direct_bn_retail_bytes(
                bridge,
                address=address,
                length=3,
            )
            cleanup_bytes = (
                struct.unpack_from("<H", retail_body, 1)[0]
                if retail_body is not None
                and len(retail_body) == 3
                and retail_body[0] == 0xC2
                else None
            )
            if cleanup_bytes != 8:
                continue
            if fact_transcript is not None:
                fact_transcript.append({
                    "fact": "provider-pointer-vector-destroy-cleanup",
                    "address": address,
                    "requested_length": 3,
                    "direct_bytes": list(retail_body),
                    "parsed_cleanup_bytes": cleanup_bytes,
                    "provider_identity": identity,
                    "provider_role": "compiler-generated-pointer-vector-destroy",
                })
            suppliers.append(
                RegisteredPointerVectorDestroyProviderSupplier(
                    address=address,
                    identity=identity,
                    target_id=str(target_id),
                    catalog_symbol=catalog_symbol,
                    stack_cleanup_bytes=cleanup_bytes,
                )
            )
    return tuple(suppliers)


def _publish_collected_identity(
    index: dict[str, str],
    key: str,
    identities: set[str],
) -> None:
    """Publish one convergent identity, preserving any prior ambiguity."""
    if not key:
        return
    identity = next(iter(identities)) if len(identities) == 1 else ""
    prior = index.get(key)
    index[key] = identity if prior in {None, identity} else ""


def _publish_current_iat_candidate_names(
    packages, *, by_address, by_candidate_name, provider_ids,
    storage_by_address, storage_by_name,
):
    """Give a proven __imp_ symbol its storage role without merging providers.

    A complete tracker IAT package has both a callable provider view and a
    four-byte storage view. The decorated imported-address symbol denotes the
    latter. A different provider, stale package, or competing name remains a
    collision; this never converts an ordinary function/thunk symbol to an IAT.
    """
    for package in packages:
        provider = f"provider:recoil:function:{package.address}"
        matching = [(name, value) for name, value in by_candidate_name.items()
                    if name.casefold() == package.object_symbol.casefold()]
        if (sum(p.address == package.address for p in packages) != 1
                or sum(p.object_symbol.casefold() == package.object_symbol.casefold() for p in packages) != 1
                or by_address.get(package.address) != provider or provider not in provider_ids
                or storage_by_address.get(package.address) != package.identity
                or storage_by_name.get(package.import_name) != package.identity
                or storage_by_name.get(package.object_symbol) != package.identity
                or matching not in ([(package.object_symbol, provider)], [(package.object_symbol, package.identity)])):
            continue
        by_candidate_name[package.object_symbol] = package.identity


def build_identity_indexes(
    document: ProgressDocument,
    *,
    bridge: BinaryNinjaBridge | None = None,
    retail_provider_fact_transcript: list[dict[str, Any]] | None = None,
    bridge_by_address: Mapping[str, Any] | None = None,
    bridge_data_rows: Sequence[Any] | None = None,
) -> IdentityIndexes:
    from _recoil.call_contract.records import (
        IdentityIndexes,
        ReviewedLogicalAlias,
        StorageContainer,
    )
    require_valid_authored_icf_groups(document.data)
    (
        reviewed_icf_group_by_address,
        reviewed_icf_group_by_logical_identity,
    ) = _cc_logical_identity._winner_unknown_icf_group_indexes(document)
    (
        reviewed_non_gating_logical_target_by_address,
        reviewed_non_gating_logical_target_by_candidate_name,
    ) = _cc_logical_identity._registered_non_gating_logical_target_indexes(document)
    by_address: dict[str, str] = {}
    provider_ids: set[str] = set()
    reviewed_logical_aliases_by_address: dict[
        str, list[ReviewedLogicalAlias]
    ] = {}
    reviewed_authored_icf_by_call_site: dict[str, tuple[str, str]] = {}
    reviewed_authored_icf_candidate_by_call_site: dict[
        str, tuple[str, str, str, str]
    ] = {}
    reviewed_authored_icf_provisional_candidate_by_name: dict[
        str, tuple[str, str]
    ] = {}
    reviewed_authored_icf_physical_by_logical_identity: dict[str, str] = {}
    authored_icf_candidate_only_names: set[str] = set()
    authored_icf_object_memberships: dict[str, tuple[str, str, str]] = {}
    for symbol_id, symbol in document.collection("symbols").items():
        if not isinstance(symbol, Mapping):
            continue
        if symbol.get("kind") not in {
            "function",
            "provider-function",
            "compiler-function",
        }:
            # Invocation targets and storage have distinct identities. In
            # particular, one IAT address may have both a provider-function
            # view and a provider-data view; the latter is indexed below as
            # storage and must not make the callable identity ambiguous.
            continue
        raw_address = symbol.get("address", symbol.get("start"))
        if not isinstance(raw_address, str):
            continue
        try:
            address = normalize_address(raw_address)
        except ProgressError:
            continue
        identity = _symbol_identity(str(symbol_id), symbol)
        logical_aliases = symbol.get("logical_aliases")
        if isinstance(logical_aliases, Mapping) and logical_aliases:
            group = symbol.get("icf_address_group")
            authored_icf_group = bool(
                isinstance(group, Mapping)
                and group.get("model") == AUTHORED_ICF_GROUP_MODEL
                and group.get("physical_gate_symbol_id") == symbol_id
            )
            reviewed_logical_aliases_by_address.setdefault(address, []).extend(
                ReviewedLogicalAlias(
                    identity=f"logical:{alias_id}",
                    original_name=(
                        str(alias.get("original_name") or "")
                        if alias.get("original_name_status") != "provisional"
                        else ""
                    ),
                    object_symbol=(
                        ""
                        if authored_icf_group
                        else str(alias.get("object_symbol") or "")
                    ),
                )
                for alias_id, alias in logical_aliases.items()
                if isinstance(alias, Mapping)
                and alias.get("pipeline_class") in {"authored", "authored-lifecycle"}
                and (
                    alias.get("fold_status") == "proven-fold-alias"
                    or authored_icf_group
                )
                and (
                    isinstance(alias.get("original_name"), str)
                    and bool(alias.get("original_name"))
                    or isinstance(alias.get("object_symbol"), str)
                    and bool(alias.get("object_symbol"))
                )
            )
            if authored_icf_group:
                for alias_id, alias in logical_aliases.items():
                    if not isinstance(alias, Mapping):
                        raise ValueError(
                            f"authored ICF logical member {alias_id!r} is not an object"
                        )
                    logical_identity = f"logical:{alias_id}"
                    reviewed_authored_icf_physical_by_logical_identity[
                        logical_identity
                    ] = str(symbol_id)
                    object_symbol = alias.get("object_symbol")
                    if (
                        not isinstance(object_symbol, str)
                        or not _decorated_coff_name(object_symbol)
                    ):
                        raise ValueError(
                            f"authored ICF logical member {alias_id!r} has no "
                            "exact decorated candidate object symbol"
                        )
                    object_membership = (
                        str(symbol_id),
                        str(alias_id),
                        object_symbol,
                    )
                    prior_object_membership = authored_icf_object_memberships.get(
                        object_symbol.casefold()
                    )
                    if prior_object_membership not in {None, object_membership}:
                        raise ValueError(
                            f"authored ICF candidate object symbol {object_symbol!r} "
                            "has an ambiguous or case-conflicting logical population"
                        )
                    authored_icf_object_memberships[
                        object_symbol.casefold()
                    ] = object_membership
                    authored_icf_candidate_only_names.add(object_symbol)
                    if alias.get("original_name_status") == "provisional":
                        provisional_selection = (
                            str(symbol_id),
                            logical_identity,
                        )
                        prior_provisional = (
                            reviewed_authored_icf_provisional_candidate_by_name.get(
                                object_symbol
                            )
                        )
                        if prior_provisional not in {
                            None,
                            provisional_selection,
                        }:
                            raise ValueError(
                                "authored ICF provisional candidate object symbol "
                                f"{object_symbol!r} selects conflicting reviewed "
                                "logical identities"
                            )
                        reviewed_authored_icf_provisional_candidate_by_name[
                            object_symbol
                        ] = provisional_selection
                    selectors = alias.get("retail_target_selectors")
                    if not isinstance(selectors, Mapping):
                        raise ValueError(
                            f"authored ICF logical member {alias_id!r} has no retail selectors"
                        )
                    direct_call_sites = selectors.get("direct_call_sites")
                    if (
                        not isinstance(direct_call_sites, list)
                        or any(
                            not isinstance(raw_call_site, str)
                            or not raw_call_site
                            for raw_call_site in direct_call_sites
                        )
                    ):
                        raise ValueError(
                            f"authored ICF logical member {alias_id!r} has a "
                            "malformed direct-call selector population"
                        )
                    normalized_call_sites = [
                        normalize_address(raw_call_site)
                        for raw_call_site in direct_call_sites
                    ]
                    if len(set(normalized_call_sites)) != len(normalized_call_sites):
                        raise ValueError(
                            f"authored ICF logical member {alias_id!r} has a "
                            "duplicate direct-call selector population"
                        )
                    for call_site in normalized_call_sites:
                        selection = (address, logical_identity)
                        prior = reviewed_authored_icf_by_call_site.get(call_site)
                        if prior not in {None, selection}:
                            raise ValueError(
                                f"authored ICF retail call site {call_site} selects "
                                "conflicting logical identities"
                            )
                        reviewed_authored_icf_by_call_site[call_site] = selection
                        candidate_selection = (
                            address,
                            str(symbol_id),
                            object_symbol,
                            logical_identity,
                        )
                        prior_candidate = (
                            reviewed_authored_icf_candidate_by_call_site.get(
                                call_site
                            )
                        )
                        if prior_candidate not in {None, candidate_selection}:
                            raise ValueError(
                                f"authored ICF retail call site {call_site} has "
                                "a conflicting candidate object-symbol population"
                            )
                        reviewed_authored_icf_candidate_by_call_site[
                            call_site
                        ] = candidate_selection
            authored_aliases = {
                str(alias_id): alias
                for alias_id, alias in logical_aliases.items()
                if isinstance(alias, Mapping)
                and alias.get("authored_order_role")
                in {"authored-body", "authored-lifecycle-body"}
            }
            winner_key = (
                str(group.get("winner_identity_key") or "")
                if isinstance(group, Mapping)
                and group.get("winner_status") == "selected-winner"
                else ""
            )
            selected_aliases = [
                alias_id
                for alias_id, alias in authored_aliases.items()
                if winner_key
                and winner_key
                in {
                    str(alias_id),
                    str(alias.get("logical_identity_key") or ""),
                    str(alias.get("object_symbol") or ""),
                }
            ]
            if not authored_icf_group:
                identity = (
                    f"logical:{selected_aliases[0]}"
                    if len(selected_aliases) == 1
                    else ""
                )
        if address in by_address and by_address[address] != identity:
            # Physical aliases are not silently selected as expected truth.
            by_address[address] = ""
        else:
            by_address[address] = identity
        if identity.startswith("provider:"):
            provider_ids.add(identity)

    by_candidate_name: dict[str, str] = {}
    reviewed_binding_names: set[str] = set()
    reviewed_data_bindings: list[
        tuple[str, Mapping[str, Any], Mapping[str, Any], str]
    ] = []

    # A governed provider-function registration carries a candidate-independent
    # object identity on the existing physical function row.  Schema v1 names
    # one exact static-library archive member; schema v2 names one exact
    # canonical-header COMDAT probe.  Index the object name directly only when
    # the complete accepted owner/evidence package remains current.  In
    # particular, v2 must not weaken the v1 archive-member provenance checks.
    for symbol_id, symbol in document.collection("symbols").items():
        if not isinstance(symbol, Mapping):
            continue
        provider_object = symbol.get("provider_object_identity")
        if not isinstance(provider_object, Mapping):
            continue
        provider_schema = provider_object.get("schema")
        object_symbol = provider_object.get("object_symbol")
        raw_address = symbol.get("address")
        evidence_id = provider_object.get("evidence_id")
        archive_member_registration = provider_schema == (
            "recoil-provider-function-object-v1"
        )
        canonical_header_registration = provider_schema == (
            "recoil-provider-function-object-v2"
        )
        physical_emitter = provider_object.get("physical_emitter")
        retail_icf = provider_object.get("retail_icf")
        logical_symbols = (
            retail_icf.get("logical_symbols")
            if isinstance(retail_icf, Mapping)
            else None
        )
        schema_registration_complete = (
            (
                archive_member_registration
                and isinstance(provider_object.get("library_path"), str)
                and bool(provider_object["library_path"])
                and isinstance(provider_object.get("archive_member"), str)
                and bool(provider_object["archive_member"])
            )
            or (
                canonical_header_registration
                and provider_object.get("proof_mode")
                == "canonical-header-comdat"
                and isinstance(provider_object.get("canonical_header"), str)
                and bool(provider_object["canonical_header"])
                and isinstance(provider_object.get("probe_recipe"), str)
                and bool(provider_object["probe_recipe"])
                and isinstance(provider_object.get("section_name"), str)
                and bool(provider_object["section_name"])
                and isinstance(provider_object.get("body_size"), int)
                and not isinstance(provider_object["body_size"], bool)
                and provider_object["body_size"] > 0
                and isinstance(provider_object.get("comdat_selection"), int)
                and not isinstance(provider_object["comdat_selection"], bool)
                and provider_object["comdat_selection"] > 0
                and isinstance(provider_object.get("semantic_provider"), str)
                and bool(provider_object["semantic_provider"])
                and isinstance(physical_emitter, Mapping)
                and physical_emitter.get("state") == "winner-unknown"
                and isinstance(retail_icf, Mapping)
                and retail_icf.get("winner_status") == "winner-unknown"
                and isinstance(logical_symbols, list)
                and bool(logical_symbols)
                and all(
                    isinstance(logical_symbol, str) and bool(logical_symbol)
                    for logical_symbol in logical_symbols
                )
                and len(set(logical_symbols)) == len(logical_symbols)
                and object_symbol in logical_symbols
            )
        )
        if (
            not schema_registration_complete
            or symbol.get("kind") != "provider-function"
            or symbol.get("disposition") != "provider"
            or symbol.get("ownership_state") != "primary-owned"
            or symbol.get("pipeline_class") != "non-authored"
            or symbol.get("authored_order_role") not in {
                "non-authored",
                "compiler-generated-icf-representative",
            }
            or not isinstance(object_symbol, str)
            or not object_symbol
            or symbol.get("object_symbol") != object_symbol
            or not isinstance(raw_address, str)
            or not isinstance(evidence_id, str)
            or evidence_id not in symbol.get("evidence_ids", ())
        ):
            raise ValueError(
                f"registered provider object identity for {symbol_id} is incomplete"
            )
        address = normalize_address(raw_address)
        identity = by_address.get(address, "")
        if not identity or identity != f"provider:{symbol_id}":
            raise ValueError(
                f"registered provider object identity for {symbol_id} has no unique "
                "provider physical identity"
            )
        matching_owners = []
        for owner_id, owner in document.collection("owners").items():
            if not isinstance(owner, Mapping):
                continue
            relationships = owner.get("relationships")
            if not isinstance(relationships, list):
                continue
            if any(
                isinstance(relationship, Mapping)
                and relationship.get("kind") == "primary-function"
                and relationship.get("symbol_id") == symbol_id
                and relationship.get("address") == address
                for relationship in relationships
            ):
                matching_owners.append((owner_id, owner))
        if len(matching_owners) != 1:
            raise ValueError(
                f"registered provider object identity for {symbol_id} does not have "
                "exactly one primary provider owner"
            )
        owner_id, owner = matching_owners[0]
        gates = owner.get("gates")
        # A fresh canonical-header proof identifies this member independently.
        # An existing provider census can contain other, unproved members, so
        # its aggregate linkage gate must remain unchanged by this lookup.
        if (
            owner.get("kind") != "provider-boundary"
            or owner.get("provider_state") != "accepted"
            or owner.get("lifecycle_state") != "accepted"
            or not isinstance(gates, Mapping)
            or gates.get("boundary") != "accepted"
            or gates.get("source") != "accepted"
            or (archive_member_registration and gates.get("owner_linkage") != "accepted")
            or evidence_id not in owner.get("evidence_ids", ())
        ):
            raise ValueError(
                f"registered provider object identity for {symbol_id} has stale "
                f"provider owner {owner_id}"
            )
        evidence = document.collection("evidence").get(evidence_id)
        provenance = evidence.get("provenance") if isinstance(evidence, Mapping) else None
        archive_member_provenance_matches = bool(
            archive_member_registration
            and isinstance(provenance, Mapping)
            and provenance.get("library_path")
            == provider_object.get("library_path")
            and provenance.get("archive_member")
            == provider_object.get("archive_member")
            and provenance.get("object_symbol") == object_symbol
        )
        canonical_header_provenance_matches = bool(
            canonical_header_registration
            and isinstance(provenance, Mapping)
            and provenance.get("proof_mode") == "canonical-header-comdat"
            and provenance.get("canonical_header")
            == provider_object.get("canonical_header")
            and provenance.get("probe_recipe")
            == provider_object.get("probe_recipe")
            and provenance.get("object_symbol") == object_symbol
            and provenance.get("section_name")
            == provider_object.get("section_name")
            and provenance.get("body_size") == provider_object.get("body_size")
            and provenance.get("comdat_selection")
            == provider_object.get("comdat_selection")
            and provenance.get("semantic_provider")
            == provider_object.get("semantic_provider")
            and provenance.get("physical_emitter_state")
            == physical_emitter.get("state")
            and provenance.get("retail_icf_winner_status")
            == retail_icf.get("winner_status")
            and provenance.get("retail_icf_logical_symbols") == logical_symbols
        )
        if (
            not isinstance(evidence, Mapping)
            or evidence.get("kind") != "provider-function-registration"
            or evidence.get("freshness") != "current"
            or not isinstance(provenance, Mapping)
            or provenance.get("candidate_independent") is not True
            or not (
                archive_member_provenance_matches
                or canonical_header_provenance_matches
            )
        ):
            raise ValueError(
                f"registered provider object identity for {symbol_id} has stale "
                "or mismatched live evidence"
            )
        prior = by_candidate_name.get(object_symbol)
        if prior not in {None, identity}:
            raise ValueError(
                f"registered provider object name {object_symbol!r} maps to "
                f"conflicting identities {prior!r} and {identity!r}"
            )
        by_candidate_name[object_symbol] = identity

    for symbol in document.collection("symbols").values():
        if not isinstance(symbol, Mapping):
            continue
        logical_aliases = symbol.get("logical_aliases")
        if not isinstance(logical_aliases, Mapping):
            continue
        for alias_id, alias in logical_aliases.items():
            if not isinstance(alias, Mapping):
                continue
            object_symbol = alias.get("object_symbol")
            if isinstance(object_symbol, str) and object_symbol:
                by_candidate_name[object_symbol] = f"logical:{alias_id}"
    for symbol_id, symbol in document.collection("symbols").items():
        if not isinstance(symbol, Mapping):
            continue
        raw_binding = symbol.get("relocation_target_binding")
        if not isinstance(raw_binding, Mapping):
            continue
        binding = normalize_relocation_target_binding(raw_binding)
        context = binding["binding_context"]
        if context["creation_mode"] != "existing-symbol":
            continue
        target = context["target"]
        if target["symbol_id"] != symbol_id:
            raise ValueError(
                f"reviewed relocation target binding for {symbol_id} points to "
                f"different existing symbol {target['symbol_id']}"
            )
        raw_address = symbol.get("address", symbol.get("start"))
        if not isinstance(raw_address, str):
            raise ValueError(
                f"reviewed relocation target binding for {symbol_id} has no physical address"
            )
        address = normalize_address(raw_address)
        if target["address"] != address:
            raise ValueError(
                f"reviewed relocation target binding for {symbol_id} has stale target address"
            )
        target_kind = symbol.get("kind")
        if target_kind in {"data", "data-symbol"}:
            if (
                symbol.get("binary") != "recoil"
                or any(
                    field in symbol
                    for field in ("import_name", "import_dll", "import_ordinal")
                )
            ):
                raise ValueError(
                    f"reviewed relocation target binding for {symbol_id} has "
                    "provider/import data identity"
                )
            current_target = relocation_target_row_context(
                symbol_id=str(symbol_id),
                row=symbol,
                object_symbol=str(binding["object_symbol"]),
            )
            if current_target != target:
                raise ValueError(
                    f"reviewed relocation target binding for {symbol_id} has "
                    "stale data target snapshot"
                )
            reviewed_data_bindings.append(
                (str(symbol_id), symbol, binding, address)
            )
            continue
        if target_kind not in {
            "function",
            "provider-function",
            "compiler-function",
        }:
            raise ValueError(
                f"reviewed relocation target binding for {symbol_id} has "
                f"unsupported target kind {target_kind!r}"
            )
        identity = by_address.get(address, "")
        if not identity:
            raise ValueError(
                f"reviewed relocation target binding for {symbol_id} has ambiguous "
                "or unresolved physical identity"
            )
        object_symbol = binding["object_symbol"]
        prior = by_candidate_name.get(object_symbol)
        if prior is not None and prior != identity:
            raise ValueError(
                f"reviewed existing-symbol object name {object_symbol!r} maps to "
                f"conflicting identities {prior!r} and {identity!r} ({symbol_id})"
            )
        by_candidate_name[object_symbol] = identity
        reviewed_binding_names.add(object_symbol)
    for target in document.collection("verification_targets").values():
        if not isinstance(target, Mapping):
            continue
        registration = target.get("registration")
        if not isinstance(registration, Mapping):
            continue
        function_rows: list[Mapping[str, Any]] = []
        for value in registration.get("functions", []):
            if isinstance(value, Mapping):
                function_rows.append(value)
        for entry in registration.get("translation_unit_function_order", []):
            if isinstance(entry, Mapping):
                function_rows.extend(
                    value
                    for value in entry.get("functions", [])
                    if isinstance(value, Mapping)
                )
        for interval in registration.get("linked_function_intervals", []):
            if isinstance(interval, Mapping):
                function_rows.extend(
                    value
                    for value in interval.get("functions", [])
                    if isinstance(value, Mapping)
                )
        for row in function_rows:
            address = row.get("address")
            symbol_name = row.get("symbol")
            if not isinstance(address, str) or not isinstance(symbol_name, str) or not symbol_name:
                continue
            identity = by_address.get(normalize_address(address), "")
            if not identity:
                continue
            prior = by_candidate_name.get(symbol_name)
            if (
                symbol_name in reviewed_binding_names
                and prior is not None
                and prior != identity
            ):
                raise ValueError(
                    f"reviewed existing-symbol object name {symbol_name!r} maps to "
                    f"conflicting identities {prior!r} and {identity!r} "
                    f"(verification target row {address})"
                )
            by_candidate_name[symbol_name] = (
                identity if prior in {None, identity} else ""
            )

    _index_finite_symbol_regex_names(
        document,
        by_address=by_address,
        by_candidate_name=by_candidate_name,
    )
    for name, identity in (
        reviewed_non_gating_logical_target_by_candidate_name.items()
    ):
        if by_candidate_name.get(name) != identity:
            raise ValueError(
                f"registered non-gating logical candidate name {name!r} "
                "does not resolve uniquely to its reviewed alias"
            )
    candidate_names_before_lifecycle_fallback = set(by_candidate_name)
    _cc_recoil_lifecycle._index_registered_lifecycle_deleting_destructor_names(
        document,
        by_address=by_address,
        by_candidate_name=by_candidate_name,
    )
    scalar_deleting_destructor_legacy_fallback_names = frozenset(
        name
        for name, identity in by_candidate_name.items()
        if (
            not identity
            and name not in candidate_names_before_lifecycle_fallback
            and _cc_recoil_lifecycle._nonvirtual_scalar_deleting_destructor_class(name)
            is not None
        )
    )
    (
        scalar_deleting_destructors,
        scalar_deleting_destructor_blockers,
    ) = _cc_recoil_lifecycle._registered_scalar_deleting_destructor_suppliers(
        document,
        by_address=by_address,
        bridge_by_address=bridge_by_address or {},
    )
    _index_registered_vector_assignment_provider_name(
        document,
        by_address=by_address,
        by_candidate_name=by_candidate_name,
        provider_ids=frozenset(provider_ids),
    )
    pointer_vector_destroy_providers = (
        _registered_pointer_vector_destroy_provider_suppliers(
            document,
            by_address=by_address,
            provider_ids=frozenset(provider_ids),
            bridge=bridge,
            fact_transcript=retail_provider_fact_transcript,
        )
    )
    hud_cmd_binding_ptr_vector_erase_provider = (
        _cc_recoil_hud_widgets._hud_cmd_binding_ptr_vector_erase_provider_supplier(
            document,
            by_address=by_address,
            by_candidate_name=by_candidate_name,
            provider_ids=frozenset(provider_ids),
            bridge=bridge,
            fact_transcript=retail_provider_fact_transcript,
        )
    )

    storage_by_address: dict[str, str] = {}
    storage_by_name: dict[str, str] = {}
    for collection_name in ("storage_contributions", "symbols"):
        for entity_id, row in document.collection(collection_name).items():
            if not isinstance(row, Mapping):
                continue
            if (
                row.get("kind") == "provider-data"
                or "import_name" in row
                or "import_dll" in row
                or "import_ordinal" in row
            ):
                # Provider IAT views are indexed only from a complete current
                # tracker package below. Partial rows never become truth.
                continue
            raw_address = row.get("address", row.get("start"))
            address = ""
            if isinstance(raw_address, str):
                try:
                    address = normalize_address(raw_address)
                except ProgressError:
                    pass
            if collection_name == "symbols" and row.get("kind") in {
                "function",
                "provider-function",
                "compiler-function",
            }:
                continue
            if address:
                identity = f"storage:{entity_id}"
                storage_by_address.setdefault(address, identity)
            for key in ("name", "navigation_name", "symbol"):
                value = row.get(key)
                if isinstance(value, str) and value:
                    storage_by_name.setdefault(value, f"storage:{entity_id}")
    packages = _cc_storage_identity._current_tracker_iat_storage_packages(document)
    packages_by_address: dict[str, set[CurrentIatStoragePackage]] = {}
    packages_by_name: dict[str, set[CurrentIatStoragePackage]] = {}
    for package in packages:
        packages_by_address.setdefault(package.address, set()).add(package)
        packages_by_name.setdefault(package.import_name, set()).add(package)
        packages_by_name.setdefault(package.object_symbol, set()).add(package)
        if package.import_ordinal is not None:
            packages_by_name.setdefault(
                f"{package.import_dll}!#{package.import_ordinal}",
                set(),
            ).add(package)
    for address, suppliers in packages_by_address.items():
        _publish_collected_identity(
            storage_by_address,
            address,
            {supplier.identity for supplier in suppliers}
            if len(suppliers) == 1
            else set(),
        )
    for name, suppliers in packages_by_name.items():
        _publish_collected_identity(
            storage_by_name,
            name,
            {supplier.identity for supplier in suppliers}
            if len(suppliers) == 1
            else set(),
        )
    storage_containers: set[StorageContainer] = set()
    data_rows = tuple(bridge_data_rows or ())
    data_row_address_counts: dict[int, int] = {}
    for row in data_rows:
        try:
            value = address_value(normalize_address(str(getattr(row, "address", ""))))
        except (ProgressError, ValueError):
            continue
        data_row_address_counts[value] = data_row_address_counts.get(value, 0) + 1
    for row in data_rows:
        raw_address = getattr(row, "address", "")
        if not raw_address:
            continue
        try:
            address = normalize_address(str(raw_address))
        except ProgressError:
            continue
        identity = storage_by_address.get(address, "")
        if not identity or identity.startswith("iat:"):
            continue
        try:
            size = int(getattr(row, "size", 0))
        except (TypeError, ValueError):
            size = 0
        start = address_value(address)
        if size <= 0:
            # BN sometimes leaves an individual function-pointer cell
            # untyped/unsized even though its immediately adjacent data rows
            # prove both exact four-byte boundaries.  Require unique rows on
            # both sides; one neighbor, a duplicate row, or a wider gap is not
            # extent evidence.
            if not (
                data_row_address_counts.get(start, 0) == 1
                and data_row_address_counts.get(start - 4, 0) == 1
                and data_row_address_counts.get(start + 4, 0) == 1
            ):
                continue
            size = 4
        storage_containers.add(
            StorageContainer(
                start=start,
                end_exclusive=start + size,
                identity=identity,
            )
        )
        bridge_names = {
            str(getattr(row, "name", "") or ""),
            str(getattr(row, "raw_name", "") or ""),
        }
        for name in bridge_names | {
            name.lower() for name in bridge_names if name
        }:
            if not name:
                continue
            prior = storage_by_name.get(name)
            storage_by_name[name] = (
                identity if prior in {None, identity} else ""
            )
    pooled_storage_by_name: dict[str, str] = {}
    _cc_storage_identity._index_reviewed_pooled_data_names(
        document,
        storage_by_address=storage_by_address,
        storage_by_name=pooled_storage_by_name,
    )
    _cc_storage_identity._index_registered_decorated_data_names(
        document,
        storage_by_name=storage_by_name,
        pooled_storage_by_name=pooled_storage_by_name,
    )
    for name, identity in pooled_storage_by_name.items():
        _publish_collected_identity(storage_by_name, name, {identity})
    for symbol_id, _symbol, binding, address in reviewed_data_bindings:
        expected_identity = f"storage:{symbol_id}"
        address_identities: set[str] = set()
        matching_name_identities: set[str] = set()
        object_symbol = str(binding["object_symbol"])
        for collection_name in ("storage_contributions", "symbols"):
            for entity_id, row in document.collection(collection_name).items():
                if not isinstance(row, Mapping):
                    continue
                if (
                    row.get("kind") == "provider-data"
                    or "import_name" in row
                    or "import_dll" in row
                    or "import_ordinal" in row
                ):
                    continue
                raw_address = row.get("address", row.get("start"))
                row_address = ""
                if isinstance(raw_address, str):
                    try:
                        row_address = normalize_address(raw_address)
                    except ProgressError:
                        pass
                identity = f"storage:{entity_id}"
                if row_address == address:
                    address_identities.add(identity)
                if any(
                    row.get(key) == object_symbol
                    for key in ("name", "navigation_name", "symbol")
                ):
                    matching_name_identities.add(identity)
        if (
            address_identities != {expected_identity}
            or storage_by_address.get(address) != expected_identity
        ):
            raise ValueError(
                f"reviewed data relocation target binding for {symbol_id} has "
                "missing, ambiguous, or colliding storage identity"
            )
        prior = storage_by_name.get(object_symbol)
        if (
            prior not in {None, expected_identity}
            or matching_name_identities - {expected_identity}
        ):
            raise ValueError(
                f"reviewed existing-data object name {object_symbol!r} maps to "
                f"a conflicting storage identity ({symbol_id})"
            )
        storage_by_name[object_symbol] = expected_identity
    _publish_current_iat_candidate_names(
        packages, by_address=by_address, by_candidate_name=by_candidate_name,
        provider_ids=provider_ids, storage_by_address=storage_by_address,
        storage_by_name=storage_by_name,
    )
    return IdentityIndexes(
        by_address=by_address,
        by_candidate_name=by_candidate_name,
        provider_ids=frozenset(provider_ids),
        storage_by_address=storage_by_address,
        storage_by_name=storage_by_name,
        candidate_only_names=frozenset(authored_icf_candidate_only_names),
        reviewed_icf_group_by_address=reviewed_icf_group_by_address,
        reviewed_icf_group_by_logical_identity=(
            reviewed_icf_group_by_logical_identity
        ),
        reviewed_logical_aliases_by_address={
            address: tuple(aliases)
            for address, aliases in reviewed_logical_aliases_by_address.items()
            if aliases
        },
        reviewed_authored_icf_by_call_site=reviewed_authored_icf_by_call_site,
        reviewed_authored_icf_candidate_by_call_site=(
            reviewed_authored_icf_candidate_by_call_site
        ),
        reviewed_authored_icf_provisional_candidate_by_name=(
            reviewed_authored_icf_provisional_candidate_by_name
        ),
        reviewed_authored_icf_physical_by_logical_identity=(
            reviewed_authored_icf_physical_by_logical_identity
        ),
        reviewed_authored_icf_by_vtable_selector=(
            authored_icf_vtable_selector_index(document.collection("symbols"))
        ),
        reviewed_non_gating_logical_target_by_address=(
            reviewed_non_gating_logical_target_by_address
        ),
        storage_containers=tuple(
            sorted(
                storage_containers,
                key=lambda row: (row.start, row.end_exclusive, row.identity),
            )
        ),
        pointer_vector_destroy_providers=(
            pointer_vector_destroy_providers
        ),
        hud_cmd_binding_ptr_vector_erase_provider=(
            hud_cmd_binding_ptr_vector_erase_provider
        ),
        scalar_deleting_destructors=scalar_deleting_destructors,
        scalar_deleting_destructor_blockers=(
            scalar_deleting_destructor_blockers
        ),
        scalar_deleting_destructor_legacy_fallback_names=(
            scalar_deleting_destructor_legacy_fallback_names
        ),
    )


def _immutable_retail_interval(
    reference: Path,
    *,
    start: int,
    end_exclusive: int,
) -> bytes:
    """Read one exact file-backed interval from the immutable retail image."""

    if end_exclusive <= start:
        return b""
    try:
        with StableReadHandle(reference) as stable_reference:
            reference_data = stable_reference.read()
        headers = parse_pe_headers(reference_data)
    except (OSError, ValueError) as exc:
        raise ValueError(
            "comparison-scoped direct IAT producer cannot read the immutable "
            "retail PE extent"
        ) from exc
    start_rva = start - headers.image_base
    end_rva = end_exclusive - headers.image_base
    start_offset = rva_to_offset(start_rva, headers.sections)
    last_offset = rva_to_offset(end_rva - 1, headers.sections)
    size = end_exclusive - start
    if (
        start_rva < 0
        or end_rva <= start_rva
        or start_offset is None
        or last_offset is None
        or last_offset != start_offset + size - 1
    ):
        raise ValueError(
            "comparison-scoped direct IAT producer retail tail is not one "
            "exact file-backed interval"
        )
    result = reference_data[start_offset : last_offset + 1]
    if len(result) != size:
        raise ValueError(
            "comparison-scoped direct IAT producer retail tail is truncated"
        )
    return result




def _bn_inbound_xref_items(
    payload: Any,
) -> tuple[list[Mapping[str, Any]], list[Mapping[str, Any]], bool]:
    """Return distinct code/data rows from one complete inbound-xref result.

    Pointer-bearing vtables, message maps, initializer arrays, and similar
    containers legitimately create inbound data references to function entry
    addresses.  They are storage provenance, not callable edges, so this
    parser never merges them into the code-reference population.  Generic
    collections must classify every row; an untyped row fails completeness.
    """

    if isinstance(payload, list):
        generic_rows: list[Any] = payload
        payload_mapping: Mapping[str, Any] = {}
    elif isinstance(payload, Mapping):
        payload_mapping = payload
        code_value = payload.get("code_references")
        data_value = payload.get("data_references")
        if isinstance(code_value, list) or isinstance(data_value, list):
            if code_value is not None and not isinstance(code_value, list):
                return [], [], False
            if data_value is not None and not isinstance(data_value, list):
                return [], [], False
            code_rows = [
                row for row in code_value or () if isinstance(row, Mapping)
            ]
            data_rows = [
                row for row in data_value or () if isinstance(row, Mapping)
            ]
            structurally_complete = (
                len(code_rows) == len(code_value or ())
                and len(data_rows) == len(data_value or ())
            )
            total = payload.get("total")
            complete = (
                structurally_complete
                and payload.get("truncated") is not True
                and payload.get("has_more") is not True
            )
            if type(total) is int:
                complete = complete and total == len(code_rows) + len(data_rows)
            return code_rows, data_rows, complete
        generic_rows = []
        for key in ("xrefs", "references", "refs", "items", "results"):
            value = payload.get(key)
            if isinstance(value, list):
                generic_rows = value
                break
        else:
            return [], [], False
    else:
        return [], [], False

    code_rows: list[Mapping[str, Any]] = []
    data_rows: list[Mapping[str, Any]] = []
    complete = True
    for raw_row in generic_rows:
        if not isinstance(raw_row, Mapping):
            complete = False
            continue
        kind = str(raw_row.get("kind", raw_row.get("type", ""))).casefold()
        if "data" in kind:
            data_rows.append(raw_row)
        elif any(token in kind for token in ("code", "call", "jump", "tail")):
            code_rows.append(raw_row)
        else:
            complete = False
    complete = (
        complete
        and payload_mapping.get("truncated") is not True
        and payload_mapping.get("has_more") is not True
    )
    total = payload_mapping.get("total")
    if type(total) is int:
        complete = complete and total == len(generic_rows)
    return code_rows, data_rows, complete


def _bn_xref_address(row: Mapping[str, Any], *keys: str) -> str:
    for key in keys:
        value = row.get(key)
        if isinstance(value, Mapping):
            nested = _bn_xref_address(
                value,
                "address",
                "addr",
                "start",
                "source_address",
                "function_address",
            )
            if nested:
                return nested
        elif value not in {None, ""}:
            try:
                return normalize_address(str(value))
            except (TypeError, ValueError):
                return ""
    return ""


def _bn_unique_containing_function(
    bridge: BinaryNinjaBridge,
    *,
    instruction_address: str,
) -> tuple[str, str]:
    """Resolve an interior instruction through BN's containing-function API.

    ``functionInfo`` is an exact function lookup and deliberately returns a
    structured 404 for an interior instruction address.  The containing
    function must therefore be selected by ``functionAt`` first.  Its unique
    declared name is then used for a named ``functionInfo`` lookup so the
    assembly request is anchored at BN's declared start, never at the xref
    instruction or a tracker contribution boundary.
    """

    normalized_instruction = normalize_address(instruction_address)
    function_at = getattr(bridge, "function_at", None)
    try:
        payload = (
            function_at(normalized_instruction)
            if callable(function_at)
            else bridge.get_json(
                "functionAt",
                address=normalized_instruction,
            )
        )
    except (AttributeError, BridgeError, OSError, RuntimeError, ValueError) as exc:
        raise ValueError(
            "BN inbound-xref authority cannot resolve one containing function"
        ) from exc

    raw_rows: list[Any]
    if isinstance(payload, list):
        raw_rows = payload
    elif isinstance(payload, Mapping):
        raw_rows = []
        for key in ("functions", "containing_functions", "items", "results"):
            value = payload.get(key)
            if isinstance(value, list):
                raw_rows = value
                break
        else:
            if any(key in payload for key in ("name", "function_name")):
                raw_rows = [payload]
    elif isinstance(payload, str):
        raw_rows = [payload]
    else:
        raw_rows = []

    containing_names: list[str] = []
    for row in raw_rows:
        if isinstance(row, str):
            name = row.strip()
        elif isinstance(row, Mapping):
            raw_name = row.get("name", row.get("function_name", ""))
            name = raw_name.strip() if isinstance(raw_name, str) else ""
        else:
            name = ""
        if not name or name in containing_names:
            raise ValueError(
                "BN inbound-xref authority has an ambiguous containing name"
            )
        containing_names.append(name)
    if len(containing_names) != 1:
        raise ValueError(
            "BN inbound-xref authority requires exactly one containing name"
        )

    containing_name = containing_names[0]
    try:
        info = bridge.function_info(containing_name)
    except (AttributeError, BridgeError, OSError, RuntimeError, ValueError) as exc:
        raise ValueError(
            "BN inbound-xref authority cannot resolve the named containing function"
        ) from exc
    function_address = (
        _bn_xref_address(
            info,
            "start",
            "function_start",
            "function",
            "address",
        )
        if isinstance(info, Mapping)
        else ""
    )
    if (
        not function_address
        or address_value(function_address) > address_value(normalized_instruction)
    ):
        raise ValueError(
            "BN inbound-xref authority cannot resolve the named containing start"
        )
    return containing_name, function_address


def _complete_bn_inbound_direct_transfers(
    bridge: BinaryNinjaBridge,
    *,
    target_address: str,
    callback_materializations_out: set[str] | None = None,
) -> tuple[tuple[str, str, tuple[Instruction, ...], str], ...] | None:
    """Prove every inbound code edge is an exact direct E8/E9 transfer.

    Xref source addresses are instruction addresses.  Assembly is requested
    only after ``functionAt`` selects exactly one containing name and named
    ``functionInfo`` supplies its declared start.  Complete data-only inbound
    sets are valid and yield an empty tuple; absence of recognizable complete
    xref authority yields ``None``.
    """

    get_json = getattr(bridge, "get_json", None)
    if not callable(get_json):
        return None
    selected: list[Mapping[str, Any]] | None = None
    for endpoint in ("getXrefsTo",):
        try:
            payload = get_json(endpoint, address=target_address, limit=100000)
        except (BridgeError, OSError, RuntimeError, ValueError):
            return None
        code_rows, _data_rows, complete = _bn_inbound_xref_items(payload)
        if code_rows or complete:
            if not complete:
                raise ValueError(
                    "BN inbound-xref authority is truncated or incomplete"
                )
            selected = code_rows
            break
    if selected is None:
        return None
    assembly_by_function: dict[str, tuple[Instruction, ...]] = {}
    transfers: list[tuple[str, str, tuple[Instruction, ...], str]] = []
    seen_sources: set[str] = set()
    for row in selected:
        source_address = _bn_xref_address(
            row,
            "source_address",
            "source_addr",
            "from_address",
            "from_addr",
            "address",
            "addr",
            "source",
            "from",
        )
        if not source_address or source_address in seen_sources:
            raise ValueError(
                "BN inbound-xref authority has an ambiguous code source address"
            )
        seen_sources.add(source_address)
        if (
            normalize_address(target_address) == "0x4bcb50"
            and source_address == "0x4bcb4f"
        ):
            # BN inventories the immediately preceding padding byte as a code
            # xref to the next function.  Suppress only this reviewed row and
            # only after immutable retail proves that the source is one NOP,
            # hence neither E8 nor E9 nor any other transfer instruction.
            if _cc_cfg._hexdump_bytes(bridge.hexdump(source_address, 1)) != b"\x90":
                raise ValueError(
                    "reviewed HudUiTextLabel padding xref byte drifted"
                )
            continue
        _function_name, function_address = _bn_unique_containing_function(
            bridge,
            instruction_address=source_address,
        )
        caller_rows = assembly_by_function.get(function_address)
        if caller_rows is None:
            caller_rows = tuple(
                _cc_listing.parse_assembly(bridge.assembly(function_address), source="bn")
            )
            assembly_by_function[function_address] = caller_rows
        matching = [
            instruction
            for instruction in caller_rows
            if _cc_cfg._source_instruction_address(instruction) == source_address
        ]
        if len(matching) != 1:
            raise ValueError(
                "BN inbound-xref authority cannot locate one exact caller instruction"
            )
        instruction = matching[0]
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            body = b""
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        expected_opcode = 0xE8 if mnemonic == "call" else 0xE9
        if (
            mnemonic not in {"call", "jmp"}
            or len(body) != 5
            or body[0] != expected_opcode
            or int(source_address, 16) + 5 + struct.unpack("<i", body[1:5])[0]
            != address_value(target_address)
        ):
            if _cc_callbacks._exact_inbound_callback_address_materialization(
                caller_rows,
                source_address=source_address,
                target_address=target_address,
            ):
                if callback_materializations_out is not None:
                    callback_materializations_out.add(source_address)
                continue
            raise ValueError(
                "BN inbound-xref authority requires exact direct E8/E9 transfers"
            )
        transfers.append(
            (source_address, function_address, caller_rows, mnemonic)
        )
    return tuple(transfers)


def _exact_unknown_push32(instruction: Instruction) -> bool:
    """Recognize an exact non-register x86 PUSH with a four-byte stack effect."""

    if _cc_cfg._instruction_mnemonic(instruction) != "push":
        return False
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return False
    operand = _cc_cfg._instruction_operand(instruction).strip().lower()
    if len(body) == 2 and body[0] == 0x6A:
        try:
            rendered = (-_cc_cfg._parse_unsigned_assembly_integer(operand[1:])
                if operand.startswith("-") else _cc_cfg._parse_unsigned_assembly_integer(operand))
        except ValueError:
            return False
        encoded = struct.unpack("<b", body[1:2])[0]
        return rendered in {encoded, encoded & 0xFFFFFFFF, body[1]}
    if len(body) == 5 and body[0] == 0x68:
        try:
            rendered = (-_cc_cfg._parse_unsigned_assembly_integer(operand[1:])
                if operand.startswith("-") else _cc_cfg._parse_unsigned_assembly_integer(operand))
        except ValueError:
            return False
        return -0x80000000 <= rendered <= 0xFFFFFFFF and (rendered & 0xFFFFFFFF) == struct.unpack("<I", body[1:5])[0]
    return bool(
        len(body) >= 2
        and body[0] == 0xFF
        and ((body[1] >> 3) & 7) == 6
        and body[1] >> 6 != 3
        and _cc_catalog.MEMORY_RE.search(_cc_cfg._instruction_operand(instruction)) is not None
    )


def _exact_unknown_pop32(instruction: Instruction) -> bool:
    """Recognize an exact memory POP with a four-byte stack effect."""

    if _cc_cfg._instruction_mnemonic(instruction) != "pop":
        return False
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return False
    return bool(
        len(body) >= 2
        and body[0] == 0x8F
        and ((body[1] >> 3) & 7) == 0
        and body[1] >> 6 != 3
        and _cc_catalog.MEMORY_RE.search(_cc_cfg._instruction_operand(instruction)) is not None
    )


def _exact_ret_cleanup_bytes(instruction: Instruction) -> int | None:
    """Decode one exact unprefixed near x86 RET stack effect."""

    if _cc_cfg._instruction_mnemonic(instruction) not in {"ret", "retn"}:
        return None
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    operand = _cc_cfg._instruction_operand(instruction).strip().lower()
    if body == b"\xc3":
        # VC5's COD renders the operand-free C3 encoding as "ret 0".
        return 0 if operand in {"", "0", "0x0"} else None
    if len(body) != 3 or body[0] != 0xC2:
        return None
    cleanup = struct.unpack("<H", body[1:3])[0]
    try:
        rendered = _cc_cfg._parse_unsigned_assembly_integer(operand)
    except ValueError:
        return None
    return cleanup if rendered == cleanup and cleanup % 4 == 0 else None


def _retail_direct_call_cleanup_by_ordinal(
    instructions: Sequence[Instruction],
    *,
    caller_start: str,
    caller_end_exclusive: str,
    bridge: BinaryNinjaBridge,
) -> dict[int, int]:
    """Prove direct-call cleanup only from a callee's unanimous exact RETs."""

    addresses = _cc_cfg._instruction_runtime_addresses(
        instructions,
        source="bn",
        caller_start=address_value(caller_start),
    )
    result: dict[int, int] = {}
    ordinal = 0
    cache: dict[str, int | None] = {}
    counts: dict[int, int] = {}
    for row in addresses:
        if row is not None:
            counts[row] = counts.get(row, 0) + 1
    by_address = {
        row: index
        for index, row in enumerate(addresses)
        if row is not None and counts[row] == 1
    }
    for index, (instruction, address) in enumerate(zip(instructions, addresses)):
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        if mnemonic not in {"call", "jmp"}:
            continue
        local = _cc_cfg._exact_local_direct_branch(
            instruction,
            instruction_index=index,
            instruction_addresses=addresses,
            instruction_index_by_address=by_address,
            source="bn",
            caller_start=address_value(caller_start),
            caller_end=address_value(caller_end_exclusive),
        )
        if mnemonic == "jmp" and local is not None:
            continue
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            body = b""
        if mnemonic == "call" and address is not None and len(body) == 5 and body[0] == 0xE8:
            target = normalize_address(
                address + 5 + struct.unpack_from("<i", body, 1)[0]
            )
            cleanup = cache.get(target)
            if target not in cache:
                try:
                    callee = _cc_listing.parse_assembly(bridge.assembly(target), source="bn")
                except (BridgeError, OSError, RuntimeError, ValueError):
                    cleanup = None
                else:
                    returns = [
                        value
                        for row in callee
                        if (value := _exact_ret_cleanup_bytes(row)) is not None
                    ]
                    cleanup = (
                        next(iter(set(returns)))
                        if returns and len(set(returns)) == 1
                        else None
                    )
                cache[target] = cleanup
            if cleanup is not None:
                result[ordinal] = cleanup
        ordinal += 1
    return result


def _retail_direct_call_cleanup_by_instruction_index(
    instructions: Sequence[Instruction],
    *,
    caller_start: str,
    caller_end_exclusive: str,
    bridge: BinaryNinjaBridge,
) -> dict[int, int]:
    """Map exact direct-call rows to unanimous callee RET stack effects."""

    cleanup_by_ordinal = _retail_direct_call_cleanup_by_ordinal(
        instructions,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        bridge=bridge,
    )
    addresses = _cc_cfg._instruction_runtime_addresses(
        instructions,
        source="bn",
        caller_start=address_value(caller_start),
    )
    counts: dict[int, int] = {}
    for address in addresses:
        if address is not None:
            counts[address] = counts.get(address, 0) + 1
    index_by_address = {
        address: index
        for index, address in enumerate(addresses)
        if address is not None and counts[address] == 1
    }
    start = address_value(caller_start)
    end = address_value(caller_end_exclusive)
    result: dict[int, int] = {}
    ordinal = 0
    for index, instruction in enumerate(instructions):
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        if mnemonic not in {"call", "jmp"}:
            continue
        local = _cc_cfg._exact_local_direct_branch(
            instruction,
            instruction_index=index,
            instruction_addresses=addresses,
            instruction_index_by_address=index_by_address,
            source="bn",
            caller_start=start,
            caller_end=end,
        )
        if mnemonic == "jmp" and local is not None:
            continue
        if ordinal in cleanup_by_ordinal:
            try:
                body = bytes(int(item, 16) for item in instruction.bytes)
            except (TypeError, ValueError):
                body = b""
            if mnemonic != "call" or len(body) != 5 or body[0] != 0xE8:
                raise ValueError(
                    "retail direct-callee cleanup joined a non-E8 instruction"
                )
            result[index] = cleanup_by_ordinal[ordinal]
        ordinal += 1
    return result


def _compose_caller_scoped_retail_proof_package(
    retail_instructions: Sequence[Instruction],
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    caller_execution_end_exclusive: str | None = None,
    indexes: IdentityIndexes,
    bridge: BinaryNinjaBridge,
    reference: Path = _cc_catalog.DEFAULT_REFERENCE,
    retail_import_targets: Sequence[Any] | None = None,
    reviewed_provenance_adapters: ReviewedRetailProvenanceAdapters | None = None,
    _trace_stage: Callable[..., None] | None = None,
) -> tuple[IdentityIndexes, RetailCallerScopedProofPackage]:
    """Compose retail-only caller facts before any extraction consumer."""
    from _recoil.call_contract.records import RetailCallerScopedProofPackage

    def trace(stage: str, **extra: Any) -> None:
        if _trace_stage is not None:
            _trace_stage(stage, **extra)

    def trace_overflow(fields: Mapping[str, Any]) -> None:
        trace("retail-register-stack-overflow", **dict(fields))

    instructions = tuple(retail_instructions)
    comparison_end_exclusive = (
        normalize_address(caller_execution_end_exclusive)
        if caller_execution_end_exclusive is not None
        else normalize_address(caller_end_exclusive)
    )
    if not (
        address_value(caller_start)
        < address_value(comparison_end_exclusive)
        <= address_value(caller_end_exclusive)
    ):
        raise ValueError(
            "retail caller proof execution extent escapes its byte envelope"
        )
    reviewed_adapter_sites: frozenset[str] = frozenset()
    reviewed_direct_thunk_sites: frozenset[str] = frozenset()
    reviewed_register_call_sites: frozenset[str] = frozenset()
    reviewed_vptr_call_sites: frozenset[str] = frozenset()
    reviewed_non_callback_loads: Mapping[str, str] = {}
    preempt_generic_register_iat = False
    if reviewed_provenance_adapters is not None:
        if indexes != reviewed_provenance_adapters.indexes:
            raise ValueError(
                "reviewed retail provenance adapters must compose immediately "
                "before the generic caller proof package"
            )
        reviewed_adapter_sites = frozenset(
            normalize_address(address)
            for address in reviewed_provenance_adapters.reviewed_call_sites
        )
        if len(reviewed_adapter_sites) != len(
            reviewed_provenance_adapters.reviewed_call_sites
        ):
            raise ValueError(
                "reviewed retail provenance adapter call sites collide"
            )
        reviewed_direct_thunk_sites = frozenset(
            reviewed_adapter_sites
            & reviewed_provenance_adapters.indexes
            .reviewed_direct_import_thunk_by_call_site.keys()
        )
        reviewed_register_call_sites = frozenset(
            normalize_address(address)
            for address in reviewed_provenance_adapters
            .register_call_storage_bridges
        )
        reviewed_vptr_call_sites = frozenset(
            normalize_address(address)
            for address in reviewed_provenance_adapters.vptr_storage_bridges
        )
        reviewed_non_callback_loads = {
            normalize_address(address): identity
            for address, identity in reviewed_provenance_adapters
            .non_callback_register_loads.items()
        }
        preempt_generic_register_iat = (
            reviewed_provenance_adapters.preempt_generic_register_iat
        )
        if preempt_generic_register_iat:
            preemption_shape = (
                normalize_address(caller_start),
                comparison_end_exclusive,
                reviewed_register_call_sites,
                reviewed_vptr_call_sites,
                frozenset(reviewed_non_callback_loads),
            )
            if preemption_shape not in {
                (
                    "0x46de50", "0x46df50",
                    frozenset({"0x46dea4"}), frozenset(),
                    frozenset({"0x46de99"}),
                ),
                (
                    "0x48a520", "0x48a909", frozenset(),
                    frozenset({"0x48a564", "0x48a897"}), frozenset(),
                ),
            }:
                raise ValueError(
                    "reviewed register-IAT preemption lacks its exact finite "
                    "caller/load/invocation population"
                )
        if len(reviewed_non_callback_loads) != len(
            reviewed_provenance_adapters.non_callback_register_loads
        ):
            raise ValueError(
                "reviewed non-callback load publications collide"
            )
        if not (
            reviewed_direct_thunk_sites
            | reviewed_register_call_sites
            | reviewed_vptr_call_sites
        ) <= reviewed_adapter_sites:
            raise ValueError(
                "reviewed retail provenance adapter publications are absent "
                "from their complete reviewed call-site population"
            )
    trace("retail-proof-switch-start")
    # Switch tables are non-executed data and may live in the retained physical
    # envelope after the executable instruction end.  Targets still have to
    # resolve to unique instruction rows, so this broadens only table lookup,
    # never the executable provenance population.
    switch_targets = _cc_cfg.retail_local_switch_targets(
        instructions,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        bridge=bridge,
    )
    znetwork_switch_targets = (
        _cc_recoil_network._znetwork_open_selected_session_retail_switch_targets(
            instructions,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
            caller_execution_end_exclusive=comparison_end_exclusive,
            bridge=bridge,
        )
    )
    if any(
        switch_targets[index] != targets
        for index, targets in znetwork_switch_targets.items()
        if index in switch_targets
    ):
        raise ValueError(
            "zNetwork OpenSelectedSession finite switch proof conflicts with "
            "generic retail switch classification"
        )
    switch_targets.update(znetwork_switch_targets)
    switch_indices = frozenset(switch_targets)
    trace("retail-proof-switch-complete")
    direct_cleanup_by_index: dict[int, int] | None = None
    trace("retail-proof-stack-effects-start")
    for instruction in instructions:
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            continue
        if (
            (len(body) == 5 and body[0] == 0xA1)
            or (
                len(body) == 6
                and body[0] == 0x8B
                and body[1] >> 6 == 0
                and body[1] & 7 == 5
            )
        ):
            direct_cleanup_by_index = (
                _retail_direct_call_cleanup_by_instruction_index(
                    instructions,
                    caller_start=caller_start,
                    caller_end_exclusive=comparison_end_exclusive,
                    bridge=bridge,
                )
            )
            break
    trace("retail-proof-stack-effects-complete")
    trace("retail-proof-zero-callback-start")
    indexes = _cc_callbacks._comparison_scoped_retail_zero_callback_storage_indexes(
        instructions,
        document=document,
        caller_start=caller_start,
        caller_end_exclusive=comparison_end_exclusive,
        indexes=indexes,
        bridge=bridge,
        local_control_flow_indices=switch_indices,
        local_control_flow_targets=switch_targets,
        precomposed_non_callback_loads=reviewed_non_callback_loads,
        call_cleanup_by_instruction_index=direct_cleanup_by_index,
        _trace_overflow=trace_overflow,
    )
    trace("retail-proof-zero-callback-complete")
    trace("retail-proof-stored-callback-start")
    indexes = _cc_callbacks._comparison_scoped_retail_stored_callback_targets(
        instructions,
        document=document,
        caller_start=caller_start,
        caller_end_exclusive=comparison_end_exclusive,
        indexes=indexes,
        bridge=bridge,
        local_control_flow_indices=switch_indices,
        local_control_flow_targets=switch_targets,
        precomposed_non_callback_loads=reviewed_non_callback_loads,
        call_cleanup_by_instruction_index=direct_cleanup_by_index,
        _trace_overflow=trace_overflow,
    )
    trace("retail-proof-stored-callback-complete")
    stored_callback_load_indices: set[int] = set()
    for index, instruction in enumerate(instructions):
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            continue
        accumulator = len(body) == 5 and body[0] == 0xA1
        absolute = (
            len(body) == 6
            and body[0] == 0x8B
            and body[1] >> 6 == 0
            and body[1] & 7 == 5
        )
        if not accumulator and not absolute:
            continue
        slot = normalize_address(
            struct.unpack_from("<I", body, 1 if accumulator else 2)[0]
        )
        storage_identity = indexes.storage_by_address.get(slot, "")
        if storage_identity in indexes.reviewed_static_callback_target_by_storage:
            if storage_identity.startswith("iat:"):
                raise ValueError(
                    "stored-callback proof package collides with an IAT identity"
                )
            stored_callback_load_indices.add(index)

    trace("retail-proof-local-import-thunk-start")
    indexes = _cc_retail_imports._retail_local_named_import_thunk_indexes(
        instructions,
        document=document,
        indexes=indexes,
        bridge=bridge,
        caller_start=caller_start,
        retail_import_targets=retail_import_targets,
        precomposed_call_sites=reviewed_direct_thunk_sites,
    )
    trace("retail-proof-local-import-thunk-complete")
    trace("retail-proof-direct-iat-start")
    indexes = _cc_iat._comparison_scoped_direct_iat_indexes(
        instructions,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        reference=reference,
        retail_import_targets=retail_import_targets,
    )
    trace("retail-proof-direct-iat-complete")
    prior_callback_targets = dict(
        indexes.reviewed_static_callback_target_by_storage
    )
    trace("retail-proof-static-callback-start")
    indexes = _cc_callbacks._comparison_scoped_retail_static_callback_targets(
        instructions,
        indexes=indexes,
        bridge=bridge,
    )
    trace("retail-proof-static-callback-complete")
    if any(
        indexes.reviewed_static_callback_target_by_storage.get(storage)
        != target
        for storage, target in prior_callback_targets.items()
    ):
        raise ValueError(
            "stored-callback proof package conflicts with static callback classification"
        )
    trace("retail-proof-register-iat-start")
    if not preempt_generic_register_iat:
        indexes = _cc_iat._comparison_scoped_retail_register_iat_indexes(
            instructions,
            document=document,
            caller_identity=caller_identity,
            caller_start=caller_start,
            caller_end_exclusive=comparison_end_exclusive,
            indexes=indexes,
            reference=reference,
            retail_import_targets=retail_import_targets,
            stored_callback_load_indices=frozenset(stored_callback_load_indices),
            local_control_flow_indices=switch_indices,
            local_control_flow_targets=switch_targets,
            precomposed_non_iat_loads=reviewed_non_callback_loads,
            call_cleanup_by_instruction_index=direct_cleanup_by_index,
            _trace_overflow=trace_overflow,
        )
    trace("retail-proof-register-iat-complete")
    if reviewed_register_call_sites:
        filtered_iat_proofs: dict[
            str, CandidateExactIatRegisterLoadProof
        ] = {}
        for definition_address, proof in (
            indexes.reviewed_retail_iat_load_proofs.items()
        ):
            transfer_rows = tuple(
                zip(
                    proof.transfer_offsets,
                    proof.transfer_instruction_indices,
                    proof.transfer_body_offsets,
                )
            )
            retained = tuple(
                row
                for row in transfer_rows
                if normalize_address(row[0])
                not in reviewed_register_call_sites
            )
            if retained:
                filtered_iat_proofs[definition_address] = replace(
                    proof,
                    transfer_offsets=tuple(row[0] for row in retained),
                    transfer_registers=tuple(proof.transfer_register(row[0]) for row in retained),
                    transfer_instruction_indices=tuple(
                        int(row[1]) for row in retained
                    ),
                    transfer_body_offsets=tuple(
                        int(row[2]) for row in retained
                    ),
                )
        indexes = replace(
            indexes,
            reviewed_retail_iat_load_proofs=filtered_iat_proofs,
        )
    trace("retail-proof-direct-cleanup-start")
    if (
        any(
            _cc_receiver_instructions._exact_stack_slot_load(row) is not None
            or _cc_receiver_instructions._exact_stack_slot_store(row) is not None
            for row in instructions
        )
        and any(
            _cc_cfg._instruction_mnemonic(row) in {"call", "jmp"}
            and _cc_catalog.MEMORY_RE.search(_cc_cfg._instruction_operand(row)) is not None
            for row in instructions
        )
    ):
        if direct_cleanup_by_index is None:
            direct_cleanup_by_index = (
                _retail_direct_call_cleanup_by_instruction_index(
                    instructions,
                    caller_start=caller_start,
                    caller_end_exclusive=comparison_end_exclusive,
                    bridge=bridge,
                )
            )
    trace("retail-proof-direct-cleanup-complete")
    from _recoil.call_contract.virtual_callees import native_virtual_cleanup
    virtual_cleanup = native_virtual_cleanup(instructions, document=document,
        indexes=indexes, caller_start=normalize_address(caller_start),
        caller_end=comparison_end_exclusive, known_cleanup=direct_cleanup_by_index or {}, bridge=bridge)
    if virtual_cleanup:
        direct_cleanup_by_index = dict(direct_cleanup_by_index or {}) | virtual_cleanup
    trace("retail-proof-targetless-vptr-start")
    targetless_proofs = _cc_receiver_proofs._exact_targetless_vptr_call_proofs(
        instructions,
        source="bn",
        caller_start=caller_start,
        caller_end_exclusive=comparison_end_exclusive,
        indexes=indexes,
        local_control_flow_indices=switch_indices,
        local_control_flow_targets=switch_targets,
        call_cleanup_by_instruction_index=(
            direct_cleanup_by_index or {}
        ),
    )
    trace("retail-proof-targetless-vptr-complete")
    _cc_proofs.merge_into(targetless_proofs, getattr(virtual_cleanup, "receivers", {}),
        family="identity.native_virtual_receivers")
    trace("retail-proof-zvid-dd-com-start")
    zvid_dd_com_proofs = _cc_receiver_fields._exact_retail_zvid_dd_com_vptr_call_proofs(
        instructions,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=comparison_end_exclusive,
        indexes=indexes,
        bridge=bridge,
        local_control_flow_indices=switch_indices,
        local_control_flow_targets=switch_targets,
    )
    trace("retail-proof-zvid-dd-com-complete")
    if any(
        instruction_index in targetless_proofs
        and targetless_proofs[instruction_index] != marker
        for instruction_index, marker in zvid_dd_com_proofs.items()
    ):
        raise ValueError(
            "zVid DD COM-vtable lineage collides with another targetless proof"
        )
    _cc_proofs.merge_into(targetless_proofs, zvid_dd_com_proofs, family="identity.targetless_proofs")
    if reviewed_vptr_call_sites:
        addresses = _cc_cfg._instruction_runtime_addresses(
            instructions,
            source="bn",
            caller_start=address_value(caller_start),
        )
        targetless_proofs = {
            instruction_index: proof
            for instruction_index, proof in targetless_proofs.items()
            if addresses[instruction_index] is None
            or normalize_address(addresses[instruction_index])
            not in reviewed_vptr_call_sites
        }
    iat_instruction_objects = tuple(
        (
            definition_address,
            instructions[int(proof.definition_instruction_index)],
            tuple(
                instructions[index]
                for index in proof.transfer_instruction_indices
            ),
        )
        for definition_address, proof in sorted(
            indexes.reviewed_retail_iat_load_proofs.items(),
            key=lambda row: address_value(row[0]),
        )
    )
    package = RetailCallerScopedProofPackage(
        caller_identity=caller_identity,
        caller_start=normalize_address(caller_start),
        caller_end_exclusive=comparison_end_exclusive,
        instruction_objects=instructions,
        iat_instruction_objects=iat_instruction_objects,
        stored_callback_load_indices=frozenset(stored_callback_load_indices),
        call_cleanup_by_instruction_index=tuple(
            sorted((direct_cleanup_by_index or {}).items())
        ),
        targetless_vptr_call_proofs=tuple(sorted(targetless_proofs.items())),
        local_control_flow_targets=tuple(sorted(switch_targets.items())),
    )
    return replace(
        indexes,
        active_retail_caller_proof_package=package,
    ), package


def _exact_bounded_forward_stack_copy_range(
    instructions: Sequence[Instruction],
    *,
    lea_index: int,
    stack_delta: int,
    successors: Mapping[int, Sequence[int]],
    source: str = "bn",
) -> tuple[int, int] | None:
    """Prove one exact forward ``REP MOVSD`` destination stack interval.

    This owns only the four-instruction VC5 shape ``MOV ECX,imm32``;
    ``MOV ESI,r32``; ``LEA EDI,[ESP+disp]``; ``REP MOVSD``.  Exact singleton
    CFG edges prevent a branch from entering with a different count or
    destination.  The returned half-open interval lets stack-lineage callers
    invalidate only slots actually overwritten instead of treating the
    bounded local-frame copy as an arbitrary stack-address escape.
    """

    if lea_index < 2 or lea_index + 1 >= len(instructions):
        return None
    count_index = lea_index - 2
    source_index = lea_index - 1
    copy_index = lea_index + 1
    if (
        tuple(successors.get(count_index, ())) != (source_index,)
        or tuple(successors.get(source_index, ())) != (lea_index,)
        or tuple(successors.get(lea_index, ())) != (copy_index,)
    ):
        return None
    try:
        count_body = bytes(
            int(item, 16) for item in instructions[count_index].bytes
        )
        copy_body = bytes(
            int(item, 16) for item in instructions[copy_index].bytes
        )
    except (TypeError, ValueError):
        return None
    count_operands = _cc_cfg._instruction_operand(
        instructions[count_index]
    ).split(",", 1)
    if (
        len(count_body) != 5
        or count_body[0] != 0xB9
        or _cc_cfg._instruction_mnemonic(instructions[count_index]) != "mov"
        or len(count_operands) != 2
        or count_operands[0].strip().lower() != "ecx"
        or copy_body != b"\xf3\xa5"
        or "movs" not in instructions[copy_index].raw_text.lower()
    ):
        return None
    count = struct.unpack_from("<I", count_body, 1)[0]
    if (
        count == 0
        or count > 0x4000
        or _cc_cfg._parse_unsigned_assembly_integer(count_operands[1].strip()) != count
    ):
        return None
    source_move = _cc_receiver_instructions._exact_register_move(instructions[source_index])
    stack_lea = _cc_receiver_candidate._exact_stack_address_lea(instructions[lea_index])
    if stack_lea is None and source == "cod":
        symbolic_stack_lea = _cc_receiver_instructions._exact_vc5_symbolic_stack_address_lea(
            instructions[lea_index]
        )
        if symbolic_stack_lea is not None:
            stack_lea = symbolic_stack_lea[:2]
    if (
        source_move is None
        or source_move[0] != "esi"
        or stack_lea is None
        or stack_lea[0] != "edi"
    ):
        return None
    start = stack_delta + stack_lea[1]
    end = start + count * 4
    if not -0x10000 <= start < end <= 0x10000:
        return None
    return start, end


def _exact_stack_receiver_escape_start(
    instructions: Sequence[Instruction],
    *,
    lea_index: int,
    stack_delta: int,
    successors: Mapping[int, Sequence[int]],
    source: str = "bn",
) -> int | None:
    """Return the lower bound of one exact ECX stack receiver invocation."""

    stack_lea = _cc_receiver_candidate._exact_stack_address_lea(instructions[lea_index])
    symbolic_stack_lea = None
    if stack_lea is None and source == "cod":
        symbolic_stack_lea = _cc_receiver_instructions._exact_vc5_symbolic_stack_address_lea(
            instructions[lea_index]
        )
        if symbolic_stack_lea is not None:
            stack_lea = symbolic_stack_lea[:2]
    if stack_lea is None or stack_lea[0] != "ecx":
        return None
    if symbolic_stack_lea is not None:
        _destination, encoded, symbol, rendered = symbolic_stack_lea
        symbolic_root = (symbol, rendered - encoded)
        matching_copy_leas = [
            prior_index
            for prior_index in range(lea_index)
            if (
                prior_lea := _cc_receiver_instructions._exact_vc5_symbolic_stack_address_lea(
                    instructions[prior_index]
                )
            )
            is not None
            and prior_lea[0] == "edi"
            and (prior_lea[2], prior_lea[3] - prior_lea[1])
            == symbolic_root
            and _exact_bounded_forward_stack_copy_range(
                instructions,
                lea_index=prior_index,
                stack_delta=0,
                successors=successors,
                source=source,
            )
            is not None
        ]
        if len(matching_copy_leas) != 1:
            return None
    successor_rows = tuple(successors.get(lea_index, ()))
    if len(successor_rows) != 1:
        return None
    invocation = instructions[successor_rows[0]]
    mnemonic = _cc_cfg._instruction_mnemonic(invocation)
    operand = re.sub(
        r"^(?:near|far)\s+(?:ptr\s+)?",
        "",
        _cc_cfg._instruction_operand(invocation).strip().lower(),
    )
    if (
        mnemonic not in {"call", "jmp"}
        or _cc_catalog.REGISTER_RE.fullmatch(operand) is None
        or not _cc_cfg._exact_invocation_encoding(invocation, mnemonic=mnemonic)
    ):
        return None
    return stack_delta + stack_lea[1]


def _preserved_entry_stack_argument_lineages(
    instructions: Sequence[Instruction],
    calls: Sequence[Mapping[str, Any]],
) -> dict[int, str]:
    """Find exact pushed nonvolatile copies of one entry-stack argument.

    Slot writes after the defining load do not change the copied register.
    Competing definitions, register kills, or any intervening control-flow
    edge fail closed and publish no observation.
    """

    register_names = (
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    )
    definitions: dict[str, tuple[int, str]] = {}
    ambiguous: set[str] = set()
    invocation_ordinal = 0
    result: dict[int, str] = {}
    stack_delta = 0
    for index, instruction in enumerate(instructions):
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            body = b""
        operands = _cc_cfg._instruction_operand(instruction).split(",", 1)
        stack_load = _cc_receiver_instructions._exact_stack_slot_load(instruction)
        if stack_load is not None:
            destination, displacement = stack_load
            entry_displacement = stack_delta + displacement
            if (
                destination in {"ebx", "esi", "edi"}
                and entry_displacement >= 4
            ):
                if destination in definitions:
                    ambiguous.add(destination)
                definitions[destination] = (
                    index,
                    _cc_receiver_instructions._abstract_with_displacement(
                        "entry-stack",
                        entry_displacement,
                    ),
                )
                continue
        if (
            mnemonic == "mov"
            and len(operands) == 2
            and len(body) in {3, 6}
            and body[0] == 0x8B
            and body[1] >> 6 in {1, 2}
            and (body[1] & 7) == 5
        ):
            destination = register_names[(body[1] >> 3) & 7]
            displacement = (
                struct.unpack("<b", body[2:3])[0]
                if len(body) == 3
                else struct.unpack("<i", body[2:6])[0]
            )
            rendered, rendered_displacement = _cc_targets._memory_slot(operands[1])
            if (
                destination in {"ebx", "esi", "edi"}
                and rendered.startswith("ebp")
                and displacement >= 8
                and rendered_displacement == displacement
                and operands[0].strip().lower() == destination
            ):
                if destination in definitions:
                    ambiguous.add(destination)
                definitions[destination] = (
                    index,
                    _cc_receiver_instructions._abstract_with_displacement("entry-stack", displacement),
                )
                continue
        if mnemonic.startswith("j") or mnemonic.startswith("loop"):
            definitions.clear()
            ambiguous.update({"ebx", "esi", "edi"})
        if mnemonic in {"call", "jmp"}:
            if invocation_ordinal < len(calls):
                target = calls[invocation_ordinal].get("target_identity")
                if target == _cc_catalog.CACHED_FREAD_IAT_IDENTITY and index > 0:
                    pushed: list[str] = []
                    for prior in range(index - 1, max(-1, index - 9), -1):
                        previous = instructions[prior]
                        previous_mnemonic = _cc_cfg._instruction_mnemonic(previous)
                        if previous_mnemonic in {"call", "jmp"} or (
                            previous_mnemonic.startswith("j")
                            or previous_mnemonic.startswith("loop")
                        ):
                            break
                        try:
                            previous_body = bytes(
                                int(item, 16) for item in previous.bytes
                            )
                        except (TypeError, ValueError):
                            previous_body = b""
                        if len(previous_body) == 1 and 0x50 <= previous_body[0] <= 0x57:
                            register = register_names[previous_body[0] - 0x50]
                            if _cc_cfg._instruction_operand(previous).strip().lower() == register:
                                pushed.append(register)
                    eligible = [
                        register
                        for register in pushed
                        if register not in ambiguous
                        and register in definitions
                    ]
                    if len(set(eligible)) == 1:
                        register = eligible[0]
                        result[invocation_ordinal] = definitions[register][1]
            invocation_ordinal += 1
            if mnemonic == "call":
                for volatile in ("eax", "ecx", "edx"):
                    definitions.pop(volatile, None)
            continue
        if (
            mnemonic in {"add", "sub"}
            and re.fullmatch(
                r"(?:add|sub)\s+esp\s*,\s*(?:0x[0-9a-f]+|\d+)",
                instruction.raw_text.strip(),
                flags=re.IGNORECASE,
            )
        ):
            immediate = _cc_cfg._parse_unsigned_assembly_integer(
                _cc_cfg._instruction_operand(instruction).split(",", 1)[1].strip()
            )
            if _cc_receiver_storage._is_exact_safe_stack_root_adjustment(
                instruction,
                operation=mnemonic,
                parsed_immediate=immediate,
            ):
                stack_delta += immediate if mnemonic == "add" else -immediate
            else:
                definitions.clear()
                ambiguous.update({"ebx", "esi", "edi"})
            continue
        if len(body) == 1 and 0x50 <= body[0] <= 0x57 and mnemonic == "push":
            stack_delta -= 4
            continue
        if len(body) == 1 and 0x58 <= body[0] <= 0x5F and mnemonic == "pop":
            stack_delta += 4
            definitions.pop(register_names[body[0] - 0x58], None)
            continue
        for written in _cc_instructions.written_registers(instruction):
            definitions.pop(written, None)
    return result


def _retail_call_contract_comparison_end_exclusive(
    retail_instructions: Sequence[Instruction],
    *,
    caller_start: str,
    caller_end_exclusive: str,
) -> str:
    """Return the finite executable comparison end for one retail caller.

    The tracker extent remains the physical/function byte envelope.  One
    reviewed zNetwork body owns non-executed alignment, switch-table, case-map,
    and padding bytes after its terminal RET, so its invocation comparison must
    use the independently complete BN instruction population instead.  Every
    other caller retains its tracker extent unchanged.
    """

    start = normalize_address(caller_start)
    envelope_end = normalize_address(caller_end_exclusive)
    if start != _cc_catalog._ZNETWORK_OPEN_SELECTED_SESSION_START:
        return envelope_end
    if envelope_end != _cc_catalog._ZNETWORK_OPEN_SELECTED_SESSION_ENVELOPE_END:
        raise ValueError(
            "zNetwork OpenSelectedSession tracker byte envelope drifted"
        )

    spans: list[tuple[int, int, Instruction, bytes]] = []
    for instruction in retail_instructions:
        raw_address = _cc_cfg._source_instruction_address(instruction)
        if raw_address is None:
            raise ValueError(
                "zNetwork OpenSelectedSession execution extent requires "
                "complete address-labelled BN instruction rows"
            )
        try:
            address = address_value(normalize_address(raw_address))
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError) as exc:
            raise ValueError(
                "zNetwork OpenSelectedSession execution extent requires "
                "complete BN instruction bytes"
            ) from exc
        instruction_end = address + len(body)
        if (
            not body
            or address < address_value(start)
            or instruction_end > address_value(envelope_end)
        ):
            raise ValueError(
                "zNetwork OpenSelectedSession BN instruction row escapes its "
                "tracker byte envelope"
            )
        spans.append((address, instruction_end, instruction, body))

    if not spans or spans != sorted(spans, key=lambda row: row[0]):
        raise ValueError(
            "zNetwork OpenSelectedSession execution extent requires one "
            "ordered complete BN instruction population"
        )
    if len({address for address, _end, _row, _body in spans}) != len(spans):
        raise ValueError(
            "zNetwork OpenSelectedSession BN instruction coordinates collide"
        )
    if spans[0][0] != address_value(start) or any(
        left[1] != right[0] for left, right in zip(spans, spans[1:])
    ):
        raise ValueError(
            "zNetwork OpenSelectedSession BN instruction population is not "
            "complete and contiguous"
        )

    derived_end = max(end for _address, end, _row, _body in spans)
    terminal_rows = [
        (instruction, body)
        for address, _end, instruction, body in spans
        if address
        == address_value(_cc_catalog._ZNETWORK_OPEN_SELECTED_SESSION_TERMINAL_RET)
    ]
    if (
        derived_end
        != address_value(_cc_catalog._ZNETWORK_OPEN_SELECTED_SESSION_EXECUTION_END)
        or len(terminal_rows) != 1
        or terminal_rows[0][1] != b"\xc3"
        or _cc_cfg._instruction_mnemonic(terminal_rows[0][0]) not in {"ret", "retn"}
        or _cc_cfg._instruction_operand(terminal_rows[0][0]).strip()
    ):
        raise ValueError(
            "zNetwork OpenSelectedSession complete retail instruction "
            "execution end drifted"
        )
    return normalize_address(derived_end)


def _reviewed_r3994_dplay_ordinal_thunk_indexes(
    retail_instructions: Sequence[Instruction],
    *,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge: BinaryNinjaBridge,
    ordinal_provider_thunks: Sequence[ProviderOrdinalImportThunk] = (),
    retail_import_targets: Sequence[Any] | None = None,
) -> IdentityIndexes:
    """Bind the one reviewed DPLAYX ordinal-4 local FF25 thunk."""

    if normalize_address(caller_start) != "0x48be10":
        return indexes
    if normalize_address(caller_end_exclusive) != "0x48be70":
        raise ValueError("reviewed DPLAYX ordinal caller extent drifted")
    addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(caller_start),
    )
    matches: list[Instruction] = []
    call_indices: list[int] = []
    for index, (address, instruction) in enumerate(
        zip(addresses, retail_instructions)
    ):
        if address is not None and normalize_address(address) == "0x48be33":
            matches.append(instruction)
            call_indices.append(index)
    instruction_ends = [
        address_value(normalize_address(address)) + len(instruction.bytes)
        for address, instruction in zip(addresses, retail_instructions)
        if address is not None and instruction.bytes
    ]
    if not instruction_ends or max(instruction_ends) != address_value("0x48be6a"):
        raise ValueError(
            "reviewed DPLAYX ordinal effective body extent drifted inside "
            "its selected call-contract extent"
        )
    if len(matches) != 1:
        raise ValueError("reviewed DPLAYX ordinal call site is missing or duplicated")
    try:
        call_body = bytes(int(item, 16) for item in matches[0].bytes)
    except (TypeError, ValueError):
        call_body = b""
    if (
        call_body != bytes.fromhex("e8 b2 a5 03 00")
        or _cc_cfg._instruction_mnemonic(matches[0]) != "call"
        or address_value("0x48be33") + 5
        + struct.unpack_from("<i", call_body, 1)[0]
        != address_value("0x4c63ea")
    ):
        raise ValueError("reviewed DPLAYX ordinal direct thunk transfer drifted")
    if (
        len(call_indices) != 1
        or _cc_cfg._cleanup_after(retail_instructions, call_indices[0]) is not None
    ):
        raise ValueError("reviewed DPLAYX ordinal call gained caller cleanup")
    expected_pushes = {
        "0x48be14": bytes.fromhex("6a 00"),
        "0x48be16": bytes.fromhex("6a 00"),
        "0x48be1c": bytes.fromhex("6a 00"),
        "0x48be1e": bytes.fromhex("50"),
        "0x48be21": bytes.fromhex("6a 00"),
    }
    argument_rows: list[tuple[str, bytes]] = []
    for address, instruction in zip(addresses, retail_instructions):
        if (
            address is None
            or not address_value("0x48be14")
            <= address
            < address_value("0x48be33")
            or _cc_cfg._instruction_mnemonic(instruction) != "push"
        ):
            continue
        try:
            body = bytes(int(item, 16) for item in instruction.bytes)
        except (TypeError, ValueError):
            body = b""
        argument_rows.append((normalize_address(address), body))
    if argument_rows != list(expected_pushes.items()):
        raise ValueError("reviewed DPLAYX ordinal five-argument route drifted")
    thunk_extent = _cc_cfg._hexdump_bytes(bridge.hexdump("0x4c63ea", 6))
    if thunk_extent != bytes.fromhex("ff 25 54 c0 4c 00"):
        raise ValueError("reviewed DPLAYX ordinal FF25 thunk body drifted")
    if (
        _immutable_retail_interval(
            _cc_catalog.DEFAULT_REFERENCE,
            start=address_value("0x48be6a"),
            end_exclusive=address_value("0x48be70"),
        )
        != b"\x90" * 6
    ):
        raise ValueError(
            "reviewed DPLAYX ordinal caller padding drifted"
        )
    import_rows = [
        row
        for row in (
            retail_import_targets
            if retail_import_targets is not None
            else _retail_import_targets(_cc_catalog.DEFAULT_REFERENCE)[0]
        )
        if getattr(row, "address", "") == "0x4cc054"
    ]
    if (
        len(import_rows) != 1
        or getattr(import_rows[0], "dll", "") != "DPLAYX.dll"
        or getattr(import_rows[0], "import_name", "") != "#4"
        or getattr(import_rows[0], "import_ordinal", None) != 4
        or getattr(import_rows[0], "iat_end_rva", "") != "0xcc058"
    ):
        raise ValueError("reviewed DPLAYX ordinal immutable import tuple drifted")
    _cc_recoil_hud_layout._require_exact_retail_storage_layout(
        address="0x4cc054",
        size=4,
        section_name=".rdata",
    )
    if _cc_cfg._hexdump_bytes(bridge.hexdump("0x4cc054", 4)) != bytes.fromhex(
        "0c 8c 7c 00"
    ):
        raise ValueError("reviewed DPLAYX ordinal immutable IAT cell drifted")
    identity = "iat:ordinal:10:DPLAYX.dll:4"
    indexed_identity = indexes.storage_by_address.get("0x4cc054", "")
    if indexed_identity and indexed_identity != identity:
        raise ValueError("reviewed DPLAYX ordinal IAT identity conflicts")
    aliases = [
        address
        for address, current in indexes.storage_by_address.items()
        if current == identity
    ]
    if aliases not in ([], ["0x4cc054"]):
        raise ValueError("reviewed DPLAYX ordinal IAT identity collides")
    provider_identity = indexes.by_address.get("0x4c63ea", "")
    reviewed = dict(indexes.reviewed_direct_import_thunk_by_call_site)
    same_thunk_packages = [
        thunk
        for thunk in ordinal_provider_thunks
        if normalize_address(thunk.thunk_address) == "0x4c63ea"
    ]
    if len(same_thunk_packages) > 1:
        raise ValueError(
            "reviewed DPLAYX ordinal provider proof is duplicated or ambiguous"
        )
    if same_thunk_packages:
        package = same_thunk_packages[0]
        if (
            package.iat_identity != identity
            or package.import_dll.casefold() != "dplayx.dll"
            or package.import_ordinal != 4
            or package.provider_identity != provider_identity
        ):
            raise ValueError(
                "reviewed DPLAYX ordinal provider proof conflicts with its "
                "governed physical identity"
            )
    if (
        not provider_identity
        or provider_identity not in indexes.provider_ids
    ):
        raise ValueError(
            "reviewed DPLAYX ordinal provider proof conflicts with its "
            "governed physical identity"
        )
    if "0x48be33" not in reviewed:
        reviewed["0x48be33"] = ("0x4c63ea", provider_identity)
        indexes = replace(
            indexes,
            reviewed_direct_import_thunk_by_call_site=reviewed,
        )
    value = ("0x4c63ea", provider_identity)
    if (
        not provider_identity.startswith("provider:")
        or provider_identity not in indexes.provider_ids
        or indexes.reviewed_direct_import_thunk_by_call_site.get("0x48be33")
        != value
    ):
        raise ValueError(
            "reviewed DPLAYX ordinal call-site lacks its canonical provider identity"
        )
    if indexed_identity == identity:
        return indexes
    storage_by_address = dict(indexes.storage_by_address)
    storage_by_address["0x4cc054"] = identity
    return replace(indexes, storage_by_address=storage_by_address)


def _reviewed_r3994_dplay_ordinal_retail_bridge_names(
    retail_instructions: Sequence[Instruction],
    *,
    caller_start: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Sequence[Any]],
) -> dict[str, list[Any]]:
    """Expose only the finite DPLAY ordinal selector's exact friendly name."""

    result = {name: list(rows) for name, rows in bridge_names.items()}
    if normalize_address(caller_start) != "0x48be10":
        return result
    call_site = "0x48be33"
    thunk_address = "0x4c63ea"
    friendly_name = "DPLAYX_Ordinal4_ImportThunk"
    provider_identity = indexes.by_address.get(thunk_address, "")
    if (
        not provider_identity.startswith("provider:")
        or provider_identity not in indexes.provider_ids
        or indexes.reviewed_direct_import_thunk_by_call_site.get(call_site)
        != (thunk_address, provider_identity)
    ):
        raise ValueError(
            "reviewed DPLAYX ordinal friendly selector lacks its exact "
            "caller-local provider proof"
        )
    addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=address_value(caller_start),
    )
    matching_calls = [
        instruction
        for address, instruction in zip(addresses, retail_instructions)
        if address is not None and normalize_address(address) == call_site
    ]
    if len(matching_calls) != 1:
        raise ValueError(
            "reviewed DPLAYX ordinal friendly selector call population drifted"
        )
    operand = re.sub(
        r"^(?:near|far)\s+(?:ptr\s+)?",
        "",
        _cc_cfg._instruction_operand(matching_calls[0]).strip(),
        flags=re.IGNORECASE,
    )
    if operand != friendly_name:
        # A numeric rendering already selects the reviewed address directly.
        # Another friendly spelling remains unresolved; it is not an operand
        # fallback for this exact exception.
        if _cc_catalog.ADDRESS_RE.search(operand):
            return result
        raise ValueError(
            "reviewed DPLAYX ordinal friendly selector spelling drifted"
        )
    colliding_names = [
        name
        for name in result
        if name.casefold() == friendly_name.casefold()
        and name != friendly_name
    ]
    prior = result.get(friendly_name)
    prior_addresses: list[str] = []
    for row in prior or ():
        try:
            prior_addresses.append(
                normalize_address(str(getattr(row, "address", "")))
            )
        except ProgressError:
            prior_addresses.append("")
    if colliding_names or (
        prior is not None and prior_addresses != [thunk_address]
    ):
        raise ValueError(
            "reviewed DPLAYX ordinal friendly selector collides with a "
            "different or ambiguous bridge population"
        )
    if prior is None:
        result[friendly_name] = [
            SimpleNamespace(
                address=thunk_address,
                name=friendly_name,
                raw_name=friendly_name,
                full_name=friendly_name,
                kind="function",
            )
        ]
    return result


def _r4564_retail_provenance_adapters(
    retail_instructions: Sequence[Instruction],
    *,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge: BinaryNinjaBridge | None = None,
    retail_import_targets: Sequence[Any] | None = None,
) -> tuple[
    dict[str, ReviewedRegisterCallStorageBridge],
    dict[str, ReviewedLoopVptrStorageBridge],
    dict[str, ReviewedExactIndirectStorageBridge],
]:
    """Publish only the finite r4564 retail call-site facts.

    Every row is guarded by exact caller extent and raw invocation bytes.  The
    callback and COM targets remain deliberately blank: only their finite
    storage/slot lineage is reviewed here.
    """
    from _recoil.call_contract.records import (
        ReviewedExactIndirectStorageBridge,
        ReviewedLoopVptrStorageBridge,
        ReviewedRegisterCallStorageBridge,
    )

    start = normalize_address(caller_start)
    end = normalize_address(caller_end_exclusive)
    by_address = {
        normalize_address(address): row
        for row in retail_instructions
        if (address := _cc_cfg._source_instruction_address(row)) is not None
    }

    def require_rows(
        expected_end: str,
        rows: Mapping[str, bytes],
        *,
        label: str,
    ) -> None:
        if end != expected_end:
            raise ValueError(f"{label} exact caller extent drifted")
        for address, expected_body in rows.items():
            row = by_address.get(address)
            try:
                body = bytes(int(item, 16) for item in row.bytes) if row else b""
            except (TypeError, ValueError):
                body = b""
            if body != expected_body:
                raise ValueError(f"{label} exact instruction drifted at {address}")

    register_calls: dict[str, ReviewedRegisterCallStorageBridge] = {}
    vptr_calls: dict[str, ReviewedLoopVptrStorageBridge] = {}
    exact_indirect: dict[str, ReviewedExactIndirectStorageBridge] = {}
    if start == "0x46de50":
        require_rows(
            "0x46df50",
            {
                "0x46de99": bytes.fromhex("a1 88 d7 53 00"),
                "0x46de9e": bytes.fromhex("85 c0"),
                "0x46dea0": bytes.fromhex("74 06"),
                "0x46dea2": bytes.fromhex("8b ce"),
                "0x46dea4": bytes.fromhex("ff d0"),
                "0x46dea6": bytes.fromhex("89 07"),
            },
            label="zImage optional fallback callback",
        )
        callback_address = "0x53d788"
        callback_identity = indexes.storage_by_address.get(
            callback_address, ""
        )
        value = address_value(callback_address)
        containers = [
            container
            for container in indexes.storage_containers
            if container.start == value
            and container.end_exclusive == value + 4
            and container.identity == callback_identity
        ]
        get_json = getattr(bridge, "get_json", None)
        if (
            not callback_identity
            or callback_identity.startswith("iat:")
            or len(containers) != 1
            or bridge is None
            or not callable(get_json)
            or _cc_cfg._hexdump_bytes(bridge.hexdump(callback_address, 4))
            != b"\x00\x00\x00\x00"
        ):
            raise ValueError(
                "zImage optional fallback callback storage/zero-cell proof drifted"
            )
        payload = get_json(
            "getXrefsTo", address=callback_address, limit=100000
        )
        code_rows, data_rows, complete = _bn_inbound_xref_items(payload)
        source_addresses = tuple(
            _bn_xref_address(
                row,
                "source_address",
                "source_addr",
                "from_address",
                "address",
            )
            for row in code_rows
        )
        if (
            not complete
            or data_rows
            or source_addresses != ("0x46de99",)
        ):
            raise ValueError(
                "zImage optional fallback callback complete xref population drifted"
            )
        register_calls["0x46dea4"] = ReviewedRegisterCallStorageBridge(
            register="eax",
            storage_identity=callback_identity,
            identity_kind="callback",
        )
        indirect_sites = {
            normalize_address(address)
            for address, row in by_address.items()
            if _cc_cfg._instruction_mnemonic(row) in {"call", "jmp"}
            and row.bytes
            and row.bytes[0].lower() == "ff"
        }
        if indirect_sites != {"0x46dea4", "0x46dedc", "0x46df09"}:
            raise ValueError(
                "zImage optional fallback callback indirect population drifted"
            )
    elif start == "0x42db50":
        require_rows(
            "0x42dc30",
            {
                "0x42dc0e": bytes.fromhex("8b 0e"),
                "0x42dc10": bytes.fromhex("ff 51 04"),
            },
            label="zCom interface-map AddRef",
        )
        vptr_calls["0x42dc10"] = ReviewedLoopVptrStorageBridge(
            register="ecx",
            storage_identity=_cc_catalog._ZCOM_INTERFACE_MAP_ADDREF_STORAGE,
            slot_displacement=4,
            assembly_source="bn",
        )
    elif start == "0x4815c0":
        require_rows(
            "0x481aa0",
            {
                "0x481a0c": bytes.fromhex("8b 35 94 c5 4c 00"),
                "0x481a54": bytes.fromhex("ff d6"),
                "0x481a56": bytes.fromhex("83 c4 10"),
            },
            label="zModel fwrite unanimous reload",
        )
        supplied_imports = tuple(
            retail_import_targets
            if retail_import_targets is not None
            else _retail_import_targets(_cc_catalog.DEFAULT_REFERENCE)[0]
        )
        matches = [
            row
            for row in supplied_imports
            if getattr(row, "address", "") == "0x4cc594"
            and getattr(row, "dll", "").casefold() == "msvcrt.dll"
            and getattr(row, "import_name", "") == "fwrite"
            and getattr(row, "import_ordinal", None) is None
        ]
        if len(matches) != 1:
            raise ValueError("zModel fwrite IAT identity drifted")
        register_calls["0x481a54"] = ReviewedRegisterCallStorageBridge(
            register="esi",
            storage_identity="iat:fwrite",
            identity_kind="iat",
        )
    elif start == "0x48a520":
        direct_sites = ("0x48a52d", "0x48a886")
        message_box_sites = (
            "0x48a595", "0x48a5b3", "0x48a5e5", "0x48a602",
            "0x48a634", "0x48a652", "0x48a683", "0x48a6a1",
            "0x48a6d3", "0x48a6f0", "0x48a722", "0x48a740",
            "0x48a771", "0x48a78f", "0x48a7c9", "0x48a7e7",
            "0x48a805", "0x48a822", "0x48a840", "0x48a85e",
            "0x48a87b",
        )
        ordered_call_sites = (
            "0x48a52d", "0x48a564",
            *message_box_sites,
            "0x48a886", "0x48a897",
        )
        require_rows(
            "0x48a909",
            {
                "0x48a564": bytes.fromhex("ff 51 60"),
                "0x48a897": bytes.fromhex("ff 51 10"),
                **{
                    site: bytes.fromhex("ff 15 64 c6 4c 00")
                    for site in message_box_sites
                },
            },
            label="zNetwork OpenSelectedSession dispatch",
        )
        encoded_by_address: dict[str, bytes] = {}
        ordered_calls: list[str] = []
        indirect_jumps: list[str] = []
        for row in retail_instructions:
            address = _cc_cfg._source_instruction_address(row)
            if address is None:
                continue
            normalized_address = normalize_address(address)
            try:
                encoded = bytes(int(item, 16) for item in row.bytes)
            except (TypeError, ValueError) as exc:
                raise ValueError(
                    "zNetwork OpenSelectedSession requires complete retail "
                    f"instruction bytes at {normalized_address}"
                ) from exc
            mnemonic = _cc_cfg._instruction_mnemonic(row)
            if mnemonic == "call":
                ordered_calls.append(normalized_address)
                if normalized_address in encoded_by_address:
                    raise ValueError(
                        "zNetwork OpenSelectedSession call coordinates collide"
                    )
                encoded_by_address[normalized_address] = encoded
            elif mnemonic == "jmp" and encoded[:1] == b"\xff":
                indirect_jumps.append(normalized_address)
        if tuple(ordered_calls) != ordered_call_sites:
            raise ValueError(
                "zNetwork OpenSelectedSession ordered 25-call population "
                f"drifted: expected={ordered_call_sites!r}, "
                f"actual={tuple(ordered_calls)!r}"
            )
        if any(
            len(encoded_by_address.get(site, b"")) != 5
            or encoded_by_address[site][:1] != b"\xe8"
            for site in direct_sites
        ):
            raise ValueError(
                "zNetwork OpenSelectedSession direct E8 population drifted"
            )
        if any(
            encoded_by_address.get(site)
            != bytes.fromhex("ff 15 64 c6 4c 00")
            for site in message_box_sites
        ):
            raise ValueError(
                "zNetwork OpenSelectedSession MessageBoxA FF15 population "
                "drifted"
            )
        jump_table = by_address.get("0x48a7b0")
        try:
            jump_table_body = bytes(
                int(item, 16) for item in jump_table.bytes
            )
        except (AttributeError, TypeError, ValueError) as exc:
            raise ValueError(
                "zNetwork OpenSelectedSession requires the retail FF24 "
                "jump-table instruction"
            ) from exc
        if (
            _cc_cfg._instruction_mnemonic(jump_table) != "jmp"
            or jump_table_body[:2] != b"\xff\x24"
            or tuple(indirect_jumps) != ("0x48a7b0",)
        ):
            raise ValueError(
                "zNetwork OpenSelectedSession FF24 jump-table "
                "classification drifted"
            )
        supplied_imports = tuple(
            retail_import_targets
            if retail_import_targets is not None
            else _retail_import_targets(_cc_catalog.DEFAULT_REFERENCE)[0]
        )
        matches = [
            row
            for row in supplied_imports
            if getattr(row, "address", "") == "0x4cc664"
            and getattr(row, "dll", "").casefold() == "user32.dll"
            and getattr(row, "import_name", "") == "MessageBoxA"
            and getattr(row, "import_ordinal", None) is None
        ]
        if len(matches) != 1:
            raise ValueError("zNetwork MessageBoxA IAT identity drifted")
        # Retain the proof population needed by IAT preemption, but establish
        # each vptr's actual global storage through all current CFG arrivals.
        from _recoil.call_contract.receiver_retail import _exact_retail_cfg_register_provenance
        if bridge is None:
            raise ValueError("DirectPlay vptr proof requires current retail switch targets")
        switch_targets = _cc_cfg.retail_local_switch_targets(
            retail_instructions, caller_start=start, caller_end_exclusive=end, bridge=bridge,
            include_backward_targets=True)
        addresses = _cc_cfg._instruction_runtime_addresses(
            retail_instructions, source="bn", caller_start=address_value(start))
        storage = indexes.storage_by_address.get("0x56aaf0", "")
        if not storage.startswith("storage:"):
            raise ValueError("DirectPlay vptr proof lacks its typed global pointer")
        for site, slot in (("0x48a564", 0x60), ("0x48a897", 0x10)):
            positions = [i for i, address in enumerate(addresses) if address == address_value(site)]
            if len(positions) != 1:
                raise ValueError("DirectPlay vptr proof requires a unique invocation coordinate")
            lineage = _exact_retail_cfg_register_provenance(
                retail_instructions, before_index=positions[0], register="ecx", addresses=addresses,
                indexes=indexes, caller_start=address_value(start), caller_end=address_value(end),
                local_control_flow_indices=frozenset(switch_targets), local_control_flow_targets=switch_targets)
            if lineage != f"load({storage})":
                raise ValueError("DirectPlay vptr proof lost its exact global-pointer lineage")
            vptr_calls[site] = ReviewedLoopVptrStorageBridge(
                register="ecx", storage_identity=lineage, slot_displacement=slot, assembly_source="bn")
    render_specs = {
        "0x493df0": (
            "0x494af0",
            (("0x494a99", "esp", 0x74, "6320c8-6320d4"),),
        ),
        "0x494af0": (
            "0x495850",
            (("0x4957fa", "esp", 0x60, "6320e0-6320ec"),),
        ),
        "0x495850": (
            "0x4969d0",
            tuple(
                (site, "ebp", 0x1c, "6320b8-6320bc-or-49f180")
                for site in (
                    "0x49673c", "0x49679f", "0x496918", "0x49697e"
                )
            ),
        ),
        "0x4969d0": (
            "0x497ac0",
            tuple(
                (site, "esp", 0x70, "6320b8-6320bc")
                for site in ("0x497916", "0x497a64")
            ),
        ),
        "0x497ac0": (
            "0x498bd0",
            tuple(
                (site, "esp", 0x88, "6320d8-6320dc")
                for site in ("0x498a32", "0x498b5c")
            ),
        ),
    }
    render_spec = render_specs.get(start)
    if render_spec is not None:
        expected_end, calls = render_spec
        call_rows = {}
        for site, register, displacement, _selection in calls:
            if register == "ebp":
                call_rows[site] = bytes.fromhex("ff 55 1c")
            elif displacement < 0x80:
                call_rows[site] = bytes((0xFF, 0x54, 0x24, displacement))
            else:
                call_rows[site] = bytes((
                    0xFF, 0x94, 0x24,
                    displacement & 0xFF, 0, 0, 0,
                ))
        require_rows(
            expected_end,
            call_rows,
            label="zRender RndrSpanLenShiftFn callback",
        )
        for site, register, displacement, selection in calls:
            exact_indirect[site] = ReviewedExactIndirectStorageBridge(
                register=register,
                storage_identity=(
                    "dynamic:RndrSpanLenShiftFn-selection:" + selection
                ),
                slot_displacement=displacement,
                assembly_source="bn",
                identity_kind="callback",
            )
    return register_calls, vptr_calls, exact_indirect


def _exact_vc5_symbolic_stack_indirect_invocation(
    instruction: Instruction,
) -> tuple[str, int, str, int] | None:
    """Decode one exact VC5 ``CALL/JMP local$[ESP+disp]`` invocation."""

    match = re.fullmatch(
        r"(?P<form>call|jmp)\s+"
        r"(?P<source>(?:dword\s+(?:ptr\s+)?)?"
        r"[A-Za-z_?$][A-Za-z0-9_@$?]*\[[^\]]+\])",
        instruction.raw_text.strip(),
        flags=re.IGNORECASE,
    )
    if match is None:
        return None
    symbolic = _cc_receiver_instructions._vc5_symbolic_stack_local(
        match.group("source"),
        index_register=None,
    )
    if symbolic is None:
        return None
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    if len(body) < 3 or body[0] != 0xFF:
        return None
    form = match.group("form").lower()
    modrm = body[1]
    mode = modrm >> 6
    expected_operation = 2 if form == "call" else 4
    if (
        (modrm >> 3) & 0x07 != expected_operation
        or modrm & 0x07 != 0x04
        or body[2] != 0x24
    ):
        return None
    if mode == 0 and len(body) == 3:
        displacement = 0
    elif mode == 1 and len(body) == 4:
        displacement = struct.unpack("<b", body[3:4])[0]
    elif mode == 2 and len(body) == 7:
        displacement = struct.unpack("<i", body[3:7])[0]
    else:
        return None
    symbol, rendered_displacement = symbolic
    return form, displacement, symbol, rendered_displacement


def _exact_vc5_symbolic_stack_function_store(
    instruction: Instruction,
) -> tuple[int, str, int, str, int] | None:
    """Decode ``MOV local$[ESP+n], OFFSET FLAT:function`` exactly."""

    match = re.fullmatch(
        r"mov\s+(?P<destination>(?:dword\s+(?:ptr\s+)?)?"
        r"[A-Za-z_?$][A-Za-z0-9_@$?]*\[[^\]]+\])\s*,\s*"
        r"(?:offset\s+(?:flat:)?\s*)?"
        r"(?P<function>[?$@A-Za-z_][?$@A-Za-z0-9_]*)",
        instruction.raw_text.strip(),
        flags=re.IGNORECASE,
    )
    if match is None:
        return None
    symbolic = _cc_receiver_instructions._vc5_symbolic_stack_local(
        match.group("destination"),
        index_register=None,
    )
    try:
        body = bytes(int(item, 16) for item in instruction.bytes)
    except (TypeError, ValueError):
        return None
    if symbolic is None or len(body) < 7 or body[0] != 0xC7:
        return None
    modrm = body[1]
    mode = modrm >> 6
    if (modrm >> 3) & 0x07 != 0 or modrm & 0x07 != 0x04 or body[2] != 0x24:
        return None
    if mode == 0 and len(body) == 7:
        displacement = 0
        immediate_delta = 3
    elif mode == 1 and len(body) == 8:
        displacement = struct.unpack("<b", body[3:4])[0]
        immediate_delta = 4
    elif mode == 2 and len(body) == 11:
        displacement = struct.unpack("<i", body[3:7])[0]
        immediate_delta = 7
    else:
        return None
    if body[immediate_delta : immediate_delta + 4] != b"\x00\x00\x00\x00":
        return None
    symbol, rendered_displacement = symbolic
    return (
        displacement,
        symbol,
        rendered_displacement,
        match.group("function"),
        immediate_delta,
    )


def _compose_reviewed_retail_provenance_adapters(
    retail_instructions: Sequence[Instruction],
    *,
    document: ProgressDocument,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge: BinaryNinjaBridge,
    ordinal_provider_thunks: Sequence[ProviderOrdinalImportThunk] = (),
    retail_import_targets: Sequence[Any] | None = None,
) -> ReviewedRetailProvenanceAdapters:
    """Compose the finite WSI-013/015 retail facts through one live route.

    Target, slice, and phase-wide verification all call this same function.
    It publishes only evidence independently proven from retail and retains
    targetless callback/object dispatch where no immutable target exists.
    """
    from _recoil.call_contract.records import ReviewedRetailProvenanceAdapters

    normalized_start = normalize_address(caller_start)
    _cc_callable_identity._reviewed_r3994_static_table_inbound_target_bridges(
        retail_instructions,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge=bridge,
    )
    composed_indexes = _reviewed_r3994_dplay_ordinal_thunk_indexes(
        retail_instructions,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge=bridge,
        ordinal_provider_thunks=ordinal_provider_thunks,
        retail_import_targets=retail_import_targets,
    )
    register_call_bridges = _cc_reviewed_dispatch._reviewed_r3994_retail_register_storage_bridges(
        retail_instructions,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=composed_indexes,
        retail_import_targets=retail_import_targets,
    )
    zcom_resolver_register_calls = (
        _cc_recoil_callbacks._zcom_interface_map_resolver_retail_register_bridge(
            retail_instructions,
            caller_start=normalized_start,
            caller_end_exclusive=caller_end_exclusive,
            indexes=composed_indexes,
        )
    )
    overlap = register_call_bridges.keys() & zcom_resolver_register_calls.keys()
    if overlap:
        raise ValueError(
            "zCom interface-map resolver register bridge conflicts with "
            "another finite retail adapter"
        )
    _cc_proofs.merge_into(register_call_bridges, zcom_resolver_register_calls, family="identity.register_call_bridges")
    (
        r4564_register_calls,
        r4564_vptr_calls,
        r4564_exact_indirect_calls,
    ) = _r4564_retail_provenance_adapters(
        retail_instructions,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=composed_indexes,
        bridge=bridge,
        retail_import_targets=retail_import_targets,
    )
    overlap = register_call_bridges.keys() & r4564_register_calls.keys()
    if overlap:
        raise ValueError(
            "r4564 register-call adapter conflicts with another finite "
            "retail adapter"
        )
    _cc_proofs.merge_into(register_call_bridges, r4564_register_calls, family="identity.register_call_bridges")
    (
        zinput_register_calls,
        zinput_vptr_calls,
    ) = _cc_recoil_input._zinput_runtime_dispatch_retail_bridges(
        retail_instructions,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=composed_indexes,
    )
    zinput_non_callback_loads = (
        _cc_recoil_input._zinput_runtime_dispatch_non_callback_register_loads(
            retail_instructions,
            document=document,
            caller_start=normalized_start,
            caller_end_exclusive=caller_end_exclusive,
            indexes=composed_indexes,
            bridge=bridge,
            retail_import_targets=retail_import_targets,
        )
    )
    reviewed_non_callback_loads = dict(zinput_non_callback_loads)
    if normalized_start == "0x46de50":
        callback_identity = composed_indexes.storage_by_address.get(
            "0x53d788", ""
        )
        if not callback_identity:
            raise ValueError(
                "zImage reviewed callback preemption lacks exact storage identity"
            )
        reviewed_non_callback_loads["0x46de99"] = callback_identity
    overlap = register_call_bridges.keys() & zinput_register_calls.keys()
    if overlap:
        raise ValueError(
            "zInput runtime-dispatch register bridge conflicts with another "
            "finite retail adapter"
        )
    _cc_proofs.merge_into(register_call_bridges, zinput_register_calls, family="identity.register_call_bridges")
    dynamic_vptr = _cc_reviewed_dispatch._reviewed_r3994_dynamic_vptr_dispatch_bridges(
        retail_instructions,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=composed_indexes,
        bridge=bridge,
    )
    ctx_callbacks = _cc_recoil_callbacks._reviewed_r4008_zinterp_ctx_callback_bridges(
        retail_instructions,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=composed_indexes,
    )
    # Constructor table writes alone do not prove a runtime receiver's table.
    # Keep only independently proved dynamic lineage; unresolved calls fail
    # in ordinary extraction instead of inheriting an unrelated static target.
    vptr_bridges = dict(dynamic_vptr)
    overlap = vptr_bridges.keys() & r4564_vptr_calls.keys()
    if overlap:
        raise ValueError(
            "r4564 vptr adapter conflicts with another finite retail adapter"
        )
    _cc_proofs.merge_into(vptr_bridges, r4564_vptr_calls, family="identity.vptr_bridges")
    overlap = vptr_bridges.keys() & zinput_vptr_calls.keys()
    if overlap:
        raise ValueError(
            "zInput runtime-dispatch virtual-slot bridge conflicts with "
            "another finite retail adapter"
        )
    _cc_proofs.merge_into(vptr_bridges, zinput_vptr_calls, family="identity.vptr_bridges")
    zwep_damage_feedback_vptr = (
        _cc_recoil_weapons._zwep_damage_feedback_retail_handler_vptr_bridge(
            retail_instructions,
            caller_start=normalized_start,
            caller_end_exclusive=caller_end_exclusive,
            indexes=composed_indexes,
        )
    )
    overlap = vptr_bridges.keys() & zwep_damage_feedback_vptr.keys()
    if overlap:
        raise ValueError(
            "zWep damage-feedback handler bridge conflicts with another finite retail adapter"
        )
    _cc_proofs.merge_into(vptr_bridges, zwep_damage_feedback_vptr, family="identity.vptr_bridges")
    zwep_damage_timer_vptr = _cc_recoil_weapons._zwep_damage_timer_retail_handler_vptr_bridge(
        retail_instructions,
        caller_start=normalized_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=composed_indexes,
    )
    overlap = vptr_bridges.keys() & zwep_damage_timer_vptr.keys()
    if overlap:
        raise ValueError(
            "zWep damage-timer handler bridge conflicts with another finite retail adapter"
        )
    _cc_proofs.merge_into(vptr_bridges, zwep_damage_timer_vptr, family="identity.vptr_bridges")
    overlap = vptr_bridges.keys() & ctx_callbacks.keys()
    if overlap:
        raise ValueError(
            "reviewed zInterp ctx callback conflicts with another finite "
            "retail provenance adapter"
        )
    _cc_proofs.merge_into(vptr_bridges, ctx_callbacks, family="identity.vptr_bridges")
    reviewed_call_sites = tuple(
        sorted(
            {
                *vptr_bridges,
                *register_call_bridges,
                *r4564_exact_indirect_calls,
                *(
                    ("0x48be33",)
                    if normalized_start == "0x48be10"
                    and composed_indexes
                    .reviewed_direct_import_thunk_by_call_site.get("0x48be33")
                    else ()
                ),
                *(
                    ("0x403f00",)
                    if normalized_start == "0x4b9850"
                    else ()
                ),
            },
            key=address_value,
        )
    )
    return ReviewedRetailProvenanceAdapters(
        indexes=composed_indexes,
        register_call_storage_bridges=register_call_bridges,
        vptr_storage_bridges=vptr_bridges,
        reviewed_call_sites=reviewed_call_sites,
        exact_indirect_storage_bridges=r4564_exact_indirect_calls,
        non_callback_register_loads=reviewed_non_callback_loads,
        preempt_generic_register_iat=normalized_start
        in {"0x46de50", "0x48a520"},
    )


def _has_exact_bn_semantic_function(
    bridge_names: Mapping[str, Any],
    *,
    semantic_name: str,
    address: str,
) -> bool:
    """Require one current BN function carrying one synchronized semantic name."""
    raw_rows = bridge_names.get(semantic_name)
    if raw_rows is None:
        return False
    rows = (
        tuple(raw_rows)
        if isinstance(raw_rows, (list, tuple))
        else (raw_rows,)
    )
    if len(rows) != 1:
        return False
    row = rows[0]
    raw_address = getattr(row, "address", "")
    try:
        row_address = normalize_address(str(raw_address))
    except ProgressError:
        return False
    names = {
        str(value)
        for value in (
            getattr(row, "name", ""),
            getattr(row, "raw_name", ""),
            getattr(row, "full_name", ""),
        )
        if value
    }
    return (
        getattr(row, "kind", "") == "function"
        and row_address == address
        and semantic_name in names
    )


def _current_absolute_tu_local_matches_reviewed_symbol(
    candidate_name: str,
    reviewed_name: str,
) -> bool:
    candidate_match = _cc_catalog._VC5_TU_LOCAL_CALLABLE_RE.fullmatch(candidate_name)
    reviewed_match = _cc_catalog._VC5_TU_LOCAL_CALLABLE_RE.fullmatch(reviewed_name)
    if candidate_match is None or reviewed_match is None:
        return False
    candidate_source = candidate_match.group("source")
    reviewed_source = reviewed_match.group("source")
    if (
        re.fullmatch(r"[A-Za-z]:[\\/].+", candidate_source) is None
        or PureWindowsPath(reviewed_source).name != reviewed_source
        or PureWindowsPath(candidate_source).name != reviewed_source
    ):
        return False
    return (
        candidate_match.group("head") == reviewed_match.group("head")
        and candidate_match.group("suffix") == reviewed_match.group("suffix")
    )


def _translated_dynamic_probe_setup(
    reviewed_calls: Sequence[int], observed_calls: Sequence[int | None],
    setup_rows: Sequence[tuple[int, bytes]],
) -> tuple[tuple[int, bytes], ...]:
    """Anchor the unchanged reviewed sequence to fresh candidate call sites.

    Absolute object offsets are observations, not retail semantic truth. Only
    one uniform translation is admitted; the caller still authenticates every
    setup byte, invocation, relocation, symbol, and complete definition.
    """
    if (not reviewed_calls or len(reviewed_calls) != len(observed_calls)
            or any(type(value) is not int or value < 0 for value in (*reviewed_calls, *observed_calls))
            or tuple(sorted(set(reviewed_calls))) != tuple(reviewed_calls)
            or tuple(sorted(set(observed_calls))) != tuple(observed_calls)):
        raise ValueError("dynamic stack-probe sites are missing, duplicated, or unordered")
    deltas = {observed - reviewed for reviewed, observed in zip(reviewed_calls, observed_calls)}
    if len(deltas) != 1:
        raise ValueError("dynamic stack-probe relative call spacing changed")
    delta = deltas.pop()
    translated = tuple((offset + delta, body) for offset, body in setup_rows)
    if not translated or any(offset < 0 or not body for offset, body in translated):
        raise ValueError("dynamic stack-probe setup lies outside its complete definition")
    return translated


def _raw_call_count_divergence(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    retail_instructions: Sequence[Instruction],
    *,
    proved_funclet_call_indices: tuple[int, ...] = (),
) -> dict[str, Any] | None:
    """Reject call expansion/folding before any caller-specific adapter.

    Count actual CALL instructions on both sides, not helper fan-out or
    hypothetical inlining. Tail transfers still require the ordinary exact
    extraction because local branches must be distinguished from tail calls.
    A separately verified COFF/xdata artifact partition may identify catch
    calls outside the parent domain. It must run before this check; the calls
    remain in the complete candidate inventory and undergo the later partition
    check again. Equality proves no identities, receiver storage, cleanup or bytes.
    """
    if (tuple(sorted(set(proved_funclet_call_indices))) != proved_funclet_call_indices
            or any(type(index) is not int or not 0 <= index < len(candidate.instructions)
                or _cc_cfg._instruction_mnemonic(candidate.instructions[index]) != "call"
                for index in proved_funclet_call_indices)):
        raise _cc_errors.CandidateCallContractEvidenceError("raw call census has an invalid funclet partition")
    retail_calls = sum(_cc_cfg._instruction_mnemonic(row) == "call" for row in retail_instructions)
    candidate_total_calls = sum(_cc_cfg._instruction_mnemonic(row) == "call" for row in candidate.instructions)
    candidate_calls = candidate_total_calls - len(proved_funclet_call_indices)
    expected_calls = sum(row.get("form") == "call" for row in expected)
    if retail_calls == expected_calls == candidate_calls:
        return None
    return {
        "kind": "verifier-blocked",
        "side": "expected" if expected_calls != retail_calls else "candidate",
        "message": "Actual CALL instruction count differs before complete invocation extraction",
        "reason": "raw-call-count-before-projection",
        "retail_call_count": retail_calls,
        "expected_call_count": expected_calls,
        "candidate_call_count": candidate_calls,
        "candidate_total_call_count": candidate_total_calls,
        "candidate_funclet_call_count": len(proved_funclet_call_indices),
    }


def _coff_alias_rows_for_comparison(specs: Sequence[CoffAliasSource]) -> list[dict[str, object]]:
    inventory = load_repository_path_inventory(REPO_ROOT)
    try:
        manifest = resolve_repository_file(
            _cc_source._call_contract_final_build_manifest_logical_path(),
            repository_root=inventory.repository_root,
            inventory=inventory,
            context="call-contract final-build manifest",
            allowed_suffixes={".json"},
        )
        raw = json.loads(manifest.physical_path.read_text(encoding="utf-8"))
    except (RepositoryPathError, OSError, json.JSONDecodeError) as exc:
        raise ValueError(
            f"call-contract COFF alias registry cannot be read: {exc}"
        ) from exc
    raw_rows = raw.get("coff_alias_sources") if isinstance(raw, Mapping) else None
    if not isinstance(raw_rows, list) or len(raw_rows) != len(specs):
        raise ValueError("call-contract COFF alias registry population mismatch")
    result: list[dict[str, object]] = []
    for index, (raw_row, spec) in enumerate(zip(raw_rows, specs)):
        if not isinstance(raw_row, Mapping):
            raise ValueError(
                f"call-contract COFF alias registry row {index} is malformed"
            )
        source_text = raw_row.get("source")
        link_after_text = raw_row.get("link_after_source")
        if not isinstance(source_text, str) or not isinstance(link_after_text, str):
            raise ValueError(
                f"call-contract COFF alias registry row {index} paths are malformed"
            )
        try:
            source = resolve_repository_file(
                source_text,
                repository_root=inventory.repository_root,
                inventory=inventory,
                context=f"call-contract COFF alias source[{index}]",
            )
            link_after = resolve_repository_file(
                link_after_text,
                repository_root=inventory.repository_root,
                inventory=inventory,
                context=f"call-contract COFF alias link-after source[{index}]",
                allowed_suffixes=_cc_catalog.CALL_CONTRACT_SOURCE_SUFFIXES,
            )
        except RepositoryPathError as exc:
            raise ValueError(str(exc)) from exc
        if (
            os.path.normcase(str(spec.source.resolve()))
            != os.path.normcase(str(source.physical_path))
            or os.path.normcase(str(spec.link_after_source.resolve()))
            != os.path.normcase(str(link_after.physical_path))
        ):
            raise ValueError(
                f"call-contract COFF alias registry row {index} physical path mismatch"
            )
        result.append(
            {
                "source": source.repository_path,
                "link_after_source": link_after.repository_path,
                "aliases": [
                    {"alias": item.alias, "target": item.target}
                    for item in spec.aliases
                ],
            }
        )
    return result


def _validated_final_build_coff_alias_bridges(
    candidates: Mapping[str, CandidateAssembly],
    *,
    indexes: IdentityIndexes,
    build_root: Path,
    active_targets: Iterable[Any] | None = None,
) -> dict[str, str]:
    config = load_final_build_config(DEFAULT_FINAL_BUILD_MANIFEST)
    specs = config.coff_alias_sources
    if not specs:
        return {}
    if active_targets is None:
        active_targets = {
            id(candidate.target): candidate.target
            for candidate in candidates.values()
            if candidate.target is not None
        }.values()
    if not any(
        _cc_callable_identity._target_authorizes_final_build_coff_aliases(target, specs=specs)
        for target in active_targets
    ):
        return {}

    alias_config = replace(
        config,
        build_dir=(build_root / "validated-coff-alias-authority").resolve(),
        build_dir_explicit=True,
    )
    paths = final_build_paths(alias_config)
    paths.obj_dir.mkdir(parents=True, exist_ok=True)
    paths.logs_dir.mkdir(parents=True, exist_ok=True)
    paths.rsp_dir.mkdir(parents=True, exist_ok=True)
    llvm_ml = resolve_llvm_ml()
    validation_by_source: dict[str, Mapping[str, object]] = {}
    for spec in specs:
        validate_alias_source(spec)
        command = make_coff_alias_command(paths, spec, llvm_ml=llvm_ml)
        result = run_final_build_command(command)
        if result.returncode != 0:
            raise ValueError(
                f"validated COFF alias assembly failed for {spec.source} "
                f"with exit code {result.returncode}"
            )
        report = validate_alias_object(alias_object_path(paths, spec.source), spec)
        validation_by_source[str(spec.source.resolve())] = report
    return resolve_validated_coff_alias_identity_bridges(
        specs,
        validation_by_source=validation_by_source,
        indexes=indexes,
    )
