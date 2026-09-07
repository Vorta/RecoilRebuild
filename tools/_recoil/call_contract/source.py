"""Recoil call-contract source evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import candidate as _cc_candidate
from _recoil.call_contract import candidate_session as _cc_candidate_session
from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import errors as _cc_errors
from _recoil.call_contract import listing as _cc_listing

if TYPE_CHECKING:
    pass

import json
import os
import re
from dataclasses import dataclass, field, replace
from pathlib import Path, PurePosixPath
from typing import Any, Callable, Iterable, Mapping, Sequence

from _recoil.commands.asm_verify import (
    IMAGE_REL_I386_DIR32,
    IMAGE_SCN_CNT_CODE,
    IMAGE_SYM_CLASS_EXTERNAL,
    CoffObject,
)
from _recoil.commands.vc5_build import DEFAULT_MANIFEST as DEFAULT_FINAL_BUILD_MANIFEST
from _recoil.commands.vc5_build import build_paths as final_build_paths
from _recoil.commands.vc5_build import (
    effective_compile_flags as final_build_effective_compile_flags,
)
from _recoil.commands.vc5_build import load_config as load_final_build_config
from _recoil.commands.vc5_build import (
    make_compile_command as make_final_build_compile_command,
)
from _recoil.commands.vc5_build import read_log_tail as read_final_build_log_tail
from _recoil.commands.vc5_build import run_command as run_final_build_command
from _recoil.commands.vc5_build import with_explicit_build_dir as with_final_build_dir
from _recoil.commands.vc5_verify import (
    canonical_source_key,
    effective_source_compile_context,
    load_manifest,
    normalize_order_edit_paths,
    prepare_clean_build_dir,
    require_clean_target_source_fragments,
)
from _recoil.lib.cpp_definition_closure import _TOKEN_RE as _CPP_DEFINITION_TOKEN_RE
from _recoil.lib.cpp_definition_closure import (
    CallableInventory,
    CallableKey,
    DecodedCallableIdentity,
    callable_inventory,
    resolve_dependent_callable_owner,
    resolve_reviewed_definition_sources,
)
from _recoil.lib.cpp_definition_closure import (
    _callable_key as _cpp_definition_callable_key,
)
from _recoil.lib.cpp_definition_closure import (
    _mask_non_code as _mask_cpp_definition_non_code,
)
from _recoil.lib.progress import (
    ProgressDocument,
    ProgressError,
    address_value,
    is_current_accepted_state,
    normalize_address,
)
from _recoil.lib.repository_paths import (
    RepositoryPathError,
    RepositoryPathInventory,
    diagnose_historical_repository_path,
    load_repository_path_inventory,
    resolve_repository_file,
    validate_repository_relative_path,
)
from _recoil.lib.tooling import REPO_ROOT
from _recoil.lib.windows_identity import StableReadHandle


def file_dependency_states(
    paths: Iterable[str],
    *,
    repository_path_inventory: RepositoryPathInventory | None = None,
) -> list[dict[str, Any]]:
    """Return exact physical/stat state for an invocation-local TOCTOU check."""

    rows: list[dict[str, Any]] = []
    repository_inventory = (
        repository_path_inventory
        or load_repository_path_inventory(REPO_ROOT)
    )
    for raw_path in sorted({str(item).replace("\\", "/") for item in paths if str(item)}):
        try:
            tracked = resolve_repository_file(
                raw_path,
                context="call-contract dependency",
                repository_root=repository_inventory.repository_root,
                inventory=repository_inventory,
            )
            path = tracked.physical_path
            relative = tracked.repository_path
        except RepositoryPathError as exc:
            if exc.kind != "repository-file-missing":
                raise
            # The dependency was exact in the immutable invocation inventory
            # but disappeared before the physical signature read. Preserve the
            # repository identity so the TOCTOU recheck reports a missing row.
            relative = raw_path
            path = repository_inventory.repository_root / relative
        try:
            with StableReadHandle(path) as handle:
                stat = os.fstat(handle.stream.fileno())
                identity = handle.identity.to_dict()
        except FileNotFoundError:
            rows.append({"path": relative, "exists": False})
            continue
        rows.append(
            {
                "path": relative,
                "exists": True,
                "physical_identity": identity,
                "size": int(stat.st_size),
                "mtime_ns": int(stat.st_mtime_ns),
                "ctime_ns": int(stat.st_ctime_ns),
            }
        )
    return rows


def _call_contract_final_build_manifest_logical_path() -> str:
    """Return the configured manifest's lexical repo path, without resolve()."""

    try:
        value = Path(DEFAULT_FINAL_BUILD_MANIFEST).relative_to(REPO_ROOT).as_posix()
    except ValueError as exc:
        raise ProgressError(
            "call-contract final-build manifest is outside the executing repository"
        ) from exc
    try:
        return validate_repository_relative_path(
            value,
            context="call-contract final-build manifest",
        )
    except RepositoryPathError as exc:
        raise ProgressError(str(exc)) from exc


@dataclass(frozen=True)
class CallContractSourceClosure:
    source_edit_paths: tuple[str, ...]
    registered_source_paths: tuple[str, ...]
    header_paths: tuple[str, ...]
    definition_source_paths: tuple[str, ...]
    dependency_paths: tuple[str, ...]
    definition_resolution: Mapping[str, Any] = field(default_factory=dict)


def _call_contract_target_source_edit_paths(
    slice_row: Mapping[str, Any],
    closure: CallContractSourceClosure,
    *,
    require_explicit: bool,
) -> dict[str, tuple[str, ...]]:
    """Return one strict normalized writable closure per selected target."""

    raw_target_ids = slice_row.get("target_ids")
    target_id_keys = (
        [value.casefold() for value in raw_target_ids]
        if isinstance(raw_target_ids, list)
        and all(isinstance(value, str) for value in raw_target_ids)
        else []
    )
    if (
        not isinstance(raw_target_ids, list)
        or not raw_target_ids
        or any(not isinstance(value, str) or not value for value in raw_target_ids)
        or len(target_id_keys) != len(set(target_id_keys))
    ):
        raise ProgressError(
            "call-contract target source-edit lookup has malformed or duplicate target ids"
        )
    target_ids = tuple(raw_target_ids)
    raw_lookup = slice_row.get("target_source_edit_paths")
    if raw_lookup is None and not require_explicit:
        return {
            target_id: tuple(closure.source_edit_paths)
            for target_id in target_ids
        }
    if not isinstance(raw_lookup, Mapping):
        raise ProgressError(
            "phase call-contract target_source_edit_paths must be an object"
        )
    if set(raw_lookup) != set(target_ids):
        raise ProgressError(
            "phase call-contract target_source_edit_paths has missing or wrong target ids"
        )
    union_paths = set(closure.source_edit_paths)
    result: dict[str, tuple[str, ...]] = {}
    for target_id in target_ids:
        raw_paths = raw_lookup.get(target_id)
        path_keys = (
            [path.casefold() for path in raw_paths]
            if isinstance(raw_paths, list)
            and all(isinstance(path, str) for path in raw_paths)
            else []
        )
        if (
            not isinstance(raw_paths, list)
            or not raw_paths
            or any(not isinstance(path, str) or not path for path in raw_paths)
            or len(path_keys) != len(set(path_keys))
        ):
            raise ProgressError(
                f"phase call-contract target {target_id} has malformed or duplicate source-edit paths"
            )
        normalized: list[str] = []
        for path in raw_paths:
            posix = PurePosixPath(path)
            if (
                "\\" in path
                or posix.is_absolute()
                or ":" in path
                or any(part in {"", ".", ".."} for part in posix.parts)
                or posix.as_posix() != path
                or Path(path).suffix.casefold()
                not in (_cc_catalog.CALL_CONTRACT_SOURCE_SUFFIXES | _cc_catalog.CALL_CONTRACT_HEADER_SUFFIXES)
                or path not in union_paths
            ):
                raise ProgressError(
                    f"phase call-contract target {target_id} has escaping, unnormalized, or out-of-union source-edit path {path!r}"
                )
            normalized.append(path)
        result[target_id] = tuple(normalized)
    return result


def _call_contract_target_registrations(
    document: ProgressDocument,
    slice_row: Mapping[str, Any],
) -> list[tuple[str, Mapping[str, Any], str]]:
    target_ids = slice_row.get("target_ids")
    if not isinstance(target_ids, list) or not target_ids:
        raise ProgressError(
            "call-contract slice has no selected verification targets"
        )
    targets = document.collection("verification_targets")
    result: list[tuple[str, Mapping[str, Any], str]] = []
    for target_id in sorted({str(value) for value in target_ids}):
        target = targets.get(target_id)
        if not isinstance(target, Mapping) or target.get("kind") != "vc5":
            raise ProgressError(
                f"call-contract target {target_id} is not an exact VC5 target"
            )
        registration = target.get("registration")
        if not isinstance(registration, Mapping):
            raise ProgressError(
                f"call-contract target {target_id} has no registration"
            )
        manifest_path = registration.get("manifest_path")
        if not isinstance(manifest_path, str) or not manifest_path:
            raise ProgressError(
                f"call-contract target {target_id} has no manifest path"
            )
        result.append((target_id, registration, manifest_path))
    return result


def _current_target_semantic_paths(target: Any) -> tuple[str, ...]:
    """Return the current manifest's ordered repository source projection."""

    order_edit_paths = tuple(
        str(path) for path in getattr(target, "order_edit_paths", ()) if str(path)
    )
    if order_edit_paths:
        return order_edit_paths
    values: list[str] = []
    source_from = str(getattr(target, "source_from", ""))
    if source_from:
        values.append(source_from)
    values.extend(
        str(path) for path in getattr(target, "source_files", ()) if str(path)
    )
    values.extend(
        str(getattr(entry, "source_from", ""))
        for entry in getattr(target, "translation_unit_function_order", ())
        if str(getattr(entry, "source_from", ""))
    )
    return tuple(dict.fromkeys(values))


def call_contract_registration_path_reconciliation(
    *,
    target_id: str,
    registration: Mapping[str, Any],
    current_target: Any,
    inventory: RepositoryPathInventory,
) -> dict[str, Any]:
    """Compare historical registration paths with current manifest paths.

    Current manifest values must first authenticate as exact repository paths.  Old
    registration spelling may then be reconciled for diagnosis only; it never
    becomes a current input or mutates tracker state.
    """

    current_paths = _current_target_semantic_paths(current_target)
    if not current_paths:
        raise ProgressError(
            f"call-contract target {target_id} current manifest has no semantic source paths"
        )
    permitted_suffixes = (
        _cc_catalog.CALL_CONTRACT_SOURCE_SUFFIXES | _cc_catalog.CALL_CONTRACT_HEADER_SUFFIXES
    )
    exact_current: list[str] = []
    for index, path in enumerate(current_paths):
        try:
            tracked = resolve_repository_file(
                path,
                repository_root=inventory.repository_root,
                inventory=inventory,
                context=(
                    f"call-contract target {target_id} current manifest "
                    f"semantic path[{index}]"
                ),
                allowed_suffixes=permitted_suffixes,
            )
        except RepositoryPathError as exc:
            raise ProgressError(str(exc)) from exc
        exact_current.append(tracked.repository_path)
    if len({path.casefold() for path in exact_current}) != len(exact_current):
        raise ProgressError(
            f"call-contract target {target_id} current manifest repeats a semantic path"
        )

    raw_stored = registration.get("order_edit_paths")
    projection_field = "order_edit_paths"
    if raw_stored is None:
        # ``source_from`` and translation-unit order are distinct contracts,
        # not a historical copy of ``order_edit_paths``. Compare those fields
        # only with their like-for-like current-manifest projection.
        projection_field = "source_from-and-translation-unit-sources"
        raw_stored = []
        stored_source_from = registration.get("source_from")
        if isinstance(stored_source_from, str) and stored_source_from:
            raw_stored.append(stored_source_from)
        for row in registration.get("translation_unit_function_order") or []:
            if isinstance(row, Mapping):
                value = row.get("source_from")
                if isinstance(value, str) and value:
                    raw_stored.append(value)
        if raw_stored:
            like_for_like_current: list[str] = []
            current_source_from = str(getattr(current_target, "source_from", ""))
            if current_source_from:
                like_for_like_current.append(current_source_from)
            like_for_like_current.extend(
                str(getattr(row, "source_from", ""))
                for row in getattr(
                    current_target, "translation_unit_function_order", ()
                )
                if str(getattr(row, "source_from", ""))
            )
            exact_current = []
            for index, path in enumerate(dict.fromkeys(like_for_like_current)):
                try:
                    tracked = resolve_repository_file(
                        path,
                        repository_root=inventory.repository_root,
                        inventory=inventory,
                        context=(
                            f"call-contract target {target_id} current manifest "
                            f"{projection_field}[{index}]"
                        ),
                        allowed_suffixes=permitted_suffixes,
                    )
                except RepositoryPathError as exc:
                    raise ProgressError(str(exc)) from exc
                exact_current.append(tracked.repository_path)
        else:
            # No field-equivalent historical projection exists. The exact
            # current manifest remains the candidate input.
            return {
                "target_id": target_id,
                "status": "current-manifest-only",
                "blocker_kind": None,
                "registration_projection_present": False,
                "registration_projection_field": None,
                "stored": [],
                "current_manifest": list(exact_current),
                "removed": [],
                "added": [],
                "unchanged": [],
                "historical_case_aliases": [],
                "ambiguous_historical_paths": [],
                "case_only": False,
                "ordering_only": False,
                "order_semantic": False,
                "current": True,
                "tracker_mutated": False,
            }
    if (
        not isinstance(raw_stored, list)
        or any(not isinstance(path, str) or not path for path in raw_stored)
    ):
        raise ProgressError(
            f"call-contract target {target_id} registration has malformed semantic paths"
        )
    stored_paths = tuple(dict.fromkeys(str(path) for path in raw_stored))

    matched_current: set[str] = set()
    unchanged: list[str] = []
    case_aliases: list[dict[str, Any]] = []
    removed: list[str] = []
    ambiguous: list[dict[str, Any]] = []
    reconciled_order: list[str] = []
    for path in stored_paths:
        try:
            diagnosis = diagnose_historical_repository_path(
                path,
                inventory=inventory,
                current_allowed_paths=exact_current,
            )
        except RepositoryPathError as exc:
            raise ProgressError(str(exc)) from exc
        if diagnosis.status == "exact-historical" and diagnosis.current_repository_path:
            unchanged.append(path)
            matched_current.add(diagnosis.current_repository_path)
            reconciled_order.append(diagnosis.current_repository_path)
        elif (
            diagnosis.status == "historical-case-alias"
            and diagnosis.current_repository_path
        ):
            case_aliases.append(
                {
                    "status": diagnosis.status,
                    "historical_path": diagnosis.historical_path,
                    "current_repository_path": diagnosis.current_repository_path,
                    "current": diagnosis.current,
                    "tracker_mutated": diagnosis.tracker_mutated,
                }
            )
            matched_current.add(diagnosis.current_repository_path)
            reconciled_order.append(diagnosis.current_repository_path)
        elif diagnosis.status == "ambiguous":
            ambiguous.append(
                {
                    "historical_path": path,
                    "candidates": list(diagnosis.candidates),
                    "current": False,
                    "tracker_mutated": False,
                }
            )
        else:
            removed.append(path)

    added = [path for path in exact_current if path not in matched_current]
    ordering_only = bool(
        not removed
        and not added
        and not ambiguous
        and reconciled_order != exact_current
    )
    membership_drift = bool(removed or added or ambiguous)
    order_semantic = bool(
        projection_field == "source_from-and-translation-unit-sources"
        and registration.get("translation_unit_function_order")
    )
    semantic_order_drift = ordering_only and order_semantic
    status = (
        "manifest-registration-drift"
        if membership_drift or semantic_order_drift
        else "historical-case-alias"
        if case_aliases
        else "ordering-only"
        if ordering_only
        else "current"
    )
    return {
        "target_id": target_id,
        "status": status,
        "blocker_kind": (
            "manifest-registration-drift"
            if membership_drift or semantic_order_drift
            else None
        ),
        "stored": list(stored_paths),
        "registration_projection_present": True,
        "registration_projection_field": projection_field,
        "current_manifest": list(exact_current),
        "removed": removed,
        "added": added,
        "unchanged": unchanged,
        "historical_case_aliases": case_aliases,
        "ambiguous_historical_paths": ambiguous,
        "case_only": (
            bool(case_aliases)
            and not membership_drift
            and not semantic_order_drift
        ),
        "ordering_only": ordering_only,
        "order_semantic": order_semantic,
        "current": status == "current",
        "tracker_mutated": False,
    }


def _appframe_v10_base_profile_target(
    target_id: str,
    manifest_path: str,
    target: Any,
) -> Any:
    """Validate and retain the exact reviewed AppFrame base compile context.

    The v9 STL queue define suppressed two authored bodies from the registered
    one-TU population, so it is not a valid target-wide compile projection.
    This finite validator keeps the live manifest target unchanged and rejects
    target, population, profile, flag, or queue-define drift before compile.
    """

    if target_id != _cc_catalog.APPFRAME_V9_TARGET_ID:
        return target

    label = "AppFrame v10 base call-contract compile profile"
    if (
        manifest_path != _cc_catalog.APPFRAME_V9_MANIFEST_PATH
        or "\\" in manifest_path
        or getattr(target, "name", "") != _cc_catalog.APPFRAME_V9_TARGET_NAME
        or getattr(target, "target_binary", "") != "recoil"
        or Path(getattr(target, "manifest_path", "")).resolve()
        != (REPO_ROOT / _cc_catalog.APPFRAME_V9_MANIFEST_PATH).resolve()
        or getattr(target, "source_from", "") != _cc_catalog.APPFRAME_V9_SOURCE_PATH
        or getattr(target, "source_filename", "") != "RecoilApp.cpp"
        or tuple(getattr(target, "source_files", ())) != ()
        or not getattr(target, "check_translation_unit_function_order", False)
    ):
        raise ProgressError(
            f"{label} rejects target, manifest, source, or TU registration drift"
        )

    entries = tuple(getattr(target, "translation_unit_function_order", ()))
    functions = tuple(getattr(entries[0], "functions", ())) if len(entries) == 1 else ()
    if (
        len(entries) != 1
        or getattr(entries[0], "source_from", "") != _cc_catalog.APPFRAME_V9_SOURCE_PATH
        or getattr(entries[0], "order_scope", "") != "authored"
        or tuple(getattr(function, "address", "") for function in functions)
        != _cc_catalog.APPFRAME_V9_ADDRESSES
        or tuple(getattr(function, "symbol", "") for function in functions)
        != _cc_catalog.APPFRAME_V9_SYMBOLS
        or tuple(getattr(function, "pipeline_class", "") for function in functions)
        != ("non-authored", "non-authored") + ("authored",) * 17
        or tuple(
            getattr(function, "authored_order_role", "") for function in functions
        )
        != ("non-authored", "non-authored") + ("authored-body",) * 17
        or len(set(_cc_catalog.APPFRAME_V9_ADDRESSES)) != len(_cc_catalog.APPFRAME_V9_ADDRESSES)
    ):
        raise ProgressError(
            f"{label} rejects translation-unit or complete address population drift"
        )

    profile, flags = effective_source_compile_context(
        target, _cc_catalog.APPFRAME_V9_SOURCE_PATH
    )
    flags = tuple(str(flag) for flag in flags)
    folded = tuple(flag.upper() for flag in flags)
    queue_define_name = _cc_catalog.APPFRAME_V9_QUEUE_DEFINE[2:].upper()
    queue_defines = tuple(
        flag
        for flag in flags
        if flag.upper().startswith("/D")
        and flag[2:].partition("=")[0].upper() == queue_define_name
    )
    if (
        profile != _cc_catalog.APPFRAME_V9_PROFILE_NAME
        or flags != _cc_catalog.APPFRAME_V9_BASE_FLAGS
        or queue_defines
        or folded.count("/TP") != 1
        or folded.count("/MD") != 1
        or folded.count("/O2") != 1
        or folded.count("/OB1") != 1
        or folded.count("/GX") != 1
        or folded.count("/GR") != 1
        or folded.count("/ZP4") != 1
        or folded.count("/FACS") != 1
        or folded[-1] != "/FACS"
    ):
        raise ProgressError(
            f"{label} rejects effective profile, flag, define, or duplicate drift"
        )

    source_key = canonical_source_key(_cc_catalog.APPFRAME_V9_SOURCE_PATH)
    profile_rows = tuple(getattr(target, "source_compile_profiles", ()))
    flag_rows = tuple(getattr(target, "source_compile_flags", ()))
    if (
        [key for key, _value in profile_rows].count(source_key) != 1
        or [key for key, _value in flag_rows].count(source_key) != 1
    ):
        raise ProgressError(
            f"{label} requires one exact source-specific compile context"
        )
    return target


def _call_contract_slice_targets(
    document: ProgressDocument,
    slice_row: Mapping[str, Any],
    *,
    repository_path_inventory: RepositoryPathInventory | None = None,
) -> dict[str, Any]:
    """Load the exact selected target manifests without compiling them."""

    inventory = repository_path_inventory or load_repository_path_inventory(
        REPO_ROOT
    )
    result: dict[str, Any] = {}
    for target_id, _registration, manifest_path in (
        _call_contract_target_registrations(document, slice_row)
    ):
        try:
            repository_manifest = resolve_repository_file(
                manifest_path,
                repository_root=inventory.repository_root,
                inventory=inventory,
                context=f"call-contract target {target_id} manifest",
                allowed_suffixes={".json"},
            )
        except RepositoryPathError as exc:
            raise ProgressError(str(exc)) from exc
        target = load_manifest(
            repository_manifest.physical_path,
            repository_path_inventory=inventory,
        )
        require_clean_target_source_fragments(target)
        result[target_id] = _appframe_v10_base_profile_target(
            target_id,
            manifest_path,
            target,
        )
    return result


def _call_contract_repository_include_roots() -> tuple[tuple[str, Path], ...]:
    try:
        config = json.loads(
            DEFAULT_FINAL_BUILD_MANIFEST.read_text(encoding="utf-8")
        )
    except (OSError, json.JSONDecodeError) as exc:
        raise ProgressError(
            "call-contract include closure cannot read the final-build "
            f"include-root contract: {exc}"
        ) from exc
    raw_include_dirs = config.get("include_dirs")
    if not isinstance(raw_include_dirs, list):
        raise ProgressError(
            "call-contract final-build include_dirs must be a list"
        )

    root = REPO_ROOT.resolve()
    result: list[tuple[str, Path]] = []
    seen: set[str] = set()
    for index, raw_value in enumerate(raw_include_dirs):
        if not isinstance(raw_value, str) or not raw_value:
            raise ProgressError(
                "call-contract final-build include_dirs entries must be "
                f"non-empty strings: index {index}"
            )
        include_dir = Path(raw_value)
        candidate = (
            include_dir.resolve()
            if include_dir.is_absolute()
            else (root / include_dir).resolve()
        )
        try:
            candidate.relative_to(root)
        except ValueError:
            # External compiler/SDK roots are deliberately outside the
            # repository-local editable dependency closure.
            continue
        if include_dir.is_absolute():
            # An absolute repository include root is a physical configuration
            # defect, not a route for manufacturing a current logical path.
            raise ProgressError(
                "call-contract repository include root must use a "
                f"repository-relative spelling: {raw_value}"
            )
        try:
            logical_root = validate_repository_relative_path(
                raw_value,
                context="call-contract repository include root",
            )
        except RepositoryPathError as exc:
            raise ProgressError(str(exc)) from exc
        if not candidate.is_dir() and logical_root.startswith("support/"):
            canonical_support_candidate = (
                _cc_catalog.DEFAULT_REFERENCE.parents[1] / Path(logical_root)
            ).resolve()
            if canonical_support_candidate.is_dir():
                candidate = canonical_support_candidate
        if not candidate.is_dir():
            raise ProgressError(
                "call-contract repository include root does not exist or is "
                f"not a directory: {raw_value}"
            )
        key = str(candidate).casefold()
        if key not in seen:
            seen.add(key)
            result.append((logical_root, candidate))
    return tuple(result)


def _call_contract_final_build_source_map(
    config: Any,
    *,
    inventory: RepositoryPathInventory | None = None,
) -> tuple[
    dict[str, list[tuple[str, Path]]],
    tuple[dict[str, Any], ...],
]:
    """Project final-build source rows through exact current repository identities.

    The checked-in final-build configuration may retain historical case-only
    spellings.  Those spellings are diagnosed as stale and never become the
    current logical result; the returned map is keyed by exact current repository
    paths and uses their separately authenticated physical files.
    """

    repository_inventory = inventory or load_repository_path_inventory(REPO_ROOT)
    explicit_repository_paths = getattr(config, "source_repository_paths", None)
    if explicit_repository_paths is not None:
        if (
            not isinstance(explicit_repository_paths, (tuple, list))
            or len(explicit_repository_paths) != len(tuple(getattr(config, "sources", ())))
            or any(not isinstance(path, str) or not path for path in explicit_repository_paths)
        ):
            raise ProgressError(
                "call-contract explicit final-build source registry is malformed"
            )
        result: dict[str, list[tuple[str, Path]]] = {}
        for repository_path, configured_path in zip(explicit_repository_paths, config.sources):
            try:
                tracked = resolve_repository_file(
                    repository_path,
                    repository_root=repository_inventory.repository_root,
                    inventory=repository_inventory,
                    context="call-contract explicit final-build source",
                    allowed_suffixes=_cc_catalog.CALL_CONTRACT_SOURCE_SUFFIXES,
                )
            except RepositoryPathError as exc:
                raise ProgressError(str(exc)) from exc
            if os.path.normcase(str(Path(configured_path).resolve())) != os.path.normcase(
                str(tracked.physical_path)
            ):
                raise ProgressError(
                    "call-contract explicit final-build source does not match "
                    f"its exact repository file: {repository_path}"
                )
            result.setdefault(tracked.repository_path.casefold(), []).append(
                (tracked.repository_path, tracked.physical_path)
            )
        return result, ()
    try:
        repository_manifest = resolve_repository_file(
            _call_contract_final_build_manifest_logical_path(),
            repository_root=repository_inventory.repository_root,
            inventory=repository_inventory,
            context="call-contract final-build manifest",
            allowed_suffixes={".json"},
        )
        raw = json.loads(
            repository_manifest.physical_path.read_text(encoding="utf-8")
        )
    except (RepositoryPathError, OSError, json.JSONDecodeError) as exc:
        raise ProgressError(
            f"call-contract final-build source registry cannot be read: {exc}"
        ) from exc
    raw_sources = raw.get("sources") if isinstance(raw, Mapping) else None
    if (
        not isinstance(raw_sources, list)
        or any(not isinstance(path, str) or not path for path in raw_sources)
        or len(raw_sources) != len(tuple(getattr(config, "sources", ())))
    ):
        raise ProgressError(
            "call-contract final-build source registry is malformed or does "
            "not match the parsed configuration"
        )

    result: dict[str, list[tuple[str, Path]]] = {}
    diagnostics: list[dict[str, Any]] = []
    for raw_path, configured_path in zip(raw_sources, config.sources):
        try:
            diagnosis = diagnose_historical_repository_path(
                raw_path,
                inventory=repository_inventory,
            )
        except RepositoryPathError as exc:
            raise ProgressError(str(exc)) from exc
        if diagnosis.status not in {
            "exact-historical",
            "historical-case-alias",
        } or not diagnosis.current_repository_path:
            raise ProgressError(
                "call-contract final-build source has no unique current repository "
                f"identity: {raw_path} ({diagnosis.status})"
            )
        try:
            tracked = resolve_repository_file(
                diagnosis.current_repository_path,
                repository_root=repository_inventory.repository_root,
                inventory=repository_inventory,
                context="call-contract final-build current source",
                allowed_suffixes=_cc_catalog.CALL_CONTRACT_SOURCE_SUFFIXES,
            )
        except RepositoryPathError as exc:
            raise ProgressError(str(exc)) from exc
        if os.path.normcase(str(Path(configured_path).resolve())) != os.path.normcase(
            str(tracked.physical_path)
        ):
            raise ProgressError(
                "call-contract parsed final-build source does not match its "
                f"current repository file: {raw_path}"
            )
        result.setdefault(tracked.repository_path.casefold(), []).append(
            (tracked.repository_path, tracked.physical_path)
        )
        if diagnosis.status == "historical-case-alias":
            diagnostics.append(
                {
                    "status": diagnosis.status,
                    "historical_path": diagnosis.historical_path,
                    "current_repository_path": diagnosis.current_repository_path,
                    "current": diagnosis.current,
                    "tracker_mutated": diagnosis.tracker_mutated,
                }
            )
    return result, tuple(diagnostics)


def _call_contract_vc5_preprocessor_defines(
    config: Any,
    source: Path,
) -> tuple[str, ...]:
    """Return the governed VC5 macro context for one final-build TU."""

    values: dict[str, str] = {}
    for raw in getattr(config, "defines", ()):
        name, separator, value = str(raw).partition("=")
        values[name] = value if separator and value else "1"
    flags = tuple(final_build_effective_compile_flags(config, source))
    upper_flags = tuple(flag.upper() for flag in flags)
    values.update({"_MSC_VER": "1100", "_WIN32": "1"})
    architecture = "300"
    for generation in (3, 4, 5, 6):
        if f"/G{generation}" in upper_flags:
            architecture = str(generation * 100)
    values["_M_IX86"] = architecture
    if any(flag in {"/MD", "/MDD", "/MT", "/MTD"} for flag in upper_flags):
        values["_MT"] = "1"
    if any(flag in {"/MD", "/MDD"} for flag in upper_flags):
        values["_DLL"] = "1"
    if any(flag in {"/MDD", "/MTD", "/LDd".upper()} for flag in upper_flags):
        values["_DEBUG"] = "1"
    if "/GX" in upper_flags and "/GX-" not in upper_flags:
        values["_CPPUNWIND"] = "1"
    for raw_flag in flags:
        if raw_flag.upper().startswith("/D") and len(raw_flag) > 2:
            name, separator, value = raw_flag[2:].partition("=")
            if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
                raise ProgressError(
                    f"call-contract final-build compile flag has invalid /D macro: {raw_flag}"
                )
            values[name] = value if separator and value else "1"
    return tuple(
        f"{name}={value}"
        for name, value in sorted(values.items())
    )


def _call_contract_cached_manifest(
    document: ProgressDocument,
    manifest_path: Path,
    *,
    repository_path_inventory: Any | None = None,
) -> Any:
    """Reuse one unchanged governed target manifest within this request."""

    canonical = manifest_path.resolve()
    try:
        before = canonical.stat()
    except OSError as exc:
        raise ProgressError(
            f"call-contract target manifest cannot be read: {manifest_path}: {exc}"
        ) from exc
    signature = (True, int(before.st_size), int(before.st_mtime_ns))
    cache_name = "_call_contract_target_manifest_cache_v1"
    cache = getattr(document, cache_name, None)
    if not isinstance(cache, dict):
        cache = {}
        setattr(document, cache_name, cache)
    cache_key = (canonical.as_posix(), signature)
    if cache_key in cache:
        return cache[cache_key]
    target = load_manifest(
        canonical,
        enforce_source_policy=False,
        repository_path_inventory=repository_path_inventory,
    )
    try:
        after = canonical.stat()
    except OSError as exc:
        raise ProgressError(
            "call-contract target manifest changed while loading: "
            f"{manifest_path}: {exc}"
        ) from exc
    if (True, int(after.st_size), int(after.st_mtime_ns)) != signature:
        raise ProgressError(
            "call-contract target manifest changed while loading: "
            f"{manifest_path}"
        )
    for stale_key in tuple(cache):
        if (
            isinstance(stale_key, tuple)
            and len(stale_key) == 2
            and stale_key[0] == cache_key[0]
            and stale_key != cache_key
        ):
            del cache[stale_key]
    cache[cache_key] = target
    return target


def _call_contract_callable_inventory_resolver(
    document: ProgressDocument,
    *,
    root: Path | None = None,
) -> Callable[[str, tuple[str, ...]], CallableInventory]:
    """Return a request-local, document-bound callable inventory resolver.

    Reuse is valid only for the same canonical file, exact governed define
    tuple, and current existence/size/mtime signature.  A file changing during
    the read or parse fails closed; nothing is persisted or shared with another
    ``ProgressDocument``.
    """

    repository_root = (root or REPO_ROOT).resolve()
    cache_name = "_call_contract_callable_inventory_cache_v1"
    cache = getattr(document, cache_name, None)
    if not isinstance(cache, dict):
        cache = {}
        setattr(document, cache_name, cache)
    path_cache_name = "_call_contract_callable_inventory_paths_v1"
    path_cache = getattr(document, path_cache_name, None)
    if not isinstance(path_cache, dict):
        path_cache = {}
        setattr(document, path_cache_name, path_cache)

    def signature(path: Path) -> tuple[bool, int, int]:
        try:
            stat = path.stat()
        except FileNotFoundError:
            return (False, 0, 0)
        except OSError as exc:
            raise ProgressError(
                "call-contract definition closure cannot stat "
                f"{path}: {exc}"
            ) from exc
        return (True, int(stat.st_size), int(stat.st_mtime_ns))

    def resolve(
        canonical_relative: str,
        defines: tuple[str, ...],
    ) -> CallableInventory:
        candidate = path_cache.get(canonical_relative)
        if not isinstance(candidate, Path):
            candidate = (repository_root / canonical_relative).resolve()
            try:
                candidate.relative_to(repository_root)
            except ValueError as exc:
                raise ProgressError(
                    "call-contract inventory path leaves the repository: "
                    f"{canonical_relative}"
                ) from exc
            path_cache[canonical_relative] = candidate
        canonical_path = candidate.as_posix()
        context = tuple(str(value) for value in defines)
        before = signature(candidate)
        cache_key = (canonical_path, context, before)
        cached = cache.get(cache_key)
        if isinstance(cached, CallableInventory):
            return cached
        if not before[0]:
            raise ProgressError(
                "call-contract definition closure source disappeared: "
                f"{canonical_relative}"
            )
        try:
            text = candidate.read_text(encoding="utf-8", errors="ignore")
        except OSError as exc:
            raise ProgressError(
                "call-contract definition closure cannot read "
                f"{canonical_relative}: {exc}"
            ) from exc
        after_read = signature(candidate)
        if after_read != before:
            raise ProgressError(
                "call-contract definition closure source changed while reading: "
                f"{canonical_relative}"
            )
        inventory = callable_inventory(text, defines=context)
        after_parse = signature(candidate)
        if after_parse != before:
            raise ProgressError(
                "call-contract definition closure source changed while parsing: "
                f"{canonical_relative}"
            )
        for stale_key in tuple(cache):
            if (
                isinstance(stale_key, tuple)
                and len(stale_key) == 3
                and stale_key[0] == canonical_path
                and stale_key[1] == context
                and stale_key != cache_key
            ):
                del cache[stale_key]
        cache[cache_key] = inventory
        return inventory

    return resolve


def _call_contract_source_closure_uncached(
    document: ProgressDocument,
    slice_row: Mapping[str, Any],
    *,
    repository_path_inventory: RepositoryPathInventory | None = None,
) -> CallContractSourceClosure:
    """Return exact registered/header/definition paths for one slice.

    Registered C/C++ implementation paths are the only traversal roots.
    Repository-local headers enter the closure only through a transitive quoted
    include from one of those roots. Matching implementation TUs are then
    derived from qualified declaration/definition keys under their governed
    final-build preprocessor contexts; callee identity alone grants no path.
    """

    repository_inventory = (
        repository_path_inventory
        or load_repository_path_inventory(REPO_ROOT)
    )
    root = repository_inventory.repository_root

    def canonical_existing_file(
        candidate: Path,
        *,
        context: str,
        required: bool,
    ) -> tuple[Path, str] | None:
        # This helper is used only for include traversal candidates assembled
        # from a current exact repository path plus a lexical quoted include.  Use the
        # inventory for the logical result; physical spelling is never
        # projected back into that result.
        try:
            lexical_relative = candidate.relative_to(root).as_posix()
        except ValueError as exc:
            raise ProgressError(
                f"{context} leaves the repository: {candidate}"
            ) from exc
        matches = repository_inventory.casefolded_paths.get(
            lexical_relative.casefold(), ()
        )
        if lexical_relative in repository_inventory.exact_paths:
            repository_path = lexical_relative
        elif not matches and not required:
            return None
        elif len(matches) == 1:
            # A quoted include operand is relative to compiler include roots,
            # not itself a repository-relative identity.  Resolve the unique
            # repository candidate for traversal while retaining only its exact
            # repository spelling in the public closure.
            repository_path = matches[0]
        elif not matches:
            raise ProgressError(f"{context} is outside the repository inventory: {candidate}")
        else:
            raise ProgressError(
                f"{context} has ambiguous repository casing: {candidate}"
            )
        try:
            tracked = resolve_repository_file(
                repository_path,
                context=context,
                repository_root=root,
                inventory=repository_inventory,
            )
        except RepositoryPathError as exc:
            if not required and exc.kind in {"unknown-path", "repository-file-missing"}:
                return None
            raise ProgressError(str(exc)) from exc
        return tracked.physical_path, tracked.repository_path

    def normalized_source_roots(
        value: Any,
        *,
        context: str,
        include_headers: bool = False,
    ) -> tuple[str, ...]:
        """Filter path metadata to exact C/C++ traversal/edit roots.

        Headers are validated as repo-local C/C++ metadata but enter only
        through the transitive quoted-include closure. Exact ``semantic:``
        no-suffix placement labels are ignored; every other non-C/C++ value
        fails closed. Source-looking values still pass through existence and
        exact-casing validation before becoming roots.
        """

        if value is None:
            return ()
        if not isinstance(value, list):
            raise ProgressError(f"{context}: order_edit_paths must be a list")
        selected: list[str] = []
        permitted_suffixes = (
            _cc_catalog.CALL_CONTRACT_SOURCE_SUFFIXES | _cc_catalog.CALL_CONTRACT_HEADER_SUFFIXES
        )
        for index, item in enumerate(value):
            if not isinstance(item, str) or not item:
                raise ProgressError(
                    f"{context}: order_edit_paths[{index}] must be a non-empty string"
                )
            suffix = Path(item).suffix.casefold()
            if item.startswith("semantic:") and not suffix:
                continue
            if suffix not in permitted_suffixes:
                raise ProgressError(
                    f"{context}: order_edit_paths[{index}] is not a C/C++ source or header: {item}"
                )
            if suffix in _cc_catalog.CALL_CONTRACT_SOURCE_SUFFIXES or include_headers:
                selected.append(item)
        try:
            return normalize_order_edit_paths(
                selected,
                context=context,
                repository_root=root,
                inventory=repository_inventory,
            )
        except ValueError as exc:
            raise ProgressError(str(exc)) from exc

    registered_paths: list[str] = []
    source_edit_paths: list[str] = []
    # Original slice source_paths are historical scheduling evidence.  They
    # must not override the current tracked target manifests or become current
    # merely through case-fold reconciliation.
    raw_overrides = slice_row.get("source_edit_paths_override", {})
    if raw_overrides is None:
        raw_overrides = {}
    if not isinstance(raw_overrides, Mapping):
        raise ProgressError(
            "call-contract internal source_edit_paths_override must be an object"
        )

    for target_id, registration, manifest_value in (
        _call_contract_target_registrations(document, slice_row)
    ):
        try:
            repository_manifest = resolve_repository_file(
                manifest_value,
                repository_root=root,
                inventory=repository_inventory,
                context=f"call-contract target {target_id} manifest",
                allowed_suffixes={".json"},
            )
        except RepositoryPathError as exc:
            raise ProgressError(str(exc)) from exc
        manifest = _call_contract_cached_manifest(
            document,
            repository_manifest.physical_path,
            repository_path_inventory=repository_inventory,
        )
        raw_order_edit_paths = _cc_candidate_session._call_contract_target_source_edit_roots(
            target_id=target_id,
            registration=registration,
            manifest=manifest,
            override_paths=raw_overrides.get(target_id),
            strict_internal_scope=(
                slice_row.get("selection_mode")
                in {
                    "target-all-authored-bodies",
                    "phase-all-authored-bodies",
                }
            ),
            repository_path_inventory=repository_inventory,
        )
        target_edit_paths = normalized_source_roots(
            raw_order_edit_paths,
            context=f"call-contract target {target_id}",
            include_headers=True,
        )
        source_edit_paths.extend(target_edit_paths)
        registered_paths.extend(
            path
            for path in target_edit_paths
            if Path(path).suffix.casefold() in _cc_catalog.CALL_CONTRACT_SOURCE_SUFFIXES
        )

    editable: dict[str, str] = {}
    included_paths: dict[str, str] = {}
    registered_sources: dict[str, str] = {}
    pending: list[tuple[str, Path]] = []
    for edit_path in source_edit_paths:
        try:
            tracked = resolve_repository_file(
                edit_path,
                repository_root=root,
                inventory=repository_inventory,
                context="call-contract registered source-edit path",
                allowed_suffixes=(
                    _cc_catalog.CALL_CONTRACT_SOURCE_SUFFIXES
                    | _cc_catalog.CALL_CONTRACT_HEADER_SUFFIXES
                ),
            )
        except RepositoryPathError as exc:
            raise ProgressError(str(exc)) from exc
        editable.setdefault(tracked.repository_path.casefold(), tracked.repository_path)
    for registered_path in registered_paths:
        if Path(registered_path).suffix.casefold() not in (
            _cc_catalog.CALL_CONTRACT_SOURCE_SUFFIXES
        ):
            continue
        try:
            tracked = resolve_repository_file(
                registered_path,
                repository_root=root,
                inventory=repository_inventory,
                context="call-contract registered source path",
                allowed_suffixes=_cc_catalog.CALL_CONTRACT_SOURCE_SUFFIXES,
            )
        except RepositoryPathError as exc:
            raise ProgressError(str(exc)) from exc
        key = tracked.repository_path.casefold()
        if key not in registered_sources:
            registered_sources[key] = tracked.repository_path
            included_paths[key] = tracked.repository_path
            pending.append((tracked.repository_path, tracked.physical_path))
    if not pending:
        raise ProgressError(
            "call-contract slice has no registered C/C++ source files"
        )

    include_roots = _call_contract_repository_include_roots()
    scanned: set[str] = set()
    while pending:
        source_repository_path, source_path = pending.pop(0)
        source_key = source_repository_path.casefold()
        if source_key in scanned:
            continue
        scanned.add(source_key)
        try:
            text = source_path.read_text(encoding="utf-8", errors="ignore")
        except OSError as exc:
            raise ProgressError(
                f"call-contract include closure cannot read {source_path}: {exc}"
            ) from exc
        for match in _cc_catalog.CALL_CONTRACT_INCLUDE_DIRECTIVE_RE.finditer(text):
            operand = match.group("operand").strip()
            if operand.startswith("<"):
                continue
            if not operand.startswith('"'):
                # Macro/system selection is outside the repository-local quoted
                # include contract.
                continue
            quoted = _cc_catalog.CALL_CONTRACT_QUOTED_INCLUDE_OPERAND_RE.fullmatch(
                operand
            )
            if quoted is None:
                raise ProgressError(
                    "call-contract source has a malformed quoted include at "
                    f"{source_repository_path}:"
                    f"{text.count(chr(10), 0, match.start()) + 1}"
                )
            include_text = quoted.group("path")
            include_posix = PurePosixPath(include_text)
            include_components = include_text.split("/")
            if (
                "\\" in include_text
                or include_posix.is_absolute()
                or re.match(r"^[A-Za-z]:", include_text)
                or ":" in include_text
                or any(
                    component in {"", ".", ".."}
                    for component in include_components
                )
            ):
                raise ProgressError(
                    "call-contract source has a malformed or escaping quoted "
                    f"include {include_text!r} in "
                    f"{source_repository_path}"
                )
            if include_posix.suffix.casefold() not in (
                _cc_catalog.CALL_CONTRACT_HEADER_SUFFIXES
            ):
                continue

            resolved_include: tuple[Path, str] | None = None
            source_parent = PurePosixPath(source_repository_path).parent.as_posix()
            candidate_roots = (
                (source_parent, source_path.parent),
                *include_roots,
            )
            seen_candidates: set[str] = set()
            for logical_root, include_root in candidate_roots:
                logical_candidate = (
                    f"{logical_root}/{include_text}"
                    if logical_root not in {"", "."}
                    else include_text
                )
                candidate = root.joinpath(*logical_candidate.split("/"))
                candidate_key = logical_candidate.casefold()
                if candidate_key in seen_candidates:
                    continue
                seen_candidates.add(candidate_key)
                resolved_include = canonical_existing_file(
                    candidate,
                    context=(
                        "call-contract quoted include "
                        f"{include_text!r} from "
                        f"{source_repository_path}"
                    ),
                    required=False,
                )
                if resolved_include is not None:
                    break
            if resolved_include is None:
                continue
            resolved, canonical_relative = resolved_include
            key = canonical_relative.casefold()
            if key not in included_paths:
                included_paths[key] = canonical_relative
                pending.append((canonical_relative, resolved))

    header_paths = tuple(
        sorted(
            (
                path
                for path in included_paths.values()
                if Path(path).suffix.casefold() in _cc_catalog.CALL_CONTRACT_HEADER_SUFFIXES
            ),
            key=lambda path: (path.casefold(), path),
        )
    )
    final_config = load_final_build_config(DEFAULT_FINAL_BUILD_MANIFEST)
    context_by_source: dict[str, tuple[str, ...]] = {}
    source_texts: list[tuple[str, str, tuple[str, ...]]] = []
    for source in final_config.sources:
        canonical = canonical_existing_file(
            source,
            context="call-contract final-build source",
            required=True,
        )
        assert canonical is not None
        resolved, canonical_relative = canonical
        context = _call_contract_vc5_preprocessor_defines(
            final_config,
            resolved,
        )
        context_by_source[canonical_relative.casefold()] = context
        source_texts.append((canonical_relative, "", context))

    header_contexts = tuple(sorted(set(context_by_source.values())))
    header_texts: list[tuple[str, str, tuple[str, ...]]] = []
    for header_path in header_paths:
        header_texts.extend(
            (header_path, "", context) for context in header_contexts
        )
    inventory_resolver = _call_contract_callable_inventory_resolver(
        document, root=root
    )
    try:
        definition_resolution_result = resolve_reviewed_definition_sources(
            reviewed_callables=(),
            header_texts=header_texts,
            source_texts=source_texts,
            inventory_resolver=inventory_resolver,
        )
    except ValueError as exc:
        raise ProgressError(str(exc)) from exc
    definition_sources = definition_resolution_result.source_paths
    editable_header_paths = tuple(
        sorted(
            (
                path
                for path in editable.values()
                if Path(path).suffix.casefold()
                in _cc_catalog.CALL_CONTRACT_HEADER_SUFFIXES
            ),
            key=lambda path: (path.casefold(), path),
        )
    )
    source_atomic_header_routes: list[dict[str, Any]] = []
    source_atomic_definition_paths: set[str] = set()
    for header_path in editable_header_paths:
        try:
            header_resolution = resolve_reviewed_definition_sources(
                reviewed_callables=(),
                header_texts=tuple(
                    (header_path, "", context)
                    for context in header_contexts
                ),
                source_texts=source_texts,
                inventory_resolver=inventory_resolver,
            )
        except ValueError as exc:
            raise ProgressError(
                "call-contract writable header definition ownership is "
                f"ambiguous for {header_path}: {exc}"
            ) from exc
        route_definitions = tuple(header_resolution.source_paths)
        source_atomic_definition_paths.update(route_definitions)
        source_atomic_header_routes.append(
            {
                "header_path": header_path,
                "definition_source_paths": list(route_definitions),
                "resolution_mode": header_resolution.mode,
                "candidate_independent": (
                    header_resolution.candidate_independent
                ),
                "ambiguity_policy": header_resolution.ambiguity_policy,
            }
        )
    if not source_atomic_definition_paths.issubset(
        set(definition_sources)
    ):
        raise ProgressError(
            "call-contract writable header definition closure escapes the "
            "complete signed definition closure"
        )
    ordered_source_atomic_definitions = tuple(
        sorted(
            source_atomic_definition_paths,
            key=lambda path: (path.casefold(), path),
        )
    )
    final_build_manifest_logical = (
        _call_contract_final_build_manifest_logical_path()
    )
    try:
        final_build_manifest_tracked = resolve_repository_file(
            final_build_manifest_logical,
            repository_root=root,
            inventory=repository_inventory,
            context="call-contract final-build manifest",
            allowed_suffixes={".json"},
        )
    except RepositoryPathError as exc:
        raise ProgressError(str(exc)) from exc
    dependency_paths = {
        *editable.values(),
        *included_paths.values(),
        *definition_sources,
        final_build_manifest_tracked.repository_path,
        *(
            manifest_path.replace("\\", "/")
            for _target_id, _registration, manifest_path in (
                _call_contract_target_registrations(document, slice_row)
            )
        ),
    }
    definition_resolution = {
        **definition_resolution_result.as_dict(),
        "reason": (
            "the pre-build evidence has no durable reviewed call-edge identity "
            "list; use the complete uniquely resolved header declaration closure"
        ),
        "source_atomic_header_definition_resolution": {
            "kind": (
                "call-contract-writable-header-definition-resolution"
            ),
            "contract_version": 1,
            "mode": "governed-writable-header-definition-closure",
            "candidate_independent": True,
            "ambiguity_policy": "fail-closed",
            "header_routes": source_atomic_header_routes,
            "definition_source_paths": list(
                ordered_source_atomic_definitions
            ),
        },
    }

    return CallContractSourceClosure(
        source_edit_paths=tuple(
            sorted(
                editable.values(), key=lambda path: (path.casefold(), path)
            )
        ),
        registered_source_paths=tuple(
            sorted(
                registered_sources.values(),
                key=lambda path: (path.casefold(), path),
            )
        ),
        header_paths=header_paths,
        definition_source_paths=definition_sources,
        dependency_paths=tuple(
            sorted(dependency_paths, key=lambda path: (path.casefold(), path))
        ),
        definition_resolution=definition_resolution,
    )


def call_contract_source_closure(
    document: ProgressDocument,
    slice_row: Mapping[str, Any],
    *,
    repository_path_inventory: RepositoryPathInventory | None = None,
) -> CallContractSourceClosure:
    """Derive the exact source closure without persistent reuse."""

    return _call_contract_source_closure_uncached(
        document,
        slice_row,
        repository_path_inventory=repository_path_inventory,
    )


def _candidate_dependent_owner_provenance(
    document: ProgressDocument,
    closure: CallContractSourceClosure,
    *,
    caller_symbol_id: str,
    caller_address: str,
    caller_end_exclusive: str,
    caller_target_id: str,
    caller_source_edit_paths: Sequence[str],
    error: _cc_errors.CandidateCallTargetBridgeError,
) -> dict[str, Any]:
    """Resolve a candidate-observed callee to one bounded source owner.

    The decorated identity and ABI are candidate observations used only for
    routing.  They never become retail expected truth or acceptance evidence.
    """

    decoded_identity = error.decoded_identity
    non_source_class = _cc_catalog._R4564_NON_SOURCE_DEPENDENT_IDENTITIES.get(
        error.decorated_identity
    )
    resolution_error = ""
    resolution = None
    if decoded_identity is not None and non_source_class is None:
        try:
            repository_inventory = load_repository_path_inventory(REPO_ROOT)
            root = repository_inventory.repository_root
            config = load_final_build_config(DEFAULT_FINAL_BUILD_MANIFEST)
            context_by_source: dict[str, tuple[str, ...]] = {}
            canonical_source_by_key: dict[str, str] = {}
            configured_sources, _stale_diagnostics = (
                _call_contract_final_build_source_map(
                    config,
                    inventory=repository_inventory,
                )
            )
            for rows in configured_sources.values():
                if len(rows) != 1:
                    raise ProgressError(
                        "call-contract dependent-owner final-build source "
                        "population has a duplicate or case collision"
                    )
                relative, resolved = rows[0]
                source_key = relative.casefold()
                prior_source = canonical_source_by_key.get(source_key)
                if prior_source is not None:
                    raise ProgressError(
                        "call-contract dependent-owner final-build source "
                        "population has a duplicate or case collision: "
                        f"{prior_source}, {relative}"
                    )
                canonical_source_by_key[source_key] = relative
                context_by_source[source_key] = (
                    _call_contract_vc5_preprocessor_defines(config, resolved)
                )
            source_texts: list[tuple[str, str, tuple[str, ...]]] = []
            definition_source_by_key: dict[str, str] = {}
            for path in closure.definition_source_paths:
                source_key = path.casefold()
                prior_source = definition_source_by_key.get(source_key)
                if prior_source is not None:
                    raise ProgressError(
                        "call-contract dependent-owner definition source "
                        "population has a duplicate or case collision: "
                        f"{prior_source}, {path}"
                    )
                definition_source_by_key[source_key] = path
                context = context_by_source.get(source_key)
                if context is None:
                    raise ProgressError(
                        "call-contract dependent-owner definition source lacks "
                        f"an exact final-build preprocessor context: {path}"
                    )
                source_texts.append((path, "", context))
            inventory_resolver = _call_contract_callable_inventory_resolver(
                document, root=root
            )
            definition_probe = resolve_dependent_callable_owner(
                callable_key=decoded_identity.callable_key,
                header_texts=(),
                source_texts=source_texts,
                inventory_resolver=inventory_resolver,
            )
            definition_contexts: set[tuple[str, ...]] = set()
            for path in definition_probe.definition_paths:
                source_key = path.casefold()
                context = context_by_source.get(source_key)
                if context is None or source_key not in definition_source_by_key:
                    raise ProgressError(
                        "call-contract dependent-owner definition probe escaped "
                        f"the exact source closure: {path}"
                    )
                definition_contexts.add(context)
            # First scan every exact definition source under its own governed
            # context.  A header declaration can own the observed callable
            # only under a context belonging to one of the matching definition
            # sources.  This retains complete definition/ambiguity detection
            # without materializing every header under every unrelated context
            # in the closure.
            header_contexts = tuple(sorted(definition_contexts))
            header_texts = tuple(
                (path, "", context)
                for path in closure.header_paths
                for context in header_contexts
            )
            resolution = resolve_dependent_callable_owner(
                callable_key=decoded_identity.callable_key,
                header_texts=header_texts,
                source_texts=source_texts,
                inventory_resolver=inventory_resolver,
            )
        except (OSError, ValueError, ProgressError) as exc:
            resolution_error = str(exc)
    caller_paths = set(caller_source_edit_paths)
    owner_paths = set(resolution.source_edit_paths if resolution else ())
    call_site_in_extent = bool(
        error.call_site_address is not None
        and address_value(caller_address)
        <= address_value(error.call_site_address)
        < address_value(caller_end_exclusive)
    )
    if not call_site_in_extent or not error.call_site_unique:
        relation = "non-source-ambiguity"
    elif resolution is None or resolution.mode != "exact":
        relation = "non-source-ambiguity"
    elif owner_paths.isdisjoint(caller_paths):
        relation = "exact-dependent-owner"
    elif owner_paths.issubset(caller_paths):
        relation = "caller-owned"
    else:
        relation = "non-source-ambiguity"
    if non_source_class is not None:
        resolution_payload = {
            "kind": "call-contract-dependent-owner-resolution",
            "contract_version": 1,
            "mode": non_source_class,
            "declaration_paths": [],
            "definition_paths": [],
            "source_edit_paths": [],
            "candidate_independent": False,
            "ambiguity_policy": "fail-closed",
            "resolution_basis": "exact-reviewed-r4564-non-source-identity",
        }
    elif resolution is not None:
        resolution_payload = resolution.as_dict()
    else:
        resolution_payload = {
            "kind": "call-contract-dependent-owner-resolution",
            "contract_version": 1,
            "mode": (
                "unsupported-identity"
                if decoded_identity is None
                else "resolver-error"
            ),
            "declaration_paths": [],
            "definition_paths": [],
            "source_edit_paths": [],
            "candidate_independent": False,
            "ambiguity_policy": "fail-closed",
        }
        if resolution_error:
            resolution_payload["error"] = resolution_error
    identity_payload = (
        decoded_identity.as_dict()
        if decoded_identity is not None
        else {
            "decorated_identity": error.decorated_identity,
            "decode_status": (
                "exact-non-source" if non_source_class else "unsupported"
            ),
            **(
                {"non_source_class": non_source_class}
                if non_source_class
                else {}
            ),
        }
    )
    return {
        "kind": "call-contract-candidate-target-dependency",
        "contract_version": 1,
        "candidate_expected_truth": False,
        "provenance": "candidate-cod-decorated-call-target",
        "observation_role": "candidate-observed-abi-routing-only",
        "caller_symbol_id": caller_symbol_id,
        "caller_address": caller_address,
        "caller_end_exclusive": caller_end_exclusive,
        "caller_target_id": caller_target_id,
        "caller_target_source_edit_paths": sorted(
            caller_paths, key=lambda path: (path.casefold(), path)
        ),
        "candidate_call_site_address": error.call_site_address,
        "candidate_call_site_unique": error.call_site_unique,
        "candidate_call_site_in_caller_extent": call_site_in_extent,
        "identity": identity_payload,
        "owner_relation": relation,
        "owner_resolution": resolution_payload,
    }


def _decode_candidate_body_zero_argument_identity(
    decorated_identity: str,
) -> DecodedCallableIdentity | None:
    """Decode the narrow VC5 global/static body ABI used for repair routing."""

    match = _cc_catalog._CANDIDATE_BODY_ZERO_ARGUMENT_IDENTITY_RE.fullmatch(
        str(decorated_identity)
    )
    if match is None:
        return None
    parts = match.group("body").split("@")
    return DecodedCallableIdentity(
        decorated_identity=str(decorated_identity),
        callable_key=CallableKey(
            "::".join((*reversed(parts[1:]), parts[0])),
            (),
        ),
        calling_convention={
            "A": "__cdecl",
            "G": "__stdcall",
            "I": "__fastcall",
        }[match.group("calling")],
        parameter_bytes=0,
        return_shape=(
            f"msvc-type-code:{match.group('return')}"
            if len(match.group("return")) == 1
            else f"msvc-type-encoding:{match.group('return')}"
        ),
        identity_format="msvc-cpp-global-zero-argument-body",
    )




def _candidate_artifact_path(path: Path) -> str:
    resolved = path.resolve()
    try:
        return resolved.relative_to(REPO_ROOT.resolve()).as_posix()
    except ValueError:
        return str(resolved)


def _decode_vc5_virtual_destructor_absence_identity(
    decorated_identity: str,
) -> dict[str, Any] | None:
    """Decode the one VC5 virtual-destructor family safe for absence proof."""

    match = _cc_catalog._VC5_VIRTUAL_DESTRUCTOR_IDENTITY_RE.fullmatch(
        str(decorated_identity)
    )
    if match is None:
        return None
    encoded_class = match.group("class_name")
    parts = encoded_class.split("@")
    terminal = parts[0]
    qualified_class = "::".join((*reversed(parts[1:]), terminal))
    return {
        "callable_key": CallableKey(
            f"{qualified_class}::~{terminal}", ()
        ),
        "qualified_class": qualified_class,
        "ordinary_destructor": str(decorated_identity),
        "scalar_deleting_destructor": (
            f"??_G{encoded_class}@@UAEPAXI@Z"
        ),
        "vector_deleting_destructor": (
            f"??_E{encoded_class}@@UAEPAXI@Z"
        ),
        "vftable": f"??_7{encoded_class}@@6B@",
    }


def _callable_definition_body_token_counts(
    text: str,
    *,
    defines: Iterable[str],
    callable_key: CallableKey,
) -> tuple[int, ...]:
    """Return active token counts for every exact callable definition body.

    This deliberately reuses the conservative definition-closure lexer and
    declarator parser.  Comments, literals, directives, and inactive
    preprocessor branches therefore cannot manufacture an empty definition.
    A body is proven empty only when its matching braces contain zero active
    C/C++ tokens.
    """

    tokens = _CPP_DEFINITION_TOKEN_RE.findall(
        _mask_cpp_definition_non_code(text, tuple(defines))
    )
    frames: list[tuple[str, str]] = []
    segment: list[str] = []
    counts: list[int] = []

    def in_function() -> bool:
        return any(kind == "function" for kind, _name in frames)

    def scope_names() -> tuple[str, ...]:
        return tuple(
            name
            for kind, name in frames
            if kind in {"namespace", "class"} and name
        )

    for index, token in enumerate(tokens):
        if token == ";":
            segment = []
            continue
        if token == "{":
            if in_function():
                frames.append(("other", ""))
            else:
                key = _cpp_definition_callable_key(segment, scope_names())
                if key is not None:
                    if key == callable_key:
                        depth = 1
                        cursor = index + 1
                        while cursor < len(tokens) and depth:
                            if tokens[cursor] == "{":
                                depth += 1
                            elif tokens[cursor] == "}":
                                depth -= 1
                            cursor += 1
                        if depth:
                            raise ValueError(
                                "unterminated callable definition in inline-absence proof"
                            )
                        counts.append(cursor - index - 2)
                    frames.append(("function", key.qualified_name))
                elif "namespace" in segment:
                    namespace_index = (
                        len(segment)
                        - 1
                        - segment[::-1].index("namespace")
                    )
                    name = (
                        segment[namespace_index + 1]
                        if namespace_index + 1 < len(segment)
                        and re.fullmatch(
                            r"[A-Za-z_][A-Za-z0-9_]*",
                            segment[namespace_index + 1],
                        )
                        else ""
                    )
                    frames.append(("namespace", name))
                elif any(
                    keyword in segment
                    for keyword in ("class", "struct", "union")
                ):
                    class_indexes = [
                        item
                        for item, value in enumerate(segment)
                        if value in {"class", "struct", "union"}
                    ]
                    class_index = class_indexes[-1]
                    name = (
                        segment[class_index + 1]
                        if class_index + 1 < len(segment)
                        and re.fullmatch(
                            r"[A-Za-z_][A-Za-z0-9_]*",
                            segment[class_index + 1],
                        )
                        else ""
                    )
                    frames.append(("class", name))
                else:
                    frames.append(("other", ""))
            segment = []
            continue
        if token == "}":
            if frames:
                frames.pop()
            segment = []
            continue
        segment.append(token)
    return tuple(counts)


def _coff_exact_named_symbols(
    coff_object: CoffObject,
    name: str,
) -> list[Any]:
    folded = name.casefold()
    rows = [
        symbol
        for symbol in coff_object.symbols
        if str(getattr(symbol, "name", "")).casefold() == folded
    ]
    if any(str(getattr(symbol, "name", "")) != name for symbol in rows):
        return []
    return rows


def _coff_exact_comdat_selection(
    coff_object: CoffObject,
    section_number: int,
) -> bool:
    definitions = [
        symbol
        for symbol in coff_object.symbols
        if getattr(symbol, "section_number", 0) == section_number
        and getattr(symbol, "section_definition_selection", None) is not None
    ]
    return bool(
        len(definitions) == 1
        and getattr(definitions[0], "section_definition_selection", None) == 2
    )


def _prove_intentionally_inlined_empty_authored_candidate(
    document: ProgressDocument,
    closure: CallContractSourceClosure,
    *,
    target_id: str,
    selected_target: Any,
    compiled_entry: Any | None,
    symbol_id: str,
    address: str,
    expected_identity: str,
    cod_path: Path,
    coff_object: CoffObject,
) -> dict[str, Any] | None:
    """Prove one empty inline authored destructor with no candidate body.

    The accommodation is intentionally conjunctive.  Retail/registration and
    current source select the identity; current VC5 output may prove only that
    the exact body is absent and that the corresponding deleting-destructor /
    vftable linkage family remains emitted.  Any missing, duplicated, aliased,
    differently optimized, non-empty, out-of-line, or malformed fact returns
    ``None`` so ordinary candidate-body acquisition remains fail-closed.
    """

    decoded = _decode_vc5_virtual_destructor_absence_identity(
        expected_identity
    )
    if decoded is None:
        return None
    target_name = str(getattr(selected_target, "name", ""))
    target_binary = str(getattr(selected_target, "target_binary", ""))
    units = tuple(
        getattr(selected_target, "translation_unit_function_order", ())
    )
    target_rows = tuple(getattr(compiled_entry, "functions", ()))
    try:
        matching_rows = [
            row
            for row in target_rows
            if normalize_address(str(getattr(row, "address", ""))) == address
        ]
    except (TypeError, ValueError, ProgressError):
        return None
    definition_path = str(getattr(compiled_entry, "source_from", ""))
    if (
        target_id != f"recoil:vc5-target:{target_name}"
        or target_binary != "recoil"
        or not bool(
            getattr(
                selected_target,
                "check_translation_unit_function_order",
                False,
            )
        )
        or len(units) != 1
        # Convergence loads the phase document and its manifest cache
        # independently from the parent census document.  Bind the compiled
        # entry to the registered unit by its complete parsed value, not by
        # Python object identity; any field drift still fails closed.
        or compiled_entry != units[0]
        or len(matching_rows) != 1
        or str(getattr(matching_rows[0], "symbol", ""))
        != expected_identity
        or getattr(matching_rows[0], "symbol_regex", None) is not None
        or str(getattr(matching_rows[0], "pipeline_class", ""))
        != "authored"
        or str(getattr(matching_rows[0], "authored_order_role", ""))
        != "authored-body"
        or getattr(matching_rows[0], "required_presence", None) is not True
        or not definition_path
        or closure.registered_source_paths.count(definition_path) != 1
        or closure.dependency_paths.count(definition_path) != 1
    ):
        return None

    symbols = document.collection("symbols")
    blocks = document.collection("physical_blocks")
    symbol = symbols.get(symbol_id)
    block_id = (
        str(symbol.get("physical_block_id", ""))
        if isinstance(symbol, Mapping)
        else ""
    )
    block = blocks.get(block_id)
    accepted = (
        block.get("accepted_order_facts")
        if isinstance(block, Mapping)
        else None
    )
    authored_order = (
        block.get("order", {}).get("authored", {})
        if isinstance(block, Mapping)
        and isinstance(block.get("order"), Mapping)
        and isinstance(block.get("order", {}).get("authored"), Mapping)
        else {}
    )
    if (
        not isinstance(symbol, Mapping)
        or symbol_id != f"recoil:function:{address}"
        or symbol.get("kind") != "function"
        or symbol.get("binary") != "recoil"
        or normalize_address(str(symbol.get("address", ""))) != address
        or symbol.get("pipeline_class") != "authored"
        or symbol.get("extent_state") != "known"
        or symbol.get("output_section_id") != "recoil:section:.text"
        or target_id not in symbol.get("verification_target_ids", [])
        or not block_id
        or not isinstance(accepted, Mapping)
        or accepted.get("target_id") != target_id
        or accepted.get("phase") != "authored-function-order"
        or accepted.get("validation_mode") != "live"
        or list(accepted.get("matched_identities", [])).count(symbol_id) != 1
        or any(
            not is_current_accepted_state(authored_order.get(dimension))
            for dimension in (
                "authored_linked_order",
                "authored_object_order",
                "block_precedence",
                "linked_identity_presence",
                "object_identity_presence",
            )
        )
    ):
        return None

    repository_inventory = load_repository_path_inventory(REPO_ROOT)
    root = repository_inventory.repository_root
    try:
        config = load_final_build_config(DEFAULT_FINAL_BUILD_MANIFEST)
        configured_sources, _stale_diagnostics = (
            _call_contract_final_build_source_map(
                config,
                inventory=repository_inventory,
            )
        )
        configured_definition = configured_sources.get(
            definition_path.casefold(), []
        )
        if (
            len(configured_definition) != 1
            or configured_definition[0][0] != definition_path
            or str(getattr(selected_target, "compile_context_from", ""))
            != _call_contract_final_build_manifest_logical_path()
        ):
            return None
        source_path = configured_definition[0][1]
        final_flags = (
            *final_build_effective_compile_flags(config, source_path),
            *(f"/D{value}" for value in config.defines),
            "/FAcs",
        )
        profile, target_flags = effective_source_compile_context(
            selected_target, definition_path
        )
        upper_flags = tuple(flag.upper() for flag in target_flags)
        if (
            not profile
            or tuple(target_flags) != final_flags
            or [
                flag
                for flag in upper_flags
                if flag in {"/OD", "/O1", "/O2", "/OX"}
            ]
            != ["/O2"]
            or [
                flag
                for flag in upper_flags
                if flag in {"/OB0", "/OB1", "/OB2"}
            ]
            != ["/OB1"]
            or "/FACS" not in upper_flags
            or "/TP" not in upper_flags
        ):
            return None
        defines = _call_contract_vc5_preprocessor_defines(
            config, source_path
        )
        resolver = _call_contract_callable_inventory_resolver(
            document, root=root
        )
        header_matches: list[tuple[str, tuple[int, ...]]] = []
        for header_path in closure.header_paths:
            inventory = resolver(header_path, defines)
            if decoded["callable_key"] not in inventory.definitions:
                continue
            header_text = (root / header_path).read_text(
                encoding="utf-8", errors="ignore"
            )
            header_matches.append(
                (
                    header_path,
                    _callable_definition_body_token_counts(
                        header_text,
                        defines=defines,
                        callable_key=decoded["callable_key"],
                    ),
                )
            )
        if len(header_matches) != 1 or header_matches[0][1] != (0,):
            return None
        header_path = header_matches[0][0]
        source_definition_matches: list[str] = []
        for candidate_path in sorted(
            {
                *closure.registered_source_paths,
                *closure.definition_source_paths,
            },
            key=lambda value: (value.casefold(), value),
        ):
            configured = configured_sources.get(candidate_path.casefold(), [])
            if len(configured) != 1 or configured[0][0] != candidate_path:
                return None
            candidate_defines = _call_contract_vc5_preprocessor_defines(
                config, configured[0][1]
            )
            if decoded["callable_key"] in resolver(
                candidate_path, candidate_defines
            ).definitions:
                source_definition_matches.append(candidate_path)
        if source_definition_matches:
            return None
    except (OSError, RuntimeError, ValueError, ProgressError):
        return None

    ordinary = str(decoded["ordinary_destructor"])
    scalar = str(decoded["scalar_deleting_destructor"])
    vector = str(decoded["vector_deleting_destructor"])
    vftable = str(decoded["vftable"])
    if _coff_exact_named_symbols(coff_object, ordinary):
        return None
    try:
        cod_text = _cc_listing._read_cod_text(cod_path, errors="ignore")
    except OSError:
        return None
    proc_re = re.compile(r"^\s*(\S+)\s+PROC\b", re.IGNORECASE | re.MULTILINE)
    endp_re = re.compile(r"^\s*(\S+)\s+ENDP\b", re.IGNORECASE | re.MULTILINE)
    ordinary_proc_rows = [
        name for name in proc_re.findall(cod_text) if name.casefold() == ordinary.casefold()
    ]
    ordinary_endp_rows = [
        name for name in endp_re.findall(cod_text) if name.casefold() == ordinary.casefold()
    ]
    ordinary_comdat_rows = re.findall(
        rf"^\s*;\s*COMDAT\s+{re.escape(ordinary)}\s*$",
        cod_text,
        re.IGNORECASE | re.MULTILINE,
    )
    if ordinary_proc_rows or ordinary_endp_rows or len(ordinary_comdat_rows) != 1:
        return None

    scalar_rows = _coff_exact_named_symbols(coff_object, scalar)
    vector_rows = _coff_exact_named_symbols(coff_object, vector)
    vftable_rows = _coff_exact_named_symbols(coff_object, vftable)
    scalar_undefined = [
        row
        for row in scalar_rows
        if row.section_number == 0
        and row.storage_class == IMAGE_SYM_CLASS_EXTERNAL
        and row.type == 0x20
    ]
    scalar_defined = [
        row
        for row in scalar_rows
        if row.section_number > 0
        and row.storage_class == IMAGE_SYM_CLASS_EXTERNAL
        and row.type == 0x20
    ]
    vector_weak = [
        row
        for row in vector_rows
        if row.section_number == 0
        and row.storage_class == 105
        and row.type == 0x20
    ]
    vftable_defined = [
        row
        for row in vftable_rows
        if row.section_number > 0
        and row.storage_class == IMAGE_SYM_CLASS_EXTERNAL
        and row.type == 0
    ]
    if (
        len(scalar_rows) != 2
        or len(scalar_undefined) != 1
        or len(scalar_defined) != 1
        or len(vector_rows) != 1
        or len(vector_weak) != 1
        or len(vftable_rows) != 1
        or len(vftable_defined) != 1
        or getattr(vector_weak[0], "weak_external_tag_index", None)
        != scalar_undefined[0].index
        or getattr(vector_weak[0], "weak_external_characteristics", None) != 2
    ):
        return None
    scalar_symbol = scalar_defined[0]
    vftable_symbol = vftable_defined[0]
    try:
        scalar_section = coff_object.section(scalar_symbol.section_number)
        vftable_section = coff_object.section(vftable_symbol.section_number)
        scalar_body = coff_object.function_bytes(scalar)
        scalar_listing = _cc_candidate._extract_cod_proc_with_local_switches(cod_path, scalar)
    except (OSError, RuntimeError, ValueError):
        return None
    if (
        scalar_section.name != ".text"
        or not scalar_section.raw_data
        or not (scalar_section.characteristics & IMAGE_SCN_CNT_CODE)
        or not (scalar_section.characteristics & _cc_catalog.IMAGE_SCN_LNK_COMDAT)
        or not _coff_exact_comdat_selection(
            coff_object, scalar_symbol.section_number
        )
        or not scalar_body.data
        or not scalar_listing.instructions
        or vftable_section.name != ".rdata"
        or not (vftable_section.characteristics & _cc_catalog.IMAGE_SCN_LNK_COMDAT)
        or not _coff_exact_comdat_selection(
            coff_object, vftable_symbol.section_number
        )
        or vftable_symbol.value != 0
        or len(vftable_section.raw_data) < 4
        or vftable_section.raw_data[:4] != b"\x00\x00\x00\x00"
    ):
        return None
    vftable_slot_relocations = [
        relocation
        for relocation in coff_object.relocations_by_section.get(
            vftable_symbol.section_number, ()
        )
        if relocation.offset == vftable_symbol.value
    ]
    scalar_vftable_relocations = [
        relocation
        for relocation in coff_object.relocations_by_section.get(
            scalar_symbol.section_number, ()
        )
        if relocation.symbol_name == vftable
    ]
    ordinary_relocations = [
        relocation
        for rows in coff_object.relocations_by_section.values()
        for relocation in rows
        if relocation.symbol_name.casefold() == ordinary.casefold()
    ]
    if (
        len(vftable_slot_relocations) != 1
        or vftable_slot_relocations[0].type != IMAGE_REL_I386_DIR32
        or vftable_slot_relocations[0].symbol_name != vector
        or vftable_slot_relocations[0].symbol_index != vector_weak[0].index
        or len(scalar_vftable_relocations) != 1
        or scalar_vftable_relocations[0].type != IMAGE_REL_I386_DIR32
        or ordinary_relocations
    ):
        return None

    return {
        "kind": _cc_catalog.INTENTIONALLY_INLINED_ABSENCE_PROOF_KIND,
        "contract_version": 1,
        "proof_mode": _cc_catalog.INTENTIONALLY_INLINED_ABSENCE_PROOF_MODE,
        "candidate_expected_truth": False,
        "symbol_id": symbol_id,
        "address": address,
        "target_id": target_id,
        "registered_identity": ordinary,
        "source_definition": {
            "header_path": header_path,
            "translation_unit_path": definition_path,
            "callable_key": str(decoded["callable_key"]),
            "active_definition_count": 1,
            "active_body_token_count": 0,
            "out_of_line_definition_count": 0,
        },
        "compiler": {
            "profile": profile,
            "effective_flags": list(target_flags),
            "required_optimization": "/O2",
            "required_inline_expansion": "/Ob1",
            "assembly_listing": "/FAcs",
        },
        "candidate_absence": {
            "object_path": _candidate_artifact_path(cod_path.with_suffix(".obj")),
            "cod_path": _candidate_artifact_path(cod_path),
            "coff_identity_count": 0,
            "cod_proc_count": 0,
            "cod_endp_count": 0,
            "cod_comdat_declaration_count": 1,
            "ordinary_relocation_count": 0,
        },
        "lifecycle_linkage": {
            "scalar_deleting_destructor": scalar,
            "vector_deleting_destructor": vector,
            "vftable": vftable,
            "vector_weak_aliases_scalar": True,
            "vftable_slot_zero_targets_vector": True,
            "scalar_references_vftable": True,
        },
        "retail_linkage_authority": {
            "physical_block_id": block_id,
            "accepted_order_target_id": target_id,
            "matched_identity_count": 1,
            "authored_order_dimensions_current": True,
        },
    }


def _valid_intentionally_inlined_absence_proof(
    proof: Any,
    *,
    symbol_id: str,
    address: str,
    target_id: str,
) -> bool:
    if not isinstance(proof, Mapping):
        return False
    source = proof.get("source_definition")
    compiler = proof.get("compiler")
    absence = proof.get("candidate_absence")
    lifecycle = proof.get("lifecycle_linkage")
    retail = proof.get("retail_linkage_authority")
    decoded = _decode_vc5_virtual_destructor_absence_identity(
        str(proof.get("registered_identity", ""))
    )
    effective_flags = (
        compiler.get("effective_flags")
        if isinstance(compiler, Mapping)
        else None
    )
    folded_flags = (
        [str(flag).upper() for flag in effective_flags]
        if isinstance(effective_flags, list)
        else []
    )
    return bool(
        proof.get("kind") == _cc_catalog.INTENTIONALLY_INLINED_ABSENCE_PROOF_KIND
        and proof.get("contract_version") == 1
        and proof.get("proof_mode") == _cc_catalog.INTENTIONALLY_INLINED_ABSENCE_PROOF_MODE
        and proof.get("candidate_expected_truth") is False
        and proof.get("symbol_id") == symbol_id
        and proof.get("address") == address
        and proof.get("target_id") == target_id
        and isinstance(proof.get("registered_identity"), str)
        and decoded is not None
        and isinstance(source, Mapping)
        and source.get("active_definition_count") == 1
        and source.get("active_body_token_count") == 0
        and source.get("out_of_line_definition_count") == 0
        and isinstance(source.get("header_path"), str)
        and isinstance(source.get("translation_unit_path"), str)
        and isinstance(source.get("callable_key"), str)
        and isinstance(compiler, Mapping)
        and isinstance(compiler.get("profile"), str)
        and bool(compiler.get("profile"))
        and isinstance(compiler.get("effective_flags"), list)
        and folded_flags.count("/O2") == 1
        and folded_flags.count("/OB1") == 1
        and folded_flags.count("/FACS") == 1
        and compiler.get("required_optimization") == "/O2"
        and compiler.get("required_inline_expansion") == "/Ob1"
        and compiler.get("assembly_listing") == "/FAcs"
        and isinstance(absence, Mapping)
        and isinstance(absence.get("object_path"), str)
        and bool(absence.get("object_path"))
        and isinstance(absence.get("cod_path"), str)
        and bool(absence.get("cod_path"))
        and absence.get("coff_identity_count") == 0
        and absence.get("cod_proc_count") == 0
        and absence.get("cod_endp_count") == 0
        and absence.get("cod_comdat_declaration_count") == 1
        and absence.get("ordinary_relocation_count") == 0
        and isinstance(lifecycle, Mapping)
        and lifecycle.get("scalar_deleting_destructor")
        == decoded.get("scalar_deleting_destructor")
        and lifecycle.get("vector_deleting_destructor")
        == decoded.get("vector_deleting_destructor")
        and lifecycle.get("vftable") == decoded.get("vftable")
        and lifecycle.get("vector_weak_aliases_scalar") is True
        and lifecycle.get("vftable_slot_zero_targets_vector") is True
        and lifecycle.get("scalar_references_vftable") is True
        and isinstance(retail, Mapping)
        and isinstance(retail.get("physical_block_id"), str)
        and bool(retail.get("physical_block_id"))
        and retail.get("accepted_order_target_id") == target_id
        and retail.get("matched_identity_count") == 1
        and retail.get("authored_order_dimensions_current") is True
    )


def _candidate_body_declaration_header_route(
    document: ProgressDocument,
    closure: CallContractSourceClosure,
    *,
    target_id: str,
    selected_target: Any,
    compiled_entry: Any | None,
    symbol_id: str,
    address: str,
    expected_identity: str,
    cod_path: Path,
    coff_object: CoffObject,
) -> dict[str, Any] | None:
    """Prove one current ABI alternative and its declaration/definition pair.

    The registered target spelling is candidate-independent expected identity.
    Current COFF/COD and source declarations are repair provenance only.  No
    diagnostic-text parsing or source-path guessing is permitted.
    """

    expected = _decode_candidate_body_zero_argument_identity(expected_identity)
    if expected is None:
        return None
    symbol_names = [
        str(getattr(item, "name", ""))
        for item in coff_object.symbols
        if str(getattr(item, "name", ""))
    ]
    candidate_names: list[str] = []
    duplicate_candidate_names: set[str] = set()
    for name in sorted(set(symbol_names)):
        observed = _decode_candidate_body_zero_argument_identity(name)
        if (
            observed is None
            or observed.callable_key != expected.callable_key
            or observed.return_shape != expected.return_shape
            or observed.calling_convention == expected.calling_convention
        ):
            continue
        try:
            coff_object.function_bytes(name)
            _cc_candidate._extract_cod_proc_with_local_switches(cod_path, name)
        except (OSError, RuntimeError, ValueError):
            continue
        candidate_names.append(name)
        if symbol_names.count(name) != 1:
            duplicate_candidate_names.add(name)
    candidate_names = sorted(set(candidate_names))
    route: dict[str, Any] = {
        "kind": "call-contract-candidate-body-declaration-header-route",
        "contract_version": 1,
        "target_id": target_id,
        "symbol_id": symbol_id,
        "address": address,
        "expected_identity": expected.as_dict(),
        "candidate_identities": [
            _decode_candidate_body_zero_argument_identity(name).as_dict()
            for name in candidate_names
        ],
        "candidate_cod_path": _candidate_artifact_path(cod_path),
        "candidate_object_path": _candidate_artifact_path(
            cod_path.with_suffix(".obj")
        ),
        "declaration_paths": [],
        "definition_paths": [],
        "source_edit_paths": [],
        "routing_mode": "blocked",
        "candidate_expected_truth": False,
        "ambiguity_policy": "fail-closed",
    }
    if len(candidate_names) != 1 or duplicate_candidate_names:
        route["blocker_reason"] = (
            "candidate-body-identity-missing"
            if not candidate_names
            else "candidate-body-identity-ambiguous"
        )
        return route

    target_name = str(getattr(selected_target, "name", ""))
    target_binary = str(getattr(selected_target, "target_binary", ""))
    target_rows = (
        tuple(getattr(compiled_entry, "functions", ()))
        if compiled_entry is not None
        else tuple(getattr(selected_target, "functions", ()))
    )
    try:
        matching_target_rows = [
            row
            for row in target_rows
            if normalize_address(str(getattr(row, "address", "")))
            == address
        ]
    except (TypeError, ValueError, ProgressError):
        route["blocker_reason"] = (
            "selected-target-definition-authority-mismatch"
        )
        return route
    translation_units = tuple(
        getattr(selected_target, "translation_unit_function_order", ())
    )
    uses_translation_units = bool(
        getattr(
            selected_target,
            "check_translation_unit_function_order",
            False,
        )
    )
    definition_path = str(
        getattr(
            compiled_entry,
            "source_from",
            getattr(selected_target, "source_from", ""),
        )
    )
    target_definition_paths = (
        tuple(
            str(getattr(entry, "source_from", ""))
            for entry in translation_units
        )
        if uses_translation_units
        else (str(getattr(selected_target, "source_from", "")),)
    )
    if (
        target_id != f"recoil:vc5-target:{target_name}"
        or target_binary != "recoil"
        or len(matching_target_rows) != 1
        or str(getattr(matching_target_rows[0], "symbol", ""))
        != expected_identity
        or getattr(matching_target_rows[0], "symbol_regex", None) is not None
        or (uses_translation_units and compiled_entry not in translation_units)
        or (uses_translation_units and compiled_entry is None)
        or (not uses_translation_units and compiled_entry is not None)
        or not definition_path
        or any(not path for path in target_definition_paths)
        or len(
            {path.casefold() for path in target_definition_paths}
        )
        != len(target_definition_paths)
        or target_definition_paths.count(definition_path) != 1
    ):
        route["blocker_reason"] = "selected-target-definition-authority-mismatch"
        return route

    repository_inventory = load_repository_path_inventory(REPO_ROOT)
    root = repository_inventory.repository_root
    try:
        config = load_final_build_config(DEFAULT_FINAL_BUILD_MANIFEST)
        configured_sources, _stale_diagnostics = (
            _call_contract_final_build_source_map(
                config,
                inventory=repository_inventory,
            )
        )
        resolver = _call_contract_callable_inventory_resolver(
            document, root=root
        )
        target_source_contexts: dict[str, tuple[str, ...]] = {}
        for target_path in target_definition_paths:
            configured_target_sources = configured_sources.get(
                target_path.casefold(), []
            )
            if (
                len(configured_target_sources) != 1
                or configured_target_sources[0][0] != target_path
            ):
                route["blocker_reason"] = (
                    "selected-target-definition-unsupported-or-aliased"
                )
                return route
            target_source_contexts[target_path] = (
                _call_contract_vc5_preprocessor_defines(
                    config, configured_target_sources[0][1]
                )
            )
        definition_context = target_source_contexts[definition_path]
        declaration_paths = sorted(
            {
                path
                for path in closure.header_paths
                if expected.callable_key
                in resolver(path, definition_context).declarations
            },
            key=lambda path: (path.casefold(), path),
        )
        definition_matches = [
            target_path
            for target_path, context in target_source_contexts.items()
            if expected.callable_key
            in resolver(target_path, context).definitions
        ]
    except (OSError, RuntimeError, ValueError, ProgressError) as exc:
        route["blocker_reason"] = (
            "declaration-header-definition-resolution-error"
        )
        route["resolution_error"] = str(exc)
        return route

    route["declaration_paths"] = declaration_paths
    if len(declaration_paths) != 1:
        route["blocker_reason"] = (
            "declaration-header-missing"
            if not declaration_paths
            else "declaration-header-ambiguous"
        )
        return route
    route["definition_paths"] = definition_matches
    if definition_matches != [definition_path]:
        route["blocker_reason"] = (
            "selected-target-definition-missing"
            if not definition_matches
            else "selected-target-definition-ambiguous-or-mismatched"
        )
        return route
    declaration_path = declaration_paths[0]
    routed_path_specs = (
        (
            declaration_path,
            _cc_catalog.CALL_CONTRACT_HEADER_SUFFIXES,
            closure.header_paths,
            "declaration-header",
        ),
        (
            definition_path,
            _cc_catalog.CALL_CONTRACT_SOURCE_SUFFIXES,
            closure.registered_source_paths,
            "selected-target-definition",
        ),
    )
    for routed_path, suffixes, authority_paths, label in routed_path_specs:
        try:
            repository_routed = resolve_repository_file(
                routed_path,
                repository_root=root,
                inventory=repository_inventory,
                context=f"call-contract {label}",
                allowed_suffixes=suffixes,
            )
        except RepositoryPathError:
            route["blocker_reason"] = f"{label}-escaping"
            return route
        authority_casefolds = [
            path.casefold() for path in authority_paths
        ]
        if (
            repository_routed.repository_path != routed_path
            or authority_casefolds.count(routed_path.casefold()) != 1
            or (
                label == "selected-target-definition"
                and routed_path not in closure.source_edit_paths
            )
            or routed_path not in closure.dependency_paths
        ):
            route["blocker_reason"] = f"{label}-unsupported-or-aliased"
            return route
    route["routing_mode"] = "exact"
    route["candidate_identity"] = route["candidate_identities"][0]
    route["source_edit_paths"] = sorted(
        {declaration_path, definition_path},
        key=lambda path: (path.casefold(), path),
    )
    return route


def _candidate_body_compiled_unit_blocker_route(
    *,
    target_id: str,
    symbol_id: str,
    address: str,
    expected_identity: str,
    unit_count: int,
) -> dict[str, Any] | None:
    """Return a typed non-repairable route when TU authority is not unique."""

    expected = _decode_candidate_body_zero_argument_identity(
        expected_identity
    )
    if expected is None:
        return None
    return {
        "kind": "call-contract-candidate-body-declaration-header-route",
        "contract_version": 1,
        "target_id": target_id,
        "symbol_id": symbol_id,
        "address": address,
        "expected_identity": expected.as_dict(),
        "candidate_identities": [],
        "candidate_cod_path": "",
        "candidate_object_path": "",
        "declaration_paths": [],
        "definition_paths": [],
        "source_edit_paths": [],
        "routing_mode": "blocked",
        "blocker_reason": (
            "selected-target-compiled-unit-missing"
            if unit_count == 0
            else "selected-target-compiled-unit-ambiguous"
        ),
        "candidate_expected_truth": False,
        "ambiguity_policy": "fail-closed",
    }




def _compile_call_contract_definition_sources(
    closure: CallContractSourceClosure,
    *,
    build_root: Path,
    vc5_env: Path,
) -> tuple[dict[str, Any], ...]:
    """Compile exact out-of-root definition TUs with final-build profiles."""

    registered = {
        path.casefold() for path in closure.registered_source_paths
    }
    additional = tuple(
        path
        for path in closure.definition_source_paths
        if path.casefold() not in registered
    )
    if not additional:
        return ()

    config = load_final_build_config(DEFAULT_FINAL_BUILD_MANIFEST)
    config = replace(config, vc5_env=vc5_env)
    requested_build_dir = (
        build_root.resolve() / "call-contract-definition-closure"
    )
    try:
        requested_build_dir_relative = requested_build_dir.relative_to(
            REPO_ROOT.resolve()
        )
    except ValueError as exc:
        raise ValueError(
            "call-contract definition-closure build root must remain inside "
            "the repository"
        ) from exc
    config = with_final_build_dir(config, requested_build_dir_relative)
    prepare_clean_build_dir(build_root, "call-contract-definition-closure")
    paths = final_build_paths(config)
    repository_inventory = load_repository_path_inventory(REPO_ROOT)
    configured_rows, _stale_diagnostics = _call_contract_final_build_source_map(
        config,
        inventory=repository_inventory,
    )
    configured = {
        key: rows[0][1]
        for key, rows in configured_rows.items()
        if len(rows) == 1
    }
    rows: list[dict[str, Any]] = []
    for relative in additional:
        source = configured.get(relative.casefold())
        if source is None:
            raise ValueError(
                "call-contract definition source is not configured by the "
                f"final build: {relative}"
            )
        command, object_path = make_final_build_compile_command(
            config,
            paths,
            source,
        )
        result = run_final_build_command(command)
        row = {
            "source": relative,
            "returncode": result.returncode,
            "object_path": object_path.resolve()
            .relative_to(REPO_ROOT.resolve())
            .as_posix(),
            "stdout_log": result.stdout_log.resolve()
            .relative_to(REPO_ROOT.resolve())
            .as_posix(),
            "stderr_log": result.stderr_log.resolve()
            .relative_to(REPO_ROOT.resolve())
            .as_posix(),
        }
        rows.append(row)
        if result.returncode != 0:
            tail = [
                *read_final_build_log_tail(result.stdout_log),
                *read_final_build_log_tail(result.stderr_log),
            ]
            detail = " | ".join(tail) if tail else "no compiler diagnostic"
            raise ValueError(
                "call-contract definition-closure VC5 compile failed for "
                f"{relative} with exit code {result.returncode}: {detail}; "
                f"logs: {row['stdout_log']}, {row['stderr_log']}"
            )
    return tuple(rows)


def _compile_definition_closure_after_comparison(
    first_divergence: Mapping[str, Any] | None,
    closure: CallContractSourceClosure,
    *,
    build_root: Path,
    vc5_env: Path,
) -> tuple[dict[str, Any], ...]:
    """Compile the complete definition closure only for a semantic PASS."""

    if first_divergence is not None:
        return ()
    return _compile_call_contract_definition_sources(
        closure,
        build_root=build_root,
        vc5_env=vc5_env,
    )


def _source_signature_recheck(
    first_divergence: dict[str, Any] | None,
    signatures_before: list[dict[str, Any]],
    signatures_after: list[dict[str, Any]],
) -> tuple[dict[str, Any] | None, bool]:
    """Apply the existing fail-closed signature rule without changing priority."""

    changed = signatures_before != signatures_after
    if changed and first_divergence is None:
        first_divergence = {
            "kind": "source-dependency-changed-during-validation",
            "before": signatures_before,
            "after": signatures_after,
        }
    return first_divergence, changed


def source_dependency_paths(
    document: ProgressDocument,
    slice_row: Mapping[str, Any],
    *,
    source_write_paths: Sequence[str] | None = None,
    source_closure: CallContractSourceClosure | None = None,
    repository_path_inventory: RepositoryPathInventory | None = None,
) -> list[str]:
    closure = source_closure or call_contract_source_closure(
        document,
        slice_row,
        repository_path_inventory=repository_path_inventory,
    )
    paths = set(closure.dependency_paths)
    if source_write_paths is not None:
        paths.update(source_write_paths)
    return sorted(paths, key=lambda path: (path.casefold(), path))
