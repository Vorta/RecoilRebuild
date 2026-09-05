from __future__ import annotations

import sys
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import argparse
from dataclasses import dataclass, replace
from functools import lru_cache
import json
import os
from pathlib import Path
import re
import shutil
import sys
import tempfile
from typing import Any, Mapping

from _recoil.commands.vc5_verify import (
    DEFAULT_MANIFEST_DIR as DEFAULT_VC5_VERIFY_MANIFEST_DIR,
    LinkedFunctionInterval,
    VerifyFunction,
    VerifyTarget,
    authored_order_role,
    function_authored_order_gate,
    function_authored_relative_order_gate,
    function_required_in_scope,
    load_manifest as load_vc5_verify_manifest,
    load_manifests as load_vc5_verify_manifests,
    safe_path_component,
)

from _recoil.lib.tooling import (
    REPO_ROOT,
    display_path,
    quote_cmd_arg,
    repo_path,
    require_string,
    require_string_list,
    response_line,
    run_cmd_script,
)
from _recoil.lib.coff_alias import (
    CoffAliasSource,
    parse_coff_alias_sources,
    resolve_llvm_ml,
    validate_alias_object,
    validate_alias_source,
)
from _recoil.lib.progress import (
    AUTHORED_PIPELINE_CLASSES,
    DEFAULT_PROGRESS_PATH,
    ProgressDocument,
    resolve_full_order_target_block,
)
from _recoil.lib.repository_paths import (
    load_repository_path_inventory,
    normalize_generated_repository_path,
    resolve_repository_file,
)
from _recoil.lib.authored_icf import require_valid_authored_icf_groups
from _recoil.lib.progress import (
    logical_alias_authored_order_role,
    symbol_authored_order_role,
    symbol_logical_aliases,
)
from _recoil.lib.vc5_compile_topology import (
    CanonicalMfc,
    PchTopology,
    coff_function_inventory,
    include_trace_report,
    parse_canonical_mfc,
    parse_pch_topology,
    reject_raw_topology_flags,
    topology_args,
    validate_canonical_mfc_roots,
)
DEFAULT_MANIFEST = REPO_ROOT / "tools" / "_recoil" / "config" / "vc5_final_build.json"
DEFAULT_PROGRESS = DEFAULT_PROGRESS_PATH
DEFAULT_PROFILES = REPO_ROOT / "tools" / "_recoil" / "config" / "compiler_linker_profiles.json"
DEFAULT_PLAYTEST_OUTPUT = REPO_ROOT / "playground" / "Recoil-rebuild.exe"
AUTHORED_INVERSION_DIAGNOSTIC_LIMIT = 25
SEMANTIC_LINKED_INVENTORY_LIMIT = 25
SEMANTIC_LINKED_CONSOLE_DIAGNOSTIC_LIMIT = 8


AuthoredOrderClass = tuple[int, int, bool]
AuthoredInversionPair = tuple[AuthoredOrderClass, AuthoredOrderClass]


@dataclass(frozen=True)
class LinkInput:
    kind: str
    value: str


@dataclass(frozen=True)
class FinalBuildConfig:
    name: str
    description: str
    vc5_env: Path
    build_dir: Path
    build_dir_explicit: bool
    output_exe: str
    playtest_output_exe: Path | None
    output_map: str
    resource_script: Path | None
    resource_output: str
    message_source: Path | None
    defines: tuple[str, ...]
    include_dirs: tuple[Path, ...]
    lib_dirs: tuple[Path, ...]
    compile_flags: tuple[str, ...]
    resource_flags: tuple[str, ...]
    link_flags: tuple[str, ...]
    libs: tuple[str, ...]
    sources: tuple[Path, ...]
    coff_alias_sources: tuple[CoffAliasSource, ...]
    link_inputs: tuple[LinkInput, ...] | None
    required_order_targets: tuple[str, ...]
    diagnostic_only: bool
    diagnostic_kind: str
    link_profile: str
    compile_profile: str
    source_compile_profiles: tuple[tuple[str, str], ...]
    library_profile: str
    pch_topology: PchTopology | None
    canonical_mfc: CanonicalMfc | None
    manifest_path: Path


@dataclass(frozen=True)
class BuildPaths:
    build_dir: Path
    obj_dir: Path
    logs_dir: Path
    rsp_dir: Path
    exe_path: Path
    map_path: Path
    resource_path: Path
    message_dir: Path
    message_rc_path: Path | None
    summary_path: Path


@dataclass(frozen=True)
class CommandSpec:
    name: str
    command: str
    cwd: Path
    stdout_log: Path
    stderr_log: Path


@dataclass(frozen=True)
class CommandResult:
    name: str
    returncode: int
    stdout_log: Path
    stderr_log: Path


@dataclass(frozen=True)
class LinkedMapSymbol:
    segment: int
    offset: int
    symbol: str
    address: int
    flags: tuple[str, ...]
    object: str
    source: str

    @property
    def is_function(self) -> bool:
        return "f" in self.flags


@dataclass(frozen=True)
class LinkedMapSection:
    segment: int
    offset: int
    length: int
    name: str
    section_class: str


@dataclass(frozen=True)
class ParsedLinkMap:
    preferred_load_address: int
    symbols: tuple[LinkedMapSymbol, ...]
    sections: tuple[LinkedMapSection, ...] = ()


@dataclass(frozen=True)
class LinkedOrderContribution:
    segment: int
    offset: int
    linked_address: int
    linked_rva: int
    symbols: tuple[str, ...]
    providers: tuple[str, ...]
    map_sources: tuple[str, ...]
    manifest_address: str
    manifest_name: str
    disposition: str


@dataclass(frozen=True)
class RawObjectFunctionDefinition:
    object_path: str
    symbols: tuple[str, ...]
    comdat: bool = False
    weak: bool = False


@dataclass(frozen=True)
class LinkedOrderCheck:
    target_name: str
    interval_name: str
    contributions: tuple[LinkedOrderContribution, ...]
    diagnostics: tuple[str, ...]
    order_scope: str = "full"
    required_presence_passed: bool = False
    authored_relative_order_passed: bool = False
    block_precedence_passed: bool = False
    exact_selected_sequence_matches_manifest: bool = False
    classified_selected_extras: tuple[dict[str, object], ...] = ()
    unclassified_selected_extras: tuple[dict[str, object], ...] = ()
    raw_authored_order_evaluated: bool = False
    raw_authored_order_passed: bool | None = None
    linked_known_authored_relative_order_passed: bool = False
    linked_known_authored_relative_order_scope: str = "manifest-local"
    linked_required_presence_passed: bool = False
    linked_projection_complete: bool = False
    linked_scope_projection_complete: bool = False
    linked_exact_selected_population_evaluated: bool = False
    linked_exact_selected_population_passed: bool | None = None
    linked_seams_and_rvas_evaluated: bool = False
    linked_seams_and_rvas_passed: bool | None = None
    alias_classifications: tuple[dict[str, object], ...] = ()
    physical_classes: tuple[dict[str, object], ...] = ()
    required_identity_dispositions: tuple[dict[str, object], ...] = ()
    raw_definition_inventory_complete: bool | None = None
    raw_definition_inventory_diagnostics: tuple[str, ...] = ()
    nonblocking_diagnostics: tuple[str, ...] = ()
    identity_catalog_source: str = "manifest-local"
    tracker_revision: int | None = None
    diagnostic_mode_kind: str = ""
    diagnostic_mode_applied: bool = False
    diagnostic_nonblocking_reason: str = ""
    diagnostic_predicate_results: tuple[dict[str, object], ...] = ()
    controlled_identity_assertions: tuple[dict[str, object], ...] = ()
    controlled_relative_order_assertions: tuple[dict[str, object], ...] = ()
    boundary_offender_proof_complete: bool = False
    boundary_offender_proof_diagnostics: tuple[str, ...] = ()
    boundary_offenders: tuple[dict[str, object], ...] = ()

    @property
    def ok(self) -> bool:
        return not self.diagnostics


@dataclass(frozen=True)
class OrderTargetRouting:
    diagnostic_isolation_applied: bool
    explicit_target_ids: tuple[str, ...]
    effective_target_ids: tuple[str, ...]
    mandatory_target_ids: tuple[str, ...]
    recognized_diagnostic_kind: str = ""
    activation: tuple[tuple[str, object], ...] = ()


@dataclass(frozen=True)
class LinkedRetailIdentityCatalog:
    functions: tuple[VerifyFunction, ...]
    source: str = "manifest-local"
    tracker_revision: int | None = None
    authored_order_prefix_end: str = ""


@dataclass(frozen=True)
class AuthoredInversionSummary:
    blocking_count: int
    future_count: int
    blocking_examples: tuple[AuthoredInversionPair, ...]
    future_examples: tuple[AuthoredInversionPair, ...]


class _PriorOrderClassIndex:
    """Fenwick-backed prior-row index for bounded inversion examples."""

    def __init__(self, rank_count: int) -> None:
        self._tree = [0] * (rank_count + 1)
        self._rows: list[list[AuthoredOrderClass]] = [
            [] for _ in range(rank_count + 1)
        ]
        self._total = 0

    def add(self, rank: int, row: AuthoredOrderClass) -> None:
        self._rows[rank].append(row)
        self._total += 1
        cursor = rank
        while cursor < len(self._tree):
            self._tree[cursor] += 1
            cursor += cursor & -cursor

    def _prefix_count(self, rank: int) -> int:
        total = 0
        cursor = rank
        while cursor:
            total += self._tree[cursor]
            cursor -= cursor & -cursor
        return total

    def count_above(self, rank: int) -> int:
        return self._total - self._prefix_count(rank)

    def _rank_for_ordinal(self, ordinal: int) -> int:
        cursor = 0
        accumulated = 0
        step = 1 << (len(self._tree).bit_length() - 1)
        while step:
            candidate = cursor + step
            if (
                candidate < len(self._tree)
                and accumulated + self._tree[candidate] < ordinal
            ):
                cursor = candidate
                accumulated += self._tree[candidate]
            step >>= 1
        return cursor + 1

    def examples_above(
        self,
        rank: int,
        limit: int,
    ) -> tuple[AuthoredOrderClass, ...]:
        if limit <= 0:
            return ()
        skipped = self._prefix_count(rank)
        take = min(limit, self._total - skipped)
        examples: list[AuthoredOrderClass] = []
        for ordinal in range(skipped + 1, skipped + take + 1):
            selected_rank = self._rank_for_ordinal(ordinal)
            preceding = self._prefix_count(selected_rank - 1)
            examples.append(self._rows[selected_rank][ordinal - preceding - 1])
        return tuple(examples)


def summarize_authored_inversions(
    scalar_order_classes: list[AuthoredOrderClass],
    *,
    diagnostic_limit: int = AUTHORED_INVERSION_DIAGNOSTIC_LIMIT,
) -> AuthoredInversionSummary:
    """Count inversions exactly in O(n log n), retaining only bounded examples.

    Rows are in candidate linked order and contain ``(linked, retail, blocks)``.
    A pair is blocking when either endpoint belongs to the current blocking
    scope; a pair is future-only when both endpoints are outside that scope.
    """
    if diagnostic_limit < 0:
        raise ValueError("diagnostic_limit must be non-negative")
    if not scalar_order_classes:
        return AuthoredInversionSummary(0, 0, (), ())

    retail_ranks = {
        retail: rank
        for rank, retail in enumerate(
            sorted({row[1] for row in scalar_order_classes}),
            start=1,
        )
    }
    all_prior = _PriorOrderClassIndex(len(retail_ranks))
    blocking_prior = _PriorOrderClassIndex(len(retail_ranks))
    future_prior = _PriorOrderClassIndex(len(retail_ranks))
    blocking_count = 0
    future_count = 0
    blocking_examples: list[AuthoredInversionPair] = []
    future_examples: list[AuthoredInversionPair] = []

    for current in scalar_order_classes:
        rank = retail_ranks[current[1]]
        if current[2]:
            blocking_count += all_prior.count_above(rank)
            prior_examples = all_prior.examples_above(
                rank,
                diagnostic_limit - len(blocking_examples),
            )
            blocking_examples.extend((prior, current) for prior in prior_examples)
        else:
            blocking_count += blocking_prior.count_above(rank)
            prior_examples = blocking_prior.examples_above(
                rank,
                diagnostic_limit - len(blocking_examples),
            )
            blocking_examples.extend((prior, current) for prior in prior_examples)

            future_count += future_prior.count_above(rank)
            prior_examples = future_prior.examples_above(
                rank,
                diagnostic_limit - len(future_examples),
            )
            future_examples.extend((prior, current) for prior in prior_examples)

        all_prior.add(rank, current)
        if current[2]:
            blocking_prior.add(rank, current)
        else:
            future_prior.add(rank, current)

    return AuthoredInversionSummary(
        blocking_count=blocking_count,
        future_count=future_count,
        blocking_examples=tuple(blocking_examples),
        future_examples=tuple(future_examples),
    )


MAP_SYMBOL_RE = re.compile(
    r"^\s*(?P<segment>[0-9A-Fa-f]{4}):(?P<offset>[0-9A-Fa-f]{8})\s+"
    r"(?P<symbol>.+?)\s+(?P<address>[0-9A-Fa-f]{8,16})\s+(?P<tail>.+?)\s*$"
)
PREFERRED_LOAD_ADDRESS_RE = re.compile(r"^\s*Preferred load address is\s+([0-9A-Fa-f]+)\s*$")
MAP_SECTION_RE = re.compile(
    r"^\s*(?P<segment>[0-9A-Fa-f]{4}):(?P<offset>[0-9A-Fa-f]{8})\s+"
    r"(?P<length>[0-9A-Fa-f]{8})H\s+(?P<name>\S+)\s+(?P<class>\S+)\s*$"
)


def load_config(path: str | Path = DEFAULT_MANIFEST) -> FinalBuildConfig:
    manifest_path = Path(path)
    with manifest_path.open("r", encoding="utf-8") as handle:
        data = json.load(handle)
    if not isinstance(data, dict):
        raise ValueError(f"{manifest_path}: manifest root must be an object")
    message_source_raw = data.get("message_source")
    if message_source_raw is not None and not isinstance(message_source_raw, str):
        raise ValueError(f"{manifest_path}: message_source must be a string when present")
    resource_script_raw = data.get("resource_script")
    if resource_script_raw is not None and not isinstance(resource_script_raw, str):
        raise ValueError(f"{manifest_path}: resource_script must be a string when present")
    if resource_script_raw is None and message_source_raw is None:
        raise ValueError(f"{manifest_path}: either resource_script or message_source is required")
    playtest_output_raw = data.get("playtest_output_exe")
    if playtest_output_raw is not None:
        if not isinstance(playtest_output_raw, str) or not playtest_output_raw:
            raise ValueError(
                f"{manifest_path}: playtest_output_exe must be a non-empty repository-relative string when present"
            )
        if Path(playtest_output_raw).is_absolute():
            raise ValueError(
                f"{manifest_path}: playtest_output_exe must be repository-relative"
            )
        playtest_output_exe = repo_path(playtest_output_raw)
        try:
            playtest_output_exe.resolve().relative_to(REPO_ROOT.resolve())
        except ValueError as exc:
            raise ValueError(
                f"{manifest_path}: playtest_output_exe must remain inside the repository"
            ) from exc
    else:
        playtest_output_exe = None

    libs = require_string_list(data, "libs", manifest_path=manifest_path)
    source_paths = tuple(repo_path(item) for item in require_string_list(data, "sources", manifest_path=manifest_path))
    build_dir = repo_path(require_string(data, "build_dir", manifest_path=manifest_path))
    compile_flags = require_string_list(data, "compile_flags", manifest_path=manifest_path)
    reject_raw_topology_flags(compile_flags, label=f"{manifest_path}: compile_flags")
    diagnostic_only = data.get("diagnostic_only", False)
    if not isinstance(diagnostic_only, bool):
        raise ValueError(f"{manifest_path}: diagnostic_only must be boolean")
    diagnostic_kind = data.get("diagnostic_kind", "")
    link_profile = data.get("link_profile", "")
    if not isinstance(diagnostic_kind, str) or not isinstance(link_profile, str):
        raise ValueError(f"{manifest_path}: diagnostic_kind/link_profile must be strings")
    raw_link_flags = require_string_list(data, "link_flags", manifest_path=manifest_path)
    if link_profile:
        if not diagnostic_only:
            raise ValueError(f"{manifest_path}: link_profile is restricted to diagnostic_only manifests")
        conflicts = [flag for flag in raw_link_flags if flag.upper().startswith("/OPT:")]
        if conflicts:
            raise ValueError(f"{manifest_path}: link_profile conflicts with raw /OPT flags: {', '.join(conflicts)}")
        raw_link_flags = (*raw_link_flags, *diagnostic_link_profile_flags(link_profile))
    pch_topology = parse_pch_topology(
        data.get("pch"),
        sources=source_paths,
        build_dir=build_dir,
        resolve_path=repo_path,
        manifest_path=manifest_path,
    )
    canonical_mfc = parse_canonical_mfc(
        data.get("canonical_mfc"),
        resolve_path=repo_path,
        manifest_path=manifest_path,
        repository_root=REPO_ROOT,
    )
    if pch_topology is not None and not diagnostic_only:
        raise ValueError(f"{manifest_path}: PCH topology is diagnostic-only until reviewed")
    if diagnostic_only:
        try:
            build_dir.resolve().relative_to((REPO_ROOT / "build" / "vc5-probes").resolve())
        except ValueError as exc:
            raise ValueError(f"{manifest_path}: diagnostic build_dir must be inside build/vc5-probes") from exc
    config = FinalBuildConfig(
        name=require_string(data, "name", manifest_path=manifest_path),
        description=require_string(data, "description", manifest_path=manifest_path),
        vc5_env=repo_path(require_string(data, "vc5_env", manifest_path=manifest_path)),
        build_dir=build_dir,
        build_dir_explicit=False,
        output_exe=require_string(data, "output_exe", manifest_path=manifest_path),
        playtest_output_exe=playtest_output_exe,
        output_map=require_string(data, "output_map", manifest_path=manifest_path),
        resource_script=repo_path(resource_script_raw) if resource_script_raw is not None else None,
        resource_output=require_string(data, "resource_output", manifest_path=manifest_path),
        message_source=repo_path(message_source_raw) if message_source_raw is not None else None,
        defines=require_string_list(data, "defines", manifest_path=manifest_path),
        include_dirs=tuple(repo_path(item) for item in require_string_list(data, "include_dirs", manifest_path=manifest_path)),
        lib_dirs=tuple(repo_path(item) for item in require_string_list(data, "lib_dirs", manifest_path=manifest_path)),
        compile_flags=compile_flags,
        resource_flags=require_string_list(data, "resource_flags", manifest_path=manifest_path),
        link_flags=raw_link_flags,
        libs=libs,
        sources=source_paths,
        coff_alias_sources=parse_coff_alias_sources(
            data,
            configured_sources=source_paths,
            manifest_path=manifest_path,
        ),
        link_inputs=parse_link_inputs(data, source_paths, libs, manifest_path),
        required_order_targets=require_string_list(
            data,
            "required_order_targets",
            manifest_path=manifest_path,
        ),
        diagnostic_only=diagnostic_only,
        diagnostic_kind=diagnostic_kind,
        link_profile=link_profile,
        compile_profile="",
        source_compile_profiles=(),
        library_profile="",
        pch_topology=pch_topology,
        canonical_mfc=canonical_mfc,
        manifest_path=manifest_path,
    )
    raw_source_profiles = data.get("source_compile_profiles", {})
    if not isinstance(raw_source_profiles, dict):
        raise ValueError(f"{manifest_path}: source_compile_profiles must be an object")
    manifest_mappings: dict[str, str] = {}
    configured_sources = (
        {canonical_source_key(source) for source in config.sources}
        if raw_source_profiles else set()
    )
    for source_text, profile in raw_source_profiles.items():
        if not isinstance(source_text, str) or not source_text or not isinstance(profile, str) or not profile:
            raise ValueError(f"{manifest_path}: source_compile_profiles requires non-empty SOURCE: PROFILE strings")
        if any(char in source_text for char in "*?[]{}"):
            raise ValueError(f"{manifest_path}: source_compile_profiles requires exact source paths")
        key = canonical_source_key(repo_path(source_text))
        if key not in configured_sources:
            raise ValueError(f"{manifest_path}: source_compile_profiles references an unconfigured source: {source_text}")
        if key in manifest_mappings:
            raise ValueError(f"{manifest_path}: duplicate normalized source_compile_profiles path: {source_text}")
        final_build_compile_profile_flags(profile, config)
        manifest_mappings[key] = profile
    if manifest_mappings:
        config = replace(config, source_compile_profiles=tuple(sorted(manifest_mappings.items())))
    if config.canonical_mfc is not None:
        validate_canonical_mfc_roots(config.canonical_mfc, config.include_dirs, config.lib_dirs)
    return config


def diagnostic_link_profile_flags(name: str) -> tuple[str, ...]:
    data = json.loads(DEFAULT_PROFILES.read_text(encoding="utf-8"))
    rows = data.get("diagnostic_link_profiles", [])
    if not isinstance(rows, list):
        raise ValueError(f"{DEFAULT_PROFILES}: diagnostic_link_profiles must be a list")
    for row in rows:
        if isinstance(row, dict) and row.get("name") == name:
            flags = row.get("link_flags")
            if not isinstance(flags, list) or not all(isinstance(item, str) and item for item in flags):
                raise ValueError(f"{DEFAULT_PROFILES}: invalid link_flags for {name}")
            return tuple(flags)
    raise ValueError(f"{DEFAULT_PROFILES}: unknown diagnostic link profile {name}")


def _profile_rows(key: str) -> list[dict[str, Any]]:
    data = json.loads(DEFAULT_PROFILES.read_text(encoding="utf-8"))
    rows = data.get(key, [])
    if not isinstance(rows, list):
        raise ValueError(f"{DEFAULT_PROFILES}: {key} must be a list")
    return [row for row in rows if isinstance(row, dict)]


def final_build_compile_profile_flags(name: str, config: FinalBuildConfig) -> tuple[str, ...]:
    for row in _profile_rows("verification_profiles"):
        if row.get("name") != name:
            continue
        if repo_path(str(row.get("compiler_env", ""))).resolve() != config.vc5_env.resolve():
            raise ValueError(f"{DEFAULT_PROFILES}: compiler environment mismatch for {name}")
        flags = row.get("final_build_compile_flags")
        if not isinstance(flags, list) or not all(isinstance(item, str) and item for item in flags):
            raise ValueError(
                f"{DEFAULT_PROFILES}: profile {name} has no reviewed final_build_compile_flags"
            )
        reject_raw_topology_flags(tuple(flags), label=f"{DEFAULT_PROFILES}: {name}.final_build_compile_flags")
        if any(flag.upper() in {"/I", "/FACS"} or flag.upper().startswith("/I.") for flag in flags):
            raise ValueError(f"{DEFAULT_PROFILES}: profile {name} contains verification-only include/listing flags")
        return tuple(flags)
    raise ValueError(f"{DEFAULT_PROFILES}: unknown verification profile {name}")


@lru_cache(maxsize=1)
def _final_build_repository_inventory():
    return load_repository_path_inventory(REPO_ROOT)


def _authored_repository_path(path: Path, *, context: str) -> str:
    absolute = path if path.is_absolute() else REPO_ROOT / path
    try:
        lexical = absolute.absolute().relative_to(REPO_ROOT.absolute()).as_posix()
    except ValueError as exc:
        raise ValueError(f"{context} is outside the repository: {path}") from exc
    return resolve_repository_file(
        lexical,
        repository_root=REPO_ROOT,
        inventory=_final_build_repository_inventory(),
        context=context,
    ).repository_path


def canonical_source_key(source: Path) -> str:
    return _authored_repository_path(
        source,
        context="final-build source profile path",
    ).casefold()


def effective_compile_flags(config: FinalBuildConfig, source: Path) -> tuple[str, ...]:
    profiles = dict(config.source_compile_profiles)
    if not profiles and not config.compile_profile:
        return config.compile_flags
    name = profiles.get(canonical_source_key(source), config.compile_profile)
    return final_build_compile_profile_flags(name, config) if name else config.compile_flags


def linked_order_compile_profile_provenance(
    target: VerifyTarget,
    config: FinalBuildConfig,
    manifests: tuple[VerifyTarget, ...],
) -> dict[str, Any]:
    """Resolve and verify the final-build TU profiles required by a linked target.

    Linked-only order targets have no compile step of their own.  When they
    declare ``compiler_profile``, their TU set is taken from their explicit
    source/TU rows or from the unique same-range object-order manifest.  This
    keeps a linked order result from silently claiming a profile that the
    objects in the final link did not use.
    """

    expected_profile = target.compiler_profile
    if not expected_profile and not target.source_compile_profiles:
        return {
            "declared": False,
            "evaluated": False,
            "passed": True,
            "expected_profile": "",
            "expected_source_profiles": [],
            "contract_manifests": [],
            "translation_units": [],
            "diagnostics": [],
        }

    diagnostics: list[str] = []
    contract_manifests: tuple[VerifyTarget, ...] = ()
    source_rows = tuple(
        dict.fromkeys(
            [
                *((target.source_from,) if target.source_from else ()),
                *(entry.source_from for entry in target.translation_unit_function_order),
            ]
        )
    )
    if not source_rows:
        candidates = tuple(
            manifest
            for manifest in manifests
            if manifest.manifest_path.resolve() != target.manifest_path.resolve()
            and manifest.target_binary == target.target_binary
            and manifest.compiler_profile == expected_profile
            and manifest.retail_start == target.retail_start
            and manifest.retail_end_exclusive == target.retail_end_exclusive
            and manifest.translation_unit_function_order
        )
        source_sets: dict[tuple[str, ...], list[VerifyTarget]] = {}
        for candidate in candidates:
            sources = tuple(
                dict.fromkeys(entry.source_from for entry in candidate.translation_unit_function_order)
            )
            source_sets.setdefault(sources, []).append(candidate)
        if len(source_sets) == 1:
            source_rows, matched = next(iter(source_sets.items()))
            contract_manifests = tuple(matched)
        elif not source_sets:
            diagnostics.append(
                "compiler profile contract has no explicit TU rows and no unique same-range object-order manifest"
            )
        else:
            diagnostics.append(
                "compiler profile contract resolves to multiple distinct same-range TU source sets"
            )
    else:
        contract_manifests = (target,)

    configured = {canonical_source_key(source): source for source in config.sources}
    mappings = dict(config.source_compile_profiles)
    expected_contexts: dict[str, tuple[str, ...]] = {}
    expected_sources: dict[str, str] = {}
    for source_text in source_rows:
        try:
            source_key = canonical_source_key(repo_path(source_text))
        except ValueError as exc:
            diagnostics.append(str(exc))
            continue
        source_profiles: set[str] = set()
        for manifest in contract_manifests:
            manifest_sources = tuple(
                dict.fromkeys(
                    [
                        *((manifest.source_from,) if manifest.source_from else ()),
                        *(entry.source_from for entry in manifest.translation_unit_function_order),
                    ]
                )
            )
            manifest_source_keys = {
                canonical_source_key(repo_path(manifest_source))
                for manifest_source in manifest_sources
            }
            if source_key not in manifest_source_keys:
                continue
            manifest_profile = dict(manifest.source_compile_profiles).get(
                source_key,
                manifest.compiler_profile,
            )
            if manifest_profile:
                source_profiles.add(manifest_profile)
        if not source_profiles:
            fallback_profile = dict(target.source_compile_profiles).get(
                source_key,
                target.compiler_profile,
            )
            if fallback_profile:
                source_profiles.add(fallback_profile)
        if not source_profiles:
            diagnostics.append(f"{source_text}: compiler profile contract has no declared profile")
            continue
        if len(source_profiles) != 1:
            diagnostics.append(
                f"{source_text}: compiler profile contract is ambiguous: "
                + ", ".join(sorted(source_profiles))
            )
            continue
        source_profile = next(iter(source_profiles))
        try:
            expected_contexts[source_key] = final_build_compile_profile_flags(source_profile, config)
        except ValueError as exc:
            diagnostics.append(str(exc))
            continue
        expected_sources[source_key] = source_profile

    rows: list[dict[str, Any]] = []
    for source_text in source_rows:
        source = repo_path(source_text)
        try:
            source_key = canonical_source_key(source)
        except ValueError as exc:
            diagnostics.append(str(exc))
            continue
        configured_source = configured.get(source_key)
        actual_profile = mappings.get(source_key, config.compile_profile or "manifest-default")
        source_expected_profile = expected_sources.get(source_key, "")
        expected_flags = expected_contexts.get(source_key, ())
        actual_flags = (
            effective_compile_flags(config, configured_source)
            if configured_source is not None
            else ()
        )
        profile_match = bool(source_expected_profile) and actual_profile == source_expected_profile
        flags_match = bool(expected_flags) and actual_flags == expected_flags
        passed = configured_source is not None and profile_match and flags_match
        row = {
            "source": source_text,
            "source_key": source_key,
            "configured": configured_source is not None,
            "expected_profile": source_expected_profile,
            "actual_profile": actual_profile,
            "expected_flags": list(expected_flags),
            "actual_flags": list(actual_flags),
            "profile_match": profile_match,
            "flags_match": flags_match,
            "passed": passed,
        }
        rows.append(row)
        if not passed:
            diagnostics.append(
                f"{source_text}: expected final-build profile {source_expected_profile or '<unresolved>'}, "
                f"actual {actual_profile}; exact final-build flags "
                f"{'match' if flags_match else 'do not match'}"
            )

    passed = bool(source_rows) and not diagnostics and all(row["passed"] for row in rows)
    contract_rows = [
        {
            "name": manifest.name,
            "path": report_path_key(manifest.manifest_path),
        }
        for manifest in contract_manifests
    ]
    return {
        "declared": True,
        "evaluated": True,
        "passed": passed,
        "expected_profile": expected_profile,
        "expected_source_profiles": [
            {"source_key": source_key, "profile": profile}
            for source_key, profile in sorted(expected_sources.items())
        ],
        "contract_manifests": contract_rows,
        "translation_units": rows,
        "diagnostics": diagnostics,
    }


def with_diagnostic_compile_profiles(
    config: FinalBuildConfig,
    *,
    compile_profile: str,
    source_profiles: tuple[str, ...],
) -> FinalBuildConfig:
    source_by_key = {canonical_source_key(source): source for source in config.sources}
    mappings: dict[str, str] = {}
    for item in source_profiles:
        if "=" not in item:
            raise ValueError("--source-profile requires SOURCE=PROFILE")
        source_text, profile = item.split("=", 1)
        if not source_text or not profile or any(char in source_text for char in "*?[]{}"):
            raise ValueError("--source-profile requires an exact SOURCE=PROFILE mapping")
        candidate = repo_path(source_text)
        key = canonical_source_key(candidate)
        if key not in source_by_key:
            raise ValueError(f"--source-profile references an unconfigured source: {source_text}")
        if key in mappings:
            raise ValueError(f"duplicate --source-profile assignment: {source_text}")
        final_build_compile_profile_flags(profile, config)
        mappings[key] = profile
    if compile_profile:
        final_build_compile_profile_flags(compile_profile, config)
    if not compile_profile and not mappings:
        return config
    return replace(
        config,
        diagnostic_only=True,
        diagnostic_kind="full-link-compile-profile-override",
        compile_profile=compile_profile,
        source_compile_profiles=tuple(sorted(mappings.items())),
    )


def with_diagnostic_library_profile(config: FinalBuildConfig, name: str) -> FinalBuildConfig:
    row = next((row for row in _profile_rows("diagnostic_library_profiles") if row.get("name") == name), None)
    if row is None:
        raise ValueError(f"{DEFAULT_PROFILES}: unknown diagnostic library profile {name}")
    libraries = row.get("libraries")
    flags = row.get("link_flags")
    if not isinstance(libraries, list) or len(libraries) != 2 or not all(isinstance(x, str) for x in libraries):
        raise ValueError(f"{DEFAULT_PROFILES}: {name} requires exactly two matched libraries")
    if not isinstance(flags, list) or not all(isinstance(x, str) for x in flags):
        raise ValueError(f"{DEFAULT_PROFILES}: invalid link flags for {name}")
    replacement = tuple(libraries)
    libs: list[str] = []
    for lib in config.libs:
        if Path(lib).name.upper() == "MFC42.LIB":
            libs.extend(replacement)
        elif Path(lib).name.upper() != "MFCS42.LIB":
            libs.append(lib)
    link_inputs = config.link_inputs
    if link_inputs is not None:
        replaced: list[LinkInput] = []
        for item in link_inputs:
            if item.kind == "lib" and Path(item.value).name.upper() == "MFC42.LIB":
                replaced.extend(LinkInput("lib", lib) for lib in replacement)
            elif not (item.kind == "lib" and Path(item.value).name.upper() == "MFCS42.LIB"):
                replaced.append(item)
        link_inputs = tuple(replaced)
    return replace(
        config,
        diagnostic_only=True,
        diagnostic_kind="full-link-library-profile-override",
        library_profile=name,
        libs=tuple(libs),
        link_inputs=link_inputs,
        link_flags=(*config.link_flags, *tuple(flags)),
    )


def with_diagnostic_link_profile(config: FinalBuildConfig, name: str) -> FinalBuildConfig:
    base_flags = tuple(flag for flag in config.link_flags if not flag.upper().startswith("/OPT:"))
    build_dir = (
        config.build_dir.parent / f"{config.build_dir.name}-{name}"
        if config.diagnostic_only
        else REPO_ROOT / "build" / "vc5-probes" / "full-link" / f"{config.name}-{name}"
    )
    return replace(
        config,
        diagnostic_only=True,
        diagnostic_kind=config.diagnostic_kind or "full-link-profile-override",
        link_profile=name,
        link_flags=(*base_flags, *diagnostic_link_profile_flags(name)),
        build_dir=build_dir,
    )


def paths_overlap(first: Path, second: Path) -> bool:
    first = first.resolve()
    second = second.resolve()
    return first == second or first in second.parents or second in first.parents


def validate_explicit_build_dir(directory: Path, *, canonical_build_dir: Path) -> Path:
    """Resolve one isolated mutable final-build root without broad clean authority."""
    if directory.is_absolute():
        raise ValueError("--build-dir must be a repository-relative path below build/")
    resolved = (REPO_ROOT / directory).resolve()
    build_root = (REPO_ROOT / "build").resolve()
    try:
        relative = resolved.relative_to(build_root)
    except ValueError as exc:
        raise ValueError("--build-dir must resolve below the repository build/ directory") from exc
    if not relative.parts:
        raise ValueError("--build-dir must name one isolated child below build/")

    canonical = canonical_build_dir.resolve()
    if paths_overlap(resolved, canonical):
        raise ValueError(
            "--build-dir must be isolated from the manifest's canonical build directory"
        )

    return resolved


def with_explicit_build_dir(config: FinalBuildConfig, directory: Path) -> FinalBuildConfig:
    if config.pch_topology is not None:
        raise ValueError(
            "--build-dir does not support manifests whose PCH topology embeds the canonical build directory"
        )
    return replace(
        config,
        build_dir=validate_explicit_build_dir(
            directory,
            canonical_build_dir=config.build_dir,
        ),
        build_dir_explicit=True,
    )


def finalize_diagnostic_build_dir(config: FinalBuildConfig) -> FinalBuildConfig:
    if not config.diagnostic_only:
        return config
    if config.source_compile_profiles:
        compile_tag = (
            f"{config.compile_profile}-source-map"
            if config.compile_profile
            else "source-map"
        )
    else:
        compile_tag = config.compile_profile or "default"
    suffix = "-".join((compile_tag, config.library_profile or "canonical", config.link_profile or "manifest-link"))
    expected_root = REPO_ROOT / "build" / "vc5-probes" / "full-link" / "link-runs"
    build_dir = expected_root / safe_path_component(config.name) / suffix
    topology = config.pch_topology
    if topology is not None:
        try:
            relative_pch = topology.pch_path.resolve().relative_to(
                config.build_dir.resolve()
            )
        except ValueError as exc:
            raise ValueError(
                "diagnostic PCH output must stay inside the manifest build directory"
            ) from exc
        topology = replace(topology, pch_path=build_dir / relative_pch)
    return replace(config, build_dir=build_dir, pch_topology=topology)


def parse_link_map(path: Path) -> ParsedLinkMap:
    preferred_load_address = 0
    current_source = ""
    symbols: list[LinkedMapSymbol] = []
    sections: list[LinkedMapSection] = []
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        preferred_match = PREFERRED_LOAD_ADDRESS_RE.match(line)
        if preferred_match:
            preferred_load_address = int(preferred_match.group(1), 16)
            continue
        section_match = MAP_SECTION_RE.match(line)
        if section_match:
            sections.append(
                LinkedMapSection(
                    segment=int(section_match.group("segment"), 16),
                    offset=int(section_match.group("offset"), 16),
                    length=int(section_match.group("length"), 16),
                    name=section_match.group("name"),
                    section_class=section_match.group("class"),
                )
            )
            continue
        stripped = line.strip()
        if "Publics by Value" in stripped:
            current_source = "Publics by Value"
            continue
        if stripped == "Static symbols":
            current_source = "Static symbols"
            continue
        match = MAP_SYMBOL_RE.match(line)
        if not match or current_source not in {"Publics by Value", "Static symbols"}:
            continue
        tail_parts = match.group("tail").strip().split()
        flags: list[str] = []
        while tail_parts and tail_parts[0] in {"f", "i"}:
            flags.append(tail_parts.pop(0))
        symbols.append(
            LinkedMapSymbol(
                segment=int(match.group("segment"), 16),
                offset=int(match.group("offset"), 16),
                symbol=match.group("symbol").strip(),
                address=int(match.group("address"), 16),
                flags=tuple(flags),
                object=" ".join(tail_parts),
                source=current_source,
            )
        )
    if preferred_load_address == 0:
        raise ValueError(f"{path}: preferred load address was not found")
    if not symbols:
        raise ValueError(f"{path}: no Publics by Value or Static symbols rows were found")
    return ParsedLinkMap(
        preferred_load_address=preferred_load_address,
        symbols=tuple(symbols),
        sections=tuple(sections),
    )


def resolve_linked_function_group(
    function: VerifyFunction,
    groups: dict[int, tuple[LinkedMapSymbol, ...]],
) -> tuple[LinkedMapSymbol, ...]:
    matches = _matching_linked_function_groups(function, groups)
    if len(matches) != 1:
        if not matches:
            detail = "no selected function matches"
        else:
            detail = "matches at " + ", ".join(f"0x{group[0].address:x}" for group in matches[:8])
        raise ValueError(f"{function.address} {function.name}: {detail}")
    return matches[0]


def linked_function_groups(parsed_map: ParsedLinkMap) -> dict[int, tuple[LinkedMapSymbol, ...]]:
    grouped: dict[int, list[LinkedMapSymbol]] = {}
    for symbol in parsed_map.symbols:
        if symbol.is_function:
            grouped.setdefault(symbol.address, []).append(symbol)
    return {
        address: tuple(sorted(items, key=lambda item: (item.source, item.symbol, item.object)))
        for address, items in grouped.items()
    }


def _function_selector_matches(function: VerifyFunction, symbol: str) -> bool:
    # Use the same selector precedence as the COFF/order verifier.
    if function.symbol_regex is not None:
        return re.fullmatch(function.symbol_regex, symbol) is not None
    return bool(function.symbol and function.symbol == symbol)


def _matching_linked_function_groups(
    function: VerifyFunction,
    groups: dict[int, tuple[LinkedMapSymbol, ...]],
    *,
    public_only: bool = False,
) -> list[tuple[LinkedMapSymbol, ...]]:
    public_matches = [
        group
        for group in groups.values()
        if any(
            item.source == "Publics by Value"
            and _function_selector_matches(function, item.symbol)
            for item in group
        )
    ]
    if public_only or public_matches:
        return public_matches
    return [
        group
        for group in groups.values()
        if any(_function_selector_matches(function, item.symbol) for item in group)
    ]


def collect_raw_object_function_definitions(
    config: FinalBuildConfig,
) -> tuple[tuple[RawObjectFunctionDefinition, ...], tuple[str, ...]]:
    paths = build_paths(config)
    definitions: list[RawObjectFunctionDefinition] = []
    diagnostics: list[str] = []
    for source in config.sources:
        path = object_path(config, paths, source)
        if not path.is_file():
            diagnostics.append(f"raw COFF inventory object is missing: {path}")
            continue
        try:
            inventory = coff_function_inventory(path)
        except (OSError, ValueError) as exc:
            diagnostics.append(f"raw COFF inventory failed for {path}: {exc}")
            continue
        functions = inventory.get("functions", [])
        if not isinstance(functions, list):
            diagnostics.append(f"raw COFF inventory has invalid function rows: {path}")
            continue
        for function in functions:
            if not isinstance(function, dict):
                continue
            symbols = function.get("symbols", [])
            if not isinstance(symbols, list) or not all(isinstance(item, str) for item in symbols):
                continue
            definitions.append(
                RawObjectFunctionDefinition(
                    object_path=report_path_key(path),
                    symbols=tuple(symbols),
                    comdat=bool(function.get("comdat", False)),
                    weak=bool(function.get("weak_externals", [])),
                )
            )
    return tuple(definitions), tuple(diagnostics)


def _matching_raw_definitions(
    function: VerifyFunction,
    raw_definitions: tuple[RawObjectFunctionDefinition, ...],
) -> tuple[RawObjectFunctionDefinition, ...]:
    return tuple(
        definition
        for definition in raw_definitions
        if any(_function_selector_matches(function, symbol) for symbol in definition.symbols)
    )


def _same_linked_retail_identity(
    left: VerifyFunction,
    right: VerifyFunction,
) -> bool:
    if left.logical_identity_key and right.logical_identity_key:
        return left.logical_identity_key == right.logical_identity_key
    return (
        left.address == right.address
        and left.name == right.name
        and authored_order_role(left) == authored_order_role(right)
    )


class _LinkedRetailIdentityLookup:
    """Pre-index the catalog for repeated MAP-alias and selector resolution."""

    def __init__(self, functions: tuple[VerifyFunction, ...]) -> None:
        exact_aliases: dict[str, list[VerifyFunction]] = {}
        logical_identities: dict[str, list[VerifyFunction]] = {}
        fallback_identities: dict[tuple[str, str, str], list[VerifyFunction]] = {}
        regex_aliases: list[tuple[re.Pattern[str], VerifyFunction]] = []
        for function in functions:
            if function.symbol:
                exact_aliases.setdefault(function.symbol, []).append(function)
            elif function.symbol_regex is not None:
                regex_aliases.append((re.compile(function.symbol_regex), function))
            if function.logical_identity_key:
                logical_identities.setdefault(function.logical_identity_key, []).append(function)
            fallback_identities.setdefault(
                (function.address, function.name, authored_order_role(function)),
                [],
            ).append(function)
        self._exact_aliases = exact_aliases
        self._regex_aliases = tuple(regex_aliases)
        self._logical_identities = logical_identities
        self._fallback_identities = fallback_identities

    def matching_alias(self, alias: str) -> tuple[VerifyFunction, ...]:
        exact = self._exact_aliases.get(alias)
        if exact:
            return _deduplicate_identity_functions(tuple(exact))
        return _deduplicate_identity_functions(tuple(
            function
            for pattern, function in self._regex_aliases
            if pattern.fullmatch(alias)
        ))

    def selectors_for(self, function: VerifyFunction) -> tuple[VerifyFunction, ...]:
        fallback_key = (
            function.address,
            function.name,
            authored_order_role(function),
        )
        if function.logical_identity_key:
            candidates = [
                *self._logical_identities.get(function.logical_identity_key, ()),
                *(
                    candidate
                    for candidate in self._fallback_identities.get(fallback_key, ())
                    if not candidate.logical_identity_key
                ),
            ]
        else:
            candidates = list(self._fallback_identities.get(fallback_key, ()))

        selectors: list[VerifyFunction] = []
        seen: set[tuple[str, str]] = set()
        for candidate in (function, *candidates):
            key = (candidate.symbol, candidate.symbol_regex or "")
            if key in seen or (not candidate.symbol and candidate.symbol_regex is None):
                continue
            seen.add(key)
            selectors.append(candidate)
        return tuple(selectors)


def _authorized_linked_selectors(
    function: VerifyFunction,
    catalog: LinkedRetailIdentityCatalog,
) -> tuple[VerifyFunction, ...]:
    """Retain every manifest-authorized spelling for one retail identity.

    Linked interval manifests can use an exact historical spelling while a raw
    translation-unit manifest authorizes the observed VC5 calling-convention
    alternatives with a regex.  Keep the logical identity fixed and carry both
    selectors into linked disposition resolution.
    """

    return _LinkedRetailIdentityLookup(catalog.functions).selectors_for(function)


def _selector_report_rows(
    selectors: tuple[VerifyFunction, ...],
) -> list[dict[str, str]]:
    return [
        (
            {"kind": "exact", "value": selector.symbol}
            if selector.symbol
            else {"kind": "regex", "value": selector.symbol_regex or ""}
        )
        for selector in selectors
    ]


def _symbol_matches_any_selector(
    selectors: tuple[VerifyFunction, ...],
    symbol: str,
) -> bool:
    return any(_function_selector_matches(selector, symbol) for selector in selectors)


class _SelectorLookup:
    def __init__(self, selectors: tuple[VerifyFunction, ...]) -> None:
        self._exact = {selector.symbol for selector in selectors if selector.symbol}
        self._regexes = tuple(
            re.compile(selector.symbol_regex)
            for selector in selectors
            if selector.symbol_regex is not None
        )

    def matches(self, symbol: str) -> bool:
        return symbol in self._exact or any(
            pattern.fullmatch(symbol) for pattern in self._regexes
        )


class _LinkedGroupLookup:
    """Index linked groups by exact MAP symbol while retaining regex support."""

    def __init__(self, groups: dict[int, tuple[LinkedMapSymbol, ...]]) -> None:
        self._groups = groups
        all_symbols: dict[str, set[int]] = {}
        public_symbols: dict[str, set[int]] = {}
        static_symbols: dict[str, set[int]] = {}
        for address, group in groups.items():
            for item in group:
                all_symbols.setdefault(item.symbol, set()).add(address)
                if item.source == "Publics by Value":
                    public_symbols.setdefault(item.symbol, set()).add(address)
                if item.source == "Static symbols":
                    static_symbols.setdefault(item.symbol, set()).add(address)
        self._all_symbols = all_symbols
        self._public_symbols = public_symbols
        self._static_symbols = static_symbols

    def matching(
        self,
        selectors: tuple[VerifyFunction, ...],
        *,
        raw_symbol_witness: str = "",
        public_only: bool = False,
        static_only: bool = False,
    ) -> list[tuple[LinkedMapSymbol, ...]]:
        if public_only and static_only:
            raise ValueError("linked group lookup cannot be both public-only and static-only")
        symbols = (
            self._public_symbols
            if public_only
            else self._static_symbols
            if static_only
            else self._all_symbols
        )
        if raw_symbol_witness:
            addresses = set(symbols.get(raw_symbol_witness, ()))
        else:
            addresses: set[int] = set()
            regexes: list[re.Pattern[str]] = []
            for selector in selectors:
                if selector.symbol:
                    addresses.update(symbols.get(selector.symbol, ()))
                elif selector.symbol_regex is not None:
                    regexes.append(re.compile(selector.symbol_regex))
            if regexes:
                for symbol, symbol_addresses in symbols.items():
                    if any(pattern.fullmatch(symbol) for pattern in regexes):
                        addresses.update(symbol_addresses)
        return [self._groups[address] for address in sorted(addresses)]


def _raw_selector_resolution(
    selectors: tuple[VerifyFunction, ...],
    raw_definitions: tuple[RawObjectFunctionDefinition, ...] | None,
) -> tuple[tuple[RawObjectFunctionDefinition, ...], str, str]:
    if raw_definitions is None:
        return (), "", "unavailable-no-raw-inventory"
    matches = tuple(
        definition
        for definition in raw_definitions
        if any(_symbol_matches_any_selector(selectors, symbol) for symbol in definition.symbols)
    )
    matched_symbols = {
        symbol
        for definition in matches
        for symbol in definition.symbols
        if _symbol_matches_any_selector(selectors, symbol)
    }
    if len(matches) == 1 and len(matched_symbols) == 1:
        return matches, next(iter(matched_symbols)), "unique"
    if not matches:
        return matches, "", "absent"
    return matches, "", "ambiguous"


def _matching_linked_groups_for_identity(
    selectors: tuple[VerifyFunction, ...],
    groups: dict[int, tuple[LinkedMapSymbol, ...]],
    *,
    raw_symbol_witness: str = "",
    public_only: bool = False,
) -> list[tuple[LinkedMapSymbol, ...]]:
    return _LinkedGroupLookup(groups).matching(
        selectors,
        raw_symbol_witness=raw_symbol_witness,
        public_only=public_only,
    )


def _target_identity_functions(target: VerifyTarget) -> tuple[VerifyFunction, ...]:
    rows: list[VerifyFunction] = list(target.functions)
    for entry in target.translation_unit_function_order:
        rows.extend(entry.functions)
    for interval in target.linked_function_intervals:
        if interval.predecessor is not None:
            rows.append(interval.predecessor)
        rows.extend(interval.functions)
        rows.append(interval.successor)
    return tuple(rows)


def _identity_key(function: VerifyFunction) -> str:
    selector = function.symbol or function.symbol_regex or function.listing_label_regex or function.name
    return function.logical_identity_key or f"{function.address}:{selector}"


def _deduplicate_identity_functions(functions: tuple[VerifyFunction, ...]) -> tuple[VerifyFunction, ...]:
    by_key: dict[tuple[str, str, str, str], VerifyFunction] = {}
    for function in functions:
        key = (
            _identity_key(function),
            function.symbol,
            function.symbol_regex or "",
            function.authored_order_role,
        )
        previous = by_key.get(key)
        if previous is None:
            by_key[key] = function
            continue
        # Prefer an exact decorated selector over a regex copy of the same
        # retail identity.  Tracker-derived role/classification remains equal.
        if function.symbol and not previous.symbol:
            by_key[key] = function
    return tuple(
        sorted(
            by_key.values(),
            key=lambda item: (int(item.address, 16), _identity_key(item), item.symbol, item.symbol_regex or ""),
        )
    )


def build_linked_retail_identity_catalog(
    *,
    target: VerifyTarget,
    manifests: tuple[VerifyTarget, ...] = (),
    progress_path: Path | None = DEFAULT_PROGRESS,
) -> LinkedRetailIdentityCatalog:
    """Join decorated manifest selectors to authoritative tracker roles.

    Linked MAP rows do not carry retail addresses.  The authored projection
    therefore needs a global identity catalog: manifests supply decorated
    selectors while the unified tracker supplies the current per-retail-row
    pipeline class, authored role, and logical ICF alias classification.
    """

    all_targets = (*manifests, target)
    manifest_rows_by_address: dict[str, list[VerifyFunction]] = {}
    for manifest_target in all_targets:
        if manifest_target.target_binary != target.target_binary:
            continue
        for function in _target_identity_functions(manifest_target):
            manifest_rows_by_address.setdefault(function.address, []).append(function)

    if progress_path is None:
        local = tuple(
            function
            for function in _target_identity_functions(target)
            if function.symbol or function.symbol_regex is not None
        )
        return LinkedRetailIdentityCatalog(_deduplicate_identity_functions(local))

    document = ProgressDocument.load(progress_path)
    if isinstance(getattr(document, "data", None), Mapping):
        require_valid_authored_icf_groups(document.data)
    pipeline = document.pipeline(target.target_binary)
    catalog_rows: list[VerifyFunction] = []
    tracker_addresses: set[str] = set()
    for symbol in document.collection("symbols").values():
        if not isinstance(symbol, dict):
            continue
        if symbol.get("binary") != target.target_binary:
            continue
        if symbol.get("kind") not in {"function", "provider-function", "compiler-function"}:
            continue
        address = str(symbol.get("address", ""))
        if not address:
            continue
        tracker_addresses.add(address)
        aliases = symbol_logical_aliases(symbol)
        physical_gate_alias_group = bool(aliases) and all(
            alias.get("gate_mode") == "physical-body-only"
            for _identity_key, alias in aliases
        )
        if aliases and not physical_gate_alias_group:
            for identity_key, alias in aliases:
                object_symbol = str(alias.get("object_symbol", ""))
                if not object_symbol:
                    continue
                alias_function = VerifyFunction(
                    address=address,
                    symbol=object_symbol,
                    name=str(alias.get("original_name", "")) or str(symbol.get("navigation_name", "")),
                    pipeline_class=str(alias.get("pipeline_class", "unresolved")),
                    authored_order_role=logical_alias_authored_order_role(alias),
                    required_presence=True,
                    full_order_gate=True,
                    logical_identity_key=str(alias.get("identity_key", "")) or identity_key,
                    icf_fold_status=str(alias.get("fold_status", "")),
                )
                catalog_rows.append(alias_function)
                for manifest_function in manifest_rows_by_address.get(address, []):
                    if not _same_linked_retail_identity(alias_function, manifest_function):
                        continue
                    catalog_rows.append(
                        replace(
                            manifest_function,
                            pipeline_class=alias_function.pipeline_class,
                            authored_order_role=alias_function.authored_order_role,
                            logical_identity_key=alias_function.logical_identity_key,
                            icf_fold_status=alias_function.icf_fold_status,
                        )
                    )
            continue

        for manifest_function in manifest_rows_by_address.get(address, []):
            if not manifest_function.symbol and manifest_function.symbol_regex is None:
                continue
            catalog_rows.append(
                replace(
                    manifest_function,
                    pipeline_class=str(symbol.get("pipeline_class", "unresolved")),
                    authored_order_role=symbol_authored_order_role(symbol),
                )
            )

    # A transient/external target can contain a newly reviewed identity not yet
    # registered in the tracker.  Retain it only when no tracker row exists at
    # that retail address; a present tracker row always owns classification.
    for function in _target_identity_functions(target):
        if function.address not in tracker_addresses and (function.symbol or function.symbol_regex is not None):
            catalog_rows.append(function)

    return LinkedRetailIdentityCatalog(
        functions=_deduplicate_identity_functions(tuple(catalog_rows)),
        source="unified-tracker-plus-vc5-manifests",
        tracker_revision=document.revision,
        authored_order_prefix_end=str(pipeline.get("authored_order_prefix_end", "")),
    )


def _matching_retail_identities(
    alias: str,
    functions: tuple[VerifyFunction, ...],
) -> tuple[VerifyFunction, ...]:
    return _LinkedRetailIdentityLookup(functions).matching_alias(alias)


def _identity_report_row(function: VerifyFunction) -> dict[str, object]:
    return {
        "identity_key": _identity_key(function),
        "retail_address": function.address,
        "name": function.name,
        "pipeline_class": function.pipeline_class,
        "authored_order_role": authored_order_role(function),
        "authored_order_gate": function_authored_order_gate(function),
        "authored_relative_order_gate": function_authored_relative_order_gate(function),
        "logical_identity_key": function.logical_identity_key,
        "icf_fold_status": function.icf_fold_status,
    }


def _candidate_extra_classification(interval: LinkedFunctionInterval, alias: str):
    for extra in interval.candidate_only_extras:
        if extra.symbol and extra.symbol == alias:
            return extra
        if extra.symbol_regex is not None:
            pattern = re.compile(extra.symbol_regex)
            if pattern.fullmatch(alias):
                return extra
    return None


class _CandidateExtraLookup:
    def __init__(self, interval: LinkedFunctionInterval) -> None:
        self._exact = {
            extra.symbol: extra
            for extra in interval.candidate_only_extras
            if extra.symbol
        }
        self._regexes = tuple(
            (re.compile(extra.symbol_regex), extra)
            for extra in interval.candidate_only_extras
            if extra.symbol_regex is not None
        )

    def matching(self, alias: str):
        exact = self._exact.get(alias)
        if exact is not None:
            return exact
        return next(
            (
                extra
                for pattern, extra in self._regexes
                if pattern.fullmatch(alias)
            ),
            None,
        )


def _check_linked_function_interval_authored(
    *,
    target: VerifyTarget,
    interval: LinkedFunctionInterval,
    parsed_map: ParsedLinkMap,
    identity_catalog: LinkedRetailIdentityCatalog | None = None,
    raw_definitions: tuple[RawObjectFunctionDefinition, ...] | None = None,
    raw_inventory_diagnostics: tuple[str, ...] = (),
) -> LinkedOrderCheck:
    groups = linked_function_groups(parsed_map)
    catalog = identity_catalog or LinkedRetailIdentityCatalog(
        _deduplicate_identity_functions(_target_identity_functions(target))
    )
    identity_lookup = _LinkedRetailIdentityLookup(catalog.functions)
    group_lookup = _LinkedGroupLookup(groups)
    expected_rows = (
        (interval.predecessor, *interval.functions, interval.successor)
        if interval.predecessor is not None
        else (*interval.functions, interval.successor)
    )
    resolved: list[tuple[VerifyFunction, tuple[LinkedMapSymbol, ...]]] = []
    diagnostics: list[str] = []
    presence_diagnostics: list[str] = []
    required_identity_dispositions: list[dict[str, object]] = []
    for item in raw_inventory_diagnostics:
        diagnostic = f"raw definition inventory is incomplete: {item}"
        diagnostics.append(diagnostic)
        presence_diagnostics.append(diagnostic)
    for function in expected_rows:
        authorized_selectors = identity_lookup.selectors_for(function)
        raw_matches, raw_symbol_witness, raw_witness_state = _raw_selector_resolution(
            authorized_selectors,
            raw_definitions,
        )
        public_matches = group_lookup.matching(
            authorized_selectors,
            raw_symbol_witness=raw_symbol_witness,
            public_only=True,
        )
        selected_matches = group_lookup.matching(
            authorized_selectors,
            raw_symbol_witness=raw_symbol_witness,
        )
        static_matches = group_lookup.matching(
            authorized_selectors,
            raw_symbol_witness=raw_symbol_witness,
            static_only=True,
        )
        if len(public_matches) == 1:
            disposition = "selected-public-map"
        elif not public_matches and len(selected_matches) == 1:
            disposition = "selected-static-map"
        elif len(public_matches) > 1 or len(selected_matches) > 1:
            disposition = "multiple-selected-groups-unresolved"
        elif raw_definitions is None:
            disposition = "public-map-absence-unresolved-no-raw-inventory"
        elif raw_matches:
            disposition = "public-map-absent-unresolved"
        else:
            disposition = "raw-and-public-map-absent-unresolved"
        required_identity_dispositions.append(
            {
                "identity": _identity_report_row(function),
                "authorized_selectors": _selector_report_rows(authorized_selectors),
                "required_in_authored_scope": function_required_in_scope(function, "authored"),
                "disposition": disposition,
                "raw_decorated_symbol_witness": raw_symbol_witness or None,
                "raw_decorated_symbol_witness_state": raw_witness_state,
                "raw_definition_count": (
                    len(raw_matches) if raw_definitions is not None else None
                ),
                "raw_definitions": [
                    {
                        "object": definition.object_path,
                        "symbols": list(definition.symbols),
                    }
                    for definition in raw_matches
                ],
                "public_map_count": len(public_matches),
                "static_map_count": len(static_matches),
                "selected_group_count": len(selected_matches),
                "selected_linked_addresses": [
                    f"0x{group[0].address:x}" for group in selected_matches
                ],
                "selected_matching_symbols": sorted({
                    item.symbol
                    for group in selected_matches
                    for item in group
                    if (
                        item.symbol == raw_symbol_witness
                        if raw_symbol_witness
                        else _symbol_matches_any_selector(authorized_selectors, item.symbol)
                    )
                }),
                "selected_proven": len(selected_matches) == 1,
                "folded_proven": False,
                "discarded_proven": False,
            }
        )
        if len(selected_matches) != 1:
            if function_required_in_scope(function, "authored"):
                if not selected_matches and raw_definitions is not None and raw_matches:
                    diagnostic = (
                        f"{function.address} {function.name}: public-map-absent required identity "
                        "remains unresolved "
                        f"(raw_definition_count={len(raw_matches)}, public_map_count=0; "
                        "selected/folded/discarded disposition is unproven)"
                    )
                elif not selected_matches and raw_definitions is not None:
                    diagnostic = (
                        f"{function.address} {function.name}: required identity is absent from raw COFF "
                        "and the public map; disposition remains unresolved "
                        "(raw_definition_count=0, public_map_count=0)"
                    )
                else:
                    diagnostic = (
                        f"{function.address} {function.name}: selected disposition is ambiguous; "
                        "matches at "
                        + ", ".join(
                            f"0x{group[0].address:x}" for group in selected_matches[:8]
                        )
                    )
                diagnostics.append(diagnostic)
                presence_diagnostics.append(diagnostic)
            continue
        group = selected_matches[0]
        resolved.append((function, group))
        if function_required_in_scope(function, "authored") and function.pipeline_class == "unresolved":
            diagnostic = (
                f"{function.address} {function.name}: required authored-scope row has unresolved pipeline_class"
            )
            diagnostics.append(diagnostic)
            presence_diagnostics.append(diagnostic)

    required_presence_passed = not presence_diagnostics and all(
        any(item[0] is function for item in resolved)
        for function in expected_rows
        if function_required_in_scope(function, "authored")
    )
    current_resolved = [
        item for item in resolved if item[0] in interval.functions
    ]
    successor_item = next((item for item in resolved if item[0] is interval.successor), None)
    predecessor_item = next((item for item in resolved if item[0] is interval.predecessor), None)

    authored_rows = [
        item
        for item in (*current_resolved, *((successor_item,) if successor_item is not None else ()))
        if function_authored_relative_order_gate(item[0])
    ]
    authored_addresses = [group[0].address for _function, group in authored_rows]
    local_authored_relative_order_passed = authored_addresses == sorted(authored_addresses)
    if not local_authored_relative_order_passed:
        diagnostics.append("authored/authored-lifecycle rows resolve to a reordered selected linked sequence")

    current_addresses = sorted({
        group[0].address
        for function, group in current_resolved
        if function_authored_order_gate(function)
    })
    successor_address = successor_item[1][0].address if successor_item is not None else None
    block_precedence_passed = bool(current_addresses and successor_address is not None)
    if block_precedence_passed:
        block_precedence_passed = all(address < successor_address for address in current_addresses)
    if interval.predecessor is not None:
        block_precedence_passed &= predecessor_item is not None
    if predecessor_item is not None and current_addresses:
        block_precedence_passed &= predecessor_item[1][0].address < min(current_addresses)
    boundary = interval.predecessor_section_boundary
    if boundary is not None and current_addresses:
        matching_sections = [section for section in parsed_map.sections if section.name == boundary.section]
        if len(matching_sections) != 1:
            block_precedence_passed = False
        else:
            section = matching_sections[0]
            section_symbols = [
                symbol.address
                for symbol in parsed_map.symbols
                if symbol.segment == section.segment and symbol.offset == section.offset
            ]
            actual_start = min(section_symbols) if section_symbols else parsed_map.preferred_load_address + 0x1000 + section.offset
            current_segments = {
                group[0].segment
                for function, group in current_resolved
                if function_authored_order_gate(function)
            }
            block_precedence_passed &= current_segments == {section.segment}
            block_precedence_passed &= min(current_addresses) >= actual_start
    if not block_precedence_passed:
        diagnostics.append("current block does not retain its required boundary/before-successor precedence")

    retail_starts = [int(function.address, 16) for function in interval.functions]
    retail_start = int(interval.retail_start, 16) if interval.retail_start else min(retail_starts)
    retail_end = (
        int(interval.retail_end_exclusive, 16)
        if interval.retail_end_exclusive
        else int(interval.successor.address, 16)
    )
    accepted_prefix_end = retail_start
    if catalog.authored_order_prefix_end:
        accepted_prefix_end = min(
            retail_start,
            int(catalog.authored_order_prefix_end, 16),
        )
    boundary_identity_keys = {_identity_key(function) for function in expected_rows}

    def identity_is_in_blocking_order_scope(function: VerifyFunction) -> bool:
        retail_address = int(function.address, 16)
        return (
            retail_address < accepted_prefix_end
            or retail_start <= retail_address < retail_end
            or _identity_key(function) in boundary_identity_keys
        )

    alias_classifications: list[dict[str, object]] = []
    physical_classes: list[dict[str, object]] = []
    classified_extras: list[dict[str, object]] = []
    unclassified_extras: list[dict[str, object]] = []
    phase_resolved_exact_ambiguities: list[dict[str, object]] = []
    selected_by_identity: dict[str, set[int]] = {}
    identity_by_key: dict[str, VerifyFunction] = {}
    physical_retail_nodes: dict[int, set[int]] = {}
    cross_retail_blocking_addresses: set[int] = set()
    scope_projection_complete = True
    scope_unresolved_messages: set[str] = set()
    for function in catalog.functions:
        address = int(function.address, 16)
        if retail_start <= address < retail_end and (
            function.pipeline_class == "unresolved"
            or authored_order_role(function) == "unresolved"
        ):
            scope_projection_complete = False
            scope_unresolved_messages.add(
                f"retail identity {function.address} {function.name} remains unresolved in the authored interval"
            )

    required_expected_rows = tuple(
        function
        for function in expected_rows
        if function_required_in_scope(function, "authored")
    )
    required_selector_lookup = _SelectorLookup(tuple(
        selector
        for function in required_expected_rows
        for selector in identity_lookup.selectors_for(function)
    ))
    candidate_extra_lookup = _CandidateExtraLookup(interval)
    for address, group in sorted(groups.items()):
        class_identities: dict[str, VerifyFunction] = {}
        class_alias_classifications: list[dict[str, object]] = []
        class_aliases = sorted({item.symbol for item in group})
        object_qualified_aliases = sorted(
            {
                f"{item.object}!{item.symbol}" if item.object else item.symbol
                for item in group
            }
        )
        for alias in class_aliases:
            alias_items = tuple(item for item in group if item.symbol == alias)
            object_local = bool(alias_items) and all(
                item.source == "Static symbols" for item in alias_items
            )
            required_local_selector = required_selector_lookup.matches(alias)
            classification = candidate_extra_lookup.matching(alias)
            exclude_unrequired_object_local = (
                object_local
                and not required_local_selector
                and classification is None
            )
            identities = (
                ()
                if exclude_unrequired_object_local
                else identity_lookup.matching_alias(alias)
            )
            identity_addresses = {function.address for function in identities}
            identity_roles = {
                (function.pipeline_class, authored_order_role(function))
                for function in identities
            }
            uniform_non_authored_retail_ambiguity = bool(
                identities
                and len(identity_addresses) > 1
                and identity_roles == {("non-authored", "non-authored")}
                and not any(function_authored_order_gate(function) for function in identities)
            )
            if exclude_unrequired_object_local:
                state = "object-local-unrequired"
            elif uniform_non_authored_retail_ambiguity:
                state = "ambiguous-non-authored-retail-identities"
            elif identities and (len(identity_addresses) > 1 or len(identity_roles) > 1):
                state = "ambiguous-retail-identity"
            elif identities and any(
                function.pipeline_class == "unresolved"
                or authored_order_role(function) == "unresolved"
                for function in identities
            ):
                state = "unresolved-retail-identity"
            elif identities:
                state = "mapped-retail-identity"
            elif classification is not None and classification.pipeline_class == "non-authored":
                state = "candidate-only-non-authored"
            elif classification is not None:
                state = "candidate-only-unresolved"
            else:
                state = "unmapped-selected-alias"

            in_scope = any(
                retail_start <= int(function.address, 16) < retail_end
                for function in identities
            )
            alias_classification: dict[str, object] = {
                "linked_address": f"0x{address:x}",
                "alias": alias,
                "object_qualified_aliases": [
                    f"{item.object}!{item.symbol}" if item.object else item.symbol
                    for item in alias_items
                ],
                "map_symbol_scope": (
                    "object-local" if object_local else "public-or-global"
                ),
                "state": state,
                "retail_identities": [_identity_report_row(function) for function in identities],
                "retail_interval_member": in_scope,
                "candidate_only_classification": (
                    {
                        "name": classification.name,
                        "pipeline_class": classification.pipeline_class,
                    }
                    if classification is not None
                    else None
                ),
            }
            if uniform_non_authored_retail_ambiguity:
                alias_classification.update(
                    {
                        "authored_scope_disposition": "phase-resolved-nonblocking",
                        "exact_retail_placement_state": (
                            "ambiguous-deferred-to-full-function-order"
                        ),
                    }
                )
                phase_resolved_exact_ambiguities.append(alias_classification)
            class_alias_classifications.append(alias_classification)
            if state == "candidate-only-non-authored":
                classified_extras.append(
                    {
                        "linked_address": f"0x{address:x}",
                        "aliases": [alias],
                        "pipeline_class": "non-authored",
                        "classification_name": classification.name if classification is not None else "",
                    }
                )
            elif state not in {
                "mapped-retail-identity",
                "ambiguous-non-authored-retail-identities",
                "object-local-unrequired",
            }:
                unclassified_extras.append(
                    {
                        "linked_address": f"0x{address:x}",
                        "aliases": [alias],
                        "pipeline_class": "unresolved",
                        "classification_name": classification.name if classification is not None else "",
                        "classification_state": state,
                        "retail_interval_member": in_scope,
                    }
                )
                if in_scope:
                    scope_projection_complete = False
                    scope_unresolved_messages.add(
                        f"selected alias mapped to the authored retail interval is unresolved: {alias} at 0x{address:x}"
                    )

            for function in identities:
                key = _identity_key(function)
                identity_by_key[key] = function
                class_identities[key] = function
                selected_by_identity.setdefault(key, set()).add(address)

        retail_nodes = sorted({
            int(function.address, 16) for function in class_identities.values()
        })
        physical_retail_nodes[address] = set(retail_nodes)
        cross_retail = len(retail_nodes) > 1
        class_in_scope = any(retail_start <= node < retail_end for node in retail_nodes)
        class_gating_in_blocking_scope = any(
            function_authored_order_gate(function)
            and identity_is_in_blocking_order_scope(function)
            for function in class_identities.values()
        )
        class_state = (
            "non-scalar-cross-retail-overfolded"
            if cross_retail
            else "scalar-retail-node"
            if len(retail_nodes) == 1
            else "unmapped-physical-class"
        )
        if cross_retail and class_gating_in_blocking_scope:
            scope_projection_complete = False
            scope_unresolved_messages.add(
                "linked physical class "
                f"0x{address:x} spans authored retail nodes "
                + ", ".join(f"0x{node:x}" for node in retail_nodes)
                + "; non-scalar cross-retail overfold disposition is unresolved"
            )
        physical_class = {
            "physical_class_id": f"linked:0x{address:x}",
            "linked_address": f"0x{address:x}",
            "aliases": class_aliases,
            "object_qualified_aliases": object_qualified_aliases,
            "logical_members": [
                _identity_report_row(function)
                for function in sorted(
                    class_identities.values(),
                    key=lambda item: (int(item.address, 16), _identity_key(item)),
                )
            ],
            "retail_nodes": [f"0x{node:x}" for node in retail_nodes],
            "state": class_state,
            "scalar_retail_ordinal_valid": len(retail_nodes) == 1,
            "representative_retail_ordinal": (
                f"0x{retail_nodes[0]:x}" if len(retail_nodes) == 1 else None
            ),
            "retail_interval_member": class_in_scope,
            "authored_order_blocking_scope_member": class_gating_in_blocking_scope,
        }
        physical_classes.append(physical_class)
        for alias_classification in class_alias_classifications:
            alias_classification.update(
                {
                    "physical_class_id": physical_class["physical_class_id"],
                    "physical_class_state": class_state,
                    "physical_class_retail_nodes": physical_class["retail_nodes"],
                    "scalar_retail_ordinal_valid": physical_class["scalar_retail_ordinal_valid"],
                    "representative_retail_ordinal": physical_class["representative_retail_ordinal"],
                }
            )
        alias_classifications.extend(class_alias_classifications)

    diagnostics.extend(sorted(scope_unresolved_messages))
    cross_retail_classes = [
        item
        for item in physical_classes
        if item["state"] == "non-scalar-cross-retail-overfolded"
    ]
    projection_complete = (
        not unclassified_extras
        and not phase_resolved_exact_ambiguities
        and not cross_retail_classes
    )
    nonblocking_diagnostics: list[str] = []
    if not projection_complete:
        exact_projection_debt_count = (
            len(unclassified_extras)
            + len(phase_resolved_exact_ambiguities)
            + len(cross_retail_classes)
        )
        nonblocking_diagnostics.append(
            "global linked projection is incomplete: "
            f"{exact_projection_debt_count} selected aliases are unmapped, ambiguous, or unresolved"
        )
    if phase_resolved_exact_ambiguities:
        nonblocking_diagnostics.append(
            "authored scope phase-resolved "
            f"{len(phase_resolved_exact_ambiguities)} uniformly non-authored alias ambiguity; "
            "exact retail placement remains deferred to full-function-order"
        )
    if not scope_projection_complete:
        diagnostics.append("authored retail-identity projection is incomplete for the interval")

    duplicate_identity_keys = [
        key for key, addresses in selected_by_identity.items() if len(addresses) != 1
    ]
    blocking_duplicate_identity_keys: list[str] = []
    future_duplicate_identity_keys: list[str] = []
    for key in duplicate_identity_keys:
        function = identity_by_key[key]
        if not function_authored_order_gate(function):
            continue
        if identity_is_in_blocking_order_scope(function):
            blocking_duplicate_identity_keys.append(key)
            diagnostics.append(
                f"authored retail identity {function.address} {function.name} selects multiple linked groups: "
                + ", ".join(f"0x{address:x}" for address in sorted(selected_by_identity[key]))
            )
        else:
            future_duplicate_identity_keys.append(key)
    for key in future_duplicate_identity_keys[:25]:
        function = identity_by_key[key]
        nonblocking_diagnostics.append(
            "future-only authored duplicate diagnostic: "
            f"retail {function.address} {function.name} selects linked groups "
            + ", ".join(f"0x{address:x}" for address in sorted(selected_by_identity[key]))
        )
    if len(future_duplicate_identity_keys) > 25:
        nonblocking_diagnostics.append(
            "future-only authored duplicate diagnostic report truncated: "
            f"{len(future_duplicate_identity_keys) - 25} additional identity(s)"
        )

    physical_identity_keys: dict[int, set[str]] = {}
    for key, addresses in selected_by_identity.items():
        for address in addresses:
            physical_identity_keys.setdefault(address, set()).add(key)

    scalar_order_classes: list[tuple[int, int, bool]] = []
    future_cross_retail_classes: list[tuple[int, tuple[int, ...]]] = []
    for address, keys in sorted(physical_identity_keys.items()):
        relevant_functions = [
            identity_by_key[key]
            for key in keys
            if function_authored_relative_order_gate(identity_by_key[key])
        ]
        if not relevant_functions:
            continue
        blocks_current_scope = any(
            identity_is_in_blocking_order_scope(function)
            for function in relevant_functions
        )
        retail_nodes = physical_retail_nodes.get(address, set())
        if len(retail_nodes) != 1:
            if blocks_current_scope:
                cross_retail_blocking_addresses.add(address)
                message = (
                    "linked physical class "
                    f"0x{address:x} spans authored retail nodes "
                    + ", ".join(f"0x{node:x}" for node in sorted(retail_nodes))
                    + "; non-scalar cross-retail overfold disposition is unresolved"
                )
                if message not in diagnostics:
                    diagnostics.append(message)
            else:
                future_cross_retail_classes.append(
                    (address, tuple(sorted(retail_nodes)))
                )
            continue
        scalar_order_classes.append(
            (address, next(iter(retail_nodes)), blocks_current_scope)
        )

    for address, retail_nodes in future_cross_retail_classes[:25]:
        nonblocking_diagnostics.append(
            "future-only non-scalar cross-retail overfold diagnostic: "
            f"linked 0x{address:x} spans retail nodes "
            + ", ".join(f"0x{node:x}" for node in retail_nodes)
        )
    if len(future_cross_retail_classes) > 25:
        nonblocking_diagnostics.append(
            "future-only non-scalar cross-retail overfold diagnostic report truncated: "
            f"{len(future_cross_retail_classes) - 25} additional physical class(es)"
        )

    inversion_summary = summarize_authored_inversions(scalar_order_classes)
    blocking_inversion_pairs = inversion_summary.blocking_examples
    future_inversion_pairs = inversion_summary.future_examples
    blocking_inversion_pair_count = inversion_summary.blocking_count
    future_inversion_pair_count = inversion_summary.future_count

    for left, right in blocking_inversion_pairs:
        diagnostics.append(
            "known authored linked inversion class: "
            f"linked 0x{left[0]:x}/retail 0x{left[1]:x} precedes "
            f"linked 0x{right[0]:x}/retail 0x{right[1]:x}"
        )
    if blocking_inversion_pair_count > len(blocking_inversion_pairs):
        diagnostics.append(
            "known authored linked inversion class report truncated: "
            f"{blocking_inversion_pair_count - len(blocking_inversion_pairs)} additional inversion pair(s)"
        )
    for left, right in future_inversion_pairs:
        nonblocking_diagnostics.append(
            "future-only known authored linked inversion diagnostic: "
            f"linked 0x{left[0]:x}/retail 0x{left[1]:x} precedes "
            f"linked 0x{right[0]:x}/retail 0x{right[1]:x}"
        )
    if future_inversion_pair_count > len(future_inversion_pairs):
        nonblocking_diagnostics.append(
            "future-only known authored linked inversion diagnostic report truncated: "
            f"{future_inversion_pair_count - len(future_inversion_pairs)} additional inversion pair(s)"
        )

    linked_known_authored_relative_order_passed = (
        local_authored_relative_order_passed
        and not blocking_duplicate_identity_keys
        and not cross_retail_blocking_addresses
        and blocking_inversion_pair_count == 0
    )

    selected_addresses = sorted({
        group[0].address
        for _function, group in (*current_resolved, *((successor_item,) if successor_item else ()))
    })
    expected_by_address = {
        group[0].address: (function, group)
        for function, group in (*current_resolved, *((successor_item,) if successor_item else ()))
    }
    contributions: list[LinkedOrderContribution] = []
    for address in selected_addresses:
        group = groups[address]
        expected = expected_by_address.get(address)
        contributions.append(
            LinkedOrderContribution(
                segment=group[0].segment,
                offset=group[0].offset,
                linked_address=address,
                linked_rva=address - parsed_map.preferred_load_address,
                symbols=tuple(sorted({item.symbol for item in group})),
                providers=tuple(sorted({item.object for item in group if item.object})),
                map_sources=tuple(sorted({item.source for item in group})),
                manifest_address=expected[0].address if expected is not None else "",
                manifest_name=expected[0].name if expected is not None else "",
                disposition=(
                    "selected-required-retail-identity"
                    if expected is not None
                    else "selected-known-authored-projection-row"
                ),
            )
        )

    boundary_offenders: list[dict[str, object]] = []
    boundary_proof_diagnostics: list[str] = []
    if successor_item is None:
        boundary_proof_diagnostics.append(
            "successor identity did not resolve exactly once"
        )
    if interval.predecessor is not None and predecessor_item is None:
        boundary_proof_diagnostics.append(
            "predecessor identity did not resolve exactly once"
        )
    if not current_resolved:
        boundary_proof_diagnostics.append("no current-interval identities resolved")
    if not scope_projection_complete:
        boundary_proof_diagnostics.append(
            "current authored interval contains unresolved or unclassified selected identities"
        )

    predecessor_address = predecessor_item[1][0].address if predecessor_item is not None else None
    for function, group in current_resolved:
        if not function_authored_order_gate(function):
            continue
        linked_address = group[0].address
        crossing_reasons: list[str] = []
        if successor_address is not None and linked_address >= successor_address:
            crossing_reasons.append("at-or-after-successor")
        if predecessor_address is not None and linked_address <= predecessor_address:
            crossing_reasons.append("at-or-before-predecessor")
        if not crossing_reasons:
            continue
        fold_dependent = bool(
            function.logical_identity_key
            and function.icf_fold_status in {"proven-fold-alias", "selected-winner"}
            and not function.full_order_gate
            and function.pipeline_class in AUTHORED_PIPELINE_CLASSES
            and authored_order_role(function) in {
                "authored-body",
                "authored-lifecycle-body",
            }
        )
        if function.pipeline_class == "unresolved" or authored_order_role(function) == "unresolved":
            classification = "unresolved"
        elif function.full_order_gate:
            classification = "full-order-gating-unique"
        elif fold_dependent:
            classification = "declared-icf-fold-dependent"
        else:
            classification = "non-fold-dependent-unique"
        boundary_offenders.append(
            {
                "identity": _identity_report_row(function),
                "linked_address": f"0x{linked_address:x}",
                "selected_objects": sorted({item.object for item in group if item.object}),
                "crossing_reasons": crossing_reasons,
                "classification": classification,
                "declared_icf_fold_dependent": fold_dependent,
            }
        )

    for extra in unclassified_extras:
        if not extra.get("retail_interval_member"):
            continue
        linked_address_text = extra.get("linked_address")
        if not isinstance(linked_address_text, str):
            boundary_proof_diagnostics.append(
                "current-scope unclassified selected identity lacks a linked address"
            )
            continue
        linked_address = int(linked_address_text, 16)
        crossing_reasons: list[str] = []
        if successor_address is not None and linked_address >= successor_address:
            crossing_reasons.append("at-or-after-successor")
        if predecessor_address is not None and linked_address <= predecessor_address:
            crossing_reasons.append("at-or-before-predecessor")
        if crossing_reasons:
            boundary_offenders.append(
                {
                    "identity": None,
                    "linked_address": linked_address_text,
                    "selected_objects": [],
                    "aliases": list(extra.get("aliases", [])),
                    "crossing_reasons": crossing_reasons,
                    "classification": str(
                        extra.get("classification_state", "unclassified")
                    ),
                    "declared_icf_fold_dependent": False,
                }
            )

    if boundary is not None and current_addresses:
        matching_sections = [section for section in parsed_map.sections if section.name == boundary.section]
        if len(matching_sections) != 1:
            boundary_proof_diagnostics.append(
                "predecessor section boundary did not resolve exactly once"
            )
        else:
            section = matching_sections[0]
            section_symbols = [
                symbol.address
                for symbol in parsed_map.symbols
                if symbol.segment == section.segment and symbol.offset == section.offset
            ]
            actual_start = (
                min(section_symbols)
                if section_symbols
                else parsed_map.preferred_load_address + 0x1000 + section.offset
            )
            current_segments = {
                group[0].segment
                for function, group in current_resolved
                if function_authored_order_gate(function)
            }
            if current_segments != {section.segment} or min(current_addresses) < actual_start:
                boundary_proof_diagnostics.append(
                    "predecessor section boundary geometry is not established"
                )
    return LinkedOrderCheck(
        target_name=target.name,
        interval_name=interval.name,
        contributions=tuple(contributions),
        diagnostics=tuple(diagnostics),
        order_scope="authored",
        required_presence_passed=required_presence_passed,
        authored_relative_order_passed=linked_known_authored_relative_order_passed,
        block_precedence_passed=block_precedence_passed,
        exact_selected_sequence_matches_manifest=False,
        classified_selected_extras=tuple(classified_extras),
        unclassified_selected_extras=tuple(unclassified_extras),
        raw_authored_order_evaluated=False,
        raw_authored_order_passed=None,
        linked_known_authored_relative_order_passed=linked_known_authored_relative_order_passed,
        linked_known_authored_relative_order_scope=(
            "accepted-prefix-retail-interval-and-boundaries"
        ),
        linked_required_presence_passed=required_presence_passed,
        linked_projection_complete=projection_complete,
        linked_scope_projection_complete=scope_projection_complete,
        linked_exact_selected_population_evaluated=False,
        linked_exact_selected_population_passed=None,
        linked_seams_and_rvas_evaluated=False,
        linked_seams_and_rvas_passed=None,
        alias_classifications=tuple(alias_classifications),
        physical_classes=tuple(physical_classes),
        required_identity_dispositions=tuple(required_identity_dispositions),
        raw_definition_inventory_complete=(
            None if raw_definitions is None else not raw_inventory_diagnostics
        ),
        raw_definition_inventory_diagnostics=raw_inventory_diagnostics,
        nonblocking_diagnostics=tuple(nonblocking_diagnostics),
        identity_catalog_source=catalog.source,
        tracker_revision=catalog.tracker_revision,
        boundary_offender_proof_complete=not boundary_proof_diagnostics,
        boundary_offender_proof_diagnostics=tuple(boundary_proof_diagnostics),
        boundary_offenders=tuple(boundary_offenders),
    )


def check_linked_function_interval(
    *,
    target: VerifyTarget,
    interval: LinkedFunctionInterval,
    parsed_map: ParsedLinkMap,
    order_scope: str | None = None,
    identity_catalog: LinkedRetailIdentityCatalog | None = None,
    raw_definitions: tuple[RawObjectFunctionDefinition, ...] | None = None,
    raw_inventory_diagnostics: tuple[str, ...] = (),
) -> LinkedOrderCheck:
    effective_scope = order_scope or interval.order_scope
    if effective_scope == "authored":
        return _check_linked_function_interval_authored(
            target=target,
            interval=interval,
            parsed_map=parsed_map,
            identity_catalog=identity_catalog,
            raw_definitions=raw_definitions,
            raw_inventory_diagnostics=raw_inventory_diagnostics,
        )
    if effective_scope != "full":
        raise ValueError(f"invalid linked order scope {effective_scope!r}")
    groups = linked_function_groups(parsed_map)
    expected_rows = (
        (interval.predecessor, *interval.functions, interval.successor)
        if interval.predecessor is not None
        else (*interval.functions, interval.successor)
    )
    resolved: list[tuple[VerifyFunction, tuple[LinkedMapSymbol, ...]]] = []
    diagnostics: list[str] = []
    presence_diagnostics: list[str] = []
    for function in expected_rows:
        try:
            resolved.append((function, resolve_linked_function_group(function, groups)))
        except ValueError as exc:
            diagnostic = str(exc)
            diagnostics.append(diagnostic)
            presence_diagnostics.append(diagnostic)

    if len(resolved) != len(expected_rows):
        return LinkedOrderCheck(
            target_name=target.name,
            interval_name=interval.name,
            contributions=(),
            diagnostics=tuple(diagnostics),
            order_scope="full",
            required_presence_passed=False,
        )

    expected_addresses = [group[0].address for _function, group in resolved]
    if len(set(expected_addresses)) != len(expected_addresses):
        diagnostics.append("two or more manifest rows resolve to the same selected function alias group")
    if expected_addresses != sorted(expected_addresses):
        diagnostics.append("manifest rows resolve to a reordered selected linked function sequence")

    boundary = interval.predecessor_section_boundary
    predecessor_address = expected_addresses[0]
    successor_address = expected_addresses[-1]
    if boundary is not None:
        boundary_address = int(boundary.address, 16)
        matching_sections = [section for section in parsed_map.sections if section.name == boundary.section]
        if len(matching_sections) != 1:
            diagnostics.append(
                f"section boundary {boundary.section} resolves to {len(matching_sections)} map section rows"
            )
        else:
            section = matching_sections[0]
            section_symbols = [
                symbol.address
                for symbol in parsed_map.symbols
                if symbol.segment == section.segment and symbol.offset == section.offset
            ]
            actual_start = min(section_symbols) if section_symbols else parsed_map.preferred_load_address + 0x1000 + section.offset
            if actual_start != boundary_address:
                diagnostics.append(
                    f"section {boundary.section} starts at 0x{actual_start:x}, not {boundary.address}"
                )
            if predecessor_address != boundary_address:
                diagnostics.append(
                    f"first interval function resolves at 0x{predecessor_address:x}, not section boundary {boundary.address}"
                )
    if predecessor_address >= successor_address:
        diagnostics.append(
            f"interval start resolves at 0x{predecessor_address:x}, not before successor 0x{successor_address:x}"
        )
        selected_addresses: list[int] = []
    else:
        predecessor_segment = resolved[0][1][0].segment
        successor_segment = resolved[-1][1][0].segment
        if predecessor_segment != successor_segment:
            diagnostics.append(
                f"sentinels resolve in different segments: {predecessor_segment:04x} and {successor_segment:04x}"
            )
        selected_addresses = sorted(
            address
            for address, group in groups.items()
            if predecessor_address <= address <= successor_address
            and group[0].segment == predecessor_segment
        )

    if selected_addresses != expected_addresses:
        expected_set = set(expected_addresses)
        selected_set = set(selected_addresses)
        for address in selected_addresses:
            if address not in expected_set:
                aliases = " | ".join(item.symbol for item in groups[address])
                diagnostics.append(f"unlisted selected function at 0x{address:x}: {aliases}")
        for function, group in resolved:
            if group[0].address not in selected_set:
                diagnostics.append(f"missing interval function {function.address} {function.name}")
        if selected_set == expected_set and selected_addresses != expected_addresses:
            diagnostics.append("selected linked function sequence is reordered")

    expected_by_address = {
        group[0].address: (index, function)
        for index, (function, group) in enumerate(resolved)
    }
    contributions: list[LinkedOrderContribution] = []
    for address in selected_addresses:
        group = groups[address]
        expected = expected_by_address.get(address)
        if expected is None:
            manifest_address = ""
            manifest_name = ""
            disposition = "selected-unlisted-function"
        else:
            index, function = expected
            manifest_address = function.address
            manifest_name = function.name
            disposition = (
                "section-boundary-first-function"
                if boundary is not None and index == 0
                else "boundary-sentinel"
                if index in {0, len(resolved) - 1}
                else "selected-at-retail-address"
            )
        contributions.append(
            LinkedOrderContribution(
                segment=group[0].segment,
                offset=group[0].offset,
                linked_address=address,
                linked_rva=address - parsed_map.preferred_load_address,
                symbols=tuple(sorted({item.symbol for item in group})),
                providers=tuple(sorted({item.object for item in group if item.object})),
                map_sources=tuple(sorted({item.source for item in group})),
                manifest_address=manifest_address,
                manifest_name=manifest_name,
                disposition=disposition,
            )
        )
    return LinkedOrderCheck(
        target_name=target.name,
        interval_name=interval.name,
        contributions=tuple(contributions),
        diagnostics=tuple(diagnostics),
        order_scope="full",
        required_presence_passed=not presence_diagnostics,
        authored_relative_order_passed=False,
        block_precedence_passed=not diagnostics,
        exact_selected_sequence_matches_manifest=not diagnostics,
        linked_required_presence_passed=not presence_diagnostics,
        linked_projection_complete=not diagnostics,
        linked_scope_projection_complete=not diagnostics,
        linked_exact_selected_population_evaluated=True,
        linked_exact_selected_population_passed=not diagnostics,
        linked_seams_and_rvas_evaluated=True,
        linked_seams_and_rvas_passed=not diagnostics,
    )


NOICF_FOLD_BOUNDARY_DIAGNOSTIC = (
    "current block does not retain its required boundary/before-successor precedence"
)


def _linked_map_object_name(value: str) -> str:
    return value.replace("\\", "/").rsplit("/", 1)[-1]


def apply_linked_order_diagnostic_mode(
    *,
    check: LinkedOrderCheck,
    target: VerifyTarget,
    parsed_map: ParsedLinkMap,
    config: FinalBuildConfig | None,
    order_scope: str,
) -> LinkedOrderCheck:
    """Apply the narrow REF+NOICF controlled-identity diagnostic contract."""

    mode = target.linked_order_diagnostic_mode
    if not mode.kind:
        return check

    binding_diagnostics: list[str] = []
    if target.linked_order_base_target != "hud_404ca0_415ab0_linked_authored_order":
        binding_diagnostics.append(
            "NOICF controlled diagnostic is bound only to the HUD authored-order base target"
        )
    if order_scope != "authored":
        binding_diagnostics.append(
            "NOICF controlled diagnostic requires authored order scope and cannot run as full order"
        )
    if config is None:
        binding_diagnostics.append(
            "NOICF controlled diagnostic requires final-build configuration provenance"
        )
        link_flags: set[str] = set()
    else:
        link_flags = {flag.upper() for flag in config.link_flags}
        if not config.diagnostic_only:
            binding_diagnostics.append(
                "NOICF controlled diagnostic requires a diagnostic-only final build"
            )
        if config.link_profile != mode.required_link_profile:
            binding_diagnostics.append(
                "NOICF controlled diagnostic requires link profile "
                f"{mode.required_link_profile!r}, got {config.link_profile!r}"
            )
        if "/OPT:REF" not in link_flags or "/OPT:NOICF" not in link_flags:
            binding_diagnostics.append(
                "NOICF controlled diagnostic requires effective /OPT:REF and /OPT:NOICF flags"
            )
        if "/OPT:NOREF" in link_flags or "/OPT:ICF" in link_flags:
            binding_diagnostics.append(
                "NOICF controlled diagnostic rejects /OPT:NOREF or /OPT:ICF"
            )

    blocking_predicates = tuple(
        {
            "predicate": predicate,
            "blocking": True,
            "nonblocking_reason": "",
        }
        for predicate in mode.nonblocking_predicates
    )
    if binding_diagnostics:
        return replace(
            check,
            diagnostics=tuple((*check.diagnostics, *binding_diagnostics)),
            diagnostic_mode_kind=mode.kind,
            diagnostic_mode_applied=False,
            diagnostic_nonblocking_reason="",
            diagnostic_predicate_results=blocking_predicates,
        )

    controlled_assertions: list[dict[str, object]] = []
    controlled_diagnostics: list[str] = []
    resolved_addresses: list[int] = []
    forbidden = {item.casefold() for item in mode.forbidden_objects}
    for identity in mode.controlled_identities:
        contribution_rows: dict[tuple[int, int, int, str], LinkedMapSymbol] = {}
        for item in parsed_map.symbols:
            if not item.is_function or item.symbol != identity.symbol:
                continue
            object_name = _linked_map_object_name(item.object)
            contribution_rows.setdefault(
                (item.segment, item.offset, item.address, object_name.casefold()),
                item,
            )
        rows = tuple(contribution_rows.values())
        objects = tuple(sorted({_linked_map_object_name(item.object) for item in rows}))
        addresses = tuple(sorted({item.address for item in rows}))
        assertion_diagnostics: list[str] = []
        forbidden_hits: list[str] = []
        if not rows:
            assertion_diagnostics.append(
                f"controlled identity {identity.name} is missing from the selected NOICF map"
            )
        elif len(rows) != 1:
            assertion_diagnostics.append(
                f"controlled identity {identity.name} is duplicated or ambiguous "
                f"(selected_contribution_count={len(rows)})"
            )
        if rows:
            actual_object_keys = {item.casefold() for item in objects}
            if actual_object_keys != {identity.expected_object.casefold()}:
                assertion_diagnostics.append(
                    f"controlled identity {identity.name} selected provider(s) "
                    f"{list(objects)!r}, expected only {identity.expected_object!r}"
                )
            forbidden_hits = sorted(
                item for item in objects if item.casefold() in forbidden
            )
            if forbidden_hits:
                assertion_diagnostics.append(
                    f"controlled identity {identity.name} has forbidden provider(s) "
                    f"{forbidden_hits!r}"
                )
        if len(rows) == 1:
            resolved_addresses.append(rows[0].address)
        controlled_diagnostics.extend(assertion_diagnostics)
        controlled_assertions.append(
            {
                "name": identity.name,
                "symbol": identity.symbol,
                "expected_object": identity.expected_object,
                "forbidden_objects": list(mode.forbidden_objects),
                "expected_selected_contribution_count": 1,
                "actual_selected_contribution_count": len(rows),
                "forbidden_selected_contribution_count": len(forbidden_hits),
                "selected_contribution_count": len(rows),
                "selected_addresses": [f"0x{address:x}" for address in addresses],
                "selected_objects": list(objects),
                "passed": not assertion_diagnostics,
                "diagnostics": assertion_diagnostics,
            }
        )

    if len(resolved_addresses) == len(mode.controlled_identities):
        if len(set(resolved_addresses)) != len(resolved_addresses):
            controlled_diagnostics.append(
                "controlled NOICF identities resolve to the same linked address"
            )
        elif resolved_addresses != sorted(resolved_addresses):
            controlled_diagnostics.append(
                "controlled NOICF identities do not retain their declared relative order"
            )

    relative_order_assertions: list[dict[str, object]] = []
    for before, after in zip(controlled_assertions, controlled_assertions[1:]):
        before_addresses = before["selected_addresses"]
        after_addresses = after["selected_addresses"]
        before_address = before_addresses[0] if len(before_addresses) == 1 else None
        after_address = after_addresses[0] if len(after_addresses) == 1 else None
        addresses_distinct = bool(
            before_address is not None
            and after_address is not None
            and before_address != after_address
        )
        before_after_passed = bool(
            addresses_distinct
            and int(before_address, 16) < int(after_address, 16)
        )
        relative_order_assertions.append(
            {
                "before_symbol": before["symbol"],
                "after_symbol": after["symbol"],
                "before_address": before_address,
                "after_address": after_address,
                "addresses_distinct": addresses_distinct,
                "before_after_order_passed": before_after_passed,
            }
        )

    boundary_diagnostic_present = NOICF_FOLD_BOUNDARY_DIAGNOSTIC in check.diagnostics
    fold_only_boundary_expansion = bool(
        boundary_diagnostic_present
        and check.boundary_offender_proof_complete
        and check.boundary_offenders
        and all(
            bool(item.get("declared_icf_fold_dependent"))
            and item.get("classification") == "declared-icf-fold-dependent"
            for item in check.boundary_offenders
        )
    )
    remaining_diagnostics = list(check.diagnostics)
    if fold_only_boundary_expansion:
        remaining_diagnostics.remove(NOICF_FOLD_BOUNDARY_DIAGNOSTIC)
    boundary_was_exempted = fold_only_boundary_expansion
    if boundary_diagnostic_present and not boundary_was_exempted:
        proof_reasons = list(check.boundary_offender_proof_diagnostics)
        if not check.boundary_offenders:
            proof_reasons.append("no structured boundary offender was established")
        non_fold_offenders = [
            item
            for item in check.boundary_offenders
            if not item.get("declared_icf_fold_dependent")
            or item.get("classification") != "declared-icf-fold-dependent"
        ]
        if non_fold_offenders:
            proof_reasons.append(
                f"{len(non_fold_offenders)} boundary offender(s) are not declared ICF-fold-dependent"
            )
        controlled_diagnostics.append(
            "NOICF boundary exemption denied: "
            + "; ".join(dict.fromkeys(proof_reasons))
        )
    nonblocking_diagnostics = list(check.nonblocking_diagnostics)
    if boundary_was_exempted:
        nonblocking_diagnostics.append(
            f"{mode.nonblocking_reason}: observed block precedence and boundary sentinels "
            "remain false under the declared REF+NOICF fold-family expansion; this exact "
            "diagnostic mode reports them without treating them as acceptance gates"
        )

    declared_fold_offenders = [
        row for row in check.boundary_offenders if row.get("declared_icf_fold_dependent")
    ]
    declared_fold_addresses = sorted(
        {
            str(row.get("linked_address"))
            for row in declared_fold_offenders
            if row.get("linked_address")
        }
    )
    predicate_results: list[dict[str, object]] = []
    for predicate in mode.nonblocking_predicates:
        result: dict[str, object] = {
            "predicate": predicate,
            "blocking": not boundary_was_exempted,
            "nonblocking_reason": (
                mode.nonblocking_reason if boundary_was_exempted else ""
            ),
        }
        if predicate in {"block_precedence", "boundary_sentinels"}:
            result["observed_passed"] = check.block_precedence_passed
            result["diagnostic_was_present"] = boundary_diagnostic_present
            result["exemption_applied"] = boundary_was_exempted
            result["offender_proof_complete"] = check.boundary_offender_proof_complete
        else:
            result.update(
                {
                    "reported": True,
                    "exemption_applied": boundary_was_exempted,
                    "offender_proof_complete": check.boundary_offender_proof_complete,
                    "offender_proof_diagnostics": list(
                        check.boundary_offender_proof_diagnostics
                    ),
                    "boundary_offenders": list(check.boundary_offenders),
                    "declared_fold_participant_count": len(declared_fold_offenders),
                    "selected_distinct_address_count": len(declared_fold_addresses),
                    "selected_linked_addresses": declared_fold_addresses,
                }
            )
        predicate_results.append(result)

    return replace(
        check,
        diagnostics=tuple((*remaining_diagnostics, *controlled_diagnostics)),
        nonblocking_diagnostics=tuple(nonblocking_diagnostics),
        diagnostic_mode_kind=mode.kind,
        diagnostic_mode_applied=True,
        diagnostic_nonblocking_reason=mode.nonblocking_reason,
        diagnostic_predicate_results=tuple(predicate_results),
        controlled_identity_assertions=tuple(controlled_assertions),
        controlled_relative_order_assertions=tuple(relative_order_assertions),
    )


def _raw_vc5_manifest_catalog(
    directory: Path | None = None,
) -> tuple[dict[str, Path], tuple[tuple[Path, dict[str, Any]], ...]]:
    """Index manifest names cheaply without running source-policy validation."""
    directory = directory or DEFAULT_VC5_VERIFY_MANIFEST_DIR
    by_name: dict[str, Path] = {}
    rows: list[tuple[Path, dict[str, Any]]] = []
    for path in sorted(directory.glob("*.json")):
        try:
            payload = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
            raise ValueError(f"{path}: cannot index VC5 manifest: {exc}") from exc
        if not isinstance(payload, dict):
            raise ValueError(f"{path}: VC5 manifest root must be an object")
        name = payload.get("name")
        if not isinstance(name, str) or not name.strip():
            raise ValueError(f"{path}: VC5 manifest requires a non-empty name")
        name = name.strip()
        previous = by_name.get(name)
        if previous is not None:
            raise ValueError(
                f"duplicate VC5 manifest name {name!r}: {previous} and {path}"
            )
        resolved = path.resolve()
        by_name[name] = resolved
        rows.append((resolved, payload))
    return by_name, tuple(rows)


def load_linked_order_support_manifests(
    targets: tuple[VerifyTarget, ...],
) -> tuple[VerifyTarget, ...]:
    """Strict-load only same-range TU contracts relevant to selected targets."""
    _by_name, raw_rows = _raw_vc5_manifest_catalog()
    selected_paths = {target.manifest_path.resolve() for target in targets}
    candidate_paths: set[Path] = set()
    for path, payload in raw_rows:
        if path in selected_paths:
            continue
        translation_units = payload.get("translation_unit_function_order")
        if not isinstance(translation_units, list) or not translation_units:
            continue
        binary = str(payload.get("target_binary", "recoil"))
        compiler_profile = str(payload.get("compiler_profile", ""))
        retail_start = str(payload.get("retail_start", ""))
        retail_end = str(payload.get("retail_end_exclusive", ""))
        if any(
            binary == target.target_binary
            and compiler_profile == target.compiler_profile
            and retail_start == target.retail_start
            and retail_end == target.retail_end_exclusive
            for target in targets
        ):
            candidate_paths.add(path)
    return tuple(
        load_vc5_verify_manifest(path, enforce_source_policy=True)
        for path in sorted(candidate_paths)
    )


def load_linked_order_targets(
    names: tuple[str, ...],
    manifest_paths: tuple[Path, ...] = (),
) -> tuple[VerifyTarget, ...]:
    if not names and not manifest_paths:
        return ()
    manifest_paths_by_name, _raw_rows = _raw_vc5_manifest_catalog()
    loaded_by_name: dict[str, VerifyTarget] = {}

    def load_named(name: str) -> VerifyTarget:
        loaded = loaded_by_name.get(name)
        if loaded is not None:
            return loaded
        path = manifest_paths_by_name.get(name)
        if path is None:
            raise ValueError(f"linked order target not found: {name}")
        loaded = load_vc5_verify_manifest(path, enforce_source_policy=True)
        if loaded.name != name:
            raise ValueError(
                f"{path}: indexed manifest name {name!r} changed during strict load"
            )
        loaded_by_name[name] = loaded
        return loaded

    def resolve_diagnostic_overlay(target: VerifyTarget) -> VerifyTarget:
        if not target.linked_order_base_target:
            return target
        base = load_named(target.linked_order_base_target)
        if base.linked_order_base_target or base.linked_order_diagnostic_mode.kind:
            raise ValueError(
                f"{target.manifest_path}: linked-order diagnostic overlays cannot inherit "
                "from another diagnostic overlay"
            )
        if not base.linked_function_intervals:
            raise ValueError(
                f"{target.manifest_path}: linked_order_base_target "
                f"{base.name} has no linked_function_intervals"
            )
        if target.target_binary != base.target_binary:
            raise ValueError(
                f"{target.manifest_path}: diagnostic target_binary {target.target_binary!r} "
                f"does not match base target {base.target_binary!r}"
            )
        if target.compiler_profile and target.compiler_profile != base.compiler_profile:
            raise ValueError(
                f"{target.manifest_path}: diagnostic compiler_profile "
                f"{target.compiler_profile!r} does not match base target "
                f"{base.compiler_profile!r}"
            )
        return replace(
            base,
            name=target.name,
            description=target.description,
            manifest_path=target.manifest_path,
            compiler_profile=target.compiler_profile or base.compiler_profile,
            linked_order_base_target=base.name,
            linked_order_diagnostic_mode=target.linked_order_diagnostic_mode,
        )

    selected: list[VerifyTarget] = []
    for name in names:
        target = load_named(name)
        target = resolve_diagnostic_overlay(target)
        if not target.linked_function_intervals:
            raise ValueError(f"{target.manifest_path}: order target {name} has no linked_function_intervals")
        selected.append(target)
    selected_names = {target.name for target in selected}
    seen_paths: set[Path] = set()
    for raw_path in manifest_paths:
        path = raw_path.resolve()
        if path in seen_paths:
            raise ValueError(f"duplicate --order-target-file path: {path}")
        seen_paths.add(path)
        if not path.is_file():
            raise ValueError(f"order target file not found: {path}")
        target = load_vc5_verify_manifest(path, enforce_source_policy=True)
        target = resolve_diagnostic_overlay(target)
        if not target.linked_function_intervals:
            raise ValueError(f"{path}: order target {target.name} has no linked_function_intervals")
        if target.name in selected_names:
            raise ValueError(f"duplicate linked order target name: {target.name}")
        selected_names.add(target.name)
        selected.append(target)
    return tuple(selected)


RECOGNIZED_DIAGNOSTIC_ISOLATION_KIND = "ref-noicf-controlled-identity-order"


def normalized_optimization_flags(config: FinalBuildConfig) -> tuple[str, ...]:
    return tuple(flag.upper() for flag in config.link_flags if flag.upper().startswith("/OPT:"))


def resolve_order_target_routing(
    *,
    config: FinalBuildConfig,
    order_scope: str,
    mandatory_target_ids: tuple[str, ...],
    explicit_targets: tuple[VerifyTarget, ...],
) -> OrderTargetRouting:
    mandatory_ids = tuple(mandatory_target_ids)
    if len(mandatory_ids) != len(set(mandatory_ids)):
        raise ValueError("mandatory linked-order target ledger contains duplicate ids")
    explicit_ids = tuple(target.name for target in explicit_targets)
    if len(explicit_ids) != len(set(explicit_ids)):
        raise ValueError("explicit linked-order target selection contains duplicate ids")
    diagnostic_targets = tuple(
        target for target in explicit_targets if target.linked_order_diagnostic_mode.kind
    )
    if not diagnostic_targets:
        effective_ids = tuple(dict.fromkeys((*mandatory_ids, *explicit_ids)))
        return OrderTargetRouting(
            diagnostic_isolation_applied=False,
            explicit_target_ids=explicit_ids,
            effective_target_ids=effective_ids,
            mandatory_target_ids=mandatory_ids,
            activation=(
                ("requested", False),
                ("applied", False),
            ),
        )

    if not explicit_targets:
        raise ValueError("diagnostic target isolation requires at least one explicit target")
    kinds = {target.linked_order_diagnostic_mode.kind for target in explicit_targets}
    if kinds != {RECOGNIZED_DIAGNOSTIC_ISOLATION_KIND}:
        raise ValueError(
            "diagnostic target isolation rejects mixed ordinary targets or unrecognized diagnostic kinds"
        )
    if not config.diagnostic_only:
        raise ValueError("diagnostic target isolation requires diagnostic_only=true")
    if order_scope != "authored":
        raise ValueError("diagnostic target isolation requires authored order scope")
    if config.link_profile != "vc5sp3_ref_noicf":
        raise ValueError(
            "diagnostic target isolation requires exact link profile 'vc5sp3_ref_noicf'"
        )
    optimization_flags = normalized_optimization_flags(config)
    if optimization_flags != ("/OPT:REF", "/OPT:NOICF"):
        raise ValueError(
            "diagnostic target isolation requires normalized exact optimization flags "
            "[/OPT:REF, /OPT:NOICF]"
        )
    if any(
        target.linked_order_base_target != "hud_404ca0_415ab0_linked_authored_order"
        for target in explicit_targets
    ):
        raise ValueError("diagnostic target isolation rejects a manifest/base-target mismatch")
    return OrderTargetRouting(
        diagnostic_isolation_applied=True,
        explicit_target_ids=explicit_ids,
        effective_target_ids=explicit_ids,
        mandatory_target_ids=mandatory_ids,
        recognized_diagnostic_kind=RECOGNIZED_DIAGNOSTIC_ISOLATION_KIND,
        activation=(
            ("requested", True),
            ("authority", "selected-manifest"),
            ("recognized_kind", RECOGNIZED_DIAGNOSTIC_ISOLATION_KIND),
            ("applied", True),
            ("activation_checks_passed", True),
            ("diagnostic_only_config", True),
            ("order_scope_expected", "authored"),
            ("order_scope_actual", order_scope),
            ("order_scope_exact_match", True),
            ("link_profile_expected", "vc5sp3_ref_noicf"),
            ("link_profile_actual", config.link_profile),
            ("link_profile_exact_match", True),
            ("optimization_flags_expected", ["/OPT:REF", "/OPT:NOICF"]),
            ("optimization_flags_actual", list(optimization_flags)),
            ("optimization_flags_exact_match", True),
            ("icf_flag_present", False),
            ("all_selected_targets_diagnostic", True),
            ("selected_target_ids", list(explicit_ids)),
            ("effective_target_ids", list(explicit_ids)),
            ("effective_targets_equal_selected_targets", True),
        ),
    )


def diagnostic_isolation_summary_fields(
    *,
    routing: OrderTargetRouting,
    order_reports: list[dict[str, str]],
    selected_targets_passed: bool,
) -> dict[str, object]:
    if not routing.diagnostic_isolation_applied:
        raise ValueError("diagnostic isolation summary requested for ordinary routing")
    mandatory_ids = routing.mandatory_target_ids
    report_ids = tuple(dict.fromkeys(row["target"] for row in order_reports))
    fields: dict[str, object] = {
        "kind": "final-build-diagnostic",
        "validation_mode": "diagnostic-target-isolation",
        "diagnostic_target_isolation": dict(routing.activation),
        "mandatory_order_targets": {
            "ledger_target_ids": list(mandatory_ids),
            "inventory_complete": True,
            "evaluated_target_ids": [],
            "evaluation_state": "not_evaluated_in_diagnostic_link_mode",
            "passed": None,
            "entries": [
                {
                    "target_id": target_id,
                    "evaluation_state": "not_evaluated_in_diagnostic_link_mode",
                    "reason": "ordinary_required_target_deferred_to_mandatory_icf_stage",
                }
                for target_id in mandatory_ids
            ],
        },
        "selected_diagnostic_targets_passed": selected_targets_passed,
        "required_order_targets_passed": None,
        "normal_linked_order_passed": None,
        "normal_final_build_qualified": False,
        "authored_order_acceptance_eligible": False,
        "order_prefix_advance_eligible": False,
        "tracker_mutation_eligible": False,
        "byte_acceptance_eligible": False,
        "final_equivalence_eligible": False,
        "followup_icf_required": True,
        "required_followup_link_profile": "vc5sp3_ref_icf",
        "order_reports_target_ids": list(report_ids),
        "prerequisites": {
            "source_guards_evaluated": False,
            "raw_order_evaluated": False,
            "binding_state": "external-unbound",
        },
    }
    validate_diagnostic_isolation_summary(fields, routing=routing, order_reports=order_reports)
    return fields


def validate_diagnostic_isolation_summary(
    summary: dict[str, object],
    *,
    routing: OrderTargetRouting,
    order_reports: list[dict[str, str]],
) -> None:
    if summary.get("kind") != "final-build-diagnostic" or summary.get("validation_mode") != "diagnostic-target-isolation":
        raise ValueError("diagnostic isolation summary has an acceptance-ambiguous kind or validation mode")
    if summary.get("required_order_targets_passed") is not None:
        raise ValueError("diagnostic isolation must not mark mandatory targets passed")
    mandatory = summary.get("mandatory_order_targets")
    if not isinstance(mandatory, dict):
        raise ValueError("diagnostic isolation summary is missing mandatory target inventory")
    ledger_ids = mandatory.get("ledger_target_ids")
    entries = mandatory.get("entries")
    if (
        not isinstance(ledger_ids, list)
        or ledger_ids != list(routing.mandatory_target_ids)
        or len(ledger_ids) != len(set(ledger_ids))
    ):
        raise ValueError("diagnostic isolation mandatory ledger inventory is missing, duplicated, or reordered")
    if not isinstance(entries, list) or [item.get("target_id") for item in entries] != list(routing.mandatory_target_ids):
        raise ValueError("diagnostic isolation mandatory ledger entries are missing or duplicated")
    if mandatory.get("evaluated_target_ids") != [] or mandatory.get("passed") is not None:
        raise ValueError("diagnostic isolation must not evaluate or pass mandatory targets")
    if any(
        item.get("evaluation_state") != "not_evaluated_in_diagnostic_link_mode"
        or not item.get("reason")
        or "passed" in item
        for item in entries
    ):
        raise ValueError("diagnostic isolation mandatory entries must be not-evaluated and never passed")
    report_ids = tuple(dict.fromkeys(row.get("target", "") for row in order_reports))
    if report_ids != routing.explicit_target_ids or tuple(summary.get("order_reports_target_ids", [])) != report_ids:
        raise ValueError("diagnostic isolation reports do not exactly match explicit diagnostic targets")
    if routing.effective_target_ids != routing.explicit_target_ids:
        raise ValueError("diagnostic isolation effective targets differ from explicit diagnostic targets")
    for field in (
        "normal_final_build_qualified",
        "authored_order_acceptance_eligible",
        "order_prefix_advance_eligible",
        "tracker_mutation_eligible",
        "byte_acceptance_eligible",
        "final_equivalence_eligible",
    ):
        if summary.get(field) is not False:
            raise ValueError(f"diagnostic isolation summary must set {field}=false")
    if summary.get("followup_icf_required") is not True or summary.get("required_followup_link_profile") != "vc5sp3_ref_icf":
        raise ValueError("diagnostic isolation summary must require the ordinary REF+ICF follow-up")


def normal_order_summary_fields(
    *,
    routing: OrderTargetRouting,
    order_reports: list[dict[str, str]],
    order_scope: str,
) -> dict[str, object]:
    if routing.diagnostic_isolation_applied:
        raise ValueError("normal order summary requested for diagnostic isolation routing")
    report_ids = tuple(dict.fromkeys(row["target"] for row in order_reports))
    missing = [target for target in routing.mandatory_target_ids if target not in report_ids]
    if missing:
        raise ValueError(
            "normal linked-order evaluation omitted mandatory target(s): " + ", ".join(missing)
        )
    return {
        "validation_mode": (
            "normal-authored-order" if order_scope == "authored" else "normal-full-order"
        ),
        "diagnostic_target_isolation": {"requested": False, "applied": False},
        "required_order_targets_evaluation_state": "evaluated",
        "required_order_target_set_matches_ledger": True,
        "evaluated_mandatory_target_ids": list(routing.mandatory_target_ids),
        "skipped_required_order_targets": [],
    }


def validate_external_order_targets(
    targets: tuple[VerifyTarget, ...],
    *,
    expected_binary: str,
    order_scope: str,
    configured_sources: tuple[Path, ...],
) -> None:
    configured = {path.resolve() for path in configured_sources}
    for target in targets:
        if target.target_binary != expected_binary:
            raise ValueError(
                f"{target.manifest_path}: target_binary {target.target_binary!r} does not match final build {expected_binary!r}"
            )
        if not target.retail_start or not target.retail_end_exclusive:
            raise ValueError(f"{target.manifest_path}: external order target requires an exact retail range")
        start = int(target.retail_start, 16)
        end = int(target.retail_end_exclusive, 16)
        if start >= end:
            raise ValueError(f"{target.manifest_path}: external order target retail range is invalid")
        if len(target.translation_unit_function_order) != 1 or len(target.linked_function_intervals) != 1:
            raise ValueError(
                f"{target.manifest_path}: external order target requires exactly one TU order and one linked interval"
            )
        entry = target.translation_unit_function_order[0]
        interval = target.linked_function_intervals[0]
        if interval.retail_start != target.retail_start or interval.retail_end_exclusive != target.retail_end_exclusive:
            raise ValueError(
                f"{target.manifest_path}: linked interval retail range must equal the target retail range"
            )
        if entry.order_scope != order_scope or interval.order_scope != order_scope:
            raise ValueError(
                f"{target.manifest_path}: external order target scope must match final-build order scope {order_scope!r}"
            )
        if order_scope != "authored":
            raise ValueError(f"{target.manifest_path}: --order-target-file currently requires authored order scope")
        if entry.source_from != target.source_from or repo_path(target.source_from).resolve() not in configured:
            raise ValueError(
                f"{target.manifest_path}: external order target compile host is not one configured final-build source"
            )
        def signature(function: VerifyFunction) -> tuple[str, str, str, str, str, str]:
            if not function.symbol or function.symbol_regex:
                raise ValueError(
                    f"{target.manifest_path}: external order target requires exact decorated identities at {function.address}"
                )
            if function.pipeline_class not in AUTHORED_PIPELINE_CLASSES:
                raise ValueError(
                    f"{target.manifest_path}: external order target body {function.address} is not authored/authored-lifecycle"
                )
            address = int(function.address, 16)
            if not start <= address < end:
                raise ValueError(f"{target.manifest_path}: function {function.address} lies outside the retail range")
            return (
                function.address,
                function.symbol,
                function.pipeline_class,
                function.authored_order_role,
                function.logical_identity_key,
                function.icf_fold_status,
            )

        object_signature = tuple(signature(function) for function in entry.functions)
        linked_signature = tuple(signature(function) for function in interval.functions)
        if object_signature != linked_signature:
            raise ValueError(
                f"{target.manifest_path}: external object and linked authored projections do not match exactly"
            )
        if int(interval.successor.address, 16) != end:
            raise ValueError(
                f"{target.manifest_path}: linked successor must equal retail_end_exclusive {target.retail_end_exclusive}"
            )
        if interval.predecessor_section_boundary is not None:
            if interval.predecessor_section_boundary.section != ".text" or int(
                interval.predecessor_section_boundary.address, 16
            ) != start:
                raise ValueError(f"{target.manifest_path}: invalid predecessor section boundary")
        elif interval.predecessor is None or int(interval.predecessor.address, 16) >= start:
            raise ValueError(f"{target.manifest_path}: invalid mechanical predecessor")


def _bound_semantic_linked_inventories(payload: dict[str, Any]) -> None:
    """Keep live reports actionable without serializing whole downstream catalogs."""
    specifications = {
        "classified_selected_extras": ("pipeline_class", "retail_interval_member"),
        "unclassified_selected_extras": ("classification_state", "retail_interval_member"),
        "identity_alias_classifications": ("state", "retail_interval_member"),
        "linked_physical_identity_classes": ("state", "authored_order_blocking_scope_member"),
    }
    summary: dict[str, dict[str, Any]] = {}
    for field, (state_key, blocking_key) in specifications.items():
        rows = payload.get(field)
        if not isinstance(rows, list):
            continue
        state_counts: dict[str, int] = {}
        for row in rows:
            state = (
                str(row.get(state_key) or "unspecified")
                if isinstance(row, Mapping)
                else "invalid-row"
            )
            state_counts[state] = state_counts.get(state, 0) + 1
        prioritized = sorted(
            enumerate(rows),
            key=lambda item: (
                0
                if isinstance(item[1], Mapping) and item[1].get(blocking_key) is True
                else 1,
                item[0],
            ),
        )
        retained = [
            row for _index, row in prioritized[:SEMANTIC_LINKED_INVENTORY_LIMIT]
        ]
        payload[field] = retained
        summary[field] = {
            "total_count": len(rows),
            "retained_count": len(retained),
            "omitted_count": len(rows) - len(retained),
            "truncated": len(retained) != len(rows),
            "state_counts": dict(sorted(state_counts.items())),
        }
    payload["semantic_inventory_summary"] = summary


def linked_order_report_data(
    check: LinkedOrderCheck,
    *,
    target: VerifyTarget | None = None,
    interval: LinkedFunctionInterval | None = None,
    map_path: Path | None = None,
    binary: str = "recoil",
    compiler_profile: str = "VC5SP3 final link",
    compile_profile_provenance: dict[str, Any] | None = None,
    config: FinalBuildConfig | None = None,
    resolved_link_inputs: list[dict[str, object]] | None = None,
    progress_document: ProgressDocument | None = None,
    raw_definitions: tuple[RawObjectFunctionDefinition, ...] | None = None,
) -> dict[str, Any]:
    starts = [int(item.address, 0) for item in interval.functions] if interval else []
    retail_start = (
        int(interval.retail_start, 0)
        if interval is not None and interval.retail_start
        else (min(starts) if starts else None)
    )
    retail_end = (
        int(interval.retail_end_exclusive, 0)
        if interval is not None and interval.retail_end_exclusive
        else (int(interval.successor.address, 0) if interval else None)
    )
    payload = {
        "report_version": 1,
        "kind": "linked-function-order-report",
        "target": check.target_name,
        "interval": check.interval_name,
        "order_scope": check.order_scope,
        "retail_start": f"0x{retail_start:x}" if retail_start is not None else "",
        "retail_end_exclusive": f"0x{retail_end:x}" if retail_end is not None else "",
        "ok": check.ok,
        "passed": check.ok,
        "binary": binary,
        "compiler_profile": compiler_profile,
        "compile_profile_provenance_passed": (
            compile_profile_provenance.get("passed", True)
            if compile_profile_provenance is not None
            else True
        ),
        "compile_profile_provenance": compile_profile_provenance or {},
        "linked_order_evaluated": True,
        "required_presence_passed": check.required_presence_passed,
        "authored_relative_order_passed": check.authored_relative_order_passed,
        "block_precedence_passed": check.block_precedence_passed,
        "exact_selected_sequence_matches_manifest": check.exact_selected_sequence_matches_manifest,
        "exact_sequence_address_seam_claimed": check.order_scope == "full",
        "linked_order_passed": check.ok,
        "boundary_sentinels_passed": check.block_precedence_passed,
        "raw_authored_order_evaluated": check.raw_authored_order_evaluated,
        "raw_authored_order_passed": check.raw_authored_order_passed,
        "linked_known_authored_relative_order_passed": check.linked_known_authored_relative_order_passed,
        "linked_known_authored_relative_order_scope": check.linked_known_authored_relative_order_scope,
        "linked_required_presence_passed": check.linked_required_presence_passed,
        "linked_projection_complete": check.linked_projection_complete,
        "linked_scope_projection_complete": check.linked_scope_projection_complete,
        "linked_exact_selected_population_evaluated": check.linked_exact_selected_population_evaluated,
        "linked_exact_selected_population_passed": check.linked_exact_selected_population_passed,
        "linked_seams_and_rvas_evaluated": check.linked_seams_and_rvas_evaluated,
        "linked_seams_and_rvas_passed": check.linked_seams_and_rvas_passed,
        "identity_catalog_source": check.identity_catalog_source,
        "tracker_revision": check.tracker_revision,
        "diagnostic_mode": {
            "kind": check.diagnostic_mode_kind,
            "applied": check.diagnostic_mode_applied,
            "link_and_map_parse_passed": check.diagnostic_mode_applied,
            "nonblocking_reason": check.diagnostic_nonblocking_reason,
            "predicate_results": list(check.diagnostic_predicate_results),
        },
        "controlled_identity_assertions": list(check.controlled_identity_assertions),
        "controlled_relative_order_assertions": list(
            check.controlled_relative_order_assertions
        ),
        "boundary_offender_proof_complete": check.boundary_offender_proof_complete,
        "boundary_offender_proof_diagnostics": list(
            check.boundary_offender_proof_diagnostics
        ),
        "boundary_offenders": list(check.boundary_offenders),
        "classified_selected_extras": list(check.classified_selected_extras),
        "unclassified_selected_extras": list(check.unclassified_selected_extras),
        "identity_alias_classifications": list(check.alias_classifications),
        "linked_physical_identity_classes": list(check.physical_classes),
        "required_identity_dispositions": list(check.required_identity_dispositions),
        "raw_definition_inventory_complete": check.raw_definition_inventory_complete,
        "raw_definition_inventory_diagnostics": list(check.raw_definition_inventory_diagnostics),
        "nonblocking_diagnostics": list(check.nonblocking_diagnostics),
        "map_path": str(map_path.resolve()) if map_path is not None else "",
        "manifest": str(target.manifest_path.resolve()) if target is not None else "",
        "diagnostics": list(check.diagnostics),
        "first_divergence": linked_order_first_divergence(check, interval=interval),
        "contributions": [
            {
                "raw_object_position": None,
                "raw_section": None,
                "decorated_identities": list(item.symbols),
                "alias_group": list(item.symbols),
                "comdat": None,
                "weak": None,
                "selected_providers": list(item.providers),
                "map_sources": list(item.map_sources),
                "segment": item.segment,
                "offset": item.offset,
                "linked_address": f"0x{item.linked_address:x}",
                "linked_rva": f"0x{item.linked_rva:x}",
                "manifest_address": item.manifest_address,
                "manifest_name": item.manifest_name,
                "disposition": item.disposition,
            }
            for item in check.contributions
        ],
    }
    if (
        check.order_scope == "full"
        and check.ok
        and config is not None
        and not config.diagnostic_only
        and target is not None
        and interval is not None
        and map_path is not None
        and resolved_link_inputs is not None
        and progress_document is not None
        and raw_definitions is not None
    ):
        payload.update(
            _typed_linked_full_order_fields(
                check=check,
                target=target,
                interval=interval,
                config=config,
                resolved_link_inputs=resolved_link_inputs,
                progress_document=progress_document,
                raw_definitions=raw_definitions,
            )
        )
    _bound_semantic_linked_inventories(payload)
    return payload


def _linked_contribution_neighbor(item: LinkedOrderContribution) -> dict[str, Any]:
    return {
        "linked_address": f"0x{item.linked_address:x}",
        "manifest_address": item.manifest_address or None,
        "manifest_name": item.manifest_name or None,
        "identities": list(item.symbols),
        "providers": list(item.providers),
        "disposition": item.disposition,
    }


def _linked_expected_neighbor(function: VerifyFunction) -> dict[str, Any]:
    return {
        "retail_address": function.address,
        "name": function.name,
        "identity": function.symbol or function.symbol_regex or function.listing_label_regex,
    }


def _authored_disposition_neighbor(row: Mapping[str, Any]) -> dict[str, Any]:
    identity = row.get("identity", {}) if isinstance(row.get("identity"), Mapping) else {}
    selected = row.get("selected_linked_addresses", [])
    return {
        "retail_address": identity.get("retail_address"),
        "name": identity.get("name"),
        "identity_key": identity.get("identity_key"),
        "identity": (
            row.get("selected_matching_symbols", [None])[0]
            if isinstance(row.get("selected_matching_symbols"), list)
            and row.get("selected_matching_symbols")
            else None
        ),
        "linked_address": selected[0] if isinstance(selected, list) and len(selected) == 1 else None,
        "selected_group_count": row.get("selected_group_count"),
        "disposition": row.get("disposition"),
    }


def linked_order_first_divergence(
    check: LinkedOrderCheck,
    *,
    interval: LinkedFunctionInterval | None,
) -> dict[str, Any] | None:
    """Return the first actionable expected/candidate linked-order mismatch."""
    if not check.diagnostics:
        return None

    if check.order_scope == "authored" and check.required_identity_dispositions:
        dispositions = [
            row
            for row in check.required_identity_dispositions
            if isinstance(row, Mapping) and isinstance(row.get("identity"), Mapping)
        ]
        required = [
            row for row in dispositions
            if row.get("required_in_authored_scope") is True
        ]
        for index, row in enumerate(required):
            if row.get("selected_group_count") == 1:
                continue
            return {
                "kind": "missing-or-ambiguous-identity",
                "message": check.diagnostics[0],
                "expected": _authored_disposition_neighbor(row),
                "actual": {
                    "selected_group_count": row.get("selected_group_count"),
                    "selected_linked_addresses": list(row.get("selected_linked_addresses", [])),
                },
                "expected_neighbors": [
                    _authored_disposition_neighbor(item)
                    for item in required[max(0, index - 1):index + 2]
                ],
                "candidate_neighbors": [],
            }

        ordered = [
            row
            for row in dispositions
            if row["identity"].get("authored_relative_order_gate") is True
            and isinstance(row.get("selected_linked_addresses"), list)
            and len(row["selected_linked_addresses"]) == 1
        ]
        for index in range(1, len(ordered)):
            previous = ordered[index - 1]
            current = ordered[index]
            previous_linked = int(previous["selected_linked_addresses"][0], 16)
            current_linked = int(current["selected_linked_addresses"][0], 16)
            # Equal addresses are a cofold/physical-class condition, not an
            # actual order inversion.  Cross-retail overfold diagnostics own
            # that blocker; continue until candidate order really decreases.
            if current_linked >= previous_linked:
                continue
            candidate_order = sorted(
                ordered,
                key=lambda row: int(row["selected_linked_addresses"][0], 16),
            )
            offender_keys = {
                previous["identity"].get("identity_key"),
                current["identity"].get("identity_key"),
            }
            candidate_indexes = [
                candidate_index
                for candidate_index, row in enumerate(candidate_order)
                if row["identity"].get("identity_key") in offender_keys
            ]
            candidate_start = max(0, min(candidate_indexes) - 1)
            candidate_end = min(len(candidate_order), max(candidate_indexes) + 2)
            return {
                "kind": "reordered",
                "message": (
                    "first authored linked inversion: expected "
                    f"{previous['identity'].get('retail_address')} {previous['identity'].get('name')} before "
                    f"{current['identity'].get('retail_address')} {current['identity'].get('name')}, "
                    f"but candidate addresses are 0x{previous_linked:x} then 0x{current_linked:x}"
                ),
                "expected": {
                    "before": _authored_disposition_neighbor(previous),
                    "after": _authored_disposition_neighbor(current),
                },
                "actual": {
                    "before": _authored_disposition_neighbor(current),
                    "after": _authored_disposition_neighbor(previous),
                },
                "expected_neighbors": [
                    _authored_disposition_neighbor(item)
                    for item in ordered[max(0, index - 2):index + 2]
                ],
                "candidate_neighbors": [
                    _authored_disposition_neighbor(item)
                    for item in candidate_order[candidate_start:candidate_end]
                ],
            }

    if check.order_scope == "full" and interval is not None:
        expected_functions = (
            (interval.predecessor, *interval.functions, interval.successor)
            if interval.predecessor is not None
            else (*interval.functions, interval.successor)
        )
        actual = list(check.contributions)
        mismatch_index = next(
            (
                index
                for index in range(max(len(expected_functions), len(actual)))
                if index >= len(expected_functions)
                or index >= len(actual)
                or actual[index].manifest_address != expected_functions[index].address
            ),
            None,
        )
        if mismatch_index is not None:
            expected_item = (
                _linked_expected_neighbor(expected_functions[mismatch_index])
                if mismatch_index < len(expected_functions)
                else None
            )
            actual_item = (
                _linked_contribution_neighbor(actual[mismatch_index])
                if mismatch_index < len(actual)
                else None
            )
            kind = (
                "unexpected-selected-contribution"
                if actual_item is not None and actual_item["manifest_address"] is None
                else "missing-selected-contribution"
                if actual_item is None
                else "reordered"
            )
            return {
                "kind": kind,
                "message": check.diagnostics[0],
                "expected": expected_item,
                "actual": actual_item,
                "expected_neighbors": [
                    _linked_expected_neighbor(item)
                    for item in expected_functions[
                        max(0, mismatch_index - 1):mismatch_index + 2
                    ]
                ],
                "candidate_neighbors": [
                    _linked_contribution_neighbor(item)
                    for item in actual[max(0, mismatch_index - 1):mismatch_index + 2]
                ],
            }

    if check.boundary_offenders:
        offender = check.boundary_offenders[0]
        linked_text = offender.get("linked_address") if isinstance(offender, Mapping) else None
        linked_address = int(linked_text, 16) if isinstance(linked_text, str) else None
        candidate_index = next(
            (
                index
                for index, contribution in enumerate(check.contributions)
                if contribution.linked_address == linked_address
            ),
            0,
        )
        return {
            "kind": "boundary-crossing",
            "message": check.diagnostics[0],
            "expected": offender.get("identity") if isinstance(offender, Mapping) else None,
            "actual": dict(offender) if isinstance(offender, Mapping) else offender,
            "expected_neighbors": [],
            "candidate_neighbors": [
                _linked_contribution_neighbor(item)
                for item in check.contributions[
                    max(0, candidate_index - 1):candidate_index + 2
                ]
            ],
        }

    return {
        "kind": "linked-order-divergence",
        "message": check.diagnostics[0],
        "expected_neighbors": [],
        "candidate_neighbors": [
            _linked_contribution_neighbor(item) for item in check.contributions[:3]
        ],
    }


def linked_order_profile_failure_report_data(
    *,
    target: VerifyTarget,
    interval: LinkedFunctionInterval,
    order_scope: str,
    map_path: Path,
    provenance: dict[str, Any],
    blocked_by_other_target: bool = False,
) -> dict[str, Any]:
    diagnostics = list(provenance.get("diagnostics", []))
    if blocked_by_other_target:
        diagnostics.append(
            "linked order was not evaluated because another selected target failed compiler-profile provenance"
        )
    payload = {
        "report_version": 1,
        "kind": "linked-function-order-report",
        "target": target.name,
        "interval": interval.name,
        "order_scope": order_scope,
        "retail_start": interval.retail_start or target.retail_start,
        "retail_end_exclusive": interval.retail_end_exclusive or target.retail_end_exclusive,
        "ok": False,
        "passed": False,
        "binary": target.target_binary,
        "compiler_profile": target.compiler_profile or "VC5SP3 final link",
        "compile_profile_provenance_passed": bool(provenance.get("passed", False)),
        "compile_profile_provenance": provenance,
        "linked_order_evaluated": False,
        "linked_order_passed": False,
        "required_presence_passed": False,
        "authored_relative_order_passed": False,
        "block_precedence_passed": False,
        "exact_selected_sequence_matches_manifest": False,
        "map_path": str(map_path.resolve()),
        "manifest": str(target.manifest_path.resolve()),
        "diagnostics": diagnostics,
        "contributions": [],
        "first_divergence": {
            "kind": "compile-profile-provenance",
            "message": diagnostics[0] if diagnostics else "compile profile provenance failed",
            "candidate_neighbors": [],
        },
    }
    return payload


def run_linked_order_targets(
    *,
    target_names: tuple[str, ...],
    target_files: tuple[Path, ...] = (),
    map_path: Path,
    report_dir: Path,
    order_scope: str | None = None,
    config: FinalBuildConfig | None = None,
    progress_path: Path = DEFAULT_PROGRESS,
) -> int:
    targets = load_linked_order_targets(target_names, target_files)
    if not targets:
        return 0
    profile_contract_manifests = load_linked_order_support_manifests(targets)
    authored_catalog_manifests: tuple[VerifyTarget, ...] | None = None
    profile_provenance: dict[str, dict[str, Any]] = {}
    if config is not None:
        profile_provenance = {
            target.name: linked_order_compile_profile_provenance(
                target,
                config,
                tuple((*profile_contract_manifests, *targets)),
            )
            for target in targets
        }
        failed_profile_targets = {
            name for name, provenance in profile_provenance.items() if not provenance["passed"]
        }
        if failed_profile_targets:
            for target in targets:
                provenance = profile_provenance[target.name]
                for interval in target.linked_function_intervals:
                    effective_scope = order_scope or interval.order_scope
                    scoped_name = (
                        f"{target.name}_{interval.name}_authored"
                        if effective_scope == "authored"
                        else f"{target.name}_{interval.name}"
                    )
                    safe_name = re.sub(r"[^A-Za-z0-9_.-]+", "_", scoped_name)
                    report_path = report_dir / f"linked_order_{safe_name}.json"
                    report_path.write_text(
                        json.dumps(
                            linked_order_profile_failure_report_data(
                                target=target,
                                interval=interval,
                                order_scope=effective_scope,
                                map_path=map_path,
                                provenance=provenance,
                                blocked_by_other_target=target.name not in failed_profile_targets,
                            ),
                            indent=2,
                        )
                        + "\n",
                        encoding="utf-8",
                    )
                    print(f"Linked function interval: {target.name}/{interval.name}")
                    print(f"- order_scope {effective_scope}")
                    print("- linked_order_evaluated False")
                    print(f"- compile_profile_provenance_passed {provenance['passed']}")
                    print(f"- report: {report_path}")
                    for diagnostic in json.loads(report_path.read_text(encoding="utf-8"))["diagnostics"]:
                        print(f"- {diagnostic}")
            return 1

    parsed_map = parse_link_map(map_path)
    raw_definitions: tuple[RawObjectFunctionDefinition, ...] | None = None
    raw_inventory_diagnostics: tuple[str, ...] = ()
    resolved_link_inputs: list[dict[str, object]] | None = None
    progress_document: ProgressDocument | None = None
    if config is not None:
        raw_definitions, raw_inventory_diagnostics = collect_raw_object_function_definitions(config)
        full_scope_requested = any(
            (order_scope or interval.order_scope) == "full"
            for target in targets
            for interval in target.linked_function_intervals
        )
        if full_scope_requested and not config.diagnostic_only:
            resolved_link_inputs = resolved_full_order_link_inputs(
                config,
                build_paths(config),
            )
            progress_document = ProgressDocument.load(progress_path)
    overall = 0
    for target in targets:
        authored_scope_requested = any(
            (order_scope or interval.order_scope) == "authored"
            for interval in target.linked_function_intervals
        )
        identity_catalog = None
        if authored_scope_requested:
            if authored_catalog_manifests is None:
                authored_catalog_manifests = load_vc5_verify_manifests(
                    DEFAULT_VC5_VERIFY_MANIFEST_DIR,
                    enforce_source_policy=False,
                )
            identity_catalog = build_linked_retail_identity_catalog(
                target=target,
                manifests=tuple((*authored_catalog_manifests, *targets)),
                progress_path=progress_path,
            )
        for interval in target.linked_function_intervals:
            effective_scope = order_scope or interval.order_scope
            check = check_linked_function_interval(
                target=target,
                interval=interval,
                parsed_map=parsed_map,
                order_scope=effective_scope,
                identity_catalog=identity_catalog if effective_scope == "authored" else None,
                raw_definitions=raw_definitions,
                raw_inventory_diagnostics=raw_inventory_diagnostics,
            )
            check = apply_linked_order_diagnostic_mode(
                check=check,
                target=target,
                parsed_map=parsed_map,
                config=config,
                order_scope=effective_scope,
            )
            scoped_name = (
                f"{target.name}_{interval.name}_authored"
                if effective_scope == "authored"
                else f"{target.name}_{interval.name}"
            )
            safe_name = re.sub(r"[^A-Za-z0-9_.-]+", "_", scoped_name)
            report_path = report_dir / f"linked_order_{safe_name}.json"
            report = linked_order_report_data(
                check,
                target=target,
                interval=interval,
                map_path=map_path,
                binary=target.target_binary,
                compiler_profile=target.compiler_profile or "VC5SP3 final link",
                compile_profile_provenance=profile_provenance.get(target.name),
                config=config,
                resolved_link_inputs=resolved_link_inputs,
                progress_document=progress_document,
                raw_definitions=raw_definitions,
            )
            report_path.write_text(
                json.dumps(report, indent=2)
                + "\n",
                encoding="utf-8",
            )
            print(f"Linked function interval: {target.name}/{interval.name}")
            print(f"- order_scope {effective_scope}")
            print(f"- required_presence_passed {check.required_presence_passed}")
            print(f"- authored_relative_order_passed {check.authored_relative_order_passed}")
            print(f"- block_precedence_passed {check.block_precedence_passed}")
            print(f"- exact_selected_sequence_matches_manifest {check.exact_selected_sequence_matches_manifest}")
            if effective_scope == "authored":
                print(f"- linked_projection_complete {check.linked_projection_complete}")
                print(f"- linked_scope_projection_complete {check.linked_scope_projection_complete}")
            print(f"- report: {report_path}")
            first_divergence = report.get("first_divergence")
            if isinstance(first_divergence, Mapping):
                print(f"- first divergence: {first_divergence.get('message', 'unspecified')}")
            for diagnostic in check.diagnostics[:SEMANTIC_LINKED_CONSOLE_DIAGNOSTIC_LIMIT]:
                print(f"- {diagnostic}")
            omitted = len(check.diagnostics) - SEMANTIC_LINKED_CONSOLE_DIAGNOSTIC_LIMIT
            if omitted > 0:
                print(f"- {omitted} additional blocking diagnostic(s); inspect the report")
            if check.nonblocking_diagnostics:
                print(
                    "- diagnostic-only: "
                    f"{len(check.nonblocking_diagnostics)} downstream diagnostic summary row(s); "
                    "inspect the report"
                )
            if not check.ok:
                overall = 1
    return overall


def link_source_key(path: Path) -> str:
    return os.path.normcase(str(path))


def parse_link_inputs(
    data: dict[str, Any],
    sources: tuple[Path, ...],
    libs: tuple[str, ...],
    manifest_path: Path,
) -> tuple[LinkInput, ...] | None:
    if "link_inputs" not in data:
        return None
    raw_inputs = data["link_inputs"]
    if not isinstance(raw_inputs, list):
        raise ValueError(f"{manifest_path}: expected list field 'link_inputs'")

    source_by_key = {link_source_key(source): source for source in sources}
    if len(source_by_key) != len(sources):
        raise ValueError(f"{manifest_path}: sources contains duplicate paths, cannot validate link_inputs")
    lib_set = set(libs)
    if len(lib_set) != len(libs):
        raise ValueError(f"{manifest_path}: libs contains duplicate entries, cannot validate link_inputs")

    seen_sources: set[str] = set()
    seen_libs: set[str] = set()
    result: list[LinkInput] = []
    for index, item in enumerate(raw_inputs):
        if not isinstance(item, dict):
            raise ValueError(f"{manifest_path}: link_inputs[{index}] must be an object with exactly one of 'source' or 'lib'")
        keys = set(item.keys())
        if keys != {"source"} and keys != {"lib"}:
            raise ValueError(f"{manifest_path}: link_inputs[{index}] must contain exactly one of 'source' or 'lib'")
        if "source" in item:
            value = item["source"]
            if not isinstance(value, str) or not value:
                raise ValueError(f"{manifest_path}: link_inputs[{index}].source must be a non-empty string")
            source_key = link_source_key(repo_path(value))
            if source_key not in source_by_key:
                expected = ", ".join(str(source) for source in sources)
                raise ValueError(f"{manifest_path}: link_inputs[{index}] references unknown source '{value}'; expected one of: {expected}")
            if source_key in seen_sources:
                raise ValueError(f"{manifest_path}: link_inputs[{index}] duplicates source '{value}'")
            seen_sources.add(source_key)
            result.append(LinkInput("source", str(source_by_key[source_key])))
            continue

        value = item["lib"]
        if not isinstance(value, str) or not value:
            raise ValueError(f"{manifest_path}: link_inputs[{index}].lib must be a non-empty string")
        if value not in lib_set:
            expected = ", ".join(libs)
            raise ValueError(f"{manifest_path}: link_inputs[{index}] references unknown lib '{value}'; expected one of: {expected}")
        if value in seen_libs:
            raise ValueError(f"{manifest_path}: link_inputs[{index}] duplicates lib '{value}'")
        seen_libs.add(value)
        result.append(LinkInput("lib", value))

    missing_sources = [str(source) for source in sources if link_source_key(source) not in seen_sources]
    missing_libs = [lib for lib in libs if lib not in seen_libs]
    if missing_sources or missing_libs:
        parts: list[str] = []
        if missing_sources:
            parts.append("missing sources: " + ", ".join(missing_sources))
        if missing_libs:
            parts.append("missing libs: " + ", ".join(missing_libs))
        raise ValueError(f"{manifest_path}: link_inputs must include every source and lib exactly once; {'; '.join(parts)}")

    return tuple(result)


def safe_object_stem(source: Path) -> Path:
    try:
        rel = source.relative_to(REPO_ROOT)
    except ValueError:
        rel = Path(source.name)
    return rel.with_suffix(".obj")


def object_path(config: FinalBuildConfig, paths: BuildPaths, source: Path) -> Path:
    return paths.obj_dir / safe_object_stem(source)


def alias_object_path(paths: BuildPaths, source: Path) -> Path:
    return paths.obj_dir / "coff-alias" / safe_object_stem(source)


def build_paths(config: FinalBuildConfig) -> BuildPaths:
    build_dir = config.build_dir
    message_dir = build_dir / "mc"
    message_rc_path = None
    if config.message_source is not None:
        message_rc_path = message_dir / config.message_source.with_suffix(".rc").name
    return BuildPaths(
        build_dir=build_dir,
        obj_dir=build_dir / "obj",
        logs_dir=build_dir / "logs",
        rsp_dir=build_dir / "rsp",
        exe_path=build_dir / config.output_exe,
        map_path=build_dir / config.output_map,
        resource_path=build_dir / config.resource_output,
        message_dir=message_dir,
        message_rc_path=message_rc_path,
        summary_path=build_dir / "summary.json",
    )


def prepare_build_root(paths: BuildPaths, *, clean: bool, dry_run: bool) -> None:
    """Prepare one validated build root and prove an explicit clean removed it."""

    if paths.build_dir.exists():
        if not paths.build_dir.is_dir():
            raise ValueError(f"--build-dir exists but is not a directory: {paths.build_dir}")
        if any(paths.build_dir.iterdir()) and not clean:
            raise ValueError(
                "--build-dir must be empty before use; select a new root or pass --clean explicitly"
            )
        if any(paths.build_dir.iterdir()) and clean and dry_run:
            raise ValueError(
                "--clean --dry-run will not remove a nonempty --build-dir; use an empty root for dry-run"
            )
    if clean and paths.build_dir.exists() and not dry_run:
        shutil.rmtree(paths.build_dir)
        if paths.build_dir.exists():
            raise ValueError(
                f"--clean did not remove the selected build directory: {paths.build_dir}"
            )


def linked_order_report_dir(config: FinalBuildConfig, paths: BuildPaths) -> Path:
    if not config.diagnostic_only:
        return paths.build_dir

    diagnostic_root = (REPO_ROOT / "build" / "vc5-probes").resolve()
    resolved_build_dir = paths.build_dir.resolve()
    try:
        resolved_build_dir.relative_to(diagnostic_root)
    except ValueError as exc:
        raise ValueError(
            "diagnostic linked-order reports require a profile-isolated "
            f"build/vc5-probes run directory, not {resolved_build_dir}"
        ) from exc
    return resolved_build_dir / "linked-order"


def ensure_inputs_exist(config: FinalBuildConfig) -> list[str]:
    missing: list[str] = []
    required_paths = [config.vc5_env]
    if config.resource_script is not None:
        required_paths.append(config.resource_script)
    if config.message_source is not None:
        required_paths.append(config.message_source)
    for path in required_paths:
        if not path.exists():
            missing.append(str(path))
    for path in config.include_dirs:
        if not path.exists():
            missing.append(str(path))
    for path in config.lib_dirs:
        if not path.exists():
            missing.append(str(path))
    for path in config.sources:
        if not path.exists():
            missing.append(str(path))
    for spec in config.coff_alias_sources:
        if not spec.source.exists():
            missing.append(str(spec.source))
    for lib in config.libs:
        exact_path = exact_lib_path(lib)
        if exact_path is not None:
            if exact_path.suffix.lower() != ".lib" or not exact_path.exists():
                missing.append(f"{lib} exact .lib path")
            continue
        if not any((lib_dir / lib).exists() for lib_dir in config.lib_dirs):
            missing.append(f"{lib} in {', '.join(str(path) for path in config.lib_dirs)}")
    return missing


def exact_lib_path(lib: str) -> Path | None:
    path = Path(lib)
    if path.is_absolute() or path.parent != Path("."):
        return repo_path(lib)
    return None


def link_lib_args(config: FinalBuildConfig) -> list[str]:
    args: list[str] = []
    for lib in config.libs:
        exact_path = exact_lib_path(lib)
        args.append(str(exact_path) if exact_path is not None else lib)
    return args


def link_lib_arg(lib: str) -> str:
    exact_path = exact_lib_path(lib)
    return str(exact_path) if exact_path is not None else lib


def write_response(path: Path, args: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(args) + "\n", encoding="ascii")


def compile_response_args(config: FinalBuildConfig, source: Path, obj_path: Path) -> list[str]:
    args = list(effective_compile_flags(config, source))
    args.extend(f"/D{define}" for define in config.defines)
    args.extend(f"/I{path}" for path in config.include_dirs)
    args.extend(topology_args(config.pch_topology, source))
    args.append(f"/Fo{obj_path}")
    args.append("/c")
    args.append(str(source))
    return [response_line(arg) for arg in args]


def resource_response_args(config: FinalBuildConfig, paths: BuildPaths) -> list[str]:
    resource_script = paths.message_rc_path if config.message_source is not None else config.resource_script
    if resource_script is None:
        raise ValueError("resource script is not configured")
    args = list(config.resource_flags)
    args.extend(f"/d{define}" for define in config.defines)
    args.extend(f"/i{path}" for path in config.include_dirs)
    if config.message_source is not None:
        args.append(f"/i{paths.message_dir}")
    args.append(f"/fo{paths.resource_path}")
    args.append(str(resource_script))
    return [response_line(arg) for arg in args]


def include_trace_response_args(config: FinalBuildConfig, source: Path) -> list[str]:
    args = list(effective_compile_flags(config, source))
    args.extend(f"/D{define}" for define in config.defines)
    args.extend(f"/I{path}" for path in config.include_dirs)
    args.extend(["/E", str(source)])
    return [response_line(arg) for arg in args]


def link_response_args(config: FinalBuildConfig, paths: BuildPaths, objects: list[Path]) -> list[str]:
    return [response_line(arg) for arg in link_raw_args(config, paths, objects)]


def link_raw_args(config: FinalBuildConfig, paths: BuildPaths, objects: list[Path]) -> list[str]:
    args = list(config.link_flags)
    args.append(f"/OUT:{paths.exe_path}")
    args.append(f"/MAP:{paths.map_path}")
    args.extend(f"/LIBPATH:{path}" for path in config.lib_dirs)
    args.extend(link_order_args(config, paths, objects))
    return args


def link_order_args(config: FinalBuildConfig, paths: BuildPaths, objects: list[Path]) -> list[str]:
    aliases_after: dict[str, list[Path]] = {}
    for spec in config.coff_alias_sources:
        aliases_after.setdefault(link_source_key(spec.link_after_source), []).append(
            alias_object_path(paths, spec.source)
        )
    if config.link_inputs is None:
        ordered_objects: list[str] = []
        for source, path in zip(config.sources, objects):
            ordered_objects.append(str(path))
            ordered_objects.extend(
                str(alias_path)
                for alias_path in aliases_after.get(link_source_key(source), ())
            )
        return [*ordered_objects, str(paths.resource_path), *link_lib_args(config)]
    if len(objects) != len(config.sources):
        raise ValueError("object count does not match configured sources")
    object_by_source = {link_source_key(source): obj for source, obj in zip(config.sources, objects)}
    ordered: list[str] = []
    last_source_index = max((index for index, item in enumerate(config.link_inputs) if item.kind == "source"), default=-1)
    if last_source_index == -1:
        ordered.append(str(paths.resource_path))
    for index, item in enumerate(config.link_inputs):
        if item.kind == "source":
            source_key = link_source_key(Path(item.value))
            ordered.append(str(object_by_source[source_key]))
            ordered.extend(str(path) for path in aliases_after.get(source_key, ()))
        elif item.kind == "lib":
            ordered.append(link_lib_arg(item.value))
        else:
            raise ValueError(f"unknown link input kind: {item.kind}")
        if index == last_source_index:
            ordered.append(str(paths.resource_path))
    return ordered


def resolve_link_library_path(config: FinalBuildConfig, value: str) -> Path:
    exact = exact_lib_path(value)
    if exact is not None:
        if not exact.is_file():
            raise ValueError(f"resolved link library is missing: {exact}")
        return exact.resolve()
    for lib_dir in config.lib_dirs:
        candidate = lib_dir / value
        if candidate.is_file():
            return candidate.resolve()
    raise ValueError(f"could not resolve exact linked library path: {value}")


def resolved_full_order_link_inputs(
    config: FinalBuildConfig,
    paths: BuildPaths,
) -> list[dict[str, object]]:
    objects = [object_path(config, paths, source).resolve() for source in config.sources]
    if len(objects) != len(config.sources):
        raise ValueError("object count does not match configured sources")
    rows: list[tuple[str, Path, str]] = []
    aliases_after: dict[str, list[CoffAliasSource]] = {}
    for spec in config.coff_alias_sources:
        aliases_after.setdefault(link_source_key(spec.link_after_source), []).append(spec)
    if config.link_inputs is None:
        for source, path in zip(config.sources, objects):
            rows.append(("object", path, str(source)))
            rows.extend(
                ("object", alias_object_path(paths, spec.source), str(spec.source))
                for spec in aliases_after.get(link_source_key(source), ())
            )
        rows.append(("resource", paths.resource_path.resolve(), config.resource_output))
        rows.extend(
            ("library", resolve_link_library_path(config, lib), lib)
            for lib in config.libs
        )
    else:
        object_by_source = {
            link_source_key(source): path
            for source, path in zip(config.sources, objects)
        }
        last_source_index = max(
            (index for index, item in enumerate(config.link_inputs) if item.kind == "source"),
            default=-1,
        )
        if last_source_index == -1:
            rows.append(("resource", paths.resource_path.resolve(), config.resource_output))
        for index, item in enumerate(config.link_inputs):
            if item.kind == "source":
                source_key = link_source_key(Path(item.value))
                rows.append(
                    (
                        "object",
                        object_by_source[source_key],
                        item.value,
                    )
                )
                rows.extend(
                    ("object", alias_object_path(paths, spec.source), str(spec.source))
                    for spec in aliases_after.get(source_key, ())
                )
            elif item.kind == "lib":
                rows.append(("library", resolve_link_library_path(config, item.value), item.value))
            else:
                raise ValueError(f"unknown link input kind: {item.kind}")
            if index == last_source_index:
                rows.append(("resource", paths.resource_path.resolve(), config.resource_output))
    result: list[dict[str, object]] = []
    for position, (kind, path, configured_value) in enumerate(rows):
        resolved = path.resolve()
        if not resolved.is_file():
            raise ValueError(f"resolved full-order link input is missing: {resolved}")
        result.append(
            {
                "path": str(resolved),
                "size": resolved.stat().st_size,
                "kind": kind,
                "position": position,
                "configured_value": configured_value,
            }
        )
    if {str(row["kind"]) for row in result} != {"object", "resource", "library"}:
        raise ValueError("full-order resolved link inputs must contain object/resource/library rows")
    return result


def _provider_input_rows(
    provider: str,
    inputs: list[dict[str, object]],
) -> list[dict[str, object]]:
    normalized = provider.replace("\\", "/").lower()
    library_prefix = normalized.split(":", 1)[0] if ":" in normalized else ""
    matches: list[dict[str, object]] = []
    for row in inputs:
        path = Path(str(row["path"]))
        candidate = str(path).replace("\\", "/").lower()
        configured = str(row.get("configured_value", "")).replace("\\", "/").lower()
        if row.get("kind") == "library" and library_prefix:
            if Path(library_prefix).name == path.name.lower() or Path(library_prefix).name == Path(configured).name:
                matches.append(row)
        elif row.get("kind") == "object" and (
            candidate.endswith(normalized) or path.name.lower() == Path(normalized).name
        ):
            matches.append(row)
    return matches


def _typed_linked_full_order_fields(
    *,
    check: LinkedOrderCheck,
    target: VerifyTarget,
    interval: LinkedFunctionInterval,
    config: FinalBuildConfig,
    resolved_link_inputs: list[dict[str, object]],
    progress_document: ProgressDocument,
    raw_definitions: tuple[RawObjectFunctionDefinition, ...],
) -> dict[str, Any]:
    block_id, block = resolve_full_order_target_block(
        progress_document,
        binary=target.target_binary,
        target_name=target.name,
        mode="linked",
        retail_start=interval.retail_start or target.retail_start,
        retail_end_exclusive=interval.retail_end_exclusive or target.retail_end_exclusive,
    )
    symbols = progress_document.collection("symbols")
    function_kinds = {"function", "provider-function", "compiler-function"}
    expected_rows = [
        (str(symbol_id), symbols[symbol_id])
        for symbol_id in block.get("contribution_ids", [])
        if isinstance(symbols.get(symbol_id), dict)
        and symbols[symbol_id].get("kind") in function_kinds
    ]
    expected_rows.sort(key=lambda item: (int(str(item[1]["address"]), 0), item[0]))
    contribution_by_address = {item.linked_address: item for item in check.contributions}

    def selected_provider(
        symbol_id: str,
        contribution: LinkedOrderContribution,
    ) -> tuple[dict[str, object], str, bool, bool, str]:
        matched: dict[Path, dict[str, object]] = {}
        for provider in contribution.providers:
            for row in _provider_input_rows(provider, resolved_link_inputs):
                matched[Path(str(row["path"])).resolve()] = row
        if len(matched) != 1:
            raise ValueError(
                f"full-order selected group {symbol_id} resolves to {len(matched)} exact link inputs"
            )
        provider_path, provider_row = next(iter(matched.items()))
        provider_kind = str(provider_row["kind"])
        if provider_kind == "object":
            definitions = [
                definition
                for definition in raw_definitions
                if (
                    (Path(definition.object_path).resolve() if Path(definition.object_path).is_absolute()
                     else repo_path(definition.object_path).resolve())
                    == provider_path
                    and set(definition.symbols).intersection(contribution.symbols)
                )
            ]
            if not definitions:
                raise ValueError(
                    f"full-order selected source group {symbol_id} has no exact raw COFF definition"
                )
            comdat = any(definition.comdat for definition in definitions)
            weak = any(definition.weak for definition in definitions)
            selection_kind = "source-object"
            provenance = "raw-object-coff-inventory"
        elif provider_kind == "library":
            raise ValueError(
                f"full-order selected provider-library group {symbol_id} lacks "
                "archive-member COMDAT evidence"
            )
        else:
            raise ValueError(f"full-order selected group {symbol_id} resolves to {provider_kind}")
        selected = {
            "kind": provider_kind,
            "classification": selection_kind,
            "path": str(provider_path),
            "size": int(provider_row["size"]),
        }
        return selected, selection_kind, comdat, weak, provenance

    selected_groups: list[dict[str, object]] = []
    comdat_icf_inventory: list[dict[str, object]] = []
    for symbol_id, symbol in expected_rows:
        address = int(str(symbol["address"]), 0)
        contribution = contribution_by_address.get(address)
        if contribution is None:
            raise ValueError(f"full-order selected group is absent from MAP check: {symbol_id}")
        provider, selection_kind, comdat, weak, selection_provenance = selected_provider(
            symbol_id,
            contribution,
        )
        icf = isinstance(symbol.get("icf_address_group"), dict)
        selection = {
            "selected": True,
            "comdat": comdat,
            "icf": icf,
            "weak": weak,
            "provenance": selection_provenance,
        }
        selected_groups.append(
            {
                "tracker_identity": symbol_id,
                "linked_address": f"0x{address:x}",
                "linked_rva": f"0x{contribution.linked_rva:x}",
                "decorated_identities": list(contribution.symbols),
                "selected_provider": provider,
                "selection_kind": selection_kind,
                "selection": selection,
            }
        )
        comdat_icf_inventory.append(
            {"tracker_identity": symbol_id, **selection}
        )

    all_function_rows = [
        (str(symbol_id), symbol)
        for symbol_id, symbol in symbols.items()
        if isinstance(symbol, dict)
        and symbol.get("binary") == target.target_binary
        and symbol.get("kind") in function_kinds
    ]
    all_function_rows.sort(key=lambda item: (int(str(item[1]["address"]), 0), item[0]))
    start = int(str(block["start"]), 0)
    end = int(str(block["end_exclusive"]), 0)
    predecessor_rows = [row for row in all_function_rows if int(str(row[1]["address"]), 0) < start]
    successor_row = next(
        (row for row in all_function_rows if int(str(row[1]["address"]), 0) >= end),
        None,
    )

    def sentinel(row: tuple[str, dict[str, Any]] | None) -> dict[str, str] | None:
        if row is None:
            return None
        symbol_id, symbol = row
        address = int(str(symbol["address"]), 0)
        contribution = contribution_by_address.get(address)
        if contribution is None:
            raise ValueError(f"full-order boundary sentinel is absent from MAP check: {symbol_id}")
        return {
            "tracker_identity": symbol_id,
            "linked_address": f"0x{address:x}",
            "linked_rva": f"0x{contribution.linked_rva:x}",
        }

    semantic_fields = {
        "full_scope_evaluated": True,
        "diagnostic_mode": False,
        "block_id": block_id,
        "normal_link_profile": True,
        "linker_flags": list(config.link_flags),
        "resolved_link_inputs_complete": True,
        "provider_inventory_complete": True,
        "selected_group_inventory_complete": True,
        "selected_groups": selected_groups,
        "comdat_icf_inventory_complete": True,
        "comdat_icf_inventory": comdat_icf_inventory,
        "provider_acceptance_inferred": False,
        "unclassified_selected_extras": [],
        "boundary_sentinels": {
            "predecessor": sentinel(predecessor_rows[-1] if predecessor_rows else None),
            "successor": sentinel(successor_row),
        },
    }
    return semantic_fields


def make_compile_command(config: FinalBuildConfig, paths: BuildPaths, source: Path) -> tuple[CommandSpec, Path]:
    obj = object_path(config, paths, source)
    obj.parent.mkdir(parents=True, exist_ok=True)
    if config.pch_topology is not None:
        config.pch_topology.pch_path.parent.mkdir(parents=True, exist_ok=True)
    rsp = paths.rsp_dir / "compile" / safe_object_stem(source).with_suffix(".rsp")
    write_response(rsp, compile_response_args(config, source, obj))
    name = "compile:" + display_path(source)
    stem = str(safe_object_stem(source)).replace("\\", "__").replace("/", "__").replace(":", "")
    return (
        CommandSpec(
            name=name,
            command=f"call {quote_cmd_arg(config.vc5_env)} && cl @{quote_cmd_arg(rsp)}",
            cwd=REPO_ROOT,
            stdout_log=paths.logs_dir / f"{stem}.out.log",
            stderr_log=paths.logs_dir / f"{stem}.err.log",
        ),
        obj,
    )


def make_resource_command(config: FinalBuildConfig, paths: BuildPaths) -> CommandSpec:
    rsp = paths.rsp_dir / "resource.rsp"
    args = resource_response_args(config, paths)
    write_response(rsp, args)
    return CommandSpec(
        name="resource",
        command=f"call {quote_cmd_arg(config.vc5_env)} && rc {' '.join(args)}",
        cwd=REPO_ROOT,
        stdout_log=paths.logs_dir / "resource.out.log",
        stderr_log=paths.logs_dir / "resource.err.log",
    )


def make_include_trace_command(config: FinalBuildConfig, paths: BuildPaths, source: Path) -> CommandSpec:
    rsp = paths.rsp_dir / "include-trace" / safe_object_stem(source).with_suffix(".rsp")
    write_response(rsp, include_trace_response_args(config, source))
    stem = str(safe_object_stem(source)).replace("\\", "__").replace("/", "__").replace(":", "")
    return CommandSpec(
        name="include-trace:" + display_path(source),
        command=f"call {quote_cmd_arg(config.vc5_env)} && cl @{quote_cmd_arg(rsp)}",
        cwd=REPO_ROOT,
        stdout_log=paths.logs_dir / f"{stem}.include-trace.out.log",
        stderr_log=paths.logs_dir / f"{stem}.include-trace.err.log",
    )


def make_message_command(config: FinalBuildConfig, paths: BuildPaths) -> CommandSpec | None:
    if config.message_source is None:
        return None
    paths.message_dir.mkdir(parents=True, exist_ok=True)
    args = [
        "-h",
        str(paths.message_dir),
        "-r",
        str(paths.message_dir),
        str(config.message_source),
    ]
    return CommandSpec(
        name="message",
        command=f"call {quote_cmd_arg(config.vc5_env)} && mc {' '.join(quote_cmd_arg(arg) for arg in args)}",
        cwd=REPO_ROOT,
        stdout_log=paths.logs_dir / "message.out.log",
        stderr_log=paths.logs_dir / "message.err.log",
    )


def make_coff_alias_command(
    paths: BuildPaths,
    spec: CoffAliasSource,
    *,
    llvm_ml: Path,
) -> CommandSpec:
    output = alias_object_path(paths, spec.source)
    output.parent.mkdir(parents=True, exist_ok=True)
    stem = str(safe_object_stem(spec.source)).replace("\\", "__").replace("/", "__").replace(":", "")
    command = " ".join(
        (
            quote_cmd_arg(llvm_ml),
            "/c",
            f"/Fo{quote_cmd_arg(output)}",
            quote_cmd_arg(spec.source),
        )
    )
    return CommandSpec(
        name="coff-alias:" + display_path(spec.source),
        command=command,
        cwd=REPO_ROOT,
        stdout_log=paths.logs_dir / f"{stem}.coff-alias.out.log",
        stderr_log=paths.logs_dir / f"{stem}.coff-alias.err.log",
    )


def make_link_command(
    config: FinalBuildConfig,
    paths: BuildPaths,
    objects: list[Path],
) -> CommandSpec:
    rsp = paths.rsp_dir / "link.rsp"
    write_response(rsp, link_response_args(config, paths, objects))
    command = f"call {quote_cmd_arg(config.vc5_env)} && link @{quote_cmd_arg(rsp)}"
    return CommandSpec(
        name="link",
        command=command,
        cwd=REPO_ROOT,
        stdout_log=paths.logs_dir / "link.out.log",
        stderr_log=paths.logs_dir / "link.err.log",
    )


def run_command(spec: CommandSpec) -> CommandResult:
    spec.stdout_log.parent.mkdir(parents=True, exist_ok=True)
    spec.stderr_log.parent.mkdir(parents=True, exist_ok=True)
    with spec.stdout_log.open("w", encoding="utf-8", errors="replace") as stdout, spec.stderr_log.open(
        "w", encoding="utf-8", errors="replace"
    ) as stderr:
        completed = run_cmd_script(
            spec.command,
            cwd=spec.cwd,
            stdout=stdout,
            stderr=stderr,
            script_name=str(spec.stdout_log.parent / f"_{spec.name.replace(':', '_').replace('/', '_')}.cmd"),
        )
    return CommandResult(
        name=spec.name,
        returncode=completed.returncode,
        stdout_log=spec.stdout_log,
        stderr_log=spec.stderr_log,
    )


def read_log_tail(path: Path, max_lines: int = 8) -> list[str]:
    if not path.exists():
        return []
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    return lines[-max_lines:]


def print_result(result: CommandResult) -> None:
    status = "OK" if result.returncode == 0 else f"FAIL({result.returncode})"
    print(f"{status}: {result.name}")
    if result.returncode != 0:
        for path in (result.stdout_log, result.stderr_log):
            tail = read_log_tail(path)
            if tail:
                print(f"  {path}:")
                for line in tail:
                    print(f"    {line}")


def playtest_deployment_eligible(
    config: FinalBuildConfig,
    *,
    dry_run: bool,
    compile_only: bool,
    linked_order_only: bool,
    diagnostic_isolation_applied: bool,
) -> bool:
    """Return whether this invocation may update the canonical play-test copy."""

    return (
        not dry_run
        and not compile_only
        and not linked_order_only
        and not diagnostic_isolation_applied
        and config.manifest_path.resolve() == DEFAULT_MANIFEST.resolve()
        and config.output_exe == "Recoil.exe"
        and config.playtest_output_exe is not None
        and config.playtest_output_exe.resolve() == DEFAULT_PLAYTEST_OUTPUT.resolve()
        and not config.diagnostic_only
        and not config.diagnostic_kind
        and not config.compile_profile
        and not config.link_profile
        and not config.library_profile
    )


def deploy_playtest_candidate(candidate: Path, destination: Path) -> dict[str, object]:
    """Best-effort atomic play-test deployment; failures never gate a build."""

    resolved_destination = destination.resolve()
    result: dict[str, object] = {
        "attempted": True,
        "updated": False,
        "destination": str(resolved_destination),
        "error": None,
    }
    if not destination.parent.is_dir():
        result["error"] = f"play-test directory is missing: {destination.parent.resolve()}"
        print(
            "WARNING: play-test deployment was not updated (non-gating): "
            f"{result['error']}",
            file=sys.stderr,
        )
        return result

    temporary_path: Path | None = None
    try:
        descriptor, temporary_name = tempfile.mkstemp(
            prefix=f".{destination.name}.",
            suffix=".tmp",
            dir=destination.parent,
        )
        temporary_path = Path(temporary_name)
        os.close(descriptor)
        shutil.copyfile(candidate, temporary_path)
        os.replace(temporary_path, destination)
        temporary_path = None
        result["updated"] = True
    except OSError as exc:
        result["error"] = f"{type(exc).__name__}: {exc}"
    finally:
        if temporary_path is not None:
            try:
                temporary_path.unlink(missing_ok=True)
            except OSError as cleanup_exc:
                cleanup_error = f"{type(cleanup_exc).__name__}: {cleanup_exc}"
                if result["error"]:
                    result["error"] = f"{result['error']}; temporary cleanup failed: {cleanup_error}"
                else:
                    result["error"] = f"temporary cleanup failed: {cleanup_error}"

    if result["updated"]:
        print(f"Play-test binary updated (play-test only): {resolved_destination}")
    else:
        print(
            "WARNING: play-test deployment was not updated (non-gating); "
            f"existing target preserved: {resolved_destination}; error: {result['error']}",
            file=sys.stderr,
        )
    return result


def write_summary(
    paths: BuildPaths,
    results: list[CommandResult],
    *,
    dry_run: bool,
    acceptance: dict[str, object] | None = None,
    config: FinalBuildConfig | None = None,
) -> None:
    data = {
        "dry_run": dry_run,
        "results": [
            {
                "name": result.name,
                "returncode": result.returncode,
                "stdout_log": str(result.stdout_log.relative_to(REPO_ROOT)),
                "stderr_log": str(result.stderr_log.relative_to(REPO_ROOT)),
            }
            for result in results
        ],
    }
    if acceptance is not None:
        data.update(acceptance)
    if config is not None and data.get("kind") in {"final-build", "final-build-diagnostic", "playground-build"}:
        compile_results = [
            result for result in results
            if result.name.startswith(("include-trace:", "compile:"))
        ]
        alias_results = [
            result for result in results
            if result.name.startswith("coff-alias:")
        ]
        resource_results = [result for result in results if result.name == "resource"]
        link_results = [result for result in results if result.name == "link"]
        compile_succeeded = (
            bool(compile_results)
            and all(result.returncode == 0 for result in compile_results)
        ) or (
            not compile_results
            and all(object_path(config, paths, source).is_file() for source in config.sources)
        )
        coff_alias_sources_succeeded = (
            not config.coff_alias_sources
            or (
                len(alias_results) == len(config.coff_alias_sources)
                and all(result.returncode == 0 for result in alias_results)
                and all(
                    alias_object_path(paths, spec.source).is_file()
                    for spec in config.coff_alias_sources
                )
            )
        )
        resource_succeeded = (
            bool(resource_results)
            and all(result.returncode == 0 for result in resource_results)
        ) or (not resource_results and paths.resource_path.is_file())
        link_succeeded = bool(link_results) and all(
            result.returncode == 0 for result in link_results
        )
        candidate_available = (
            link_succeeded and paths.exe_path.is_file() and paths.map_path.is_file()
        )
        is_normal_final_build = data.get("kind") == "final-build"
        linked_order_passed = (
            bool(data.get("required_order_targets_passed"))
            if is_normal_final_build
            else bool(data.get("selected_diagnostic_targets_passed"))
        )
        data.update(
            {
                "compile_succeeded": compile_succeeded,
                "coff_alias_sources_succeeded": coff_alias_sources_succeeded,
                "coff_alias_sources": [
                    {
                        **validate_alias_source(spec),
                        "object": str(alias_object_path(paths, spec.source).resolve()),
                        "link_after_source": report_path_key(spec.link_after_source),
                    }
                    for spec in config.coff_alias_sources
                ],
                "resource_succeeded": resource_succeeded,
                "link_succeeded": link_succeeded,
                "candidate_available": candidate_available,
                "linked_order_passed": linked_order_passed,
                "authored_byte_eligible": (
                    is_normal_final_build
                    and compile_succeeded
                    and coff_alias_sources_succeeded
                    and link_succeeded
                    and candidate_available
                ),
            }
        )
        # A linked-order failure still owns a complete live compile/link observation.
        data.setdefault("compiler_profile", "VC5SP3")
        data.setdefault("config_path", str(config.manifest_path.resolve()))
        data.setdefault("compiler_env_path", str(config.vc5_env.resolve()))
        data.setdefault("map_path", str(paths.map_path.resolve()))
        data.setdefault("resource_path", str(paths.resource_path.resolve()))
        data.setdefault("candidate_path", str(paths.exe_path.resolve()))
    paths.summary_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")


def canonical_include_trace_for_results(
    config: FinalBuildConfig,
    results: list[CommandResult],
) -> dict[str, object] | None:
    if config.canonical_mfc is None:
        return None
    logs = [
        path
        for result in results
        if result.name.startswith("include-trace:")
        for path in (result.stdout_log, result.stderr_log)
    ]
    return include_trace_report(config.canonical_mfc, logs)


def compile_profile_rows(config: FinalBuildConfig) -> list[dict[str, object]]:
    mappings = dict(config.source_compile_profiles)
    return [
        {
            "source": report_path_key(source),
            "profile": mappings.get(canonical_source_key(source), config.compile_profile or "manifest-default"),
            "effective_flags": list(effective_compile_flags(config, source)),
        }
        for source in config.sources
    ]


def write_dry_run_summary(paths: BuildPaths, results: list[CommandResult]) -> None:
    dry_run_summary_path = paths.build_dir / "dry_run_summary.json"
    data = {
        "dry_run": True,
        "results": [
            {
                "name": result.name,
                "returncode": result.returncode,
                "stdout_log": str(result.stdout_log.relative_to(REPO_ROOT)),
                "stderr_log": str(result.stderr_log.relative_to(REPO_ROOT)),
            }
            for result in results
        ],
    }
    dry_run_summary_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")


def print_dry_run(commands: list[CommandSpec]) -> None:
    for spec in commands:
        print(f"{spec.name}:")
        print(f"  cwd: {spec.cwd}")
        print(f"  cmd: {spec.command}")
        print(f"  stdout: {spec.stdout_log}")
        print(f"  stderr: {spec.stderr_log}")


def ledger_required_order_targets(config: FinalBuildConfig, *, order_scope: str = "full") -> tuple[str, ...]:
    """Derive default Recoil linked-order gates from unified progress."""
    if config.manifest_path.resolve() != DEFAULT_MANIFEST.resolve():
        return config.required_order_targets
    try:
        document = ProgressDocument.load(DEFAULT_PROGRESS)
    except (OSError, ValueError) as exc:
        raise ValueError(f"cannot derive final order targets from {DEFAULT_PROGRESS}: {exc}") from exc
    targets: list[str] = []
    current_added = False
    for _, block in document._blocks_for_binary("recoil"):
        if not isinstance(block, dict):
            continue
        accepted = (
            document._block_authored_order_accepted(block)
            if order_scope == "authored"
            else document._block_full_order_accepted(block)
        )
        is_current = not accepted and not current_added
        if is_current:
            current_added = True
        if accepted or is_current:
            order_targets = block.get("order_targets", {})
            target = str(order_targets.get("linked", "") if isinstance(order_targets, dict) else "")
            if target:
                targets.append(target)
        if is_current:
            break
    return tuple(dict.fromkeys(targets))


def report_path_key(path: Path) -> str:
    absolute = path if path.is_absolute() else REPO_ROOT / path
    try:
        lexical = absolute.absolute().relative_to(REPO_ROOT.absolute()).as_posix()
    except ValueError:
        return str(path.resolve())
    first = lexical.split("/", 1)[0]
    if first == "build":
        return normalize_generated_repository_path(
            lexical,
            allowed_roots=("build",),
            context="final-build generated report path",
        ).logical_path
    if first in {".codex", "docs", "src", "tests", "tools"}:
        return _authored_repository_path(
            absolute,
            context="final-build authored report path",
        )
    return str(path.resolve())


def required_authored_linked_functions(
    document: ProgressDocument,
    *,
    static_object_scopes: dict[str, set[str]] | None = None,
) -> tuple[VerifyFunction, ...]:
    """Select presence obligations from the accepted order census, not the MAP.

    This is deliberately independent of the serial linked-order cursor. A
    missing/stale registration or an uncovered census member blocks deployment.
    """
    from _recoil.lib.verification_targets import vc5_target_registration

    slices = document.authored_call_contract_slices("recoil")
    addresses = {address for row in slices for address in row["addresses"]}
    target_ids = {target for row in slices for target in row["target_ids"]}
    if not addresses or not target_ids:
        raise ValueError("linked presence requires a nonempty accepted authored census")
    inventory = load_repository_path_inventory(REPO_ROOT)
    selected: dict[tuple[str, str | None, str | None], VerifyFunction] = {}
    for target_id in sorted(target_ids):
        registered = document.collection("verification_targets")[target_id]
        registration = registered["registration"]
        path = resolve_repository_file(
            registration["manifest_path"],
            repository_root=REPO_ROOT,
            inventory=inventory,
            context=f"linked presence target {target_id}",
            allowed_suffixes={".json"},
        ).physical_path
        current_id, current = vc5_target_registration(path)
        if current_id != target_id or current["registration"] != registration:
            raise ValueError(f"linked presence target registration is stale: {target_id}")
        target = load_vc5_verify_manifest(path)
        if static_object_scopes is not None:
            for unit in target.translation_unit_function_order:
                for function in unit.functions:
                    if function.address in addresses:
                        static_object_scopes.setdefault(function.address, set()).add(
                            Path(unit.source_from).with_suffix(".obj").name
                        )
        functions = [*target.functions]
        functions.extend(f for entry in target.translation_unit_function_order for f in entry.functions)
        functions.extend(f for interval in target.linked_function_intervals for f in interval.functions)
        for function in functions:
            # Census membership already defines the obligation. A manifest's
            # compiler-lifecycle classification must not discard a selected
            # identity; required presence and exact selector resolution still
            # apply to every member.
            if function.address not in addresses:
                continue
            if not function.required_presence:
                raise ValueError(f"authored census member lacks required presence: {function.address}")
            key = (function.address, function.symbol, function.symbol_regex)
            selected[key] = function
    uncovered = addresses - {function.address for function in selected.values()}
    if uncovered:
        raise ValueError(f"linked presence has uncovered authored identities: {sorted(uncovered)}")
    return tuple(sorted(selected.values(), key=lambda f: (f.address, f.symbol or f.symbol_regex or "")))


def authored_linked_presence_report(
    functions: tuple[VerifyFunction, ...], parsed_map: ParsedLinkMap,
    *,
    static_object_scopes: Mapping[str, set[str]] | None = None,
) -> dict[str, Any]:
    """Check identity presence only; never require retail RVA, order, or bytes.

    COFF/ICF aliases named in the map may share an address. Two different
    addresses for one selector are ambiguous, not a successful presence proof.
    """
    if not functions:
        raise ValueError("linked presence requires nonempty function obligations")
    groups = linked_function_groups(parsed_map)
    missing: list[dict[str, str]] = []
    for function in functions:
        try:
            scoped_groups = groups
            expected_objects = (static_object_scopes or {}).get(function.address)
            if expected_objects:
                # Public identities remain global (including ICF aliases).
                # File-local generated names require their registered TU.
                scoped_groups = {
                    address: tuple(item for item in group
                                   if item.source != "Static symbols"
                                   or item.object in expected_objects)
                    for address, group in groups.items()
                }
            resolve_linked_function_group(function, scoped_groups)
        except ValueError as exc:
            missing.append({
                "symbol_id": f"recoil:function:{function.address}",
                "selector": function.symbol_regex or function.symbol or "",
                "detail": str(exc),
            })
    return {
        "kind": "required-authored-linked-presence",
        "passed": not missing,
        "required_selector_count": len(functions),
        "required_body_count": len({f.address for f in functions}),
        "divergences": missing,
        "accepts_order_or_bytes": False,
    }


def order_report_rows(
    target_names: tuple[str, ...],
    target_files: tuple[Path, ...],
    report_dir: Path,
    *,
    order_scope: str,
) -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    for target in load_linked_order_targets(target_names, target_files):
        for interval in target.linked_function_intervals:
            effective_scope = order_scope or interval.order_scope
            scoped_name = (
                f"{target.name}_{interval.name}_authored"
                if effective_scope == "authored"
                else f"{target.name}_{interval.name}"
            )
            safe_name = re.sub(r"[^A-Za-z0-9_.-]+", "_", scoped_name)
            path = report_dir / f"linked_order_{safe_name}.json"
            if not path.is_file():
                raise ValueError(f"required linked order report is missing: {path}")
            rows.append(
                {
                    "target": target.name,
                    "interval": interval.name,
                    "path": str(path.resolve()),
                }
            )
    return rows


def validate_playground_build_request(
    config: FinalBuildConfig, *, clean: bool, compile_only: bool,
    linked_order_only: bool, linkability_only: bool, keep_going: bool,
    order_targets: tuple[str, ...], progress_path: Path,
) -> None:
    """Test deployment never substitutes for a reconstruction acceptance run."""
    if clean or compile_only or linked_order_only or linkability_only or keep_going or order_targets:
        raise ValueError("--playground-only rejects clean, partial, diagnostic, keep-going, and order-target modes")
    if not playtest_deployment_eligible(
        config, dry_run=False, compile_only=False, linked_order_only=False,
        diagnostic_isolation_applied=False,
    ) or config.output_exe != "Recoil.exe" or progress_path.resolve() != DEFAULT_PROGRESS.resolve():
        raise ValueError("--playground-only requires the canonical Recoil profile and progress authority")
    root = config.build_dir.resolve()
    live_root = (REPO_ROOT / "build" / "live-validation").resolve()
    if not config.build_dir_explicit or root == live_root or not root.is_relative_to(live_root) or root.exists():
        raise ValueError("--playground-only requires a fresh absent --build-dir below build/live-validation")


def required_authored_presence_at_map(map_path: Path, progress_path: Path) -> dict[str, object]:
    """Shared fail-closed presence gate for normal and playground builds."""
    try:
        static_object_scopes: dict[str, set[str]] = {}
        return authored_linked_presence_report(
            required_authored_linked_functions(
                ProgressDocument.load(progress_path), static_object_scopes=static_object_scopes,
            ),
            parse_link_map(map_path), static_object_scopes=static_object_scopes,
        )
    except (OSError, ValueError) as exc:
        return {"passed": False, "error": str(exc)}


def finish_playground_build(
    config: FinalBuildConfig, paths: BuildPaths, results: list[CommandResult], *,
    progress_path: Path, required_order_targets: tuple[str, ...],
    canonical_include_trace: dict[str, object] | None,
) -> int:
    presence = required_authored_presence_at_map(paths.map_path, progress_path)
    deployment = {
        "attempted": False, "updated": False,
        "destination": str(config.playtest_output_exe.resolve()), "error": None,
    }
    if presence["passed"]:
        deployment = deploy_playtest_candidate(paths.exe_path, config.playtest_output_exe)
    success = bool(presence["passed"] and deployment["updated"])
    write_summary(paths, results, dry_run=False, config=config, acceptance={
        "report_version": 1, "kind": "playground-build", "success": success,
        "failure_stage": None if success else ("deployment" if presence["passed"] else "linked-presence"),
        "binary": "recoil", "validation_mode": "fresh-canonical-playground-only",
        "fresh_build": True, "reuse": False,
        "canonical_mfc_include_trace": canonical_include_trace,
        "compile_profiles": compile_profile_rows(config),
        "required_order_targets": list(required_order_targets), "effective_order_targets": [],
        "order_reports": [], "linked_order_evaluation_suppressed": True,
        "required_order_targets_evaluation_state": "not-evaluated-in-playground-only-mode",
        "required_order_targets_passed": False,
        "linked_presence": presence, "playtest_deploy": deployment,
        "candidate_expected_truth": False, "accepts_order": False,
        "accepts_bytes": False, "accepts_final_image": False,
        "final_image_validation": "not-run", "diagnostic_only": True,
    })
    if not presence["passed"]:
        print("Required authored linked presence failed; playground executable was not replaced.")
        print(json.dumps(presence, indent=2))
    print(f"Playground build candidate: {paths.exe_path}")
    return 0 if success else 1


def run_build(
    config: FinalBuildConfig,
    *,
    clean: bool,
    dry_run: bool,
    compile_only: bool,
    keep_going: bool,
    order_targets: tuple[str, ...] = (),
    order_scope: str = "full",
    compile_only_skip_linked_order: bool = False,
    required_order_targets_override: tuple[str, ...] | None = None,
    linked_order_only: bool = False,
    linkability_only: bool = False,
    playground_only: bool = False,
    progress_path: Path = DEFAULT_PROGRESS,
) -> int:
    order_target_files: tuple[Path, ...] = ()
    if playground_only:
        validate_playground_build_request(
            config, clean=clean, compile_only=compile_only,
            linked_order_only=linked_order_only, linkability_only=linkability_only,
            keep_going=keep_going, order_targets=order_targets, progress_path=progress_path,
        )
    required_order_targets = (
        ledger_required_order_targets(config, order_scope=order_scope)
        if required_order_targets_override is None
        else required_order_targets_override
    )
    if compile_only_skip_linked_order and not compile_only:
        raise ValueError("--compile-only-skip-linked-order requires --compile-only")
    if linkability_only:
        if compile_only or linked_order_only:
            raise ValueError(
                "--linkability-only requires a complete link and cannot be combined "
                "with --compile-only or --linked-order-only"
            )
        if order_targets or order_target_files:
            raise ValueError(
                "--linkability-only does not accept linked-order targets"
            )
        if not clean or not config.build_dir_explicit:
            raise ValueError(
                "--linkability-only requires --clean and one explicit isolated --build-dir"
            )
        if (
            config.manifest_path.resolve() != DEFAULT_MANIFEST.resolve()
            or config.output_exe != "Recoil.exe"
            or config.diagnostic_only
            or config.diagnostic_kind
            or config.compile_profile
            or config.link_profile
            or config.library_profile
        ):
            raise ValueError(
                "--linkability-only requires the unmodified canonical Recoil final-build profile"
            )
        if keep_going:
            raise ValueError("--linkability-only is fail-fast and rejects --keep-going")
    if linked_order_only:
        if len(order_targets) != 1:
            raise ValueError("linked-order-only validation requires exactly one --order-target")
        if compile_only:
            raise ValueError("linked-order-only validation requires a linked MAP and cannot use --compile-only")
    if compile_only and (required_order_targets or order_targets or order_target_files) and not compile_only_skip_linked_order:
        raise ValueError(
            "--compile-only cannot be used with required/selected order targets because linked order requires Recoil.map; "
            "use --compile-only-skip-linked-order for a diagnostic compile sweep that produces no order evidence"
        )
    paths = build_paths(config)
    order_report_dir = linked_order_report_dir(config, paths)
    expected_binary = "recoil" if config.output_exe.lower() == "recoil.exe" else "messages"
    if compile_only_skip_linked_order or linkability_only or playground_only:
        routing = OrderTargetRouting(
            diagnostic_isolation_applied=False,
            explicit_target_ids=order_targets,
            effective_target_ids=(),
            mandatory_target_ids=required_order_targets,
        )
        effective_order_targets = ()
    else:
        external_targets = load_linked_order_targets((), order_target_files)
        validate_external_order_targets(
            external_targets,
            expected_binary=expected_binary,
            order_scope=order_scope,
            configured_sources=config.sources,
        )
        # Load both sides before routing so isolation cannot hide a malformed
        # selected/base manifest or a malformed mandatory ledger target.
        load_linked_order_targets(required_order_targets)
        explicit_target_manifests = load_linked_order_targets(order_targets, order_target_files)
        routing = resolve_order_target_routing(
            config=config,
            order_scope=order_scope,
            mandatory_target_ids=required_order_targets,
            explicit_targets=explicit_target_manifests,
        )
        effective_order_targets = (
            order_targets
            if routing.diagnostic_isolation_applied
            else tuple(dict.fromkeys((*required_order_targets, *order_targets)))
        )
    suppressed_order_targets = (
        tuple(dict.fromkeys((*required_order_targets, *order_targets)))
        if compile_only_skip_linked_order
        else ()
    )
    if compile_only_skip_linked_order:
        print(
            "DIAGNOSTIC ONLY: linked-order target evaluation is suppressed for this compile-only sweep; "
            "no Recoil.map or linked-order report will be produced, and this run cannot satisfy any "
            "linked-order or final-build acceptance gate."
        )
        effective_order_targets = ()
        order_target_files = ()
    if linkability_only:
        print(
            "LINKABILITY ONLY: compiling and linking the canonical whole program once; "
            "linked-order, byte, final-image, and play-test deployment acceptance are suppressed."
        )
    if playground_only:
        print(
            "PLAYGROUND ONLY: fresh canonical compile/resource/link plus required authored linked presence; "
            "linked order, bytes, aliases, and final-image facts are not accepted."
        )
    order_target_manifests = load_linked_order_targets(effective_order_targets, order_target_files)
    if playground_only:
        paths.build_dir.mkdir(parents=True, exist_ok=False)
    if config.build_dir_explicit:
        prepare_build_root(paths, clean=clean, dry_run=dry_run)
    elif clean and paths.build_dir.exists() and not dry_run:
        shutil.rmtree(paths.build_dir)
    paths.obj_dir.mkdir(parents=True, exist_ok=True)
    paths.logs_dir.mkdir(parents=True, exist_ok=True)
    paths.rsp_dir.mkdir(parents=True, exist_ok=True)

    missing = ensure_inputs_exist(config)
    if missing:
        print("Missing build inputs:", file=sys.stderr)
        for item in missing:
            print(f"- {item}", file=sys.stderr)
        if config.diagnostic_only:
            write_summary(paths, [], dry_run=False, config=config)
        return 2

    compile_commands: list[CommandSpec] = []
    include_trace_commands: list[CommandSpec] = []
    objects: list[Path] = []
    for source in config.sources:
        if config.canonical_mfc is not None:
            include_trace_commands.append(make_include_trace_command(config, paths, source))
        command, obj = make_compile_command(config, paths, source)
        compile_commands.append(command)
        objects.append(obj)
    alias_commands: list[tuple[CoffAliasSource, CommandSpec]] = []
    if config.coff_alias_sources:
        llvm_ml = resolve_llvm_ml()
        alias_commands = [
            (
                spec,
                make_coff_alias_command(paths, spec, llvm_ml=llvm_ml),
            )
            for spec in config.coff_alias_sources
        ]
    message_command = make_message_command(config, paths)
    resource_command = make_resource_command(config, paths)
    link_command = make_link_command(config, paths, objects)
    all_commands = include_trace_commands + compile_commands + [
        command for _spec, command in alias_commands
    ] + (
        [message_command] if message_command is not None else []
    ) + [resource_command]
    if not compile_only:
        all_commands.append(link_command)

    if dry_run:
        print(f"VC5SP3 final build manifest: {config.manifest_path}")
        print(f"Build directory: {paths.build_dir}")
        print(f"Linked order scope: {order_scope}")
        for target in order_target_manifests:
            print(f"Linked order target: {target.name} ({len(target.linked_function_intervals)} interval(s))")
        print_dry_run(all_commands)
        write_dry_run_summary(paths, [])
        return 0

    results: list[CommandResult] = []
    failed = False
    canonical_include_trace = None
    for command in include_trace_commands:
        result = run_command(command)
        results.append(result)
        print_result(result)
        if result.returncode != 0:
            write_summary(paths, results, dry_run=False, config=config)
            return result.returncode
    canonical_include_trace = canonical_include_trace_for_results(config, results)
    if canonical_include_trace is not None and not canonical_include_trace["ok"]:
        write_summary(
            paths,
            results,
            dry_run=False,
            config=config,
            acceptance={
                "diagnostic_only": True,
                "canonical_mfc_include_trace": canonical_include_trace,
            },
        )
        return 1
    for command in compile_commands:
        result = run_command(command)
        results.append(result)
        print_result(result)
        if result.returncode != 0:
            failed = True
            if not keep_going:
                write_summary(paths, results, dry_run=False, config=config)
                return result.returncode

    for alias_spec, command in alias_commands:
        result = run_command(command)
        results.append(result)
        print_result(result)
        if result.returncode != 0:
            write_summary(paths, results, dry_run=False, config=config)
            return result.returncode
        try:
            validate_alias_source(alias_spec)
            validate_alias_object(
                alias_object_path(paths, alias_spec.source),
                alias_spec,
            )
        except (OSError, ValueError) as exc:
            print(f"FAIL: {command.name}: {exc}", file=sys.stderr)
            write_summary(paths, results, dry_run=False, config=config)
            return 1

    if message_command is not None:
        message_result = run_command(message_command)
        results.append(message_result)
        print_result(message_result)
        if message_result.returncode != 0:
            failed = True
            if not keep_going:
                write_summary(paths, results, dry_run=False, config=config)
                return message_result.returncode

    resource_result = run_command(resource_command)
    results.append(resource_result)
    print_result(resource_result)
    if resource_result.returncode != 0:
        failed = True
        if not keep_going:
            write_summary(paths, results, dry_run=False, config=config)
            return resource_result.returncode

    if compile_only:
        write_summary(
            paths,
            results,
            dry_run=False,
            config=config,
            acceptance={
                "kind": "compile-only-diagnostic",
                "success": not failed,
                "compile_only": True,
                "linked_order_evaluation_suppressed": compile_only_skip_linked_order,
                "suppressed_required_order_targets": [
                    target for target in required_order_targets if target in suppressed_order_targets
                ],
                "suppressed_selected_order_targets": [
                    target for target in order_targets if target in suppressed_order_targets
                ],
                "diagnostic_only": True,
            },
        )
        return 1 if failed else 0

    if failed:
        print("Skipping link because compile/resource failures remain.", file=sys.stderr)
        write_summary(paths, results, dry_run=False, config=config)
        return 1

    link_result = run_command(link_command)
    results.append(link_result)
    print_result(link_result)
    if link_result.returncode != 0:
        write_summary(paths, results, dry_run=False, config=config)
        return link_result.returncode
    if config.library_profile:
        log_text = "\n".join(
            path.read_text(encoding="utf-8", errors="replace")
            for path in (link_result.stdout_log, link_result.stderr_log)
            if path.is_file()
        ).lower()
        canonical_lib_root = str(config.canonical_mfc.lib_root.resolve()).lower() if config.canonical_mfc else ""
        if canonical_lib_root and canonical_lib_root in log_text:
            print("diagnostic library profile mixed canonical MFC libraries", file=sys.stderr)
            write_summary(paths, results, dry_run=False, config=config)
            return 1

    if linkability_only:
        write_summary(
            paths,
            results,
            dry_run=False,
            config=config,
            acceptance={
                "report_version": 1,
                "kind": "final-build-diagnostic",
                "diagnostic_kind": "whole-program-linkability",
                "success": True,
                "validation_mode": "live",
                "fresh_build": True,
                "reuse": False,
                "binary": expected_binary,
                "compiler_profile": "VC5SP3",
                "compile_profiles": compile_profile_rows(config),
                "config_path": str(config.manifest_path.resolve()),
                "compiler_env_path": str(config.vc5_env.resolve()),
                "canonical_mfc_include_trace": canonical_include_trace,
                "build_root": str(paths.build_dir.resolve()),
                "map_path": str(paths.map_path.resolve()),
                "resource_path": str(paths.resource_path.resolve()),
                "candidate_path": str(paths.exe_path.resolve()),
                "required_order_targets": [],
                "effective_order_targets": [],
                "linked_order_evaluation_suppressed": True,
                "playtest_deployment_suppressed": True,
                "playtest_deploy": {
                    "attempted": False,
                    "updated": False,
                    "destination": (
                        str(config.playtest_output_exe.resolve())
                        if config.playtest_output_exe is not None
                        else None
                    ),
                    "error": None,
                    "suppression_reason": "whole-program-linkability",
                },
                "candidate_expected_truth": False,
                "accepts_linked_order": False,
                "accepts_bytes": False,
                "accepts_final_image": False,
                "diagnostic_only": True,
                "final_image_validation": "not-run",
                "selected_diagnostic_targets_passed": False,
            },
        )
        print(f"Whole-program linkability candidate: {paths.exe_path}")
        print(f"Map file: {paths.map_path}")
        return 0

    if playground_only:
        return finish_playground_build(
            config, paths, results, progress_path=progress_path,
            required_order_targets=required_order_targets,
            canonical_include_trace=canonical_include_trace,
        )

    order_report_dir.mkdir(parents=True, exist_ok=True)
    order_rc = run_linked_order_targets(
        target_names=effective_order_targets,
        target_files=order_target_files,
        map_path=paths.map_path,
        report_dir=order_report_dir,
        order_scope=order_scope,
        config=config,
        progress_path=progress_path,
    )
    if linked_order_only:
        semantic_reports: list[dict[str, object]] = []
        for target in order_target_manifests:
            for interval in target.linked_function_intervals:
                scoped_name = (
                    f"{target.name}_{interval.name}_authored"
                    if order_scope == "authored"
                    else f"{target.name}_{interval.name}"
                )
                safe_name = re.sub(r"[^A-Za-z0-9_.-]+", "_", scoped_name)
                report_path = order_report_dir / f"linked_order_{safe_name}.json"
                semantic_reports.append({
                    "target": target.name,
                    "interval": interval.name,
                    "path": str(report_path.resolve()),
                })
        write_summary(
            paths,
            results,
            dry_run=False,
            acceptance={
                "report_version": 1,
                "kind": "linked-function-order-run",
                "success": order_rc == 0,
                "binary": expected_binary,
                "order_scope": order_scope,
                "order_reports": semantic_reports,
                "diagnostic_only": True,
            },
        )
        return order_rc
    if order_rc != 0:
        order_reports = order_report_rows(
            effective_order_targets,
            (),
            order_report_dir,
            order_scope=order_scope,
        )
        isolation_fields = (
            diagnostic_isolation_summary_fields(
                routing=routing,
                order_reports=order_reports,
                selected_targets_passed=False,
            )
            if routing.diagnostic_isolation_applied
            else normal_order_summary_fields(
                routing=routing,
                order_reports=order_reports,
                order_scope=order_scope,
            )
        )
        write_summary(
            paths,
            results,
            dry_run=False,
            config=config,
            acceptance={
                "report_version": 1,
                "kind": "final-build",
                "success": False,
                "failure_stage": "linked-order",
                "binary": expected_binary,
                "compile_profiles": compile_profile_rows(config),
                "map_path": str(paths.map_path.resolve()),
                "required_order_targets": list(required_order_targets),
                "effective_order_targets": list(effective_order_targets),
                "order_scope": order_scope,
                "order_reports": order_reports,
                "required_order_targets_passed": False,
                "diagnostic_only": True,
                **isolation_fields,
            },
        )
        return order_rc
    order_reports = order_report_rows(
        effective_order_targets,
        (),
        order_report_dir,
        order_scope=order_scope,
    )
    isolation_fields = (
        diagnostic_isolation_summary_fields(
            routing=routing,
            order_reports=order_reports,
            selected_targets_passed=order_rc == 0,
        )
        if routing.diagnostic_isolation_applied
        else normal_order_summary_fields(
            routing=routing,
            order_reports=order_reports,
            order_scope=order_scope,
        )
    )
    linked_presence = None
    if config.manifest_path.resolve() == DEFAULT_MANIFEST.resolve() and not routing.diagnostic_isolation_applied:
        linked_presence = required_authored_presence_at_map(paths.map_path, progress_path)
        if not linked_presence["passed"]:
            write_summary(
                paths, results, dry_run=False, config=config,
                acceptance={
                    "report_version": 1, "kind": "final-build", "success": False,
                    "failure_stage": "linked-presence", "binary": expected_binary,
                    "linked_presence": linked_presence, "playtest_deployed": False,
                    "order_reports": order_reports, "diagnostic_only": True,
                    **isolation_fields,
                },
            )
            print("Required authored linked presence failed; playground executable was not replaced.")
            print(json.dumps(linked_presence, indent=2))
            return 1
    playtest_deploy = None
    if playtest_deployment_eligible(
        config,
        dry_run=dry_run,
        compile_only=compile_only,
        linked_order_only=linked_order_only,
        diagnostic_isolation_applied=routing.diagnostic_isolation_applied,
    ):
        playtest_deploy = deploy_playtest_candidate(
            paths.exe_path,
            config.playtest_output_exe,
        )
    playtest_summary = (
        {"playtest_deploy": playtest_deploy}
        if playtest_deploy is not None
        else {}
    )
    write_summary(
        paths,
        results,
        dry_run=False,
        config=config,
        acceptance={
            "report_version": 1,
            "kind": "final-build",
            "success": not failed,
            "binary": expected_binary,
            "compiler_profile": "VC5SP3",
            "compile_profiles": compile_profile_rows(config),
            "config_path": str(config.manifest_path.resolve()),
            "compiler_env_path": str(config.vc5_env.resolve()),
            "canonical_mfc_include_trace": canonical_include_trace,
            "map_path": str(paths.map_path.resolve()),
            "resource_path": str(paths.resource_path.resolve()),
            "candidate_path": str(paths.exe_path.resolve()),
            "required_order_targets": list(required_order_targets),
            "effective_order_targets": list(effective_order_targets),
            "order_scope": order_scope,
            "order_reports": order_reports,
            "required_order_targets_passed": order_rc == 0,
            "final_image_validation": "deferred-to-verify-final-image",
            "linked_presence": linked_presence,
            "diagnostic_only": config.diagnostic_only,
            **playtest_summary,
            **isolation_fields,
        },
    )
    print(f"Candidate binary: {paths.exe_path}")
    print(f"Map file: {paths.map_path}")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Freshly compile and link a VC5SP3 candidate for live order, byte, and final-image validation."
        )
    )
    parser.add_argument("--manifest", default=str(DEFAULT_MANIFEST))
    parser.add_argument("--progress", type=Path, default=DEFAULT_PROGRESS)
    parser.add_argument("--dry-run", action="store_true", help="Generate response files and print commands only.")
    parser.add_argument("--compile-only", action="store_true", help="Compile objects and resources, then stop.")
    parser.add_argument(
        "--compile-only-skip-linked-order",
        action="store_true",
        help=(
            "With --compile-only, suppress required/selected linked-order target evaluation for a diagnostic "
            "compile sweep; produces no order evidence and cannot satisfy acceptance gates."
        ),
    )
    parser.add_argument("--keep-going", action="store_true", help="Keep compiling after source failures.")
    parser.add_argument(
        "--build-dir",
        type=Path,
        help=(
            "Use one repository-relative isolated root below build/ for every generated object, resource, "
            "log, response file, map, executable, order report, and summary; the root must not overlap "
            "the manifest's canonical build directory."
        ),
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        help="Remove exactly the selected validated build directory before building.",
    )
    parser.add_argument(
        "--link-profile",
        help="Run a named diagnostic_link_profiles entry in an isolated build/vc5-probes root without changing the manifest.",
    )
    parser.add_argument("--source-profile", action="append", default=[], metavar="SOURCE=PROFILE")
    parser.add_argument("--compile-profile", help="Apply one reviewed full-build-safe compiler profile to every source.")
    parser.add_argument("--library-profile", help="Use one diagnostic matched provider-library profile.")
    parser.add_argument(
        "--order-scope",
        choices=("authored", "full", "auto"),
        default="auto",
        help="Validate authored-first or exact full linked order; auto follows the active tracker phase.",
    )
    parser.add_argument(
        "--linked-order-only",
        action="store_true",
        help=(
            "Compile and link current code, run exactly one selected authored/full linked-order "
            "comparison, and stop before resource, PE, or byte gates."
        ),
    )
    parser.add_argument(
        "--linkability-only",
        action="store_true",
        help=(
            "Run one fresh canonical whole-program compile/resource/link diagnostic in an "
            "explicit isolated build directory, without linked-order evaluation or play-test deployment."
        ),
    )
    parser.add_argument(
        "--playground-only",
        action="store_true",
        help=(
            "Freshly compile/link the canonical Recoil program and deploy only after complete authored "
            "linked-presence verification. Requires an absent build/live-validation root; accepts no "
            "linked-order, byte, alias, or final-image facts."
        ),
    )
    parser.add_argument(
        "--order-target",
        action="append",
        default=[],
        metavar="VC5-MANIFEST-NAME",
        help="Validate linked_function_intervals from a VC5 verification manifest after linking; repeatable.",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    raw_argv = list(sys.argv[1:] if argv is None else argv)
    if raw_argv[:1] == ["--"]:
        raw_argv = raw_argv[1:]
    args = parser.parse_args(raw_argv)
    try:
        config = load_config(args.manifest)
        config = with_diagnostic_compile_profiles(
            config,
            compile_profile=args.compile_profile or "",
            source_profiles=tuple(args.source_profile),
        )
        if args.library_profile:
            config = with_diagnostic_library_profile(config, args.library_profile)
        if args.link_profile:
            config = with_diagnostic_link_profile(config, args.link_profile)
        config = finalize_diagnostic_build_dir(config)
        if args.build_dir is not None:
            config = with_explicit_build_dir(config, args.build_dir)
        order_scope = args.order_scope
        if order_scope == "auto":
            if config.manifest_path.resolve() == DEFAULT_MANIFEST.resolve():
                phase = ProgressDocument.load(DEFAULT_PROGRESS).pipeline("recoil").get("phase")
                order_scope = (
                    "authored"
                    if phase in {"authored-function-order", "authored-byte-match"}
                    else "full"
                )
            else:
                order_scope = "full"
        if args.linked_order_only and args.order_scope == "auto":
            parser.error("--linked-order-only requires explicit --order-scope authored or full")
        return run_build(
            config,
            clean=args.clean,
            dry_run=args.dry_run,
            compile_only=args.compile_only,
            keep_going=args.keep_going,
            order_targets=tuple(args.order_target),
            order_scope=order_scope,
            compile_only_skip_linked_order=args.compile_only_skip_linked_order,
            linked_order_only=args.linked_order_only,
            linkability_only=args.linkability_only,
            playground_only=args.playground_only,
            required_order_targets_override=(
                () if args.linked_order_only or args.linkability_only else None
            ),
            progress_path=args.progress,
        )
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(str(exc), file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
