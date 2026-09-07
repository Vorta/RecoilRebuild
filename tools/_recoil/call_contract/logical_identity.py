"""Recoil call-contract logical identity evidence and checks."""

from __future__ import annotations

from pathlib import Path
from typing import Any, Mapping

from _recoil.call_contract import identity as _cc_identity
from _recoil.lib.progress import (
    ProgressDocument,
    ProgressError,
    address_value,
    is_current_accepted_state,
    normalize_address,
)
from _recoil.lib.tooling import REPO_ROOT


def _winner_unknown_icf_group_indexes(
    document: ProgressDocument,
) -> tuple[dict[str, str], dict[str, str]]:
    """Build exact reviewed ICF-group authority before inspecting a candidate."""
    targets = document.collection("verification_targets")
    owners = document.collection("owners")
    evidence_rows = document.collection("evidence")
    groups_by_address: dict[str, str] = {}
    groups_by_logical_identity: dict[str, str] = {}

    for symbol_id, symbol in document.collection("symbols").items():
        if not isinstance(symbol, Mapping):
            continue
        group = symbol.get("icf_address_group")
        if not isinstance(group, Mapping) or group.get("winner_status") != "winner-unknown":
            continue
        if (
            symbol.get("binary") != "recoil"
            or symbol.get("kind") not in {"function", "compiler-function"}
            or symbol.get("pipeline_class") != "non-authored"
            or symbol.get("authored_order_role")
            != "compiler-generated-icf-representative"
            or symbol.get("linked_provider_binding") is not None
            or group.get("winner_identity_key") not in {None, ""}
        ):
            # Other inventoried winner-unknown groups remain on the existing
            # exact-alias/fail-closed path. They do not gain group authority.
            continue
        group_evidence = group.get("evidence_ids")
        opted_in = isinstance(group_evidence, list) and any(
            isinstance(evidence_id, str)
            and isinstance(evidence_rows.get(evidence_id), Mapping)
            and evidence_rows[evidence_id].get("kind")
            == "authored-order-icf-logical-alias-review"
            for evidence_id in group_evidence
        )
        if not opted_in:
            # Historical logical-alias inventories describe exact aliases only.
            # Group arbitration is a separate, explicit current review contract.
            continue
        if (
            not isinstance(group_evidence, list)
            or not group_evidence
            or any(not isinstance(item, str) or not item for item in group_evidence)
            or len(set(group_evidence)) != len(group_evidence)
        ):
            raise ValueError(
                f"winner-unknown ICF group {symbol_id} has missing or duplicate evidence"
            )
        raw_address = symbol.get("address", symbol.get("start"))
        if not isinstance(raw_address, str):
            raise ValueError(f"winner-unknown ICF group {symbol_id} has no address")
        address = normalize_address(raw_address)
        expected_alias_prefix = f"recoil:logical-function:{address}:"
        aliases = symbol.get("logical_aliases")
        if not isinstance(aliases, Mapping) or len(aliases) < 2:
            continue
        if any(
            not isinstance(alias_id, str)
            or not isinstance(alias, Mapping)
            or (
                alias.get("pipeline_class"),
                alias.get("authored_order_role"),
            )
            not in {
                ("authored", "authored-body"),
                ("authored-lifecycle", "authored-lifecycle-body"),
            }
            or alias.get("fold_status") != "proven-fold-alias"
            or alias.get("source_owner_status") != "authored-owner"
            or alias.get("original_name_status")
            not in {"recovered", "provisional"}
            for alias_id, alias in aliases.items()
        ):
            continue
        expected_evidence_scope = {
            symbol_id,
            *aliases.keys(),
            *(alias.get("owner_id") for alias in aliases.values()),
        }

        object_symbols: set[str] = set()
        recovered_original_names: set[str] = set()
        alias_ids: set[str] = set()
        for alias_id, alias in aliases.items():
            if (
                not isinstance(alias_id, str)
                or not alias_id.startswith(expected_alias_prefix)
            ):
                raise ValueError(
                    f"winner-unknown ICF group {symbol_id} has incomplete or "
                    "wrong-address logical alias membership"
                )
            object_symbol = alias.get("object_symbol")
            original_name = alias.get("original_name")
            original_name_status = alias.get("original_name_status")
            owner_id = alias.get("owner_id")
            alias_evidence = alias.get("evidence_ids")
            if (
                not isinstance(object_symbol, str)
                or not _cc_identity._decorated_coff_name(object_symbol)
                or not isinstance(original_name, str)
                or not original_name
                or (
                    original_name_status == "recovered"
                    and original_name.startswith("CompilerICF::")
                )
                or not isinstance(owner_id, str)
                or not owner_id
                or not isinstance(alias_evidence, list)
                or alias_evidence != group_evidence
                or len(set(alias_evidence)) != len(alias_evidence)
            ):
                raise ValueError(
                    f"winner-unknown ICF group {symbol_id} has incomplete alias "
                    f"identity/evidence for {alias_id}"
                )
            if object_symbol in object_symbols or (
                original_name_status == "recovered"
                and original_name in recovered_original_names
            ):
                raise ValueError(
                    f"winner-unknown ICF group {symbol_id} has duplicate alias names"
                )
            owner = owners.get(owner_id)
            gates = owner.get("gates") if isinstance(owner, Mapping) else None
            if (
                not isinstance(owner, Mapping)
                or owner.get("binary") != "recoil"
                or owner.get("kind") == "provider-boundary"
                or owner.get("provider_state")
                in {"accepted", "provider-boundary", "provider-owned"}
                or not isinstance(gates, Mapping)
                or gates.get("source") != "accepted"
                or gates.get("owner_linkage") != "accepted"
            ):
                raise ValueError(
                    f"winner-unknown ICF group {symbol_id} has missing, provider, "
                    f"or unaccepted owner {owner_id!r}"
                )
            for evidence_id in alias_evidence:
                evidence = evidence_rows.get(evidence_id)
                scopes = (
                    evidence.get("scope_ids")
                    if isinstance(evidence, Mapping)
                    else None
                )
                if (
                    not isinstance(scopes, list)
                    or len(set(scopes)) != len(scopes)
                    or symbol_id not in scopes
                    or alias_id not in scopes
                    or owner_id not in scopes
                ):
                    raise ValueError(
                        f"winner-unknown ICF group {symbol_id} has missing or "
                        f"conflicting evidence membership for {alias_id}"
                    )
                if isinstance(evidence, Mapping) and evidence.get("kind") == (
                    "authored-order-icf-logical-alias-review"
                ):
                    provenance = evidence.get("provenance")
                    validation_context = (
                        provenance.get("validation_context")
                        if isinstance(provenance, Mapping)
                        else None
                    )
                    if (
                        len(group_evidence) != 1
                        or not is_current_accepted_state(evidence)
                        or evidence.get("gating") is not True
                        or not isinstance(scopes, list)
                        or len(scopes) != len(expected_evidence_scope)
                        or set(scopes) != expected_evidence_scope
                        or not isinstance(provenance, Mapping)
                        or provenance.get("candidate_independent") is not True
                        or not isinstance(validation_context, Mapping)
                        or validation_context.get("candidate_output_used") is not False
                        or (
                            provenance.get("evidence_contract") is not None
                            and (
                                provenance.get("evidence_contract")
                                != "existing-winner-unknown-physical-group-refresh-v1"
                                or validation_context.get("authority_scope")
                                != "physical-icf-group-only"
                                or validation_context.get(
                                    "original_name_used_as_authority"
                                )
                                is not False
                                or validation_context.get("winner_identity_claimed")
                                is not False
                            )
                        )
                    ):
                        raise ValueError(
                            f"winner-unknown ICF group {symbol_id} has stale, "
                            "candidate-dependent, or inexact current alias-review evidence"
                        )
            alias_ids.add(alias_id)
            object_symbols.add(object_symbol)
            if original_name_status == "recovered":
                recovered_original_names.add(original_name)

        # A reviewed physical-group refresh names its authority target. Other
        # registered diagnostics/linked projections may describe the same aliases;
        # their presence is not a second authority and must not make it ambiguous.
        from _recoil.lib.authored_icf import reviewed_authority_targets
        physical_block = document.collection("physical_blocks").get(symbol.get("physical_block_id"), {})
        accepted_order = physical_block.get("accepted_order_facts", {})
        authority_target_ids = reviewed_authority_targets(
            [evidence_rows[eid].get("provenance") for eid in group_evidence],
            accepted_order.get("target_id"),
        )
        matching_targets: list[tuple[str, Mapping[str, Any], dict[str, Mapping[str, Any]]]] = []
        for target_id, target in targets.items():
            if (
                not isinstance(target, Mapping)
                or target.get("binary") != "recoil"
                or target.get("kind") != "vc5"
                or (authority_target_ids and target_id not in authority_target_ids)
            ):
                continue
            rows = {
                str(row.get("logical_identity_key")): row
                for row in _cc_identity._mapping_target_function_rows(target)
                if row.get("logical_identity_key") in alias_ids
            }
            if rows:
                matching_targets.append((str(target_id), target, rows))
        if len(matching_targets) != 1:
            raise ValueError(
                f"winner-unknown ICF group {symbol_id} has missing, duplicate, or "
                "conflicting governed target membership"
            )
        target_id, target, stored_rows = matching_targets[0]
        for evidence_id in group_evidence:
            evidence = evidence_rows.get(evidence_id)
            provenance = (
                evidence.get("provenance")
                if isinstance(evidence, Mapping)
                else None
            )
            if (
                isinstance(provenance, Mapping)
                and provenance.get("evidence_contract")
                == "existing-winner-unknown-physical-group-refresh-v1"
                and provenance.get("governed_target_id") != target_id
            ):
                raise ValueError(
                    f"winner-unknown ICF group {symbol_id} has conflicting governed "
                    "target evidence"
                )
        if set(stored_rows) != alias_ids:
            raise ValueError(
                f"winner-unknown ICF group {symbol_id} has incomplete governed "
                "target alias membership"
            )
        stored_all_at_address = [
            row
            for row in _cc_identity._mapping_target_function_rows(target)
            if isinstance(row.get("address"), str)
            and normalize_address(row["address"]) == address
            and isinstance(row.get("logical_identity_key"), str)
            and row.get("logical_identity_key")
        ]
        if (
            len(stored_all_at_address) != len(alias_ids)
            or {str(row["logical_identity_key"]) for row in stored_all_at_address}
            != alias_ids
        ):
            raise ValueError(
                f"winner-unknown ICF group {symbol_id} has extra or duplicate "
                "registered aliases"
            )
        for alias_id, row in stored_rows.items():
            alias = aliases[alias_id]
            alternatives = _cc_identity._finite_literal_symbol_regex_alternatives(
                row.get("symbol_regex")
            )
            registered_names = (
                {str(row.get("symbol"))}
                if isinstance(row.get("symbol"), str) and row.get("symbol")
                else set(alternatives or ())
            )
            if (
                normalize_address(row.get("address")) != address
                or (
                    row.get("pipeline_class"),
                    row.get("authored_order_role"),
                )
                != (
                    alias.get("pipeline_class"),
                    alias.get("authored_order_role"),
                )
                or row.get("authored_order_gate") is not True
                or row.get("required_presence") is not True
                or row.get("icf_fold_status") != "proven-fold-alias"
                or alias.get("object_symbol") not in registered_names
            ):
                raise ValueError(
                    f"winner-unknown ICF group {symbol_id} has stale or conflicting "
                    f"registered alias {alias_id}"
                )

        registration = target.get("registration")
        manifest_value = (
            registration.get("manifest_path")
            if isinstance(registration, Mapping)
            else None
        )
        if not isinstance(manifest_value, str) or not manifest_value:
            raise ValueError(
                f"winner-unknown ICF authority target {target_id!r} has no manifest"
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
                f"winner-unknown ICF authority target {target_id!r} is outside "
                "tools/vc5_verify_targets"
            ) from exc
        from _recoil.lib.verification_targets import vc5_target_registration

        try:
            current_id, current = vc5_target_registration(manifest_path)
        except (OSError, ValueError) as exc:
            raise ValueError(
                f"winner-unknown ICF authority target {target_id!r} cannot be "
                f"synchronized: {exc}"
            ) from exc
        current_registration = current.get("registration")
        if (
            current_id != target_id
            or current.get("binary") != target.get("binary")
            or current.get("kind") != target.get("kind")
            or current.get("name") != target.get("name")
            or not isinstance(current_registration, Mapping)
            or current_registration.get("manifest_path") != manifest_value
        ):
            raise ValueError(
                f"winner-unknown ICF authority target {target_id!r} is stale or "
                "conflicting"
            )
        current_rows = [
            row
            for row in _cc_identity._mapping_target_function_rows(current)
            if row.get("logical_identity_key") in alias_ids
        ]
        current_all_at_address = [
            row
            for row in _cc_identity._mapping_target_function_rows(current)
            if isinstance(row.get("address"), str)
            and normalize_address(row["address"]) == address
            and isinstance(row.get("logical_identity_key"), str)
            and row.get("logical_identity_key")
        ]
        if (
            len(current_rows) != len(alias_ids)
            or {str(row["logical_identity_key"]) for row in current_rows} != alias_ids
            or len(current_all_at_address) != len(alias_ids)
            or {
                str(row["logical_identity_key"])
                for row in current_all_at_address
            }
            != alias_ids
            or any(
                dict(row) != dict(stored_rows[str(row["logical_identity_key"])])
                for row in current_rows
            )
        ):
            raise ValueError(
                f"winner-unknown ICF authority target {target_id!r} has stale, "
                "missing, duplicate, or conflicting current alias rows"
            )

        group_identity = f"icf-group:{symbol_id}"
        prior_group = groups_by_address.get(address)
        if prior_group not in {None, group_identity}:
            raise ValueError(
                f"winner-unknown ICF address {address} has conflicting groups"
            )
        groups_by_address[address] = group_identity
        for alias_id in alias_ids:
            logical_identity = f"logical:{alias_id}"
            prior = groups_by_logical_identity.get(logical_identity)
            if prior not in {None, group_identity}:
                raise ValueError(
                    f"winner-unknown ICF logical alias {alias_id} belongs to "
                    "conflicting groups"
                )
            groups_by_logical_identity[logical_identity] = group_identity

    return groups_by_address, groups_by_logical_identity


def _registered_non_gating_logical_target_indexes(
    document: ProgressDocument,
) -> tuple[dict[str, str], dict[str, str]]:
    """Index synchronized single logical targets without selecting a physical row.

    A physical function whose authored classification is still unresolved does
    not become ordinary ``by_address`` truth.  This narrower authority applies
    only when one non-gating registered logical body was accepted exactly once
    in authored order and its current manifest, tracker alias, owner, block,
    and physical neutral-group facts all converge.
    """
    symbols = document.collection("symbols")
    blocks = document.collection("physical_blocks")
    targets = document.collection("verification_targets")
    by_address: dict[str, str] = {}
    by_candidate_name: dict[str, str] = {}

    candidate_name_memberships: dict[
        str, list[tuple[str, str, str]]
    ] = {}
    for physical_id, candidate_symbol in symbols.items():
        if not isinstance(candidate_symbol, Mapping):
            continue
        aliases = candidate_symbol.get("logical_aliases")
        if not isinstance(aliases, Mapping):
            continue
        for alias_id, alias in aliases.items():
            object_symbol = (
                alias.get("object_symbol")
                if isinstance(alias, Mapping)
                else None
            )
            if isinstance(object_symbol, str) and object_symbol:
                candidate_name_memberships.setdefault(
                    object_symbol.casefold(), []
                ).append(
                    (
                        str(physical_id),
                        str(alias_id),
                        object_symbol,
                    )
                )

    accepted_occurrences: dict[str, int] = {}
    for block in blocks.values():
        facts = (
            block.get("accepted_order_facts")
            if isinstance(block, Mapping)
            else None
        )
        if (
            not isinstance(facts, Mapping)
            or facts.get("phase") != "authored-function-order"
        ):
            continue
        for identity in facts.get("matched_identities", []):
            if isinstance(identity, str):
                accepted_occurrences[identity] = (
                    accepted_occurrences.get(identity, 0) + 1
                )

    for physical_id, symbol in symbols.items():
        if not isinstance(symbol, Mapping):
            continue
        aliases = symbol.get("logical_aliases")
        if (
            not isinstance(aliases, Mapping)
            or not any(
                isinstance(alias, Mapping)
                and alias.get("fold_status") == "not-established"
                for alias in aliases.values()
            )
        ):
            continue
        raw_address = symbol.get("address", symbol.get("start"))
        if not isinstance(raw_address, str):
            raise ValueError(
                f"registered non-gating logical target {physical_id} has no "
                "physical address"
            )
        address = normalize_address(raw_address)
        if (
            symbol.get("binary") != "recoil"
            or symbol.get("kind") != "function"
            or symbol.get("disposition") != "unresolved"
            or symbol.get("pipeline_class") != "unresolved"
            or symbol.get("authored_order_role") != "unresolved"
            or symbol.get("ownership_state") != "unresolved"
            or symbol.get("extent_state") != "known"
            or symbol.get("output_section_id") != "recoil:section:.text"
            or symbol.get("accepted_order_facts") is not None
            or symbol.get("accepted_byte_facts") is not None
            or symbol.get("icf_address_group") is not None
            or symbol.get("linked_provider_binding") is not None
            or symbol.get("lifecycle_variant_of") is not None
            or len(aliases) != 1
        ):
            raise ValueError(
                f"registered non-gating logical target {physical_id} lacks "
                "the exact unresolved physical classification"
            )
        start = address_value(address)
        try:
            size = int(symbol.get("size"))
        except (TypeError, ValueError) as exc:
            raise ValueError(
                f"registered non-gating logical target {physical_id} has no "
                "known positive extent"
            ) from exc
        if (
            size <= 0
            or normalize_address(str(symbol.get("end_exclusive", "")))
            != normalize_address(start + size)
        ):
            raise ValueError(
                f"registered non-gating logical target {physical_id} has a "
                "conflicting physical extent"
            )
        linked_group = symbol.get("linked_address_group")
        if (
            not isinstance(linked_group, Mapping)
            or linked_group.get("group_kind")
            != "neutral-linked-address-group"
            or linked_group.get("fold_state") != "not-established"
            or linked_group.get("winner_status") != "not-established"
            or linked_group.get("winner_identity_key") is not None
        ):
            raise ValueError(
                f"registered non-gating logical target {physical_id} lacks "
                "neutral unresolved source/linkage state"
            )

        alias_id, alias = next(iter(aliases.items()))
        if not isinstance(alias, Mapping):
            raise ValueError(
                f"registered non-gating logical target {physical_id} has a "
                "malformed logical alias"
            )
        alias_id = str(alias_id)
        logical_identity = f"logical:{alias_id}"
        expected_alias_prefix = f"recoil:logical-function:{address}:"
        object_symbol = alias.get("object_symbol")
        original_name = alias.get("original_name")
        owner_id = alias.get("owner_id")
        if (
            not alias_id.startswith(expected_alias_prefix)
            or alias.get("pipeline_class") != "authored"
            or alias.get("authored_order_role") != "authored-body"
            or alias.get("fold_status") != "not-established"
            or alias.get("source_owner_status") != "authored-owner"
            or not isinstance(object_symbol, str)
            or not _cc_identity._decorated_coff_name(object_symbol)
            or not isinstance(original_name, str)
            or not original_name
            or not isinstance(owner_id, str)
            or not owner_id
        ):
            raise ValueError(
                f"registered non-gating logical target {physical_id} has "
                "incomplete authored alias authority"
            )
        name_memberships = candidate_name_memberships.get(
            object_symbol.casefold(), []
        )
        if name_memberships != [
            (str(physical_id), alias_id, object_symbol)
        ]:
            raise ValueError(
                f"registered non-gating logical target {physical_id} has an "
                "ambiguous or case-conflicting candidate object name"
            )

        block_id = symbol.get("physical_block_id")
        block = blocks.get(str(block_id))
        facts = (
            block.get("accepted_order_facts")
            if isinstance(block, Mapping)
            else None
        )
        expected_target_id = (
            facts.get("target_id")
            if isinstance(facts, Mapping)
            else None
        )
        vc5_target_ids = [
            str(candidate_target_id)
            for candidate_target_id in symbol.get(
                "verification_target_ids", []
            )
            if isinstance(
                targets.get(str(candidate_target_id)), Mapping
            )
            and targets[str(candidate_target_id)].get("kind") == "vc5"
        ]
        from _recoil.lib.authored_icf import exact_selected_target_membership
        if (
            not isinstance(block_id, str)
            or not isinstance(block, Mapping)
            or not isinstance(expected_target_id, str)
            or not expected_target_id
            or not exact_selected_target_membership(vc5_target_ids, expected_target_id)
        ):
            raise ValueError(
                f"registered non-gating logical target {physical_id} lacks "
                "one exact physical-block/VC5-target relationship"
            )
        stored_matches: list[
            tuple[str, Mapping[str, Any], str, Mapping[str, Any]]
        ] = []
        stored_same_address: list[
            tuple[str, str, Mapping[str, Any]]
        ] = []
        for target_id, target in targets.items():
            if (
                str(target_id) != expected_target_id
                or not isinstance(target, Mapping)
            ):
                continue
            for view, row in _cc_identity._mapping_target_function_rows_with_views(target):
                raw_row_address = row.get("address")
                if not isinstance(raw_row_address, str):
                    continue
                try:
                    row_address = normalize_address(raw_row_address)
                except ProgressError:
                    continue
                if row_address != address:
                    continue
                if isinstance(row.get("logical_identity_key"), str) and row.get(
                    "logical_identity_key"
                ):
                    stored_same_address.append((str(target_id), view, row))
                if row.get("logical_identity_key") == alias_id:
                    stored_matches.append(
                        (str(target_id), target, view, row)
                    )
        if (
            not stored_matches
            or not stored_same_address
            or len({item[0] for item in stored_matches}) != 1
            or any(
                dict(item[3]) != dict(stored_matches[0][3])
                for item in stored_matches
            )
            or any(
                item[0] != stored_matches[0][0]
                or dict(item[2]) != dict(stored_matches[0][3])
                for item in stored_same_address
            )
        ):
            raise ValueError(
                f"registered non-gating logical target {physical_id} has a "
                "missing, duplicate, or conflicting registered row"
            )
        target_id, target, _view, row = stored_matches[0]
        registration = target.get("registration")
        if (
            target.get("binary") != "recoil"
            or target.get("kind") != "vc5"
            or not isinstance(registration, Mapping)
            or registration.get("binary") != "recoil"
            or registration.get("name") != target.get("name")
            or normalize_address(str(row.get("address", ""))) != address
            or row.get("symbol") != object_symbol
            or row.get("symbol_regex") is not None
            or row.get("name") != original_name
            or row.get("pipeline_class") != "authored"
            or row.get("authored_order_role") != "authored-body"
            or row.get("authored_order_gate") is not True
            or row.get("authored_relative_order_gate") is not True
            or row.get("required_presence") is not True
            or row.get("full_order_gate") is not False
            or row.get("icf_fold_status") != "not-established"
        ):
            raise ValueError(
                f"registered non-gating logical target {physical_id} has a "
                "stale or gating registered alias row"
            )
        registered_addresses = target.get("registered_addresses")
        if (
            not isinstance(registered_addresses, list)
            or sum(
                1
                for raw_registered in registered_addresses
                if isinstance(raw_registered, str)
                and normalize_address(raw_registered) == address
            )
            != 1
        ):
            raise ValueError(
                f"registered non-gating logical target {physical_id} lacks "
                "one exact registered address"
            )
        manifest_value = (
            registration.get("manifest_path")
            if isinstance(registration, Mapping)
            else None
        )
        if not isinstance(manifest_value, str) or not manifest_value:
            raise ValueError(
                f"registered non-gating logical target {physical_id} has no "
                "manifest path"
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
                f"registered non-gating logical target {physical_id} has a "
                "manifest outside tools/vc5_verify_targets"
            ) from exc
        from _recoil.lib.verification_targets import vc5_target_registration

        try:
            current_id, current_target = vc5_target_registration(
                manifest_path
            )
        except (OSError, ValueError) as exc:
            raise ValueError(
                f"registered non-gating logical target {physical_id} cannot "
                "synchronize its registered target"
            ) from exc
        current_registration = current_target.get("registration")
        current_registered_addresses = current_target.get(
            "registered_addresses"
        )
        current_rows = [
            current_row
            for current_row in _cc_identity._mapping_target_function_rows(current_target)
            if isinstance(current_row.get("address"), str)
            and normalize_address(current_row["address"]) == address
        ]
        if (
            current_id != target_id
            or current_target.get("binary") != target.get("binary")
            or current_target.get("kind") != target.get("kind")
            or current_target.get("name") != target.get("name")
            or not isinstance(current_registration, Mapping)
            or current_registration.get("manifest_path") != manifest_value
            or current_registration.get("binary")
            != registration.get("binary")
            or current_registration.get("name")
            != registration.get("name")
            or not isinstance(current_registered_addresses, list)
            or sum(
                1
                for raw_registered in current_registered_addresses
                if isinstance(raw_registered, str)
                and normalize_address(raw_registered) == address
            )
            != 1
            or not current_rows
            or any(
                dict(current_row) != dict(row)
                for current_row in current_rows
            )
        ):
            raise ValueError(
                f"registered non-gating logical target {physical_id} has a "
                "stale or conflicting synchronized manifest row"
            )

        authored_order = (
            block.get("order", {}).get("authored", {})
            if isinstance(block, Mapping)
            and isinstance(block.get("order"), Mapping)
            else None
        )
        if (
            not isinstance(block_id, str)
            or not isinstance(block, Mapping)
            or not isinstance(facts, Mapping)
            or facts.get("phase") != "authored-function-order"
            or facts.get("validation_mode") != "live"
            or facts.get("target_id") != target_id
            or not isinstance(facts.get("covered_block_ids"), list)
            or facts["covered_block_ids"].count(block_id) != 1
            or not isinstance(facts.get("matched_identities"), list)
            or facts["matched_identities"].count(alias_id) != 1
            or facts["matched_identities"].count(str(physical_id)) != 0
            or accepted_occurrences.get(alias_id, 0) != 1
            or accepted_occurrences.get(str(physical_id), 0) != 0
            or not exact_selected_target_membership(vc5_target_ids, target_id)
            or not isinstance(authored_order, Mapping)
            or not authored_order
            or any(
                not isinstance(state, Mapping)
                or state.get("disposition") != "accepted"
                or state.get("freshness") != "current"
                or state.get("result") != "passed"
                or state.get("validation_mode") != "live"
                for state in authored_order.values()
            )
        ):
            raise ValueError(
                f"registered non-gating logical target {physical_id} lacks "
                "one exact accepted authored-order block/target occurrence"
            )

        registered_name_rows = [
            (
                str(candidate_target_id),
                candidate_row,
            )
            for candidate_target_id, candidate_target in targets.items()
            if isinstance(candidate_target, Mapping)
            for candidate_row in _cc_identity._mapping_target_function_rows(
                candidate_target
            )
            if isinstance(candidate_row.get("symbol"), str)
            and candidate_row["symbol"].casefold()
            == object_symbol.casefold()
        ]
        authority_name_rows = [item for item in registered_name_rows if item[0] == target_id]
        if (
            len(authority_name_rows) != 1
            or dict(authority_name_rows[0][1]) != dict(row)
            or any(
                candidate_row.get("symbol") != object_symbol
                or candidate_row.get("logical_identity_key") != alias_id
                or normalize_address(candidate_row.get("address")) != address
                for _, candidate_row in registered_name_rows
            )
        ):
            raise ValueError(
                f"registered non-gating logical target {physical_id} has an "
                "ambiguous registered candidate name"
            )
        prior_address = by_address.get(address)
        prior_name = by_candidate_name.get(object_symbol)
        if prior_address not in {None, logical_identity} or prior_name not in {
            None,
            logical_identity,
        }:
            raise ValueError(
                f"registered non-gating logical target {physical_id} "
                "collides with another reviewed logical target"
            )
        by_address[address] = logical_identity
        by_candidate_name[object_symbol] = logical_identity

    return by_address, by_candidate_name
