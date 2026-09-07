"""Recoil call-contract session evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import abi as _cc_abi
from _recoil.call_contract import callable_identity as _cc_callable_identity
from _recoil.call_contract import candidate_session as _cc_candidate_session
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import comparison as _cc_comparison
from _recoil.call_contract import contributions as _cc_contributions
from _recoil.call_contract import errors as _cc_errors
from _recoil.call_contract import identity as _cc_identity
from _recoil.call_contract import (
    phase_candidate_dispatch as _cc_phase_candidate_dispatch,
)
from _recoil.call_contract import (
    phase_candidate_iat_and_abi as _cc_phase_candidate_iat_and_abi,
)
from _recoil.call_contract import (
    phase_candidate_lifecycle as _cc_phase_candidate_lifecycle,
)
from _recoil.call_contract import (
    phase_candidate_providers as _cc_phase_candidate_providers,
)
from _recoil.call_contract import (
    phase_candidate_receivers as _cc_phase_candidate_receivers,
)
from _recoil.call_contract import phase_candidate_storage as _cc_phase_candidate_storage
from _recoil.call_contract import phase_retail_receivers as _cc_phase_retail_receivers
from _recoil.call_contract import phase_retail_storage as _cc_phase_retail_storage
from _recoil.call_contract import (
    phase_selected_dependencies as _cc_phase_selected_dependencies,
)
from _recoil.call_contract import recoil_application as _cc_recoil_application
from _recoil.call_contract import recoil_mfc as _cc_recoil_mfc
from _recoil.call_contract import reporting as _cc_reporting
from _recoil.call_contract import retail as _cc_retail
from _recoil.call_contract import retail_imports as _cc_retail_imports
from _recoil.call_contract import source as _cc_source
from _recoil.call_contract import work as _cc_work

if TYPE_CHECKING:
    from _recoil.call_contract.records import (
        CandidateConstructorDefinition,
        CurrentIatStoragePackage,
        ReviewedInboundEntryRegisterTargetBridge,
        _RetailAuthoredNamespaceAbi,
    )


import time
from copy import deepcopy
from dataclasses import replace
from pathlib import Path
from typing import Any, Mapping, Sequence

from _recoil.commands.asm_verify import CoffObject
from _recoil.commands.provider_target_mutation import _retail_import_targets
from _recoil.commands.vc5_verify import DEFAULT_VC5_ENV
from _recoil.lib.binja import DEFAULT_BRIDGE_URL, BinaryNinjaBridge, BridgeError
from _recoil.lib.progress import (
    CALL_CONTRACT_CONTRACT_VERSION,
    ProgressDocument,
    ProgressError,
)
from _recoil.lib.repository_paths import (
    RepositoryPathInventory,
    load_repository_path_inventory,
)
from _recoil.lib.tooling import REPO_ROOT


def _caller_isolated_diagnostic_census(
    *,
    symbol_ids: Sequence[str],
    addresses: Sequence[str],
    expected_contracts: Mapping[str, Sequence[Mapping[str, Any]]],
    candidate_contracts: Mapping[str, Sequence[Mapping[str, Any]]],
    caller_divergences: Sequence[Mapping[str, Any]],
    complete: bool,
) -> dict[str, Any]:
    """Build one fail-closed, strictly nonaccepting caller diagnostic census."""

    ordered_coordinates = list(zip(symbol_ids, addresses))
    if (
        len(ordered_coordinates) != len(symbol_ids)
        or len({symbol_id for symbol_id, _address in ordered_coordinates})
        != len(ordered_coordinates)
        or len({address for _symbol_id, address in ordered_coordinates})
        != len(ordered_coordinates)
    ):
        raise ValueError(
            "caller-isolated diagnostic census has colliding caller coordinates"
        )
    coordinate_by_symbol = dict(ordered_coordinates)
    ordinal_by_symbol = {
        symbol_id: ordinal
        for ordinal, (symbol_id, _address) in enumerate(ordered_coordinates)
    }
    divergence_rows: list[dict[str, Any]] = []
    prior_ordinal = -1
    seen: set[str] = set()
    for raw in caller_divergences:
        symbol_id = str(raw.get("symbol_id", ""))
        address = str(raw.get("address", ""))
        ordinal = ordinal_by_symbol.get(symbol_id, -1)
        if (
            ordinal < 0
            or symbol_id in seen
            or coordinate_by_symbol.get(symbol_id) != address
            or ordinal <= prior_ordinal
        ):
            raise ValueError(
                "caller-isolated diagnostic census divergence coordinates "
                "collide, drift, or are out of retail order"
            )
        seen.add(symbol_id)
        prior_ordinal = ordinal
        divergence_rows.append(deepcopy(dict(raw)))

    expected_symbols = set(expected_contracts)
    candidate_symbols = set(candidate_contracts)
    census_symbols = set(symbol_ids)
    if (
        not expected_symbols.issubset(census_symbols)
        or not candidate_symbols.issubset(census_symbols)
        or candidate_symbols != expected_symbols
    ):
        raise ValueError(
            "caller-isolated diagnostic census contract population drifted"
        )
    completed_symbols = expected_symbols | seen
    if complete and completed_symbols != census_symbols:
        raise ValueError(
            "complete caller-isolated diagnostic census omitted callers"
        )
    mismatch_count = sum(
        row.get("kind") == "mismatch" for row in divergence_rows
    )
    blocker_count = len(divergence_rows) - mismatch_count
    return {
        "kind": "authored-call-contract-caller-diagnostic-census",
        "contract_version": 1,
        "nonaccepting": True,
        "acceptance_eligible": False,
        "acceptance_route": None,
        "candidate_expected_truth": False,
        "complete": complete,
        "ordered_caller_count": len(ordered_coordinates),
        "evaluated_caller_count": len(completed_symbols),
        "passing_caller_count": len(expected_symbols - seen),
        "divergent_caller_count": len(divergence_rows),
        "semantic_mismatch_caller_count": mismatch_count,
        "verifier_blocked_caller_count": blocker_count,
        "later_divergence_count": max(0, len(divergence_rows) - 1),
        "divergences": divergence_rows,
    }


def live_call_contract_result(
    *,
    document: ProgressDocument,
    slice_id: str | None = None,
    target_id: str | None = None,
    all_authored_bodies: bool = False,
    phase_all_authored_bodies: bool = False,
    build_root: Path,
    vc5_env: Path = DEFAULT_VC5_ENV,
    bridge_url: str = DEFAULT_BRIDGE_URL,
    bridge: BinaryNinjaBridge | None = None,
    collect_all_divergences: bool = False,
    compile_definition_closure: bool = True,
    compile_definition_closure_on_divergence: bool = False,
    precompiled_target_units: Mapping[
        str, tuple[Any, tuple[tuple[Any, Path, CoffObject], ...]]
    ] | None = None,
    numeric_constructor_authority: tuple[
        Any,
        dict[str, CandidateConstructorDefinition],
        dict[str, CandidateConstructorDefinition],
        dict[str, CandidateConstructorDefinition],
    ] | None = None,
    _diagnostic_comparison_context: dict[str, Any] | None = None,
    _memory_trace: _cc_reporting._CallContractMemoryTrace | None = None,
    _repository_path_inventory: RepositoryPathInventory | None = None,
) -> dict[str, Any]:
    memory_trace = _memory_trace or _cc_reporting._CallContractMemoryTrace.from_environment()
    memory_trace.emit("verification-start")
    total_started = time.perf_counter()
    timings_ms = _cc_reporting._empty_call_contract_timings_ms()
    setup_started = time.perf_counter()
    target_mode = target_id is not None
    phase_mode = bool(phase_all_authored_bodies)
    if target_mode and phase_mode:
        raise ProgressError(
            "call-contract verification cannot combine target and phase scopes"
        )
    if phase_mode:
        if slice_id is not None or all_authored_bodies or not collect_all_divergences:
            raise ProgressError(
                "phase call-contract verification requires the internal complete-census route"
            )
        slice_row = _cc_candidate_session._resolve_phase_all_authored_bodies(document)
        result_slice_id = str(slice_row["id"])
    elif target_mode:
        if slice_id is not None or not all_authored_bodies:
            raise ProgressError(
                "target call-contract verification requires --target with --all-authored-bodies only"
            )
        slice_row = _cc_candidate_session._resolve_target_all_authored_bodies(document, target_id)
        result_slice_id = str(slice_row["id"])
    else:
        if slice_id is None or all_authored_bodies:
            raise ProgressError(
                "slice call-contract verification requires exactly --slice"
            )
        slice_row = _cc_candidate_session._resolve_slice(document, slice_id)
        result_slice_id = slice_id
    caller_total = int(slice_row["body_count"])
    memory_trace.emit(
        "selection-complete",
        caller_total=caller_total,
        selected_target_count=len(slice_row["target_ids"]),
    )
    repository_path_inventory = (
        _repository_path_inventory
        or load_repository_path_inventory(REPO_ROOT)
    )
    source_closure = _cc_source.call_contract_source_closure(
        document,
        slice_row,
        repository_path_inventory=repository_path_inventory,
    )
    target_source_edit_paths = _cc_source._call_contract_target_source_edit_paths(
        slice_row,
        source_closure,
        require_explicit=False,
    )
    dependency_paths = _cc_source.source_dependency_paths(
        document,
        slice_row,
        source_write_paths=source_closure.source_edit_paths,
        source_closure=source_closure,
        repository_path_inventory=repository_path_inventory,
    )
    signatures_before = _cc_source.file_dependency_states(
        dependency_paths,
        repository_path_inventory=repository_path_inventory,
    )
    slice_targets = _cc_source._call_contract_slice_targets(
        document,
        slice_row,
        repository_path_inventory=repository_path_inventory,
    )
    candidate_session = _cc_candidate_session._LazyCallContractCandidateSession(
        document,
        slice_row,
        build_root=build_root,
        vc5_env=vc5_env,
        preloaded_targets=slice_targets,
        precompiled_target_units=precompiled_target_units,
        numeric_constructor_authority=numeric_constructor_authority,
    )
    memory_trace.emit(
        "source-closure-complete",
        caller_total=caller_total,
        source_edit_path_count=len(source_closure.source_edit_paths),
        header_path_count=len(source_closure.header_paths),
        definition_source_path_count=len(
            source_closure.definition_source_paths
        ),
        dependency_path_count=len(dependency_paths),
    )
    _cc_reporting._add_elapsed_ms(
        timings_ms,
        "source_closure_dependency_setup",
        setup_started,
    )
    inventory_started = time.perf_counter()
    if bridge is None:
        bridge = _cc_retail._new_call_contract_bridge(
            bridge_url=bridge_url,
            body_count=int(slice_row["body_count"]),
        )
    phase_bridge_cache: _cc_retail._CallContractBinaryNinjaFactCache | None = None
    if phase_mode:
        phase_bridge_cache = _cc_retail._CallContractBinaryNinjaFactCache(bridge)
        bridge = phase_bridge_cache
        phase_bridge_cache.preload_assemblies(slice_row["addresses"])
    by_address, by_name = bridge.symbols()
    bridge_data_rows = bridge.data_variables()
    # Immutable PE import truth is invocation-scoped.  Several independent
    # proof producers consume this same population; deriving it once avoids
    # rereading and reparsing the retail image for every selected caller while
    # preserving the live invocation's stat/race boundary.
    retail_import_targets = tuple(_retail_import_targets(_cc_catalog.DEFAULT_REFERENCE)[0])
    memory_trace.emit(
        "retail-inventory-complete",
        caller_total=caller_total,
        bridge_symbol_count=len(by_address),
        bridge_name_count=len(by_name),
        bridge_data_count=len(bridge_data_rows),
        retail_import_target_count=len(retail_import_targets),
    )
    _cc_reporting._add_elapsed_ms(timings_ms, "binary_ninja_inventory", inventory_started)
    exact_bridge_names: dict[str, list[Any]] = {}
    for bridge_symbol in by_address.values():
        for name in {
            getattr(bridge_symbol, "name", ""),
            getattr(bridge_symbol, "raw_name", ""),
            getattr(bridge_symbol, "full_name", ""),
        }:
            if name:
                exact_bridge_names.setdefault(str(name), []).append(bridge_symbol)
    retail_provider_fact_transcript: list[dict[str, Any]] = []
    indexes = _cc_identity.build_identity_indexes(
        document,
        bridge=bridge,
        retail_provider_fact_transcript=retail_provider_fact_transcript,
        bridge_by_address=by_address,
        bridge_data_rows=bridge_data_rows,
    )
    zeroarg_abi_bridges = _cc_abi.resolve_manifest_zeroarg_abi_identity_bridges(
        document,
        indexes=indexes,
        bridge_by_address=by_address,
    )
    if zeroarg_abi_bridges:
        merged_candidate_names = dict(indexes.by_candidate_name)
        for symbol_name, identity in zeroarg_abi_bridges.items():
            prior = merged_candidate_names.get(symbol_name)
            if prior not in {None, identity}:
                raise ValueError(
                    f"validated zeroarg ABI symbol {symbol_name!r} conflicts "
                    "with current candidate identity"
                )
            merged_candidate_names[symbol_name] = identity
        indexes = replace(indexes, by_candidate_name=merged_candidate_names)
    alias_started = time.perf_counter()
    coff_alias_bridges = _cc_identity._validated_final_build_coff_alias_bridges(
        {},
        indexes=indexes,
        build_root=build_root,
        active_targets=slice_targets.values(),
    )
    _cc_reporting._add_elapsed_ms(
        timings_ms,
        "target_candidate_compilation",
        alias_started,
    )
    if coff_alias_bridges:
        merged_candidate_names = dict(indexes.by_candidate_name)
        for alias, identity in coff_alias_bridges.items():
            prior = merged_candidate_names.get(alias)
            if prior is not None and prior != identity:
                raise ValueError(
                    f"validated COFF alias {alias!r} conflicts with current candidate identity"
                )
            merged_candidate_names[alias] = identity
        indexes = replace(indexes, by_candidate_name=merged_candidate_names)
    (
        ftol_provider_identity,
        ftol_retail_bridge_names,
    ) = _cc_retail_imports._ftol_import_thunk_retail_bridge(
        document=document,
        indexes=indexes,
        bridge_by_address=by_address,
        bridge_by_name=by_name,
        bridge_data_rows=bridge_data_rows,
        bridge=bridge,
    )
    for name, symbols_for_name in ftol_retail_bridge_names.items():
        if name in exact_bridge_names:
            raise ValueError(
                f"MSVC _ftol import-thunk bridge name {name!r} already "
                "has a different ordinary bridge population"
            )
        exact_bridge_names[name] = symbols_for_name
    (
        gettickcount_named_import_thunk,
        gettickcount_retail_bridge_names,
    ) = _cc_retail_imports._gettickcount_named_import_thunk_retail_bridge(
        document=document,
        indexes=indexes,
        bridge_by_address=by_address,
        bridge_by_name=by_name,
        bridge=bridge,
    )
    for name, symbols_for_name in gettickcount_retail_bridge_names.items():
        prior = exact_bridge_names.get(name)
        prior_addresses = {
            getattr(symbol, "address", "") for symbol in prior or ()
        }
        current_addresses = {
            getattr(symbol, "address", "") for symbol in symbols_for_name
        }
        if prior is not None and prior_addresses != current_addresses:
            raise ValueError(
                f"GetTickCount named import-thunk bridge name {name!r} "
                "collides with a different ordinary bridge population"
            )
        exact_bridge_names[name] = symbols_for_name
    preproven_named_packages: tuple[CurrentIatStoragePackage, ...] = ()
    if gettickcount_named_import_thunk is not None:
        indexes, preproven_named_packages = (
            _cc_retail_imports._integrate_preproven_named_import_thunk_packages(
                indexes,
                (gettickcount_named_import_thunk,),
            )
            )
    (
        _provider_named_import_thunks,
        provider_named_import_thunk_names,
    ) = _cc_retail_imports._provider_named_import_thunk_retail_bridges(
        document=document,
        indexes=indexes,
        bridge_by_address=by_address,
        bridge_by_name=by_name,
        bridge=bridge,
        additional_named_packages=preproven_named_packages,
        preproven_named_thunks=(
            (gettickcount_named_import_thunk,)
            if gettickcount_named_import_thunk is not None
            else ()
        ),
        retail_import_targets=retail_import_targets,
    )
    for name, symbols_for_name in provider_named_import_thunk_names.items():
        prior = exact_bridge_names.get(name)
        prior_addresses = {
            getattr(symbol, "address", "")
            for symbol in prior or ()
        }
        current_addresses = {
            getattr(symbol, "address", "")
            for symbol in symbols_for_name
        }
        if prior is not None and prior_addresses != current_addresses:
            raise ValueError(
                f"named import thunk bridge name {name!r} collides with "
                "a different ordinary bridge population"
            )
        exact_bridge_names[name] = symbols_for_name
    (
        provider_ordinal_import_thunks,
        provider_ordinal_import_thunk_names,
    ) = _cc_retail_imports._provider_ordinal_import_thunk_retail_bridges(
        document=document,
        indexes=indexes,
        bridge_by_address=by_address,
        bridge_by_name=by_name,
        bridge=bridge,
        excluded_symbol_ids=frozenset({_cc_catalog.WOL_CSTRING_TARGET_SYMBOL_ID}),
    )
    (
        wol_cstring_ordinal_import_thunks,
        wol_cstring_ordinal_import_thunk_names,
    ) = _cc_recoil_mfc._wol_cstring_provider_ordinal_import_thunk_retail_bridge(
        document=document,
        indexes=indexes,
        bridge_by_address=by_address,
        bridge_by_name=by_name,
        bridge=bridge,
        retail_import_targets=retail_import_targets,
    )
    (
        indexes,
        pe_ordinal_import_thunks,
        pe_ordinal_import_thunk_names,
    ) = _cc_retail_imports._provider_pe_ordinal_import_thunk_retail_bridges(
        document=document,
        indexes=indexes,
        bridge_by_address=by_address,
        bridge_by_name=by_name,
        bridge=bridge,
        retail_import_targets=retail_import_targets,
    )
    (
        terminal_ordinal_import_thunks,
        terminal_ordinal_import_thunk_names,
    ) = _cc_retail_imports._provider_terminal_ordinal_import_thunk_retail_bridges(
        document=document,
        indexes=indexes,
        bridge_by_address=by_address,
        bridge_by_name=by_name,
        bridge=bridge,
    )
    combined_ordinal_thunks = (
        *provider_ordinal_import_thunks,
        *wol_cstring_ordinal_import_thunks,
        *pe_ordinal_import_thunks,
        *terminal_ordinal_import_thunks,
    )
    for attribute in (
        "provider_identity",
        "thunk_address",
        "retail_name",
        "callable_symbol",
        "iat_object_symbol",
        "iat_identity",
    ):
        values = [getattr(thunk, attribute) for thunk in combined_ordinal_thunks]
        if len(values) != len(set(values)):
            raise ValueError(
                "ordinal import thunk routes collide after terminal-block "
                f"composition for {attribute.replace('_', ' ')}"
            )
    provider_ordinal_import_thunks = tuple(combined_ordinal_thunks)
    for name, rows in wol_cstring_ordinal_import_thunk_names.items():
        provider_ordinal_import_thunk_names.setdefault(name, []).extend(rows)
    for name, rows in pe_ordinal_import_thunk_names.items():
        provider_ordinal_import_thunk_names.setdefault(name, []).extend(rows)
    for name, rows in terminal_ordinal_import_thunk_names.items():
        provider_ordinal_import_thunk_names.setdefault(name, []).extend(rows)
    for name, symbols_for_name in provider_ordinal_import_thunk_names.items():
        prior = exact_bridge_names.get(name)
        prior_addresses = {
            getattr(symbol, "address", "")
            for symbol in prior or ()
        }
        current_addresses = {
            getattr(symbol, "address", "")
            for symbol in symbols_for_name
        }
        if prior is not None and prior_addresses != current_addresses:
            raise ValueError(
                f"ordinal import thunk bridge name {name!r} collides with "
                "a different ordinary bridge population"
            )
        exact_bridge_names[name] = symbols_for_name
    symbols = document.collection("symbols")
    proof_diagnostics_by_symbol: dict[str, list[dict[str, Any]]] = {}
    expected_by_symbol: dict[str, list[dict[str, Any]]] = {}
    candidate_by_symbol: dict[str, list[dict[str, Any]]] = {}
    evaluated_symbol_ids: set[str] = set()
    retail_provenance_adapter_sites_by_symbol: dict[str, list[str]] = {}
    inline_absence_proofs_by_symbol: dict[str, dict[str, Any]] = {}
    candidate_cleanup_receipts_by_symbol: dict[str, list[dict[str, Any]]] = {}
    candidate_expansion_receipts_by_symbol: dict[str, dict[str, Any]] = {}
    first_divergence: dict[str, Any] | None = None
    caller_divergences: list[dict[str, Any]] = []
    authored_decorated_abi_retail_cache: dict[
        tuple[str, str, str], _RetailAuthoredNamespaceAbi
    ] = {}
    base_indexes = indexes

    def trace_caller(
        stage: str,
        *,
        caller_index: int,
        symbol_id: str,
        address: str,
        **extra: Any,
    ) -> None:
        memory_trace.emit(
            stage,
            caller_index=caller_index,
            caller_total=caller_total,
            symbol_id=symbol_id,
            address=address,
            candidate_cache_count=len(candidate_session._candidates),
            candidate_failure_count=len(
                candidate_session._target_acquisition_failures
            ),
            callable_inventory_cache_count=len(
                getattr(
                    document,
                    "_call_contract_callable_inventory_cache_v1",
                    {},
                )
            ),
            manifest_cache_count=len(
                getattr(
                    document,
                    "_call_contract_target_manifest_cache_v1",
                    {},
                )
            ),
            expected_result_count=len(expected_by_symbol),
            candidate_result_count=len(candidate_by_symbol),
            divergence_count=len(caller_divergences),
            **extra,
        )

    for caller_index, (symbol_id, address) in enumerate(
        zip(slice_row["symbol_ids"], slice_row["addresses"]),
        start=1,
    ):
        indexes = base_indexes
        symbol = symbols.get(symbol_id)
        if not isinstance(symbol, Mapping):
            raise ValueError(f"unknown call-contract symbol {symbol_id}")
        caller_identity = indexes.by_address.get(address, "")
        if not caller_identity:
            raise ValueError(f"ambiguous or unresolved caller identity at {address}")
        end_exclusive = str(symbol.get("end_exclusive", ""))
        if not end_exclusive:
            raise ValueError(f"call-contract caller {symbol_id} has no known extent")
        candidate_target_id = candidate_session.target_id(address)
        if not candidate_target_id:
            raise ValueError(
                f"call-contract caller {symbol_id} has no exact candidate target"
            )
        caller_source_edit_paths = target_source_edit_paths.get(
            candidate_target_id
        )
        if caller_source_edit_paths is None:
            raise ProgressError(
                "call-contract caller "
                f"{symbol_id} target {candidate_target_id} is absent from "
                "the target-scoped source-edit lookup"
            )
        trace_caller(
            "candidate-construction-start",
            caller_index=caller_index,
            symbol_id=symbol_id,
            address=address,
        )
        candidate_started = time.perf_counter()
        try:
            candidate_assembly = candidate_session.candidate(address)
        except (OSError, RuntimeError, ValueError) as exc:
            if not collect_all_divergences:
                raise
            divergence = _cc_candidate_session._candidate_target_acquisition_divergence(
                candidate_session,
                symbol_id=symbol_id,
                address=address,
                error=exc,
                caller_source_edit_paths=caller_source_edit_paths,
                caller_symbol=symbol,
            )
            caller_divergences.append(divergence)
            if first_divergence is None:
                first_divergence = divergence
            trace_caller(
                "divergence-retained",
                caller_index=caller_index,
                symbol_id=symbol_id,
                address=address,
                divergence_side="candidate-construction",
            )
            _cc_reporting._add_elapsed_ms(
                timings_ms,
                "target_candidate_compilation",
                candidate_started,
            )
            continue
        _cc_reporting._add_elapsed_ms(
            timings_ms,
            "target_candidate_compilation",
            candidate_started,
        )
        trace_caller(
            "candidate-construction-complete",
            caller_index=caller_index,
            symbol_id=symbol_id,
            address=address,
            candidate_instruction_count=len(candidate_assembly.instructions),
        )
        assembly_started = time.perf_counter()
        inbound_entry_target_bridges: dict[
            int, ReviewedInboundEntryRegisterTargetBridge
        ] = {}
        inbound_entry_register_roots: frozenset[str] = frozenset()
        trace_caller(
            "retail-proof-start",
            caller_index=caller_index,
            symbol_id=symbol_id,
            address=address,
        )
        work = _cc_work.CallerWork(
            _provider_named_import_thunks=_provider_named_import_thunks,
            address=address,
            assembly_started=assembly_started,
            authored_decorated_abi_retail_cache=authored_decorated_abi_retail_cache,
            bridge=bridge,
            bridge_data_rows=bridge_data_rows,
            caller_identity=caller_identity,
            caller_index=caller_index,
            candidate_assembly=candidate_assembly,
            candidate_cleanup_receipts_by_symbol=candidate_cleanup_receipts_by_symbol,
            candidate_expansion_receipts_by_symbol=candidate_expansion_receipts_by_symbol,
            document=document,
            end_exclusive=end_exclusive,
            exact_bridge_names=exact_bridge_names,
            ftol_provider_identity=ftol_provider_identity,
            gettickcount_named_import_thunk=gettickcount_named_import_thunk,
            inbound_entry_register_roots=inbound_entry_register_roots,
            inbound_entry_target_bridges=inbound_entry_target_bridges,
            indexes=indexes,
            provider_ordinal_import_thunks=provider_ordinal_import_thunks,
            retail_import_targets=retail_import_targets,
            retail_provenance_adapter_sites_by_symbol=retail_provenance_adapter_sites_by_symbol,
            symbol_id=symbol_id,
            timings_ms=timings_ms,
            trace_caller=trace_caller,
        )
        try:
            _cc_phase_retail_storage.recover_retail_storage(work)
            _cc_phase_retail_receivers.recover_retail_receivers(work)
        except (BridgeError, OSError, RuntimeError, ValueError) as exc:
            divergence_side = (
                "candidate"
                if isinstance(exc, _cc_errors.CandidateCallContractEvidenceError)
                else "retail"
            )
            body_error = _cc_errors.CallContractBodyError(
                symbol_id=work.symbol_id,
                address=work.address,
                side=divergence_side,
                message=str(exc),
            )
            if not collect_all_divergences:
                raise body_error from exc
            divergence = {
                "symbol_id": work.symbol_id,
                "address": work.address,
                "kind": "verifier-blocked",
                "side": divergence_side,
                "message": str(exc),
            }
            if hasattr(exc, "diagnostic"):
                divergence["proof_diagnostic"] = dict(exc.diagnostic)
            caller_divergences.append(divergence)
            if first_divergence is None:
                first_divergence = divergence
            work.trace_caller(
                "divergence-retained",
                caller_index=work.caller_index,
                symbol_id=work.symbol_id,
                address=work.address,
                divergence_side=divergence_side,
            )
            continue
        try:
            # Prove artifact boundaries before comparing populations. These
            # are separately catalogued catch bodies, not discarded parent calls.
            work.candidate_funclet_partition = _cc_recoil_application._appframe_run_catch_funclet_proof(
                work.candidate_assembly, caller_identity=work.caller_identity,
                caller_start=work.address, caller_end_exclusive=work.end_exclusive)
            raw_target_divergence = _cc_identity._raw_call_count_divergence(
                work.expected, work.candidate_assembly, work.retail_instructions,
                proved_funclet_call_indices=(
                    work.candidate_funclet_partition.excluded_invocation_indices
                    if work.candidate_funclet_partition is not None else ()),
            ) or _cc_callable_identity._known_authored_direct_target_divergence(
                work.expected, work.candidate_assembly, indexes=work.indexes,
            )
        except (ValueError, RuntimeError) as exc:
            raw_target_divergence = {
                "kind": "verifier-blocked", "side": "candidate", "message": str(exc),
                "reason": "artifact-partition-before-raw-call-census",
            }
        if raw_target_divergence is not None:
            divergence = {
                "symbol_id": work.symbol_id, "address": work.address,
                **raw_target_divergence,
            }
            caller_divergences.append(divergence)
            if first_divergence is None:
                first_divergence = divergence
            _cc_reporting._add_elapsed_ms(work.timings_ms, "extraction_compare", work.comparison_started)
            work.trace_caller(
                "divergence-retained", caller_index=work.caller_index,
                symbol_id=work.symbol_id, address=work.address, divergence_side="comparison",
            )
            if not collect_all_divergences:
                break
            continue
        inline_absence_proof = (
            work.candidate_assembly.intentionally_inlined_absence_proof
        )
        if inline_absence_proof is not None:
            if not _cc_source._valid_intentionally_inlined_absence_proof(
                inline_absence_proof,
                symbol_id=work.symbol_id,
                address=work.address,
                target_id=candidate_target_id,
            ):
                raise _cc_errors.CallContractBodyError(
                    symbol_id=work.symbol_id,
                    address=work.address,
                    side="candidate",
                    message=(
                        "candidate inline-absence proof is malformed or bound "
                        "to a different exact body"
                    ),
                )
            work.candidate: list[dict[str, Any]] = []
            expected_by_symbol[work.symbol_id] = work.expected
            candidate_by_symbol[work.symbol_id] = work.candidate
            evaluated_symbol_ids.add(work.symbol_id)
            inline_absence_proofs_by_symbol[work.symbol_id] = dict(
                inline_absence_proof
            )
            comparison = _cc_comparison.compare_call_contracts(work.expected, work.candidate)
            _cc_reporting._add_elapsed_ms(
                work.timings_ms,
                "extraction_compare",
                work.comparison_started,
            )
            if not comparison["passed"]:
                _cc_comparison._capture_call_contract_diagnostic_context(
                    _diagnostic_comparison_context,
                    symbol_id=work.symbol_id,
                    address=work.address,
                    expected=work.expected,
                    candidate=work.candidate,
                )
                divergence = {
                    "symbol_id": work.symbol_id,
                    "address": work.address,
                    **comparison["first_divergence"],
                }
                caller_divergences.append(divergence)
                if first_divergence is None:
                    first_divergence = divergence
                if not collect_all_divergences:
                    break
            continue
        try:
            _cc_phase_candidate_iat_and_abi.recover_candidate_iat_and_abi(work)
            _cc_phase_candidate_providers.recover_candidate_providers(work)
            _cc_phase_candidate_dispatch.recover_candidate_dispatch(work)
            _cc_phase_candidate_storage.recover_candidate_storage(work)
            _cc_phase_candidate_receivers.recover_candidate_receivers(work)
            _cc_phase_candidate_lifecycle.recover_candidate_lifecycle(work)
            _cc_phase_selected_dependencies.compare_selected_dependencies(work)
        except (OSError, RuntimeError, ValueError) as exc:
            body_error = _cc_errors.CallContractBodyError(
                symbol_id=work.symbol_id,
                address=work.address,
                side="candidate",
                message=str(exc),
            )
            if not collect_all_divergences:
                raise body_error from exc
            divergence = {
                "symbol_id": work.symbol_id,
                "address": work.address,
                "kind": "verifier-blocked",
                "side": "candidate",
                "message": str(exc),
            }
            if hasattr(exc, "diagnostic"):
                divergence["proof_diagnostic"] = dict(exc.diagnostic)
            if isinstance(exc, _cc_errors.CandidateCallbackAuthorityError):
                work.trace_caller(
                    "provenance-routing-start",
                    caller_index=work.caller_index,
                    symbol_id=work.symbol_id,
                    address=work.address,
                    provenance_kind="callback-authority",
                )
                divergence["dependent_authority_provenance"] = deepcopy(
                    exc.provenance
                )
                work.trace_caller(
                    "provenance-routing-complete",
                    caller_index=work.caller_index,
                    symbol_id=work.symbol_id,
                    address=work.address,
                    provenance_kind="callback-authority",
                )
            if (
                isinstance(exc, _cc_errors.CandidateCallTargetBridgeError)
                and not _cc_reporting._r4564_candidate_extra_helper_is_caller_source(
                    caller_address=work.address,
                    caller_end_exclusive=work.end_exclusive,
                    error=exc,
                )
            ):
                work.trace_caller(
                    "owner-routing-start",
                    caller_index=work.caller_index,
                    symbol_id=work.symbol_id,
                    address=work.address,
                    provenance_kind="dependent-owner",
                )
                dependent_owner_provenance = (
                    _cc_source._candidate_dependent_owner_provenance(
                        work.document,
                        source_closure,
                        caller_symbol_id=work.symbol_id,
                        caller_address=work.address,
                        caller_end_exclusive=work.end_exclusive,
                        caller_target_id=candidate_target_id,
                        caller_source_edit_paths=caller_source_edit_paths,
                        error=exc,
                    )
                )
                divergence["dependent_owner_provenance"] = (
                    dependent_owner_provenance
                )
                work.trace_caller(
                    "owner-routing-complete",
                    caller_index=work.caller_index,
                    symbol_id=work.symbol_id,
                    address=work.address,
                    provenance_kind="dependent-owner",
                    owner_relation=str(
                        dependent_owner_provenance.get(
                            "owner_relation", ""
                        )
                    ),
                )
            caller_divergences.append(divergence)
            if first_divergence is None:
                first_divergence = divergence
            work.trace_caller(
                "divergence-retained",
                caller_index=work.caller_index,
                symbol_id=work.symbol_id,
                address=work.address,
                divergence_side="candidate",
            )
            continue
        expected_by_symbol[work.symbol_id] = work.expected
        candidate_by_symbol[work.symbol_id] = work.candidate
        evaluated_symbol_ids.add(work.symbol_id)
        work.trace_caller(
            "contracts-retained",
            caller_index=work.caller_index,
            symbol_id=work.symbol_id,
            address=work.address,
            expected_invocation_count=len(work.expected),
            candidate_invocation_count=len(work.candidate),
        )
        provider_iat_equivalences = (
            _cc_comparison._compose_reviewed_provider_iat_equivalences(
                work.gettickcount_candidate_retail_iat_equivalences,
                work.named_thunk_candidate_iat_equivalences,
                work.named_thunk_candidate_direct_equivalences,
                work.zwep_ciacos_provider_iat_equivalences,
                work.r4564_provider_iat_equivalences,
            )
        )
        direct_target_equivalences = (
            {
                _cc_catalog.MSVC_FTOL_IAT_STORAGE_IDENTITY:
                work.ftol_provider_identity,
            }
            if work.ftol_provider_bridges
            else {}
        )
        work.trace_caller(
            "comparison-start",
            caller_index=work.caller_index,
            symbol_id=work.symbol_id,
            address=work.address,
        )
        comparison = _cc_comparison.compare_call_contracts(
            work.expected,
            work.candidate,
            reviewed_direct_target_equivalences=direct_target_equivalences,
            reviewed_provider_iat_equivalences=(
                provider_iat_equivalences
            ),
        )
        physical_results = _cc_contributions.contribution_results(work.retail_physical_contributions, work.candidate_physical_contributions)
        comparison["proof_results"].extend(result.as_json() for result in physical_results)
        physical_failure = next((result for result in physical_results if result.status.value != "proven"), None)
        if physical_failure is not None:
            comparison["passed"] = False
            comparison["first_divergence"] = {"kind": "physical-contribution-mismatch", **physical_failure.as_json()}
        proof_diagnostics_by_symbol[work.symbol_id] = comparison["proof_results"]
        work.trace_caller(
            "comparison-complete",
            caller_index=work.caller_index,
            symbol_id=work.symbol_id,
            address=work.address,
            comparison_passed=bool(comparison["passed"]),
        )
        _cc_reporting._add_elapsed_ms(
            work.timings_ms,
            "extraction_compare",
            work.comparison_started,
        )
        if not comparison["passed"]:
            _cc_comparison._capture_call_contract_diagnostic_context(
                _diagnostic_comparison_context,
                symbol_id=work.symbol_id,
                address=work.address,
                expected=work.expected,
                candidate=work.candidate,
                reviewed_direct_target_equivalences=(
                    direct_target_equivalences
                ),
                reviewed_provider_iat_equivalences=(
                    provider_iat_equivalences
                ),
            )
            divergence = {
                "symbol_id": work.symbol_id,
                "address": work.address,
                **comparison["first_divergence"],
            }
            caller_divergences.append(divergence)
            if first_divergence is None:
                first_divergence = divergence
            work.trace_caller(
                "divergence-retained",
                caller_index=work.caller_index,
                symbol_id=work.symbol_id,
                address=work.address,
                divergence_side="comparison",
            )
            if not collect_all_divergences:
                break
    memory_trace.emit(
        "caller-loop-complete",
        caller_total=caller_total,
        candidate_cache_count=len(candidate_session._candidates),
        callable_inventory_cache_count=len(
            getattr(
                document,
                "_call_contract_callable_inventory_cache_v1",
                {},
            )
        ),
        expected_result_count=len(expected_by_symbol),
        candidate_result_count=len(candidate_by_symbol),
        divergence_count=len(caller_divergences),
    )
    definition_compile_started = time.perf_counter()
    memory_trace.emit(
        "definition-closure-compilation-start",
        caller_total=caller_total,
        definition_source_path_count=len(
            source_closure.definition_source_paths
        ),
    )
    if compile_definition_closure:
        definition_compile_results = (
            _cc_source._compile_call_contract_definition_sources(
                source_closure,
                build_root=build_root,
                vc5_env=vc5_env,
            )
            if compile_definition_closure_on_divergence
            else _cc_source._compile_definition_closure_after_comparison(
                first_divergence,
                source_closure,
                build_root=build_root,
                vc5_env=vc5_env,
            )
        )
    else:
        definition_compile_results = ()
    memory_trace.emit(
        "definition-closure-compilation-complete",
        caller_total=caller_total,
        definition_compile_result_count=len(definition_compile_results),
    )
    _cc_reporting._add_elapsed_ms(
        timings_ms,
        "definition_compilation",
        definition_compile_started,
    )
    signature_started = time.perf_counter()
    signatures_after = _cc_source.file_dependency_states(
        dependency_paths,
        repository_path_inventory=repository_path_inventory,
    )
    first_divergence, source_changed_during_validation = (
        _cc_source._source_signature_recheck(
            first_divergence,
            signatures_before,
            signatures_after,
        )
    )
    _cc_reporting._add_elapsed_ms(timings_ms, "signature_recheck", signature_started)
    candidate_cod_index = candidate_session.cod_listing_index_metrics()
    timings_ms["candidate_cod_file_index"] = float(
        candidate_cod_index["file_index_ms"]
    )
    timings_ms["candidate_cod_procedure_lookup"] = float(
        candidate_cod_index["procedure_lookup_ms"]
    )
    timings_ms["total"] = round(
        (time.perf_counter() - total_started) * 1000.0,
        3,
    )
    memory_trace.emit(
        "result-construction-start",
        caller_total=caller_total,
        expected_result_count=len(expected_by_symbol),
        candidate_result_count=len(candidate_by_symbol),
        divergence_count=len(caller_divergences),
    )
    body_results = _cc_reporting._call_contract_body_results(
        document=document,
        slice_row=slice_row,
        candidate_session=candidate_session,
        expected_by_symbol=expected_by_symbol,
        candidate_by_symbol=candidate_by_symbol,
        caller_divergences=caller_divergences,
        evaluated_symbol_ids=evaluated_symbol_ids,
    )
    for row in body_results:
        diagnostics = proof_diagnostics_by_symbol.get(row["symbol_id"])
        if diagnostics is None:
            divergence = row.get("divergence") or {}
            diagnostics = [divergence["proof_diagnostic"]] if "proof_diagnostic" in divergence else [{
                "family": "evidence-acquisition", "status": "unresolved",
                "reason": divergence.get("message", row.get("status", "not-evaluated"))}]
        row["proof_diagnostics"] = diagnostics
    if source_changed_during_validation:
        for row in body_results:
            row["comparison_passed"] = False
            row["status"] = "blocked"
            row["divergence"] = {
                "kind": "source-changed-during-verification",
                "symbol_id": row["symbol_id"],
            }
    retail_fact_transcript = [
        {
            "symbol_id": row["symbol_id"],
            "address": row["address"],
            "expected_fact_row": deepcopy(row["expected_fact_row"]),
        }
        for row in body_results
        if row.get("expected_fact_row") is not None
    ]
    routed_caller_divergences: list[dict[str, Any]] = []
    for raw_divergence in caller_divergences:
        divergence = deepcopy(dict(raw_divergence))
        routing = _cc_candidate_session._continuation_repair_routing(divergence)
        if routing is not None:
            divergence["repair_routing"] = routing
        routed_caller_divergences.append(divergence)
    if caller_divergences:
        caller_divergences = routed_caller_divergences
        first_divergence = deepcopy(caller_divergences[0])
    result = {
        "kind": (
            "authored-call-contract-phase-replay-result"
            if phase_mode
            else (
                "authored-call-contract-target-convergence-result"
                if target_mode
                else "authored-call-contract-live-result"
            )
        ),
        "contract_version": CALL_CONTRACT_CONTRACT_VERSION,
        "closure_contract_version": 2,
        "slice_id": result_slice_id,
        "body_count": slice_row["body_count"],
        "symbol_ids": slice_row["symbol_ids"],
        "target_ids": slice_row["target_ids"],
        "physical_block_ids": slice_row["physical_block_ids"],
        "source_paths": slice_row["source_paths"],
        "source_edit_paths": list(source_closure.source_edit_paths),
        "source_write_paths": list(source_closure.source_edit_paths),
        "definition_source_paths": list(
            source_closure.definition_source_paths
        ),
        "definition_compile_results": list(definition_compile_results),
        "dependency_paths": dependency_paths,
        "definition_resolution": dict(
            source_closure.definition_resolution
        ),
        "dependency_states_before": signatures_before,
        "dependency_states_after": signatures_after,
        "source_changed_during_validation": source_changed_during_validation,
        "passed": first_divergence is None,
        "expected_contracts": expected_by_symbol,
        "candidate_contracts": candidate_by_symbol,
        "retail_provenance_adapter_sites_by_symbol": (
            retail_provenance_adapter_sites_by_symbol
        ),
        "candidate_inline_absence_proofs": (
            inline_absence_proofs_by_symbol
        ),
        "candidate_cleanup_receipts": candidate_cleanup_receipts_by_symbol,
        "candidate_expansion_receipts": candidate_expansion_receipts_by_symbol,
        "candidate_toolchain_stability_receipts": deepcopy(
            candidate_session.toolchain_receipts
        ),
        "body_results": body_results,
        "exact_fact_transcript": retail_fact_transcript,
        "provider_fact_transcript": retail_provider_fact_transcript,
        "first_divergence": first_divergence,
        "caller_divergences": caller_divergences,
        "expected_truth": "retail-binary-ninja-plus-reviewed-tracker-identities",
        "candidate_expected_truth": False,
        "candidate_cod_index": candidate_cod_index,
        "all_caller_divergences_collected": bool(
            collect_all_divergences
            and all(row.get("status") != "not-evaluated" for row in body_results)
        ),
        "timings_ms": timings_ms,
    }
    if not source_changed_during_validation:
        if first_divergence is None and caller_divergences:
            raise ValueError(
                "caller diagnostic divergences exist without first_divergence"
            )
        if (
            first_divergence is not None
            and (
                not caller_divergences
                or first_divergence != caller_divergences[0]
            )
        ):
            raise ValueError(
                "caller diagnostic census does not preserve the earliest "
                "first_divergence"
            )
    if target_mode:
        result["caller_census"] = _caller_isolated_diagnostic_census(
            symbol_ids=slice_row["symbol_ids"],
            addresses=slice_row["addresses"],
            expected_contracts=expected_by_symbol,
            candidate_contracts=candidate_by_symbol,
            caller_divergences=caller_divergences,
            complete=bool(
                collect_all_divergences
                and not source_changed_during_validation
            ),
        )
    if target_mode:
        result.update(
            {
                "target_id": target_id,
                "all_authored_bodies": True,
                "original_slice_ids": list(
                    slice_row["original_slice_ids"]
                ),
                "acceptance_eligible": False,
                "nonaccepting": True,
                "acceptance_route": None,
                "compiled_target_ids": list(
                    candidate_session.compiled_target_ids
                ),
                "selected_target_compile_count": (
                    candidate_session.compiled_target_ids.count(target_id)
                ),
                "all_caller_divergences_collected": collect_all_divergences,
                "definition_closure_deferred": not compile_definition_closure,
            }
        )
    if phase_mode:
        result.update(
            {
                "phase_all_authored_bodies": True,
                "original_slice_ids": list(slice_row["original_slice_ids"]),
                "slice_boundaries": deepcopy(slice_row["slice_boundaries"]),
                "acceptance_eligible": False,
                "nonaccepting": True,
                "acceptance_route": "project-to-original-slices",
                "compiled_target_ids": list(candidate_session.compiled_target_ids),
                "attempted_target_ids": list(candidate_session.attempted_target_ids),
                "binary_ninja_fact_cache": (
                    phase_bridge_cache.metrics()
                    if phase_bridge_cache is not None
                    else {}
                ),
                "definition_closure_deferred": not compile_definition_closure,
            }
        )
    memory_trace.emit(
        "result-construction-complete",
        caller_total=caller_total,
        result_key_count=len(result),
        expected_result_count=len(expected_by_symbol),
        candidate_result_count=len(candidate_by_symbol),
        divergence_count=len(caller_divergences),
        caller_census_present="caller_census" in result,
    )
    return result
