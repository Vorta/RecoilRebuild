"""Recoil call-contract storage identity evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import identity as _cc_identity

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CurrentIatStoragePackage,
        RegisteredDecoratedDataNameSupplier,
    )

from pathlib import Path
from typing import Any, Mapping

from _recoil.commands.vc5_verify import load_manifest
from _recoil.lib.progress import (
    ProgressDocument,
    ProgressError,
    address_value,
    is_current_accepted_state,
    normalize_address,
)
from _recoil.lib.tooling import REPO_ROOT


def _registered_data_supplier(
    document: ProgressDocument,
    *,
    target_id: str,
    target: Mapping[str, Any],
    current_target: Mapping[str, Any],
    row: Any,
) -> RegisteredDecoratedDataNameSupplier | None:
    from _recoil.call_contract.records import RegisteredDecoratedDataNameSupplier
    name = str(getattr(row, "symbol", "") or "")
    if (
        not name.startswith("?")
        or not _cc_identity._decorated_coff_name(name)
        or getattr(row, "symbol_regex", None) is not None
    ):
        return None
    try:
        address = normalize_address(str(getattr(row, "address", "")))
    except ProgressError:
        return None
    registration = target.get("registration")
    current_registration = current_target.get("registration")
    if not isinstance(registration, Mapping) or not isinstance(
        current_registration,
        Mapping,
    ):
        return None
    for data_addresses in (
        registration.get("data_addresses"),
        current_registration.get("data_addresses"),
    ):
        if (
            not isinstance(data_addresses, list)
            or sum(
                1
                for item in data_addresses
                if isinstance(item, str)
                and normalize_address(item) == address
            )
            != 1
        ):
            return None
    symbols = document.collection("symbols")
    address_symbols: list[tuple[str, Mapping[str, Any]]] = []
    for symbol_id, symbol in symbols.items():
        if (
            not isinstance(symbol, Mapping)
            or symbol.get("binary") != "recoil"
            or symbol.get("kind") not in {"data", "provider-data"}
        ):
            continue
        raw_address = symbol.get("address", symbol.get("start"))
        if not isinstance(raw_address, str):
            continue
        try:
            if normalize_address(raw_address) == address:
                address_symbols.append((str(symbol_id), symbol))
        except ProgressError:
            continue
    if len(address_symbols) != 1:
        return None
    symbol_id, symbol = address_symbols[0]
    if (
        symbol.get("kind") != "data"
        or symbol.get("disposition") != "authored"
        or symbol.get("navigation_name") != getattr(row, "name", "")
        or getattr(row, "bn_name", "") != getattr(row, "name", "")
    ):
        return None
    target_symbol_ids = target.get("symbol_ids")
    verification_target_ids = symbol.get("verification_target_ids")
    if (
        not isinstance(target_symbol_ids, list)
        or target_symbol_ids.count(symbol_id) != 1
        or len(target_symbol_ids) != len(set(target_symbol_ids))
        or len(target_symbol_ids)
        != len(registration.get("data_addresses", []))
        or not isinstance(verification_target_ids, list)
        or verification_target_ids.count(target_id) != 1
        or len(verification_target_ids) != len(set(verification_target_ids))
    ):
        return None
    target_symbol_addresses: list[str] = []
    for attached_symbol_id in target_symbol_ids:
        attached_symbol = symbols.get(attached_symbol_id)
        if (
            not isinstance(attached_symbol_id, str)
            or not isinstance(attached_symbol, Mapping)
            or attached_symbol.get("binary") != "recoil"
            or attached_symbol.get("kind") != "data"
        ):
            return None
        raw_attached_address = attached_symbol.get(
            "address",
            attached_symbol.get("start"),
        )
        if not isinstance(raw_attached_address, str):
            return None
        target_symbol_addresses.append(
            normalize_address(raw_attached_address)
        )
    if target_symbol_addresses != [
        normalize_address(item)
        for item in registration.get("data_addresses", [])
        if isinstance(item, str)
    ]:
        return None
    raw_storage_ids = symbol.get("storage_contribution_ids")
    if (
        not isinstance(raw_storage_ids, list)
        or len(raw_storage_ids) != 1
        or not isinstance(raw_storage_ids[0], str)
    ):
        return None
    storage_id = raw_storage_ids[0]
    storage_rows_at_address: list[tuple[str, Mapping[str, Any]]] = []
    for entity_id, storage in document.collection(
        "storage_contributions"
    ).items():
        if not isinstance(storage, Mapping):
            continue
        reference = storage.get("reference")
        raw_address = (
            reference.get("address")
            if isinstance(reference, Mapping)
            else storage.get("address", storage.get("start"))
        )
        if not isinstance(raw_address, str):
            continue
        try:
            if normalize_address(raw_address) == address:
                storage_rows_at_address.append((str(entity_id), storage))
        except ProgressError:
            continue
    if (
        len(storage_rows_at_address) != 1
        or storage_rows_at_address[0][0] != storage_id
    ):
        return None
    storage = storage_rows_at_address[0][1]
    reference = storage.get("reference")
    raw_owner_ids = storage.get("owner_ids")
    if (
        storage.get("binary") != "recoil"
        or storage.get("overlap") != "none"
        or storage.get("symbol_ids") != [symbol_id]
        or not isinstance(reference, Mapping)
        or normalize_address(str(reference.get("address", ""))) != address
        or not isinstance(raw_owner_ids, list)
        or len(raw_owner_ids) != 1
        or not isinstance(raw_owner_ids[0], str)
    ):
        return None
    owner_id = raw_owner_ids[0]
    owner = document.collection("owners").get(owner_id)
    if not isinstance(owner, Mapping):
        return None
    gates = owner.get("gates")
    if (
        owner.get("binary") != "recoil"
        or owner.get("kind") == "provider-boundary"
        or owner.get("provider_state") == "accepted"
        or owner.get("lifecycle_state") != "accepted"
        or not isinstance(gates, Mapping)
        or any(
            gates.get(gate) != "accepted"
            for gate in ("boundary", "source", "data", "owner_linkage")
        )
    ):
        return None
    owner_relationships = [
        (
            str(candidate_owner_id),
            relationship,
        )
        for candidate_owner_id, candidate_owner in document.collection(
            "owners"
        ).items()
        if isinstance(candidate_owner, Mapping)
        for relationship in candidate_owner.get("relationships", [])
        if isinstance(relationship, Mapping)
        and relationship.get("kind") == "primary-data"
        and relationship.get("symbol_id") == symbol_id
    ]
    if len(owner_relationships) != 1:
        return None
    relationship_owner_id, relationship = owner_relationships[0]
    if (
        relationship_owner_id != owner_id
        or normalize_address(str(relationship.get("address", ""))) != address
        or relationship.get("name") != getattr(row, "name", "")
    ):
        return None
    byte_length = getattr(row, "byte_length", None)
    if (
        isinstance(byte_length, bool)
        or not isinstance(byte_length, int)
        or byte_length <= 0
    ):
        return None
    extent_state = symbol.get("extent_state")
    if extent_state == "known":
        raw_size = symbol.get("size")
        raw_end = symbol.get("end_exclusive")
        if (
            isinstance(raw_size, bool)
            or not isinstance(raw_size, int)
            or raw_size != byte_length
            or not isinstance(raw_end, str)
            or address_value(normalize_address(raw_end))
            - address_value(address)
            != byte_length
        ):
            return None
    elif extent_state == "unknown":
        if symbol.get("size") is not None or symbol.get("end_exclusive") is not None:
            return None
    else:
        return None
    return RegisteredDecoratedDataNameSupplier(
        name=name,
        address=address,
        storage_identity=f"storage:{symbol_id}",
        target_id=target_id,
    )


def _index_registered_decorated_data_names(
    document: ProgressDocument,
    *,
    storage_by_name: dict[str, str],
    pooled_storage_by_name: Mapping[str, str] | None = None,
) -> None:
    """Publish exact decorated data names only after all suppliers converge."""
    from _recoil.lib.verification_targets import vc5_target_registration

    suppliers_by_name: dict[
        str, list[RegisteredDecoratedDataNameSupplier]
    ] = {}
    names_by_address: dict[str, set[str]] = {}
    ambiguous_names: set[str] = set()
    manifest_root = (REPO_ROOT / "tools" / "vc5_verify_targets").resolve()
    for target_id, target in document.collection(
        "verification_targets"
    ).items():
        if (
            not isinstance(target, Mapping)
            or target.get("binary") != "recoil"
            or target.get("kind") != "vc5"
        ):
            continue
        registration = target.get("registration")
        if (
            not isinstance(registration, Mapping)
            or not registration.get("data_addresses")
            or not target.get("symbol_ids")
        ):
            continue
        manifest_value = registration.get("manifest_path")
        if not isinstance(manifest_value, str) or not manifest_value:
            continue
        manifest_path = Path(manifest_value)
        if not manifest_path.is_absolute():
            manifest_path = REPO_ROOT / manifest_path
        try:
            manifest_path = manifest_path.resolve()
            manifest_path.relative_to(manifest_root)
            if not manifest_path.is_file():
                continue
            current_id, current_target = vc5_target_registration(
                manifest_path
            )
            manifest = load_manifest(
                manifest_path,
                enforce_source_policy=False,
            )
        except (OSError, ProgressError, ValueError):
            continue
        decorated_rows = [
            row
            for row in getattr(manifest, "data_symbols", ())
            if isinstance(getattr(row, "symbol", None), str)
            and row.symbol.startswith("?")
            and getattr(row, "symbol_regex", None) is None
        ]
        if not decorated_rows:
            continue
        for row in decorated_rows:
            try:
                address = normalize_address(str(row.address))
            except ProgressError:
                ambiguous_names.add(str(row.symbol))
                continue
            names_by_address.setdefault(address, set()).add(str(row.symbol))
        current_registration = current_target.get("registration")
        stable_target = (
            current_id == target_id
            and current_target.get("binary") == target.get("binary") == "recoil"
            and current_target.get("kind") == target.get("kind") == "vc5"
            and current_target.get("name") == target.get("name")
            and isinstance(current_registration, Mapping)
            and current_registration.get("manifest_path") == manifest_value
        )
        for row in decorated_rows:
            name = str(row.symbol)
            if not stable_target:
                ambiguous_names.add(name)
                continue
            same_address_rows = [
                candidate
                for candidate in decorated_rows
                if normalize_address(str(candidate.address))
                == normalize_address(str(row.address))
            ]
            if len(same_address_rows) != 1:
                ambiguous_names.add(name)
                continue
            if name in (pooled_storage_by_name or {}):
                supplier = _pooled_data_manifest_supplier(
                    document, row=row, target_id=str(target_id),
                    identity=pooled_storage_by_name[name],
                )
            else:
                supplier = _registered_data_supplier(
                    document,
                    target_id=str(target_id),
                    target=target,
                    current_target=current_target,
                    row=row,
                )
            if supplier is None:
                ambiguous_names.add(name)
                continue
            suppliers_by_name.setdefault(name, []).append(supplier)
    for names in names_by_address.values():
        if len(names) > 1:
            ambiguous_names.update(names)
    all_names = set(suppliers_by_name) | ambiguous_names
    for name in all_names:
        suppliers = suppliers_by_name.get(name, [])
        identities = {
            (supplier.storage_identity, supplier.address)
            for supplier in suppliers
        }
        if (
            name in ambiguous_names
            or len(identities) != 1
            or name in storage_by_name
        ):
            storage_by_name[name] = ""
            continue
        storage_by_name[name] = next(iter(identities))[0]


def _current_tracker_iat_storage_packages(
    document: ProgressDocument,
) -> tuple[CurrentIatStoragePackage, ...]:
    """Return complete current candidate-independent named/ordinal IAT packages."""
    from _recoil.call_contract.records import CurrentIatStoragePackage
    symbols = document.collection("symbols")
    storage_contributions = document.collection("storage_contributions")
    owners = document.collection("owners")
    evidence = document.collection("evidence")
    packages: list[CurrentIatStoragePackage] = []
    for function_id, function in symbols.items():
        if (
            not isinstance(function, Mapping)
            or function.get("kind") != "provider-function"
            or "import_name" not in function
            or "import_dll" not in function
        ):
            continue
        import_name = function.get("import_name")
        import_dll = function.get("import_dll")
        object_symbol = function.get("object_symbol")
        raw_address = function.get("address")
        has_ordinal = "import_ordinal" in function
        raw_ordinal = function.get("import_ordinal")
        import_ordinal = (
            raw_ordinal
            if (
                has_ordinal
                and isinstance(raw_ordinal, int)
                and not isinstance(raw_ordinal, bool)
                and 0 <= raw_ordinal <= 0xFFFF
            )
            else None
        )
        if (
            not isinstance(import_name, str)
            or not import_name
            or not isinstance(import_dll, str)
            or not import_dll
            or not isinstance(object_symbol, str)
            or not object_symbol.startswith("__imp_")
            or not _cc_identity._decorated_coff_name(object_symbol)
            or not isinstance(raw_address, str)
            or (has_ordinal and import_ordinal is None)
            or (
                import_ordinal is not None
                and import_name != f"#{import_ordinal}"
            )
        ):
            continue
        try:
            address = normalize_address(raw_address)
        except ProgressError:
            continue
        data_id = f"recoil:data:{address}"
        storage_id = f"recoil:storage:va:{address}"
        if function_id != f"recoil:function:{address}":
            continue
        data = symbols.get(data_id)
        storage = storage_contributions.get(storage_id)
        if not isinstance(storage, Mapping):
            continue
        owner_ids = storage.get("owner_ids")
        if (
            not isinstance(owner_ids, list)
            or len(owner_ids) != 1
            or not isinstance(owner_ids[0], str)
        ):
            continue
        owner_id = owner_ids[0]
        owner = owners.get(owner_id)
        end_exclusive = normalize_address(address_value(address) + 4)
        function_end = normalize_address(address_value(address) + 1)
        expected_relationships = [
            {"kind": "anchor-address", "address": address},
            {
                "kind": "primary-function",
                "address": address,
                "symbol_id": function_id,
            },
            {
                "kind": "primary-data",
                "address": address,
                "symbol_id": data_id,
                "name": f"{import_dll}!{import_name} IAT",
            },
        ]
        if (
            not isinstance(owner, Mapping)
            or owner.get("binary") != "recoil"
            or owner.get("kind") != "provider-boundary"
            or owner.get("blocker") != "none"
            or owner.get("lifecycle_state") != "accepted"
            or owner.get("provider_state") != "accepted"
            or owner.get("relationships") != expected_relationships
            or owner.get("source_paths") != []
            or function.get("binary") != "recoil"
            or function.get("disposition") != "provider"
            or function.get("pipeline_class") != "non-authored"
            or function.get("authored_order_role") != "non-authored"
            or function.get("address") != address
            or function.get("end_exclusive") != function_end
            or function.get("extent_state") != "known"
            or function.get("size") != 1
            or function.get("output_section_id") != "recoil:section:.rdata"
            or function.get("ownership_state") != "primary-owned"
            or function.get("physical_block_id") is not None
            or function.get("storage_contribution_ids") != []
        ):
            continue
        if (
            not isinstance(data, Mapping)
            or data.get("binary") != "recoil"
            or data.get("kind") != "data"
            or data.get("disposition") != "provider"
            or data.get("address") != address
            or data.get("end_exclusive") != end_exclusive
            or data.get("extent_state") != "known"
            or data.get("size") != 4
            or data.get("import_dll") != import_dll
            or data.get("import_name") != import_name
            or ("import_ordinal" in data) != has_ordinal
            or data.get("import_ordinal") != import_ordinal
            or data.get("output_section_id") != "recoil:section:.rdata"
            or data.get("ownership_state") != "primary-owned"
            or data.get("physical_block_id") is not None
            or data.get("storage_contribution_ids") != [storage_id]
        ):
            continue
        reference = storage.get("reference")
        verification = storage.get("verification")
        extent = (
            verification.get("extent")
            if isinstance(verification, Mapping)
            else None
        )
        if (
            storage.get("binary") != "recoil"
            or storage.get("kind") != "provider-data"
            or storage.get("output_section_id") != "recoil:section:.rdata"
            or storage.get("overlap") != "none"
            or storage.get("owner_ids") != [owner_id]
            or storage.get("parent_contribution_id") is not None
            or storage.get("symbol_ids") != [data_id]
            or not isinstance(reference, Mapping)
            or reference.get("address") != address
            or reference.get("end_exclusive") != end_exclusive
            or reference.get("extent_state") != "known"
            or reference.get("size") != 4
            or not isinstance(extent, Mapping)
            or not is_current_accepted_state(extent)
            or extent.get("validation_mode") != "live"
            or extent.get("gating") is not True
        ):
            continue
        shared_evidence = set(owner.get("evidence_ids", ()))
        for row in (function, data, storage, reference, extent):
            shared_evidence &= set(row.get("evidence_ids", ()))
        expected_scope_ids = {owner_id, function_id, data_id, storage_id}
        expected_iat_rva = normalize_address(
            address_value(address) - 0x400000
        )
        expected_iat_end_rva = normalize_address(
            address_value(end_exclusive) - 0x400000
        )
        current_evidence: list[Mapping[str, Any]] = []
        for evidence_id in shared_evidence:
            row = evidence.get(evidence_id)
            scope_ids = (
                row.get("scope_ids") if isinstance(row, Mapping) else None
            )
            if (
                not isinstance(row, Mapping)
                or row.get("kind")
                not in {
                    "provider-target-registration",
                    "provider-target-legacy-completion",
                }
                or not is_current_accepted_state(row)
                or row.get("validation_mode") != "live"
                or row.get("gating") is not True
                or not isinstance(scope_ids, list)
                or len(scope_ids) != len(expected_scope_ids)
                or set(scope_ids) != expected_scope_ids
            ):
                continue
            provenance = row.get("provenance")
            if not isinstance(provenance, Mapping):
                continue
            operation = provenance.get("operation")
            if (
                row.get("kind") == "provider-target-legacy-completion"
                and operation != "legacy-provider-completion"
            ) or (
                row.get("kind") == "provider-target-registration"
                and operation == "legacy-provider-completion"
            ):
                continue
            descriptor_index = provenance.get("descriptor_index")
            thunk_index = provenance.get("thunk_index")
            if (
                provenance.get("producer") != "pe32-import-directory"
                or provenance.get("reference") != "support/Recoil.exe"
                or provenance.get("candidate_independent") is not True
                or provenance.get("address") != address
                or provenance.get("dll") != import_dll
                or provenance.get("import_name") != import_name
                or provenance.get("import_ordinal") != import_ordinal
                or provenance.get("iat_rva") != expected_iat_rva
                or provenance.get("iat_end_rva") != expected_iat_end_rva
                or not isinstance(descriptor_index, int)
                or isinstance(descriptor_index, bool)
                or descriptor_index < 0
                or not isinstance(thunk_index, int)
                or isinstance(thunk_index, bool)
                or thunk_index < 0
                or provenance.get("object_symbol") != object_symbol
                or provenance.get("object_symbol_basis")
                != "reviewed-vc5-provider-declaration"
            ):
                continue
            current_evidence.append(row)
        if len(current_evidence) != 1:
            continue
        packages.append(
            CurrentIatStoragePackage(
                address=address,
                import_dll=import_dll,
                import_name=import_name,
                import_ordinal=import_ordinal,
                object_symbol=object_symbol,
                identity=(
                    f"iat:{import_name}"
                    if import_ordinal is None
                    else (
                        "iat:ordinal:"
                        f"{len(import_dll)}:{import_dll}:{import_ordinal}"
                    )
                ),
            )
        )
    return tuple(packages)


def _pooled_data_manifest_supplier(
    document: ProgressDocument, *, row: Any, target_id: str, identity: str,
) -> RegisteredDecoratedDataNameSupplier | None:
    """Require a literal manifest occurrence to agree with typed pooling.

    The caller supplies an independently validated pooling identity. Old
    navigation names do not turn a provider literal into authored storage.
    A differing physical address or extent remains a conflicting supplier.
    """
    from _recoil.call_contract.records import RegisteredDecoratedDataNameSupplier
    if not identity.startswith("storage:"):
        return None
    physical = document.collection("symbols").get(identity[len("storage:"):])
    if not isinstance(physical, Mapping):
        return None
    try:
        address = normalize_address(str(row.address))
    except (AttributeError, ValueError, ProgressError):
        return None
    if (address != physical.get("address")
            or type(getattr(row, "byte_length", None)) is not int
            or row.byte_length != physical.get("size")):
        return None
    return RegisteredDecoratedDataNameSupplier(
        name=str(row.symbol), address=address, storage_identity=identity,
        target_id=target_id,
    )


def _index_reviewed_pooled_data_names(
    document: ProgressDocument,
    *,
    storage_by_address: Mapping[str, str],
    storage_by_name: dict[str, str],
) -> None:
    """Resolve accepted compiler-literal aliases to one physical artifact.

    A literal's contents or candidate spelling alone never establishes its
    retail address. Only existing typed pooling relationships are indexed;
    missing evidence, malformed extents and competing suppliers fail closed.
    """
    evidence = document.collection("evidence")
    suppliers: dict[str, set[str]] = {}
    for symbol_id, physical in document.collection("symbols").items():
        if not isinstance(physical, Mapping):
            continue
        aliases = physical.get("logical_aliases")
        if not isinstance(aliases, Mapping):
            continue
        for alias in aliases.values():
            if not isinstance(alias, Mapping):
                continue
            name = alias.get("object_symbol")
            if not isinstance(name, str) or not name.startswith("??_C@"):
                continue
            identity = f"storage:{symbol_id}"
            pooling = alias.get("pooling")
            trace = physical.get("source_traceability")
            try:
                address = normalize_address(physical.get("address"))
                start = address_value(address)
                end = address_value(normalize_address(physical.get("end_exclusive")))
            except (TypeError, ValueError, ProgressError):
                address, start, end = "", 0, 0
            evidence_lists = (alias.get("evidence_ids"),
                pooling.get("evidence_ids") if isinstance(pooling, Mapping) else None)
            valid = (
                physical.get("binary") == "recoil"
                and physical.get("kind") == "data"
                and physical.get("disposition") == "provider"
                and physical.get("extent_state") == "known"
                and type(physical.get("size")) is int
                and physical["size"] == end - start > 0
                and storage_by_address.get(address) == identity
                and isinstance(trace, Mapping)
                and trace.get("state") == "not-applicable"
                and trace.get("reason_code") == "compiler-linker-literal-pooling"
                and trace.get("source_edges") == []
                and alias.get("kind") == "data"
                and isinstance(pooling, Mapping)
                and pooling.get("mode") == "compiler-literal-pooling"
                and pooling.get("physical_artifact_id") == symbol_id
                and all(isinstance(ids, list) and bool(ids)
                    and all(isinstance(item, str) and item in evidence for item in ids)
                    for ids in evidence_lists)
            )
            suppliers.setdefault(name, set()).add(identity if valid else "")
    for name, identities in suppliers.items():
        _cc_identity._publish_collected_identity(storage_by_name, name, identities)
