"""Recoil call-contract recoil lifecycle evidence and checks."""

from __future__ import annotations

from _recoil.call_contract import targets as _cc_targets

from typing import TYPE_CHECKING

from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import cfg as _cc_cfg
from _recoil.call_contract import comparison as _cc_comparison
from _recoil.call_contract import dispatch as _cc_dispatch
from _recoil.call_contract import errors as _cc_errors
from _recoil.call_contract import extraction as _cc_extraction
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract import lifecycle as _cc_lifecycle
from _recoil.call_contract import listing as _cc_listing
from _recoil.call_contract import recoil_audio as _cc_recoil_audio
from _recoil.call_contract import recoil_hud_lifetimes as _cc_recoil_hud_lifetimes
from _recoil.call_contract import retail as _cc_retail

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateAssembly,
        IdentityIndexes,
        RegisteredLifecycleDeletingDestructorSupplier,
        RegisteredScalarDeletingDestructorSupplier,
        ReviewedEhArrayDestructorCallSpec,
    )

import json
import re
import struct
from dataclasses import replace
from pathlib import Path
from typing import Any, Mapping, NoReturn, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_DIR32,
    IMAGE_REL_I386_REL32,
    Instruction,
)
from _recoil.lib.binja import BinaryNinjaBridge
from _recoil.lib.progress import (
    ProgressDocument,
    ProgressError,
    address_value,
    normalize_address,
)
from _recoil.lib.tooling import REPO_ROOT


def _deleting_destructor_names_for_complete(
    complete_name: Any,
) -> tuple[str, str]:
    """Return the two exact VC5 deleting variants for one complete destructor."""
    if not isinstance(complete_name, str):
        return ()
    match = _cc_catalog.MSVC_COMPLETE_DESTRUCTOR_PARTS_RE.fullmatch(complete_name)
    if match is None:
        return ()
    class_name = match.group("class_name")
    qualifier = match.group("qualifier")
    return tuple(
        f"??_{variant}{class_name}{qualifier}PAXI@Z"
        for variant in ("E", "G")
    )


def _registered_lifecycle_deleting_destructor_supplier(
    document: ProgressDocument,
    *,
    target_id: str,
    row: Mapping[str, Any],
    by_address: Mapping[str, str],
) -> tuple[RegisteredLifecycleDeletingDestructorSupplier, ...] | None:
    """Prove one current manifest row is an exact non-gating lifecycle identity."""
    from _recoil.call_contract.records import (
        RegisteredLifecycleDeletingDestructorSupplier,
    )
    complete_name = row.get("symbol")
    candidate_names = _deleting_destructor_names_for_complete(complete_name)
    raw_address = row.get("address")
    if not candidate_names or not isinstance(raw_address, str):
        return None
    try:
        address = normalize_address(raw_address)
    except ProgressError:
        return None
    if (
        row.get("symbol_regex") is not None
        or row.get("pipeline_class") != "authored-lifecycle"
        or row.get("authored_order_role")
        != "compiler-generated-implicit-cleanup"
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
        if (
            not isinstance(symbol, Mapping)
            or symbol.get("kind")
            not in {"function", "provider-function", "compiler-function"}
        ):
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
    if (
        symbol.get("binary") != "recoil"
        or symbol.get("kind") != "function"
        or symbol.get("pipeline_class") != "authored-lifecycle"
        or symbol.get("authored_order_role")
        != "compiler-generated-implicit-cleanup"
        or symbol.get("extent_state") != "known"
        or isinstance(raw_size, bool)
        or not isinstance(raw_size, int)
        or raw_size <= 0
        or not isinstance(raw_end, str)
        or address_value(normalize_address(raw_end))
        - address_value(address)
        != raw_size
        or symbol.get("logical_identity_key") not in {None, ""}
        or symbol.get("icf_fold_status") not in {None, ""}
        or isinstance(logical_aliases, Mapping)
        and bool(logical_aliases)
        or isinstance(icf_group, Mapping)
        and bool(icf_group)
    ):
        return None
    identity = f"symbol:{symbol_id}"
    if by_address.get(address) != identity:
        return None
    return tuple(
        RegisteredLifecycleDeletingDestructorSupplier(
            candidate_name=candidate_name,
            complete_name=str(complete_name),
            address=address,
            identity=identity,
            target_id=target_id,
        )
        for candidate_name in candidate_names
    )


def _index_registered_lifecycle_deleting_destructor_names(
    document: ProgressDocument,
    *,
    by_address: Mapping[str, str],
    by_candidate_name: dict[str, str],
) -> None:
    """Bridge exact VC5 deleting names only from convergent current lifecycle rows."""
    from _recoil.lib.verification_targets import vc5_target_registration

    suppliers_by_name: dict[
        str, list[RegisteredLifecycleDeletingDestructorSupplier]
    ] = {}
    complete_names_by_address: dict[str, set[str]] = {}
    ambiguous_names: set[str] = set()
    explicit_identities: dict[str, set[str]] = {}
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
        if not isinstance(registration, Mapping):
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
        except (OSError, ProgressError, ValueError):
            continue
        current_registration = current_target.get("registration")
        stable_target = (
            current_id == target_id
            and current_target.get("binary") == target.get("binary") == "recoil"
            and current_target.get("kind") == target.get("kind") == "vc5"
            and current_target.get("name") == target.get("name")
            and isinstance(current_registration, Mapping)
            and current_registration.get("manifest_path") == manifest_value
        )
        stored_rows = tuple(_cc_identity._mapping_target_function_rows(target))
        for explicit_row in _cc_identity._mapping_target_function_rows(current_target):
            explicit_name = explicit_row.get("symbol")
            if (not stable_target or explicit_row not in stored_rows
                    or not isinstance(explicit_name, str)
                    or re.fullmatch(r"\?\?_G.+@@(?:UAE|QAE)PAXI@Z", explicit_name) is None
                    or explicit_row.get("authored_order_role") != "compiler-generated-deleting-variant"):
                continue
            explicit_address = normalize_address(explicit_row["address"])
            explicit_identity = by_address.get(explicit_address)
            if explicit_identity:
                explicit_identities.setdefault(explicit_name, set()).add(explicit_identity)
        current_rows = [
            row
            for row in _cc_identity._mapping_target_function_rows(current_target)
            if _deleting_destructor_names_for_complete(row.get("symbol"))
        ]
        for row in current_rows:
            complete_name = str(row["symbol"])
            candidate_names = _deleting_destructor_names_for_complete(
                complete_name
            )
            raw_address = row.get("address")
            try:
                address = normalize_address(str(raw_address))
            except ProgressError:
                ambiguous_names.update(candidate_names)
                continue
            complete_names_by_address.setdefault(address, set()).add(
                complete_name
            )
            same_identity_rows = [
                candidate
                for candidate in current_rows
                if candidate.get("symbol") == complete_name
                and isinstance(candidate.get("address"), str)
                and normalize_address(candidate["address"]) == address
            ]
            if not stable_target or len(same_identity_rows) != 1:
                ambiguous_names.update(candidate_names)
                continue
            suppliers = _registered_lifecycle_deleting_destructor_supplier(
                document,
                target_id=str(target_id),
                row=row,
                by_address=by_address,
            )
            if suppliers is None:
                ambiguous_names.update(candidate_names)
                continue
            for supplier in suppliers:
                suppliers_by_name.setdefault(
                    supplier.candidate_name, []
                ).append(supplier)

    for address, complete_names in complete_names_by_address.items():
        if len(complete_names) <= 1:
            continue
        for complete_name in complete_names:
            ambiguous_names.update(
                _deleting_destructor_names_for_complete(complete_name)
            )
    for name in set(suppliers_by_name) | ambiguous_names:
        suppliers = suppliers_by_name.get(name, [])
        identities = {
            (
                supplier.identity,
                supplier.address,
                supplier.complete_name,
            )
            for supplier in suppliers
        }
        prior = by_candidate_name.get(name)
        explicit = explicit_identities.get(name, set())
        if explicit:
            # A literal synchronized row is not an inferred complete->deleting
            # spelling fallback. It retains its own physical target identity;
            # conflicting explicit registrations still fail closed.
            by_candidate_name[name] = _explicit_lifecycle_target_identity(prior, explicit)
            continue
        if (
            name in ambiguous_names
            or len(identities) != 1
            or prior not in {None, next(iter(identities))[0]}
        ):
            by_candidate_name[name] = ""
            continue
        by_candidate_name[name] = next(iter(identities))[0]


def _explicit_lifecycle_target_identity(prior: str | None, identities: set[str]) -> str:
    if len(identities) != 1 or not prior or prior not in identities:
        return ""
    return prior


def _nonvirtual_scalar_deleting_destructor_class(
    name: Any,
) -> tuple[str, str] | None:
    """Parse one exact natural VC5 x86 nonvirtual scalar-deleting ABI."""
    if not isinstance(name, str):
        return None
    match = _cc_catalog.MSVC_NONVIRTUAL_SCALAR_DELETING_DESTRUCTOR_RE.fullmatch(name)
    if match is None:
        return None
    encoded = match.group("class_scope")
    return encoded, "::".join(reversed(encoded.split("@")))


def _scalar_deleting_destructor_row_class(
    row: Mapping[str, Any],
) -> str | None:
    """Return the one literal ``??_G`` class family carried by a row."""
    symbol = row.get("symbol")
    symbol_regex = row.get("symbol_regex")
    if bool(symbol) == bool(symbol_regex):
        return None
    if symbol is not None and not isinstance(symbol, str):
        return None
    if isinstance(symbol, str) and symbol:
        parsed = _nonvirtual_scalar_deleting_destructor_class(symbol)
        return parsed[1] if parsed is not None else None
    if not isinstance(symbol_regex, str):
        return None
    try:
        re.compile(symbol_regex)
    except re.error:
        return None
    readable = symbol_regex.replace(r"\?", "?")
    encoded_classes = {
        match.group("class_scope")
        for match in re.finditer(
            r"\?\?_G"
            r"(?P<class_scope>[A-Za-z_][A-Za-z0-9_]*"
            r"(?:@[A-Za-z_][A-Za-z0-9_]*)*)@@",
            readable,
        )
    }
    if len(encoded_classes) != 1:
        return None
    encoded = next(iter(encoded_classes))
    canonical = f"??_G{encoded}@@QAEPAXI@Z"
    if re.fullmatch(symbol_regex, canonical) is None:
        return None
    return "::".join(reversed(encoded.split("@")))


def _synchronized_scalar_deleting_destructor_targets(
    document: ProgressDocument,
    *,
    address: str,
    class_identity: str,
    navigation_name: str,
) -> tuple[tuple[str, ...], str | None]:
    """Find convergent current VC5 rows for one reviewed deleting variant."""
    from _recoil.lib.verification_targets import vc5_target_registration

    evidence: set[str] = set()
    manifest_root = (REPO_ROOT / "tools" / "vc5_verify_targets").resolve()

    def normalized_address_value(value: Any) -> str | None:
        if not isinstance(value, str):
            return None
        try:
            return normalize_address(value)
        except ProgressError:
            return None

    def normalized_row_address(row: Mapping[str, Any]) -> str | None:
        return normalized_address_value(row.get("address"))

    def relevant_row(row: Mapping[str, Any]) -> bool:
        return (
            normalized_row_address(row) == address
            or row.get("name") == navigation_name
        )

    for target_id, target in document.collection(
        "verification_targets"
    ).items():
        if (
            not isinstance(target, Mapping)
            or target.get("binary") != "recoil"
            or target.get("kind") != "vc5"
        ):
            continue
        stored_rows = [
            row
            for row in _cc_identity._mapping_target_function_rows(target)
            if relevant_row(row)
        ]
        if not stored_rows:
            continue
        registration = target.get("registration")
        manifest_value = (
            registration.get("manifest_path")
            if isinstance(registration, Mapping)
            else None
        )
        if not isinstance(manifest_value, str) or not manifest_value:
            return (), "stale or conflicting registered scalar-deleting-destructor evidence"
        manifest_path = Path(manifest_value)
        if not manifest_path.is_absolute():
            manifest_path = REPO_ROOT / manifest_path
        try:
            manifest_path = manifest_path.resolve()
            manifest_path.relative_to(manifest_root)
            if not manifest_path.is_file():
                raise ValueError("missing manifest")
            current_id, current = vc5_target_registration(manifest_path)
            raw_manifest = json.loads(
                manifest_path.read_text(encoding="utf-8")
            )
            if not isinstance(raw_manifest, Mapping):
                raise ValueError("manifest root is not an object")
        except (OSError, ProgressError, ValueError):
            return (), "stale or conflicting registered scalar-deleting-destructor evidence"
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
            return (), "stale or conflicting registered scalar-deleting-destructor evidence"
        registered_addresses = current.get("registered_addresses")
        if (
            not isinstance(registered_addresses, list)
            or sum(
                1
                for item in registered_addresses
                if normalized_address_value(item) == address
            )
            != 1
        ):
            return (), "stale or conflicting registered scalar-deleting-destructor evidence"
        current_rows = [
            row
            for row in _cc_identity._mapping_target_function_rows(current)
            if relevant_row(row)
        ]
        stored_matches = [
            row
            for row in stored_rows
            if normalized_row_address(row) == address
            and row.get("name") == navigation_name
            and _scalar_deleting_destructor_row_class(row)
            == class_identity
        ]
        current_matches = [
            row
            for row in current_rows
            if normalized_row_address(row) == address
            and row.get("name") == navigation_name
            and _scalar_deleting_destructor_row_class(row)
            == class_identity
        ]
        raw_rows: list[Mapping[str, Any]] = [
            row
            for row in raw_manifest.get("functions", [])
            if isinstance(row, Mapping)
        ]
        for collection_name in (
            "translation_unit_function_order",
            "linked_function_intervals",
        ):
            for entry in raw_manifest.get(collection_name, []):
                if isinstance(entry, Mapping):
                    raw_rows.extend(
                        row
                        for row in entry.get("functions", [])
                        if isinstance(row, Mapping)
                    )
        raw_matches = [
            row
            for row in raw_rows
            if normalized_row_address(row) == address
            and row.get("name") == navigation_name
            and _scalar_deleting_destructor_row_class(row)
            == class_identity
        ]
        raw_relevant_rows = [row for row in raw_rows if relevant_row(row)]

        def authority(row: Mapping[str, Any]) -> tuple[Any, ...]:
            raw_symbol = row.get("symbol")
            raw_symbol_regex = row.get("symbol_regex")
            return (
                normalize_address(str(row.get("address"))),
                row.get("name"),
                raw_symbol if isinstance(raw_symbol, str) else "",
                (
                    raw_symbol_regex
                    if isinstance(raw_symbol_regex, str)
                    else ""
                ),
                _scalar_deleting_destructor_row_class(row),
            )

        if (
            len(stored_rows) != 1
            or len(current_rows) != 1
            or len(raw_relevant_rows) != 1
            or len(stored_matches) != 1
            or len(current_matches) != 1
            or len(raw_matches) != 1
            or authority(stored_matches[0])
            != authority(current_matches[0])
            or authority(stored_matches[0])
            != authority(raw_matches[0])
        ):
            return (), "stale or conflicting registered scalar-deleting-destructor evidence"
        evidence.add(f"registered:{target_id}")
    return tuple(sorted(evidence)), None


def _scalar_deleting_destructor_bn_evidence(
    bridge_by_address: Mapping[str, Any],
    *,
    address: str,
    class_identity: str,
    navigation_name: str,
) -> tuple[bool, bool]:
    """Return ``(present, conflicting)`` for exact retail BN symbol evidence."""
    rows: list[Any] = []
    for raw_address, row in bridge_by_address.items():
        try:
            if normalize_address(str(raw_address)) == address:
                rows.append(row)
        except ProgressError:
            continue
    if len(rows) > 1:
        return False, True
    if not rows:
        return False, False
    row = rows[0]
    try:
        row_address = normalize_address(str(getattr(row, "address", "")))
    except ProgressError:
        return False, True
    if row_address != address:
        return False, True
    names = {
        str(value)
        for value in (
            getattr(row, "name", ""),
            getattr(row, "raw_name", ""),
            getattr(row, "full_name", ""),
        )
        if value
    }
    observed_classes: set[str] = set()
    malformed_abi = False
    for name in names:
        navigation = _cc_catalog.SCALAR_DELETING_DESTRUCTOR_NAVIGATION_RE.fullmatch(name)
        if navigation is not None:
            observed_classes.add(navigation.group("class_name"))
        if name.startswith(_cc_catalog.MSVC_SCALAR_DELETING_DESTRUCTOR_PREFIX):
            parsed = _nonvirtual_scalar_deleting_destructor_class(name)
            if parsed is None:
                malformed_abi = True
            else:
                observed_classes.add(parsed[1])
    if malformed_abi or observed_classes - {class_identity}:
        return False, True
    return (
        navigation_name in names or class_identity in observed_classes,
        False,
    )


def _registered_scalar_deleting_destructor_suppliers(
    document: ProgressDocument,
    *,
    by_address: Mapping[str, str],
    bridge_by_address: Mapping[str, Any],
) -> tuple[
    tuple[RegisteredScalarDeletingDestructorSupplier, ...],
    dict[str, str],
]:
    """Index reviewed direct deleting variants without candidate-derived truth."""
    from _recoil.call_contract.records import RegisteredScalarDeletingDestructorSupplier
    candidates_by_class: dict[
        str, list[tuple[str, Mapping[str, Any], str]]
    ] = {}
    for symbol_id, symbol in document.collection("symbols").items():
        if not isinstance(symbol, Mapping):
            continue
        navigation_name = symbol.get("navigation_name")
        navigation = (
            _cc_catalog.SCALAR_DELETING_DESTRUCTOR_NAVIGATION_RE.fullmatch(
                navigation_name
            )
            if isinstance(navigation_name, str)
            else None
        )
        if navigation is None:
            continue
        candidates_by_class.setdefault(
            navigation.group("class_name"), []
        ).append((str(symbol_id), symbol, navigation_name))

    suppliers: list[RegisteredScalarDeletingDestructorSupplier] = []
    blockers: dict[str, str] = {}
    for class_identity, candidates in candidates_by_class.items():
        if len(candidates) != 1:
            blockers[class_identity] = (
                "ambiguous reviewed scalar-deleting-destructor tracker identity"
            )
            continue
        symbol_id, symbol, navigation_name = candidates[0]
        raw_address = symbol.get("address", symbol.get("start"))
        raw_end = symbol.get("end_exclusive")
        raw_size = symbol.get("size")
        try:
            address = normalize_address(str(raw_address))
            end_exclusive = normalize_address(str(raw_end))
        except ProgressError:
            blockers[class_identity] = (
                "stale or conflicting scalar-deleting-destructor tracker identity"
            )
            continue
        identity = f"symbol:{symbol_id}"
        logical_aliases = symbol.get("logical_aliases")
        icf_group = symbol.get("icf_address_group")
        if (
            symbol.get("binary") != "recoil"
            or symbol.get("kind") != "function"
            or symbol.get("pipeline_class") != "authored-lifecycle"
            or symbol.get("authored_order_role")
            != "compiler-generated-deleting-variant"
            or symbol.get("extent_state") != "known"
            or isinstance(raw_size, bool)
            or not isinstance(raw_size, int)
            or raw_size <= 0
            or address_value(end_exclusive) - address_value(address)
            != raw_size
            or by_address.get(address) != identity
            or symbol.get("logical_identity_key") not in {None, ""}
            or symbol.get("icf_fold_status") not in {None, ""}
            or isinstance(logical_aliases, Mapping)
            and bool(logical_aliases)
            or isinstance(icf_group, Mapping)
            and bool(icf_group)
        ):
            blockers[class_identity] = (
                "stale or conflicting scalar-deleting-destructor tracker identity"
            )
            continue
        registered, registration_blocker = (
            _synchronized_scalar_deleting_destructor_targets(
                document,
                address=address,
                class_identity=class_identity,
                navigation_name=navigation_name,
            )
        )
        bn_present, bn_conflicting = (
            _scalar_deleting_destructor_bn_evidence(
                bridge_by_address,
                address=address,
                class_identity=class_identity,
                navigation_name=navigation_name,
            )
        )
        if registration_blocker is not None or bn_conflicting:
            blockers[class_identity] = (
                registration_blocker
                or "conflicting BN scalar-deleting-destructor evidence"
            )
            continue
        evidence_sources = (
            *registered,
            *(("bn",) if bn_present else ()),
        )
        if not evidence_sources:
            blockers[class_identity] = (
                "missing current registered or BN-backed scalar-deleting-destructor evidence"
            )
            continue
        suppliers.append(
            RegisteredScalarDeletingDestructorSupplier(
                class_identity=class_identity,
                address=address,
                identity=identity,
                evidence_sources=tuple(evidence_sources),
            )
        )
    return tuple(suppliers), blockers


def _validate_comparison_scoped_lifecycle_extent(
    retail_instructions: Sequence[Instruction],
    *,
    document: ProgressDocument,
    caller_id: str,
    caller: Mapping[str, Any],
    caller_start: str,
    caller_end_exclusive: str,
    reference: Path,
) -> None:
    """Derive the lifecycle body end from retail code and its next function.

    Registered lifecycle contributions may include alignment or a separately
    declared shared-return artifact.  They are ownership/order extents, not
    callable-body truth.  The live body therefore ends at its last decoded
    terminal instruction before the next unique declared function start; only
    the intervening retail seam must be NOP padding.
    """

    start = address_value(caller_start)
    registered_end = address_value(caller_end_exclusive)
    runtime_addresses = _cc_cfg._instruction_runtime_addresses(
        retail_instructions,
        source="bn",
        caller_start=start,
    )
    if (
        not retail_instructions
        or len(runtime_addresses) != len(retail_instructions)
        or any(address is None for address in runtime_addresses)
        or any(not instruction.bytes for instruction in retail_instructions)
    ):
        raise ValueError(
            "comparison-scoped lifecycle caller has ambiguous decoded extent"
        )
    block_id = caller.get("physical_block_id")
    block = document.collection("physical_blocks").get(str(block_id))
    if (
        not isinstance(block, Mapping)
        or block.get("binary") not in {None, "recoil"}
        or block.get("row_kind") != "physical-source-block"
        or block.get("contribution_kind") != "authored"
        or address_value(str(block.get("start", ""))) > start
        or address_value(str(block.get("end_exclusive", "")))
        < registered_end
    ):
        raise ValueError(
            "comparison-scoped lifecycle caller lacks one exact authored "
            "physical contribution extent"
        )

    later_boundaries: list[tuple[int, str]] = []
    for symbol_id, row in document.collection("symbols").items():
        if symbol_id == caller_id or not isinstance(row, Mapping):
            continue
        if (
            row.get("binary") != "recoil"
            or row.get("kind")
            not in {"function", "provider-function", "compiler-function"}
            or row.get("pipeline_class")
            not in {"authored", "authored-lifecycle", "non-authored"}
            or row.get("ownership_state")
            not in {"primary-owned", "unresolved"}
            or not isinstance(row.get("address"), str)
            or row.get("extent_state") != "known"
            or not isinstance(row.get("end_exclusive"), str)
        ):
            continue
        boundary = address_value(str(row["address"]))
        boundary_end = address_value(str(row["end_exclusive"]))
        if (
            boundary_end <= boundary
            or row.get("size") != boundary_end - boundary
        ):
            continue
        if boundary > start:
            later_boundaries.append((boundary, symbol_id))
    if not later_boundaries:
        raise ValueError(
            "comparison-scoped lifecycle caller has no next registered "
            "function contribution boundary"
        )
    next_boundary = min(boundary for boundary, _ in later_boundaries)
    rows_at_next_boundary = [
        symbol_id
        for boundary, symbol_id in later_boundaries
        if boundary == next_boundary
    ]
    if (
        len(rows_at_next_boundary) != 1
        or next_boundary > registered_end
        or next_boundary <= start
    ):
        raise ValueError(
            "comparison-scoped lifecycle caller has an ambiguous or "
            "overlapping next declared function start"
        )

    body_rows = sorted(
        (
            int(address),
            int(address) + len(instruction.bytes),
            instruction,
        )
        for address, instruction in zip(
            runtime_addresses,
            retail_instructions,
        )
        if address is not None and start <= int(address) < next_boundary
    )
    if (
        not body_rows
        or any(
            current_start < prior_end
            for (_, prior_end, _), (current_start, _, _) in zip(
                body_rows,
                body_rows[1:],
            )
        )
        or any(end > next_boundary for _, end, _ in body_rows)
    ):
        raise ValueError(
            "comparison-scoped lifecycle caller has overlapping or "
            "out-of-body decoded instructions"
        )
    terminal_candidates = [
        (end, instruction)
        for _address, end, instruction in body_rows
        if (
            _cc_cfg._exact_return_terminates(instruction)
            or (
                _cc_cfg._instruction_mnemonic(instruction) == "jmp"
                and _cc_cfg._exact_invocation_encoding(instruction, mnemonic="jmp")
            )
        )
    ]
    if not terminal_candidates:
        raise ValueError(
            "comparison-scoped lifecycle caller lacks one exact terminal "
            "instruction before the next declared function"
        )
    decoded_end = max(end for end, _instruction in terminal_candidates)
    terminal_rows = [
        instruction
        for end, instruction in terminal_candidates
        if end == decoded_end
    ]
    trailing_rows = [
        instruction
        for address, _end, instruction in body_rows
        if address >= decoded_end
    ]
    if len(terminal_rows) != 1 or any(
        not instruction.bytes
        or any(int(value, 16) != 0x90 for value in instruction.bytes)
        or _cc_cfg._instruction_mnemonic(instruction) != "nop"
        for instruction in trailing_rows
    ):
        raise ValueError(
            "comparison-scoped lifecycle caller has ambiguous terminal or "
            "non-NOP decoded seam"
        )
    if decoded_end == next_boundary:
        return

    tail = _cc_identity._immutable_retail_interval(
        reference,
        start=decoded_end,
        end_exclusive=next_boundary,
    )
    if not tail or any(value != 0x90 for value in tail):
        raise ValueError(
            "comparison-scoped lifecycle caller residual retail tail is not "
            "complete NOP padding"
        )


def _r4564_wol_release_retired_scalar_deleting_destructor_proof(
    candidate: CandidateAssembly,
) -> None:
    """Authenticate the P1-inlined ``delete this`` Release artifact.

    Under the governed P1 build, VC5 inlines the scalar-deleting destructor
    into the retail sequence: saved receiver -> ordinary destructor -> cdecl
    operator delete.  This proof retires the obsolete ``??_G`` call-site
    selector without inventing an identity bridge; the ordinary destructor and
    delete calls continue through the normal retail comparison.
    """

    def reject(detail: str) -> NoReturn:
        raise _cc_errors.CandidateCallContractEvidenceError(
            "r4564 WOL Release retired scalar-deleting-destructor bridge "
            "requires " + detail
        )

    caller = candidate.caller_definition
    natural_data = bytes.fromhex(
        "56 8b 74 24 08 57 8d 46 04 50 ff 15 00 00 00 00 "
        "8b f8 85 ff 75 14 85 f6 74 10 8b ce e8 00 00 00 00 "
        "56 e8 00 00 00 00 83 c4 04 8b c7 5f 5e c2 04 00"
    )
    expected_data = natural_data + b"\x90" * (0x40 - len(natural_data))
    relocation_specs = (
        (0x0C, IMAGE_REL_I386_DIR32, "__imp__InterlockedDecrement@4"),
        (
            0x1D,
            IMAGE_REL_I386_REL32,
            "??1WestwoodOnlineUpgradeDownloadEventSink@@QAE@XZ",
        ),
        (0x23, IMAGE_REL_I386_REL32, "??3@YAXPAX@Z"),
    )
    expected_mask = tuple(
        any(offset <= index < offset + 4 for offset, _kind, _name in relocation_specs)
        for index in range(len(expected_data))
    )
    if (
        caller is None
        or caller.symbol
        != "?Release@WestwoodOnlineUpgradeDownloadEventSink@@UAGKXZ"
        or caller.section_start != 0
        or caller.section_end != len(expected_data)
        or caller.data != expected_data
        or caller.relocation_mask != expected_mask
        or tuple(
            (row.offset, row.type, row.symbol_name)
            for row in caller.relocations
        )
        != relocation_specs
    ):
        reject(
            "the exact complete 64-byte P1 COMDAT, 49-byte natural body, "
            "padding, and relocations"
        )

    offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    if (
        len(offsets) != len(candidate.instructions)
        or any(offset is None for offset in offsets)
        or b"".join(
            bytes(int(item, 16) for item in instruction.bytes)
            for instruction in candidate.instructions
        )
        != natural_data
    ):
        reject("one complete contiguous COD instruction population")
    index_by_offset = {
        int(offset): index for index, offset in enumerate(offsets)
        if offset is not None
    }
    exact_operands = {
        0x01: "esi, dword _this$[esp]",
        0x06: "eax, dword [esi+4]",
        0x09: "eax",
        0x0A: "dword __imp__InterlockedDecrement@4",
        0x1A: "ecx, esi",
        0x1C: "??1WestwoodOnlineUpgradeDownloadEventSink@@QAE@XZ",
        0x21: "esi",
        0x22: "??3@YAXPAX@Z",
        0x27: "esp, 4",
        0x2A: "eax, edi",
        0x2E: "4",
    }
    if any(
        offset not in index_by_offset
        or _cc_cfg._instruction_operand(candidate.instructions[index_by_offset[offset]]).strip()
        != operand
        for offset, operand in exact_operands.items()
    ):
        reject("the exact receiver, delete argument, cleanup, and return lineage")
    invocation_offsets = tuple(
        int(offsets[index])
        for index in _cc_callable_identity._candidate_static_invocation_indices(
            candidate,
            caller_start="0x0",
            caller_end_exclusive=hex(len(natural_data)),
        )
        if offsets[index] is not None
    )
    if invocation_offsets != (0x0A, 0x1C, 0x22):
        reject("the exact three-site invocation population")

    scalar_name = (
        "??_GWestwoodOnlineUpgradeDownloadEventSink@@QAEPAXI@Z"
    )
    all_symbol_names = {
        *(row.symbol_name for row in caller.relocations),
        *(row.name for row in caller.coff_symbols),
        *caller.undefined_external_functions,
        *caller.defined_external_functions,
        *caller.undefined_external_data,
        *caller.defined_external_data,
    }
    if scalar_name in all_symbol_names or any(
        scalar_name in _cc_cfg._instruction_operand(instruction)
        for instruction in candidate.instructions
    ):
        reject("an exact zero scalar-deleting-destructor symbol population")


def _exact_zeroarg_lifecycle_row_name(
    candidate_name: str,
) -> str | None:
    """Map one exact ordinary VC5 lifecycle decoration to its row name."""
    constructor_name = _cc_lifecycle._exact_zeroarg_constructor_row_name(candidate_name)
    if constructor_name is not None:
        return constructor_name
    constructor_name = _cc_lifecycle._exact_parameterized_constructor_row_name(
        candidate_name
    )
    if constructor_name is not None:
        return constructor_name
    constructor_name = _cc_lifecycle._exact_named_zeroarg_constructor_row_name(
        candidate_name
    )
    if constructor_name is not None:
        return constructor_name
    return _exact_zeroarg_destructor_row_name(candidate_name)


def _exact_zeroarg_destructor_row_name(
    candidate_name: str,
) -> str | None:
    """Map one exact VC5 complete destructor to ``Class::~Class``."""
    match = _cc_catalog.MSVC_EXACT_ZEROARG_DESTRUCTOR_RE.fullmatch(candidate_name)
    if match is None:
        return None
    encoded_parts = match.group("class_scope").split("@")
    class_identity = "::".join(reversed(encoded_parts))
    return f"{class_identity}::~{encoded_parts[0]}"


def _resolve_eh_array_destructor_provider_identity(
    *,
    document: ProgressDocument,
    indexes: IdentityIndexes,
) -> str:
    """Resolve ``??_M`` from the complete governed provider catalog.

    Candidate call order and retail expected rows are deliberately absent from
    this resolver.  The VC5 spelling identifies the compiler-lifecycle family;
    the unique tracker row, accepted provider index, and exact known extent
    establish its retail identity.  Any duplicate, malformed, or ungoverned
    catalog row fails closed instead of letting a caller/ordinal select one.
    """

    catalog_rows = [
        (symbol_id, symbol)
        for symbol_id, symbol in document.collection("symbols").items()
        if isinstance(symbol, Mapping)
        and symbol.get("navigation_name") == "MSVC_EH_ArrayDestructor"
    ]
    if len(catalog_rows) != 1:
        raise ValueError(
            "EH array-destructor bridge requires exactly one complete "
            "MSVC_EH_ArrayDestructor tracker catalog row"
        )
    provider_symbol_id, provider_symbol = catalog_rows[0]
    provider_identity = _cc_identity._symbol_identity(provider_symbol_id, provider_symbol)
    provider_addresses = sorted(
        address
        for address, identity in indexes.by_address.items()
        if identity == provider_identity
    )
    provider_address = normalize_address(
        str(provider_symbol.get("address", provider_symbol.get("start", "")))
    )
    provider_end = normalize_address(str(provider_symbol.get("end_exclusive", "")))
    provider_size = address_value(provider_end) - address_value(provider_address)
    if (
        not provider_identity.startswith("provider:")
        or provider_identity not in indexes.provider_ids
        or provider_addresses != [provider_address]
        or provider_symbol.get("pipeline_class") != "non-authored"
        or provider_symbol.get("authored_order_role")
        != "compiler-generated-eh-helper"
        or provider_symbol.get("kind")
        not in {"function", "provider-function", "compiler-function"}
        or provider_size <= 0
        or provider_symbol.get("size") != provider_size
        or provider_symbol.get("extent_state") != "known"
    ):
        raise ValueError(
            "EH array-destructor bridge catalog row lacks one exact governed "
            "provider identity and known retail extent"
        )
    return provider_identity


def _prove_eh_array_destructor_provider_bridge(
    provider_identity: str,
    *,
    invocation_offsets: Sequence[int],
    document: ProgressDocument,
    candidate: CandidateAssembly,
    indexes: IdentityIndexes,
    bridge: BinaryNinjaBridge,
) -> None:
    if provider_identity not in indexes.provider_ids:
        raise ValueError(
            "EH array-destructor bridge target "
            f"{provider_identity!r} is not a governed provider identity"
        )
    provider_addresses = sorted(
        address
        for address, identity in indexes.by_address.items()
        if identity == provider_identity
    )
    if len(provider_addresses) != 1 or not provider_identity.startswith("provider:"):
        raise ValueError(
            "EH array-destructor bridge provider identity must resolve to one "
            "retail address"
        )
    provider_address = provider_addresses[0]
    provider_symbol_id = provider_identity.removeprefix("provider:")
    provider_symbol = document.collection("symbols").get(provider_symbol_id)
    if (
        not isinstance(provider_symbol, Mapping)
        or _cc_identity._symbol_identity(provider_symbol_id, provider_symbol) != provider_identity
        or provider_symbol.get("pipeline_class") != "non-authored"
        or provider_symbol.get("authored_order_role")
        != "compiler-generated-eh-helper"
        or provider_symbol.get("kind")
        not in {"function", "provider-function", "compiler-function"}
        or provider_symbol.get("navigation_name") != "MSVC_EH_ArrayDestructor"
        or normalize_address(
            str(provider_symbol.get("address", provider_symbol.get("start", "")))
        )
        != provider_address
    ):
        raise ValueError(
            "EH array-destructor bridge target lacks one exact governed "
            "MSVC_EH_ArrayDestructor tracker function"
        )
    provider_end = normalize_address(str(provider_symbol.get("end_exclusive", "")))
    provider_size = address_value(provider_end) - address_value(provider_address)
    if (
        provider_size <= 0
        or provider_symbol.get("size") != provider_size
        or provider_symbol.get("extent_state") != "known"
    ):
        raise ValueError(
            "EH array-destructor bridge provider lacks one exact known extent"
        )

    definition = candidate.caller_definition
    if (
        definition is None
        or definition.undefined_external_functions.count(
            _cc_catalog.MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL
        )
        != 1
    ):
        raise ValueError(
            "EH array-destructor bridge requires the exact undefined VC5 "
            "external function symbol"
        )
    relocations = tuple(
        relocation
        for relocation in definition.relocations
        if relocation.symbol_name == _cc_catalog.MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL
    )
    if (
        len(relocations) != len(invocation_offsets)
        or any(relocation.type != IMAGE_REL_I386_REL32 for relocation in relocations)
        or tuple(relocation.offset for relocation in relocations)
        != tuple(offset + 1 for offset in invocation_offsets)
    ):
        raise ValueError(
            "EH array-destructor bridge requires one exact REL32 relocation "
            "per candidate invocation; every row must be site-bound REL32"
        )
    for relocation in relocations:
        if (
            relocation.offset <= 0
            or relocation.offset + 4 > len(definition.data)
            or definition.data[relocation.offset - 1] != 0xE8
            or struct.unpack_from("<I", definition.data, relocation.offset)[0]
            != 0
            or not all(
                definition.relocation_mask[index]
                for index in range(relocation.offset, relocation.offset + 4)
            )
        ):
            raise ValueError(
                "EH array-destructor bridge relocation is not one complete "
                "direct-call displacement"
            )

    retail_bytes = _cc_cfg._hexdump_bytes(bridge.hexdump(provider_address, provider_size))
    if len(retail_bytes) != provider_size:
        raise ValueError(
            "EH array-destructor bridge retail provider returned an incomplete body"
        )
    retail_instructions = _cc_listing.parse_assembly(
        bridge.assembly(provider_address),
        source="bn",
    )
    if (
        not retail_instructions
        or _cc_cfg._instruction_mnemonic(retail_instructions[-1]) not in {"ret", "retn"}
        or _cc_cfg._instruction_operand(retail_instructions[-1]) not in {"0x10", "16"}
    ):
        raise ValueError(
            "EH array-destructor bridge retail provider lacks the exact "
            "callee-cleaned four-argument return"
        )


def _eh_array_destructor_provider_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    bridge: BinaryNinjaBridge,
    candidate_storage_bridges: Mapping[str, str] | None = None,
) -> dict[str, str]:
    """Bridge exact candidate ``??_M`` calls without consulting retail rows.

    The returned identity comes only from the complete governed provider
    catalog.  Expected invocation rows remain solely the later comparison
    oracle; changing their ordinal, kind, or target cannot influence this
    candidate identity acquisition.
    """

    del expected, candidate_storage_bridges
    helper_indices = tuple(
        index
        for index, instruction in enumerate(candidate.instructions)
        if _cc_cfg._instruction_mnemonic(instruction) in {"call", "jmp"}
        and (
            match := _cc_catalog.DECORATED_RE.search(_cc_cfg._instruction_operand(instruction))
        )
        is not None
        and match.group(0) == _cc_catalog.MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL
    )
    if not helper_indices:
        return {}
    reviewed_spec: ReviewedEhArrayDestructorCallSpec | None = None
    briefing_spec = _cc_catalog.BRIEFING_RUNTIME_EH_ARRAY_DESTRUCTOR_CALL_BRIDGE
    if caller_identity == briefing_spec.caller_identity and (
        normalize_address(caller_start) != briefing_spec.caller_start
        or normalize_address(caller_end_exclusive)
        != briefing_spec.caller_end_exclusive
    ):
        return {}
    if caller_identity == briefing_spec.caller_identity:
        reviewed_spec = _cc_retail._reviewed_eh_array_destructor_call_context(
            candidate,
            caller_identity=caller_identity,
            caller_start=caller_start,
            caller_end_exclusive=caller_end_exclusive,
        )
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start="0x0",
        caller_end_exclusive=hex(
            max(1, len(candidate.caller_definition.data))
            if candidate.caller_definition is not None else 1
        ),
    )
    invocation_ordinal_by_index = {
        instruction_index: ordinal
        for ordinal, instruction_index in enumerate(invocation_indices)
    }
    if (
        any(
            _cc_cfg._instruction_mnemonic(candidate.instructions[index]) != "call"
            or index in candidate.local_control_flow_indices
            or index not in invocation_ordinal_by_index
            for index in helper_indices
        )
        or len(set(helper_indices)) != len(helper_indices)
    ):
        raise ValueError(
            "EH array-destructor bridge requires only exact nonlocal direct "
            "candidate call instructions"
        )
    helper_ordinals = tuple(
        invocation_ordinal_by_index[index] for index in helper_indices
    )
    if reviewed_spec is not None and (
        len(helper_indices) != reviewed_spec.invocation_count
        or helper_ordinals != (reviewed_spec.ordinal,)
    ):
        raise ValueError(
            f"{reviewed_spec.label} requires candidate invocation ordinal "
            f"{reviewed_spec.ordinal} and count {reviewed_spec.invocation_count}"
        )
    instruction_offsets = _cc_callable_identity._candidate_complete_instruction_offsets(candidate)
    if any(instruction_offsets[index] is None for index in helper_indices):
        raise ValueError(
            "EH array-destructor bridge candidate COD call population is not "
            "one complete site set"
        )
    invocation_offsets = tuple(
        int(instruction_offsets[index]) for index in helper_indices
    )
    provider_identity = _resolve_eh_array_destructor_provider_identity(
        document=document,
        indexes=indexes,
    )
    indexed_identity = indexes.by_candidate_name.get(
        _cc_catalog.MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL
    )
    if indexed_identity not in {None, "", provider_identity}:
        raise ValueError(
            "EH array-destructor bridge candidate-name index conflicts with "
            "the unique governed provider identity"
        )
    raw_matches = bridge_names.get(_cc_catalog.MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL)
    matches = (
        list(raw_matches)
        if isinstance(raw_matches, (list, tuple, set, frozenset))
        else ([raw_matches] if raw_matches is not None else [])
    )
    if matches:
        provider_addresses = {
            address
            for address, identity in indexes.by_address.items()
            if identity == provider_identity
        }
        bridge_addresses = {
            normalize_address(str(getattr(item, "address", "")))
            for item in matches
            if getattr(item, "address", "")
        }
        if bridge_addresses != provider_addresses:
            raise ValueError(
                "EH array-destructor bridge Binary Ninja name population "
                "conflicts with the unique governed provider address"
            )
    _prove_eh_array_destructor_provider_bridge(
        provider_identity,
        invocation_offsets=invocation_offsets,
        document=document,
        candidate=candidate,
        indexes=indexes,
        bridge=bridge,
    )
    result = {_cc_catalog.MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL: provider_identity}
    if reviewed_spec is not None:
        result.update(
            _cc_retail._reviewed_eh_array_destructor_operand_bridges(
                candidate,
                provider_identity,
            )
        )
    return result


def _preflight_eh_array_destructor_candidate_identity(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    bridge: BinaryNinjaBridge,
) -> tuple[dict[str, str], IdentityIndexes]:
    """Install the generic ``??_M`` result before candidate preflights.

    The returned indexes are invocation-local.  Publishing the already-proven
    provider identity there ensures every downstream bridge producer sees the
    same exact compiler-lifecycle authority, including producers that perform
    their own complete candidate-contract preflight before final extraction.
    """

    bridges = _eh_array_destructor_provider_bridges(
        expected,
        candidate,
        document=document,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge_names=bridge_names,
        bridge=bridge,
    )
    if not bridges:
        return {}, indexes
    provider_identity = bridges.get(_cc_catalog.MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL, "")
    if (
        not provider_identity
        or provider_identity not in indexes.provider_ids
        or any(
            identity != provider_identity
            for identity in bridges.values()
        )
    ):
        raise ValueError(
            "EH array-destructor preflight produced an incomplete or "
            "conflicting provider identity population"
        )
    by_candidate_name = dict(indexes.by_candidate_name)
    prior = by_candidate_name.get(_cc_catalog.MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL)
    if prior not in {None, "", provider_identity}:
        raise ValueError(
            "EH array-destructor preflight conflicts with the candidate-name "
            "identity index"
        )
    by_candidate_name[_cc_catalog.MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL] = provider_identity
    return bridges, replace(indexes, by_candidate_name=by_candidate_name)


def _assert_eh_array_destructor_candidate_identity_stage(
    bridges: Mapping[str, str],
    indexes: IdentityIndexes,
    *,
    stage: str,
    final_bridges: Mapping[str, str] | None = None,
) -> None:
    """Fail at the first live producer boundary that drops proven ``??_M``.

    This is deliberately an invariant over the invocation-local result, not a
    second resolver.  Empty bridge sets remain the common no-op path.  Once the
    exact compiler-lifecycle proof exists, every provisional producer and the
    final extractor must retain the same governed provider identity.
    """

    if not bridges:
        return
    provider_identity = bridges.get(_cc_catalog.MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL, "")
    if (
        not provider_identity
        or provider_identity not in indexes.provider_ids
        or indexes.by_candidate_name.get(_cc_catalog.MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL)
        != provider_identity
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "EH array-destructor identity was discarded at live candidate "
            f"stage {stage!r}"
        )
    if final_bridges is not None and any(
        final_bridges.get(name) != identity
        for name, identity in bridges.items()
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "EH array-destructor bridge was discarded at live candidate "
            f"stage {stage!r}"
        )


def _eh_array_destructor_callback_noncall_relocation_offsets(
    candidate: CandidateAssembly,
    *,
    indexes: IdentityIndexes,
) -> dict[str, frozenset[int]]:
    """Prove exact ``??_M`` destructor-callback argument relocations.

    VC5 passes the ordinary destructor as the first pushed argument to every
    EH array-destructor helper call.  Those DIR32 references share a symbol
    with ordinary direct destructor calls, so the ordinal-import producer must
    classify them independently instead of counting them as calls.
    """

    provider_identity = indexes.by_candidate_name.get(
        _cc_catalog.MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL, ""
    )
    if not provider_identity:
        return {}
    if provider_identity not in indexes.provider_ids:
        raise _cc_errors.CandidateCallContractEvidenceError(
            "EH array-destructor callback proof lacks a governed provider identity"
        )
    definition = candidate.caller_definition
    if definition is None:
        raise _cc_errors.CandidateCallContractEvidenceError(
            "EH array-destructor callback proof requires one exact selected "
            "caller definition interval"
        )
    definition_start, _definition_end = _cc_callable_identity._candidate_caller_definition_interval(
        definition
    )

    helper_indices = tuple(
        index
        for index, instruction in enumerate(candidate.instructions)
        if _cc_cfg._instruction_mnemonic(instruction) == "call"
        and _cc_cfg._instruction_operand(instruction).strip()
        == _cc_catalog.MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL
    )
    if not helper_indices:
        return {}
    if any(index in candidate.local_control_flow_indices for index in helper_indices):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "EH array-destructor callback proof rejects caller-local helper calls"
        )

    offsets_by_symbol: dict[str, set[int]] = {}
    previous_helper = -1
    for helper_index in helper_indices:
        callback_pushes: list[tuple[Instruction, str]] = []
        for instruction in candidate.instructions[previous_helper + 1 : helper_index]:
            if _cc_cfg._instruction_mnemonic(instruction) != "push":
                continue
            operand = _cc_cfg._instruction_operand(instruction).strip()
            prefix = "OFFSET FLAT:"
            if not operand.startswith(prefix):
                continue
            callback_symbol = operand[len(prefix) :]
            if callback_symbol.startswith("??1"):
                callback_pushes.append((instruction, callback_symbol))
        if len(callback_pushes) != 1:
            raise _cc_errors.CandidateCallContractEvidenceError(
                "EH array-destructor callback proof requires exactly one "
                "ordinary destructor PUSH argument per helper call"
            )
        callback_instruction, callback_symbol = callback_pushes[0]
        if (
            len(callback_instruction.bytes) != 5
            or tuple(value.lower() for value in callback_instruction.bytes)
            != ("68", "00", "00", "00", "00")
        ):
            raise _cc_errors.CandidateCallContractEvidenceError(
                "EH array-destructor callback proof requires exact zero-addend "
                "PUSH imm32 encoding"
            )
        folded_undefined = tuple(
            name
            for name in definition.undefined_external_functions
            if name.casefold() == callback_symbol.casefold()
        )
        folded_defined = tuple(
            name
            for name in definition.defined_external_functions
            if name.casefold() == callback_symbol.casefold()
        )
        if (
            (folded_undefined, folded_defined)
            not in (((callback_symbol,), ()), ((), (callback_symbol,)))
        ):
            raise _cc_errors.CandidateCallContractEvidenceError(
                "EH array-destructor callback proof requires one exact "
                "ordinary external destructor symbol"
            )
        field_offset = (
            _cc_callable_identity._candidate_instruction_section_offset(
                callback_instruction, definition
            )
            + 1
        )
        matching_relocations = tuple(
            relocation
            for relocation in definition.relocations
            if relocation.offset == field_offset
        )
        relative = field_offset - definition_start
        if (
            len(matching_relocations) != 1
            or matching_relocations[0].type != IMAGE_REL_I386_DIR32
            or matching_relocations[0].symbol_name != callback_symbol
            or relative <= 0
            or relative + 4 > len(definition.data)
            or definition.data[relative - 1] != 0x68
            or struct.unpack_from("<I", definition.data, relative)[0] != 0
            or definition.relocation_mask[relative - 1]
            or not all(
                definition.relocation_mask[index]
                for index in range(relative, relative + 4)
            )
        ):
            raise _cc_errors.CandidateCallContractEvidenceError(
                "EH array-destructor callback proof requires one exact "
                "site-bound zero-addend DIR32 relocation"
            )
        offsets_by_symbol.setdefault(callback_symbol, set()).add(field_offset)
        previous_helper = helper_index
    if sum(len(offsets) for offsets in offsets_by_symbol.values()) != len(
        helper_indices
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "EH array-destructor callback proof has duplicate callback fields"
        )
    return {
        symbol: frozenset(offsets)
        for symbol, offsets in offsets_by_symbol.items()
    }


def _prove_compiler_destructor_provider_bridge(
    candidate_name: str,
    provider_identity: str,
    *,
    document: ProgressDocument,
    candidate: CandidateAssembly,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    bridge: BinaryNinjaBridge,
) -> None:
    if provider_identity not in indexes.provider_ids:
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} target "
            f"{provider_identity!r} is not a governed provider identity"
        )
    provider_addresses = sorted(
        address
        for address, identity in indexes.by_address.items()
        if identity == provider_identity
    )
    if len(provider_addresses) != 1:
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} provider identity "
            f"{provider_identity!r} must resolve to one retail address"
        )
    provider_address = provider_addresses[0]
    if not provider_identity.startswith("provider:"):
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} has malformed provider identity"
        )
    provider_symbol_id = provider_identity.removeprefix("provider:")
    provider_symbol = document.collection("symbols").get(provider_symbol_id)
    if (
        not isinstance(provider_symbol, Mapping)
        or _cc_identity._symbol_identity(provider_symbol_id, provider_symbol) != provider_identity
        or provider_symbol.get("pipeline_class") != "non-authored"
        or provider_symbol.get("kind")
        not in {"function", "provider-function", "compiler-function"}
        or normalize_address(
            str(provider_symbol.get("address", provider_symbol.get("start", "")))
        )
        != provider_address
    ):
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} target "
            f"{provider_identity!r} lacks one exact non-authored tracker function"
        )
    provider_end = normalize_address(str(provider_symbol.get("end_exclusive", "")))
    provider_size = address_value(provider_end) - address_value(provider_address)
    if provider_size <= 0 or provider_symbol.get("size") != provider_size:
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} provider "
            f"{provider_address} lacks one exact known extent"
        )

    target = candidate.target
    provider_rows = [
        row
        for row in _cc_retail._target_function_rows(target)
        if normalize_address(str(getattr(row, "address", ""))) == provider_address
    ]
    if len(provider_rows) != 1:
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} provider "
            f"{provider_address} must resolve to one exact selected-target row"
        )
    provider_row = provider_rows[0]
    provider_name = str(getattr(provider_row, "symbol", ""))
    if (
        _cc_catalog.MSVC_COMPLETE_DESTRUCTOR_RE.fullmatch(provider_name) is None
        or provider_name == candidate_name
        or getattr(provider_row, "provenance", "") != "provider-boundary"
        or getattr(provider_row, "pipeline_class", "") != "non-authored"
        or getattr(provider_row, "authored_order_role", "") != "non-authored"
        or not bool(getattr(provider_row, "required_presence", False))
        or not bool(getattr(provider_row, "full_order_gate", False))
    ):
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} target row at "
            f"{provider_address} is not an exact required provider destructor"
        )

    definition = candidate.complete_destructor_definitions.get(candidate_name)
    if definition is None:
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} has no same-object "
            "complete-destructor definition"
        )
    if (
        not definition.section_is_comdat
        or definition.comdat_selection != 2
        or definition.section_external_functions != (candidate_name,)
        or len(definition.data) != definition.section_size
        or definition.section_size != provider_size
    ):
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} is not one exact "
            "full-section VC5 COMDAT SELECT_ANY definition"
        )
    if (
        not definition.relocations
        or any(
            relocation.type != IMAGE_REL_I386_REL32
            for relocation in definition.relocations
        )
    ):
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} must contain only "
            "non-empty REL32 invocation relocations"
        )
    expected_mask: set[int] = set()
    for relocation in definition.relocations:
        if relocation.offset < 0 or relocation.offset + 4 > len(definition.data):
            raise ValueError(
                f"compiler destructor bridge {candidate_name!r} has an "
                "out-of-range REL32 relocation"
            )
        expected_mask.update(range(relocation.offset, relocation.offset + 4))
    actual_mask = {
        index for index, masked in enumerate(definition.relocation_mask) if masked
    }
    if actual_mask != expected_mask:
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} relocation mask "
            "does not exactly cover its REL32 fields"
        )

    retail_bytes = _cc_cfg._hexdump_bytes(bridge.hexdump(provider_address, provider_size))
    if len(retail_bytes) != provider_size:
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} retail target "
            f"{provider_address} returned an incomplete body"
        )
    mismatches = [
        index
        for index, (candidate_byte, retail_byte) in enumerate(
            zip(definition.data, retail_bytes)
        )
        if not definition.relocation_mask[index] and candidate_byte != retail_byte
    ]
    if mismatches:
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} does not byte-match "
            f"provider {provider_address} outside REL32 fields"
        )

    retail_instructions = _cc_listing.parse_assembly(
        bridge.assembly(provider_address),
        source="bn",
    )
    retail_switch_targets = _cc_cfg.retail_local_switch_targets(
        retail_instructions,
        caller_start=provider_address,
        caller_end_exclusive=provider_end,
        bridge=bridge,
    )
    retail_contract = _cc_extraction.extract_invocation_contract(
        retail_instructions,
        source="bn",
        caller_identity=provider_identity,
        caller_start=provider_address,
        caller_end_exclusive=provider_end,
        indexes=indexes,
        bridge_names=bridge_names,
        local_control_flow_indices=frozenset(retail_switch_targets),
        local_control_flow_targets=retail_switch_targets,
    )
    candidate_contract = _cc_extraction.extract_invocation_contract(
        definition.instructions,
        source="cod",
        caller_identity=provider_identity,
        caller_start=provider_address,
        caller_end_exclusive=provider_end,
        indexes=indexes,
        bridge_names=bridge_names,
        local_control_flow_indices=definition.local_control_flow_indices,
        local_control_flow_targets=definition.local_control_flow_targets,
    )
    if (
        len(candidate_contract) != len(definition.relocations)
        or any(row.get("dispatch") != "direct" for row in candidate_contract)
        or _cc_comparison.compare_call_contracts(retail_contract, candidate_contract)["passed"]
        is not True
    ):
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} relocation target "
            f"contract does not exactly match provider {provider_address}"
        )


def _complete_destructor_authored_target_names(
    candidate_name: str,
) -> tuple[str, str]:
    match = _cc_catalog.MSVC_COMPLETE_DESTRUCTOR_PARTS_RE.fullmatch(candidate_name)
    if match is None:
        return ()
    encoded_scope = match.group("class_name")
    if not encoded_scope.endswith("@@"):
        return ()
    scope_parts = encoded_scope[:-2].split("@")
    if not scope_parts or any(
        re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", part) is None
        for part in scope_parts
    ):
        # Template and otherwise complex scopes need separate reviewed
        # decoration handling; do not guess a source-level class identity.
        return ()
    qualifier = match.group("qualifier")
    target_symbol = f"?DestructorCore@{encoded_scope}{qualifier}XXZ"
    navigation_name = (
        f"{'::'.join(reversed(scope_parts))}::DestructorCore"
    )
    return target_symbol, navigation_name


def _prove_zsnd_wave_data_generated_destructor_ordinary_body_bridge(
    candidate_name: str,
    target_identity: str,
    *,
    expected: Sequence[Mapping[str, Any]],
    document: ProgressDocument,
    candidate: CandidateAssembly,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    invocation_row: Mapping[str, Any],
) -> bool:
    """Prove only the reviewed zSndWaveData compiler-destructor thunk.

    The immutable retail row supplies the expected 0x4a5440 identity.  Exact
    target/tracker/source authority and the independently compiled caller plus
    generated COMDAT establish only why the candidate ``??1`` edge names that
    already-reviewed ordinary body.  This intentionally does not provide a
    class-name-derived destructor mapping for any other candidate.
    """

    if candidate_name != _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_CANDIDATE_SYMBOL:
        return False

    prefix = "zSnd wave-data generated destructor ordinary-body bridge"
    normalized_caller_start = normalize_address(caller_start)
    if (
        caller_identity != _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_CALLER_IDENTITY
        or normalized_caller_start != _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_CALLER_START
        or normalize_address(caller_end_exclusive)
        != _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_CALLER_END_EXCLUSIVE
        or indexes.by_address.get(normalized_caller_start) != caller_identity
        or caller_identity in indexes.provider_ids
    ):
        raise ValueError(
            f"{prefix} requires the exact reviewed caller identity and extent"
        )

    expected_row = (
        expected[_cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_CALL_ORDINAL]
        if len(expected) > _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_CALL_ORDINAL
        else None
    )
    exact_expected_row = {
        "ordinal": _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_CALL_ORDINAL,
        "form": "call",
        "dispatch": "direct",
        "identity_kind": "direct",
        "target_identity": _cc_catalog.ZSND_WAVE_DATA_ORDINARY_DESTRUCTOR_IDENTITY,
        "storage_identity": "",
        "slot_displacement": None,
        "cleanup_bytes": None,
    }
    exact_candidate_row = {
        **exact_expected_row,
        "target_identity": (
            "compiler-candidate-destructor:"
            + _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_CANDIDATE_SYMBOL
        ),
    }
    if (
        target_identity != _cc_catalog.ZSND_WAVE_DATA_ORDINARY_DESTRUCTOR_IDENTITY
        or not isinstance(expected_row, Mapping)
        or dict(expected_row) != exact_expected_row
        or dict(invocation_row) != exact_candidate_row
    ):
        raise ValueError(
            f"{prefix} requires one exact immutable retail row and matching "
            "candidate invocation shape"
        )

    target = candidate.target
    caller = candidate.caller_definition
    target_rows = [
        row
        for row in getattr(target, "functions", ())
        if normalize_address(str(getattr(row, "address", "")))
        == _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_CALLER_START
    ]
    if (
        target is None
        or getattr(target, "name", "")
        != _cc_catalog.ZSND_DESTROY_OWNED_DATA_TARGET_NAME
        or getattr(target, "target_binary", "") != "recoil"
        or Path(str(getattr(target, "manifest_path", ""))).resolve()
        != (
            REPO_ROOT / _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_CALLER_TARGET_MANIFEST
        ).resolve()
        or getattr(target, "source_from", "")
        != _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_CALLER_SOURCE_PATH
        or getattr(target, "check_translation_unit_function_order", False)
        is not True
        or len(target_rows) != 1
        or getattr(target_rows[0], "symbol", "")
        != _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_CALLER_SYMBOL
        or getattr(target_rows[0], "symbol_regex", None) is not None
        or getattr(target_rows[0], "pipeline_class", "") != "authored"
        or getattr(target_rows[0], "authored_order_role", "")
        != "authored-body"
        or getattr(target_rows[0], "required_presence", None) is not True
        or getattr(target_rows[0], "full_order_gate", None) is not True
        or caller is None
        or caller.symbol != _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_CALLER_SYMBOL
    ):
        raise ValueError(
            f"{prefix} requires the exact selected caller target, source, "
            "row, and candidate COFF symbol"
        )

    caller_target = document.collection("verification_targets").get(
        _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_CALLER_TARGET_ID
    )
    caller_registration = (
        caller_target.get("registration")
        if isinstance(caller_target, Mapping)
        else None
    )
    caller_tracker_rows = [
        row
        for row in _cc_identity._mapping_target_function_rows(caller_target)
        if row.get("symbol") is not None
        and normalize_address(str(row.get("address", "")))
        == _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_CALLER_START
    ]
    if (
        not isinstance(caller_target, Mapping)
        or caller_target.get("binary") != "recoil"
        or caller_target.get("kind") != "vc5"
        or caller_target.get("name") != _cc_catalog.ZSND_DESTROY_OWNED_DATA_TARGET_NAME
        or not isinstance(caller_registration, Mapping)
        or caller_registration.get("binary") != "recoil"
        or caller_registration.get("name")
        != _cc_catalog.ZSND_DESTROY_OWNED_DATA_TARGET_NAME
        or caller_registration.get("manifest_path")
        != _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_CALLER_TARGET_MANIFEST
        or caller_registration.get("source_from")
        != _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_CALLER_SOURCE_PATH
        or caller_registration.get("check_translation_unit_function_order")
        is not True
        or caller_registration.get("function_order_scope") != "authored"
        or len(caller_tracker_rows) != 2
        or any(
            row.get("symbol") != _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_CALLER_SYMBOL
            or row.get("symbol_regex") is not None
            or row.get("name")
            != "zSndSample::CreateQueuedStreamingSample"
            or row.get("pipeline_class") != "authored"
            or row.get("authored_order_role") != "authored-body"
            or row.get("authored_order_gate") is not True
            or row.get("authored_relative_order_gate") is not True
            or row.get("required_presence") is not True
            or row.get("full_order_gate") is not True
            for row in caller_tracker_rows
        )
    ):
        raise ValueError(
            f"{prefix} requires one synchronized current caller target authority"
        )

    symbol_id = _cc_catalog.ZSND_WAVE_DATA_ORDINARY_DESTRUCTOR_IDENTITY.removeprefix(
        "symbol:"
    )
    symbol = document.collection("symbols").get(symbol_id)
    trace = (
        symbol.get("source_traceability")
        if isinstance(symbol, Mapping)
        else None
    )
    source_edges = (
        trace.get("source_edges") if isinstance(trace, Mapping) else None
    )
    verification_target_ids = (
        symbol.get("verification_target_ids")
        if isinstance(symbol, Mapping)
        else None
    )
    if (
        not isinstance(symbol, Mapping)
        or _cc_identity._symbol_identity(symbol_id, symbol)
        != _cc_catalog.ZSND_WAVE_DATA_ORDINARY_DESTRUCTOR_IDENTITY
        or symbol.get("binary") != "recoil"
        or symbol.get("kind") != "function"
        or symbol.get("pipeline_class") != "authored"
        or symbol.get("ownership_state") != "primary-owned"
        or symbol.get("address")
        != _cc_catalog.ZSND_WAVE_DATA_ORDINARY_DESTRUCTOR_ADDRESS
        or symbol.get("end_exclusive")
        != _cc_catalog.ZSND_WAVE_DATA_ORDINARY_DESTRUCTOR_END_EXCLUSIVE
        or symbol.get("size") != 0x20
        or symbol.get("extent_state") != "known"
        or symbol.get("navigation_name")
        != _cc_catalog.ZSND_WAVE_DATA_ORDINARY_DESTRUCTOR_NAVIGATION_NAME
        or symbol.get("output_section_id") != "recoil:section:.text"
        or symbol.get("physical_block_id") != "recoil:block:0x4a53f0"
        or not isinstance(verification_target_ids, list)
        or len(verification_target_ids) != len(set(verification_target_ids))
        or _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_ORDER_TARGET_ID
        not in verification_target_ids
        or _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_FOCUSED_TARGET_ID
        not in verification_target_ids
        or not isinstance(trace, Mapping)
        or trace.get("state") != "resolved"
        or trace.get("reason_code") not in {None, ""}
        or not isinstance(source_edges, list)
        or len(source_edges) != 1
        or not isinstance(source_edges[0], Mapping)
        or source_edges[0].get("relation") != "defines"
        or source_edges[0].get("anchor_id")
        != _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_SOURCE_ANCHOR_ID
        or source_edges[0].get("emission_context")
        != {"translation_unit": _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_SOURCE_PATH}
        or indexes.by_address.get(
            _cc_catalog.ZSND_WAVE_DATA_ORDINARY_DESTRUCTOR_ADDRESS
        )
        != _cc_catalog.ZSND_WAVE_DATA_ORDINARY_DESTRUCTOR_IDENTITY
        or indexes.by_candidate_name.get(
            _cc_catalog.ZSND_WAVE_DATA_ORDINARY_DESTRUCTOR_SYMBOL
        )
        != _cc_catalog.ZSND_WAVE_DATA_ORDINARY_DESTRUCTOR_IDENTITY
        or indexes.by_candidate_name.get(
            _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_CORE_SYMBOL
        )
        is not None
    ):
        raise ValueError(
            f"{prefix} requires the exact reviewed ordinary-body tracker, "
            "source-anchor, and candidate-name identities"
        )

    order_target = document.collection("verification_targets").get(
        _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_ORDER_TARGET_ID
    )
    order_registration = (
        order_target.get("registration")
        if isinstance(order_target, Mapping)
        else None
    )
    order_rows = [
        row
        for row in _cc_identity._mapping_target_function_rows(order_target)
        if row.get("symbol") is not None
        and normalize_address(str(row.get("address", "")))
        == _cc_catalog.ZSND_WAVE_DATA_ORDINARY_DESTRUCTOR_ADDRESS
    ]
    focused_target = document.collection("verification_targets").get(
        _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_FOCUSED_TARGET_ID
    )
    focused_registration = (
        focused_target.get("registration")
        if isinstance(focused_target, Mapping)
        else None
    )
    if (
        not isinstance(order_target, Mapping)
        or order_target.get("binary") != "recoil"
        or order_target.get("kind") != "vc5"
        or order_target.get("name")
        != _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_ORDER_TARGET_NAME
        or not isinstance(order_registration, Mapping)
        or order_registration.get("binary") != "recoil"
        or order_registration.get("name")
        != _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_ORDER_TARGET_NAME
        or order_registration.get("manifest_path")
        != _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_ORDER_TARGET_MANIFEST
        or order_registration.get("source_from")
        != _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_SOURCE_PATH
        or order_registration.get("check_translation_unit_function_order")
        is not True
        or order_registration.get("function_order_scope") != "authored"
        or len(order_rows) != 2
        or any(
            row.get("symbol")
            != _cc_catalog.ZSND_WAVE_DATA_ORDINARY_DESTRUCTOR_SYMBOL
            or row.get("symbol_regex") is not None
            or row.get("name")
            != _cc_catalog.ZSND_WAVE_DATA_ORDINARY_DESTRUCTOR_NAVIGATION_NAME
            or row.get("pipeline_class") != "authored-lifecycle"
            or row.get("authored_order_role")
            != "authored-lifecycle-body"
            or row.get("authored_order_gate") is not True
            or row.get("authored_relative_order_gate") is not True
            or row.get("required_presence") is not True
            or row.get("full_order_gate") is not True
            or row.get("logical_identity_key") not in {None, ""}
            or row.get("icf_fold_status") not in {None, ""}
            for row in order_rows
        )
        or not isinstance(focused_target, Mapping)
        or focused_target.get("binary") != "recoil"
        or focused_target.get("kind") != "vc5"
        or focused_target.get("name")
        != _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_FOCUSED_TARGET_NAME
        or _cc_targets._registered_target_artifact_ids(
            focused_target, document.collection("symbols")
        ) != [symbol_id]
        or not isinstance(focused_registration, Mapping)
        or focused_registration.get("binary") != "recoil"
        or focused_registration.get("name")
        != _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_FOCUSED_TARGET_NAME
        or focused_registration.get("manifest_path")
        != _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_FOCUSED_TARGET_MANIFEST
        or focused_registration.get("source_from")
        != _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_SOURCE_PATH
        or focused_registration.get("function_addresses")
        != [_cc_catalog.ZSND_WAVE_DATA_ORDINARY_DESTRUCTOR_ADDRESS]
    ):
        raise ValueError(
            f"{prefix} requires synchronized authored-order and focused "
            "ordinary-body target authorities"
        )

    definition = candidate.complete_destructor_definitions.get(candidate_name)
    relocation = (
        definition.relocations[0]
        if definition is not None and len(definition.relocations) == 1
        else None
    )
    listing = (
        definition.instructions[0]
        if definition is not None and len(definition.instructions) == 1
        else None
    )
    associated = (
        definition.associated_sections[0]
        if definition is not None
        and len(definition.associated_sections) == 1
        else None
    )
    associated_relocation = (
        associated.relocations[0]
        if associated is not None and len(associated.relocations) == 1
        else None
    )
    if (
        definition is None
        or definition.symbol != candidate_name
        or not definition.section_is_comdat
        or definition.comdat_selection != 2
        or definition.section_external_functions != (candidate_name,)
        or definition.section_size != 0x10
        or definition.data
        != b"\xe9\x00\x00\x00\x00" + b"\x90" * 0x0B
        or definition.relocation_mask
        != (False, True, True, True, True) + (False,) * 0x0B
        or definition.local_control_flow_indices != frozenset()
        or bool(definition.local_control_flow_targets)
        or relocation is None
        or relocation.offset != 1
        or relocation.type != IMAGE_REL_I386_REL32
        or relocation.symbol_name
        != _cc_catalog.ZSND_WAVE_DATA_ORDINARY_DESTRUCTOR_SYMBOL
        or listing is None
        or _cc_cfg._source_instruction_address(listing) != "0x0"
        or _cc_cfg._instruction_mnemonic(listing) != "jmp"
        or _cc_cfg._instruction_operand(listing).strip()
        != _cc_catalog.ZSND_WAVE_DATA_ORDINARY_DESTRUCTOR_SYMBOL
        or tuple(value.lower() for value in listing.bytes)
        != ("e9", "00", "00", "00", "00")
        or associated is None
        or associated.section_index <= 0
        or associated.association_section_index <= 0
        or associated.section_index == associated.association_section_index
        or associated.name != ".debug$F"
        or associated.data
        != b"\x00\x00\x00\x00\x05" + b"\x00" * 0x0B
        or associated.relocation_mask
        != (True, True, True, True) + (False,) * 0x0C
        or associated.section_size != 0x10
        or not associated.section_is_comdat
        or associated.comdat_selection != 5
        or associated.section_external_symbols != ()
        or associated.symbols != ()
        or associated_relocation is None
        or associated_relocation.offset != 0
        or associated_relocation.type != 0x07
        or associated_relocation.symbol_name != candidate_name
    ):
        raise ValueError(
            f"{prefix} requires the exact padded VC5 SELECT_ANY COMDAT "
            "E9/REL32 thunk and associative debug section in synchronized "
            "COFF and COD"
        )

    _prove_compiler_destructor_authored_bridge(
        candidate_name,
        target_identity,
        document=document,
        candidate=candidate,
        indexes=indexes,
        invocation_row=invocation_row,
        zsnd_wave_data_ordinary_body_proven=True,
    )
    return True


def _prove_compiler_destructor_authored_bridge(
    candidate_name: str,
    target_identity: str,
    *,
    document: ProgressDocument,
    candidate: CandidateAssembly,
    indexes: IdentityIndexes,
    invocation_row: Mapping[str, Any],
    zsnd_wave_data_ordinary_body_proven: bool = False,
) -> None:
    target_names = _complete_destructor_authored_target_names(candidate_name)
    if not target_names:
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} has no exact "
            "simple VC5 class/destructor identity"
        )
    if zsnd_wave_data_ordinary_body_proven:
        if (
            candidate_name != _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_CANDIDATE_SYMBOL
            or target_names
            != (
                _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_CORE_SYMBOL,
                _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_CORE_NAVIGATION_NAME,
            )
        ):
            raise ValueError(
                "zSnd wave-data destructor finite bridge rejects a nonexact "
                "compiler destructor/DestructorCore identity"
            )
        target_names = (
            _cc_catalog.ZSND_WAVE_DATA_ORDINARY_DESTRUCTOR_SYMBOL,
            _cc_catalog.ZSND_WAVE_DATA_ORDINARY_DESTRUCTOR_NAVIGATION_NAME,
        )
    elif candidate_name == _cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_CANDIDATE_SYMBOL:
        raise ValueError(
            "zSnd wave-data destructor requires its finite ordinary-body proof"
        )
    target_symbol, navigation_name = target_names
    if target_identity.startswith("provider:") or not target_identity.startswith(
        "symbol:"
    ):
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} target "
            f"{target_identity!r} is not one authored function identity"
        )
    target_addresses = sorted(
        address
        for address, identity in indexes.by_address.items()
        if identity == target_identity
    )
    if len(target_addresses) != 1:
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} authored identity "
            f"{target_identity!r} must resolve to one retail address"
        )
    target_address = target_addresses[0]
    if indexes.by_candidate_name.get(target_symbol) != target_identity:
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} exact ordinary "
            f"DestructorCore symbol {target_symbol!r} does not resolve to "
            f"{target_identity!r}"
        )

    symbol_id = target_identity.removeprefix("symbol:")
    symbol = document.collection("symbols").get(symbol_id)
    if (
        not isinstance(symbol, Mapping)
        or _cc_identity._symbol_identity(symbol_id, symbol) != target_identity
        or symbol.get("binary") != "recoil"
        or symbol.get("kind") != "function"
        or symbol.get("pipeline_class") != "authored"
        or symbol.get("extent_state") != "known"
        or symbol.get("navigation_name") != navigation_name
        or normalize_address(
            str(symbol.get("address", symbol.get("start", "")))
        )
        != target_address
        or indexes.by_address.get(target_address) != target_identity
    ):
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} target "
            f"{target_identity!r} lacks one exact reviewed authored "
            "DestructorCore tracker function"
        )
    target_end = normalize_address(str(symbol.get("end_exclusive", "")))
    target_size = address_value(target_end) - address_value(target_address)
    logical_aliases = symbol.get("logical_aliases")
    icf_group = symbol.get("icf_address_group")
    if (
        target_size <= 0
        or symbol.get("size") != target_size
        or symbol.get("logical_identity_key") not in {None, ""}
        or symbol.get("icf_fold_status") not in {None, ""}
        or isinstance(logical_aliases, Mapping)
        and bool(logical_aliases)
        or isinstance(icf_group, Mapping)
        and bool(icf_group)
    ):
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} authored target "
            f"{target_address} lacks one exact unaliased known extent"
        )

    verification_target_ids = symbol.get("verification_target_ids")
    if (
        not isinstance(verification_target_ids, list)
        or not verification_target_ids
        or any(
            not isinstance(target_id, str) or not target_id
            for target_id in verification_target_ids
        )
        or len(set(verification_target_ids)) != len(verification_target_ids)
    ):
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} authored target "
            f"{target_address} has missing or duplicate verification authority"
        )
    reviewed_verification_target_ids = (
        (_cc_catalog.ZSND_WAVE_DATA_DESTRUCTOR_ORDER_TARGET_ID,)
        if zsnd_wave_data_ordinary_body_proven
        else tuple(verification_target_ids)
    )
    qualifying_target_ids: set[str] = set()
    for target_id in reviewed_verification_target_ids:
        target = document.collection("verification_targets").get(target_id)
        if not isinstance(target, Mapping):
            continue
        selected_rows: list[Mapping[str, Any]] = []
        for row in _cc_identity._mapping_target_function_rows(target):
            raw_address = row.get("address")
            if not isinstance(raw_address, str):
                continue
            try:
                row_address = normalize_address(raw_address)
            except ProgressError:
                continue
            if row_address != target_address or (
                row.get("symbol") is None
                and row.get("symbol_regex") is None
            ):
                continue
            selected_rows.append(row)
        if not selected_rows:
            continue
        if any(
            row.get("symbol") != target_symbol
            or row.get("symbol_regex") not in {None, ""}
            or row.get("name") != navigation_name
            or row.get("pipeline_class") != "authored-lifecycle"
            or row.get("authored_order_role")
            != "authored-lifecycle-body"
            or row.get("authored_order_gate") is not True
            or row.get("authored_relative_order_gate") is not True
            or row.get("required_presence") is not True
            or row.get("full_order_gate") is not True
            or row.get("logical_identity_key") not in {None, ""}
            or row.get("icf_fold_status") not in {None, ""}
            for row in selected_rows
        ):
            raise ValueError(
                f"compiler destructor bridge {candidate_name!r} authored target "
                f"{target_address} has a conflicting selected destructor row"
            )
        qualifying_target_ids.add(target_id)
    if len(qualifying_target_ids) != 1:
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} authored target "
            f"{target_address} must have one exact selected destructor authority"
        )

    caller = candidate.caller_definition
    exact_caller_symbol_counts = (
        (0, 1) if zsnd_wave_data_ordinary_body_proven else (1, 0)
    )
    if (
        caller is None
        or caller.undefined_external_functions.count(candidate_name)
        != exact_caller_symbol_counts[0]
        or caller.defined_external_functions.count(candidate_name)
        != exact_caller_symbol_counts[1]
    ):
        if not zsnd_wave_data_ordinary_body_proven:
            raise ValueError(
                f"compiler destructor bridge {candidate_name!r} requires "
                "exactly one undefined and no defined caller-object function "
                "symbol"
            )
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} requires its "
            "exact reviewed same-object defined compiler-destructor symbol"
        )
    references = tuple(
        relocation
        for relocation in caller.relocations
        if relocation.symbol_name == candidate_name
    )
    if len(references) != 1:
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} requires exactly "
            "one caller-object relocation"
        )
    reference = references[0]
    if reference.type != IMAGE_REL_I386_REL32:
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} caller-object "
            "relocation is not REL32"
        )
    if (
        len(caller.relocation_mask) != len(caller.data)
        or reference.offset < 1
        or reference.offset + 4 > len(caller.data)
    ):
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} caller-object "
            "REL32 relocation is out of range"
        )
    if not all(
        caller.relocation_mask[index]
        for index in range(reference.offset, reference.offset + 4)
    ):
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} caller-object "
            "REL32 relocation is not fully masked"
        )
    if struct.unpack_from("<I", caller.data, reference.offset)[0] != 0:
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} caller-object "
            "REL32 relocation has a nonzero addend"
        )
    expected_opcode = {
        "call": 0xE8,
        "tail": 0xE9,
    }.get(str(invocation_row.get("form", "")))
    expected_mnemonic = {
        "call": "call",
        "tail": "jmp",
    }.get(str(invocation_row.get("form", "")))
    if (
        invocation_row.get("dispatch") != "direct"
        or expected_opcode is None
        or expected_mnemonic is None
        or caller.data[reference.offset - 1] != expected_opcode
    ):
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} caller-object "
            "opcode does not match its exact direct invocation form"
        )
    listing_matches: list[tuple[int, Instruction]] = []
    for index, instruction in enumerate(candidate.instructions):
        operand = _cc_cfg._instruction_operand(instruction).strip()
        match = _cc_catalog.DECORATED_RE.search(operand)
        if match is not None and match.group(0) == candidate_name:
            listing_matches.append((index, instruction))
    if len(listing_matches) != 1:
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} requires one exact "
            "candidate-name COD invocation instruction"
        )
    listing_index, listing_instruction = listing_matches[0]
    listing_operand = _cc_cfg._instruction_operand(listing_instruction).strip()
    listing_bytes = tuple(
        value.lower() for value in listing_instruction.bytes
    )
    if (
        listing_index in candidate.local_control_flow_indices
        or listing_operand != candidate_name
        or _cc_cfg._instruction_mnemonic(listing_instruction) != expected_mnemonic
        or not listing_bytes
        or listing_bytes[0] != f"{expected_opcode:02x}"
    ):
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} COD instruction "
            "is not the exact nonlocal direct invocation form"
        )
    listing_address = _cc_cfg._source_instruction_address(listing_instruction)
    if (
        not listing_address
        or address_value(listing_address) != reference.offset - 1
    ):
        raise ValueError(
            f"compiler destructor bridge {candidate_name!r} COD invocation "
            "offset does not match its caller-object REL32 relocation"
        )


def _compiler_destructor_provider_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    retail_instructions: Sequence[Instruction] = (),
    document: ProgressDocument,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
    bridge_names: Mapping[str, Any],
    bridge: BinaryNinjaBridge,
    compiler_generated_bridges: Mapping[str, str] | None = None,
    registered_regex_bridges: Mapping[str, str] | None = None,
    candidate_storage_bridges: Mapping[str, str] | None = None,
) -> dict[str, str]:
    base_bridges = dict(compiler_generated_bridges or {})
    exact_registered_bridges = dict(registered_regex_bridges or {})
    candidate_names = _cc_retail._candidate_unresolved_complete_destructors(
        candidate,
        indexes=indexes,
        bridge_names=bridge_names,
    )
    if (
        caller_identity == "symbol:recoil:function:0x42a000"
        and normalize_address(caller_start) == "0x42a000"
        and normalize_address(caller_end_exclusive) == "0x42a070"
    ):
        # The governed `/Ob0` zInput proof expands this complete local vector
        # destructor recursively.  Do not let same-ordinal generic destructor
        # arbitration borrow retail's operator-delete identity before that
        # finite helper graph has been authenticated.
        candidate_names = tuple(
            name for name in candidate_names
            if name
            != "??1?$vector@HV?$allocator@H@std@@@std@@QAE@XZ"
        )
    zsnd_vector_dtor_bridge = _cc_recoil_audio._zsnd_static_vector_dtor_candidate_bridge(
        expected,
        candidate,
        retail_instructions,
        caller_identity=caller_identity,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
        indexes=indexes,
        bridge=bridge,
    )
    if zsnd_vector_dtor_bridge:
        if set(candidate_names) != set(zsnd_vector_dtor_bridge):
            raise ValueError(
                "zSnd E4 vector-dtor bridge rejects extra or missing candidate destructor population"
            )
        return zsnd_vector_dtor_bridge
    if not candidate_names:
        return {}
    bridge_collisions = set(candidate_names) & set(base_bridges)
    generic_collisions = bridge_collisions - set(exact_registered_bridges)
    if generic_collisions:
        raise ValueError(
            "compiler destructor bridge collides with another reviewed bridge: "
            + ", ".join(repr(name) for name in sorted(generic_collisions))
        )
    if bridge_collisions:
        if any(
            base_bridges[name] != exact_registered_bridges.get(name)
            for name in bridge_collisions
        ):
            raise ValueError(
                "compiler destructor bridge registered-regex arbitration "
                "has conflicting supplied identities: "
                + ", ".join(
                    repr(name) for name in sorted(bridge_collisions)
                )
            )
        independently_proven = (
            _cc_dispatch._registered_target_symbol_regex_direct_candidate_bridges(
                candidate,
                document=document,
                indexes=indexes,
                bridge_names=bridge_names,
                compiler_generated_bridges={},
            )
        )
        if any(
            independently_proven.get(name)
            != exact_registered_bridges[name]
            for name in bridge_collisions
        ):
            raise ValueError(
                "compiler destructor bridge registered-regex arbitration "
                "lacks one independently proven identical lifecycle supplier: "
                + ", ".join(
                    repr(name) for name in sorted(bridge_collisions)
                )
            )
    provisional_identities = {
        name: f"compiler-candidate-destructor:{name}"
        for name in candidate_names
    }
    invocation_indices = _cc_callable_identity._candidate_static_invocation_indices(
        candidate,
        caller_start=caller_start,
        caller_end_exclusive=caller_end_exclusive,
    )
    invocation_ordinals = {
        index: ordinal
        for ordinal, index in enumerate(invocation_indices)
    }
    provisional: list[dict[str, Any]] = []
    for candidate_name, provisional_identity in provisional_identities.items():
        listing_indices = [
            index
            for index, instruction in enumerate(candidate.instructions)
            if (
                (match := _cc_catalog.DECORATED_RE.search(
                    _cc_cfg._instruction_operand(instruction).strip()
                ))
                is not None
                and match.group(0) == candidate_name
            )
        ]
        if (
            len(listing_indices) != 1
            or listing_indices[0] not in invocation_ordinals
        ):
            raise ValueError(
                f"compiler destructor bridge {candidate_name!r} requires one "
                "exact statically counted candidate invocation"
            )
        listing_index = listing_indices[0]
        instruction = candidate.instructions[listing_index]
        mnemonic = _cc_cfg._instruction_mnemonic(instruction)
        provisional.append(
            {
                "ordinal": invocation_ordinals[listing_index],
                "form": "call" if mnemonic == "call" else "tail",
                "dispatch": "direct",
                "identity_kind": "direct",
                "target_identity": provisional_identity,
                "storage_identity": "",
                "slot_displacement": None,
                "cleanup_bytes": _cc_cfg._cleanup_after(
                    candidate.instructions,
                    listing_index,
                ),
            }
        )
    resolved: dict[str, str] = {}
    for candidate_name, provisional_identity in provisional_identities.items():
        rows = [
            row
            for row in provisional
            if row.get("target_identity") == provisional_identity
        ]
        if not rows:
            raise ValueError(
                f"compiler destructor bridge {candidate_name!r} has no exact "
                "candidate invocation row"
            )
        if candidate_name in bridge_collisions:
            registered_identity = exact_registered_bridges[candidate_name]
            row = rows[0] if len(rows) == 1 else None
            if (
                row is None
                or _exact_zeroarg_destructor_row_name(candidate_name) is None
                or not registered_identity.startswith("symbol:")
                or registered_identity in indexes.provider_ids
                or row.get("form") not in {"call", "tail"}
                or row.get("dispatch") != "direct"
                or row.get("identity_kind") != "direct"
                or row.get("target_identity") != provisional_identity
                or row.get("storage_identity") != ""
                or row.get("slot_displacement") is not None
                or row.get("cleanup_bytes") is not None
            ):
                raise ValueError(
                    "compiler destructor bridge registered-regex arbitration "
                    f"rejects lifecycle decoration, direct authored identity, "
                    f"call/tail form, cleanup, or unique invocation for "
                    f"{candidate_name!r}"
                )
            resolved[candidate_name] = registered_identity
            continue
        target_identities: set[str] = set()
        target_modes: set[str] = set()
        for row in rows:
            ordinal = int(row["ordinal"])
            if ordinal >= len(expected):
                raise ValueError(
                    f"compiler destructor bridge {candidate_name!r} has no "
                    "retail invocation at the same ordinal"
                )
            expected_row = expected[ordinal]
            expected_identity = str(expected_row.get("target_identity", ""))
            is_provider = (
                expected_row.get("identity_kind") == "provider"
                and expected_identity in indexes.provider_ids
            )
            is_authored = (
                expected_row.get("identity_kind") == "direct"
                and expected_identity.startswith("symbol:")
                and expected_identity not in indexes.provider_ids
            )
            if (
                _cc_retail._contract_shape_without_target(expected_row)
                != _cc_retail._contract_shape_without_target(row)
                or not (is_provider or is_authored)
            ):
                raise ValueError(
                    f"compiler destructor bridge {candidate_name!r} is not the "
                    "same call form/dispatch/cleanup as one exact retail provider "
                    "or reviewed authored destructor"
                )
            target_identities.add(expected_identity)
            target_modes.add("provider" if is_provider else "authored")
        if len(target_identities) != 1 or len(target_modes) != 1:
            raise ValueError(
                f"compiler destructor bridge {candidate_name!r} maps to "
                "multiple or conflicting retail destructor identities"
            )
        target_identity = next(iter(target_identities))
        if target_modes == {"provider"}:
            _prove_compiler_destructor_provider_bridge(
                candidate_name,
                target_identity,
                document=document,
                candidate=candidate,
                indexes=indexes,
                bridge_names=bridge_names,
                bridge=bridge,
            )
        else:
            if target_identity in resolved.values():
                raise ValueError(
                    f"compiler destructor bridge authored target "
                    f"{target_identity!r} has duplicate candidate symbols"
                )
            if len(rows) != 1:
                raise ValueError(
                    f"compiler destructor bridge {candidate_name!r} authored "
                    "target requires one exact candidate invocation"
                )
            if not _cc_recoil_hud_lifetimes._prove_player_hud_ui_element_generated_destructor_bridge(
                candidate_name,
                target_identity,
                document=document,
                candidate=candidate,
                caller_identity=caller_identity,
                caller_start=caller_start,
                caller_end_exclusive=caller_end_exclusive,
                indexes=indexes,
                invocation_row=rows[0],
            ) and not _prove_zsnd_wave_data_generated_destructor_ordinary_body_bridge(
                candidate_name,
                target_identity,
                expected=expected,
                document=document,
                candidate=candidate,
                caller_identity=caller_identity,
                caller_start=caller_start,
                caller_end_exclusive=caller_end_exclusive,
                indexes=indexes,
                invocation_row=rows[0],
            ) and not _cc_recoil_hud_lifetimes._prove_hud_ui_string_menu_generated_destructor_bridge(
                candidate_name,
                target_identity,
                expected=expected,
                document=document,
                candidate=candidate,
                caller_identity=caller_identity,
                caller_start=caller_start,
                caller_end_exclusive=caller_end_exclusive,
                indexes=indexes,
                bridge_names=bridge_names,
                bridge=bridge,
                invocation_row=rows[0],
            ) and not _cc_recoil_hud_lifetimes._prove_hud_ui_shield_message_generated_destructor_bridge(
                candidate_name,
                target_identity,
                expected=expected,
                document=document,
                candidate=candidate,
                caller_identity=caller_identity,
                caller_start=caller_start,
                caller_end_exclusive=caller_end_exclusive,
                indexes=indexes,
                bridge_names=bridge_names,
                bridge=bridge,
                invocation_row=rows[0],
            ) and not _cc_recoil_hud_lifetimes._prove_hud_ui_message_stack_generated_destructor_bridge(
                candidate_name,
                target_identity,
                expected=expected,
                document=document,
                candidate=candidate,
                caller_identity=caller_identity,
                caller_start=caller_start,
                caller_end_exclusive=caller_end_exclusive,
                indexes=indexes,
                bridge_names=bridge_names,
                bridge=bridge,
                invocation_row=rows[0],
            ):
                _prove_compiler_destructor_authored_bridge(
                    candidate_name,
                    target_identity,
                    document=document,
                    candidate=candidate,
                    indexes=indexes,
                    invocation_row=rows[0],
                )
        resolved[candidate_name] = target_identity
    return resolved
