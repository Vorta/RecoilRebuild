"""Recoil call-contract candidate session evidence and checks."""

from __future__ import annotations

from copy import deepcopy
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Mapping, Sequence

from _recoil.call_contract import candidate as _cc_candidate
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import errors as _cc_errors
from _recoil.call_contract import listing as _cc_listing
from _recoil.call_contract import source as _cc_source
from _recoil.call_contract.records import (
    CandidateAssembly,
    CandidateConstructorDefinition,
)
from _recoil.commands.asm_verify import CoffObject
from _recoil.lib.progress import (
    CALL_CONTRACT_SLICE_MAX_BODIES,
    ProgressDocument,
    ProgressError,
    address_value,
    normalize_address,
)
from _recoil.lib.repository_paths import RepositoryPathInventory
from _recoil.lib.tooling import REPO_ROOT


@dataclass
class _LazyCallContractCandidateSession:
    """Compile one selected target when its first retail-ordered body is read."""

    document: ProgressDocument
    slice_row: Mapping[str, Any]
    build_root: Path
    vc5_env: Path
    preloaded_targets: Mapping[str, Any] | None = None
    precompiled_target_units: Mapping[
        str, tuple[Any, tuple[tuple[Any, Path, CoffObject], ...]]
    ] | None = None
    numeric_constructor_authority: tuple[
        Any,
        dict[str, CandidateConstructorDefinition],
        dict[str, CandidateConstructorDefinition],
        dict[str, CandidateConstructorDefinition],
    ] | None = None
    _target_for_address: dict[str, str] = field(init=False, default_factory=dict)
    _members_by_target: dict[str, tuple[tuple[str, str], ...]] = field(
        init=False,
        default_factory=dict,
    )
    _candidates: dict[str, CandidateAssembly] = field(
        init=False,
        default_factory=dict,
    )
    compiled_target_ids: list[str] = field(init=False, default_factory=list)
    attempted_target_ids: list[str] = field(init=False, default_factory=list)
    toolchain_receipts: list[dict[str, Any]] = field(
        init=False,
        default_factory=list,
    )
    _target_acquisition_failures: dict[str, Exception] = field(
        init=False,
        default_factory=dict,
    )
    _cod_listing_index: _cc_listing._CodListingInvocationIndex = field(
        init=False,
        default_factory=lambda: _cc_listing._CodListingInvocationIndex(),
    )

    def __post_init__(self) -> None:
        symbol_ids = self.slice_row.get("symbol_ids")
        addresses = self.slice_row.get("addresses")
        target_ids = self.slice_row.get("target_ids")
        if (
            not isinstance(symbol_ids, list)
            or not isinstance(addresses, list)
            or len(symbol_ids) != len(addresses)
            or not isinstance(target_ids, list)
        ):
            raise ValueError(
                "call-contract candidate session requires exact ordered slice membership"
            )
        selected_target_ids = {str(value) for value in target_ids}
        members: dict[str, list[tuple[str, str]]] = {}
        symbols = self.document.collection("symbols")
        blocks = self.document.collection("physical_blocks")
        for raw_symbol_id, raw_address in zip(symbol_ids, addresses):
            symbol_id = str(raw_symbol_id)
            address = str(raw_address)
            symbol = symbols.get(symbol_id)
            block = (
                blocks.get(str(symbol.get("physical_block_id", "")))
                if isinstance(symbol, Mapping)
                else None
            )
            facts = (
                block.get("accepted_order_facts")
                if isinstance(block, Mapping)
                else None
            )
            target_id = (
                str(facts.get("target_id", ""))
                if isinstance(facts, Mapping)
                else ""
            )
            if not target_id or target_id not in selected_target_ids:
                raise ValueError(
                    f"call-contract symbol {symbol_id} has no exact selected target"
                )
            self._target_for_address[address] = target_id
            members.setdefault(target_id, []).append((symbol_id, address))
        if set(members) != selected_target_ids:
            missing = sorted(selected_target_ids - set(members))
            raise ValueError(
                "call-contract candidate session has targets without retail bodies: "
                + ", ".join(missing)
            )
        self._members_by_target = {
            target_id: tuple(rows) for target_id, rows in members.items()
        }

    def target_id(self, address: str) -> str | None:
        """Return the exact accepted target identity for one caller address."""

        return self._target_for_address.get(address)

    def candidate(self, address: str) -> CandidateAssembly:
        target_id = self.target_id(address)
        if target_id is None:
            raise ValueError(
                f"call-contract candidate session has no body at {address}"
            )
        cached_failure = self._target_acquisition_failures.get(target_id)
        if cached_failure is not None:
            if isinstance(cached_failure, _cc_errors.CandidateBodyIdentityAcquisitionError):
                raise cached_failure
            raise ValueError(
                "call-contract candidate target acquisition previously failed "
                f"for {target_id}: {cached_failure}"
            )
        if target_id not in self.compiled_target_ids:
            if target_id not in self.attempted_target_ids:
                self.attempted_target_ids.append(target_id)
            members = self._members_by_target[target_id]
            target_slice = dict(self.slice_row)
            target_slice.update(
                {
                    "body_count": len(members),
                    "symbol_ids": [symbol_id for symbol_id, _ in members],
                    "addresses": [member_address for _, member_address in members],
                    "target_ids": [target_id],
                }
            )
            try:
                token = _cc_listing._ACTIVE_COD_LISTING_INDEX.set(self._cod_listing_index)
                try:
                    compiled = _cc_candidate._compile_slice_candidates(
                        self.document,
                        target_slice,
                        build_root=self.build_root,
                        vc5_env=self.vc5_env,
                        preloaded_targets=self.preloaded_targets,
                        precompiled_target_units=self.precompiled_target_units,
                        numeric_constructor_authority=self.numeric_constructor_authority,
                        toolchain_receipts=self.toolchain_receipts,
                    )
                finally:
                    _cc_listing._ACTIVE_COD_LISTING_INDEX.reset(token)
            except (OSError, RuntimeError, ValueError) as exc:
                self._target_acquisition_failures[target_id] = exc
                raise
            overlap = set(self._candidates) & set(compiled)
            if overlap:
                raise ValueError(
                    "call-contract lazy candidate compilation produced duplicate bodies: "
                    + ", ".join(sorted(overlap))
                )
            self._candidates.update(compiled)
            self.compiled_target_ids.append(target_id)
        candidate = self._candidates.get(address)
        if candidate is None:
            raise ValueError(
                f"call-contract candidate target did not produce body {address}"
            )
        return candidate

    def cod_listing_index_metrics(self) -> dict[str, Any]:
        """Return diagnostic-only invocation-local COD index counters."""

        return self._cod_listing_index.metrics()


def _candidate_target_acquisition_divergence(
    session: _LazyCallContractCandidateSession,
    *,
    symbol_id: str,
    address: str,
    error: Exception,
    caller_source_edit_paths: Sequence[str] = (),
    caller_symbol: Mapping[str, Any] | None = None,
) -> dict[str, Any]:
    target_id = session.target_id(address)
    if not target_id:
        raise ValueError(
            f"call-contract caller {symbol_id} has no exact candidate target"
        )
    result = {
        "symbol_id": symbol_id,
        "address": address,
        "kind": "verifier-blocked",
        "side": "candidate",
        "error_kind": "candidate-target-acquisition",
        "target_id": target_id,
        "message": str(error),
    }
    if isinstance(error, _cc_errors.CandidateBodyIdentityAcquisitionError):
        provenance = deepcopy(error.provenance)
        caller_path_rows = [
            str(path) for path in caller_source_edit_paths
        ]
        caller_paths = sorted(
            set(caller_path_rows),
            key=lambda path: (path.casefold(), path),
        )
        routed_paths = {
            str(path) for path in provenance.get("source_edit_paths", [])
        }
        declaration_paths = {
            str(path)
            for path in provenance.get("declaration_paths", [])
        }
        definition_paths = {
            str(path)
            for path in provenance.get("definition_paths", [])
        }
        declaration_keys = {
            path.casefold() for path in declaration_paths
        }
        definition_keys = {path.casefold() for path in definition_paths}
        routed_keys = {path.casefold() for path in routed_paths}
        caller_keys = {path.casefold() for path in caller_paths}
        routes = provenance.get("routes")
        route_keys: list[tuple[str, str, str]] = []
        routed_declaration_paths: set[str] = set()
        routed_definition_paths: set[str] = set()
        exact_group_routes = bool(isinstance(routes, list) and routes)
        if exact_group_routes:
            for route in routes:
                if not isinstance(route, Mapping):
                    exact_group_routes = False
                    break
                expected = route.get("expected_identity")
                route_declarations = route.get("declaration_paths")
                route_definitions = route.get("definition_paths")
                route_sources = route.get("source_edit_paths")
                route_key = (
                    str(route.get("symbol_id", "")),
                    str(route.get("address", "")),
                    str(
                        expected.get("decorated_identity", "")
                        if isinstance(expected, Mapping)
                        else ""
                    ),
                )
                route_keys.append(route_key)
                if not (
                    route.get("routing_mode") == "exact"
                    and isinstance(route_declarations, list)
                    and len(route_declarations) == 1
                    and route_declarations[0] in declaration_paths
                    and isinstance(route_definitions, list)
                    and len(route_definitions) == 1
                    and route_definitions[0] in definition_paths
                    and isinstance(route_sources, list)
                    and set(route_sources)
                    == set(route_declarations) | set(route_definitions)
                    and len(route_sources) == 2
                    and len(
                        {
                            str(path).casefold()
                            for path in route_sources
                        }
                    )
                    == 2
                ):
                    exact_group_routes = False
                    break
                routed_declaration_paths.update(
                    str(path) for path in route_declarations
                )
                routed_definition_paths.update(
                    str(path) for path in route_definitions
                )
        exact_group_routes = bool(
            exact_group_routes
            and all(all(key) for key in route_keys)
            and len(route_keys) == len(set(route_keys))
            and routed_declaration_paths == declaration_paths
            and routed_definition_paths == definition_paths
        )
        exact_coordinated_paths = bool(
            provenance.get("routing_mode") == "exact"
            and declaration_paths
            and definition_paths
            and routed_paths == declaration_paths | definition_paths
            and not (declaration_keys & caller_keys)
            and definition_keys.issubset(caller_keys)
            and exact_group_routes
        )
        exact_caller_owned_pair = bool(
            provenance.get("routing_mode") == "exact"
            and declaration_paths
            and len(definition_paths) == 1
            and routed_paths == declaration_paths | definition_paths
            and routed_paths.issubset(set(caller_paths))
            and routed_keys.issubset(caller_keys)
            and len(caller_path_rows) == len(caller_paths)
            and len(caller_keys) == len(caller_paths)
            and exact_group_routes
        )
        provenance["caller_target_source_edit_paths"] = caller_paths
        provenance["owner_relation"] = (
            "exact-dependent-declaration-header-definition"
            if provenance.get("routing_mode") == "exact"
            and caller_paths
            and exact_coordinated_paths
            else (
                "exact-caller-owned-declaration-header-definition"
                if exact_caller_owned_pair
                else (
                    "caller-owned"
                    if declaration_keys & caller_keys
                    else "non-source-ambiguity"
                )
            )
        )
        if (
            provenance["owner_relation"] not in {
                "exact-dependent-declaration-header-definition",
                "exact-caller-owned-declaration-header-definition",
            }
        ):
            provenance["routing_mode"] = "blocked"
            provenance["source_edit_paths"] = []
            provenance["declaration_paths"] = []
            provenance["definition_paths"] = []
        result["dependent_header_provenance"] = provenance
        if isinstance(caller_symbol, Mapping):
            result["caller_tracker_route"] = {
                "caller_symbol_id": symbol_id,
                "caller_physical_block_id": str(
                    caller_symbol.get("physical_block_id", "")
                ),
                "caller_owner_id": str(caller_symbol.get("owner_id", "")),
                "caller_semantic_span_ids": sorted(
                    {
                        str(value)
                        for value in caller_symbol.get("semantic_span_ids", [])
                        if isinstance(value, str) and value
                    }
                ),
            }
    return result


def _continuation_repair_routing(
    divergence: Mapping[str, Any],
) -> dict[str, Any] | None:
    """Project only a unique verifier-proven authored caller/owner pair.

    This projection is routing-only candidate evidence.  It neither supplies
    expected call facts nor makes any path writable; the parent continuation
    descriptor independently joins it to the retained predecessor closure.
    """

    provenance = divergence.get("dependent_header_provenance")
    if not isinstance(provenance, Mapping) or provenance.get("routing_mode") != "exact":
        return None
    if provenance.get("owner_relation") not in {
        "exact-dependent-declaration-header-definition",
        "exact-caller-owned-declaration-header-definition",
    }:
        return None
    declarations = provenance.get("declaration_paths")
    definitions = provenance.get("definition_paths")
    caller_paths = provenance.get("caller_target_source_edit_paths")
    routes = provenance.get("routes")
    tracker_route = divergence.get("caller_tracker_route")
    if not (
        isinstance(declarations, list) and len(declarations) == 1
        and isinstance(definitions, list) and len(definitions) == 1
        and isinstance(caller_paths, list)
        and isinstance(routes, list) and routes
        and isinstance(tracker_route, Mapping)
        and tracker_route.get("caller_symbol_id") == divergence.get("symbol_id")
        and isinstance(tracker_route.get("caller_physical_block_id"), str)
        and bool(tracker_route.get("caller_physical_block_id"))
        and isinstance(tracker_route.get("caller_owner_id"), str)
        and bool(tracker_route.get("caller_owner_id"))
        and isinstance(tracker_route.get("caller_semantic_span_ids"), list)
        and all(isinstance(path, str) and path for path in (*declarations, *definitions, *caller_paths))
    ):
        return None
    definition_key = definitions[0].casefold()
    caller_candidates = [
        path for path in caller_paths if path.casefold() != definition_key
    ]
    if len({path.casefold() for path in caller_candidates}) != 1:
        return None
    exact_routes = [
        route for route in routes
        if isinstance(route, Mapping)
        and route.get("routing_mode") == "exact"
        and route.get("declaration_paths") == declarations
        and route.get("definition_paths") == definitions
    ]
    if len(exact_routes) != 1:
        return None
    return {
        "schema": "call-contract-continuation-routing-evidence-v1",
        "caller_edit_path": caller_candidates[0],
        "controlling_declaration_path": declarations[0],
        "controlling_definition_path": definitions[0],
        **deepcopy(dict(tracker_route)),
        "unique_controlling_pair": True,
        "authored_route": True,
        "provider_boundary": False,
        "out_of_policy": False,
        "candidate_expected_truth": False,
        "source_provenance": deepcopy(dict(provenance)),
    }


def _resolve_slice(document: ProgressDocument, slice_id: str) -> dict[str, Any]:
    matches = [
        row
        for row in document.authored_call_contract_slices()
        if row.get("id") == slice_id
    ]
    if len(matches) != 1:
        raise ProgressError(
            f"call-contract slice {slice_id!r} must resolve exactly once; found {len(matches)}"
        )
    row = deepcopy(matches[0])
    if row["body_count"] < 1 or row["body_count"] > CALL_CONTRACT_SLICE_MAX_BODIES:
        raise ProgressError(f"call-contract slice {slice_id} violates the 160-body cap")
    if len(set(row["symbol_ids"])) != row["body_count"]:
        raise ProgressError(f"call-contract slice {slice_id} contains duplicate identities")
    return row


def _exact_target_compile_tu_source_edit_override(manifest: Any) -> list[str]:
    """Return only exact implementation TUs, never headers or definitions."""

    return list(
        dict.fromkeys(
            str(getattr(entry, "source_from", ""))
            for entry in getattr(
                manifest, "translation_unit_function_order", ()
            )
            if str(getattr(entry, "source_from", ""))
            and Path(str(getattr(entry, "source_from", ""))).suffix.casefold()
            in _cc_catalog.CALL_CONTRACT_SOURCE_SUFFIXES
        )
    )


def _call_contract_target_source_edit_roots(
    *,
    target_id: str,
    registration: Mapping[str, Any],
    manifest: Any,
    override_paths: Any,
    strict_internal_scope: bool,
    repository_path_inventory: RepositoryPathInventory,
) -> Any:
    """Project registered authority through current repository-case manifest paths."""

    reconciliation = _cc_source.call_contract_registration_path_reconciliation(
        target_id=target_id,
        registration=registration,
        current_target=manifest,
        inventory=repository_path_inventory,
    )
    if reconciliation["blocker_kind"] is not None:
        raise ProgressError(
            f"call-contract target {target_id} manifest-registration-drift: "
            f"removed={reconciliation['removed']!r}; "
            f"added={reconciliation['added']!r}; "
            f"unchanged={reconciliation['unchanged']!r}"
        )
    registered_order_edit_paths = registration.get("order_edit_paths")
    if manifest.order_edit_paths:
        manifest_paths = list(manifest.order_edit_paths)
        return manifest_paths
    if registered_order_edit_paths:
        return registered_order_edit_paths
    if override_paths:
        return override_paths
    if strict_internal_scope:
        raise ProgressError(
            f"call-contract target {target_id} has no authoritative "
            "order_edit_paths or internal exact compile-TU override"
        )
    return []


def _resolve_target_all_authored_bodies(
    document: ProgressDocument,
    target_id: str,
) -> dict[str, Any]:
    """Join one registered target to every original-slice gating body.

    Original slice identities remain immutable scheduling/acceptance units.
    This request-local projection is only a convergence diagnostic and derives
    membership from each physical block's accepted order-target identity.
    """

    target = document.collection("verification_targets").get(target_id)
    if not isinstance(target, Mapping) or target.get("kind") != "vc5":
        raise ProgressError(
            f"call-contract target {target_id!r} is not one exact registered VC5 target"
        )
    original_slices = document.authored_call_contract_slices()
    symbols = document.collection("symbols")
    blocks = document.collection("physical_blocks")
    selected: list[tuple[str, str, str, str]] = []
    seen_symbols: set[str] = set()
    original_slice_ids: list[str] = []
    for slice_row in original_slices:
        slice_selected = False
        symbol_ids = slice_row.get("symbol_ids")
        addresses = slice_row.get("addresses")
        if (
            not isinstance(symbol_ids, list)
            or not isinstance(addresses, list)
            or len(symbol_ids) != len(addresses)
        ):
            raise ProgressError(
                "call-contract original slice has malformed ordered membership"
            )
        for raw_symbol_id, raw_address in zip(symbol_ids, addresses):
            symbol_id = str(raw_symbol_id)
            address = normalize_address(str(raw_address))
            symbol = symbols.get(symbol_id)
            if not isinstance(symbol, Mapping):
                raise ProgressError(
                    f"call-contract original slice references unknown symbol {symbol_id}"
                )
            block_id = str(symbol.get("physical_block_id", ""))
            block = blocks.get(block_id)
            facts = (
                block.get("accepted_order_facts")
                if isinstance(block, Mapping)
                else None
            )
            if not isinstance(facts, Mapping) or facts.get("target_id") != target_id:
                continue
            if (
                symbol.get("pipeline_class") not in {"authored", "authored-lifecycle"}
            ):
                raise ProgressError(
                    f"call-contract target {target_id} selected non-gating symbol {symbol_id}"
                )
            if symbol_id in seen_symbols:
                raise ProgressError(
                    f"call-contract target {target_id} repeats physical body {symbol_id}"
                )
            seen_symbols.add(symbol_id)
            selected.append((symbol_id, address, block_id, str(slice_row.get("id", ""))))
            slice_selected = True
        if slice_selected:
            original_slice_ids.append(str(slice_row.get("id", "")))
    if not selected:
        raise ProgressError(
            f"call-contract target {target_id!r} has no authored gating bodies in the original slices"
        )
    addresses = [address for _symbol_id, address, _block_id, _slice_id in selected]
    if addresses != sorted(addresses, key=address_value):
        raise ProgressError(
            f"call-contract target {target_id!r} projection is not in retail order"
        )
    physical_block_ids = list(
        dict.fromkeys(block_id for _symbol_id, _address, block_id, _slice_id in selected)
    )
    result = {
        "id": f"recoil:call-contract-target-scope:{target_id}",
        "ordinal": 0,
        "start": addresses[0],
        "end": addresses[-1],
        "body_count": len(selected),
        "symbol_ids": [row[0] for row in selected],
        "addresses": addresses,
        "target_ids": [target_id],
        "physical_block_ids": physical_block_ids,
        "source_paths": [],
        "target_id": target_id,
        "original_slice_ids": original_slice_ids,
        "selection_mode": "target-all-authored-bodies",
    }
    registration = target.get("registration")
    if not isinstance(registration, Mapping):
        raise ProgressError(
            f"call-contract target {target_id!r} has no exact registration"
        )
    manifest_value = registration.get("manifest_path")
    if not isinstance(manifest_value, str) or not manifest_value:
        raise ProgressError(
            f"call-contract target {target_id!r} has no manifest path"
        )
    manifest_path = (REPO_ROOT / manifest_value).resolve()
    manifest = _cc_source._call_contract_cached_manifest(document, manifest_path)
    registered_edit_paths = registration.get("order_edit_paths")
    if registered_edit_paths is None:
        registered_edit_paths = list(manifest.order_edit_paths)
    if not registered_edit_paths:
        exact_compile_sources = _exact_target_compile_tu_source_edit_override(
            manifest
        )
        if not exact_compile_sources:
            raise ProgressError(
                f"call-contract target {target_id!r} has neither order_edit_paths "
                "nor exact translation-unit source_from implementation roots"
            )
        result["source_edit_paths_override"] = {
            target_id: exact_compile_sources
        }
    return result


def _resolve_phase_all_authored_bodies(
    document: ProgressDocument,
) -> dict[str, Any]:
    """Project the immutable original slices into one retail-ordered proof scope.

    The projection is invocation-local.  Original slice identities remain the
    only acceptance units; this wider scope exists solely so target builds,
    source discovery, COD indexing, and Binary Ninja reads can be shared by a
    governed replay.
    """

    original_slices = document.authored_call_contract_slices()
    if not original_slices:
        raise ProgressError("call-contract phase has no original slices")
    symbol_ids: list[str] = []
    addresses: list[str] = []
    target_ids: list[str] = []
    physical_block_ids: list[str] = []
    source_paths: list[str] = []
    slice_boundaries: list[dict[str, Any]] = []
    source_edit_paths_override: dict[str, list[str]] = {}
    offset = 0
    for raw_slice in original_slices:
        slice_row = _resolve_slice(document, str(raw_slice.get("id", "")))
        count = int(slice_row["body_count"])
        slice_boundaries.append(
            {
                "slice_id": str(slice_row["id"]),
                "start_index": offset,
                "end_index_exclusive": offset + count,
                "body_count": count,
            }
        )
        offset += count
        symbol_ids.extend(str(value) for value in slice_row["symbol_ids"])
        addresses.extend(
            normalize_address(str(value)) for value in slice_row["addresses"]
        )
        target_ids.extend(str(value) for value in slice_row["target_ids"])
        physical_block_ids.extend(
            str(value) for value in slice_row["physical_block_ids"]
        )
        source_paths.extend(str(value) for value in slice_row["source_paths"])
    if len(symbol_ids) != len(set(symbol_ids)):
        raise ProgressError("call-contract phase projection repeats a body identity")
    if addresses != sorted(addresses, key=address_value):
        raise ProgressError("call-contract phase projection is not in retail order")

    selected_target_ids = list(dict.fromkeys(target_ids))
    targets = document.collection("verification_targets")
    for selected_target_id in selected_target_ids:
        target = targets.get(selected_target_id)
        registration = (
            target.get("registration") if isinstance(target, Mapping) else None
        )
        if not isinstance(target, Mapping) or target.get("kind") != "vc5":
            raise ProgressError(
                f"call-contract phase target {selected_target_id!r} is not one exact VC5 target"
            )
        if not isinstance(registration, Mapping):
            raise ProgressError(
                f"call-contract phase target {selected_target_id!r} has no exact registration"
            )
        manifest_value = registration.get("manifest_path")
        if not isinstance(manifest_value, str) or not manifest_value:
            raise ProgressError(
                f"call-contract phase target {selected_target_id!r} has no manifest path"
            )
        manifest = _cc_source._call_contract_cached_manifest(
            document,
            (REPO_ROOT / manifest_value).resolve(),
        )
        registered_edit_paths = registration.get("order_edit_paths")
        if registered_edit_paths is None:
            registered_edit_paths = list(manifest.order_edit_paths)
        if not registered_edit_paths:
            exact_compile_sources = _exact_target_compile_tu_source_edit_override(
                manifest
            )
            if not exact_compile_sources:
                raise ProgressError(
                    f"call-contract phase target {selected_target_id!r} has neither "
                    "order_edit_paths nor exact translation-unit implementation roots"
                )
            source_edit_paths_override[selected_target_id] = exact_compile_sources

    result = {
        "id": "recoil:call-contract-phase-scope:authored-call-contract",
        "ordinal": 0,
        "start": addresses[0],
        "end": addresses[-1],
        "body_count": len(symbol_ids),
        "symbol_ids": symbol_ids,
        "addresses": addresses,
        "target_ids": selected_target_ids,
        "physical_block_ids": list(dict.fromkeys(physical_block_ids)),
        "source_paths": list(dict.fromkeys(source_paths)),
        "original_slice_ids": [row["slice_id"] for row in slice_boundaries],
        "slice_boundaries": slice_boundaries,
        "selection_mode": "phase-all-authored-bodies",
    }
    if source_edit_paths_override:
        result["source_edit_paths_override"] = source_edit_paths_override
    return result
